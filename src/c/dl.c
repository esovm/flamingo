/* Dynamic (shared) libraries for Flamingo */

#include <stddef.h>

#include "object.h"

#if defined(__linux__) || defined(__APPLE__) || defined(__MACH__) \
    || defined(__FreeBSD__) || defined(__unix__)
#include <dlfcn.h>
#define UNIXLIB
#elif defined(_WIN32)
/* fuck windows, we'll deal with you later */
#define WINLIB
#include <windows.h>
#define dlopen(lib) LoadLibrary(lib)
#endif

typedef struct {
    bool is_ptr;
    enum {
        VOID,
        CHAR,
        SHORT,
        INT,
        LONG,
        LLONG,
        DOUBLE
    } type;
} NativeType;

NativeType *get_native_type(char *typestr)
{
    NativeType *ret = malloc(sizeof(NativeType));
    if (!ret) return NULL;

    char basetype[32];
    int baseidx = 0;
    int len = strlen(typestr);
    if (len < 0) return NULL;

    bool invalid = false;

    /* TODO: complete dl.c */

    /* parse character by character */
    for (int i = 0; i < len; ++i) {
        if (typestr[i] >= 'A' && typestr[i] <= 'Z' ||
            typestr[i] >= 'a' && typestr[i] <= 'z' ||
            typestr[i] == '*') {
            if (typestr[i] == '*') {
                ret->is_ptr = true;
            } else {
                basetype[baseidx] = typestr[i];
            }
        }
        else return NULL;
    }
}

Object *native_type(Env *env, Object *list)
{
    UNUSED(env);
    NARG("native-type", list, 1);
    EXPECT("native-type", list, 0, O_STRING);

    char *typestr = obj_pop(list, 0)->r.string;

    NativeType *t = get_native_type(typestr);

    if (t == NULL) return obj_new_error("invalid native type descriptor '%s'", typestr);

    return obj_new_raw(t);
}

Object *dl_open(Env *env, Object *list)
{
    UNUSED(env);
    NARG("dl-open", list, 1);
    EXPECT("dl-open", list, 0, O_STRING);

    Object *libname = obj_pop(list, 0);

#ifdef UNIXLIB
    void *handle = dlopen(libname->r.string, RTLD_LAZY);
    return obj_new_raw(handle);
#endif
}

Object *dl_proc(Env *env, Object *list)
{
    NARG("dl-proc", list, 2);
    /* handle */
    EXPECT("dl-proc", list, 0, O_RAW);
    EXPECT("dl-proc", list, 1, O_STRING);

    Object *handle = obj_pop(list, 0);
    Object *procname = obj_pop(list, 0);

#ifdef UNIXLIB
    void *handle_proc = dlsym(handle, procname->r.string);
    return obj_new_raw(handle_proc);
#endif
}

Object *dl_call(Env *env, Object *list)
{
    NARG("dl-call", list, 4);
    /* proc handle */
    EXPECT("dl-call", list, 0, O_RAW);
    EXPECT("dl-call", list, 1, O_STRING);
    EXPECT("dl-call", list, 2, O_BEXPR);
    EXPECT("dl-call", list, 3, O_BEXPR);


    Object *handle = obj_pop(list, 0);
    Object *return_type = obj_pop(list, 0);

    char type;

    if (strcmp(return_type, "double") == 0) {

    }

    Object *param_types = obj_pop(list, 0);
    Object *param_values = obj_pop(list, 0);

// #ifdef UNIXLIB
//     void *handle_proc = (handle, procname->r.string);
//     return obj_new_raw(handle_proc);
// #endif
}
