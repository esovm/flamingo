#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include "util.h"

const char *os_name(void) {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#ifdef _WIN64
    return "Windows (64-bit)";
#else
    return "Windows (32-bit)";
#endif
#elif __APPLE__
    return "macOS";
#elif __linux__
#include <stdint.h>
#if UINTPTR_MAX == 0xffffffff
    return "Linux (32-bit)";
#elif UINTPTR_MAX == 0xffffffffffffffff
    return "Linux (64-bit)";
#else
    return "Linux";
#endif
#elif __FreeBSD__
    return "FreeBSD";
#elif __NetBSD__
    return "NetBSD";
#elif __OpenBSD__
    return "OpenBSD";
#elif __DragonFly__
    return "DragonFly BSD";
#elif __unix__
    return "Unix";
#elif defined(_POSIX_VERSION)
    return "Posix";
#else
    return "Unknown";
#endif
}

const char *get_home(void) {
    const char *ret = getenv("HOME");
    return ret != NULL ? ret : getpwuid(getuid())->pw_dir;
}

char *strip(char *str) {
    /* strip leading space */
    while (isspace((unsigned char)*str))
        ++str;
    if (!*str) /* all spaces? */
        return str;
    /* strip trailing space */
    char *end = &str[strlen(str) - 1];
    while (end > str && isspace((unsigned char)*end))
        --end;
    /* write new null terminator character */
    end[1] = '\0';
    return str;
}
