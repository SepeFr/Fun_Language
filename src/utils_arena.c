#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arena.h"

void *checked_malloc( size_t size )
{
  /* Interpreter allocations share the global arena. */
  return arena_alloc( size );
}

char *checked_strdup( const char *s )
{
  /* String storage follows the arena lifetime. */
  return arena_strdup( s );
}
