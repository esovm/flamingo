/* Dynamic (shared) libraries for Flamingo */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "object.h"

#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__unix__)
#include <dlfcn.h>
#define UNIXLIB
#define dlproc(hndl, sym_name) dlsym(hndl, sym_name);
#elif defined(_WIN32)
#define WINLIB
#include <windows.h>
#define dlopen(lib) LoadLibrary(lib)
#define dlproc(hndl, sym_name) GetProcAddress(hndl, sym_name);
#endif

typedef void *(*ArbitraryFn)();

typedef enum {
    NT_INVALID = -1,
    NT_VOID,
    NT_CHAR,
    NT_SHORT,
    NT_INT,
    NT_LONG,
    NT_LLONG,
    NT_DOUBLE,
    LEN
} TypeEnum;

typedef struct {
    bool is_ptr;
    TypeEnum type;
} NativeType;

TypeEnum resolve_string_enum(char *);
NativeType *parse_native_type(char *);
Object *dump_native_type(Env *, Object *);
Object *native_type(Env *, Object *);
Object *dl_open(Env *, Object *);
Object *dl_proc(Env *, Object *);
Object *dl_call(Env *, Object *);
