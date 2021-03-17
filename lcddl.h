#ifndef LCDDL_H
#define LCDDL_H

#include <stdbool.h>

typedef enum
{
 // binary operators
 LCDDL_BINARY_OPERATOR_BEGIN,
 LCDDL_BIN_OP_KIND_multiply,                 // '*'
 LCDDL_BIN_OP_KIND_divide,                   // '/'
 LCDDL_BIN_OP_KIND_add,                      // '+'
 LCDDL_BIN_OP_KIND_subtract,                 // '-'
 LCDDL_BIN_OP_KIND_bit_shift_left,           // '<<'
 LCDDL_BIN_OP_KIND_bit_shift_right,          // '>>'
 LCDDL_BIN_OP_KIND_lesser_than,              // '<'
 LCDDL_BIN_OP_KIND_greater_than,             // '>'
 LCDDL_BIN_OP_KIND_lesser_than_or_equal_to,  // '<='
 LCDDL_BIN_OP_KIND_greater_than_or_equal_to, // '>='
 LCDDL_BIN_OP_KIND_equality,                 // '=='
 LCDDL_BIN_OP_KIND_not_equal_to,             // '!='
 LCDDL_BIN_OP_KIND_bitwise_and,              // '&'
 LCDDL_BIN_OP_KIND_bitwise_xor,              // '^'
 LCDDL_BIN_OP_KIND_bitwise_or,               // '|'
 LCDDL_BIN_OP_KIND_boolean_and,              // '&&'
 LCDDL_BIN_OP_KIND_boolean_or,               // '||'
 LCDDL_BINARY_OPERATOR_END,
 
 // unary operators
 LCDDL_UNARY_OPERATOR_BEGIN,
 LCDDL_UN_OP_KIND_positive,                  // '+'
 LCDDL_UN_OP_KIND_negative,                  // '-'
 LCDDL_UN_OP_KIND_bitwise_not,               // '~'
 LCDDL_UN_OP_KIND_boolean_not,               // '!'
 LCDDL_UNARY_OPERATOR_END,
} LcddlOperatorKind;

#define lcddl_operator_kind_to_string(_op_kind) \
((_op_kind) == LCDDL_BIN_OP_KIND_multiply                 ? "*"  :         \
(_op_kind) == LCDDL_BIN_OP_KIND_divide                   ? "/"  :         \
(_op_kind) == LCDDL_BIN_OP_KIND_add                      ? "+"  :         \
(_op_kind) == LCDDL_BIN_OP_KIND_subtract                 ? "-"  :         \
(_op_kind) == LCDDL_BIN_OP_KIND_bit_shift_left           ? "<<" :         \
(_op_kind) == LCDDL_BIN_OP_KIND_bit_shift_right          ? ">>" :         \
(_op_kind) == LCDDL_BIN_OP_KIND_lesser_than              ? "<"  :         \
(_op_kind) == LCDDL_BIN_OP_KIND_greater_than             ? ">"  :         \
(_op_kind) == LCDDL_BIN_OP_KIND_lesser_than_or_equal_to  ? "<=" :         \
(_op_kind) == LCDDL_BIN_OP_KIND_greater_than_or_equal_to ? ">=" :         \
(_op_kind) == LCDDL_BIN_OP_KIND_equality                 ? "==" :         \
(_op_kind) == LCDDL_BIN_OP_KIND_not_equal_to             ? "!=" :         \
(_op_kind) == LCDDL_BIN_OP_KIND_bitwise_and              ? "&"  :         \
(_op_kind) == LCDDL_BIN_OP_KIND_bitwise_xor              ? "^"  :         \
(_op_kind) == LCDDL_BIN_OP_KIND_bitwise_or               ? "|"  :         \
(_op_kind) == LCDDL_BIN_OP_KIND_boolean_and              ? "&&" :         \
(_op_kind) == LCDDL_BIN_OP_KIND_boolean_or               ? "||" :         \
(_op_kind) == LCDDL_UN_OP_KIND_positive                  ? "+"  :         \
(_op_kind) == LCDDL_UN_OP_KIND_negative                  ? "-"  :         \
(_op_kind) == LCDDL_UN_OP_KIND_bitwise_not               ? "~"  :         \
(_op_kind) == LCDDL_UN_OP_KIND_boolean_not               ? "!"  : NULL)

typedef enum
{
 LCDDL_NODE_KIND_root,               // the type passed to the user callback. each child represents a file
 LCDDL_NODE_KIND_file,               // represents a file passed as input
 LCDDL_NODE_KIND_declaration,        // declaration of a variable or struct or similar
 LCDDL_NODE_KIND_type,               // used as the type field of a declaration
 LCDDL_NODE_KIND_binary_operator,    // an operator with two operands used in an expression
 LCDDL_NODE_KIND_unary_operator,     // an operator with as single operand used in an expression
 LCDDL_NODE_KIND_string_literal,     // a series of characters enclosed in '"'
 LCDDL_NODE_KIND_float_literal,      // a series of numbers or '.'s begining with a number or '-'
 LCDDL_NODE_KIND_integer_literal,    // a series of numbers begining with a number or '-'
 LCDDL_NODE_KIND_variable_reference, // an identifier used in an expression
 LCDDL_NODE_KIND_annotation,         // extre meta information added to a statement
} LcddlNodeKind;

typedef struct LcddlNode LcddlNode;
struct LcddlNode
{
 LcddlNodeKind kind;
 LcddlNode *first_child, *first_annotation;
 union
 {
  LcddlNode *next_sibling;
  LcddlNode *next_annotation;
 };
 
 union
 {
  struct
  {
   char *filename;
  } file;
  
  struct
  {
   char *name;
   LcddlNode *type;  // may be ommited and left NULL. otherwise is a type
   LcddlNode *value; // may be ommited and left NULL. otherwise is an expression
  } declaration;
  
  struct
  {
   char *type_name;
   unsigned int array_count;       // the number of elements in the array. normal declarations have an `array_count` of 0
   unsigned int indirection_level; // the indirection level of the pointer. normal declarations have an `indirection_level` of 0
  } type;
  
  struct
  {
   LcddlOperatorKind kind;
   LcddlNode *left;
   LcddlNode *right;
  } binary_operator;
  
  struct
  {
   LcddlOperatorKind kind;
   LcddlNode *operand;
  } unary_operator;
  
  struct
  {
   char *value;
  } literal;
  
  struct
  {
   char *name;
  } var_reference;
  
  struct
  {
   char *tag;
   LcddlNode *value;
  } annotation;
 };
};

typedef struct LcddlSearchResult LcddlSearchResult;
struct LcddlSearchResult
{
 LcddlSearchResult *next;
 LcddlNode *node;
};

#ifndef LCDDL_AS_LIBRARY

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define LCDDL_CALLBACK __declspec(dllexport)
#else
#define LCDDL_CALLBACK
#endif

LCDDL_CALLBACK void lcddl_user_callback(LcddlNode *root);

#else

void lcddl_initialise(void);
LcddlNode *lcddl_parse_file(char *filename);
LcddlNode *lcddl_parse_from_memory(char *buffer, unsigned long long buffer_size);
LcddlNode *lcddl_parse_cstring(char *string);
void lcddl_free_file(LcddlNode *root);
#endif

void lcddl_write_node_to_file_as_c_struct(LcddlNode *node, FILE *file);
void lcddl_write_node_to_file_as_c_enum(LcddlNode *node, FILE *file);
LcddlNode *lcddl_get_annotation_value(LcddlNode *node, char *tag);
bool lcddl_does_node_have_tag(LcddlNode *node, char *tag);
LcddlSearchResult *lcddl_find_top_level_declaration(char *name);
LcddlSearchResult *lcddl_find_all_top_level_declarations_with_tag(char *tag);
bool lcddl_is_declaration_type(LcddlNode *declaration, char *type_name);
double lcddl_evaluate_expression(LcddlNode *expression);

#endif
