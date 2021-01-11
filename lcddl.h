#ifndef LCDDL_H
#define LCDDL_H

typedef struct LcddlNode LcddlNode;
struct LcddlNode
{
    LcddlNode *first_child, *next_sibling;
    char *identifier, *type, *value;
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
