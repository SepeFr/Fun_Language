#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>


/*
 * Allocation helpers backed by the global arena.
 */

void *checked_malloc( size_t size );
char *checked_strdup( const char *s );

#endif // !UTILS_H
