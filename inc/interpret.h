#ifndef INTERPRET_H
#define INTERPRET_H
/*
 * Interpreter.
 *
 * The evaluator executes ASTs produced by the parser and returns runtime values.
 * The interpreter supports three evaluation strategies:
 *   - static scoping + eager evaluation
 *   - static scoping + lazy evaluation (thunks)
 *   - dynamic scoping + eager evaluation
 *
 * Environments are modeled as a chain of frames.
 * Each frame maps variable names to stacks of bound values.
 */
#include "dynamic_array.h"
#include "hash_map.h"
#include "parser.h"

typedef enum
{
  ENV_TYPE_INT,
  ENV_TYPE_FN,
  ENV_TYPE_THUNK,
  _ENV_NUM_OF_TYPES
} EnvValueType;

extern const char *EnvValueTypeName[];

typedef struct EnvFrame EnvFrame;

typedef struct EnvFrame
{

  /* Parent frame for nested scopes. */
  EnvFrame *parent;

  /* Bindings introduced by this frame. */
  HashMap *associations;
} EnvFrame;

typedef struct Closure
{
  /* Function parameter name. */
  char *parameter_name;


  /* Function body expression. */
  AST *body_expression;

  /* Captured environment for static scoping; NULL for dynamic scoping. */
  EnvFrame *closure_env;
} Closure;

typedef struct EnvValue EnvValue;

typedef struct Thunk
{
  /* Expression evaluated on demand. */
  AST *expression;

  /* Environment used to force this thunk. */
  EnvFrame *environment;

  /* Evaluation and recursion-detection flags. */
  int is_evaluated;
  int is_evaluating;

  /* Result cached after the first successful evaluation. */
  EnvValue *cached_value;
} Thunk;

typedef struct EnvValue
{
  EnvValueType type;
  union
  {
    Closure function;
    int integer;
    Thunk *thunk;
  } value;
} EnvValue;

/* Stack of values used to represent shadowed identifiers. */
typedef struct EnvironmentValueStack EnvironmentValueStack;
DEFINE_DYNAMIC_ARRAY( EnvironmentValueStack, EnvValue * );

/* Creates a shallow copy of a value stack for hash-map cloning. */
EnvironmentValueStack *env_value_stack_clone( const EnvironmentValueStack *src );

/* Runtime-value constructors. */
EnvValue *env_value_make_int( int value );
EnvValue *env_value_make_fn( Closure fn );
EnvValue *env_value_make_thunk( AST *expr, EnvFrame *env );

/* Evaluate and print an AST using the selected evaluation strategy. */
void run_fun_dynamic_eager( AST *ast );
void run_fun_static_eager( AST *ast );
void run_fun_static_lazy( AST *ast );

#endif // !INTERPRET_H
