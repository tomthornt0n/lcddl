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
    uint32_t IndirectionLevel;
    uint32_t ArrayCount;

    lcddlNode_t *Next;
    lcddlNode_t *Children;

    lcddlAnnotation_t *Tags;
};

void lcddlUserInitCallback(void);
void lcddlUserTopLevelCallback(lcddlNode_t *node);
void lcddlUserCleanupCallback(void);

void lcddlWriteNodeToFileAsC(FILE *file, lcddlNode_t *node);
uint8_t lcddlNodeHasTag(lcddlNode_t *node, char *tag);

#endif
