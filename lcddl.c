#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "lcddl.h"

enum
{
    TOKEN_TYPE_NONE,
    TOKEN_TYPE_UNKNOWN,
    TOKEN_TYPE_EOF,
    TOKEN_TYPE_TYPENAME,
    TOKEN_TYPE_COLON,
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_SEMICOLON,
    TOKEN_TYPE_OPEN_CURLY_BRACKET,
    TOKEN_TYPE_CLOSE_CURLY_BRACKET,
    TOKEN_TYPE_ASTERIX
};

typedef struct token_t token_t;
struct token_t
{
    union
    {
        char String[32];
        int IndirectionLevel;
    } Value;
    int Type;
};

typedef struct lexerInputStream_t lexerInputStream_t;
struct lexerInputStream_t
{
    int PreviousTokenType;

    int ContentsSize;
    char *ContentsStart;
    char *Contents;
};


void ReadFile(char *filename, lexerInputStream_t *result)
{
    int size = 0;
    FILE *f = fopen(filename, "rb");
    if (f == NULL)
    {
        result->Contents = NULL;
        fprintf(stderr, "\x1b[31mERROR: could not open file\x1b[0m");
        lcddlUserCleanupCallback();
        exit(-1);
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    result->Contents = malloc(size + 1);
    if (size != fread(result->Contents, sizeof(char), size, f))
    {
        free(result->Contents);
        fprintf(stderr, "\x1b[31mERROR: could not open file\x1b[0m");
        lcddlUserCleanupCallback();
        exit(-1);
    }
    fclose(f);
    (result->Contents)[size] = 0;

    result->ContentsStart = result->Contents;
    result->ContentsSize = size;
    result->PreviousTokenType = TOKEN_TYPE_NONE;
}

char ConsumeChar(lexerInputStream_t *stream)
{
    if (stream->ContentsSize == 0) return -1;
    --stream->ContentsSize;
    return *(stream->Contents++);
}

char *TokenTypeToString(int type)
{
    switch(type)
    {
        case TOKEN_TYPE_NONE:
            return "TOKEN_TYPE_NONE";
        case TOKEN_TYPE_UNKNOWN:
            return "TOKEN_TYPE_UNKNOWN";
        case TOKEN_TYPE_EOF:
            return "TOKEN_TYPE_EOF";
        case TOKEN_TYPE_TYPENAME:
            return "TOKEN_TYPE_TYPENAME";
        case TOKEN_TYPE_COLON:
            return "TOKEN_TYPE_COLON";
        case TOKEN_TYPE_IDENTIFIER:
            return "TOKEN_TYPE_IDENTIFIER";
        case TOKEN_TYPE_SEMICOLON:
            return "TOKEN_TYPE_SEMICOLON";
        case TOKEN_TYPE_OPEN_CURLY_BRACKET:
            return "TOKEN_TYPE_OPEN_CURLY_BRACKET";
        case TOKEN_TYPE_CLOSE_CURLY_BRACKET:
            return "TOKEN_TYPE_CLOSE_CURLY_BRACKET";
        case TOKEN_TYPE_ASTERIX:
            return "TOKEN_TYPE_ASTERIX";
        default:
            return "\x1b[31mERROR converting to string: "
                   "Unknown token type\x1b[0m";
    }
}

token_t GetToken(lexerInputStream_t *stream)
{
    char lastChar = ConsumeChar(stream);

    int identifierLength = 1;
    char *identifier = malloc(sizeof*identifier);

    /* skip over white space */
    while (isspace((unsigned char) lastChar))
    {
        lastChar = ConsumeChar(stream);
        if (lastChar == -1)
        {
            token_t token = { .Type = TOKEN_TYPE_EOF };
            free(identifier);
            return token;
        }
    }
    
    if (isalpha(lastChar) ||
        lastChar == '_')
    {
        identifier[identifierLength - 1] = lastChar;
        identifier = realloc(identifier, ++identifierLength *
                                         sizeof *identifier);
        while (isalnum(stream->Contents[0]) ||
               stream->Contents[0] == '_')
        {
            lastChar = ConsumeChar(stream);
            if (lastChar == -1)
            {
                token_t token = { .Type = TOKEN_TYPE_EOF };
                free(identifier);
                return token;
            }
            identifier[identifierLength - 1] = lastChar;
            identifier = realloc(identifier, ++identifierLength *
                                             sizeof *identifier);
        }
    }
    else if (lastChar == ':')
    {
        token_t token = { .Type = TOKEN_TYPE_COLON,
                          .Value.String = ":"
                        };
        stream->PreviousTokenType = TOKEN_TYPE_COLON;
        free(identifier);
        return token;
    }
    else if (lastChar == ';')
    {
        token_t token = { .Type = TOKEN_TYPE_SEMICOLON,
                          .Value.String = ";"
                        };
        stream->PreviousTokenType = TOKEN_TYPE_SEMICOLON;
        free(identifier);
        return token;
    }
    else if (lastChar == '{')
    {
        token_t token = { .Type = TOKEN_TYPE_OPEN_CURLY_BRACKET,
                          .Value.String = "{"
                        };
        stream->PreviousTokenType = TOKEN_TYPE_OPEN_CURLY_BRACKET;
        free(identifier);
        return token;
    }
    else if (lastChar == '}')
    {
        token_t token = { .Type = TOKEN_TYPE_CLOSE_CURLY_BRACKET,
                          .Value.String = "}"
                        };
        stream->PreviousTokenType = TOKEN_TYPE_CLOSE_CURLY_BRACKET;
        free(identifier);
        return token;
    }
    else if (lastChar == '*')
    {
        token_t token = {.Type = TOKEN_TYPE_ASTERIX,
                         .Value.IndirectionLevel = 1
                        };
        while(stream->Contents[0] == '*')
        {
            lastChar = ConsumeChar(stream);
            ++token.Value.IndirectionLevel;
        }
        free(identifier);
        return token;
    }
    else if (lastChar == '#')
    {
        do continue; while ((lastChar = ConsumeChar(stream)) != '\n');
        free(identifier);
        return GetToken(stream);
    }
    else
    {
        fprintf(stderr, "%c", lastChar);
        token_t token = { .Type = TOKEN_TYPE_UNKNOWN, .Value = "ERROR" };
        free(identifier);
        return token;
    }

    token_t token;

    identifier[identifierLength - 1] = 0;
    memcpy(token.Value.String,
           identifier, identifierLength < 32 ? identifierLength : 32);

    free(identifier);

    if (stream->PreviousTokenType == TOKEN_TYPE_COLON)
    {
        token.Type = TOKEN_TYPE_IDENTIFIER;
        stream->PreviousTokenType = TOKEN_TYPE_IDENTIFIER;
        return token;
    }
    else
    {
        token.Type = TOKEN_TYPE_TYPENAME;
        stream->PreviousTokenType = TOKEN_TYPE_TYPENAME;
        return token;
    }
}

node_t *Parse(lexerInputStream_t *stream)
{
    node_t *topLevelFirst = NULL;
    node_t *topLevelLast = NULL;

    node_t *parent = NULL;

    token_t token;
    for(;;)
    {
        token = GetToken(stream);
        if (token.Type == TOKEN_TYPE_TYPENAME)
        {
            node_t *node = malloc(sizeof *node);

            node->Parent = parent;
            node->Child = NULL;
            node->Next = NULL;

            memcpy(node->Type,
                   token.Value.String, 32);

            token = GetToken(stream);
            if (token.Type != TOKEN_TYPE_COLON &&
                token.Type != TOKEN_TYPE_ASTERIX)
            {
                fprintf(stderr, "Expected a colon or asterix.");
                lcddlUserCleanupCallback();
                exit(-1);
            }
            if (token.Type == TOKEN_TYPE_ASTERIX) 
            {
                node->IndirectionLevel = token.Value.IndirectionLevel;
                if (GetToken(stream).Type != TOKEN_TYPE_COLON)
                {
                    fprintf(stderr, "Expected a colon.");
                    lcddlUserCleanupCallback();
                    exit(-1);
                }
            }
            else
            {
                node->IndirectionLevel = 0;
            }

            token_t identifier = GetToken(stream);
            if (identifier.Type != TOKEN_TYPE_IDENTIFIER)
            {
                fprintf(stderr, "Expected an identifier.");
                lcddlUserCleanupCallback();
                exit(-1);
            }

            memcpy(node->Name,
                   identifier.Value.String, 32);

            if (parent)
            {
                if (!(parent->Child))
                {
                    parent->Child = node;
                }
                else
                {
                    node_t *head = parent->Child;
                    while (head->Next)
                        head = head->Next;
                    head->Next = node;
                }
            }
            else
            {
                if (!topLevelFirst)
                    topLevelFirst = node;
                if (topLevelLast)
                    topLevelLast->Next = node;
                topLevelLast = node;
            }

            if (0 == strcmp(node->Type, "struct"))
            {
                if (parent != NULL)
                {
                    fprintf(stderr, "Nested structs are not allowed.");
                    lcddlUserCleanupCallback();
                    exit(-1);
                }
                if (GetToken(stream).Type != TOKEN_TYPE_OPEN_CURLY_BRACKET)
                {
                    fprintf(stderr, "Struct declarations must be followed by a"
                                    "curly bracket block.");
                    lcddlUserCleanupCallback();
                    exit(-1);
                }
                parent = node;
            }
            else
            {
                if(GetToken(stream).Type != TOKEN_TYPE_SEMICOLON)
                {
                    fprintf(stderr, "Expected a semicolon");
                    lcddlUserCleanupCallback();
                    exit(-1);
                }
            }
        }
        else if (token.Type == TOKEN_TYPE_CLOSE_CURLY_BRACKET)
        {
            parent = NULL;
        }
        else if (token.Type == TOKEN_TYPE_EOF)
        {
            break;
        }
        else
        {
            fprintf(stderr, "ERROR: unexpected token %s",
                    TokenTypeToString(token.Type));
            lcddlUserCleanupCallback();
            exit(-1);
        }
    }

    return topLevelFirst;
}

int main(int argc, char **argv)
{
    lexerInputStream_t file;
    ReadFile(argv[1], &file);

    lcddlUserInitCallback();

    node_t *ast = Parse(&file);

    node_t *node = ast;
    lcddlUserTopLevelCallback(node);
    while (node->Next)
    {
        node = node->Next;
        lcddlUserTopLevelCallback(node);
    }

    node = ast;
    while (node)
    {
        node_t *prev = node;
        node = node->Next;

        if (prev->Child)
        {
            node_t *child = prev->Child;
            while (child)
            {
                node_t *childPrev = child;
                child = child->Next;
                free(childPrev);
            }
        }

        free(prev);
    }

    free(file.ContentsStart);

    lcddlUserCleanupCallback();
}

