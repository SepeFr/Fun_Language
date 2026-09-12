#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>

/*
 * Simple arena allocator.
 *
 * Objects are allocated from one shared buffer and released together when the
 * interpreter finishes.
 */
typedef struct Arena
{

  /* Base address of the allocated buffer. */
  char *memory;

  /* Total capacity in bytes. */
  size_t size;

  /* Number of bytes allocated so far. */
  size_t used;
} Arena;

/* Allocates the global arena buffer and resets its allocation state. */
void arena_init( size_t size );

/* Returns an aligned chunk of memory from the arena */
void *arena_alloc( size_t size );

/* Copies a null-terminated string into the arena. */
char *arena_strdup( const char *s );

/* Frees the entire arena buffer and resets the global arena state */
void arena_destroy( void );

/* Writes current arena usage statistics to stderr. */
void arena_print_stats( void );

#endif // ARENA_H
