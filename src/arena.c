#include "arena.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Process-wide arena used by interpreter-owned allocations. */
static Arena global_arena = { 0 };

void arena_init( size_t size )
{
  global_arena.memory = malloc( size );
  if ( !global_arena.memory )
  {
    fprintf( stderr, "[Arena] Failed to allocate %zu bytes\n", size );
    exit( EXIT_FAILURE );
  }
  global_arena.size = size;
  global_arena.used = 0;
  fprintf( stderr, "[Arena] Initialized with %zu bytes (%.2f MB)\n", size, size / ( 1024.0 * 1024.0 ) );
}

void *arena_alloc( size_t size )
{
  /* Round up to an 8-byte boundary to preserve alignment. */
  size = ( size + 7 ) & ~7;

  if ( global_arena.used + size > global_arena.size )
  {
    fprintf( stderr, "[Arena] Out of memory! Requested: %zu, Used: %zu, Total: %zu\n", size, global_arena.used,
             global_arena.size );
    exit( EXIT_FAILURE );
  }

  void *ptr = global_arena.memory + global_arena.used;
  global_arena.used += size;
  return ptr;
}

char *arena_strdup( const char *s )
{
  if ( !s )
    return NULL;

  size_t len = strlen( s ) + 1;
  char *copy = arena_alloc( len );
  memcpy( copy, s, len );
  return copy;
}

void arena_destroy( void )
{
  if ( global_arena.memory )
  {
    fprintf( stderr,
             "[Arena] Freeing arena. Used: %zu / %zu bytes (%.2f MB / %.2f MB, "
             "%.1f%% utilization)\n",
             global_arena.used, global_arena.size, global_arena.used / ( 1024.0 * 1024.0 ),
             global_arena.size / ( 1024.0 * 1024.0 ), ( global_arena.used * 100.0 ) / global_arena.size );
    free( global_arena.memory );
    global_arena.memory = NULL;
    global_arena.size = 0;
    global_arena.used = 0;
  }
}

void arena_print_stats( void )
{
  fprintf( stderr,
           "[Arena Stats] Used: %zu / %zu bytes (%.2f MB / %.2f MB, %.1f%% "
           "utilization)\n",
           global_arena.used, global_arena.size, global_arena.used / ( 1024.0 * 1024.0 ),
           global_arena.size / ( 1024.0 * 1024.0 ), ( global_arena.used * 100.0 ) / global_arena.size );
}
