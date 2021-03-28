#include "UpnpString.h"

#include <stdio.h>
#include <stdlib.h>

#define PRINTF_GOTO(msg, n) \
        printf("%s:%d: Error: %s.\n", __FILE__, __LINE__, msg); \
        goto error##n;

int main()
{
        int ret_code = EXIT_FAILURE;
        UpnpString *s;
        UpnpString *t;
        UpnpString *u;
        UpnpString *v;
        UpnpString *w;
        size_t len;
        const char test1[] = "TeSt";
        const char test2[] = "tEsT";
        const char test3[] = "atest";
        const char test4[] = "ztest";

        s = UpnpString_new();
        if (!s) {
                PRINTF_GOTO("UpnpString_new() returned NULL", 1);
        }
        t = UpnpString_new();
        if (!t) {
                PRINTF_GOTO("UpnpString_new() returned NULL", 2);
        }
        u = UpnpString_new();
        if (!u) {
                PRINTF_GOTO("UpnpString_new() returned NULL", 3);
        }
        v = UpnpString_new();
        if (!v) {
                PRINTF_GOTO("UpnpString_new() returned NULL", 4);
        }
        len = UpnpString_get_Length(s);
        if (len) {
                PRINTF_GOTO("New string should have zero lenght", 5);
        }
        UpnpString_set_String(s, test1);
        UpnpString_set_String(t, test2);
        UpnpString_set_String(u, test3);
        UpnpString_set_String(v, test4);
        len = UpnpString_get_Length(s);
        if (len != 4) {
                PRINTF_GOTO("New string should have lenght 4", 5);
        }
        if (!UpnpString_cmp(s, t)) {
                PRINTF_GOTO("Strings should have compared different", 5);
        }
        if (!UpnpString_cmp(s, u)) {
                PRINTF_GOTO("Strings should have compared different", 5);
        }
        if (UpnpString_casecmp(s, t)) {
                PRINTF_GOTO("Strings should have compared equal", 5);
        }
        if (UpnpString_casecmp(s, u) < 0) {
                PRINTF_GOTO("Strings should have compared less than", 5);
        }
        if (UpnpString_casecmp(s, v) > 0) {
                PRINTF_GOTO("Strings should have compared greater than", 5);
        }
        w = UpnpString_dup(s);
        if (UpnpString_casecmp(s, w)) {
                PRINTF_GOTO("Strings should have compared equal", 6);
        }
        UpnpString_assign(w, s);
        if (UpnpString_casecmp(s, w)) {
                PRINTF_GOTO("Strings should have compared equal", 6);
        }

        ret_code = EXIT_SUCCESS;

error6:
        UpnpString_delete(w);
error5:
        UpnpString_delete(v);
error4:
        UpnpString_delete(u);
error3:
        UpnpString_delete(t);
error2:
        UpnpString_delete(s);
error1:
        return ret_code;
}
