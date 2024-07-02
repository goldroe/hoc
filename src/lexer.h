#ifndef LEXER_H
#define LEXER_H

struct Lexer {
    String8 file_name;
    int column_number;
    int line_number;
    char *stream;
};

Introspect enum Token_Type {
    Token_EOF,
    Token_OpenBrace,
    Token_CloseBrace,
    Token_OpenBracket,
    Token_CloseBracket,
    Token_OpenParen,
    Token_CloseParen,
    Token_Colon,
    Token_Semicolon,
    Token_Comma,

    Token_Star,
    Token_Ampersand,
    Token_Assign,

    Token_Ident,
    Token_String,
    Token_Integer,
    Token_Float,
};

struct Token {
    Token_Type type;
    int column_number;
    int line_number;
    String8 literal;
    union {
        u64 int_value;
        f64 float_value;
    };
};


#endif // LEXER_H
