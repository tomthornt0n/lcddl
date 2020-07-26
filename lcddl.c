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
    LCDDL_TOKEN_TYPE_ANNOTATION,
    LCDDL_TOKEN_TYPE_SQUARE_BRACKETS
};

typedef struct 
{
    union
    {
        char String[32];
        uint32_t Integer;
    } Value;
    int Type;
} lcddlToken_t;

static char *
lcddl_TokenTypeToString(int type)
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
        case LCDDL_TOKEN_TYPE_ANNOTATION:
            return "TOKEN_TYPE_ANNOTATION";
        case LCDDL_TOKEN_TYPE_SQUARE_BRACKETS:
            return "TOKEN_TYPE_SQUARE_BRACKETS";
        default:
            return "\x1b[31mERROR converting to string: "
                   "Unknown token type\x1b[0m";
    }
}

static char
lcddl_FilePeekChar(FILE *file)
{
    char c = getc(file);
    return c == EOF ? EOF : ungetc(c, file);
}

static int
lcddl_ReadIntFromFile(FILE *file,
                      int *lastChar)
{
    int result = 0;

    char c = fgetc(file);
    while (c >= 48 &&
           c <= 57)
    {
        result *= 10;
        result += c - 48;
        c = fgetc(file);
    }

    *lastChar = c;
    return result;
}

static lcddlToken_t
lcddl_GetToken(FILE *file)
{
    int lastChar = fgetc(file);

    if (lastChar == EOF)
    {
        fprintf(stderr,
                "\x1b[31mERROR: Unexpected EOF.\x1b[0m\n");
        exit(-1);
    }

    static char expectingIdentifier = 0;

    /* NOTE(tbt): skip over white space */
    while (isspace((unsigned char)lastChar))
    {
        lastChar = fgetc(file);
        if (lastChar == EOF)
        {
            lcddlToken_t token = { .Type = LCDDL_TOKEN_TYPE_EOF };
            return token;
        }
    }
    
    if (isalpha(lastChar) ||
        lastChar == '_'   ||
        lastChar == '@')
    {
        lcddlToken_t token;

        uint8_t isAnnotation = (lastChar == '@');

        int identifierLength = 1;
        char *identifier = malloc(sizeof*identifier);

        identifier[identifierLength - 1] = lastChar;
        identifier = realloc(identifier, ++identifierLength *
                                         sizeof *identifier);

        while (isalnum(lcddl_FilePeekChar(file)) ||
               lcddl_FilePeekChar(file) == '_')
        {
            lastChar = fgetc(file);
            identifier[identifierLength - 1] = lastChar;
            identifier = realloc(identifier, ++identifierLength *
                                             sizeof *identifier);
        }

        identifier[identifierLength - 1] = 0;
        memcpy(token.Value.String,
               identifier, identifierLength < 32 ? identifierLength : 32);

        free(identifier);

        if (isAnnotation)
        {
            token.Type = LCDDL_TOKEN_TYPE_ANNOTATION;
            return token;
        }
        else if (expectingIdentifier)
        {
            token.Type = LCDDL_TOKEN_TYPE_IDENTIFIER;
            expectingIdentifier = 0;
            return token;
        }
        else
        {
            token.Type = LCDDL_TOKEN_TYPE_TYPENAME;
            return token;
        }
    }
    else if (lastChar == ':')
    {
        lcddlToken_t token = { .Type = LCDDL_TOKEN_TYPE_COLON };
        expectingIdentifier = 1;
        return token;
    }
    else if (lastChar == ';')
    {
        lcddlToken_t token = { .Type = LCDDL_TOKEN_TYPE_SEMICOLON };
        return token;
    }
    else if (lastChar == '{')
    {
        lcddlToken_t token = { .Type = LCDDL_TOKEN_TYPE_OPEN_CURLY_BRACKET };
        return token;
    }
    else if (lastChar == '}')
    {
        lcddlToken_t token = { .Type = LCDDL_TOKEN_TYPE_CLOSE_CURLY_BRACKET };
        return token;
    }
    else if (lastChar == '*')
    {
        lcddlToken_t token = { .Type = LCDDL_TOKEN_TYPE_ASTERIX,
                               .Value.Integer = 1
                             };
        while(lcddl_FilePeekChar(file) == '*')
        {
            lastChar = fgetc(file);
            ++token.Value.Integer;
        }
        return token;
    }
    else if (lastChar == '[')
    {
        if (isdigit(lcddl_FilePeekChar(file)))
        {
            uint32_t arrayCount;
            arrayCount = lcddl_ReadIntFromFile(file, &lastChar);
            if (lastChar != ']')
            {
                fprintf(stderr,
                        "\x1b[31m"
                        "ERROR: Expected ']'. Instead got: '%c'."
                        "\x1b[0m\n",
                        lastChar);
                lcddlUserCleanupCallback();
                exit(-1);
            }

            lcddlToken_t token = { .Type = LCDDL_TOKEN_TYPE_SQUARE_BRACKETS,
                                   .Value.Integer = arrayCount
                                 };
            return token;
        }
        else
        {
            fprintf(stderr,
                    "\x1b[31mERROR: expected a numeric value.\x1b[0m\n");
            lcddlUserCleanupCallback();
            exit(-1);
        }
    }
    else if (lastChar == '#')
    {
        for (;(lastChar = fgetc(file)) != '\n';);
        return lcddl_GetToken(file);
    }
    else
    {
        lcddlToken_t token = { .Type = LCDDL_TOKEN_TYPE_UNKNOWN };
        fprintf(stderr, "%s\n", lcddl_TokenTypeToString(token.Type));
        return token;
    }

}

static lcddlNode_t *lcddl_Parse(FILE *file)
{
    lcddlNode_t *topLevelFirst = NULL;
    lcddlNode_t *topLevelLast = NULL;

    lcddlNode_t *parent = NULL;

    lcddlAnnotation_t *annotations = NULL;

    lcddlToken_t token = lcddl_GetToken(file);
    for (; token.Type != LCDDL_TOKEN_TYPE_EOF; token = lcddl_GetToken(file))
    {
        if (token.Type == LCDDL_TOKEN_TYPE_ANNOTATION)
        {
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

            node->IndirectionLevel = 0;
            node->ArrayCount = 1;

            node->Tags = annotations;
            annotations = NULL;

            memcpy(node->Type,
                   token.Value.String, 32);

            token = lcddl_GetToken(file);

            if (token.Type == LCDDL_TOKEN_TYPE_ASTERIX) 
            {
                node->IndirectionLevel = token.Value.Integer;
                token = lcddl_GetToken(file);
            }

            if (token.Type == LCDDL_TOKEN_TYPE_SQUARE_BRACKETS)
            {
                node->ArrayCount = token.Value.Integer;
                token = lcddl_GetToken(file);
            }

            if (token.Type == LCDDL_TOKEN_TYPE_COLON)
            {
                lcddlToken_t identifier = lcddl_GetToken(file);
                if (identifier.Type != LCDDL_TOKEN_TYPE_IDENTIFIER)
                {
                    fprintf(stderr,
                            "\x1b[31mERROR: Expected an identifier.\x1b[0m\n");
                    lcddlUserCleanupCallback();
                    exit(-1);
                }

                memcpy(node->Name,
                       identifier.Value.String, 32);
            }
            else
            {
                fprintf(stderr,
                        "\x1b[31mERROR: Unexpected token: %s.\x1b[0m\n",
                        lcddl_TokenTypeToString(token.Type));
                lcddlUserCleanupCallback();
                exit(-1);
            }


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
                    fprintf(stderr,
                            "\x1b[31m"
                            "ERROR: Nested structs are not allowed."
                            "\x1b[0m\n");
                    lcddlUserCleanupCallback();
                    exit(-1);
                }
                if (lcddl_GetToken(file).Type != LCDDL_TOKEN_TYPE_OPEN_CURLY_BRACKET)
                {
                    fprintf(stderr,
                            "\x1b[31m"
                            "ERROR: Struct declarations must be followed by a"
                            "curly bracket block."
                            "\x1b[0m\n");
                    lcddlUserCleanupCallback();
                    exit(-1);
                }
                parent = node;
            }
            else
            {
                if(lcddl_GetToken(file).Type != LCDDL_TOKEN_TYPE_SEMICOLON)
                {
                    fprintf(stderr,
                            "\x1b[31mERROR: Expected a semicolon.\x1b[0m\n");
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
            fprintf(stderr,
                    "\x1b[31mERROR: Unexpected token %s.\x1b[0m\n",
                    lcddl_TokenTypeToString(token.Type));
            lcddlUserCleanupCallback();
            exit(-1);
        }

        
    }

    return topLevelFirst;
}

static void
lcddl_FreeNode(lcddlNode_t *node)
{
    lcddlNode_t *child = node->Children;
    while (child)
    {
        lcddlNode_t *prev = child;
        child = child->Next;
        lcddl_FreeNode(prev);
    }

    lcddlAnnotation_t *tag = node->Tags;
    while (tag)
    {
        lcddlAnnotation_t *prev = tag;
        tag = tag->Next;
        free(prev);
    }

    free(node);
}

int main(int argc, char **argv)
{
    lcddlUserInitCallback();

    int i;
    for (i = 1; i < argc; ++i)
    {
        FILE *file = fopen(argv[i], "rt");
        if (!file)
        {
            fprintf(stderr, "\x1b[31mERROR: Could not open input file\n\x1b[0m");
            exit(-1);
        }

        lcddlNode_t *ast = lcddl_Parse(file);

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

            lcddl_FreeNode(prev);
        }

        fclose(file);
    }

    lcddlUserCleanupCallback();
}

/*****************************************************************************/
/* utility functions to assist introspection and code generation             */
/*****************************************************************************/

static void
lcddl_WriteFieldToFileAsC(FILE *file,
                          lcddlNode_t *node,
                          int indentation)
{
    int i;

    for (i = 0; i < indentation; ++i)
    {
        fprintf(file, " ");
    }

    fprintf(file,
            "%s ",
            node->Type);

    for (i = 0; i < node->IndirectionLevel; ++i)
    {
        fprintf(file, "*");
    }

    fprintf(file, "%s", node->Name);

    if (node->ArrayCount > 1)
    {
        fprintf(file, "[%d]", node->ArrayCount);
    }

    fprintf(file, ";\n");
}

static void
lcddl_WriteStructToFileAsC(FILE *file,
                           lcddlNode_t *node)
{
    fprintf(file, "typedef struct %s %s;\n", node->Name, node->Name);
    fprintf(file, "struct %s\n{\n", node->Name);

    lcddlNode_t *member;
    for (member = node->Children;
         member;
         member = member->Next)
    {
        lcddl_WriteFieldToFileAsC(file, member, 4);
    }

    fprintf(file, "};\n");
}

void
lcddl_WriteNodeToFileAsC(FILE *file,
                         lcddlNode_t *node)
{
    if (node->Children)
    {
        lcddl_WriteStructToFileAsC(file, node);
    }
    else
    {
        lcddl_WriteFieldToFileAsC(file, node, 0);
    }
    fprintf(file, "\n");
}

uint8_t lcddl_NodeHasTag(lcddlNode_t *node, char *tag)
{
    lcddlAnnotation_t *annotation;
    for (annotation = node->Tags;
         annotation;
         annotation = annotation->Next)
    {
        /* NOTE(tbt): increment the annotation's string to ignore the '@'
           symbol
        */
        if (0 == strcmp(annotation->Tag + 1, tag)) return 1;
    }
    return 0;
}
