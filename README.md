# LCDDL
_LCDDL_ is a simple utility which parses a minimal data description format.
Once a data layout has been extracted from the input file, a corresponding
tree is constructed and sent to the user code bellow.
The tree is arranged as a linked list of _top level_ nodes, with structs
containing a separate list of _child_ nodes.
Each node in the tree consists of a `Name`, a `Type` and an `IndirectionLevel`,
which can be introspected upon when lcddl is run.
_LCDDL_ is part of [lucerna](https://github.com/tomthornt0n/lucerna).
## Usage
#### 1. Write an input file
See `example.lcd` for more info.
#### 2. Write a custom layer
Fill out the functions in `lcddlUserLayer.c`.
#### 3. Build LCDDL
* with GCC: `gcc lcddl.c lcddlUserLayer.c -o lcddl`
* with Visual Studio: `cl lcddl.c lcddlUserLayer.c /link /out lcddl.exe`
#### 4. Run LCDDL
* *nix: `./lcddl path/to/input/file.lcd`
* windows: `lcddl path/to/input/file.lcd`
