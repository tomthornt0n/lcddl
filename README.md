# LCDDL

LCDDL is a small utility to allow for compile time type introspection in languages such as C.

It parses a simple format which describes a series of declarations into a corresponding AST, which it then passes to a custom user layer, where this data can be used for whatever your project requires.

LCDDL was recently rewritten.
The old version is available on the `legacy` branch.

## Documentation:
### Building LCDDL:
#### On Windows:
> To build LCDDL on windows, `cl` must be in your path. The easiest way to achieve this is by using a visual studio developer command prompt. This can be found by searching for `x64 Native Tools Command Prompt for VS****` in the start menu.
* First build LCDDL itself: `.\windows_build.bat`
* Next, build the user layer: `cl /nologo (path to user layer source files) /link /nologo /dll /out:lcddl_user_layer.dll`

#### On \*nix:
* Run the build script to build LCDDL itself: `./linux_build.sh`
* Build the user layer: `gcc (path to user layer source files) -fPIC -shared -o lcddl_user_layer.so`

### Running LCDDL:
LCDDL can be run with `./lcddl (path to shared library) (input file 1) (input file 2) ...`

### Writing a user layer:
The user layer must implement the function:
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
### Writing input files:
Each input file consists of a set of declarations
#### Declarations:
Declarations come in two forms:
```
identifier : type = expression;
```
* In a normal declaration the type or the value may be ommited (but not both).

```
identifier :: type
{
    [declarations]
}
```
* No value may be specified
* The type must not be ommited

#### Expressions:
An expression is defined as:
```
primary_expression [binary_operator expression]
```
Primary expressions are any literal or an identifier, optionally preceded by a unary operator.

#### Operators:
Unary operators:
```
LCDDL_UN_OP_KIND_positive,                  // '+'
LCDDL_UN_OP_KIND_negative,                  // '-'
LCDDL_UN_OP_KIND_bitwise_not,               // '~'
LCDDL_UN_OP_KIND_boolean_not,               // '!'
```
Binary operators:
```
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

### literals
* an integer literal is defined as a sequence of digits.
* a float literal is defined as a sequence of digits containing exactly 1 '.'.
* a string literal is defined as a sequence of any characters enclosed in '"'.

#### Identifiers
An identifier is defined as a sequence of alphanumeric characters beginning with a letter or and underscore.

