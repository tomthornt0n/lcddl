#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lcddl.h"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    #include <windows.h>

    // disable ANSI escape codes for colours on windows
    // TODO(tbt): setup the virtual terminal on windows to use ANSI escape codes
    #define LOG_WARN_BEGIN  "%s:%lu WARNING : "
    #define LOG_ERROR_BEGIN "%s:%lu ERROR : "
#else
    #include <dlfcn.h>

    #define LOG_WARN_BEGIN   "\x1b[33m%s : line %lu : WARNING : \x1b[0m"
    #define LOG_ERROR_BEGIN  "\x1b[31m%s : line %lu : ERROR : \x1b[0m"
#endif

///////////////////////////////////////////
// UTILITIES
//

#define print_warning(_stream, _message) fprintf(stderr,                                \
                                                 LOG_WARN_BEGIN                         \
                                                 _message                               \
                                                 "\n",                                  \
                                                 (_stream)->path,                       \
                                                 (_stream)->current_token.line);              \

#define print_error_and_exit(_stream, _message) fprintf(stderr,                         \
                                                        LOG_ERROR_BEGIN                 \
                                                        _message                        \
                                                        "\n",                           \
                                                        (_stream)->path,                \
                                                        (_stream)->current_token.line);       \
                                                exit(EXIT_FAILURE)

#define print_error_and_exit_f(_stream, _message, ...) fprintf(stderr,                  \
                                                               LOG_ERROR_BEGIN          \
                                                               _message                 \
                                                               "\n",                    \
                                                               (_stream)->path,         \
                                                               (_stream)->current_token.line, \
                                                               __VA_ARGS__);            \
                                                       exit(EXIT_FAILURE)

internal bool
is_char_space(char c)
{
    if (c == ' '  ||
        c == '\t' ||
        c == '\n' ||
        c == '\r' ||
        c == '\f' ||
        c == '\0')
    {
        return true;
    }
    return false;
}

internal bool
is_char_alphanumeric(char c)
{
    if ((c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9'))
    {
        return true;
    }
    return false;
}

internal bool
is_char_number(char c)
{
    if (c >= '0' && c <= '9')
    {
        return true;
    }
    return false;
}

internal bool
is_char_letter(char c)
{
    if ((c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z'))
    {
        return true;
    }
    return false;
}

///////////////////////////////////////////
// LEXER
//

typedef enum
{
    TOKEN_KIND_none,

    TOKEN_KIND_identifier,               // a series of alphanumeric characters begining with a letter or '_'
    TOKEN_KIND_integer_literal,          // a series of numbers
    TOKEN_KIND_float_literal,            // a series of numbers containing exactly one '.'
    TOKEN_KIND_string_literal,           // a series of characters bounded by '"'
    TOKEN_KIND_colon,                    // ':'
    TOKEN_KIND_equals,                   // '='
    TOKEN_KIND_open_curly_bracket,       // '{'
    TOKEN_KIND_close_curly_bracket,      // '}'
    TOKEN_KIND_open_square_bracket,      // '['
    TOKEN_KIND_close_square_bracket,     // ']'
    TOKEN_KIND_open_bracket,             // '('
    TOKEN_KIND_close_bracket,            // ')'
    TOKEN_KIND_asterisk,                 // '*'
    TOKEN_KIND_dash,                     // '-'
    TOKEN_KIND_slash,                    // '/'
    TOKEN_KIND_add,                      // '+'
    TOKEN_KIND_tilde,                    // '~'
    TOKEN_KIND_exclamation,              // '!'
    TOKEN_KIND_lesser_than,              // '<'
    TOKEN_KIND_greater_than,             // '>'
    TOKEN_KIND_lesser_than_or_equal_to,  // '<='
    TOKEN_KIND_greater_than_or_equal_to, // '>='
    TOKEN_KIND_equality,                 // '=='
    TOKEN_KIND_not_equal_to,             // '!='
    TOKEN_KIND_bit_shift_left,           // '<<'
    TOKEN_KIND_bit_shift_right,          // '>>'
    TOKEN_KIND_bitwise_and,              // '&'
    TOKEN_KIND_bitwise_or,               // '|'
    TOKEN_KIND_bitwise_xor,              // '^'
    TOKEN_KIND_boolean_and,              // '&&'
    TOKEN_KIND_boolean_or,               // '||'
    TOKEN_KIND_at_symbol,                // '@'
    TOKEN_KIND_semicolon,                // ';'
    TOKEN_KIND_eof,                      // EOF
} TokenKind;

#define token_kind_to_string(_token_kind)                                      \
    ((_token_kind) == TOKEN_KIND_none                     ? "none" :           \
     (_token_kind) == TOKEN_KIND_identifier               ? "identifier" :     \
     (_token_kind) == TOKEN_KIND_integer_literal          ? "integer literal" :\
     (_token_kind) == TOKEN_KIND_float_literal            ? "float literal" :  \
     (_token_kind) == TOKEN_KIND_string_literal           ? "string literal" : \
     (_token_kind) == TOKEN_KIND_colon                    ? ":" :              \
     (_token_kind) == TOKEN_KIND_equals                   ? "=" :              \
     (_token_kind) == TOKEN_KIND_open_curly_bracket       ? "{" :              \
     (_token_kind) == TOKEN_KIND_close_curly_bracket      ? "}" :              \
     (_token_kind) == TOKEN_KIND_open_square_bracket      ? "[" :              \
     (_token_kind) == TOKEN_KIND_close_square_bracket     ? "]" :              \
     (_token_kind) == TOKEN_KIND_open_bracket             ? "(" :              \
     (_token_kind) == TOKEN_KIND_close_bracket            ? ")" :              \
     (_token_kind) == TOKEN_KIND_asterisk                 ? "*" :              \
     (_token_kind) == TOKEN_KIND_dash                     ? "-" :              \
     (_token_kind) == TOKEN_KIND_slash                    ? "/" :              \
     (_token_kind) == TOKEN_KIND_add                      ? "+" :              \
     (_token_kind) == TOKEN_KIND_tilde                    ? "~" :              \
     (_token_kind) == TOKEN_KIND_lesser_than              ? "<" :              \
     (_token_kind) == TOKEN_KIND_greater_than             ? ">" :              \
     (_token_kind) == TOKEN_KIND_lesser_than_or_equal_to  ? "<=" :             \
     (_token_kind) == TOKEN_KIND_greater_than_or_equal_to ? ">=" :             \
     (_token_kind) == TOKEN_KIND_equality                 ? "==" :             \
     (_token_kind) == TOKEN_KIND_not_equal_to             ? "!=" :             \
     (_token_kind) == TOKEN_KIND_bit_shift_left           ? "<<" :             \
     (_token_kind) == TOKEN_KIND_bit_shift_right          ? ">>" :             \
     (_token_kind) == TOKEN_KIND_at_symbol                ? "@" :              \
     (_token_kind) == TOKEN_KIND_semicolon                ? ";" :              \
     (_token_kind) == TOKEN_KIND_eof                      ? "eof" : NULL)


typedef struct
{
    TokenKind kind;
    unsigned int len;
    char *value;
    unsigned long line;
} Token;

#define PATH_MAX_LEN 96

typedef struct
{
    char *buffer;
    unsigned long long size;
    unsigned long long index;

    char *path;
    unsigned long current_line;

    Token current_token;
} Stream;

internal Token get_next_token(Stream *stream);

static Stream
load_entire_file_as_stream(char *filename)
{
    Stream result       = {0};
    result.current_line = 1;
    result.path         = calloc(1, strlen(filename) + 1);
    strcpy(result.path, filename);
    
    FILE *file = fopen(filename, "rb");
    if(file)
    {
        fseek(file, 0, SEEK_END);
        result.size     = ftell(file);
        fseek(file, 0, SEEK_SET);
        result.buffer   = malloc(result.size);
        if(result.buffer)
        {
            fread(result.buffer, 1, result.size, file);
        }
        fclose(file);
    }

    result.current_token = get_next_token(&result);
    
    return result;
}

internal int
get_character(Stream *stream)
{
    if (stream->index < stream->size)
    {
        return (int)stream->buffer[stream->index];
    }
    return EOF;
}

internal int
peek_character(Stream *stream)
{
    if (stream->index + 1 < stream->size)
    {
        return (int)stream->buffer[stream->index + 1];
    }
    return EOF;
}

internal void
consume_character(Stream *stream)
{
    stream->index += 1;
}

internal Token
get_next_token(Stream *stream)
{
    Token result = {0};
    result.line  = stream->current_line;
    int c = get_character(stream);

    // NOTE(tbt): ignore white space
    while (is_char_space(c))
    {
        if (c == '\n')
        {
            ++stream->current_line;
        }
        consume_character(stream);
        c = get_character(stream);
    }

    // NOTE(tbt): skip comments
    if (c == '/' &&
        peek_character(stream) == '/')
    {
        while (c != '\n')
        {
            consume_character(stream);
            c = get_character(stream);
        }
        ++stream->current_line;
        return get_next_token(stream);
    }


    if (is_char_letter(c) ||
        c == '_')
    {
        result.kind  = TOKEN_KIND_identifier;
        result.value = &stream->buffer[stream->index];

        while (is_char_alphanumeric(c) ||
               c == '_')
        {
            ++result.len;
            consume_character(stream);
            c = get_character(stream);
        }
    }
    else if (is_char_number(c))
    {
        result.kind  = TOKEN_KIND_integer_literal;
        result.value = &stream->buffer[stream->index];

        while (is_char_number(c) ||
               c == '.')
        {
            if (c == '.')
            {
                if (result.kind == TOKEN_KIND_integer_literal)
                {
                    result.kind = TOKEN_KIND_float_literal;
                }
                else
                {
                    print_error_and_exit(stream, "float literal may contain only one '.'");
                }
            }

            ++result.len;
            consume_character(stream);
            c = get_character(stream);
        }
    }
    else if (c == '"')
    {
        consume_character(stream);
        c = get_character(stream);

        result.kind  = TOKEN_KIND_string_literal;
        result.value = &stream->buffer[stream->index];

        while (c != '"')
        {
            ++result.len;
            consume_character(stream);
            c = get_character(stream);
        }

        consume_character(stream);
    }
    else if (c == '=')
    {
        result.kind  = TOKEN_KIND_equals;
        result.value = &stream->buffer[stream->index];
        result.len   = 1;
        consume_character(stream);

        c = get_character(stream);
        if (c == '=')
        {
            result.kind = TOKEN_KIND_equality;
            result.len  = 2;
            consume_character(stream);
        }
    }
    else if (c == '!')
    {
        result.kind = TOKEN_KIND_exclamation;
        result.value = &stream->buffer[stream->index];
        result.len   = 1;
        consume_character(stream);

        c = get_character(stream);
        if (c == '=')
        {
            result.kind = TOKEN_KIND_not_equal_to;
            result.len  = 2;
            consume_character(stream);
        }
    }
    else if (c == '<')
    {
        result.kind = TOKEN_KIND_lesser_than;
        result.value = &stream->buffer[stream->index];
        result.len   = 1;
        consume_character(stream);

        c = get_character(stream);
        if (c == '=')
        {
            result.kind = TOKEN_KIND_lesser_than_or_equal_to;
            result.len  = 2;
            consume_character(stream);
        }
        else if (c == '<')
        {
            result.kind = TOKEN_KIND_bit_shift_right;
            result.len  = 2;
            consume_character(stream);
        }
    }
    else if (c == '>')
    {
        result.kind = TOKEN_KIND_greater_than;
        result.value = &stream->buffer[stream->index];
        result.len   = 1;
        consume_character(stream);

        c = get_character(stream);
        if (c == '=')
        {
            result.kind = TOKEN_KIND_greater_than_or_equal_to;
            result.len  = 2;
            consume_character(stream);
        }
        else if (c == '>')
        {
            result.kind = TOKEN_KIND_bit_shift_right;
            result.len  = 2;
            consume_character(stream);
        }
    }
    else if (c == '&')
    {
        result.kind = TOKEN_KIND_bitwise_and;
        result.value = &stream->buffer[stream->index];
        result.len   = 1;
        consume_character(stream);

        c = get_character(stream);
        if (c == '&')
        {
            result.kind = TOKEN_KIND_boolean_and;
            result.len  = 2;
            consume_character(stream);
        }
    }
    else if (c == '|')
    {
        result.kind  = TOKEN_KIND_bitwise_or;
        result.value = &stream->buffer[stream->index];
        result.len   = 1;
        consume_character(stream);

        c = get_character(stream);
        if (c == '|')
        {
            result.kind = TOKEN_KIND_boolean_or;
            result.len  = 2;
            consume_character(stream);
        }
    }
    else if (c == EOF)
    {
        result.kind  = TOKEN_KIND_eof;
        result.value = NULL;
        result.len   = 0;
    }
    else
    {
        switch(c)
        {
            case ':':
            {
                result.kind = TOKEN_KIND_colon;
                break;
            }
            case '{':
            {
                result.kind = TOKEN_KIND_open_curly_bracket;
                break;
            }
            case '}':
            {
                result.kind = TOKEN_KIND_close_curly_bracket;
                break;
            }
            case '[':
            {
                result.kind = TOKEN_KIND_open_square_bracket;
                break;
            }
            case ']':
            {
                result.kind = TOKEN_KIND_close_square_bracket;
                break;
            }
            case '(':
            {
                result.kind = TOKEN_KIND_open_bracket;
                break;
            }
            case ')':
            {
                result.kind = TOKEN_KIND_close_bracket;
                break;
            }
            case '*':
            {
                result.kind = TOKEN_KIND_asterisk;
                break;
            }
            case '-':
            {
                result.kind = TOKEN_KIND_dash;
                break;
            }
            case '/':
            {
                result.kind = TOKEN_KIND_slash;
                break;
            }
            case '+':
            {
                result.kind = TOKEN_KIND_add;
                break;
            }
            case '~':
            {
                result.kind = TOKEN_KIND_tilde;
                break;
            }
            case '@':
            {
                result.kind = TOKEN_KIND_at_symbol;
                break;
            }
            case ';':
            {
                result.kind = TOKEN_KIND_semicolon;
                break;
            }
            case '^':
            {
                result.kind = TOKEN_KIND_bitwise_xor;
                break;
            }
            default:
            {
                print_error_and_exit_f(stream, "Unexpected character '%c'", c);
            }
        }

        result.value    = &stream->buffer[stream->index];
        result.len      = 1;
        consume_character(stream);
    }

    return result;
} 

internal void
consume_token(Stream *stream,
              TokenKind required_kind)
{
    if (stream->current_token.kind == required_kind)
    {
        stream->current_token = get_next_token(stream);
    }
    else
    {
        print_error_and_exit_f(stream,
                               "Expected '%s', got '%s'",
                               token_kind_to_string(required_kind),
                               token_kind_to_string(stream->current_token.kind));
    }
}

internal LcddlOperatorKind
token_to_operator_kind(Token token,
                       bool unary) // whether to use the unary or binary version if ambiguous
{
    switch (token.kind)
    {
        case TOKEN_KIND_tilde:
        {
            return LCDDL_UN_OP_KIND_bitwise_not;
        }
        case TOKEN_KIND_exclamation:
        {
            return LCDDL_UN_OP_KIND_boolean_not;
        }
        case TOKEN_KIND_add:
        {
            if (unary) { return LCDDL_UN_OP_KIND_positive; }
            else       { return LCDDL_BIN_OP_KIND_add; }
        }
        case TOKEN_KIND_dash:
        {
            if (unary) { return LCDDL_UN_OP_KIND_negative; }
            else       { return LCDDL_BIN_OP_KIND_subtract; }
        }
        case TOKEN_KIND_asterisk:
        {
            return LCDDL_BIN_OP_KIND_multiply;
        }
        case TOKEN_KIND_slash:
        {
            return LCDDL_BIN_OP_KIND_divide;
        }
        case TOKEN_KIND_lesser_than:
        {
            return LCDDL_BIN_OP_KIND_lesser_than;
        }
        case TOKEN_KIND_greater_than:
        {
            return LCDDL_BIN_OP_KIND_greater_than;
        }
        case TOKEN_KIND_lesser_than_or_equal_to:
        {
            return LCDDL_BIN_OP_KIND_lesser_than_or_equal_to;
        }
        case TOKEN_KIND_greater_than_or_equal_to:
        {
            return LCDDL_BIN_OP_KIND_greater_than_or_equal_to;
        }
        case TOKEN_KIND_equality:
        {
            return LCDDL_BIN_OP_KIND_equality;
        }
        case TOKEN_KIND_not_equal_to:
        {
            return LCDDL_BIN_OP_KIND_not_equal_to;
        }
        case TOKEN_KIND_bit_shift_left:
        {
            return LCDDL_BIN_OP_KIND_bit_shift_left;
        }
        case TOKEN_KIND_bit_shift_right:
        {
            return LCDDL_BIN_OP_KIND_bit_shift_right;
        }
        case TOKEN_KIND_bitwise_and:
        {
            return LCDDL_BIN_OP_KIND_bitwise_and;
        }
        case TOKEN_KIND_bitwise_or:
        {
            return LCDDL_BIN_OP_KIND_bitwise_or;
        }
        case TOKEN_KIND_bitwise_xor:
        {
            return LCDDL_BIN_OP_KIND_bitwise_xor;
        }
        case TOKEN_KIND_boolean_and:
        {
            return LCDDL_BIN_OP_KIND_boolean_and;
        }
        case TOKEN_KIND_boolean_or:
        {
            return LCDDL_BIN_OP_KIND_boolean_or;
        }
        default:
        {
            return -1;
        }
    }
}

internal bool
is_token_usable_as_binary_operator(Token token)
{
    LcddlOperatorKind kind = token_to_operator_kind(token, false);
    if (kind == -1                          ||
        kind <= LCDDL_BINARY_OPERATOR_BEGIN ||
        kind >= LCDDL_BINARY_OPERATOR_END)
    {
        return false;
    }
    return true;
}

internal bool
is_token_usable_as_unary_operator(Token token)
{
    LcddlOperatorKind kind = token_to_operator_kind(token, true);
    if (kind == -1                          ||
        kind <= LCDDL_UNARY_OPERATOR_BEGIN ||
        kind >= LCDDL_UNARY_OPERATOR_END)
    {
        return false;
    }
    return true;
}

///////////////////////////////////////////
// PARSER
//

internal LcddlNode *parse_file(char *path);
internal LcddlNode *parse_statement(Stream *stream);
internal LcddlNode *parse_statement_list(Stream *stream);
internal LcddlNode *parse_annotations(Stream *stream);
internal LcddlNode *parse_declaration(Stream *stream);
internal LcddlNode *parse_type(Stream *stream);
internal LcddlNode *parse_binary_operator_rhs(Stream *stream, unsigned int expression_precedence, LcddlNode *lhs);
internal LcddlNode *parse_expression(Stream *stream);
internal LcddlNode *parse_parenthesis(Stream *lexer);
internal LcddlNode *parse_primary(Stream *stream);
internal LcddlNode *parse_unary_operator(Stream *stream);
internal LcddlNode *parse_literal(Stream *stream);
internal LcddlNode *parse_variable_reference(Stream *stream);

internal LcddlNode *
parse_file(char *path)
{
    Stream stream         = load_entire_file_as_stream(path);

    LcddlNode *result     = calloc(1, sizeof *result);
    result->kind          = LCDDL_NODE_KIND_file;
    result->file.filename = calloc(1, strlen(path) + 1);
    strcpy(result->file.filename, path);
    result->first_child   = parse_statement_list(&stream);

    return result;
}

internal LcddlNode *
parse_statement(Stream *stream)
{
    LcddlNode *result      = NULL;
    LcddlNode *annotations = parse_annotations(stream);

    if (stream->current_token.kind == TOKEN_KIND_identifier)
    {
        result = parse_declaration(stream);
        consume_token(stream, TOKEN_KIND_semicolon);
    }
    else
    {
        print_error_and_exit(stream, "Expected a statement");
    }

    result->first_annotation = annotations;
    return result;
}

internal LcddlNode *
parse_statement_list(Stream *stream)
{
    LcddlNode *result = NULL;

    while (stream->current_token.kind == TOKEN_KIND_identifier ||
           stream->current_token.kind == TOKEN_KIND_at_symbol)
    {
        LcddlNode *node = parse_statement(stream);
        node->next_sibling = result;
        result = node;
    }

    return result;
}

internal LcddlNode *
parse_annotations(Stream *stream)
{
    LcddlNode *result = NULL;

    while (stream->current_token.kind == TOKEN_KIND_at_symbol)
    {
        consume_token(stream, TOKEN_KIND_at_symbol);
        LcddlNode *annotation      = calloc(1, sizeof *annotation);
        annotation->kind           = LCDDL_NODE_KIND_annotation;
        annotation->annotation.tag = calloc(1, stream->current_token.len + 1);
        strncpy(annotation->annotation.tag,
                stream->current_token.value,
                stream->current_token.len);
        consume_token(stream, TOKEN_KIND_identifier);

        if (stream->current_token.kind == TOKEN_KIND_equals)
        {
            consume_token(stream, TOKEN_KIND_equals);
            annotation->annotation.value = parse_expression(stream);
        }

        annotation->next_annotation = result;
        result = annotation;
    }

    return result;
}

internal LcddlNode *
parse_declaration(Stream *stream)
{
    LcddlNode *result        = calloc(1, sizeof *result);
    result->kind             = LCDDL_NODE_KIND_declaration;
    result->declaration.name = calloc(1, stream->current_token.len + 1);
    strncpy(result->declaration.name,
            stream->current_token.value,
            stream->current_token.len);

    consume_token(stream, TOKEN_KIND_identifier);
    consume_token(stream, TOKEN_KIND_colon);

    if (stream->current_token.kind == TOKEN_KIND_colon)
    // NOTE(tbt): double colon means compound declaration
    {
        consume_token(stream, TOKEN_KIND_colon);
        result->declaration.type = parse_type(stream);
        consume_token(stream, TOKEN_KIND_open_curly_bracket);
        result->first_child      = parse_statement_list(stream);
        consume_token(stream, TOKEN_KIND_close_curly_bracket);
    }
    else
    {
        if (stream->current_token.kind == TOKEN_KIND_equals)
        // NOTE(tbt): type has been ommited - go straight to value;
        {
            consume_token(stream, TOKEN_KIND_equals);
            result->declaration.value = parse_expression(stream);
        }
        else
        {
            result->declaration.type = parse_type(stream);
            if (stream->current_token.kind == TOKEN_KIND_equals)
            {
                consume_token(stream, TOKEN_KIND_equals);
                result->declaration.value = parse_expression(stream);
            }
        }
    }

    return result;
}

internal LcddlNode *
parse_type(Stream *stream)
{
    LcddlNode *result = calloc(1, sizeof *result);
    result->kind      = LCDDL_NODE_KIND_type;

    if (stream->current_token.kind == TOKEN_KIND_open_square_bracket)
    {
        consume_token(stream, TOKEN_KIND_open_square_bracket);
        char *array_count_str = calloc(1, stream->current_token.len + 1);
        strncpy(array_count_str,
                stream->current_token.value,
                stream->current_token.len);
        consume_token(stream, TOKEN_KIND_integer_literal);

        result->type.array_count = strtoul(array_count_str, NULL, 10);

        consume_token(stream, TOKEN_KIND_close_square_bracket);
    }

    result->type.type_name = calloc(1, stream->current_token.len + 1);
    strncpy(result->type.type_name,
            stream->current_token.value,
            stream->current_token.len);
    consume_token(stream, TOKEN_KIND_identifier);

    while (stream->current_token.kind == TOKEN_KIND_asterisk)
    {
        consume_token(stream, TOKEN_KIND_asterisk);
        result->type.indirection_level += 1; }

    return result;
}

internal LcddlNode *
parse_binary_operator_rhs(Stream *stream,
                          unsigned int expression_precedence,
                          LcddlNode *lhs)
{
    static unsigned int precedence_table[1 << 8] = 
    {
        [LCDDL_BIN_OP_KIND_multiply]                 = 10,
        [LCDDL_BIN_OP_KIND_divide]                   = 10,
        [LCDDL_BIN_OP_KIND_add]                      = 9,
        [LCDDL_BIN_OP_KIND_subtract]                 = 9,
        [LCDDL_BIN_OP_KIND_bit_shift_left]           = 8,
        [LCDDL_BIN_OP_KIND_bit_shift_right]          = 8,
        [LCDDL_BIN_OP_KIND_lesser_than]              = 7,
        [LCDDL_BIN_OP_KIND_lesser_than_or_equal_to]  = 7,
        [LCDDL_BIN_OP_KIND_greater_than]             = 7,
        [LCDDL_BIN_OP_KIND_greater_than_or_equal_to] = 7,
        [LCDDL_BIN_OP_KIND_equality]                 = 6,
        [LCDDL_BIN_OP_KIND_not_equal_to]             = 6,
        [LCDDL_BIN_OP_KIND_bitwise_and]              = 5,
        [LCDDL_BIN_OP_KIND_bitwise_xor]              = 4,
        [LCDDL_BIN_OP_KIND_bitwise_or]               = 3,
        [LCDDL_BIN_OP_KIND_boolean_and]              = 2,
        [LCDDL_BIN_OP_KIND_boolean_or]               = 1,
    };

    for (;;)
    {
        LcddlOperatorKind operator_kind;
        unsigned int token_precedence;

        if (is_token_usable_as_binary_operator(stream->current_token))
        {
            operator_kind    = token_to_operator_kind(stream->current_token, false);
            token_precedence = precedence_table[operator_kind];

            if (token_precedence < expression_precedence)
            {
                return lhs;
            }
        }
        else
        {
            return lhs;
        }

        consume_token(stream, stream->current_token.kind);

        LcddlNode *rhs = parse_primary(stream);

        if (is_token_usable_as_binary_operator(stream->current_token))
        {
            unsigned int next_precedence = precedence_table[token_to_operator_kind(stream->current_token, false)];
            if (token_precedence < next_precedence)
            {
                rhs = parse_binary_operator_rhs(stream, token_precedence + 1, rhs);
            }
        }

        LcddlNode *new_left            = calloc(1, sizeof *new_left);
        new_left->kind                 = LCDDL_NODE_KIND_binary_operator;
        new_left->binary_operator.kind  = operator_kind;
        new_left->binary_operator.left  = lhs;
        new_left->binary_operator.right = rhs;

        lhs = new_left;
    }
}

internal LcddlNode *
parse_expression(Stream *stream)
{
    LcddlNode *lhs = parse_primary(stream);
    return parse_binary_operator_rhs(stream, 0, lhs);
}

internal LcddlNode *
parse_parenthesis(Stream *stream)
{
    consume_token(stream, TOKEN_KIND_open_bracket);  // eat '('
    LcddlNode *result = parse_expression(stream);
    consume_token(stream, TOKEN_KIND_close_bracket); // eat ')'

    return result;
}

internal LcddlNode *
parse_primary(Stream *stream)
{
    switch (stream->current_token.kind)
    {
        case TOKEN_KIND_add:         // unary plus operator
        case TOKEN_KIND_dash:        // unary negative operator
        case TOKEN_KIND_tilde:       // bitwise not operator
        case TOKEN_KIND_exclamation: // boolean not operator
        {
            return parse_unary_operator(stream);
        }
        case TOKEN_KIND_float_literal:
        case TOKEN_KIND_integer_literal:
        case TOKEN_KIND_string_literal:
        {
            return parse_literal(stream);
        }
        case TOKEN_KIND_identifier:
        {
            return parse_variable_reference(stream);
        }
        case TOKEN_KIND_open_bracket:
        {
            return parse_parenthesis(stream);
        }
        default:
        {
            print_error_and_exit_f(stream,
                                   "Got unexpected token '%s' when expecting an expression",
                                   token_kind_to_string(stream->current_token.kind));
            return NULL;
        }
    }
}

internal LcddlNode *
parse_unary_operator(Stream *stream)
{
    LcddlNode *result = calloc(1, sizeof *result);
    result->kind      = LCDDL_NODE_KIND_unary_operator;

    if (is_token_usable_as_unary_operator(stream->current_token))
    {
        result->unary_operator.kind = token_to_operator_kind(stream->current_token, true);
        consume_token(stream, stream->current_token.kind);
    }
    else
    {
        print_error_and_exit_f(stream,
                               "expected a unary operator, instead got '%s'",
                               token_kind_to_string(stream->current_token.kind));
    }
    result->unary_operator.operand = parse_expression(stream);

    return result;
}

internal LcddlNode *
parse_literal(Stream *stream)
{
    LcddlNode *result        = calloc(1, sizeof *result);
    result->literal.value = calloc(1, stream->current_token.len + 1);
    strncpy(result->literal.value,
            stream->current_token.value,
            stream->current_token.len);

    switch (stream->current_token.kind)
    {
        case TOKEN_KIND_string_literal:
        {
            result->kind = LCDDL_NODE_KIND_string_literal;
            consume_token(stream, TOKEN_KIND_string_literal);
            break;
        }
        case TOKEN_KIND_integer_literal:
        {
            result->kind = LCDDL_NODE_KIND_integer_literal;
            consume_token(stream, TOKEN_KIND_integer_literal);
            break;
        }
        case TOKEN_KIND_float_literal:
        {
            result->kind = LCDDL_NODE_KIND_float_literal;
            consume_token(stream, TOKEN_KIND_float_literal);
            break;
        }
        default:
        {
            print_error_and_exit_f(stream,
                                   "Expecting a literal, got '%s'",
                                   token_kind_to_string(stream->current_token.kind));
        }
    }

    return result;
}

internal LcddlNode *
parse_variable_reference(Stream *stream)
{
    LcddlNode *result          = calloc(1, sizeof *result);
    result->kind               = LCDDL_NODE_KIND_variable_reference;
    result->var_reference.name = calloc(1, stream->current_token.len + 1);
    strncpy(result->var_reference.name,
            stream->current_token.value,
            stream->current_token.len);

    consume_token(stream, TOKEN_KIND_identifier);

    return result;
}

///////////////////////////////////////////
// USER LAYER
//

typedef void (*UserCallback)(LcddlNode *);

internal UserCallback get_user_callback_functions(char *lib_path);

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
internal UserCallback
get_user_callback_functions(char *lib_path)
{
    UserCallback result = NULL;

    HINSTANCE library = LoadLibrary(lib_path);
    if (!library)
    {
        fprintf(stderr, "ERROR: Could not open custom layer library '%s'\n", lib_path);
        exit(EXIT_FAILURE);
    }

    result = (UserCallback)GetProcAddress(library, "lcddl_user_callback");

    if (!result)
    {
        fprintf(stderr, "ERROR: Could not find callback in user layer library '%s'\n", lib_path);
        exit(EXIT_FAILURE);
    }

    return result;
}
#else
internal UserCallback
get_user_callback_functions(char *lib_path)
{
    UserCallback result;

    void *library = dlopen(lib_path, RTLD_NOW | RTLD_GLOBAL);
    if (!library)
    {
        fprintf(stderr, "ERROR: Could not open user layer library '%s'\n", lib_path);
        exit(EXIT_FAILURE);
    }

    result = dlsym(library, "lcddl_user_callback");

    if (!result)
    {
        fprintf(stderr, "ERROR: Could not find callback in user layer library '%s'\n", lib_path);
        exit(EXIT_FAILURE);
    }

    return result;
}
#endif


int
main(int argc,
     char **argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s custom_layer_library path input_file_1 input_file_2...\n", argv[0]);
        return EXIT_FAILURE;
    }


    UserCallback user_callback = get_user_callback_functions(argv[1]);

    LcddlNode *root = calloc(1, sizeof *root);
    root->kind      = LCDDL_NODE_KIND_root;
    for (int i = 2;
         i < argc;
         ++i)
    {
        LcddlNode *file    = parse_file(argv[i]);
        file->next_sibling = root->first_child;
        root->first_child  = file;
    }

    user_callback(root);

    return EXIT_SUCCESS;
}

