#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "lcddl.h"

#define LCDDL_WARN(message) fprintf(stderr,                                    \
                                    "\x1b[33mWARNING: "                        \
                                    message                                    \
                                    "\x1b[0m\n")

#define LCDDL_ERROR(line, message) fprintf(stderr,                             \
                                           "\x1b[31mERROR at line %d: "        \
                                           message                             \
                                           "\x1b[0m\n",                        \
                                           line);                              \
                             lcddlUserCleanupCallback();                       \
                             exit(-1)

#define LCDDL_ERRORF(line, message, ...) fprintf(stderr,                       \
                                                 "\x1b[31mERROR at line %d: "  \
                                                 message                       \
                                                 "\x1b[0m\n",                  \
                                                 line,                         \
                                                 __VA_ARGS__);                 \
                             lcddlUserCleanupCallback();                       \
                             exit(-1)

enum
{
    LCDDL_TOKEN_TYPE_NONE,
    LCDDL_TOKEN_TYPE_UNKNOWN,
    LCDDL_TOKEN_TYPE_EOF,
    LCDDL_TOKEN_TYPE_TYPENAME,
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
    int LineNumber;
} lcddlToken_t;

static char *
lcddlTokenTypeToString(int type)
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
lcddlFilePeekChar(FILE *file)
{
    char c = getc(file);
    return c == EOF ? EOF : ungetc(c, file);
}

static int
lcddlReadIntFromFile(FILE *file,
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
lcddlGetToken(FILE *file)
{
    static char expectingIdentifier = 0;
    static int lineCount = 1;
    static int lineNumber = 1;

    int lastChar = fgetc(file);

    if (lastChar == EOF)
    {
        LCDDL_ERROR(0, "Unexpected EOF");
    }
    else if (lastChar == '\n')
    {
        ++lineCount;
    }

    /* NOTE(tbt): skip over white space */
    while (isspace((unsigned char)lastChar))
    {
        lastChar = fgetc(file);
        if (lastChar == EOF)
        {
            lcddlToken_t token = { .Type = LCDDL_TOKEN_TYPE_EOF };
            return token;
        }
        else if (lastChar == '\n')
        {
            ++lineCount;
        }
    }
    
    if (isalpha(lastChar) ||
        lastChar == '_'   ||
        lastChar == '@')
    {
        lcddlToken_t token;

        uint8_t isAnnotation = (lastChar == '@');

        int identifierLength = 1;
        char identifier[32];

        identifier[identifierLength - 1] = lastChar;
        ++identifierLength;

        while ((isalnum(lcddlFilePeekChar(file)) ||
               lcddlFilePeekChar(file) == '_'    ||
               lcddlFilePeekChar(file) == '-')   &&
               identifierLength <= 32)

        {
            lastChar = fgetc(file);
            identifier[identifierLength - 1] = lastChar;
            ++identifierLength;
        }

        identifier[identifierLength - 1] = 0;
        memcpy(token.Value.String,
               identifier, 32);

        token.LineNumber = lineNumber;
        lineNumber = lineCount;

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
            expectingIdentifier = 1;
            return token;
        }
    }
    else if (lastChar == ';')
    {
        lcddlToken_t token = { .Type = LCDDL_TOKEN_TYPE_SEMICOLON,
                               .LineNumber = lineNumber};
        lineNumber = lineCount;
        return token;
    }
    else if (lastChar == '{')
    {
        lcddlToken_t token = { .Type = LCDDL_TOKEN_TYPE_OPEN_CURLY_BRACKET,
                               .LineNumber = lineNumber};
        lineNumber = lineCount;
        return token;
    }
    else if (lastChar == '}')
    {
        lcddlToken_t token = { .Type = LCDDL_TOKEN_TYPE_CLOSE_CURLY_BRACKET,
                               .LineNumber = lineNumber};
        lineNumber = lineCount;
        return token;
    }
    else if (lastChar == '*')
    {
        lcddlToken_t token = { .Type = LCDDL_TOKEN_TYPE_ASTERIX,
                               .LineNumber = lineNumber,
                               .Value.Integer = 1
                             };
        lineNumber = lineCount;
        while(lcddlFilePeekChar(file) == '*')
        {
            lastChar = fgetc(file);
            ++token.Value.Integer;
        }
        return token;
    }
    else if (lastChar == '[')
    {
        if (isdigit(lcddlFilePeekChar(file)))
        {
            uint32_t arrayCount;
            arrayCount = lcddlReadIntFromFile(file, &lastChar);

            if (lastChar != ']')
            {
                LCDDL_ERRORF(lineCount,
                             "Expected ']'. Instead got: '%c'.",
                             lastChar);
            }

            lcddlToken_t token = { .Type = LCDDL_TOKEN_TYPE_SQUARE_BRACKETS,
                                   .LineNumber = lineNumber,
                                   .Value.Integer = arrayCount
                                 };
            lineNumber = lineCount;
            return token;
        }
        else
        {
            LCDDL_ERROR(lineCount, "Expected a numeric value.");
        }
    }
    else if (lastChar == '#')
    {
        for (;(lastChar = fgetc(file)) != '\n';);
        ++lineCount;
        return lcddlGetToken(file);
    }
    else
    {
        lcddlToken_t token = { .Type = LCDDL_TOKEN_TYPE_UNKNOWN,
                               .LineNumber = lineNumber};
        lineNumber = lineCount;
        return token;
    }

}

static lcddlNode_t *
lcddlParse(FILE *file)
{
    lcddlNode_t *topLevelFirst = NULL;
    lcddlNode_t *topLevelLast = NULL;

    lcddlNode_t *parent = NULL;

    lcddlAnnotation_t *annotations = NULL;

    lcddlToken_t token = lcddlGetToken(file);
    for (; token.Type != LCDDL_TOKEN_TYPE_EOF; token = lcddlGetToken(file))
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

            node->Tags = annotations;
            node->Children = NULL;
            node->Next = NULL;
            annotations = NULL;
            node->IndirectionLevel = 0;
            node->ArrayCount = 1;

            memcpy(node->Type,
                   token.Value.String, 32);

            token = lcddlGetToken(file);

            if (token.Type == LCDDL_TOKEN_TYPE_ASTERIX) 
            {
                node->IndirectionLevel = token.Value.Integer;
                token = lcddlGetToken(file);
            }

            if (token.Type == LCDDL_TOKEN_TYPE_IDENTIFIER)
            {
                memcpy(node->Name,
                       token.Value.String, 32);
                token = lcddlGetToken(file);
            }
            else
            {
                LCDDL_ERROR(token.LineNumber, "Expected an identifier.");
            }

            if (token.Type == LCDDL_TOKEN_TYPE_SQUARE_BRACKETS)
            {
                node->ArrayCount = token.Value.Integer;
                token = lcddlGetToken(file);
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
                    LCDDL_ERROR(token.LineNumber,
                                "Nested structs are not allowed.");
                }
                if (token.Type != LCDDL_TOKEN_TYPE_OPEN_CURLY_BRACKET)
                {
                    LCDDL_ERROR(token.LineNumber,
                                "Struct declarations must be followed by a "
                                "curly bracket block.");
                }
                parent = node;
            }
            else
            {
                if(token.Type != LCDDL_TOKEN_TYPE_SEMICOLON)
                {
                    LCDDL_ERROR(token.LineNumber,
                                "Expected a semicolon.");
                }
            }
        }
        else if (token.Type == LCDDL_TOKEN_TYPE_CLOSE_CURLY_BRACKET)
        {
            parent = NULL;
        }
        else
        {
            LCDDL_ERRORF(token.LineNumber,
                         "Unexpected token %s.",
                         lcddlTokenTypeToString(token.Type));
        }

        
    }

    if (parent)
    {
        LCDDL_WARN("Missing '}'");
    }

    return topLevelFirst;
}

static void
lcddlFreeNode(lcddlNode_t *node)
{
    lcddlNode_t *child = node->Children;
    while (child)
    {
        lcddlNode_t *prev = child;
        child = child->Next;
        lcddlFreeNode(prev);
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
#ifdef _WIN32
#include "windowsLogInit.c"
#endif
    lcddlUserInitCallback();

    int i;
    for (i = 1; i < argc; ++i)
    {
        FILE *file = fopen(argv[i], "rt");
        if (!file)
        {
            LCDDL_ERRORF(0, "Could not open input file: %s", argv[i]);
        }

        lcddlNode_t *ast = lcddlParse(file);

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

            lcddlFreeNode(prev);
        }

        fclose(file);
    }

    lcddlUserCleanupCallback();
#ifdef _WIN32
#include "windowsLogDeinit.c"
#endif
}

/*****************************************************************************/
/* utility functions to assist introspection and code generation             */
/*****************************************************************************/

static void
lcddlWriteFieldToFileAsC(FILE *file,
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
lcddlWriteStructToFileAsC(FILE *file,
                           lcddlNode_t *node)
{
    fprintf(file, "typedef struct %s %s;\n", node->Name, node->Name);
    fprintf(file, "struct %s\n{\n", node->Name);

    lcddlNode_t *member;
    for (member = node->Children;
         member;
         member = member->Next)
    {
        lcddlWriteFieldToFileAsC(file, member, 4);
    }

    fprintf(file, "};\n");
}

void
lcddlWriteNodeToFileAsC(FILE *file,
                         lcddlNode_t *node)
{
    if (node->Children)
    {
        lcddlWriteStructToFileAsC(file, node);
    }
    else
    {
        lcddlWriteFieldToFileAsC(file, node, 0);
    }
    fprintf(file, "\n");
}

uint8_t lcddlNodeHasTag(lcddlNode_t *node, char *tag)
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
