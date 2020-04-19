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


static const char *str_types[] = { "void", "char", "short", "int", "long", "long long", "double" };

TypeEnum resolve_string_enum(char *type)
{
    const char str_types_enum[] = { NT_VOID, NT_CHAR, NT_SHORT, NT_INT, NT_LONG, NT_LLONG, NT_DOUBLE };
    for (int i = 0; i < LEN; ++i)
        if (strcmp(type, str_types[i]) == 0)
            return str_types_enum[i];
    return NT_INVALID; /* none found */
}

void *native_object_by_type(Object *obj, NativeType *nt)
{
    if (nt->is_ptr) return obj->r.rawptr;

    switch (nt->type) {
    case NT_CHAR: return &obj->r.u8;
    case NT_SHORT: return &obj->r.u16;
    case NT_INT: return &obj->r.u32;
    case NT_LONG: return &obj->r.integer;
    case NT_LLONG: return &obj->r.integer;
    case NT_DOUBLE: return &obj->r.real;
    }

    return NULL;
}

NativeType *parse_native_type(char *typestr)
{
    NativeType *ret = malloc(sizeof(NativeType));
    if (!ret) return NULL;

    char basetype[32];
    int baseidx = 0;
    int len = strlen(typestr);

    bool invalid = false;

    /* parse character by character */
    for (int i = 0; i < len; ++i) {
        if ((typestr[i] >= 'A' && typestr[i] <= 'Z') ||
            (typestr[i] >= 'a' && typestr[i] <= 'z') ||
            typestr[i] == '*') {
            if (typestr[i] == '*') {
                ret->is_ptr = true;
                /* star indicates it's pointer whatever after it is invalid syntax */
                if (typestr[i+1] != '\0') return NULL;
            } else {
                /* too long */
                if (baseidx > 30) return NULL;
                basetype[baseidx] = tolower(typestr[i]);
                basetype[baseidx + 1] = '\0';
                ++baseidx;
            }
        }
        else {
            return NULL;
        }
    }

    ret->type = resolve_string_enum(basetype);
    if (ret->type == NT_INVALID) return NULL;
    return ret;
}

Object *dump_native_type(Env *env, Object *list)
{
    UNUSED(env);
    NARG("native-dump", list, 1);
    EXPECT("native-type", list, 0, O_RAW);

    NativeType *nt = obj_pop(list, 0)->r.rawptr;

    char res[256];
    snprintf(res, sizeof(res), "%s%s", str_types[nt->type], str_types[nt->is_ptr] ? "" : "*");
    return obj_new_str(res);
}

Object *native_type(Env *env, Object *list)
{
    UNUSED(env);
    NARG("native-type", list, 1);
    EXPECT("native-type", list, 0, O_STRING);

    char *typestr = obj_pop(list, 0)->r.string;

    NativeType *t = parse_native_type(typestr);

    if (!t) return obj_new_err("invalid native type descriptor '%s'", typestr);

    return obj_new_raw(t);
}

Object *dl_open(Env *env, Object *list)
{
    UNUSED(env);
    NARG("dl-open", list, 1);
    EXPECT("dl-open", list, 0, O_STRING);

    Object *libname = obj_pop(list, 0);

    void *handle = dlopen(libname->r.string, RTLD_LAZY);
    return obj_new_raw(handle);
}

Object *dl_proc(Env *env, Object *list)
{
    NARG("dl-proc", list, 2);
    /* handle */
    EXPECT("dl-proc", list, 0, O_RAW);
    EXPECT("dl-proc", list, 1, O_STRING);

    Object *handle = obj_pop(list, 0);
    Object *procname = obj_pop(list, 0);

    void *handle_proc = dlproc(handle, procname->r.string);
    return obj_new_raw(handle_proc);
}

Object *dl_call(Env *env, Object *list)
{
    NARG("dl-call", list, 4);
    /* proc handle */
    EXPECT("dl-call", list, 0, O_RAW);
    EXPECT("dl-call", list, 1, O_RAW);
    EXPECT("dl-call", list, 2, O_BEXPR);
    EXPECT("dl-call", list, 3, O_BEXPR);

    char type;
    Object *handle = obj_pop(list, 0),
        *return_type = obj_pop(list, 0),
        *param_types = obj_pop(list, 0),
        *param_values = obj_pop(list, 0);
#ifdef UNIXLIB

    *(void**) (&handle_proc) = handle->r.rawptr;
    return obj_new_raw(handle_proc());
#endif
}
