#include "lexer.h"

internal Lexer make_lexer(String8 file_name) {
    OS_Handle file_handle = os_open_file(file_name);
    String8 file_string = os_read_file_string(file_handle);
    os_close_handle(file_handle);
    Lexer lexer{};
    lexer.file_name = file_name;
    lexer.column_number = 0;
    lexer.line_number = 1;
    lexer.stream = (char *)file_string.data;
    return lexer;
}

internal void error(Lexer *lexer, const char *fmt, ...) {
    printf("error: ");
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

internal void advance(Lexer *lexer) {
    if (*lexer->stream == '\r') {
        lexer->stream++;
        if (*lexer->stream == '\n') {
            lexer->stream++;
        }
        lexer->line_number++;
        lexer->column_number = 0;
    } else if (*lexer->stream == '\n') {
        lexer->stream++;
        lexer->line_number++;
        lexer->column_number = 0;
    } else {
        lexer->stream++;
        lexer->column_number++;
    }
}

internal s64 scan_integer(Lexer *lexer) {
    s64 result = 0;
    int base = 10;
    while (isdigit(*lexer->stream)) {
        int digit = *lexer->stream - '0';
        advance(lexer);
        result = result * base + digit;
    }
    return result;
}

internal f64 scan_float(Lexer *lexer) {
    char *start = lexer->stream;
    while (isdigit(*lexer->stream)) {
        advance(lexer);
    }
    if (*lexer->stream == '.') {
        advance(lexer);
    } else {
        // @Todo Invalid character in float
    }

    while (isdigit(*lexer->stream)) {
        advance(lexer);
    }
    if (*lexer->stream == 'f') {
        advance(lexer);
    }
    f64 result = strtod((const char *)start, NULL);
    return result;
}

internal void eat_line(Lexer *lexer) {
    while (*lexer->stream != '\n' && *lexer->stream != '\r') {
        advance(lexer);
    }
}

internal Token get_token(Lexer *lexer) {
    Token token{};
begin:
    token.line_number = lexer->line_number;
    token.column_number = lexer->column_number;

    char *start = lexer->stream;
    advance(lexer);

    switch (*start) {
    default:
        goto begin;

    case 0:
        token.type = Token_EOF;
        break;

    case '{': token.type = Token_OpenBrace; break;
    case '}': token.type = Token_CloseBrace; break;
    case '(': token.type = Token_OpenParen; break;
    case ')': token.type = Token_CloseParen; break;
    case '[': token.type = Token_OpenBracket; break;
    case ']': token.type = Token_CloseBracket; break; 

    case ':': token.type = Token_Colon; break;
    case ';': token.type = Token_Semicolon; break;
    case ',': token.type = Token_Comma; break;

    case '=': token.type = Token_Assign; break;

    case '#':
    case '/':
        eat_line(lexer);
        goto begin;

    case '"':
    {
        token.type = Token_String;
        advance(lexer);
        for (;;) {
            switch (*lexer->stream) {
            case '"':
                advance(lexer);
                goto string_end;
            case '\n':
                error(lexer, "newline in string literal");
                goto string_end;
            case 0:
                error(lexer, "unexpected end of file in string literal");
                goto string_end;
            }
            advance(lexer);
        }
string_end:
        
        s64 count = lexer->stream - start - 2;
        token.literal = str8((u8*)start + 1, count);
        break;
    }

    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z': case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z': case '_':
    {
        token.type = Token_Ident;
        while (isalnum(*lexer->stream) || *lexer->stream == '_') {
            advance(lexer);
        }
        s64 count = lexer->stream - start;
        token.literal = str8((u8 *)start, count);
        break;
    }
    
    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
    {
        while (isalnum(*lexer->stream)) {
            advance(lexer);
        }
        if (*lexer->stream == '.') {
            lexer->stream = start;
            token.type = Token_Float;
            token.float_value = scan_float(lexer);
        } else {
            lexer->stream = start;
            token.type = Token_Integer;
            token.int_value = scan_integer(lexer);
        }
        break;
    }

    case ' ': case '\n': case '\r': case '\f': case '\t':
        while (isspace(*lexer->stream)) {
            advance(lexer);
        }
        goto begin;
        break;
    }
    return token;
}

internal Token peek_token(Lexer *lexer) {
    Lexer temp = *lexer;
    Token token = get_token(&temp);
    return token;
}

internal void expect_token(Lexer *lexer, Token_Type type) {
    Token token = get_token(lexer);
    if (token.type != type) {
    //     Meta_Enum_Field meta_expected = meta_fields_Token_Type[type];
    //     Meta_Enum_Field meta_type = meta_fields_Token_Type[token.type];
    //     error(lexer, "Expected type '%s', got '%s'\n", meta_expected.name, meta_type.name);
        error(lexer, "Expected type %d, got %d\n", (int)type, (int)token.type);
    }
}
