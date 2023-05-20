/**
 * Runs pre-process step for modules
 *
 * A folder named "Intermediate" will be created in the source code directory of
 * the module. As an example:
 *
 * - Root
 *     - Modules
 *         - MyModule
 *             - MyModule.h
 *             - MySystem.h
 *             - MySystem.cpp
 *             - ...
 *             - Intermediate <- This is the generated folder
 *                 - MySystem.generated.h
 *                 - GeneratedFiles.txt <- The files that were generated
 *                 - Module.generated.cpp
 *                 - Module.generated.h
 *
 */
#include "PreprocessAction.h"

#include <Metadesk/metadesk.h>

#include "Arg.h"
#include "Compat/Metadesk.h"
#include "Doll.h"
#include "FileSystem/DirectoryIterator.h"
#include "FileSystem/Extras.h"
#include "Parsing.h"
#include "Str.h"
#include "Traits.h"

struct PreprocessContext {
    Arena<ArenaMode::Dynamic>       arena;
    TArray<MetaSystemDescriptor>    systems;
    TArray<MetaComponentDescriptor> components;
    TArray<MetaHook>                hooks;
    TArray<Str>                     folder_context;
    TArray<Str>                     files_generated;
    TArray<Str>                     module_includes;
    Str                             module_name;

    void init()
    {
        arena = Arena<ArenaMode::Dynamic>(System_Allocator, MEGABYTES(1));
        arena.init();
        systems         = TArray<MetaSystemDescriptor>(&System_Allocator);
        components      = TArray<MetaComponentDescriptor>(&System_Allocator);
        hooks           = TArray<MetaHook>(&System_Allocator);
        folder_context  = TArray<Str>(&System_Allocator);
        files_generated = TArray<Str>(&System_Allocator);
        module_includes = TArray<Str>(&System_Allocator);
    }
};

static PreprocessContext G;

/** Push folder onto the stack */
static void push_folder_context(Str folder)
{
    G.folder_context.add(folder.clone(System_Allocator));
}

/** Pop folder from the stack */
static void pop_folder_context()
{
    ASSERT(G.folder_context.size > 0);

    Str* last = G.folder_context.last();
    System_Allocator.release((umm)last->data);
    G.folder_context.pop();
}

/** Concatenate all the folders in the stack into a single path */
static Str get_folder_context(Allocator& allocator)
{
    if (G.folder_context.size == 0) return Str::NullStr;

    Str current = G.folder_context[0].clone(allocator);
    for (u64 i = 1; i < G.folder_context.size; ++i) {
        current = format(allocator, LIT("{}/{}"), current, G.folder_context[i]);
        print(LIT("{} = {}\n"), i, G.folder_context[i]);
    }

    return current;
}

/**
 * Add the include.
 * This will generate a #include directive in the Module.cpp files
 */
static void add_module_include(Str filename)
{
    SAVE_ARENA(G.arena);
    Str folder = get_folder_context(G.arena);
    Str include;

    if (folder == Str::NullStr) {
        G.module_includes.add(filename.clone(System_Allocator));
    } else {
        Str include = format(G.arena, LIT("{}/{}"), folder, filename);
        G.module_includes.add(include.clone(System_Allocator));
    }
}

static void parse_folder_recursive(Str from, Str to);
static bool parse_file(Str from_path, Str to_path);
static void parse_system(WriteTape& out, MD_Node* node);
static void parse_component(WriteTape& out, MD_Node* node);
static void parse_component_property(
    MetaComponentDescriptor& desc, MD_Node* node);
static void write_component(WriteTape& out, MetaComponentDescriptor& component);
static void parse_term_source(MD_Node* node, MetaTermID& source);
static void parse_hook(WriteTape& out, MD_Node* node);
static MD_Node* find_tag(MD_Node* node, Str name);
static MD_Node* find_child(MD_Node* node, Str name);
static void     write_module(Str directory);

static int doll_preprocess(Slice<Str> args)
{
    G.init();
    ArgCollection arg_collection(G.arena);

    arg_collection.register_arg<Str>(
        LIT("path"),
        Str::NullStr,
        LIT("[REQUIRED] The path to the source code of the module that will be "
            "processed"));
    arg_collection.register_arg<Str>(
        LIT("name"),
        Str::NullStr,
        LIT("[OPTIONAL] The name of the module"));

    if (!arg_collection.parse_args(args)) {
        print(LIT("Invalid argument format. Printing summary...\n"));
        arg_collection.summary();
        return -1;
    }

    Str path_to_parse = *arg_collection.get_arg<Str>(LIT("path"));
    if (path_to_parse == Str::NullStr) {
        print(LIT("Argument '-path' must be specified.\n"));
        arg_collection.summary();
    }

    Str from = to_absolute_path(path_to_parse, G.arena);
    Str path_to_generate =
        format(G.arena, LIT("{}/Intermediate/"), FmtPath(from));

    G.module_name = get_file_name(path_to_parse).clone(System_Allocator);
    print(LIT("Module name: {}\n"), G.module_name);

    print(LIT("Processing directory: {} -> {}\n"), from, path_to_generate);

    create_dir(path_to_generate);
    parse_folder_recursive(from, path_to_generate);
    write_module(path_to_generate);

    return 0;
}

static void parse_folder_recursive(Str from, Str to)
{
    DirectoryIterator it = open_dir(from);
    DEFER(it.close());

    FileData it_data;
    while (it.next_file(&it_data)) {
        if (it_data.filename == LIT(".")) continue;
        if (it_data.filename == LIT("..")) continue;
        if (it_data.filename == LIT("Intermediate")) continue;

        SAVE_ARENA(G.arena);

        if (it_data.attributes == FileAttributes::File) {
            Str extension = it_data.filename.chop_left_last_of('.');
            if (extension != LIT(".h")) {
                continue;
            }

            Str file_part = it_data.filename.chop_right_last_of(LIT("."));

            Str from_path =
                format(G.arena, LIT("{}/{}"), from, it_data.filename);
            Str to_path =
                format(G.arena, LIT("{}/{}.generated.h"), to, file_part);

            if (parse_file(from_path, to_path)) {
                add_module_include(it_data.filename);
                G.files_generated.add(to_path.clone(System_Allocator));
            }

        } else {
            Str new_from =
                format(G.arena, LIT("{}/{}/"), from, it_data.filename);
            Str new_directory =
                format(G.arena, LIT("{}/{}/"), to, it_data.filename);
            create_dir(new_directory);

            print(LIT("{} -> {}\n"), new_from, new_directory);

            push_folder_context(it_data.filename);
            parse_folder_recursive(new_from, new_directory);
            pop_folder_context();
        }
    }
}

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

    const bool has_metadesk_code =
        (start_index != end_index) && (start_index + end_index) > 0;

    if (!has_metadesk_code) return false;

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
        MD_PrintMessage(stderr, code_location, message->kind, message->string);
    }

    BufferedWriteTape<true> out(open_file_write(to_path));
    format(&out, LIT("// Metacompiler code-gen\n"));
    format(&out, LIT("// clang-format off\n"));
    format(&out, LIT("#include \"Reflection.h\"\n"));

    for (MD_EachNode(child, result.node->first_child)) {
        if (MD_NodeHasTag(child, MD_S8Lit("system"), 0)) {
            parse_system(out, child);
        } else if (MD_NodeHasTag(child, MD_S8Lit("component"), 0)) {
            parse_component(out, child);
        } else if (MD_NodeHasTag(child, MD_S8Lit("hook"), 0)) {
            parse_hook(out, child);
        }
    }
    format(&out, LIT("// clang-format on\n"));

    return true;
}

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

static void parse_component(WriteTape& out, MD_Node* node)
{
    MetaComponentDescriptor result;
    result.properties.alloc = &System_Allocator;
    result.name             = mds8_to_str(node->string).clone(System_Allocator);
    result.flags            = 0;

    {
        MD_Node* tag = find_tag(node, LIT("component"));
        ASSERT(tag);

        if (find_child(tag, LIT("nodefine"))) {
            result.flags |= MetaComponentFlag::NoDefine;
        }

        if (find_child(tag, LIT("nodescriptor"))) {
            result.flags |= MetaComponentFlag::NoDescriptor;
        }
    }

    for (MD_EachNode(it, node->first_child)) {
        parse_component_property(result, it);
    }

    G.components.add(result);

    write_component(out, result);
}

static void parse_hook(WriteTape& out, MD_Node* node)
{
    MD_Node* hook_tag = find_tag(node, LIT("hook"));
    Str      hook_name =
        mds8_to_str(hook_tag->first_child->string).clone(System_Allocator);
    Str function = mds8_to_str(node->string).clone(System_Allocator);

    format(&out, LIT("// Hook {} -> {}\n"), hook_name, function);
    format(&out, LIT("void {}();\n"), function);

    G.hooks.add(MetaHook{
        .hook     = hook_name,
        .function = function,
    });
}

static void write_component_idescriptor(
    WriteTape& out, MetaComponentDescriptor& component)
{
    format(&out, LIT("// IDescriptor for {}\n"), component.name);

    format(&out, LIT("struct "));
    component.format_descriptor_name(out);
    format(&out, LIT(" : IDescriptor {\n"));

    int subdescriptor_count = 0;

    for (const MetaComponentProperty& property : component.properties) {
        format(&out, LIT("\t"));

        property.format_desc_type(out);
        format(&out, LIT(" "));
        property.format_desc_name(out);

        format(&out, LIT(" = {"));
        format(
            &out,
            LIT("OFFSET_OF({}, {}), LIT(\"{}\")"),
            component.name,
            property.name,
            property.name);
        format(&out, LIT("};\n"));

        subdescriptor_count++;
    }

    // Write subdescriptor array
    format(&out, LIT("\n"));
    format(&out, LIT("\tIDescriptor* descs[{}] = {\n"), subdescriptor_count);

    for (const MetaComponentProperty& property : component.properties) {
        format(&out, LIT("\t\t&"));
        property.format_desc_name(out);
        format(&out, LIT(",\n"));
    }

    format(&out, LIT("\t};"));
    format(&out, LIT("\n"));

    // Implement IDescriptor

    // >> Constructor
    format(&out, LIT("\tCUSTOM_DESC_DEFAULT("));
    component.format_descriptor_name(out);
    format(&out, LIT(")\n"));

    // >> type_name()
    format(
        &out,
        LIT("\tvirtual Str type_name() const override { return "
            "LIT(\"{}\"); }\n"),
        component.name);

    // >> subdescriptors()
    format(
        &out,
        LIT("\tvirtual Slice<IDescriptor*> subdescriptors(umm self) "
            "override { return Slice<IDescriptor*>(descs, "
            "ARRAY_COUNT(descs)); }\n"));

    format(&out, LIT("};\n"));

    format(&out, LIT("DECLARE_DESCRIPTOR_OF({});\n"), component.name);

    format(&out, LIT("\n"));
}

static void write_component_struct(
    WriteTape& out, MetaComponentDescriptor& component)
{
    format(&out, LIT("struct {} {\n"), component.name);

    for (const MetaComponentProperty& property : component.properties) {
        format(&out, LIT("\t{} {};\n"), property.type.to_str(), property.name);
    }

    format(&out, LIT("};\n"), component.name);
}

static void write_component(WriteTape& out, MetaComponentDescriptor& component)
{
    format(&out, LIT("//\n"));
    format(&out, LIT("// Component: {}\n"), component.name);
    format(&out, LIT("//\n"));

    if (!(component.flags & MetaComponentFlag::NoDefine)) {
        write_component_struct(out, component);
    }

    if (component.has_generated_descriptor()) {
        write_component_idescriptor(out, component);
    }
}

static void parse_component_property(
    MetaComponentDescriptor& desc, MD_Node* node)
{
    MetaComponentProperty property = {};
    property.name = mds8_to_str(node->string).clone(System_Allocator);

    Str base_type = mds8_to_str(node->first_child->string);
    if (base_type == LIT("Array")) {
        Str array_type = mds8_to_str(node->first_child->first_child->string)
                             .clone(System_Allocator);
        Str array_count =
            mds8_to_str(node->first_child->first_child->next->string)
                .clone(System_Allocator);
        MetaArray array = {
            .type  = array_type,
            .count = array_count,
        };

        property.type.value.set(array);
    } else {
        if (is_meta_primitive(base_type)) {
            MetaPrimitive primitive = {
                .type = base_type.clone(System_Allocator),
            };

            property.type.value.set(primitive);
        } else {
            MetaCompound compound = {
                .type = base_type.clone(System_Allocator),
            };
            property.type.value.set(compound);
        }
    }

    desc.properties.add(property);
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

static MD_Node* find_child(MD_Node* node, Str name)
{
    for (MD_EachNode(it, node->first_child)) {
        Str child_name = mds8_to_str(it->string);

        if (name == child_name) {
            return it;
        }
    }

    return nullptr;
}

static void write_component_descriptors_header(WriteTape& out)
{
    for (const MetaComponentDescriptor& component : G.components) {
        // @todo: ComponentDescriptor
    }
}

static void write_component_descriptors_impl(WriteTape& out)
{
    for (const MetaComponentDescriptor& component : G.components) {
        if (component.has_generated_descriptor()) {
            format(&out, LIT("DEFINE_DESCRIPTOR_OF({});\n"), component.name);
        }
    }

    format(&out, LIT("\n"));

    for (const MetaComponentDescriptor& component : G.components) {
        format(&out, LIT("static ComponentDescriptor "));
        component.format_ecs_descriptor_name(out);
        format(&out, LIT(" = {\n"));
        format(&out, LIT("\t.name = LIT(\"{}\"),\n"), component.name);
        format(&out, LIT("\t.size = sizeof({}),\n"), component.name);
        format(
            &out,
            LIT("\t.alignment = static_cast<i64>(alignof({})),\n"),
            component.name);
        format(
            &out,
            LIT("\t.descriptor = descriptor_of<{}>(nullptr),\n"),
            component.name);
        format(&out, LIT("};\n"));
    }
    format(&out, LIT("\n"));
}

static void write_system_descriptors_impl(WriteTape& out)
{
    int i = 0;
    for (const auto& system : G.systems) {
        Str system_id = format(G.arena, LIT("_system_{}"), i);
        Str invoke_id = format(G.arena, LIT("{}_invoke"), system_id);

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
}

static void write_module(Str directory)
{
    SAVE_ARENA(G.arena);
    Str header         = format(G.arena, LIT("{}/Mod.h"), directory);
    Str implementation = format(G.arena, LIT("{}/Mod.cpp"), directory);
    Str genlist        = format(G.arena, LIT("{}/ModFiles.txt"), directory);

    print(LIT("Writing module {}, {}\n"), header, implementation);
    G.files_generated.add(header);
    G.files_generated.add(implementation);

    // Write header
    {
        BufferedWriteTape<true> out(open_file_write(header));

        format(&out, LIT("// Metacompiler code-gen\n"));
        format(&out, LIT("// clang-format off\n"));

        format(&out, LIT("#pragma once\n"));
        format(&out, LIT("#include \"Engine/Module.h\"\n"));

        write_component_descriptors_header(out);

        format(
            &out,
            LIT("extern \"C\" void import_module_{}(ModuleInitParams* "
                "params);\n"),
            G.module_name);

        format(&out, LIT("// clang-format on\n"));
    }

    // Write implementation
    {
        BufferedWriteTape<true> out(open_file_write(implementation));
        format(&out, LIT("// Metacompiler code-gen\n"));
        format(&out, LIT("// clang-format off\n"));
        format(&out, LIT("\n"));
        format(&out, LIT("#include \"Mod.h\"\n"));
        format(&out, LIT("#include \"Engine/Engine.h\"\n"));
        for (const Str& include : G.module_includes) {
            format(&out, LIT("#include \"{}\"\n"), include);
        }

        format(&out, LIT("//\n"));
        format(&out, LIT("// COMPONENTS\n"));
        format(&out, LIT("//\n"));
        write_component_descriptors_impl(out);

        format(&out, LIT("//\n"));
        format(&out, LIT("// SYSTEMS\n"));
        format(&out, LIT("//\n"));
        write_system_descriptors_impl(out);

        // Write import_module_<ModuleName>

        format(
            &out,
            LIT("void import_module_{}(ModuleInitParams* params) {\n"),
            G.module_name);

        // Set engine instance ptr
        format(&out, LIT("\tEngine::set_instance(params->engine_instance);\n"));

        for (const MetaComponentDescriptor& component : G.components) {
            format(&out, LIT("\tparams->components->add("));
            component.format_ecs_descriptor_name(out);
            format(&out, LIT(");\n"));
        }

        int i = 0;
        for (const MetaSystemDescriptor& system : G.systems) {
            format(&out, LIT("\tparams->systems->add(&_system_{}_desc);\n"), i);
            i++;
        }

        for (const MetaHook& hook : G.hooks) {
            if (hook.hook == LIT("init")) {
                format(&out, LIT("\t{}();\n"), hook.function);
            }
        }

        format(&out, LIT("}\n"));

        format(&out, LIT("// clang-format on\n"));
    }

    // Write ModFiles.txt
    {
        BufferedWriteTape<true> out(open_file_write(genlist));
        for (Str file_path : G.files_generated) {
            format(&out, LIT("{}\n"), FmtPath(file_path));
        }
    }
}

DOLL_DECLARE_ACTION("preprocess", doll_preprocess);