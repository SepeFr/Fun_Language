#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stddef.h>

/*
 * Singly linked list used for hash-map buckets.
 *
 * Each node binds a variable name to a stack of values. The stack represents
 * variable shadowing across nested scopes.
 */


typedef struct EnvironmentValueStack EnvironmentValueStack;

typedef struct EnvBindingNode
{
  char *key;
  EnvironmentValueStack *value;
  struct EnvBindingNode *next;
} EnvBindingNode;

typedef struct EnvBindingList
{
  EnvBindingNode *head;
  EnvBindingNode *tail;

  /* Maximum key length used for comparisons. */
  size_t key_maximum_length;
} EnvBindingList;


/* Creates an empty list for one hash-map bucket. */
EnvBindingList *env_binding_list_create( size_t keyMaxLen );

/* Performs a linear search for a matching key. */
EnvBindingNode *env_binding_list_find_by_key( EnvBindingList *list, const char *key );


/* Appends a node to the list. */
void env_binding_list_push_back( EnvBindingList *list, EnvBindingNode *node );

/* Removes the first node with a matching key. */
void env_binding_list_remove_by_key( EnvBindingList *list, const char *key );


/* Removes and returns the first node. */
EnvBindingNode *env_binding_list_pop_front( EnvBindingList *list );

/* Writes keys in traversal order to stdout. */
void env_binding_list_print( const EnvBindingList *list );

/* Destroys the list structure. */
void env_binding_list_destroy( EnvBindingList *list );


/* Returns non-zero when list is NULL or empty. */
int env_binding_list_is_empty( const EnvBindingList *list );

#endif // !LINKED_LIST_H
