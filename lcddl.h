#ifndef LCDDL_H
#define LCDDL_H


typedef struct node_t node_t;
struct node_t
{
    char Name[32];
    char Type[32];
    int IndirectionLevel;

    node_t *Next;

    node_t *Child;
    node_t *Parent;
};

void lcddlUserInitCallback(void);
void lcddlUserTopLevelCallback(node_t *node);
void lcddlUserCleanupCallback(void);

#endif
