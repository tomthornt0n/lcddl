#include <stdio.h>

#include "lcddl.h"

/* LCDDL - 'lucerna data description language'
   
   lcddl is a simple utility which parses a minimal data description format.
   Once a data layout has been extracted from the input file, a corresponding
   tree is constructed and sent to the user code bellow.
   The tree is arranged as a linked list of 'top level' nodes, with structs
   containing a separate list of member fields.
   Each node in the tree consists of a Name, a Type and an indirection level,
   which can be introspected upon when lcddl is run.
*/

void lcddlUserInitCallback(void)
{
    /* Called once at when lcddl is run. */
    fprintf(stderr, "LCDDL Init Callback\n");
}

void lcddlUserTopLevelCallback(lcddlNode_t *node)
{
    /* Called for every 'top level' node */
    fprintf(stderr, "%s: %s\n", node->Type, node->Name);
}

void lcddlUserCleanupCallback(void)
{
    /* called just before the program exits */
    fprintf(stderr, "LCDDL Cleanup Callback\n");
}

