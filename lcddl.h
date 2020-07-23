#ifndef LCDDL_H
#define LCDDL_H

#include <stdint.h>

typedef struct lcddlAnnotation_t lcddlAnnotation_t;
struct lcddlAnnotation_t
{
    char Tag[32];
    lcddlAnnotation_t *Next;
};

typedef struct lcddlNode_t lcddlNode_t;
struct lcddlNode_t
{
    char Name[32];
    char Type[32];
    int IndirectionLevel;

    lcddlNode_t *Next;
    lcddlNode_t *Children;

    lcddlAnnotation_t *Tags;
};

void lcddlUserInitCallback(void);
void lcddlUserTopLevelCallback(lcddlNode_t *node);
void lcddlUserCleanupCallback(void);

void lcddl_WriteStructToFileAsC(FILE *file, lcddlNode_t *node);
uint8_t lcddl_NodeHasTag(lcddlNode_t *node, char *tag);

#endif
