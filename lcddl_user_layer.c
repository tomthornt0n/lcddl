#include <stdio.h>
#include "lcddl.h"

internal void
print_ast(int indent,
          LcddlNode *root)
{
    for (int i = indent; i; --i) { putc('\t', stderr); }

    switch(root->kind)
    {
        case LCDDL_NODE_KIND_root:
        {
            fprintf(stderr, "root\n");
            for (LcddlNode *child = root->first_child;
                 NULL != child;
                 child = child->next_sibling)
            {
                print_ast(indent + 1, child);
            }
            break;
        }
        case LCDDL_NODE_KIND_declaration:
        {
            fprintf(stderr, "declaration: %s\n", root->declaration.name);

            if (root->first_annotation)
            {
                for (int i = indent; i; --i) { putc('\t', stderr); }
                fprintf(stderr, " ~annotations:\n");
                for (LcddlNode *annotation = root->first_annotation;
                     NULL != annotation;
                     annotation = annotation->next_annotation)
                {
                    print_ast(indent + 1, annotation);
                }
            }

            if (root->declaration.type)
            {
                for (int i = indent; i; --i) { putc('\t', stderr); }
                fprintf(stderr, " ~type:\n");
                print_ast(indent + 1, root->declaration.type);
            }

            if (root->declaration.value)
            {
                for (int i = indent; i; --i) { putc('\t', stderr); }
                fprintf(stderr, " ~value:\n");
                print_ast(indent + 1, root->declaration.value);
            }

            if (root->first_child)
            {
                for (int i = indent; i; --i) { putc('\t', stderr); }
                fprintf(stderr, " ~children:\n");
                for (LcddlNode *child = root->first_child;
                     NULL != child;
                     child = child->next_sibling)
                {
                    print_ast(indent + 1, child);
                }
            }
            break;
        }
        case LCDDL_NODE_KIND_type:
        {
            fprintf(stderr, "type: ");
            if (root->type.array_count)
            {
                fprintf(stderr, "[%u]", root->type.array_count);
            }
            fprintf(stderr, "%s", root->type.type_name);
            for (int i = root->type.indirection_level;
                 0 != i;
                 --i)
            {
                putc('*', stderr);
            }
                putc('\n', stderr);
            break;
        }
        case LCDDL_NODE_KIND_annotation:
        {
            fprintf(stderr, "annotation: %s\n", root->annotation.tag);
            if (root->annotation.value)
            {
                for (int i = indent; i; --i) { putc('\t', stderr); }
                fprintf(stderr, " ~value:\n");
                print_ast(indent + 1, root->annotation.value);
            }
            break;
        }
        case LCDDL_NODE_KIND_file:
        {
            fprintf(stderr, "file: %s\n", root->file.filename);
            for (LcddlNode *child = root->first_child;
                 NULL != child;
                 child = child->next_sibling)
            {
                print_ast(indent + 1, child);
            }
            break;
        }
        case LCDDL_NODE_KIND_float_literal:
        {
            fprintf(stderr, "float literal: %s\n", root->literal.value);
            break;
        }
        case LCDDL_NODE_KIND_integer_literal:
        {
            fprintf(stderr, "integer literal: %s\n", root->literal.value);
            break;
        }
        case LCDDL_NODE_KIND_string_literal:
        {
            fprintf(stderr, "string literal: %s\n", root->literal.value);
            break;
        }
        case LCDDL_NODE_KIND_variable_reference:
        {
            fprintf(stderr, "variable reference: %s\n", root->var_reference.name);
            break;
        }
        case LCDDL_NODE_KIND_binary_operator:
        {
            fprintf(stderr, "binary operator: %s\n", lcddl_operator_kind_to_string(root->binary_operator.kind));
            for (int i = indent; i; --i) { putc('\t', stderr); }
            fprintf(stderr, " ~left:\n");
            print_ast(indent + 1, root->binary_operator.left);
            for (int i = indent; i; --i) { putc('\t', stderr); }
            fprintf(stderr, " ~right:\n");
            print_ast(indent + 1, root->binary_operator.right);
            break;
        }
        case LCDDL_NODE_KIND_unary_operator:
        {
            fprintf(stderr, "unary operator: %s\n", lcddl_operator_kind_to_string(root->unary_operator.kind));
            print_ast(indent + 1, root->unary_operator.operand);
            break;
        }
    }
}

LCDDL_CALLBACK void
lcddl_user_callback(LcddlNode *root)
{
    print_ast(0, root);
}

