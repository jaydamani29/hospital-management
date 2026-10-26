#ifndef UTIL_H
#define UTIL_H
#include <stddef.h>

/* Safe input helpers (no scanf, no buffer overflows). */
void read_line(const char *prompt, char *buf, size_t size);
int  read_int(const char *prompt, int lo, int hi);
void read_blood(const char *prompt, char *out);      /* out must hold 4 bytes */
int  valid_blood(const char *s);
int  blood_compatible(const char *donor, const char *recipient); /* ABO rule */

#endif
