#include <stdio.h>

#include "lcddl.h"

//sample function to recursively print the tree
internal void
print_tree(LcddlNode *root,
           int indent)
{
    for (int i = indent;
         i;
         --i)
    {
        putc(' ', stderr);
    }
    fprintf(stderr, root->name);
    if (root->type)
    {
        if (root->array_count)
        {
            fprintf(stderr, " : [%u]%s", root->array_count, root->type);
        }
        else
        {
            fprintf(stderr, " : %s", root->type);
        }

        if (root->indirection_level)
        {
            for (int i = root->indirection_level;
                 i;
                 --i)
            {
                putc('*', stderr);
            }
        }
    }
    if (root->value)
    {
        fprintf(stderr, " = %s", root->value);
    }
    putc('\n', stderr);

    for (LcddlNode *child = root->first_child;
         NULL != child;
         child = child->next_sibling)
    {
        print_tree(child, indent + 4);
    }
}

LCDDL_CALLBACK void
lcddl_user_init_callback(void)
{
    // Called once when lcddl is run
    fprintf(stderr, "lcddl init callback\n\n");
}

LCDDL_CALLBACK void
lcddl_user_top_level_callback(char *file,
                              LcddlNode *node)
{
    // Called for every top level node
    print_tree(node, 4);
    putc('\n', stderr);
}

LCDDL_CALLBACK void
lcddl_user_cleanup_callback(void)
{
    // called just before the program exits
    fprintf(stderr, "lcddl cleanup callback\n");
}

