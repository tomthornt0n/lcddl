#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lcddl.h"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    #include <windows.h>

    // HACK(tbt): disable ANSI escape codes for colours on windows
    // TODO(tbt): setup the virtual terminal on windows to use ANSI escape codes
    #define LOG_WARN_BEGIN  "%s:%d WARNING : "
    #define LOG_ERROR_BEGIN "%s:%d ERROR : "
#else
    #include <dlfcn.h>

    #define LOG_WARN_BEGIN   "\x1b[33m%s : line %d : WARNING : \x1b[0m"
    #define LOG_ERROR_BEGIN  "\x1b[31m%s : line %d : ERROR : \x1b[0m"
#endif

#define LCDDL_WARN(_message) fprintf(stderr,                            \
                                     LOG_WARN_BEGIN                     \
                                     _message                           \
                                     "\n",                              \
                                     global_current_file,               \
                                     global_current_line)

#define LCDDL_ERROR(_message) fprintf(stderr,                           \
                                      LOG_ERROR_BEGIN                   \
                                      _message                          \
                                      "\n",                             \
                                      global_current_file,              \
                                      global_current_line);             \
                              exit(EXIT_FAILURE)

#define LCDDL_ERRORF(_message, ...) fprintf(stderr,                     \
                                            LOG_ERROR_BEGIN             \
                                            _message                    \
                                            "\n",                       \
                                            global_current_file,        \
                                            global_current_line,        \
                                            __VA_ARGS__);               \
                                    exit(EXIT_FAILURE)

#define internal static

enum
{
    TOKEN_TYPE_none,

    TOKEN_TYPE_identifier,
    TOKEN_TYPE_colon,
    TOKEN_TYPE_type,
    TOKEN_TYPE_equals,
    TOKEN_TYPE_value,
    TOKEN_TYPE_open_curly_bracket,
    TOKEN_TYPE_close_curly_bracket,
    TOKEN_TYPE_open_square_bracket,
    TOKEN_TYPE_close_square_bracket,
    TOKEN_TYPE_integral_literal,
    TOKEN_TYPE_semicolon,
    TOKEN_TYPE_eof,
};

#define TOKEN_TYPE_TO_STRING(_token_type)                                        \
    ((_token_type) == TOKEN_TYPE_none                 ? "none" :                 \
     (_token_type) == TOKEN_TYPE_identifier           ? "identifier" :           \
     (_token_type) == TOKEN_TYPE_colon                ? "colon" :                \
     (_token_type) == TOKEN_TYPE_type                 ? "type" :                 \
     (_token_type) == TOKEN_TYPE_equals               ? "equals" :               \
     (_token_type) == TOKEN_TYPE_value                ? "value" :                \
     (_token_type) == TOKEN_TYPE_open_curly_bracket   ? "open_curly_bracket" :   \
     (_token_type) == TOKEN_TYPE_close_curly_bracket  ? "close_curly_bracket" :  \
     (_token_type) == TOKEN_TYPE_open_square_bracket  ? "open_square_bracket" :  \
     (_token_type) == TOKEN_TYPE_close_square_bracket ? "close_square_bracket" : \
     (_token_type) == TOKEN_TYPE_integral_literal     ? "integral literal" :     \
     (_token_type) == TOKEN_TYPE_semicolon            ? "semicolon" :            \
     (_token_type) == TOKEN_TYPE_eof                  ? "eof" : NULL)

typedef struct
{
    int type;
    char *value;
} Token;

#define STRING_CHUNK_SIZE 32
#define PATH_MAX_LEN      96

internal int   global_current_line  = 1;
internal Token global_current_token = {0};
internal char  global_current_file[PATH_MAX_LEN] = {0};

int
fpeekc(FILE *file)
{
    int c = fgetc(file);
    return c == EOF ? EOF : ungetc(c, file);
}

internal Token
get_next_token(FILE *f)
{
    Token result = {0};
    int c = fgetc(f);
    static bool expecting_type    = false;
    static bool expecting_value   = false;
    static bool expecting_integer = false;

    // NOTE(tbt): ignore white space
    while (isspace((unsigned char)c))
    {
        if (c == '\n')
        {
            ++global_current_line;
        }
        c = fgetc(f);
    }

    // NOTE(tbt): skip comments
    if (c == '/' &&
        fpeekc(f) == '/')
    {
        while (c != '\n') { c = fgetc(f); };
        ++global_current_line;
        return get_next_token(f);
    }


    if (expecting_value)
    {
        expecting_value = false;

        int value_len = 0, value_capacity = STRING_CHUNK_SIZE;
        char *value = calloc(value_capacity, 1);

        value[value_len++] = c;
        while (fpeekc(f) != ';')
        {
            c = fgetc(f);
            value[value_len++] = c;

            if (value_len > value_capacity)
            {
                value_capacity += STRING_CHUNK_SIZE;
                value = realloc(value, value_capacity);
            }
        }

        result.type  = TOKEN_TYPE_value;
        result.value = value;
    }
    else if (expecting_integer)
    {
        if (isdigit(c))
        {
            expecting_integer = false;

            int literal_len = 0, literal_capacity = STRING_CHUNK_SIZE;
            char *literal = calloc(literal_capacity, 1);

            literal[literal_len++] = c;
            while (isdigit(fpeekc(f)))
            {
                c = fgetc(f);
                literal[literal_len++] = c;

                if (literal_len > literal_capacity)
                {
                    literal_capacity += STRING_CHUNK_SIZE;
                    literal = realloc(literal, literal_capacity);
                }
            }

            result.type  = TOKEN_TYPE_integral_literal;
            result.value = literal;
        }
        else
        {
            LCDDL_ERROR("Array size must be an integral literal");
        }
    }
    else if (isalpha(c) ||
             c == '_')
    // NOTE(tbt): identifier can contain alphanumeric character and underscores
    //            and must begin with a letter or an underscore, like C.
    {
        int identifier_len = 0, identifier_capacity = STRING_CHUNK_SIZE;
        char *identifier = calloc(identifier_capacity, 1);

        identifier[identifier_len++] = c;
        while (isalnum(fpeekc(f)) ||
               fpeekc(f) == '_')
        {
            c = fgetc(f);
            identifier[identifier_len++] = c;

            if (identifier_len > identifier_capacity)
            {
                identifier_capacity += STRING_CHUNK_SIZE;
                identifier = realloc(identifier, identifier_capacity);
            }
        }

        // NOTE(tbt): determine whether it is a type or an identifier depending
        //            on the previous token
        if (expecting_type)
        {
            result.type    = TOKEN_TYPE_type;
            expecting_type = false;
        }
        else
        {
            result.type = TOKEN_TYPE_identifier;
        }
        result.value = identifier;
    }
    else if (c == ':')
    {
        expecting_type = true;
        result.type    = TOKEN_TYPE_colon;
    }
    else if (c == '=')
    {
        expecting_type  = false;
        expecting_value = true;
        result.type     = TOKEN_TYPE_equals;
    }
    else if (c == '{')
    {
        result.type = TOKEN_TYPE_open_curly_bracket;
    }
    else if (c == '}')
    {
        result.type = TOKEN_TYPE_close_curly_bracket;
    }
    else if (c == '[')
    {
        expecting_integer = true;
        result.type       = TOKEN_TYPE_open_square_bracket;
    }
    else if (c == ']')
    {
        expecting_integer = false;
        result.type       = TOKEN_TYPE_close_square_bracket;
    }
    else if (c == ';')
    {
        result.type = TOKEN_TYPE_semicolon;
    }
    else if (c == EOF)
    {
        result.type = TOKEN_TYPE_eof;
    }
    else
    {
        LCDDL_ERRORF("Unexpected character '%c'", c);
    }

    return result;
} 

internal void
eat(FILE *f,
    int token_type)
{
    if (global_current_token.type == token_type)
    {
        global_current_token = get_next_token(f);
    }
    else
    {
        LCDDL_ERRORF("Expected %s, got %s",
                     TOKEN_TYPE_TO_STRING(token_type),
                     TOKEN_TYPE_TO_STRING(global_current_token.type));
    }
}

internal LcddlNode *statement(FILE *f);
internal LcddlNode *statement_list(FILE *f);
internal LcddlNode *declaration(FILE *f);

internal LcddlNode *
statement(FILE *f)
{
    LcddlNode *result;

    if (global_current_token.type == TOKEN_TYPE_open_curly_bracket)
    {
        LCDDL_WARN("Usage of anonymous compound statements is discouraged");
        eat(f, TOKEN_TYPE_open_curly_bracket);
        result = statement_list(f);
        eat(f, TOKEN_TYPE_close_curly_bracket);

    }
    else if (global_current_token.type == TOKEN_TYPE_identifier)
    {
        result = declaration(f);
        eat(f, TOKEN_TYPE_semicolon);
    }
    else
    {
        LCDDL_ERRORF("Unexpected token '%s'",
                     TOKEN_TYPE_TO_STRING(global_current_token.type));
    }

    return result;
}

internal LcddlNode *
statement_list(FILE *f)
{
    LcddlNode *result = calloc(1, sizeof(*result));

    // NOTE(tbt): statements can begin with an identifier or a curly bracket.
    //            there is probably a better way of dealing with this but i'm
    //            not clever enought to think of it
    while (global_current_token.type == TOKEN_TYPE_identifier ||
           global_current_token.type == TOKEN_TYPE_open_curly_bracket)
    {
        LcddlNode *child = statement(f);
        child->next_sibling = result->first_child;
        result->first_child = child;
    }

    if (global_current_token.type == TOKEN_TYPE_identifier)
    {
        LCDDL_ERRORF("Unexpected identifier '%s'",
                     global_current_token.value);
    }

    return result;
}

internal LcddlNode *
declaration(FILE *f)
{
    char *identifier = global_current_token.value, *type = NULL, *value = NULL;

    eat(f, TOKEN_TYPE_identifier);
    eat(f, TOKEN_TYPE_colon);

    if (global_current_token.type == TOKEN_TYPE_colon)
    // NOTE(tbt): double colon means compound definition
    {
        eat(f, TOKEN_TYPE_colon);
        type = global_current_token.value;
        eat(f, TOKEN_TYPE_type);
        eat(f, TOKEN_TYPE_open_curly_bracket);

        LcddlNode *result  = statement_list(f);
        result->identifier = identifier;
        result->type       = type;

        eat(f, TOKEN_TYPE_close_curly_bracket);

        return result;
    }
    else
    {
        if (global_current_token.type == TOKEN_TYPE_equals)
        // NOTE(tbt): type has been ommited - go straight to value;
        {
            eat(f, TOKEN_TYPE_equals);
            value = global_current_token.value;
            eat(f, TOKEN_TYPE_value);

            LcddlNode *result  = calloc(sizeof(*result), 1);
            result->identifier = identifier;
            result->value      = value;

            return result;
        }
        else
        {
            LcddlNode *result  = calloc(sizeof(*result), 1);

            if (global_current_token.type == TOKEN_TYPE_open_square_bracket)
            {
                eat(f, TOKEN_TYPE_open_square_bracket);
                result->array_count = atoi(global_current_token.value);
                eat(f, TOKEN_TYPE_integral_literal);
                eat(f, TOKEN_TYPE_close_square_bracket);
            }

            type = global_current_token.value;
            eat(f, TOKEN_TYPE_type);

            if (global_current_token.type == TOKEN_TYPE_equals)
            {
                eat(f, TOKEN_TYPE_equals);

                value = global_current_token.value;
                eat(f, TOKEN_TYPE_value);
            }

            result->identifier = identifier;
            result->type       = type;
            result->value      = value;

            return result;
        }
    }
}

typedef void (*UserInitCallback)(void);
typedef void (*UserTopLevelCallback)(char *, LcddlNode *);
typedef void (*UserCleanupCallback)(void);

typedef struct
{
    UserInitCallback user_init_callback;
    UserTopLevelCallback user_top_level_callback;
    UserCleanupCallback user_cleanup_callback;
} UserCallbacks;

internal UserCallbacks get_user_callback_functions(char *lib_path);

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
internal UserCallbacks
get_user_callback_functions(char *lib_path)
{
    UserCallbacks result;

    HINSTANCE library = LoadLibrary(lib_path);
    if (!library)
    {
        LCDDL_ERRORF("Could not open custom layer library '%s'", lib_path);
    }

    result.user_init_callback      = (UserInitCallback)GetProcAddress(library, "lcddl_user_init_callback");
    result.user_top_level_callback = (UserTopLevelCallback)GetProcAddress(library, "lcddl_user_top_level_callback");
    result.user_cleanup_callback   = (UserCleanupCallback)GetProcAddress(library, "lcddl_user_cleanup_callback");

    if (!result.user_init_callback      ||
        !result.user_top_level_callback ||
        !result.user_cleanup_callback)
    {
        LCDDL_ERRORF("Could not find all required callbacks in custom layer library '%s'", lib_path);
    }

    return result;
}
#else
internal UserCallbacks
get_user_callback_functions(char *lib_path)
{
    UserCallbacks result;

    void *library = dlopen(lib_path, RTLD_NOW | RTLD_GLOBAL);
    if (!library)
    {
        LCDDL_ERRORF("Could not open user layer library '%s'", lib_path);
    }

    result.user_init_callback      = dlsym(library, "lcddl_user_init_callback");
    result.user_top_level_callback = dlsym(library, "lcddl_user_top_level_callback");
    result.user_cleanup_callback   = dlsym(library, "lcddl_user_cleanup_callback");

    if (!result.user_init_callback      ||
        !result.user_top_level_callback ||
        !result.user_cleanup_callback)
    {
        LCDDL_ERRORF("Could not find all required callbacks in user layer library '%s'", lib_path);
    }

    return result;
}
#endif

internal LcddlNode *
parse_file(char *path)
{
    FILE *f = fopen(path, "r");

    global_current_line = 1;
    global_current_token = get_next_token(f);
    strncpy(global_current_file, path, PATH_MAX_LEN);
    LcddlNode *result = statement_list(f);

    fclose(f);
    return result;
}

int
main(int argc,
     char **argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s custom_layer_library path input_file_1 input_file_2...\n", argv[0]);
    }

    UserCallbacks callbacks = get_user_callback_functions(argv[1]);

    callbacks.user_init_callback();

    for (int i = 2;
         i < argc;
         ++i)
    {
        LcddlNode *root = parse_file(argv[i]);

        for (LcddlNode *node = root->first_child;
             NULL != node;
             node = node->next_sibling)
        {
            callbacks.user_top_level_callback(argv[i], node);
        }
    }

    callbacks.user_cleanup_callback();

    return EXIT_SUCCESS;
}

