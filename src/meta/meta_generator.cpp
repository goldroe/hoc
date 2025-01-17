#define _CRT_SECURE_NO_WARNINGS
#define STB_SPRINTF_IMPLEMENTATION
#define STB_SPRINTF_DECORATE(name) hoc_##name
#include <stb_sprintf.h>

#include "base/base_core.h"

#include "base/base_arena.h"
#include "base/base_strings.h"
#include "os/os.h"
#include "os/core/os_core.h"
#include "meta.h"
#include "auto_array.h"
#include "lexer/lexer.h"

#include "base/base_arena.cpp"
#include "base/base_strings.cpp"
#include "os/os.cpp"
#include "lexer/lexer.cpp"

struct Metagen_Member {
    String8 ident;
    String8 type;
};

struct Metagen_Struct {
    String8 ident;
    int member_count;
    Metagen_Member members[64];
};

struct Metagen_Field {
    String8 ident;
    u32 value;
};

struct Metagen_Enum {
    String8 ident;
    int field_count;
    Metagen_Field fields[64];
};

Arena *string_arena;

Auto_Array<String8> command_names;

int main(int argc, char **argv) {
    string_arena = make_arena(get_malloc_allocator());

    String8 files[] = {
        // str8_lit("lexer.h")
        str8_lit("src/hoc/hoc_commands.cpp"),
    };

    for (int i = 0; i < ArrayCount(files); i++) {
        Lexer lexer = make_lexer(files[i]);
        printf("Parsing '%.*s'\n", (int)lexer.file_name.count, lexer.file_name.data);
        Token token{};
        do {
            token = get_token(&lexer);
           
            if (token.type == Token_Ident && str8_match(token.literal, str8_lit("HOC_COMMAND"))) {
                if (get_token(&lexer).type == Token_OpenParen) {
                    Token name = get_token(&lexer);
                    String8 string = str8_copy(string_arena, name.literal);
                    command_names.push(string);
                }
            }
        } while (token.type != Token_EOF);
    }

    #if 1
    // FILE *meta_h_file = fopen("meta_generated.h", "w");
    // fprintf(meta_h_file, "enum Meta_Type {\n");
    // for (int i = 0; i < type_count; i++) {
    //     String8 type = types[i];
    //     fprintf(meta_h_file, "    META(%.*s),\n", (int)type.count, type.data);
    // }
    // fprintf(meta_h_file, "};\n");

    FILE *meta_c_file = fopen("src/hoc/generated_hoc_commands.cpp", "w");
    Assert(meta_c_file);
    fprintf(meta_c_file, "global Hoc_Command hoc_commands[] = {\n");
    for (u64 i = 0; i < command_names.count; i++) {
        String8 command = command_names[i];
        fprintf(meta_c_file, "{ ");
        fprintf(meta_c_file, "str8_lit(\"");
        fprintf(meta_c_file, (const char *)command.data);
        fprintf(meta_c_file, "\"), ");
        fprintf(meta_c_file, (const char *)command.data);
        fprintf(meta_c_file, " },\n");
    }
    fprintf(meta_c_file, "};\n");

    fclose(meta_c_file);
    #endif

    printf("Done meta generation\n");

    return 0;
}
