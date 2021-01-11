#include <stdio.h>

#include "lcddl.h"

//sample function to recursively print the tree
internal void
print_tree(LcddlNode *root,
           int indent)
{
    for (int i = indent; i; --i)
    {
        putc(' ', stderr);
    }

    if (root->first_annotation)
    {
        for (LcddlNode *annotation = root->first_annotation;
             NULL != annotation;
             annotation = annotation->next_annotation)
        {
            fprintf(stderr, "@%s", annotation->name);
            if (annotation->value)
            {
                fprintf(stderr, "(%s)", annotation->value);
            }
            fprintf(stderr, "; ");
        }
        putc('\n', stderr);
        for (int i = indent; i; --i)
        {
            putc(' ', stderr);
        }
    }

    fprintf(stderr, "%s", root->name);

    if (root->type)
    {
        fprintf(stderr, " : ");
        if (root->array_count)
        {
            fprintf(stderr, "[%u]", root->array_count);
        }
        fprintf(stderr, "%s", root->type);

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

