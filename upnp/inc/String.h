

#ifndef STRING_H
#define STRING_H


/** \file */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/** Type of the string objects inside libupnp. */
typedef struct {} UpnpString;


/** Constructor */
UpnpString *UpnpString_new();

/** Destructor */
void UpnpString_delete(UpnpString *p);

/** Copy Constructor */
UpnpString *UpnpString_dup(const UpnpString *p);

/** Assignment operator */
void UpnpString_assign(UpnpString *q, const UpnpString *p);

/** The length of the string */
int UpnpString_get_Length(const UpnpString *p);

/** The pointer to char */
const char *UpnpString_get_String(const UpnpString *p);
void UpnpString_set_String(UpnpString *p, const char *s);
void UpnpString_set_StringN(UpnpString *p, const char *s, int n);

/* Clears the string, sets its size to zero */
void UpnpString_clear(UpnpString *p);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* STRING_H */

