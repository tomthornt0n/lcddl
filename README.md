# LCDDL
_LCDDL_ is a simple utility which which parses a minimal data-description
format to construct a corresponding abstract syntax tree, which may be
introspected upon by user layer code in order to perform compile-time code
generation, or any other task for which this utility may be suited.
_LCDDL_ is part of [lucerna](https://github.com/tomthornt0n/lucerna).
## Usage
#### 1. Write an input file
See `example.lcd` for more info.
#### 2. Write a custom layer
Fill out the functions in `lcddlUserLayer.c`.
#### 3. Build LCDDL
* with GCC: `gcc lcddl.c lcddlUserLayer.c -o lcddl`
* with Visual Studio: `cl lcddl.c lcddlUserLayer.c /link /out:lcddl.exe`
#### 4. Run LCDDL
`./lcddl path/to/input/file1.lcd path/to/input/file2.lcd ...`
