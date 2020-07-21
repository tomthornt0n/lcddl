#ifndef LCDDL_H
#define LCDDL_H


typedef struct lcddlNode_t lcddlNode_t;
struct lcddlNode_t
{
    char Name[32];
    char Type[32];
    int IndirectionLevel;

    lcddlNode_t *Next;

    lcddlNode_t *Child;
    lcddlNode_t *Parent;
};

void lcddlUserInitCallback(void);
void lcddlUserTopLevelCallback(lcddlNode_t *node);
void lcddlUserCleanupCallback(void);

#endif
