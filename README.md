# LCDDL

LCDDL is a structured text format and accompanying C library to parse it, originally designed with the intention of being used to facilitate metaprogramming and compile-time type introspection in C.
The format is designed such that it can be aesthetically similar to C declarations, however it can be used to encode any kind of data.

LCDDL was recently rewritten.
The old version is available on the `legacy` branch.

# Documentation:

## Using LCDDL:
There are two ways to use LCDDL - as an [executable](https://github.com/tomthornt0n/lcddl#as-an-executable) which calls into a custom layer, or included in your project as a [library](https://github.com/tomthornt0n/lcddl#as-a-library).

---

## As an executable:
### On Windows:
> To build LCDDL on windows, `cl` must be in your path. The easiest way to achieve this is by using a visual studio developer command prompt. This can be found by searching for `x64 Native Tools Command Prompt for VS****` in the start menu.
* First build LCDDL itself: `.\windows_build.bat`
* Next, build the user layer: `cl /nologo (path to user layer source files) /link /nologo /dll /out:lcddl_user_layer.dll`

### On \*nix:
* Run the build script to build LCDDL itself: `./linux_build.sh`
* Build the user layer: `gcc (path to user layer source files) -fPIC -shared -o lcddl_user_layer.so`

### Writing a user layer:
The user layer is a DLL which the LCDDL executable calls into, passing the root of the parsed AST.
It must implement the function:
``` c
LCDDL_CALLBACK void
lcddl_user_callback(LcddlNode *root)
{
 // do something here
}
```
* `root` is the root of a tree composed entirely of `LcddlNode`s.
* each child of `root` represents a file passed to LCDDL as input.
* each child of the file represents a declaration within the file.
* see `lcddl.h` for more information about `LcddlNode`

### Running LCDDL:
LCDDL can be run with `./lcddl (path to user layer shared library) (input file 1) (input file 2) ...`

## As a library:
Alternatively, LCDDL may be used as a library

```
void lcddl_initialise(void);
```
* Call this before using any other LCDDL APIs when LCDDL is being used as a library.

```
LcddlNode *lcddl_parse_file(char *filename);
```
* Parses the file specified by `filename` and returns a pointer to the corresponding `LcddlNode`.

```
LcddlNode *lcddl_parse_from_memory(char *buffer, unsigned long long buffer_size);
```
* Parses `buffer_size` bytes from the memory pointed to by buffer and returns a pointer to the corresponding `LcddlNode`.

```
LcddlNode *lcddl_parse_cstring(char *string);
```
* Thin wrapper around `lcddl_parse_from_memory` - equivalent to `lcddl_parse_from_memory(string, strlen(string))`


# The LCD file format:

Each input file consists of a set of declarations.
See `example.lcd` for an example.
Bellow, anything surrounded by `< >` denotes it may be ommited

## Declarations:
A declaration is defined as
```
identifier <: <type> <= expression>> < { delcarations }>;
```
Any declaration may be prefixed by a series of [tags](https://github.com/tomthornt0n/lcddl#tags).
                                                      
## Identifiers
An identifier is defined as a sequence of alphanumeric characters beginning with a letter or an underscore.

## Types
A type is defined as an identifier, optionally prefixed by `[integer literal]` where the integer literal corresponds to the size of the array, and optionally suffixed by n`*`, where the number of `*`s correspond to the indirection level. Both the array count and the indirection level default to 0 if ommited.

## Expressions:
An expression is defined as:
```
primary_expression <binary_operator expression>
```
Primary expressions are any literal or an identifier, optionally preceded by a unary operator.

### Unary operators
```
LCDDL_UN_OP_KIND_positive                   '+'
LCDDL_UN_OP_KIND_negative                   '-'
LCDDL_UN_OP_KIND_bitwise_not                '~'
LCDDL_UN_OP_KIND_boolean_not                '!'
```

### Binary operators
```
LCDDL_BIN_OP_KIND_multiply                   '*'
LCDDL_BIN_OP_KIND_divide                     '/'
LCDDL_BIN_OP_KIND_add                        '+'
LCDDL_BIN_OP_KIND_subtract                   '-'
LCDDL_BIN_OP_KIND_bit_shift_left             '<<'
LCDDL_BIN_OP_KIND_bit_shift_right            '>>'
LCDDL_BIN_OP_KIND_lesser_than                '<'
LCDDL_BIN_OP_KIND_greater_than               '>'
LCDDL_BIN_OP_KIND_lesser_than_or_equal_to    '<='
LCDDL_BIN_OP_KIND_greater_than_or_equal_to   '>='
LCDDL_BIN_OP_KIND_equality                   '=='
LCDDL_BIN_OP_KIND_not_equal_to               '!='
LCDDL_BIN_OP_KIND_bitwise_and                '&'
LCDDL_BIN_OP_KIND_bitwise_xor                '^'
LCDDL_BIN_OP_KIND_bitwise_or                 '|'
LCDDL_BIN_OP_KIND_boolean_and                '&&'
LCDDL_BIN_OP_KIND_boolean_or                 '||'
```

Binary operators have the following precedence:
```
LCDDL_BIN_OP_KIND_multiply                        10
LCDDL_BIN_OP_KIND_divide                          10
LCDDL_BIN_OP_KIND_add                             9
LCDDL_BIN_OP_KIND_subtract                        9
LCDDL_BIN_OP_KIND_bit_shift_left                  8
LCDDL_BIN_OP_KIND_bit_shift_right                 8
LCDDL_BIN_OP_KIND_lesser_than                     7
LCDDL_BIN_OP_KIND_lesser_than_or_equal_to         7
LCDDL_BIN_OP_KIND_greater_than                    7
LCDDL_BIN_OP_KIND_greater_than_or_equal_to        7
LCDDL_BIN_OP_KIND_equality                        6
LCDDL_BIN_OP_KIND_not_equal_to                    6
LCDDL_BIN_OP_KIND_bitwise_and                     5
LCDDL_BIN_OP_KIND_bitwise_xor                     4
LCDDL_BIN_OP_KIND_bitwise_or                      3
LCDDL_BIN_OP_KIND_boolean_and                     2
LCDDL_BIN_OP_KIND_boolean_or                      1
```

## Tags
Tags are used to providea additional metadata about the structure to the user layer.
A tag is in the form:
```
@identifier <= expression>
```

## Literals
* an integer literal is defined as a sequence of digits.
* a float literal is defined as a sequence of digits containing exactly 1 '.'.
* a string literal is defined as a sequence of any characters enclosed in '"'.


# User Layer Helpers
LCDDL provides a set of helper functions to assist writing a custom layer.
These are still a work in progress and will be documented soon.

