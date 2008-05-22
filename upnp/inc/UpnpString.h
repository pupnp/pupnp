

#ifndef STRING_H
#define STRING_H


/*!
 * \file
 *
 * \defgroup UpnpString The UpnpString Class
 *
 * \author Marcelo Roberto Jimenez
 *
 * \version 1.0
 *
 * \brief String implementation for the UPnP library.
 *
 * This class implements string operations in the UPnP library.
 *
 * @{
 */


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*!
 * \brief Type of the string objects inside libupnp.
 */
typedef struct s_UpnpString UpnpString;


/*!
 * \brief Constructor.
 *
 * \return A pointer to a new allocated object.
 */
UpnpString *UpnpString_new();


/*!
 * \brief Destructor.
 */
void UpnpString_delete(
	/*! \em \b this pointer. */
	UpnpString *p);


/*!
 * \brief Copy Constructor.
 *
 * \return A pointer to a new allocated copy of the original object.
 */
UpnpString *UpnpString_dup(
	/*! \em \b this pointer. */
	const UpnpString *p);


/*!
 * \brief Assignment operator.
 */
void UpnpString_assign(
	/*! \em \b this pointer. */
	UpnpString *p,
	/*! Pointer to the right hand side object. */
	const UpnpString *q);


/*!
 * \brief Returns the length of the string.
 *
 * \return The length of the string.
 * */
int UpnpString_get_Length(
	/*! \em \b this pointer. */
	const UpnpString *p);


/*!
 * \brief Returns the pointer to char.
 *
 * \return The pointer to char.
 */
const char *UpnpString_get_String(
	/*! \em \b this pointer. */
	const UpnpString *p);


/*!
 * \brief Sets the string from a pointer to char.
 */
void UpnpString_set_String(
	/*! \em \b this pointer. */
	UpnpString *p,
	/*! (char *) to copy from. */
	const char *s);


/*!
 * \brief Sets the string from a pointer to char using a maximum of N chars.
 */
void UpnpString_set_StringN(
	/*! \em \b this pointer. */
	UpnpString *p,
	/*! (char *) to copy from. */
	const char *s,
	/*! Maximum number of chars to copy.*/
	int n);


/*!
 * \brief Clears the string, sets its size to zero.
 */
void UpnpString_clear(
	/*! \em \b this pointer. */
	UpnpString *p);


#ifdef __cplusplus
}
#endif /* __cplusplus */


/* @} UpnpString The UpnpString API */


#endif /* STRING_H */

