#ifndef ANUBIS_MEMUTILS_H
#define ANUBIS_MEMUTILS_H

#define checked_free(ptr) if ((ptr) != NULL) free(ptr)
#define checked_array_free(array, size, element_free_handler) \
	if ((array) != NULL) {\
		for (int i = 0; i < (size); i++) {\
			element_free_handler((array)[i]);\
		}\
	}

#endif // ANUBIS_MEMUTILS_H
