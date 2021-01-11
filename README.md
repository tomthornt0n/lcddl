# LCDDL

LCDDL is a small utitility to allow for compile time type introspection in languages such as C.

It parses a simple format which describes a series of declarations into a 'list of lists' style tree, which is then passes to a custom user layer, which can then use this data for whatever your project requires.  

LCDDL was recently rewritten and some features that were previously avaible are currently missing:
* arrays
* pointers
* annotations

---

## Documentations:
### Building LCDDL:
#### On Windows:
> To build LCDDL on windows, `cl` must be in your path. The easiest way to achieve this is by using a visual studio developer command prompt. This can be found by searching for 'x64 Native Tools Command Prompt for VS\*\*\*\*'
* First build LCDDL itself: `.\windows_build.bat`
* Next, build the user layer: `cl /nologo [path to user layer source files] /link /nologo /dll /out:lcddl_user_layer.dll`
#### On \*nix:
* Run the build script to build LCDDL itself: `./linux_build.sh`
* Build the user layer: `gcc [path to user layer source files] -fPIC -shared -o lcddl_user_layer.so`
### Running LCDDL:
* lcddl can be run with `./lcddl (path to shared library) (input file 1) (input file 2) ...`
### Writing a user layer:
* fill out the function stubs provided in `lcddl_user_layer.c`
### Writing input files:
* see `example.lcd` for more details

