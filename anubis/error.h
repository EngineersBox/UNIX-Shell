#ifndef ANUBIS_ERROR_H
#define ANUBIS_ERROR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

// Use this _DEBUG variable to control the error messages printed
// by the ERROR function.
//  _DEBUG=0: print only one error message; 
//  _DEBUG=1: print custom messages.
// Make sure you set _DEBUG to 0 for the submitted version. 

#define _DEBUG 0

void ERROR(int errnum, const char * format, ... );

#endif // ANUBIS_ERROR_H
