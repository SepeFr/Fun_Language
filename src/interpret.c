#include "interpret.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "global.h"
#include "hash_map.h"
#include "linked_list.h"
#include "parser.h"
#include "utils_arena.h"

/* Evaluator configuration. */

typedef enum
{
  EVAL_DYNAMIC_EAGER,
  EVAL_STATIC_EAGER,
  EVAL_STATIC_LAZY
} EvalMode;
const char *EnvValueTypeName[] = { "ENV_TYPE_INT", "ENV_TYPE_FN", "ENV_TYPE_THUNK" };


static void env_frame_destroy( EnvFrame *frame );
static void env_frame_destroy( EnvFrame *frame )
{
  (void) frame;
  /* The arena owns all interpreter allocations. */
}


/* Forward declarations for mutually recursive evaluator helpers. */
static EnvValue *eval( AST *ast, EnvFrame *env, EvalMode mode );
static EnvValue *force_thunk( EnvValue *v );

/* Runtime errors are fatal because the interpreter has no recovery path. */
static void runtime_error( const char *message )
{
  fprintf( stderr, "[Run_Time_Error] %s\n", message );
  exit( EXIT_FAILURE );
}

static void unbound_variable_error( const char *name )
{
  fprintf( stderr, "[Run_Time_Error] Unbound variable '%s'\n", name );
  exit( EXIT_FAILURE );
}

/* Runtime-value constructors. */

/* Allocates a runtime value container of a given type (int, function, thunk). */
static EnvValue *env_value_new( EnvValueType type )
{
  EnvValue *env_value = checked_malloc( sizeof( EnvValue ) );
  env_value->type = type;
  return env_value;
}

/* Constructs integer and function runtime values. */
EnvValue *env_value_make_int( int value )
{
  EnvValue *env_value = env_value_new( ENV_TYPE_INT );
  env_value->value.integer = value;
  return env_value;
}

EnvValue *env_value_make_fn( Closure fn )
{
  EnvValue *env_value = env_value_new( ENV_TYPE_FN );
  env_value->value.function = fn;
  return env_value;
}

/*
 * creates a thunk for lazy evaluation.
 *
 * thunks stores an expression + the environment needed to evaluate it later.
 * It also caches the result after the first evaluation.
 */
EnvValue *env_value_make_thunk( AST *expr, EnvFrame *env )
{
  Thunk *th = checked_malloc( sizeof( Thunk ) );
  th->expression = expr;

  th->environment = env;

  th->is_evaluated = 0;
  th->is_evaluating = 0;
  th->cached_value = NULL;

  EnvValue *v = env_value_new( ENV_TYPE_THUNK );
  v->value.thunk = th;
  return v;
}


/* Environment helpers. */

EnvironmentValueStack *env_value_stack_clone( const EnvironmentValueStack *src )
{
  if ( !src )
    return NULL;

  EnvironmentValueStack *clone = EnvironmentValueStack_create();
  clone->capacity = src->capacity;
  clone->length = src->length;

  clone->data = checked_malloc( sizeof( EnvValue * ) * clone->capacity );

  for ( size_t i = 0; i < src->length; i++ )
  {
    clone->data[i] = src->data[i];
  }

  return clone;
}


/* Creates a new environment frame linked to parent. */
EnvFrame *env_frame_new( struct EnvFrame *env_log )
{
  EnvFrame *log = checked_malloc( sizeof( EnvFrame ) );
  log->associations = hash_map_create( DEFAULT_HASHMAP_SIZE, DEFAULT_KEY_SIZE );
  log->parent = env_log;
  return log;
}

/* Looks up a variable binding stack by walking outward through parent frames. */
EnvBindingNode *env_frame_get_variable_stack( EnvFrame *env_frame_head, const char *key )
{
  for ( EnvFrame *env_frame = env_frame_head; env_frame != NULL; env_frame = env_frame->parent )
  {
    assert( env_frame->associations != NULL );

    EnvBindingNode *node = hash_map_find( env_frame->associations, key );

    if ( node != NULL )
    {
      return node;
    }
  }
  return NULL;
}

/* AST evaluation. */

/* Evaluates an integer literal from the AST */
static EnvValue *eval_int_literal( AST *ast )
{
  if ( !ast || !ast->matched_token || !ast->matched_token->string )
  {
    runtime_error( "Invalid integer literal AST node" );
  }
  int value = atoi( ast->matched_token->string );
  return env_value_make_int( value );
}

/*
 * Evaluates a '+' node
 * In lazy mode we force operands first because addition needs concrete integers.
 */
static EnvValue *eval_sum( AST *ast, EnvFrame *env, EvalMode mode )
{
  if ( !ast->left || !ast->right )
  {
    runtime_error( "Sum node must have both left and right operands" );
  }

  EnvValue *left_val = eval( ast->left, env, mode );
  EnvValue *right_val = eval( ast->right, env, mode );

  if ( mode == EVAL_STATIC_LAZY )
  {
    left_val = force_thunk( left_val );
    right_val = force_thunk( right_val );
  }

  if ( left_val->type != ENV_TYPE_INT || right_val->type != ENV_TYPE_INT )
  {
    runtime_error( "Sum is only defined for integer values" );
  }

  int sum_res = left_val->value.integer + right_val->value.integer;
  return env_value_make_int( sum_res );
}


/*
 * Evaluates a variable reference.
 * - eager modes return the bound value directly
 * - lazy mode forces thunks on lookup so variables behave like values
 */
static EnvValue *eval_variable( AST *ast, EnvFrame *env, EvalMode mode )
{
  if ( !ast->matched_token || !ast->matched_token->string )
  {
    runtime_error( "Variable node without a name" );
  }

  char *name = ast->matched_token->string;
  EnvBindingNode *node = env_frame_get_variable_stack( env, name );
  if ( !node )
  {
    unbound_variable_error( name );
  }

  EnvironmentValueStack *assignments = node->value;

  EnvValue *top = *EnvironmentValueStack_back( assignments );

  if ( mode == EVAL_STATIC_LAZY )
  {
    return force_thunk( top );
  }

  return top;
}


/*
 * Evaluates `let x = expr in body`.
 *
 * A new frame extends the current environment. Eager modes bind the evaluated
 * expression; lazy mode binds a thunk that captures the current environment.
 */
static EnvValue *eval_let_in( AST *ast, EnvFrame *env, EvalMode mode )
{
  AST *let_node = ast->left;
  if ( !let_node || !let_node->matched_token || let_node->matched_token->token != PARSER_TOKEN_LET )
  {
    runtime_error( "Malformed let-in: missing 'let' node" );
  }

  AST *var_node = let_node->left;
  if ( !var_node || !var_node->matched_token || var_node->matched_token->token != PARSER_TOKEN_VARIABLE )
  {
    runtime_error( "Malformed let-in: invalid variable name" );
  }

  char *var_name = var_node->matched_token->string;

  AST *assignment_expr = let_node->right;
  if ( !assignment_expr )
  {
    runtime_error( "Malformed let-in: missing assignment expression" );
  }

  AST *body = ast->right;
  if ( !body )
  {
    runtime_error( "Malformed let-in: missing body expression" );
  }

  EnvFrame *new_env_frame = env_frame_new( env );
  EnvBindingNode *stack = hash_map_insert_binding_stack( new_env_frame->associations, var_name );


  switch ( mode )
  {
    case EVAL_DYNAMIC_EAGER:
    case EVAL_STATIC_EAGER:
    {
      EnvValue *assignment_value = eval( assignment_expr, env, mode );
      EnvironmentValueStack_push( stack->value, assignment_value );
      break;
    }
    case EVAL_STATIC_LAZY:
    {
      EnvValue *thunk_value = env_value_make_thunk( assignment_expr, new_env_frame );
      EnvironmentValueStack_push( stack->value, thunk_value );
      break;
    }
  }

  EnvValue *result = eval( body, new_env_frame, mode );
  return result;
}


/*
 * Evaluates a function literal `fn x => body`
 * Static modes capture the current environment. Dynamic mode leaves
 * closure_env NULL and resolves names in the caller environment.
 */
static EnvValue *eval_function( AST *ast, EnvFrame *env, EvalMode mode )
{
  AST *param_node = ast->left;
  AST *body = ast->right;

  if ( !param_node || !param_node->matched_token || param_node->matched_token->token != PARSER_TOKEN_VARIABLE )
  {
    runtime_error( "Malformed function: missing parameter name" );
  }
  if ( !body )
  {
    runtime_error( "Malformed function: missing body" );
  }

  Closure fn = {
    .parameter_name = param_node->matched_token->string,
    .body_expression = body,
    .closure_env = NULL,
  };

  EnvValue *fn_value = env_value_make_fn( fn );

  if ( mode == EVAL_STATIC_EAGER || mode == EVAL_STATIC_LAZY )
  {
    fn_value->value.function.closure_env = env;
  }

  return fn_value;
}


/* Function-application strategies. */

/*
 * Dynamic scoping binds the argument in the caller environment, evaluates the
 * body, then restores the previous binding.
 */
static EnvValue *apply_dynamic_eager( Closure fn, AST *arg_expr, EnvFrame *env, EvalMode mode )
{
  EnvValue *arg_value = eval( arg_expr, env, mode );

  char *var_name = fn.parameter_name;
  EnvBindingNode *stack = env_frame_get_variable_stack( env, var_name );
  if ( !stack )
  {
    stack = hash_map_insert_binding_stack( env->associations, var_name );
  }


  EnvironmentValueStack_push( stack->value, arg_value );

  EnvValue *result = eval( fn.body_expression, env, mode );
  EnvironmentValueStack_pop_back( stack->value );
  return result;
}

/*
 * Static eager evaluation computes the argument before evaluating the body in
 * an environment extending the function's captured environment.
 */

static EnvValue *apply_static_eager( Closure fn, AST *arg_expr, EnvFrame *caller_env, EvalMode mode )
{

  EnvValue *arg_value = eval( arg_expr, caller_env, mode );

  EnvFrame *extended_env = env_frame_new( fn.closure_env );

  EnvBindingNode *stack = hash_map_insert_binding_stack( extended_env->associations, fn.parameter_name );
  EnvironmentValueStack_push( stack->value, arg_value );

  EnvValue *result = eval( fn.body_expression, extended_env, mode );

  return result;
}


/*
 * Static lazy evaluation binds the parameter to an argument thunk. The thunk
 * is forced only when the parameter is needed.
 */
static EnvValue *apply_static_lazy( Closure fn, AST *arg_expr, EnvFrame *caller_env, EvalMode mode )
{
  EnvFrame *extended_env = env_frame_new( fn.closure_env );

  EnvBindingNode *stack = hash_map_insert_binding_stack( extended_env->associations, fn.parameter_name );

  EnvValue *thunk_value = env_value_make_thunk( arg_expr, caller_env );
  EnvironmentValueStack_push( stack->value, thunk_value );

  EnvValue *result = eval( fn.body_expression, extended_env, mode );

  return result;
}

/*
 * Evaluates function application.
 *
 * The left side must evaluate to a function value. The selected mode determines
 * the application strategy.
 */
static EnvValue *eval_apply( AST *ast, EnvFrame *env, EvalMode mode )
{
  if ( INTERPRETER_DEBUG )
  {
    printf( "[INTERPRETER_DEBUG] entering Apply \n" );
  }

  AST *lhs = ast->left;
  AST *rhs = ast->right;

  if ( !lhs || !rhs )
  {
    runtime_error( "Application node requires both lhs and rhs" );
  }

  EnvValue *lhs_result = eval( lhs, env, mode );

  if ( lhs_result->type != ENV_TYPE_FN )
  {
    runtime_error( "Left-hand side of application is not a function" );
  }

  Closure fn = lhs_result->value.function;

  switch ( mode )
  {
    case EVAL_DYNAMIC_EAGER:
      return apply_dynamic_eager( fn, rhs, env, mode );
    case EVAL_STATIC_EAGER:
      return apply_static_eager( fn, rhs, env, mode );
    case EVAL_STATIC_LAZY:
    {
      if ( INTERPRETER_DEBUG )
      {
        printf( "[INTERPRETER_DEBUG] Exiting Apply \n" );
      }
      return apply_static_lazy( fn, rhs, env, mode );
    }
  }

  runtime_error( "Unknown evaluation mode in application" );

  return NULL;
}

/* Lazy-evaluation support. */

/*
 * Forces a thunk in lazy mode.
 * Detects self-recursive forcing, evaluates the expression once, and caches
 * the result.
 */
static EnvValue *force_thunk( EnvValue *v )
{
  if ( v->type != ENV_TYPE_THUNK )
  {
    return v;
  }

  Thunk *th = v->value.thunk;

  if ( th->is_evaluating )
  {
    runtime_error( "Infinite loop detected: thunk references itself during evaluation" );
  }

  if ( !th->is_evaluated )
  {
    th->is_evaluating = 1;

    EnvValue *res = eval( th->expression, th->environment, EVAL_STATIC_LAZY );

    th->cached_value = force_thunk( res );
    th->is_evaluated = 1;
    th->is_evaluating = 0;
  }

  return th->cached_value;
}


/*
 * Dispatches evaluation according to the AST node kind.
 */
static EnvValue *eval( AST *ast, EnvFrame *env, EvalMode mode )
{
  if ( !ast || !ast->matched_token )
  {
    runtime_error( "Attempted to evaluate a NULL AST node" );
  }

  switch ( ast->matched_token->token )
  {
    case PARSER_TOKEN_INT:
      return eval_int_literal( ast );

    case PARSER_TOKEN_SUM:
      return eval_sum( ast, env, mode );

    case PARSER_TOKEN_VARIABLE:
      return eval_variable( ast, env, mode );

    case PARSER_TOKEN_IN:
      return eval_let_in( ast, env, mode );

    case PARSER_TOKEN_FN:
      return eval_function( ast, env, mode );

    case PARSER_TOKEN_APPLY:
      return eval_apply( ast, env, mode );

    default:
      runtime_error( "Unknown AST node type in eval" );
      return NULL;
  }
}

EnvValue *eval_fun_dynamic_eager( AST *ast, EnvFrame *env ) { return eval( ast, env, EVAL_DYNAMIC_EAGER ); }

EnvValue *eval_fun_static_eager( AST *ast, EnvFrame *env ) { return eval( ast, env, EVAL_STATIC_EAGER ); }

EnvValue *eval_fun_static_lazy( AST *ast, EnvFrame *env ) { return eval( ast, env, EVAL_STATIC_LAZY ); }

static void print_result( const EnvValue *res )
{
  const char *type_string = EnvValueTypeName[res->type];
  printf( "Program Terminated!\n" );
  printf( "Resulting Type : `%s`, with value : ", type_string );

  switch ( res->type )
  {
    case ENV_TYPE_FN:
      printf( "(variable : `%s`, body : `", res->value.function.parameter_name );
      AST_print_no_new_line( res->value.function.body_expression, 1 );
      printf( "`)\n" );
      break;
    case ENV_TYPE_INT:
      printf( "`%d`\n", res->value.integer );
      break;
    case ENV_TYPE_THUNK:
      printf( "<thunk>\n" );
      break;
    default:
      printf( "<unknown>\n" );
      break;
  }
  printf( "\n" );
}

static void run_fun_common( AST *ast, EnvValue *( *eval_fun )( AST *ast, EnvFrame *env ) )
{
  if ( !ast )
  {
    runtime_error( "run_fun_* called with NULL AST" );
  }

  if ( PARSER_DEBUG )
  {
    AST_print( ast, 0 );
  }

  EnvFrame *env_frame = env_frame_new( NULL );
  EnvValue *res = eval_fun( ast, env_frame );

  print_result( res );
  env_frame_destroy( env_frame );
  AST_destroy( ast );
}

void run_fun_dynamic_eager( AST *ast ) { run_fun_common( ast, eval_fun_dynamic_eager ); }

void run_fun_static_eager( AST *ast ) { run_fun_common( ast, eval_fun_static_eager ); }

void run_fun_static_lazy( AST *ast ) { run_fun_common( ast, eval_fun_static_lazy ); }
