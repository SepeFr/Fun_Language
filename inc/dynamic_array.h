#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Lightweight generic dynamic array implemented as a macro.
 *
 * The macro generates:
 *   - struct definition (data/length/capacity)
 *   - create/grow/push/back/pop_back/destroy helpers
 */


#define DYNARRAY_DEFAULT_CAPACITY 1024
#define DYNARRAY_GROWTH_FACTOR 2

#define DEFINE_DYNAMIC_ARRAY( Name, Type )                                                                             \
  typedef struct Name                                                                                                  \
  {                                                                                                                    \
    Type *data;                                                                                                        \
    size_t length;                                                                                                     \
    size_t capacity;                                                                                                   \
  } Name;                                                                                                              \
                                                                                                                       \
  static inline Name *Name##_create( void )                                                                            \
  {                                                                                                                    \
    Name *arr = malloc( sizeof( Name ) );                                                                              \
    if ( !arr )                                                                                                        \
    {                                                                                                                  \
      perror( "Error allocating DynamicArray" );                                                                       \
      exit( EXIT_FAILURE );                                                                                            \
    }                                                                                                                  \
    arr->data = malloc( sizeof( Type ) * DYNARRAY_DEFAULT_CAPACITY );                                                  \
    if ( !arr->data )                                                                                                  \
    {                                                                                                                  \
      perror( "Error allocating DynamicArray data" );                                                                  \
      exit( EXIT_FAILURE );                                                                                            \
    }                                                                                                                  \
    arr->length = 0;                                                                                                   \
    arr->capacity = DYNARRAY_DEFAULT_CAPACITY;                                                                         \
    return arr;                                                                                                        \
  }                                                                                                                    \
                                                                                                                       \
  static inline void Name##_grow( Name *arr )                                                                          \
  {                                                                                                                    \
    arr->capacity *= DYNARRAY_GROWTH_FACTOR;                                                                           \
    Type *tmp = realloc( arr->data, sizeof( Type ) * arr->capacity );                                                  \
    if ( !tmp )                                                                                                        \
    {                                                                                                                  \
      perror( "Error reallocating DynamicArray" );                                                                     \
      exit( EXIT_FAILURE );                                                                                            \
    }                                                                                                                  \
    arr->data = tmp;                                                                                                   \
  }                                                                                                                    \
                                                                                                                       \
  static inline void Name##_push( Name *arr, Type value )                                                              \
  {                                                                                                                    \
    if ( arr->length >= arr->capacity )                                                                                \
    {                                                                                                                  \
      Name##_grow( arr );                                                                                              \
    }                                                                                                                  \
    arr->data[arr->length++] = value;                                                                                  \
  }                                                                                                                    \
                                                                                                                       \
  static inline Type *Name##_back( Name *arr )                                                                         \
  {                                                                                                                    \
    if ( arr->length == 0 )                                                                                            \
    {                                                                                                                  \
      fprintf( stderr, "[Runtime Error] accessing empty variable stack!\n" );                                          \
      exit( EXIT_FAILURE );                                                                                            \
    }                                                                                                                  \
    return &arr->data[arr->length - 1];                                                                                \
  }                                                                                                                    \
                                                                                                                       \
  static inline void Name##_pop_back( Name *arr )                                                                      \
  {                                                                                                                    \
    if ( arr->length == 0 )                                                                                            \
    {                                                                                                                  \
      fprintf( stderr, "[BUG] pop_back on empty EnvironmentValueStack\n" );                                            \
      abort();                                                                                                         \
    }                                                                                                                  \
    arr->length--;                                                                                                     \
  }                                                                                                                    \
                                                                                                                       \
  static inline void Name##_destroy( Name *arr )                                                                       \
  {                                                                                                                    \
    free( arr->data );                                                                                                 \
    arr->data = NULL;                                                                                                  \
    arr->length = 0;                                                                                                   \
    arr->capacity = 0;                                                                                                 \
  }

/* Common dynamic-array specializations. */

DEFINE_DYNAMIC_ARRAY( IntVector, int );
#endif // !DYNAMIC_ARRAY_H
