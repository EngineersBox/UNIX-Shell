#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h> 
#include <fcntl.h>
#include <assert.h>
#include <stdarg.h>


// Use this _DEBUG variable to control the error messages printed
// by the ERROR function.
//  _DEBUG=0: print only one error message; 
//  _DEBUG=1: print custom messages.
// Make sure you set _DEBUG to 0 for the submitted version. 

int _DEBUG = 0;

void ERROR(int errnum, const char * format, ... )
{
    if(_DEBUG) {
        va_list args;
        va_start (args, format);
        vfprintf(stderr, format, args);
        va_end (args);
        if(errnum > 0) fprintf(stderr, " : %s", strerror(errnum));
        fprintf(stderr, "\n"); 
        return; 
    }

    fprintf(stderr, "An error has occurred\n"); 
}

// TODO: implement your shell!

int main(int argc, char *argv[])
{

    return 0;
}
