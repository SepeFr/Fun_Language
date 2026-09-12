#include "hash_map.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interpret.h"
#include "linked_list.h"

/* FNV-1a constants used for bucket indexing. */
const long FNV_offset_basis = 2166136261;
const long FNV_prime = 16777619;

static EnvBindingNode *env_binding_node_create( const char *key )
{
  EnvBindingNode *node = checked_malloc( sizeof( EnvBindingNode ) );
  node->key = checked_strdup( key );
  node->next = NULL;
  node->value = EnvironmentValueStack_create();
  return node;
}

HashMap *hash_map_create( size_t bucket_count, size_t max_key_length )
{
  HashMap *map = checked_malloc( sizeof( HashMap ) );

  map->buckets = checked_malloc( sizeof( EnvBindingList * ) * bucket_count );
  for ( size_t i = 0; i < bucket_count; i++ )
  {
    map->buckets[i] = env_binding_list_create( max_key_length );
  }

  map->max_key_length = max_key_length;
  map->bucket_count = bucket_count;
  return map;
}

HashMap *hash_map_clone( const HashMap *src )
{
  fprintf( stderr, "[DEBUG] hash_map_clone(%p)\n", (void *) src );

  if ( !src )
  {
    return NULL;
  }

  HashMap *clone = hash_map_create( src->bucket_count, src->max_key_length );
  fprintf( stderr, "[DEBUG] clone created: %p\n", (void *) clone );

  for ( size_t i = 0; i < src->bucket_count; i++ )
  {
    EnvBindingList *source_bucket = src->buckets[i];
    if ( env_binding_list_is_empty( source_bucket ) )
    {
      continue;
    }

    EnvBindingNode *current_node = source_bucket->head;

    fprintf( stderr, "[DEBUG] cloning bucket %zu\n", i );
    while ( current_node )
    {
      EnvBindingNode *new_node = checked_malloc( sizeof( EnvBindingNode ) );

      fprintf( stderr, "[DEBUG] cloning node key='%s'\n", current_node->key );
      new_node->key = checked_strdup( current_node->key );

      if ( current_node->value )
      {
        new_node->value = env_value_stack_clone( current_node->value );
      }
      else
      {
        new_node->value = NULL;
      }

      new_node->next = NULL;

      env_binding_list_push_back( clone->buckets[i], new_node );
      current_node = current_node->next;
    }
  }

  return clone;
}

size_t hash_string( const char *string, const HashMap *map )
{
  size_t hash = FNV_offset_basis;
  while ( *string != '\0' )
  {
    hash = hash ^ (size_t) ( *string );
    hash = hash * FNV_prime;
    string++;
  }
  return hash % map->bucket_count;
}

EnvBindingNode *hash_map_insert_binding_stack( HashMap *map, const char *key )
{
  if ( !map || !key )
  {
    fprintf( stderr, "[HashMap] hash_map_insert_binding_stack: map or key is NULL\n" );
    return NULL;
  }

  EnvBindingNode *existing = hash_map_find( map, key );
  if ( existing != NULL )
  {
    printf( " > [Fun Warning] key `%s` already inserted", key );
    return existing;
  }

  EnvBindingNode *new_node = env_binding_node_create( key );
  size_t bucket_index = hash_string( key, map );
  env_binding_list_push_back( map->buckets[bucket_index], new_node );
  return new_node;
}

EnvBindingNode *hash_map_find( HashMap *map, const char *key )
{
  if ( !map || !key )
  {
    return NULL;
  }

  size_t bucket_index = hash_string( key, map );
  return env_binding_list_find_by_key( map->buckets[bucket_index], key );
}

void hash_map_remove( HashMap *map, const char *key )
{
  if ( !map || !key )
  {
    return;
  }

  size_t bucket_index = hash_string( key, map );
  env_binding_list_remove_by_key( map->buckets[bucket_index], key );
}

void hash_map_print( const HashMap *map )
{
  if ( !map )
  {
    printf( "[Warning] Printing HashMap: pointer is NULL\n" );
    return;
  }

  for ( size_t i = 0; i < map->bucket_count; i++ )
  {
    EnvBindingList *list = map->buckets[i];

    if ( list == NULL )
    {
      continue;
    }
    env_binding_list_print( list );
  }
}

void hash_map_destroy( HashMap *map ) { (void) map; }
