#include "UpnpString.h"

#include <stdio.h>
#include <stdlib.h>

#define PRINTF_GOTO(msg) \
        printf("%s:%d: Error: %s.\n", __FILE__, __LINE__, msg); \
        goto error;

int main()
{
        UpnpString *s;
        UpnpString *t;
        UpnpString *u;
        UpnpString *v;
        size_t len;
        const char test1[] = "TeSt";
        const char test2[] = "tEsT";
        const char test3[] = "atest";
        const char test4[] = "ztest";

        s = UpnpString_new();
        if (!s) {
                PRINTF_GOTO("UpnpString_new() returned NULL");
        }
        t = UpnpString_new();
        if (!t) {
                PRINTF_GOTO("UpnpString_new() returned NULL");
        }
        u = UpnpString_new();
        if (!u) {
                PRINTF_GOTO("UpnpString_new() returned NULL");
        }
        v = UpnpString_new();
        if (!v) {
                PRINTF_GOTO("UpnpString_new() returned NULL");
        }
        len = UpnpString_get_Length(s);
        if (len) {
                PRINTF_GOTO("New string should have zero lenght")
        }
        UpnpString_set_String(s, test1);
        UpnpString_set_String(t, test2);
        UpnpString_set_String(u, test3);
        UpnpString_set_String(v, test4);
        if (!UpnpString_cmp(s, t)) {
                PRINTF_GOTO("Strings should have compared different");
        }
        if (!UpnpString_cmp(s, u)) {
                PRINTF_GOTO("Strings should have compared different");
        }
        if (UpnpString_casecmp(s, t)) {
                PRINTF_GOTO("Strings should have compared equal");
        }
        if (UpnpString_casecmp(s, u) < 0) {
                PRINTF_GOTO("Strings should have compared less than");
        }
        if (UpnpString_casecmp(s, v) > 0) {
                PRINTF_GOTO("Strings should have compared greater than");
        }

        return EXIT_SUCCESS;
error:
        return EXIT_FAILURE;
}
