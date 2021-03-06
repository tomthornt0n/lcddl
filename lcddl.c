#ifndef LCDDL_C
#define LCDDL_C

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lcddl.h"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <windows.h>

// disable ANSI escape codes for colours on windows
// TODO(tbt): setup the virtual terminal on windows to use ANSI escape codes
#define LOG_WARN_BEGIN  "%s:%lu WARNING : "
#define LOG_ERROR_BEGIN "%s:%lu ERROR : "
#else
#include <dlfcn.h>

#define LOG_WARN_BEGIN   "\x1b[33m%s : line %lu : WARNING : \x1b[0m"
#define LOG_ERROR_BEGIN  "\x1b[31m%s : line %lu : ERROR : \x1b[0m"
#endif

///////////////////////////////////////////
// UTILITIES
//~

#define print_warning(_stream, _message) fprintf(stderr,                                \
LOG_WARN_BEGIN                         \
_message                               \
"\n",                                  \
(_stream)->path,                       \
(_stream)->current_token.line);        \

#define print_error_and_exit(_stream, _message) fprintf(stderr,                         \
LOG_ERROR_BEGIN                 \
_message                        \
"\n",                           \
(_stream)->path,                \
(_stream)->current_token.line); \
exit(EXIT_FAILURE)

#define print_error_and_exit_f(_stream, _message, ...) fprintf(stderr,                        \
LOG_ERROR_BEGIN                \
_message                       \
"\n",                          \
(_stream)->path,               \
(_stream)->current_token.line, \
__VA_ARGS__);                  \
exit(EXIT_FAILURE)

static bool
_lcddl_is_char_space(char c)
{
 if (c == ' '  ||
     c == '\t' ||
     c == '\n' ||
     c == '\r' ||
     c == '\f' ||
     c == '\0')
 {
  return true;
 }
 return false;
}

static bool
_lcddl_is_char_alphanumeric(char c)
{
 if ((c >= 'a' && c <= 'z') ||
     (c >= 'A' && c <= 'Z') ||
     (c >= '0' && c <= '9'))
 {
  return true;
 }
 return false;
}

static bool
_lcddl_is_char_number(char c)
{
 if (c >= '0' && c <= '9')
 {
  return true;
 }
 return false;
}

static bool
_lcddl_is_char_letter(char c)
{
 if ((c >= 'a' && c <= 'z') ||
     (c >= 'A' && c <= 'Z'))
 {
  return true;
 }
 return false;
}

///////////////////////////////////////////
// LEXER
//~

typedef enum
{
 TOKEN_KIND_none,
 
 TOKEN_KIND_identifier,               // a series of alphanumeric characters begining with a letter or '_'
 TOKEN_KIND_integer_literal,          // a series of numbers
 TOKEN_KIND_float_literal,            // a series of numbers containing exactly one '.'
 TOKEN_KIND_string_literal,           // a series of characters bounded by '"'
 TOKEN_KIND_colon,                    // ':'
 TOKEN_KIND_equals,                   // '='
 TOKEN_KIND_open_curly_bracket,       // '{'
 TOKEN_KIND_close_curly_bracket,      // '}'
 TOKEN_KIND_open_square_bracket,      // '['
 TOKEN_KIND_close_square_bracket,     // ']'
 TOKEN_KIND_open_bracket,             // '('
 TOKEN_KIND_close_bracket,            // ')'
 TOKEN_KIND_asterisk,                 // '*'
 TOKEN_KIND_dash,                     // '-'
 TOKEN_KIND_slash,                    // '/'
 TOKEN_KIND_add,                      // '+'
 TOKEN_KIND_tilde,                    // '~'
 TOKEN_KIND_exclamation,              // '!'
 TOKEN_KIND_lesser_than,              // '<'
 TOKEN_KIND_greater_than,             // '>'
 TOKEN_KIND_lesser_than_or_equal_to,  // '<='
 TOKEN_KIND_greater_than_or_equal_to, // '>='
 TOKEN_KIND_equality,                 // '=='
 TOKEN_KIND_not_equal_to,             // '!='
 TOKEN_KIND_bit_shift_left,           // '<<'
 TOKEN_KIND_bit_shift_right,          // '>>'
 TOKEN_KIND_bitwise_and,              // '&'
 TOKEN_KIND_bitwise_or,               // '|'
 TOKEN_KIND_bitwise_xor,              // '^'
 TOKEN_KIND_boolean_and,              // '&&'
 TOKEN_KIND_boolean_or,               // '||'
 TOKEN_KIND_at_symbol,                // '@'
 TOKEN_KIND_semicolon,                // ';'
 TOKEN_KIND_eof,                      // EOF
} _LcddlTokenKind;

#define token_kind_to_string(_token_kind)                                      \
((_token_kind) == TOKEN_KIND_none                     ? "none" :           \
(_token_kind) == TOKEN_KIND_identifier               ? "identifier" :     \
(_token_kind) == TOKEN_KIND_integer_literal          ? "integer literal" :\
(_token_kind) == TOKEN_KIND_float_literal            ? "float literal" :  \
(_token_kind) == TOKEN_KIND_string_literal           ? "string literal" : \
(_token_kind) == TOKEN_KIND_colon                    ? ":" :              \
(_token_kind) == TOKEN_KIND_equals                   ? "=" :              \
(_token_kind) == TOKEN_KIND_open_curly_bracket       ? "{" :              \
(_token_kind) == TOKEN_KIND_close_curly_bracket      ? "}" :              \
(_token_kind) == TOKEN_KIND_open_square_bracket      ? "[" :              \
(_token_kind) == TOKEN_KIND_close_square_bracket     ? "]" :              \
(_token_kind) == TOKEN_KIND_open_bracket             ? "(" :              \
(_token_kind) == TOKEN_KIND_close_bracket            ? ")" :              \
(_token_kind) == TOKEN_KIND_asterisk                 ? "*" :              \
(_token_kind) == TOKEN_KIND_dash                     ? "-" :              \
(_token_kind) == TOKEN_KIND_slash                    ? "/" :              \
(_token_kind) == TOKEN_KIND_add                      ? "+" :              \
(_token_kind) == TOKEN_KIND_tilde                    ? "~" :              \
(_token_kind) == TOKEN_KIND_lesser_than              ? "<" :              \
(_token_kind) == TOKEN_KIND_greater_than             ? ">" :              \
(_token_kind) == TOKEN_KIND_lesser_than_or_equal_to  ? "<=" :             \
(_token_kind) == TOKEN_KIND_greater_than_or_equal_to ? ">=" :             \
(_token_kind) == TOKEN_KIND_equality                 ? "==" :             \
(_token_kind) == TOKEN_KIND_not_equal_to             ? "!=" :             \
(_token_kind) == TOKEN_KIND_bit_shift_left           ? "<<" :             \
(_token_kind) == TOKEN_KIND_bit_shift_right          ? ">>" :             \
(_token_kind) == TOKEN_KIND_at_symbol                ? "@" :              \
(_token_kind) == TOKEN_KIND_semicolon                ? ";" :              \
(_token_kind) == TOKEN_KIND_eof                      ? "eof" : NULL)


typedef struct
{
 _LcddlTokenKind kind;
 unsigned int len;
 char *value;
 unsigned long line;
} _LcddlToken;

#define PATH_MAX_LEN 96

typedef struct
{
 char *buffer;
 unsigned long long size;
 unsigned long long index;
 
 char *path;
 unsigned long current_line;
 
 _LcddlToken current_token;
} _LcddlStream;

static _LcddlToken _lcddl_get_next_token(_LcddlStream *stream);

static _LcddlStream
_lcddl_load_entire_file_as_stream(char *filename)
{
 _LcddlStream result = {0};
 result.current_line = 1;
 result.path         = calloc(1, strlen(filename) + 1);
 strcpy(result.path, filename);
 
 FILE *file = fopen(filename, "rb");
 if(file)
 {
  fseek(file, 0, SEEK_END);
  result.size     = ftell(file);
  fseek(file, 0, SEEK_SET);
  result.buffer   = malloc(result.size);
  if(result.buffer)
  {
   fread(result.buffer, 1, result.size, file);
  }
  fclose(file);
 }
 
 result.current_token = _lcddl_get_next_token(&result);
 
 return result;
}

static int
_lcddl_get_character(_LcddlStream *stream)
{
 if (stream->index < stream->size)
 {
  return (int)stream->buffer[stream->index];
 }
 return EOF;
}

static int
_lcddl_peek_character(_LcddlStream *stream)
{
 if (stream->index + 1 < stream->size)
 {
  return (int)stream->buffer[stream->index + 1];
 }
 return EOF;
}

static void
_lcddl_consume_character(_LcddlStream *stream)
{
 stream->index += 1;
}

static _LcddlToken
_lcddl_get_next_token(_LcddlStream *stream)
{
 _LcddlToken result = {0};
 result.line        = stream->current_line;
 int c              = _lcddl_get_character(stream);
 
 // NOTE(tbt): ignore white space
 while (_lcddl_is_char_space(c))
 {
  if (c == '\n')
  {
   ++stream->current_line;
  }
  _lcddl_consume_character(stream);
  c = _lcddl_get_character(stream);
 }
 
 // NOTE(tbt): skip comments
 if (c == '/' &&
     _lcddl_peek_character(stream) == '/')
 {
  while (c != '\n')
  {
   _lcddl_consume_character(stream);
   c = _lcddl_get_character(stream);
  }
  ++stream->current_line;
  return _lcddl_get_next_token(stream);
 }
 
 
 if (_lcddl_is_char_letter(c) ||
     c == '_')
 {
  result.kind  = TOKEN_KIND_identifier;
  result.value = &stream->buffer[stream->index];
  
  while (_lcddl_is_char_alphanumeric(c) ||
         c == '_')
  {
   ++result.len;
   _lcddl_consume_character(stream);
   c = _lcddl_get_character(stream);
  }
 }
 else if (_lcddl_is_char_number(c))
 {
  result.kind  = TOKEN_KIND_integer_literal;
  result.value = &stream->buffer[stream->index];
  
  while (_lcddl_is_char_number(c) ||
         c == '.')
  {
   if (c == '.')
   {
    if (result.kind == TOKEN_KIND_integer_literal)
    {
     result.kind = TOKEN_KIND_float_literal;
    }
    else
    {
     print_error_and_exit(stream, "float literal may contain only one '.'");
    }
   }
   
   ++result.len;
   _lcddl_consume_character(stream);
   c = _lcddl_get_character(stream);
  }
 }
 else if (c == '"')
 {
  _lcddl_consume_character(stream);
  c = _lcddl_get_character(stream);
  
  result.kind  = TOKEN_KIND_string_literal;
  result.value = &stream->buffer[stream->index];
  
  while (c != '"')
  {
   ++result.len;
   _lcddl_consume_character(stream);
   c = _lcddl_get_character(stream);
  }
  
  _lcddl_consume_character(stream);
 }
 else if (c == '=')
 {
  result.kind  = TOKEN_KIND_equals;
  result.value = &stream->buffer[stream->index];
  result.len   = 1;
  _lcddl_consume_character(stream);
  
  c = _lcddl_get_character(stream);
  if (c == '=')
  {
   result.kind = TOKEN_KIND_equality;
   result.len  = 2;
   _lcddl_consume_character(stream);
  }
 }
 else if (c == '!')
 {
  result.kind  = TOKEN_KIND_exclamation;
  result.value = &stream->buffer[stream->index];
  result.len   = 1;
  _lcddl_consume_character(stream);
  
  c = _lcddl_get_character(stream);
  if (c == '=')
  {
   result.kind = TOKEN_KIND_not_equal_to;
   result.len  = 2;
   _lcddl_consume_character(stream);
  }
 }
 else if (c == '<')
 {
  result.kind  = TOKEN_KIND_lesser_than;
  result.value = &stream->buffer[stream->index];
  result.len   = 1;
  _lcddl_consume_character(stream);
  
  c = _lcddl_get_character(stream);
  if (c == '=')
  {
   result.kind = TOKEN_KIND_lesser_than_or_equal_to;
   result.len  = 2;
   _lcddl_consume_character(stream);
  }
  else if (c == '<')
  {
   result.kind = TOKEN_KIND_bit_shift_right;
   result.len  = 2;
   _lcddl_consume_character(stream);
  }
 }
 else if (c == '>')
 {
  result.kind  = TOKEN_KIND_greater_than;
  result.value = &stream->buffer[stream->index];
  result.len   = 1;
  _lcddl_consume_character(stream);
  
  c = _lcddl_get_character(stream);
  if (c == '=')
  {
   result.kind = TOKEN_KIND_greater_than_or_equal_to;
   result.len  = 2;
   _lcddl_consume_character(stream);
  }
  else if (c == '>')
  {
   result.kind = TOKEN_KIND_bit_shift_right;
   result.len  = 2;
   _lcddl_consume_character(stream);
  }
 }
 else if (c == '&')
 {
  result.kind  = TOKEN_KIND_bitwise_and;
  result.value = &stream->buffer[stream->index];
  result.len   = 1;
  _lcddl_consume_character(stream);
  
  c = _lcddl_get_character(stream);
  if (c == '&')
  {
   result.kind = TOKEN_KIND_boolean_and;
   result.len  = 2;
   _lcddl_consume_character(stream);
  }
 }
 else if (c == '|')
 {
  result.kind  = TOKEN_KIND_bitwise_or;
  result.value = &stream->buffer[stream->index];
  result.len   = 1;
  _lcddl_consume_character(stream);
  
  c = _lcddl_get_character(stream);
  if (c == '|')
  {
   result.kind = TOKEN_KIND_boolean_or;
   result.len  = 2;
   _lcddl_consume_character(stream);
  }
 }
 else if (c == EOF)
 {
  result.kind  = TOKEN_KIND_eof;
  result.value = NULL;
  result.len   = 0;
 }
 else
 {
  switch(c)
  {
   case ':':
   {
    result.kind = TOKEN_KIND_colon;
    break;
   }
   case '{':
   {
    result.kind = TOKEN_KIND_open_curly_bracket;
    break;
   }
   case '}':
   {
    result.kind = TOKEN_KIND_close_curly_bracket;
    break;
   }
   case '[':
   {
    result.kind = TOKEN_KIND_open_square_bracket;
    break;
   }
   case ']':
   {
    result.kind = TOKEN_KIND_close_square_bracket;
    break;
   }
   case '(':
   {
    result.kind = TOKEN_KIND_open_bracket;
    break;
   }
   case ')':
   {
    result.kind = TOKEN_KIND_close_bracket;
    break;
   }
   case '*':
   {
    result.kind = TOKEN_KIND_asterisk;
    break;
   }
   case '-':
   {
    result.kind = TOKEN_KIND_dash;
    break;
   }
   case '/':
   {
    result.kind = TOKEN_KIND_slash;
    break;
   }
   case '+':
   {
    result.kind = TOKEN_KIND_add;
    break;
   }
   case '~':
   {
    result.kind = TOKEN_KIND_tilde;
    break;
   }
   case '@':
   {
    result.kind = TOKEN_KIND_at_symbol;
    break;
   }
   case ';':
   {
    result.kind = TOKEN_KIND_semicolon;
    break;
   }
   case '^':
   {
    result.kind = TOKEN_KIND_bitwise_xor;
    break;
   }
   default:
   {
    print_error_and_exit_f(stream, "Unexpected character '%c'", c);
   }
  }
  
  result.value    = &stream->buffer[stream->index];
  result.len      = 1;
  _lcddl_consume_character(stream);
 }
 
 return result;
} 

static void
_lcddl_consume_token(_LcddlStream *stream,
                     _LcddlTokenKind required_kind)
{
 if (stream->current_token.kind == required_kind)
 {
  stream->current_token = _lcddl_get_next_token(stream);
 }
 else
 {
  print_error_and_exit_f(stream,
                         "Expected '%s', got '%s'",
                         token_kind_to_string(required_kind),
                         token_kind_to_string(stream->current_token.kind));
 }
}

static LcddlOperatorKind
_lcddl_token_to_operator_kind(_LcddlToken token,
                              bool unary) // whether to use the unary or binary version if ambiguous
{
 switch (token.kind)
 {
  case TOKEN_KIND_tilde:
  {
   return LCDDL_UN_OP_KIND_bitwise_not;
  }
  case TOKEN_KIND_exclamation:
  {
   return LCDDL_UN_OP_KIND_boolean_not;
  }
  case TOKEN_KIND_add:
  {
   if (unary) { return LCDDL_UN_OP_KIND_positive; }
   else       { return LCDDL_BIN_OP_KIND_add; }
  }
  case TOKEN_KIND_dash:
  {
   if (unary) { return LCDDL_UN_OP_KIND_negative; }
   else       { return LCDDL_BIN_OP_KIND_subtract; }
  }
  case TOKEN_KIND_asterisk:
  {
   return LCDDL_BIN_OP_KIND_multiply;
  }
  case TOKEN_KIND_slash:
  {
   return LCDDL_BIN_OP_KIND_divide;
  }
  case TOKEN_KIND_lesser_than:
  {
   return LCDDL_BIN_OP_KIND_lesser_than;
  }
  case TOKEN_KIND_greater_than:
  {
   return LCDDL_BIN_OP_KIND_greater_than;
  }
  case TOKEN_KIND_lesser_than_or_equal_to:
  {
   return LCDDL_BIN_OP_KIND_lesser_than_or_equal_to;
  }
  case TOKEN_KIND_greater_than_or_equal_to:
  {
   return LCDDL_BIN_OP_KIND_greater_than_or_equal_to;
  }
  case TOKEN_KIND_equality:
  {
   return LCDDL_BIN_OP_KIND_equality;
  }
  case TOKEN_KIND_not_equal_to:
  {
   return LCDDL_BIN_OP_KIND_not_equal_to;
  }
  case TOKEN_KIND_bit_shift_left:
  {
   return LCDDL_BIN_OP_KIND_bit_shift_left;
  }
  case TOKEN_KIND_bit_shift_right:
  {
   return LCDDL_BIN_OP_KIND_bit_shift_right;
  }
  case TOKEN_KIND_bitwise_and:
  {
   return LCDDL_BIN_OP_KIND_bitwise_and;
  }
  case TOKEN_KIND_bitwise_or:
  {
   return LCDDL_BIN_OP_KIND_bitwise_or;
  }
  case TOKEN_KIND_bitwise_xor:
  {
   return LCDDL_BIN_OP_KIND_bitwise_xor;
  }
  case TOKEN_KIND_boolean_and:
  {
   return LCDDL_BIN_OP_KIND_boolean_and;
  }
  case TOKEN_KIND_boolean_or:
  {
   return LCDDL_BIN_OP_KIND_boolean_or;
  }
  default:
  {
   return -1;
  }
 }
}

static bool
_lcddl_is_token_usable_as_binary_operator(_LcddlToken token)
{
 LcddlOperatorKind kind = _lcddl_token_to_operator_kind(token, false);
 if (kind == -1                          ||
     kind <= LCDDL_BINARY_OPERATOR_BEGIN ||
     kind >= LCDDL_BINARY_OPERATOR_END)
 {
  return false;
 }
 return true;
}

static bool
_lcddl_is_token_usable_as_unary_operator(_LcddlToken token)
{
 LcddlOperatorKind kind = _lcddl_token_to_operator_kind(token, true);
 if (kind == -1                          ||
     kind <= LCDDL_UNARY_OPERATOR_BEGIN ||
     kind >= LCDDL_UNARY_OPERATOR_END)
 {
  return false;
 }
 return true;
}

///////////////////////////////////////////
// PARSER
//~

static LcddlNode *_lcddl_parse_file(char *path);
static LcddlNode *_lcddl_parse_statement(_LcddlStream *stream);
static LcddlNode *_lcddl_parse_statement_list(_LcddlStream *stream);
static LcddlNode *_lcddl_parse_annotations(_LcddlStream *stream);
static LcddlNode *_lcddl_parse_declaration(_LcddlStream *stream);
static LcddlNode *_lcddl_parse_type(_LcddlStream *stream);
static LcddlNode *_lcddl_parse_binary_operator_rhs(_LcddlStream *stream, unsigned int expression_precedence, LcddlNode *lhs);
static LcddlNode *_lcddl_parse_expression(_LcddlStream *stream);
static LcddlNode *_lcddl_parse_parenthesis(_LcddlStream *lexer);
static LcddlNode *_lcddl_parse_primary(_LcddlStream *stream);
static LcddlNode *_lcddl_parse_unary_operator(_LcddlStream *stream);
static LcddlNode *_lcddl_parse_literal(_LcddlStream *stream);
static LcddlNode *_lcddl_parse_variable_reference(_LcddlStream *stream);

static LcddlNode *
_lcddl_parse_stream(_LcddlStream stream)
{
 LcddlNode *result     = calloc(1, sizeof *result);
 result->kind          = LCDDL_NODE_KIND_file;
 result->file.filename = calloc(1, strlen(stream.path) + 1);
 strcpy(result->file.filename, stream.path);
 result->first_child   = _lcddl_parse_statement_list(&stream);
 
 return result;
}

static LcddlNode *
_lcddl_parse_statement(_LcddlStream *stream)
{
 LcddlNode *result      = NULL;
 LcddlNode *annotations = _lcddl_parse_annotations(stream);
 
 if (stream->current_token.kind == TOKEN_KIND_identifier)
 {
  result = _lcddl_parse_declaration(stream);
  _lcddl_consume_token(stream, TOKEN_KIND_semicolon);
 }
 else
 {
  print_error_and_exit(stream, "Expected a statement");
 }
 
 result->first_annotation = annotations;
 return result;
}

static LcddlNode *
_lcddl_parse_statement_list(_LcddlStream *stream)
{
 LcddlNode *result = NULL;
 
 while (stream->current_token.kind == TOKEN_KIND_identifier ||
        stream->current_token.kind == TOKEN_KIND_at_symbol)
 {
  LcddlNode *node    = _lcddl_parse_statement(stream);
  node->next_sibling = result;
  result             = node;
 }
 
 return result;
}

static LcddlNode *
_lcddl_parse_annotations(_LcddlStream *stream)
{
 LcddlNode *result = NULL;
 
 while (stream->current_token.kind == TOKEN_KIND_at_symbol)
 {
  _lcddl_consume_token(stream, TOKEN_KIND_at_symbol);
  LcddlNode *annotation      = calloc(1, sizeof *annotation);
  annotation->kind           = LCDDL_NODE_KIND_annotation;
  annotation->annotation.tag = calloc(1, stream->current_token.len + 1);
  strncpy(annotation->annotation.tag,
          stream->current_token.value,
          stream->current_token.len);
  _lcddl_consume_token(stream, TOKEN_KIND_identifier);
  
  if (stream->current_token.kind == TOKEN_KIND_equals)
  {
   _lcddl_consume_token(stream, TOKEN_KIND_equals);
   annotation->annotation.value = _lcddl_parse_expression(stream);
  }
  
  annotation->next_annotation = result;
  result = annotation;
 }
 
 return result;
}

static LcddlNode *
_lcddl_parse_declaration(_LcddlStream *stream)
{
 LcddlNode *result        = calloc(1, sizeof *result);
 result->kind             = LCDDL_NODE_KIND_declaration;
 result->declaration.name = calloc(1, stream->current_token.len + 1);
 strncpy(result->declaration.name,
         stream->current_token.value,
         stream->current_token.len);
 
 _lcddl_consume_token(stream, TOKEN_KIND_identifier);
 
 if (stream->current_token.kind != TOKEN_KIND_semicolon)
 {
  if (stream->current_token.kind == TOKEN_KIND_colon)
  {
   _lcddl_consume_token(stream, TOKEN_KIND_colon);
   
   if (stream->current_token.kind == TOKEN_KIND_equals)
    // NOTE(tbt): type has been ommited - go straight to value;
   {
    _lcddl_consume_token(stream, TOKEN_KIND_equals);
    result->declaration.value = _lcddl_parse_expression(stream);
   }
   else
   {
    result->declaration.type = _lcddl_parse_type(stream);
    if (stream->current_token.kind == TOKEN_KIND_equals)
    {
     _lcddl_consume_token(stream, TOKEN_KIND_equals);
     result->declaration.value = _lcddl_parse_expression(stream);
    }
   }
  }
  
  if (stream->current_token.kind != TOKEN_KIND_semicolon)
  {
   _lcddl_consume_token(stream, TOKEN_KIND_open_curly_bracket);
   result->first_child      = _lcddl_parse_statement_list(stream);
   _lcddl_consume_token(stream, TOKEN_KIND_close_curly_bracket);
  }
 }
 
 return result;
}

static LcddlNode *
_lcddl_parse_type(_LcddlStream *stream)
{
 LcddlNode *result = calloc(1, sizeof *result);
 result->kind      = LCDDL_NODE_KIND_type;
 
 if (stream->current_token.kind == TOKEN_KIND_open_square_bracket)
 {
  _lcddl_consume_token(stream, TOKEN_KIND_open_square_bracket);
  char *array_count_str = calloc(1, stream->current_token.len + 1);
  strncpy(array_count_str,
          stream->current_token.value,
          stream->current_token.len);
  _lcddl_consume_token(stream, TOKEN_KIND_integer_literal);
  
  result->type.array_count = strtoul(array_count_str, NULL, 10);
  
  _lcddl_consume_token(stream, TOKEN_KIND_close_square_bracket);
 }
 
 result->type.type_name = calloc(1, stream->current_token.len + 1);
 strncpy(result->type.type_name,
         stream->current_token.value,
         stream->current_token.len);
 _lcddl_consume_token(stream, TOKEN_KIND_identifier);
 
 while (stream->current_token.kind == TOKEN_KIND_asterisk)
 {
  _lcddl_consume_token(stream, TOKEN_KIND_asterisk);
  result->type.indirection_level += 1; }
 
 return result;
}

static LcddlNode *
_lcddl_parse_binary_operator_rhs(_LcddlStream *stream,
                                 unsigned int expression_precedence,
                                 LcddlNode *lhs)
{
 static unsigned int precedence_table[1 << 8] = 
 {
  [LCDDL_BIN_OP_KIND_multiply]                 = 10,
  [LCDDL_BIN_OP_KIND_divide]                   = 10,
  [LCDDL_BIN_OP_KIND_add]                      = 9,
  [LCDDL_BIN_OP_KIND_subtract]                 = 9,
  [LCDDL_BIN_OP_KIND_bit_shift_left]           = 8,
  [LCDDL_BIN_OP_KIND_bit_shift_right]          = 8,
  [LCDDL_BIN_OP_KIND_lesser_than]              = 7,
  [LCDDL_BIN_OP_KIND_lesser_than_or_equal_to]  = 7,
  [LCDDL_BIN_OP_KIND_greater_than]             = 7,
  [LCDDL_BIN_OP_KIND_greater_than_or_equal_to] = 7,
  [LCDDL_BIN_OP_KIND_equality]                 = 6,
  [LCDDL_BIN_OP_KIND_not_equal_to]             = 6,
  [LCDDL_BIN_OP_KIND_bitwise_and]              = 5,
  [LCDDL_BIN_OP_KIND_bitwise_xor]              = 4,
  [LCDDL_BIN_OP_KIND_bitwise_or]               = 3,
  [LCDDL_BIN_OP_KIND_boolean_and]              = 2,
  [LCDDL_BIN_OP_KIND_boolean_or]               = 1,
 };
 
 for (;;)
 {
  LcddlOperatorKind operator_kind;
  unsigned int token_precedence;
  
  if (_lcddl_is_token_usable_as_binary_operator(stream->current_token))
  {
   operator_kind    = _lcddl_token_to_operator_kind(stream->current_token, false);
   token_precedence = precedence_table[operator_kind];
   
   if (token_precedence < expression_precedence)
   {
    return lhs;
   }
  }
  else
  {
   return lhs;
  }
  
  _lcddl_consume_token(stream, stream->current_token.kind);
  
  LcddlNode *rhs = _lcddl_parse_primary(stream);
  
  if (_lcddl_is_token_usable_as_binary_operator(stream->current_token))
  {
   unsigned int next_precedence = precedence_table[_lcddl_token_to_operator_kind(stream->current_token, false)];
   if (token_precedence < next_precedence)
   {
    rhs = _lcddl_parse_binary_operator_rhs(stream, token_precedence + 1, rhs);
   }
  }
  
  LcddlNode *new_left             = calloc(1, sizeof *new_left);
  new_left->kind                  = LCDDL_NODE_KIND_binary_operator;
  new_left->binary_operator.kind  = operator_kind;
  new_left->binary_operator.left  = lhs;
  new_left->binary_operator.right = rhs;
  
  lhs = new_left;
 }
}

static LcddlNode *
_lcddl_parse_expression(_LcddlStream *stream)
{
 LcddlNode *lhs = _lcddl_parse_primary(stream);
 return _lcddl_parse_binary_operator_rhs(stream, 0, lhs);
}

static LcddlNode *
_lcddl_parse_parenthesis(_LcddlStream *stream)
{
 _lcddl_consume_token(stream, TOKEN_KIND_open_bracket);  // eat '('
 LcddlNode *result = _lcddl_parse_expression(stream);
 _lcddl_consume_token(stream, TOKEN_KIND_close_bracket); // eat ')'
 
 return result;
}

static LcddlNode *
_lcddl_parse_primary(_LcddlStream *stream)
{
 switch (stream->current_token.kind)
 {
  case TOKEN_KIND_add:         // unary plus operator
  case TOKEN_KIND_dash:        // unary negative operator
  case TOKEN_KIND_tilde:       // bitwise not operator
  case TOKEN_KIND_exclamation: // boolean not operator
  {
   return _lcddl_parse_unary_operator(stream);
  }
  case TOKEN_KIND_float_literal:
  case TOKEN_KIND_integer_literal:
  case TOKEN_KIND_string_literal:
  {
   return _lcddl_parse_literal(stream);
  }
  case TOKEN_KIND_identifier:
  {
   return _lcddl_parse_variable_reference(stream);
  }
  case TOKEN_KIND_open_bracket:
  {
   return _lcddl_parse_parenthesis(stream);
  }
  default:
  {
   print_error_and_exit_f(stream,
                          "Got unexpected token '%s' when expecting an expression",
                          token_kind_to_string(stream->current_token.kind));
   return NULL;
  }
 }
}

static LcddlNode *
_lcddl_parse_unary_operator(_LcddlStream *stream)
{
 LcddlNode *result = calloc(1, sizeof *result);
 result->kind      = LCDDL_NODE_KIND_unary_operator;
 
 if (_lcddl_is_token_usable_as_unary_operator(stream->current_token))
 {
  result->unary_operator.kind = _lcddl_token_to_operator_kind(stream->current_token, true);
  _lcddl_consume_token(stream, stream->current_token.kind);
 }
 else
 {
  print_error_and_exit_f(stream,
                         "expected a unary operator, instead got '%s'",
                         token_kind_to_string(stream->current_token.kind));
 }
 result->unary_operator.operand = _lcddl_parse_expression(stream);
 
 return result;
}

static LcddlNode *
_lcddl_parse_literal(_LcddlStream *stream)
{
 LcddlNode *result     = calloc(1, sizeof *result);
 result->literal.value = calloc(1, stream->current_token.len + 1);
 strncpy(result->literal.value,
         stream->current_token.value,
         stream->current_token.len);
 
 switch (stream->current_token.kind)
 {
  case TOKEN_KIND_string_literal:
  {
   result->kind = LCDDL_NODE_KIND_string_literal;
   _lcddl_consume_token(stream, TOKEN_KIND_string_literal);
   break;
  }
  case TOKEN_KIND_integer_literal:
  {
   result->kind = LCDDL_NODE_KIND_integer_literal;
   _lcddl_consume_token(stream, TOKEN_KIND_integer_literal);
   break;
  }
  case TOKEN_KIND_float_literal:
  {
   result->kind = LCDDL_NODE_KIND_float_literal;
   _lcddl_consume_token(stream, TOKEN_KIND_float_literal);
   break;
  }
  default:
  {
   print_error_and_exit_f(stream,
                          "Expecting a literal, got '%s'",
                          token_kind_to_string(stream->current_token.kind));
  }
 }
 
 return result;
}

static LcddlNode *
_lcddl_parse_variable_reference(_LcddlStream *stream)
{
 LcddlNode *result          = calloc(1, sizeof *result);
 result->kind               = LCDDL_NODE_KIND_variable_reference;
 result->var_reference.name = calloc(1, stream->current_token.len + 1);
 strncpy(result->var_reference.name,
         stream->current_token.value,
         stream->current_token.len);
 
 _lcddl_consume_token(stream, TOKEN_KIND_identifier);
 
 return result;
}

///////////////////////////////////////////
// USER LAYER
//~

#ifndef LCDDL_AS_LIBRARY

typedef void (*_LcddlUserCallback)(LcddlNode *);

static _LcddlUserCallback get_user_callback_functions(char *lib_path);

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
static _LcddlUserCallback
get_user_callback_functions(char *lib_path)
{
 _LcddlUserCallback result = NULL;
 
 HINSTANCE library = LoadLibrary(lib_path);
 if (!library)
 {
  fprintf(stderr, "ERROR: Could not open custom layer library '%s'\n", lib_path);
  exit(EXIT_FAILURE);
 }
 
 result = (_LcddlUserCallback)GetProcAddress(library, "lcddl_user_callback");
 
 if (!result)
 {
  fprintf(stderr, "ERROR: Could not find callback in user layer library '%s'\n", lib_path);
  exit(EXIT_FAILURE);
 }
 
 return result;
}
#else
static _LcddlUserCallback
get_user_callback_functions(char *lib_path)
{
 _LcddlUserCallback result;
 
 void *library = dlopen(lib_path, RTLD_NOW | RTLD_GLOBAL);
 if (!library)
 {
  fprintf(stderr, "ERROR: Could not open user layer library '%s'\n", lib_path);
  exit(EXIT_FAILURE);
 }
 
 result = dlsym(library, "lcddl_user_callback");
 
 if (!result)
 {
  fprintf(stderr, "ERROR: Could not find callback in user layer library '%s'\n", lib_path);
  exit(EXIT_FAILURE);
 }
 
 return result;
}
#endif

#endif

///////////////////////////////////////////
// MAIN
//~

static LcddlNode *_lcddl_global_root;

#ifndef LCDDL_AS_LIBRARY
int
main(int argc,
     char **argv)
{
 if (argc < 3)
 {
  fprintf(stderr, "Usage: %s custom_layer_library path input_file_1 input_file_2...\n", argv[0]);
  return EXIT_FAILURE;
 }
 
 _LcddlUserCallback user_callback = get_user_callback_functions(argv[1]);
 _lcddl_global_root               = calloc(1, sizeof *_lcddl_global_root);
 _lcddl_global_root->kind         = LCDDL_NODE_KIND_root;
 
 for (int i = 2;
      i < argc;
      ++i)
 {
  LcddlNode *file                 = _lcddl_parse_stream(_lcddl_load_entire_file_as_stream(argv[i]));
  file->next_sibling              = _lcddl_global_root->first_child;
  _lcddl_global_root->first_child = file;
 }
 
 user_callback(_lcddl_global_root);
 
 return EXIT_SUCCESS;
}

#else

void
lcddl_initialise(void)
{
 _lcddl_global_root       = calloc(1, sizeof *_lcddl_global_root);
 _lcddl_global_root->kind = LCDDL_NODE_KIND_root;
}

LcddlNode *
lcddl_parse_file(char *filename)
{
 LcddlNode *file                  = _lcddl_parse_stream(_lcddl_load_entire_file_as_stream(filename));
 file->next_sibling               = _lcddl_global_root->first_child;
 _lcddl_global_root->first_child  = file;
 
 return file;
}

LcddlNode *
lcddl_parse_from_memory(char *buffer,
                        unsigned long long buffer_size)
{
 _LcddlStream stream              = {0};
 stream.buffer                    = buffer;
 stream.size                      = buffer_size;
 stream.path                      = "memory";
 stream.current_line              = 1;
 stream.current_token             = _lcddl_get_next_token(&stream);
 LcddlNode *file                  = _lcddl_parse_stream(stream);
 file->next_sibling               = _lcddl_global_root->first_child;
 _lcddl_global_root->first_child  = file;
 
 return file;
}

LcddlNode *
lcddl_parse_cstring(char *string)
{
 return lcddl_parse_from_memory(string, strlen(string));
}

static void
_lcddl_free_tree(LcddlNode *root)
{
 if (root)
 {
  // free sub-type specific data
  switch(root->kind)
  {
   case LCDDL_NODE_KIND_file:
   {
    free(root->file.filename);
    break;
   }
   
   case LCDDL_NODE_KIND_declaration:
   {
    free(root->declaration.name);
    _lcddl_free_tree(root->declaration.type);
    _lcddl_free_tree(root->declaration.value);
    break;
   }
   
   case LCDDL_NODE_KIND_type:
   {
    free(root->type.type_name);
    break;
   }
   
   case LCDDL_NODE_KIND_binary_operator:
   {
    _lcddl_free_tree(root->binary_operator.left);
    _lcddl_free_tree(root->binary_operator.right);
    break;
   }
   
   case LCDDL_NODE_KIND_unary_operator:
   {
    _lcddl_free_tree(root->unary_operator.operand);
    break;
   }
   
   case LCDDL_NODE_KIND_string_literal:
   case LCDDL_NODE_KIND_float_literal:
   case LCDDL_NODE_KIND_integer_literal:
   {
    free(root->literal.value);
    break;
   }
   
   case LCDDL_NODE_KIND_variable_reference:
   {
    free(root->var_reference.name);
    break;
   }
   
   case LCDDL_NODE_KIND_annotation:
   {
    free(root->annotation.tag);
    _lcddl_free_tree(root->annotation.value);
    break;
   }
  }
  
  // free children
  LcddlNode *next_child = NULL;
  for (LcddlNode *child = root->first_child;
       NULL != child;
       child = next_child)
  {
   next_child = child->next_sibling;
   _lcddl_free_tree(child);
  }
  
  // free annotations
  LcddlNode *next_annotation = NULL;
  for (LcddlNode *annotation = root->first_annotation;
       NULL != annotation;
       annotation = next_annotation)
  {
   next_annotation = annotation->next_annotation;
   _lcddl_free_tree(annotation);
  }
  
  // free the node itself
  free(root);
 }
}

void
lcddl_free_file(LcddlNode *root)
{
 if (root->kind == LCDDL_NODE_KIND_file)
 {
  // remove from global tree
  LcddlNode **indirect = &_lcddl_global_root->first_child;
  while (*indirect != root)
  {
   indirect = &(*indirect)->next_sibling;
  }
  *indirect = (*indirect)->next_sibling;
  
  // free the tree
  _lcddl_free_tree(root);
 }
}

#endif

///////////////////////////////////////////
// USER LAYER HELPERS
//~

LcddlNode *
lcddl_get_annotation_value(LcddlNode *node,
                           char *tag)
{
 for (LcddlNode *a = node->first_annotation;
      NULL != a;
      a = a->next_annotation)
 {
  if (0 == strcmp(a->annotation.tag, tag))
  {
   return a->annotation.value;
  }
 }
 
 return NULL;
}

bool
lcddl_does_node_have_tag(LcddlNode *node,
                         char *tag)
{
 for (LcddlNode *a = node->first_annotation;
      NULL != a;
      a = a->next_annotation)
 {
  if (0 == strcmp(a->annotation.tag, tag))
  {
   return true;
  }
 }
 
 return false;
}

static void
_lcddl_write_field_to_file_as_c(LcddlNode *node,
                                unsigned int indentation,
                                FILE *file)
{
 for (unsigned int i = indentation;
      0 != i;
      --i)
 {
  putc('\t', file);
 }
 
 if (node->kind == LCDDL_NODE_KIND_declaration)
 {
  if (node->first_child)
  {
   if (!node->declaration.type->type.indirection_level &&
       !node->declaration.type->type.array_count)
   {
    if (0 == strcmp(node->declaration.type->type.type_name, "struct"))
    {
     fprintf(file, "struct\n{\n");
    }
    else if (0 == strcmp(node->declaration.type->type.type_name, "union"))
    {
     fprintf(file, "union\n{\n");
    }
    else
    {
     fprintf(file, "struct // type '%s' not available in c\n{\n", node->declaration.type->type.type_name);
    }
    
    for (LcddlNode *child = node->first_child;
         NULL != child;
         child = child->next_sibling)
    {
     _lcddl_write_field_to_file_as_c(child, indentation + 1, file);
    }
    
    fprintf(file, "};\n");
   }
   else
   {
    fprintf(file, "// could not write field '%s' as c\n", node->declaration.name);
   }
  }
  else
  {
   fprintf(file, "%s ", node->declaration.type->type.type_name);
   
   for (int i = node->declaration.type->type.indirection_level;
        0 != i;
        --i)
   {
    putc('*', file);
   }
   
   fprintf(file, "%s", node->declaration.name);
   
   if (node->declaration.type->type.array_count)
   {
    fprintf(file, "[%u]", node->declaration.type->type.array_count);
   }
   
   putc(';', file);
   
   if (node->declaration.value)
   {
    fprintf(file, " // c does not support initialisers in structs/unions");
   }
   
   putc('\n', file);
  }
 }
 else
 {
  fprintf(file, "// could not write field");
 }
}

void
lcddl_write_node_to_file_as_c_struct(LcddlNode *node,
                                     FILE *file)
{
 if (node->kind == LCDDL_NODE_KIND_declaration)
 {
  fprintf(file,
          "typedef struct %s %s;\nstruct %s\n{\n",
          node->declaration.name,
          node->declaration.name,
          node->declaration.name);
  
  for (LcddlNode *child = node->first_child;
       NULL != child;
       child = child->next_sibling)
  {
   _lcddl_write_field_to_file_as_c(child, 1, file);
  }
  fprintf(file, "};\n\n");
 }
 else
 {
  fprintf(file, "// could not write node as struct");
 }
}

void
lcddl_write_node_to_file_as_c_enum(LcddlNode *node,
                                   FILE *file)
{
 if (node->kind == LCDDL_NODE_KIND_declaration)
 {
  fprintf(file, "typedef enum\n{\n");
  
  for (LcddlNode *child = node->first_child;
       NULL != child;
       child = child->next_sibling)
  {
   if (child->kind == LCDDL_NODE_KIND_declaration)
   {
    fprintf(file, "\t%s,\n", child->declaration.name);
   }
  }
  fprintf(file, "} %s;\n\n", node->declaration.name);
 }
 else
 {
  fprintf(file, "// could not write node as enum");
 }
}

LcddlSearchResult *
lcddl_find_top_level_declaration(char *name)
{
 fprintf(stderr, "looking for top level decl '%s'\n", name);
 LcddlSearchResult *result = NULL;
 
 for (LcddlNode *file = _lcddl_global_root->first_child;
      NULL != file;
      file = file->next_sibling)
 {
  for (LcddlNode *node = file->first_child;
       NULL != node;
       node = node->next_sibling)
  {
   if (node->kind == LCDDL_NODE_KIND_declaration &&
       0 == strcmp(node->declaration.name, name))
   {
    LcddlSearchResult *search_node = calloc(1, sizeof(*search_node));
    search_node->next = result;
    result = search_node;
    search_node->node = node;
   }
  }
 }
 
 return result;
}

LcddlSearchResult *
lcddl_find_all_top_level_declarations_with_tag(char *tag)
{
 LcddlSearchResult *result = NULL;
 
 for (LcddlNode *file = _lcddl_global_root->first_child;
      NULL != file;
      file = file->next_sibling)
 {
  for (LcddlNode *node = file->first_child;
       NULL != node;
       node = node->next_sibling)
  {
   if (node->kind == LCDDL_NODE_KIND_declaration &&
       lcddl_does_node_have_tag(node, tag))
   {
    LcddlSearchResult *search_node = calloc(1, sizeof(*search_node));
    search_node->next = result;
    result = search_node;
    search_node->node = node;
   }
  }
 }
 
 return result;
}

bool
lcddl_is_declaration_type(LcddlNode *declaration,
                          char *type_name)
{
 if (declaration->kind == LCDDL_NODE_KIND_declaration)
 {
  return (0 == strcmp(declaration->declaration.type->type.type_name, type_name));
 }
 return false;
}

double
lcddl_evaluate_expression(LcddlNode *expression)
{
 switch (expression->kind)
 {
  case LCDDL_NODE_KIND_float_literal:
  {
   return strtod(expression->literal.value, NULL);
  }
  
  case LCDDL_NODE_KIND_integer_literal:
  {
   return (double)strtol(expression->literal.value, NULL, 10);
  }
  
  case LCDDL_NODE_KIND_unary_operator:
  {
   switch (expression->unary_operator.kind)
   {
    case LCDDL_UN_OP_KIND_positive:
    {
     return lcddl_evaluate_expression(expression->unary_operator.operand);
    }
    
    case LCDDL_UN_OP_KIND_negative:
    {
     return lcddl_evaluate_expression(expression->unary_operator.operand) * -1.0;
    }
    
    case LCDDL_UN_OP_KIND_bitwise_not:
    {
     return (double)(~((unsigned long long)lcddl_evaluate_expression(expression->unary_operator.operand)));
    }
    
    case LCDDL_UN_OP_KIND_boolean_not:
    {
     return (double)(!((long long)lcddl_evaluate_expression(expression->unary_operator.operand)));
    }
   }
  }
  
  case LCDDL_NODE_KIND_binary_operator:
  {
   switch (expression->binary_operator.kind)
   {
    case LCDDL_BIN_OP_KIND_multiply:
    {
     return lcddl_evaluate_expression(expression->binary_operator.left) * lcddl_evaluate_expression(expression->binary_operator.right);
    }
    
    case LCDDL_BIN_OP_KIND_divide:
    {
     return lcddl_evaluate_expression(expression->binary_operator.left) / lcddl_evaluate_expression(expression->binary_operator.right);
    }
    
    case LCDDL_BIN_OP_KIND_add:
    {
     return lcddl_evaluate_expression(expression->binary_operator.left) + lcddl_evaluate_expression(expression->binary_operator.right);
    }
    
    case LCDDL_BIN_OP_KIND_subtract:
    {
     return lcddl_evaluate_expression(expression->binary_operator.left) - lcddl_evaluate_expression(expression->binary_operator.right);
    }
    
    case LCDDL_BIN_OP_KIND_bit_shift_left:
    {
     return (double)((unsigned long long)lcddl_evaluate_expression(expression->binary_operator.left) << (long long)lcddl_evaluate_expression(expression->binary_operator.right));
    }
    
    case LCDDL_BIN_OP_KIND_bit_shift_right:
    {
     return (double)((unsigned long long)lcddl_evaluate_expression(expression->binary_operator.left) >> (long long)lcddl_evaluate_expression(expression->binary_operator.right));
    }
    
    case LCDDL_BIN_OP_KIND_lesser_than:
    {
     return (double)(lcddl_evaluate_expression(expression->binary_operator.left) < lcddl_evaluate_expression(expression->binary_operator.right));
    }
    
    case LCDDL_BIN_OP_KIND_greater_than:
    {
     return (double)(lcddl_evaluate_expression(expression->binary_operator.left) > lcddl_evaluate_expression(expression->binary_operator.right));
    }
    
    case LCDDL_BIN_OP_KIND_lesser_than_or_equal_to:
    {
     return (double)(lcddl_evaluate_expression(expression->binary_operator.left) <= lcddl_evaluate_expression(expression->binary_operator.right));
    }
    
    case LCDDL_BIN_OP_KIND_greater_than_or_equal_to:
    {
     return (double)(lcddl_evaluate_expression(expression->binary_operator.left) >= lcddl_evaluate_expression(expression->binary_operator.right));
    }
    
    case LCDDL_BIN_OP_KIND_equality:
    {
     return (double)(lcddl_evaluate_expression(expression->binary_operator.left) == lcddl_evaluate_expression(expression->binary_operator.right));
    }
    
    case LCDDL_BIN_OP_KIND_not_equal_to:
    {
     return (double)(lcddl_evaluate_expression(expression->binary_operator.left) != lcddl_evaluate_expression(expression->binary_operator.right));
    }
    
    case LCDDL_BIN_OP_KIND_bitwise_and:
    {
     return (double)((unsigned long long)lcddl_evaluate_expression(expression->binary_operator.left) & (unsigned long long)lcddl_evaluate_expression(expression->binary_operator.right));
    }
    
    case LCDDL_BIN_OP_KIND_bitwise_xor:
    {
     return (double)((unsigned long long)lcddl_evaluate_expression(expression->binary_operator.left) ^ (unsigned long long)lcddl_evaluate_expression(expression->binary_operator.right));
    }
    
    case LCDDL_BIN_OP_KIND_bitwise_or:
    {
     return (double)((unsigned long long)lcddl_evaluate_expression(expression->binary_operator.left) | (unsigned long long)lcddl_evaluate_expression(expression->binary_operator.right));
    }
    
    case LCDDL_BIN_OP_KIND_boolean_and:
    {
     return (double)(lcddl_evaluate_expression(expression->binary_operator.left) && lcddl_evaluate_expression(expression->binary_operator.right));
    }
    
    case LCDDL_BIN_OP_KIND_boolean_or:
    {
     return (double)(lcddl_evaluate_expression(expression->binary_operator.left) || lcddl_evaluate_expression(expression->binary_operator.right));
    }
   }
  }
  
  default:
  {
   fprintf(stderr, "Error evaluating expression.\n");
   return 0.0;
  }
 }
}

#undef LOG_ERROR_BEGIN
#undef LOG_WARN_BEGIN
#undef PATH_MAX_LEN
#undef print_warning
#undef print_error_and_exit
#undef print_error_and_exit_f
#undef token_kind_to_string

#endif