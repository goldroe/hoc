#ifndef META_H
#define META_H

#define META(type_name) Meta_Type_##type_name

struct Meta_Member {
    // Meta_Type type;
    const char *name;
    b32 is_pointer;
    u32 pointer_count;
    b32 is_array;
    u32 array_count;
    u64 offset;
};

struct Meta_Enum_Field {
    const char *name;
    u32 value;
};

#endif // META_H
