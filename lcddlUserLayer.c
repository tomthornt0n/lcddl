#include <stdio.h>

#include "lcddl.h"

/* LCDDL - 'lucerna data description language'
   
   lcddl is a simple utility which parses a minimal data description format.
   Once a data layout has been extracted from the input file, a corresponding
   tree is constructed and sent to the user code.
   
   The tree is arranged as a linked list of 'top level' nodes, each of which
   contains a Name, a Type, a IndirectionLevel, a pointer, which may be NULL,
   to a linked list of child nodes, and a pointer, which may be NULL, to a
   linked list of annotations.

   lcddlNode_t is defined as:

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

   lcddlAnnotation_t is defined as:
   
    typedef struct lcddlAnnotation_t lcddlAnnotation_t;
    struct lcddlAnnotation_t
    {
        char Tag[32];
        lcddlAnnotation_t *Next;
    };
*/

void lcddlUserInitCallback(void)
{
    /* Called once at when lcddl is run. */
}

void lcddlUserTopLevelCallback(lcddlNode_t *node)
{
    /* Called for every 'top level' node */
}

void lcddlUserCleanupCallback(void)
{
    /* called just before the program exits */
}

