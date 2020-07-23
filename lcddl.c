#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "lcddl.h"

enum
{
    LCDDL_TOKEN_TYPE_NONE,
    LCDDL_TOKEN_TYPE_UNKNOWN,
    LCDDL_TOKEN_TYPE_EOF,
    LCDDL_TOKEN_TYPE_TYPENAME,
    LCDDL_TOKEN_TYPE_COLON,
    LCDDL_TOKEN_TYPE_IDENTIFIER,
    LCDDL_TOKEN_TYPE_SEMICOLON,
    LCDDL_TOKEN_TYPE_OPEN_CURLY_BRACKET,
    LCDDL_TOKEN_TYPE_CLOSE_CURLY_BRACKET,
    LCDDL_TOKEN_TYPE_ASTERIX,
    LCDDL_TOKEN_TYPE_AT,
    LCDDL_TOKEN_TYPE_ANNOTATION
};

typedef struct lcddlToken_t lcddlToken_t;
struct lcddlToken_t
{
    union
    {
        char String[32];
        int IndirectionLevel;
    } Value;
    int Type;
};

typedef struct lcddlLexerInputStream_t lcddlLexerInputStream_t;
struct lcddlLexerInputStream_t
{
    int PreviousTokenType;

    int ContentsSize;
    char *ContentsStart;
    char *Contents;
};


static void lcddl_ReadFile(char *filename, lcddlLexerInputStream_t *result)
{
    int size = 0;
    FILE *f = fopen(filename, "rb");
    if (f == NULL)
    {
        result->Contents = NULL;
        fprintf(stderr, "\x1b[31mERROR: could not open file\x1b[0m\n");
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
        fprintf(stderr, "\x1b[31mERROR: could not open file\x1b[0m\n");
        lcddlUserCleanupCallback();
        exit(-1);
    }
    fclose(f);
    (result->Contents)[size] = 0;

    result->ContentsStart = result->Contents;
    result->ContentsSize = size;
    result->PreviousTokenType = LCDDL_TOKEN_TYPE_NONE;
}

static char lcddl_ConsumeChar(lcddlLexerInputStream_t *stream)
{
    if (stream->ContentsSize == 0) return -1;
    --stream->ContentsSize;
    return *(stream->Contents++);
}

static char *lcddl_TokenTypeToString(int type)
{
    switch(type)
    {
        case LCDDL_TOKEN_TYPE_NONE:
            return "TOKEN_TYPE_NONE";
        case LCDDL_TOKEN_TYPE_UNKNOWN:
            return "TOKEN_TYPE_UNKNOWN";
        case LCDDL_TOKEN_TYPE_EOF:
            return "TOKEN_TYPE_EOF";
        case LCDDL_TOKEN_TYPE_TYPENAME:
            return "TOKEN_TYPE_TYPENAME";
        case LCDDL_TOKEN_TYPE_COLON:
            return "TOKEN_TYPE_COLON";
        case LCDDL_TOKEN_TYPE_IDENTIFIER:
            return "TOKEN_TYPE_IDENTIFIER";
        case LCDDL_TOKEN_TYPE_SEMICOLON:
            return "TOKEN_TYPE_SEMICOLON";
        case LCDDL_TOKEN_TYPE_OPEN_CURLY_BRACKET:
            return "TOKEN_TYPE_OPEN_CURLY_BRACKET";
        case LCDDL_TOKEN_TYPE_CLOSE_CURLY_BRACKET:
            return "TOKEN_TYPE_CLOSE_CURLY_BRACKET";
        case LCDDL_TOKEN_TYPE_ASTERIX:
            return "TOKEN_TYPE_ASTERIX";
        default:
            return "\x1b[31mERROR converting to string: "
                   "Unknown token type\x1b[0m";
    }
}

static lcddlToken_t
lcddl_GetToken(lcddlLexerInputStream_t *stream)
{
    char lastChar = lcddl_ConsumeChar(stream);


    /* skip over white space */
    while (isspace((unsigned char) lastChar))
    {
        lastChar = lcddl_ConsumeChar(stream);
        if (lastChar == -1)
        {
            lcddlToken_t token = { .Type = LCDDL_TOKEN_TYPE_EOF };
            return token;
        }
    }
    
    if (isalpha(lastChar) ||
        lastChar == '_')
    {
        lcddlToken_t token;

        int identifierLength = 1;
        char *identifier = malloc(sizeof*identifier);

        identifier[identifierLength - 1] = lastChar;
        identifier = realloc(identifier, ++identifierLength *
                                         sizeof *identifier);

        while (isalnum(stream->Contents[0]) ||
               stream->Contents[0] == '_')
        {
            lastChar = lcddl_ConsumeChar(stream);
            if (lastChar == -1)
            {
                token.Type = LCDDL_TOKEN_TYPE_EOF;
                free(identifier);
                return token;
            }
            identifier[identifierLength - 1] = lastChar;
            identifier = realloc(identifier, ++identifierLength *
                                             sizeof *identifier);
        }

        identifier[identifierLength - 1] = 0;
        memcpy(token.Value.String,
               identifier, identifierLength < 32 ? identifierLength : 32);

        free(identifier);

        if (stream->PreviousTokenType == LCDDL_TOKEN_TYPE_COLON)
        {
            token.Type = LCDDL_TOKEN_TYPE_IDENTIFIER;
            stream->PreviousTokenType = LCDDL_TOKEN_TYPE_IDENTIFIER;
            return token;
        }
        else if (stream->PreviousTokenType == LCDDL_TOKEN_TYPE_AT)
        {
            token.Type = LCDDL_TOKEN_TYPE_ANNOTATION;
            stream->PreviousTokenType = LCDDL_TOKEN_TYPE_ANNOTATION;
            return token;
        }
        else
        {
            token.Type = LCDDL_TOKEN_TYPE_TYPENAME;
            stream->PreviousTokenType = LCDDL_TOKEN_TYPE_TYPENAME;
            return token;
        }
    }
    else if (lastChar == ':')
    {
        lcddlToken_t token = { .Type = LCDDL_TOKEN_TYPE_COLON,
                          .Value.String = ":"
                        };
        stream->PreviousTokenType = LCDDL_TOKEN_TYPE_COLON;
        return token;
    }
    else if (lastChar == ';')
    {
        lcddlToken_t token = { .Type = LCDDL_TOKEN_TYPE_SEMICOLON,
                          .Value.String = ";"
                        };
        stream->PreviousTokenType = LCDDL_TOKEN_TYPE_SEMICOLON;
        return token;
    }
    else if (lastChar == '{')
    {
        lcddlToken_t token = { .Type = LCDDL_TOKEN_TYPE_OPEN_CURLY_BRACKET,
                               .Value.String = "{"
                        };
        stream->PreviousTokenType = LCDDL_TOKEN_TYPE_OPEN_CURLY_BRACKET;
        return token;
    }
    else if (lastChar == '}')
    {
        lcddlToken_t token = { .Type = LCDDL_TOKEN_TYPE_CLOSE_CURLY_BRACKET,
                               .Value.String = "}"
                        };
        stream->PreviousTokenType = LCDDL_TOKEN_TYPE_CLOSE_CURLY_BRACKET;
        return token;
    }
    else if (lastChar == '*')
    {
        lcddlToken_t token = {.Type = LCDDL_TOKEN_TYPE_ASTERIX,
                              .Value.IndirectionLevel = 1
                        };
        while(stream->Contents[0] == '*')
        {
            lastChar = lcddl_ConsumeChar(stream);
            ++token.Value.IndirectionLevel;
        }
        return token;
    }
    else if (lastChar == '@')
    {
        lcddlToken_t token = { .Type = LCDDL_TOKEN_TYPE_AT,
                               .Value.String = "@"
                        };
        stream->PreviousTokenType = LCDDL_TOKEN_TYPE_AT;
        return token;
    }
    else if (lastChar == '#')
    {
        for (;(lastChar = lcddl_ConsumeChar(stream)) != '\n';);
        return lcddl_GetToken(stream);
    }
    else
    {
        fprintf(stderr, "%c", lastChar);
        lcddlToken_t token = { .Type = LCDDL_TOKEN_TYPE_UNKNOWN, .Value = "ERROR" };
        return token;
    }

}

static lcddlNode_t *lcddl_Parse(lcddlLexerInputStream_t *stream)
{
    lcddlNode_t *topLevelFirst = NULL;
    lcddlNode_t *topLevelLast = NULL;

    lcddlNode_t *parent = NULL;

    lcddlAnnotation_t *annotations = NULL;

    lcddlToken_t token = lcddl_GetToken(stream);
    for (; token.Type != LCDDL_TOKEN_TYPE_EOF; token = lcddl_GetToken(stream))
    {
        if (token.Type == LCDDL_TOKEN_TYPE_AT)
        {
            token = lcddl_GetToken(stream);
            if (token.Type != LCDDL_TOKEN_TYPE_ANNOTATION)
            {
                fprintf(stderr, "Expected an annotation");
                lcddlUserCleanupCallback();
                exit(-1);
            }

            lcddlAnnotation_t *annotation = malloc(sizeof(lcddlAnnotation_t));
            annotation->Next = NULL;
            memcpy(annotation->Tag,
                   token.Value.String, 32);

            if (!(annotations))
            {
                annotations = annotation;
            }
            else
            {
                lcddlAnnotation_t *head;
                for (head = annotations;
                     head->Next;
                     head = head->Next);

                head->Next = annotation;
            }
        }
        else if (token.Type == LCDDL_TOKEN_TYPE_TYPENAME)
        {
            lcddlNode_t *node = malloc(sizeof *node);

            node->Children = NULL;
            node->Next = NULL;

            node->Tags = annotations;
            annotations = NULL;

            memcpy(node->Type,
                   token.Value.String, 32);

            token = lcddl_GetToken(stream);
            if (token.Type != LCDDL_TOKEN_TYPE_COLON &&
                token.Type != LCDDL_TOKEN_TYPE_ASTERIX)
            {
                fprintf(stderr, "Expected a colon or asterix.\n");
                lcddlUserCleanupCallback();
                exit(-1);
            }
            if (token.Type == LCDDL_TOKEN_TYPE_ASTERIX) 
            {
                node->IndirectionLevel = token.Value.IndirectionLevel;
                if (lcddl_GetToken(stream).Type != LCDDL_TOKEN_TYPE_COLON)
                {
                    fprintf(stderr, "Expected a colon.\n");
                    lcddlUserCleanupCallback();
                    exit(-1);
                }
            }
            else
            {
                node->IndirectionLevel = 0;
            }

            lcddlToken_t identifier = lcddl_GetToken(stream);
            if (identifier.Type != LCDDL_TOKEN_TYPE_IDENTIFIER)
            {
                fprintf(stderr, "Expected an identifier.\n");
                lcddlUserCleanupCallback();
                exit(-1);
            }

            memcpy(node->Name,
                   identifier.Value.String, 32);

            if (parent)
            {
                if (!(parent->Children))
                {
                    parent->Children = node;
                }
                else
                {
                    lcddlNode_t *head;
                    for (head = parent->Children;
                         head->Next;
                         head = head->Next);

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
                    fprintf(stderr, "Nested structs are not allowed.\n");
                    lcddlUserCleanupCallback();
                    exit(-1);
                }
                if (lcddl_GetToken(stream).Type != LCDDL_TOKEN_TYPE_OPEN_CURLY_BRACKET)
                {
                    fprintf(stderr, "Struct declarations must be followed by a"
                                    "curly bracket block.\n");
                    lcddlUserCleanupCallback();
                    exit(-1);
                }
                parent = node;
            }
            else
            {
                if(lcddl_GetToken(stream).Type != LCDDL_TOKEN_TYPE_SEMICOLON)
                {
                    fprintf(stderr, "Expected a semicolon.\n");
                    lcddlUserCleanupCallback();
                    exit(-1);
                }
            }
        }
        else if (token.Type == LCDDL_TOKEN_TYPE_CLOSE_CURLY_BRACKET)
        {
            parent = NULL;
        }
        else
        {
            fprintf(stderr, "ERROR: unexpected token %s.\n",
                    lcddl_TokenTypeToString(token.Type));
            lcddlUserCleanupCallback();
            exit(-1);
        }

        
    }

    return topLevelFirst;
}

int main(int argc, char **argv)
{
    lcddlUserInitCallback();

    int i;
    for (i = 1; i < argc; ++i)
    {
        lcddlLexerInputStream_t file;
        lcddl_ReadFile(argv[i], &file);

        lcddlNode_t *ast = lcddl_Parse(&file);

        lcddlNode_t *node = ast;
        lcddlUserTopLevelCallback(node);
        while (node->Next)
        {
            node = node->Next;
            lcddlUserTopLevelCallback(node);
        }

        node = ast;
        while (node)
        {
            lcddlNode_t *prev = node;
            node = node->Next;

            if (prev->Children)
            {
                lcddlNode_t *child = prev->Children;
                while (child)
                {
                    lcddlNode_t *childPrev = child;
                    child = child->Next;
                    free(childPrev);
                }
            }

            if (prev->Tags)
            {
                lcddlAnnotation_t *tag = prev->Tags;
                while (tag)
                {
                    lcddlAnnotation_t *tagPrev = tag;
                    tag = tag->Next;
                    free(tagPrev);
                }
            }

            free(prev);
        }

        free(file.ContentsStart);
    }

    lcddlUserCleanupCallback();
}

/*****************************************************************/
/* utility functions to assist introspection and code generation */
/*****************************************************************/

void lcddl_WriteStructToFileAsC(FILE *file, lcddlNode_t *node)
{
    fprintf(file, "typedef struct %s %s;\n", node->Name, node->Name);
    fprintf(file, "struct %s\n{\n", node->Name);

    lcddlNode_t *member;
    for (member = node->Children;
         member;
         member = member->Next)
    {
        fprintf(file, "    %s ", member->Type);

        int i;
        for (i = 0; i < member->IndirectionLevel; ++i)
            fprintf(file, "*");

        fprintf(file, "%s;\n", member->Name);
    }

    fprintf(file, "};\n\n");
}

uint8_t lcddl_NodeHasTag(lcddlNode_t *node, char *tag)
{
    lcddlAnnotation_t *annotation;
    for (annotation = node->Tags;
         annotation;
         annotation = annotation->Next)
    {
        return !strcmp(annotation->Tag, tag);
    }
    return 0;
}
