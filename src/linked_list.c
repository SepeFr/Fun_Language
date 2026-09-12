#include "linked_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils_arena.h"

int env_binding_list_is_empty( const EnvBindingList *list ) { return !list || !list->head; }

EnvBindingList *env_binding_list_create( size_t keyMaxLen )
{

  EnvBindingList *list = checked_malloc( sizeof( EnvBindingList ) );
  list->key_maximum_length = keyMaxLen;
  list->head = NULL;
  list->tail = NULL;
  return list;
}

void env_binding_list_push_back( EnvBindingList *list, EnvBindingNode *node )
{
  if ( !list || !node )
  {
    return;
  }

  if ( list->head == NULL )
  {
    list->head = node;
    list->tail = node;
    return;
  }

  list->tail->next = node;
  list->tail = node;
}

EnvBindingNode *env_binding_list_find_by_key( EnvBindingList *list, const char *key )
{

  if ( !list || !key )
  {
    return NULL;
  }

  EnvBindingNode *curr = list->head;
  while ( curr )
  {
    if ( strncmp( curr->key, key, list->key_maximum_length ) == 0 )
    {
      return curr;
    }
    curr = curr->next;
  }

  return NULL;
}

EnvBindingNode *env_binding_list_pop_front( EnvBindingList *list )
{
  if ( env_binding_list_is_empty( list ) )
  {
    return NULL;
  }

  EnvBindingNode *ptr = list->head;

  list->head = list->head->next;
  if ( !list->head )
  {
    list->tail = NULL;
  }

  ptr->next = NULL;
  return ptr;
}

void env_binding_list_remove_by_key( EnvBindingList *list, const char *key )
{
  if ( !key || env_binding_list_is_empty( list ) )
  {
    return;
  }

  if ( strncmp( list->head->key, key, list->key_maximum_length ) == 0 )
  {
    EnvBindingNode *ptr = list->head;
    list->head = list->head->next;
    if ( !list->head )
    {
      list->tail = NULL;
    }
    ptr->next = NULL;
    free( ptr->key );
    free( ptr );
    return;
  }

  EnvBindingNode *curr = list->head;
  EnvBindingNode *prev = NULL;
  while ( curr != NULL )
  {
    if ( strncmp( curr->key, key, list->key_maximum_length ) == 0 )
    {
      prev->next = curr->next;
      if ( curr->next == NULL )
      {
        list->tail = prev;
      }
      curr->next = NULL;
      free( curr->key );
      free( curr );
      return;
    }
    prev = curr;
    curr = curr->next;
  }
}

void env_binding_list_print( const EnvBindingList *list )
{
  if ( !list || !list->head )
  {
    return;
  }

  const EnvBindingNode *curr = list->head;
  printf( "LinkedList: " );
  while ( curr != NULL )
  {
    printf( "%s", curr->key );
    if ( curr->next != NULL )
    {
      printf( " -> " );
    }
    curr = curr->next;
  }
  printf( "\n" );
}

void env_binding_list_destroy( EnvBindingList *list ) { (void) list; }
