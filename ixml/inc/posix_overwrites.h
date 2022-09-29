#ifndef POSIX_OVERWRTIES_H
#define POSIX_OVERWRTIES_H
#ifdef _WIN32

	/* POSIX names for functions */
	#define fileno _fileno
	#define unlink _unlink
	#define strcasecmp _stricmp
	#define strdup _strdup
	#define stricmp _stricmp
	#define strncasecmp strnicmp
	#define strnicmp _strnicmp

	/* Secure versions of functions */
	#if 0
		/*
		 * The current issues with those 4 defines:
		 * - strncpy redefinition is wrong
		 * - Theses functions assume they are being called on C arrays
		 * only. Using `countof` on a heap allocated pointer is
		 * undefined behavior and `sizeof` will only return the byte
		 * size of the pointer.
		 *
		 * The reason we can't pin-point the places where it fails is
		 * because *_s functions have a significantly different
		 * behaviour than the replaced functions and have actual error
		 * returns values that are simply ignored here, leading to
		 * numerous unseen regressions.
		 *
		 * A first step could be to actually crash or log on _s failures
		 * to detect the potentials overflows or bad usages of the
		 * wrappers.
		 */
		#define strcat(arg1, arg2) strcat_s(arg1, sizeof(arg1), arg2)
		#define strcpy(arg1, arg2) strcpy_s(arg1, _countof(arg1), arg2)
		#define strncpy(arg1, arg2, arg3) \
			strncpy_s(arg1, arg3, arg2, arg3)
		#define sprintf(arg1, ...) \
			sprintf_s(arg1, sizeof(arg1), __VA_ARGS__)
	#endif

#endif /* _WIN32 */
#endif /* POSIX_OVERWRTIES_H */
