#ifndef ANUBIS_MATHUTILS_H
#define ANUBIS_MATHUTILS_H

#define MAX(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a > _b ? _a : _b; })
#define MIN(a,b) ({ __typeof__ (a) _a = (a); __typeof__ (b) _b = (b); _a < _b ? _a : _b; })
#define DEC_FLOOR(value) ((value) > 0 ? (value) - 1 : (value))

#endif // ANUBIS_MATHUTILS_H
