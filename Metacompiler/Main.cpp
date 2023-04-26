#include <Metadesk/metadesk.h>

#include "Base.h"
#include "Compat/Metadesk.h"
#include "Containers/Array.h"
#include "Containers/Slice.h"
#include "FileSystem/DirectoryIterator.h"
#include "FileSystem/Extras.h"
#include "FileSystem/FileSystem.h"
#include "Internal.h"
#include "Parsing.h"
#include "Str.h"
#include "Traits.h"
/*
EXAMPLE

#if METADESK

@system(OnUpdate)
move_and_update: {
    @op(in)
    transform: TransformComponent,

    @op(out)
    rotation: RotationComponent,
}

#endif

*/

struct {
    Arena                        allocator;
    TArray<MetaSystemDescriptor> systems{&System_Allocator};

    TArray<Str> folder_context{&System_Allocator};
    TArray<Str> module_includes{&System_Allocator};
} G;

static void push_folder_context(Str folder)
{
    G.folder_context.add(folder.clone(System_Allocator));
}

static void pop_folder_context()
{
    ASSERT(G.folder_context.size > 0);

    Str* last = G.folder_context.last();
    System_Allocator.release((umm)last->data);
    G.folder_context.pop();
}

static Str get_folder_context(IAllocator& allocator)
{
    if (G.folder_context.size == 0) return Str::NullStr;

    print(LIT("{} = {}\n"), 0, G.folder_context[0]);
    Str current = G.folder_context[0].clone(allocator);
    for (u64 i = 1; i < G.folder_context.size; ++i) {
        current = format(allocator, LIT("{}/{}"), current, G.folder_context[i]);
        print(LIT("{} = {}\n"), i, G.folder_context[i]);
    }

    return current;
}

static void add_module_include(Str filename)
{
    SAVE_ARENA(G.allocator);
    Str folder = get_folder_context(G.allocator);
    Str include;

    if (folder == Str::NullStr) {
        G.module_includes.add(filename.clone(System_Allocator));

    } else {
        Str include = format(G.allocator, LIT("{}/{}"), folder, filename);
        G.module_includes.add(include.clone(System_Allocator));
    }
}

static bool parse_file(Str from_path, Str to_path);
static void write_module(Str target);

static void parse_and_generate(Str from, Str to)
{
    DirectoryIterator it = open_dir(from);
    DEFER(it.close());

    FileData it_data;
    while (it.next_file(&it_data)) {
        if (it_data.filename == LIT(".")) continue;
        if (it_data.filename == LIT("..")) continue;

        if (it_data.attributes == FileAttributes::File) {
            Str extension = it_data.filename.chop_left_last_of('.');
            if (extension != LIT(".h")) {
                continue;
            }

            Str file_part = it_data.filename.chop_right_last_of(LIT("."));

            SAVE_ARENA(G.allocator);
            Str from_path =
                format(G.allocator, LIT("{}/{}"), from, it_data.filename);
            Str to_path =
                format(G.allocator, LIT("{}/{}.generated.h"), to, file_part);

            if (parse_file(from_path, to_path)) {
                add_module_include(it_data.filename);
            }
        } else {
            SAVE_ARENA(G.allocator);

            Str new_from =
                format(G.allocator, LIT("{}/{}/"), from, it_data.filename);
            Str new_directory =
                format(G.allocator, LIT("{}/{}/"), to, it_data.filename);
            create_dir(new_directory);

            push_folder_context(it_data.filename);
            parse_and_generate(new_from, new_directory);
            pop_folder_context();
        }
    }
}

static void parse_and_generate(Str from, Str to, const Slice<Str>& filter)
{
    DirectoryIterator it = open_dir(from);
    DEFER(it.close());

    FileData it_data;
    while (it.next_file(&it_data)) {
        if (it_data.filename == LIT(".")) continue;
        if (it_data.filename == LIT("..")) continue;
        if (it_data.attributes != FileAttributes::Directory) {
            continue;
        }

        if (it_data.filename != LIT("Engine")) continue;

        SAVE_ARENA(G.allocator);

        Str new_from =
            format(G.allocator, LIT("{}/{}/"), from, it_data.filename);
        Str new_directory =
            format(G.allocator, LIT("{}/{}/"), to, it_data.filename);
        print(LIT("{} -> {}\n"), new_from, new_directory);
        create_dir(new_directory);

        parse_and_generate(new_from, new_directory);

        write_module(new_directory);
    }
}

int main(int argc, char* argv[])
{
    G.allocator = Arena(&System_Allocator, MEGABYTES(1));

    Str current_directory = get_cwd(G.allocator);
    Str parse_directory   = format(G.allocator, LIT("{}/"), current_directory);

    Str intermediate_directory =
        format(G.allocator, LIT("{}/Intermediate"), current_directory);
    print(LIT("Starting at directory: {}\n"), parse_directory);
    print(LIT("Creating intermediate directory: {}\n"), intermediate_directory);
    create_dir(intermediate_directory);

    auto filter = arr<Str>(LIT("Engine"));

    parse_and_generate(parse_directory, intermediate_directory, slice(filter));

    return 0;
}

static void     parse_system(WriteTape& out, MD_Node* node);
static MD_Node* find_tag(MD_Node* node, Str name);

static bool parse_file(Str from_path, Str to_path)
{
    Raw file_contents_raw = dump_file(from_path, System_Allocator);
    DEFER(System_Allocator.release((umm)file_contents_raw.buffer));

    Str file((char*)file_contents_raw.buffer, file_contents_raw.size);

    u64 index       = 0;
    u64 start_index = NumProps<u64>::max;
    u64 end_index   = NumProps<u64>::max;

    while (index < file.len) {
        Str this_string = file.chop_left(index - 1);

        if (this_string.starts_with(LIT("#if METADESK"))) {
            start_index = eat_line(file, index);
            index       = start_index;

            while (index < file.len) {
                Str end_str = file.chop_left(index - 1);
                if (end_str.starts_with(LIT("#endif"))) {
                    end_index = index;
                    break;
                }

                index = eat_line(file, index);
            }

            index = eat_line(file, index);
        } else {
            index = eat_line(file, index);
        }
    }

    if ((start_index != end_index) && (start_index + end_index) > 0) {
        Str metadesk_code = file.chop_middle(start_index, end_index);

        // print(LIT("For file {} -> {}, metadesk code is\n"), from_path,
        // to_path); print(metadesk_code);

        MD_Arena* md_arena = MD_ArenaAlloc();
        DEFER(MD_ArenaRelease(md_arena));
        MD_String8     md_code = str_to_mds8(metadesk_code);
        MD_ParseResult result =
            MD_ParseWholeString(md_arena, MD_S8Lit("File"), md_code);

        for (MD_Message* message = result.errors.first; message != 0;
             message             = message->next)
        {
            MD_CodeLoc code_location = MD_CodeLocFromNode(message->node);
            MD_PrintMessage(
                stderr,
                code_location,
                message->kind,
                message->string);
        }

        BufferedWriteTape<true> out(open_file_write(to_path));
        format(&out, LIT("// Metacompiler code-gen\n"));
        format(&out, LIT("// clang-format off\n"));

        for (MD_EachNode(child, result.node->first_child)) {
            if (MD_NodeHasTag(child, MD_S8Lit("system"), 0)) {
                parse_system(out, child);
            }
        }
        format(&out, LIT("// clang-format on\n"));

        return true;
    }

    return false;
}

static void parse_term_source(MD_Node* node, MetaTermID& source);

static void parse_system(WriteTape& out, MD_Node* node)
{
    MetaSystemDescriptor result;
    result.terms.alloc = &System_Allocator;

    result.name = mds8_to_str(node->string).clone(System_Allocator);

    {
        MD_Node* tag = find_tag(node, LIT("system"));
        ASSERT(tag);

        result.phase =
            mds8_to_str(tag->first_child->string).clone(System_Allocator);
    }

    for (MD_EachNode(it, node->first_child)) {
        MetaSystemDescriptorTerm term;
        term.component_id = mds8_to_str(it->string).clone(System_Allocator);
        term.component_type =
            mds8_to_str(it->first_child->string).clone(System_Allocator);

        if (auto* tag = find_tag(it, LIT("access"))) {
            Str access = mds8_to_str(tag->first_child->string);

            RawReadTape tape(access);
            parse(&tape, term.access);
        }

        if (auto* tag = find_tag(it, LIT("op"))) {
            Str op = mds8_to_str(tag->first_child->string);

            RawReadTape tape(op);
            parse(&tape, term.op);
        }

        if (auto* tag = find_tag(it, LIT("source"))) {
            parse_term_source(tag, term.source);
        }

        result.terms.add(term);
    }

    // Print the function declaration
    {
        format(&out, LIT("extern void {}"), result.name);
        format(&out, LIT("("));

        format(&out, LIT("flecs::entity entity, "));
        for (u64 i = 0; i < result.terms.size; ++i) {
            const MetaSystemDescriptorTerm& term = result.terms[i];
            format(
                &out,
                LIT("{}{}{} {}"),
                term.is_const() ? LIT("const ") : Str::NullStr,
                term.component_type,
                term.is_pointer() ? LIT("*") : LIT("&"),
                term.component_id);

            if (i != (result.terms.size - 1)) {
                format(&out, LIT(", "));
            }
        }

        format(&out, LIT(");\n"));
    }

    G.systems.add(result);
}

static void parse_term_source(MD_Node* node, MetaTermID& source)
{
    for (MD_EachNode(it, node->first_child)) {
        Str id = mds8_to_str(it->string);

        if (id == LIT("id")) {
            Str value = mds8_to_str(it->first_child->string);
            if (value == LIT("this")) {
                source.entity = LIT("EcsThis");
            } else {
                ASSERT(false && "NA/Not Implemented");
            }
        }

        if (id == LIT("trav")) {
            Str value                = mds8_to_str(it->first_child->string);
            source.relationship.name = value.clone(System_Allocator);
        }

        if (id == LIT("flags")) {
            MetaTermIDFlags flags;
            for (MD_EachNode(flag_it, it->first_child)) {
                Str flag = mds8_to_str(flag_it->string);
                // clang-format off
                if (flag == LIT("self"))        flags.f |= MetaTermIDFlags::Self;
                if (flag == LIT("up"))          flags.f |= MetaTermIDFlags::Up;
                if (flag == LIT("cascade"))     flags.f |= MetaTermIDFlags::Cascade;
                if (flag == LIT("is_variable")) flags.f |= MetaTermIDFlags::IsVariable;
                if (flag == LIT("is_entity"))   flags.f |= MetaTermIDFlags::IsEntity;
                if (flag == LIT("is_name"))     flags.f |= MetaTermIDFlags::IsName;
                if (flag == LIT("filter"))      flags.f |= MetaTermIDFlags::Filter;
                // clang-format on
            }
            source.flags = flags;
        }
    }
}

static MD_Node* find_tag(MD_Node* node, Str name)
{
    for (MD_EachNode(it, node->first_tag)) {
        Str tag_name = mds8_to_str(it->string);

        if (name == tag_name) {
            return it;
        }
    }
    return 0;
}

static void write_module_header(Str path);
static void write_module_implementation(Str path);

static void write_module(Str target)
{
    SAVE_ARENA(G.allocator);
    Str header         = format(G.allocator, LIT("{}/Module.h"), target);
    Str implementation = format(G.allocator, LIT("{}/Module.cpp"), target);

    print(LIT("Writing module {}, {}\n"), header, implementation);

    write_module_header(header);
    write_module_implementation(implementation);
}

static void write_module_header(Str path)
{
    BufferedWriteTape<true> out(open_file_write(path));

    format(&out, LIT("// Metacompiler code-gen\n"));
    format(&out, LIT("// clang-format off\n"));

    format(&out, LIT("#pragma once\n"));
    format(&out, LIT("#include \"SystemDescriptor.h\"\n"));
    format(
        &out,
        LIT("extern void import_module_{}(SystemDescriptorRegistrar* reg);\n"),
        LIT("Engine"));

    format(&out, LIT("// clang-format on\n"));
}

static void write_module_implementation(Str path)
{
    BufferedWriteTape<true> out(open_file_write(path));
    format(&out, LIT("// Metacompiler code-gen\n"));
    format(&out, LIT("// clang-format off\n"));
    format(&out, LIT("\n"));
    format(&out, LIT("#include \"Module.h\"\n"));

    for (const Str& include : G.module_includes) {
        format(&out, LIT("#include \"{}\"\n"), include);
    }

    int i = 0;
    for (const auto& system : G.systems) {
        Str system_id = format(G.allocator, LIT("_system_{}"), i);
        Str invoke_id = format(G.allocator, LIT("{}_invoke"), system_id);

        format(&out, LIT("// System {}\n"), system.name);
        format(&out, LIT("static void {}(ecs_iter_t* it)\n"), invoke_id);
        format(&out, LIT("{\n"), invoke_id);

        // Get Fields
        for (int term_index = 0; term_index < system.terms.size; ++term_index) {
            const MetaSystemDescriptorTerm& term = system.terms[term_index];

            format(
                &out,
                LIT("\t{}* c{} = ecs_field(it, {}, {});\n"),
                term.component_type,
                term_index,
                term.component_type,
                term_index + 1);
        }

        // Invoke actual function
        {
            format(&out, LIT("\tfor (int i = 0; i < it->count; ++i)\n"));
            format(&out, LIT("\t{\n"));
            format(
                &out,
                LIT("\tflecs::entity entity(it->world, it->entities[i]);\n"));

            format(&out, LIT("\t\t{}(\n"), system.name);
            format(&out, LIT("\t\t\tentity,\n"));

            for (int term_index = 0; term_index < system.terms.size;
                 ++term_index)

            {
                const MetaSystemDescriptorTerm& term = system.terms[term_index];

                if (term.is_pointer()) {
                    format(&out, LIT("\t\t\t&c{}[i]"), term_index);
                } else {
                    format(&out, LIT("\t\t\tc{}[i]"), term_index);
                }

                if (term_index != (system.terms.size - 1)) {
                    format(&out, LIT(",\n"));
                }
            }
            format(&out, LIT(");\n"), system.name);

            format(&out, LIT("\t}\n"));
        }

        format(&out, LIT("}\n"), invoke_id);

        format(&out, LIT("static SystemDescriptor {}_desc = {\n"), system_id);

        format(&out, LIT("\t.name = \"{}\",\n"), system.name);
        format(&out, LIT("\t.invoke = {},\n"), invoke_id);

        format(&out, LIT("\t.filter_desc = {\n"));

        // Terms
        {
            format(&out, LIT("\t\t.terms = {\n"));

            for (int term_index = 0; term_index < system.terms.size;
                 ++term_index)
            {
                const MetaSystemDescriptorTerm& term = system.terms[term_index];
                format(&out, LIT("\t\t\t{\n"));

                // Component ID
                // format(
                //     &out,
                //     LIT("\t\t\t\t.id = ecs_id({}),\n"),
                //     term.component_type);

                // Source
                format(&out, LIT("\t\t\t\t.src = {\n"));
                {
                    // ID
                    format(
                        &out,
                        LIT("\t\t\t\t\t.id = {},\n"),
                        term.source.entity);

                    // Name
                    format(&out, LIT("\t\t\t\t\t.name = nullptr,\n"));

                    // Relationship
                    if (term.source.relationship.is_default()) {
                        format(
                            &out,
                            LIT("\t\t\t\t\t.trav = Ecs{},\n"),
                            term.source.relationship.name);
                    } else {
                        format(
                            &out,
                            LIT("\t\t\t\t\t.trav = ecs_id({}),\n"),
                            term.source.relationship.name);
                    }

                    // Flags
                    format(
                        &out,
                        LIT("\t\t\t\t\t.flags = {}, "),
                        term.source.flags.f);
                    term.source.flags.print_ors(out);

                    format(&out, LIT("\n"));
                }
                format(&out, LIT("\t\t\t\t},\n"));

                // First
                format(&out, LIT("\t\t\t\t.first = {\n"));
                {
                    // @todo: need to move this to .first property of term!
                    format(
                        &out,
                        LIT("\t\t\t\t\t.name = (char*)\"{}\",\n"),
                        term.component_type);
                }
                format(&out, LIT("\t\t\t\t},\n"));

                // Inout
                format(&out, LIT("\t\t\t\t.inout = {},\n"), term.access);

                // Operator
                format(&out, LIT("\t\t\t\t.oper = {},\n"), term.op);

                format(&out, LIT("\t\t\t},\n"));
            }

            format(&out, LIT("\t\t},\n"));
        }

        format(&out, LIT("\t},\n"));
        format(&out, LIT("\t.phase = Ecs{},\n"), system.phase);

        format(&out, LIT("};\n"));

        format(&out, LIT("\n"));

        ++i;
    }

    format(
        &out,
        LIT("void import_module_{}(SystemDescriptorRegistrar* reg)\n"),
        LIT("Engine"));

    format(&out, LIT("{\n"));
    i = 0;
    for (const MetaSystemDescriptor& system : G.systems) {
        format(&out, LIT("\treg->add(&_system_{}_desc);\n"), i);

        ++i;
    }
    format(&out, LIT("}\n"));

    format(&out, LIT("// clang-format on\n"));
}
