#include "error.h"

void ERROR(int errnum, const char * format, ... ) {
    if(_DEBUG) {
        va_list args;
        va_start (args, format);
        vfprintf(stderr, format, args);
        va_end (args);
        if(errnum > 0) fprintf(stderr, ": %s", strerror(errnum));
        fprintf(stderr, "\n"); 
        return; 
    }
    fprintf(stderr, "An error has occurred\n"); 
}
