#ifndef HASH_MAP_H
#define HASH_MAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linked_list.h"
#include "utils_arena.h"

#define DEFAULT_HASHMAP_SIZE 1024
#define DEFAULT_KEY_SIZE 64

/*
 * Hash map used for environment variable lookup.
 *
 * Each key maps to an EnvBindingNode containing a stack of values. The stack
 * supports variable shadowing:
 *   let x = 1 in (let x = 2 in x)
 *
 * Buckets are implemented as linked lists. Storage is arena-backed, so
 * destruction does not release individual allocations.
 */

typedef struct HashMap
{
  EnvBindingList **buckets;
  size_t bucket_count;
  size_t max_key_length;
} HashMap;


/* Creates a map with a fixed bucket count and maximum key length. */
HashMap *hash_map_create( size_t bucket_count, size_t max_key_length );

/* Copies buckets and bindings; value stacks are cloned shallowly. */
HashMap *hash_map_clone( const HashMap *src );

/* Retained for ownership symmetry; arena-backed storage is released at once. */
void hash_map_destroy( HashMap *map );

/*
 * Inserts a key and returns its binding node. Returns the existing binding
 * when the key is already present.
 */
EnvBindingNode *hash_map_insert_binding_stack( HashMap *map, const char *key );

/* Returns the binding node for a key, or NULL when it is absent. */
EnvBindingNode *hash_map_find( HashMap *map, const char *key );


/* Removes a key from the map. */
void hash_map_remove( HashMap *map, const char *key );

/* Writes all buckets and keys to stdout. */
void hash_map_print( const HashMap *map );

/* Computes an FNV-1a bucket index for a key. */
size_t hash_string( const char *string, const HashMap *map );

#endif // HASH_MAP_H
