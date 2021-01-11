#ifndef LCDDL_H
#define LCDDL_H

#define internal static

typedef struct LcddlNode LcddlNode;
struct LcddlNode
{
    LcddlNode *first_child, *first_annotation;
    union
    {
        LcddlNode *next_sibling;
        LcddlNode *next_annotation;
    };
    char *name, *type, *value;
    unsigned int array_count; // NOTE(tbt): the number of elements in the array. normal declarations have an `array_count` of 0
    unsigned int indirection_level; // NOTE(tbt): the indirection level of the pointer. normal declarations have an `indirection level` of 0
};

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    #define LCDDL_CALLBACK __declspec(dllexport)
#else
    #define LCDDL_CALLBACK
#endif

LCDDL_CALLBACK void lcddl_user_init_callback(void);
LCDDL_CALLBACK void lcddl_user_top_level_callback(char *file, LcddlNode *node);
LCDDL_CALLBACK void lcddl_user_cleanup_callback(void);

#endif
