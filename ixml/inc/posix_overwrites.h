#ifndef POSIX_OVERWRTIES_H
#define POSIX_OVERWRTIES_H
#ifdef _WIN32

	#define fileno _fileno
	#define unlink _unlink
	#define strcasecmp _stricmp
	#define strdup _strdup
	#define stricmp _stricmp
	#define strncasecmp strnicmp
	#define strnicmp _strnicmp

#endif /* _WIN32 */
#endif /* POSIX_OVERWRTIES_H */
