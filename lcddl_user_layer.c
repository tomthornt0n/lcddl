#include <stdio.h>

#include "lcddl.h"

LCDDL_CALLBACK void
lcddl_user_init_callback(void)
{
    /* Called once at when lcddl is run. */
    fprintf(stderr, "lcddl init callback\n");
}

LCDDL_CALLBACK void
lcddl_user_top_level_callback(char *file,
                              LcddlNode *node)
{
    /* Called for every top level node */
    fprintf(stderr, "top level callback in file '%s' for node '%s'", file, node->identifier);
}

LCDDL_CALLBACK void
lcddl_user_cleanup_callback(void)
{
    /* called just before the program exits */
    fprintf(stderr, "lcddl cleanup callback\n");
}

