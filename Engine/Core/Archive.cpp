#include "Archive.h"

#include "Tape.h"

/**
 * Archive Format
 *
 * Each archive begins with the following header:
 * [8 name_size, 8 type_size, 8 size, name, type, data]
 *
 * Where:
 *     - name_size: The size of the name designated by the string
 *     - type_size: The size of the type name
 *     - size:      The size (in bytes) of the data
 *     - data:      The data itself
 *
 * Right after the archive entry is the actual data
 */

struct ArchiveHeader {
    u64 magic = 0x3148435241584b56;
};

struct ArchiveEntry {
    u64 name_size;
    u64 type_size;
    u64 size;
};

static bool serialize_entry(WriteTape* out, DescPair pair, u64& size);
static bool serialize_value(WriteTape* out, DescPair pair, u64& size);
static bool serialize_primitive(WriteTape* out, DescPair pair, u64& size);
static bool serialize_string(WriteTape* out, DescPair pair, u64& size);
static bool serialize_array(WriteTape* out, DescPair pair, u64& size);
static bool serialize_object(WriteTape* out, DescPair pair, u64& size);

PROC_SERIALIZE(archive_serialize)
{
    // Write archive header
    ArchiveHeader header;
    out->write(&header, sizeof(header));

    u64 size = 0;
    // Serialize first entry
    return serialize_entry(out, DescPair{desc, ptr}, size);
}

static bool serialize_entry(WriteTape* out, DescPair pair, u64& size)
{
    Str name      = pair.desc->name;
    Str type_name = pair.desc->type_name();

    ArchiveEntry entry = {
        .name_size = name.len,
        .type_size = type_name.len,
        .size      = 0,
    };

    // Write entry & name
    {
        out->write(&entry, sizeof(entry));
        out->write_str(name);
        out->write_str(type_name);
    }

    if (!serialize_value(out, pair, size)) return false;

    i64 total_seek_back = size + entry.type_size + entry.name_size;

    out->seek(-total_seek_back);
    out->seek(-((i64)sizeof(entry)));

    entry.size = size;
    out->write(&entry, sizeof(entry));
    out->seek(total_seek_back);

    size += sizeof(entry) + entry.name_size + entry.type_size;
    return true;
}

static bool serialize_value(WriteTape* out, DescPair pair, u64& size)
{
    bool success = false;
    switch (pair.desc->type_class) {
        case TypeClass::Primitive: {
            success = serialize_primitive(out, pair, size);
        } break;
        case TypeClass::String: {
            success = serialize_string(out, pair, size);
        } break;
        case TypeClass::Array: {
            success = serialize_array(out, pair, size);
        } break;
        case TypeClass::Object: {
            success = serialize_object(out, pair, size);
        } break;
        case TypeClass::Enumeration: {
        } break;
    }

    return success;
}

static bool serialize_primitive(WriteTape* out, DescPair pair, u64& size)
{
    if (!IS_A(pair.desc, IPrimitiveDescriptor)) return false;

    IPrimitiveDescriptor* d = (IPrimitiveDescriptor*)pair.desc;
    size                    = d->get_size();
    if (!out->write(pair.ptr, size)) return false;

    return true;
}

static bool serialize_string(WriteTape* out, DescPair pair, u64& size)
{
    if (!IS_A(pair.desc, StrDescriptor)) return false;

    Str* s     = (Str*)pair.ptr;
    u64  s_len = s->len;

    // Write string size first
    size = sizeof(s_len) + s->len;
    out->write(&s_len, sizeof(s_len));

    // Write actual string
    out->write_str(*s);

    return true;
}

static bool serialize_array(WriteTape* out, DescPair pair, u64& size)
{
    if (!IS_A(pair.desc, IArrayDescriptor)) return false;

    IArrayDescriptor* d = (IArrayDescriptor*)pair.desc;
    d->init_read();

    IDescriptor* sub         = d->get_subtype_descriptor();
    u64          array_count = d->size(pair.ptr);

    // Write array size first
    size += sizeof(array_count);
    out->write(&array_count, sizeof(array_count));

    // Write rest of array values
    for (u64 i = 0; i < array_count; ++i) {
        u64      value_len = 0;
        DescPair sub_pair  = {
            sub,
            d->get(pair.ptr, i),
        };
        if (!serialize_value(out, sub_pair, value_len)) return false;

        size += value_len;
    }

    return true;
}

static bool serialize_object(WriteTape* out, DescPair pair, u64& size)
{
    size                     = 0;
    Slice<IDescriptor*> subs = pair.desc->subdescriptors(pair.ptr);

    // Write number of subdescriptor entries
    u64 subdesc_count = subs.count;

    out->write(&subdesc_count, sizeof(subdesc_count));
    size += sizeof(subdesc_count);

    for (IDescriptor* sub : subs) {
        DescPair sub_pair = {
            .desc = sub,
            .ptr  = pair.ptr + sub->offset,
        };
        u64 sub_size = 0;
        if (!serialize_entry(out, sub_pair, sub_size)) return false;
        size += sub_size;
    }

    return true;
}

static bool deserialize_entry(
    IAllocator& allocator, ReadTape* in, DescPair pair);
static bool deserialize_value(
    IAllocator& allocator, ReadTape* in, DescPair pair);
static bool deserialize_primitive(
    IAllocator& allocator, ReadTape* in, DescPair pair);
static bool deserialize_string(
    IAllocator& allocator, ReadTape* in, DescPair pair);
static bool deserialize_array(
    IAllocator& allocator, ReadTape* in, DescPair pair);
static bool deserialize_object(
    IAllocator& allocator, ReadTape* in, DescPair pair);

PROC_DESERIALIZE(archive_deserialize)
{
    ArchiveHeader hdr;
    ArchiveHeader hdr_cmp;

    if (!in->read_struct(hdr)) return false;

    if (hdr.magic != hdr_cmp.magic) return false;

    return deserialize_entry(alloc, in, DescPair{desc, ptr});
}

static bool deserialize_entry(
    IAllocator& allocator, ReadTape* in, DescPair pair)
{
    ArchiveEntry entry;
    if (!in->read_struct(entry)) return false;

    Str name      = Str::NullStr;
    Str type_name = Str::NullStr;

    if (entry.name_size != 0) {
        char* data = (char*)allocator.reserve(entry.name_size);
        if (in->read(data, entry.name_size) != entry.name_size) return false;
        name = Str(data, entry.name_size);
    }

    char* data = (char*)allocator.reserve(entry.type_size);
    if (in->read(data, entry.type_size) != entry.type_size) return false;
    type_name = Str(data, entry.type_size);

    if (pair.desc->type_name() != type_name) return false;

    return deserialize_value(allocator, in, pair);
}

static bool deserialize_value(
    IAllocator& allocator, ReadTape* in, DescPair pair)
{
    bool success = false;
    switch (pair.desc->type_class) {
        case TypeClass::Primitive: {
            success = deserialize_primitive(allocator, in, pair);
        } break;
        case TypeClass::String: {
            success = deserialize_string(allocator, in, pair);
        } break;
        case TypeClass::Array: {
            success = deserialize_array(allocator, in, pair);
        } break;
        case TypeClass::Object: {
            success = deserialize_object(allocator, in, pair);
        } break;
        case TypeClass::Enumeration: {
        } break;
    }

    return success;
}

static bool deserialize_primitive(
    IAllocator& allocator, ReadTape* in, DescPair pair)
{
    if (!IS_A(pair.desc, IPrimitiveDescriptor)) return false;

    IPrimitiveDescriptor* primitive_desc = (IPrimitiveDescriptor*)pair.desc;

    u32 size = primitive_desc->get_size();

    if (in->read(pair.ptr, size) != size) return false;

    return true;
}

static bool deserialize_string(
    IAllocator& allocator, ReadTape* in, DescPair pair)
{
    if (!IS_A(pair.desc, StrDescriptor)) return false;

    Str* pstr    = (Str*)pair.ptr;
    u64  str_len = 0;

    // Read in length
    if (in->read(&str_len, sizeof(u64)) != sizeof(u64)) return false;

    // No need to allocate if it's an empty string
    if (str_len == 0) {
        *pstr = Str::NullStr;
        return true;
    }

    char* data = (char*)allocator.reserve(str_len);
    if (in->read(data, str_len) != str_len) return false;

    *pstr = Str(data, str_len, false);

    return true;
}

static bool deserialize_array(
    IAllocator& allocator, ReadTape* in, DescPair pair)
{
    if (!IS_A(pair.desc, IArrayDescriptor)) return false;

    u64 array_count = 0;

    // Read in length
    if (in->read(&array_count, sizeof(array_count)) != sizeof(array_count))
        return false;

    IArrayDescriptor* d = (IArrayDescriptor*)pair.desc;
    d->init(pair.ptr, allocator);

    IDescriptor* sub_desc = d->get_subtype_descriptor();

    for (u64 i = 0; i < array_count; ++i) {
        umm sub_ptr = d->add(pair.ptr);

        if (!deserialize_value(allocator, in, DescPair{sub_desc, sub_ptr}))
            return false;
    }

    return true;
}

static bool deserialize_object(
    IAllocator& allocator, ReadTape* in, DescPair pair)
{
    // Read in number of subdescriptors
    u64 num_parts = 0;

    if (in->read(&num_parts, sizeof(num_parts)) != sizeof(num_parts))
        return false;

    for (u64 i = 0; i < num_parts; ++i) {
        ArchiveEntry entry;
        if (!in->read_struct(entry)) return false;

        // In this case, entries _must_ have a name
        if (entry.name_size == 0) return false;

        Str name = Str::NullStr;

        {
            char* data = (char*)allocator.reserve(entry.name_size);
            if (in->read(data, entry.name_size) != entry.name_size)
                return false;

            name = Str(data, entry.name_size);
        }

        DEFER(allocator.release(name.data));

        in->seek(-((i64)entry.name_size));
        in->seek(-((i64)sizeof(entry)));

        IDescriptor* sub     = pair.desc->find_descriptor(pair.ptr, name);
        umm          sub_ptr = pair.ptr + sub->offset;
        if (!deserialize_entry(allocator, in, DescPair{sub, sub_ptr}))
            return false;
    }

    return true;
}