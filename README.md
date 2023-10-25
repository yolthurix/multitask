# Multitask
Simple cooperative multitasking library in C.
## Notes
This library is very incomplete, so there are many unimplemented features and bugs that can result in undefined behavior.
## Usage
### Including the library
Since multitask is an stb-style single header file library, there must be `#define MULTITASK_IMPLEMENTATION` defined in _**one**_ file where the library is included before the `#include "multitask.h"` directive.

All other files in the project, that use the library can include the library without the definition of `MULTITASK_IMPLEMENTATION`.

### Compilation
For compilation I have only used gcc with -masm-intel and -O0 options.
