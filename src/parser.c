#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "lexer_regex.h"
#include "lexer_tokens.h"

const char *ParserTokenName[_NUMER_OF_PARSER_TOKENS] = {
  "PARSER_TOKEN_INT",    "PARSER_TOKEN_SUM",    "PARSER_TOKEN_LET", "PARSER_TOKEN_EQUALS", "PARSER_TOKEN_IN",
  "PARSER_TOKEN_LPAREN", "PARSER_TOKEN_RPAREN", "PARSER_TOKEN_FN",  "PARSER_TOKEN_APPLY",  "PARSER_TOKEN_VARIABLE",
};

/* Maps lexer token kinds to the token kinds stored in AST nodes. */
static ParserToken map_lexer_to_parser( LexerToken t )
{
  switch ( t )
  {
    case LEXER_TOKEN_INT:
      return PARSER_TOKEN_INT;
    case LEXER_TOKEN_SUM:
      return PARSER_TOKEN_SUM;
    case LEXER_TOKEN_LET:
      return PARSER_TOKEN_LET;
    case LEXER_TOKEN_EQUALS:
      return PARSER_TOKEN_EQUALS;
    case LEXER_TOKEN_IN:
      return PARSER_TOKEN_IN;
    case LEXER_TOKEN_LPAREN:
      return PARSER_TOKEN_LPAREN;
    case LEXER_TOKEN_RPAREN:
      return PARSER_TOKEN_RPAREN;
    case LEXER_TOKEN_FN:
      return PARSER_TOKEN_FN;
    case LEXER_TOKEN_VARIABLE:
      return PARSER_TOKEN_VARIABLE;
    default:
      fprintf( stderr, "[PARSER ERROR] cannot map lexer token %d to parser token\n", t );
      exit( EXIT_FAILURE );
  }
}

/* Allocates a parser token and copies its matched lexeme. */
static ParserMatchedToken *parser_token_from_lexer_token( const LexerMatchedToken *tok )
{
  ParserMatchedToken *ptok = malloc( sizeof( ParserMatchedToken ) );
  if ( !ptok )
  {
    perror( "Error allocating ParserMatchedToken" );
    exit( EXIT_FAILURE );
  }
  ptok->token = map_lexer_to_parser( tok->token );
  ptok->string = strdup( tok->string );
  if ( !ptok->string )
  {
    perror( "Error duplicating token string" );
    exit( EXIT_FAILURE );
  }
  return ptok;
}

/* Creates a synthetic parser token for nodes such as APPLY. */
static ParserMatchedToken *parser_token_new( ParserToken token, const char *literal )
{
  ParserMatchedToken *ptok = malloc( sizeof( ParserMatchedToken ) );
  if ( !ptok )
  {
    perror( "Error allocating ParserMatchedToken" );
    exit( EXIT_FAILURE );
  }
  ptok->token = token;
  if ( literal )
  {
    ptok->string = strdup( literal );
    if ( !ptok->string )
    {
      perror( "Error duplicating token string" );
      exit( EXIT_FAILURE );
    }
  }
  else
  {
    ptok->string = NULL;
  }
  return ptok;
}


/* Initializes a token-stream cursor. */
MatchedTokensGenerator *MatchedTokensGenerator_init( LexerMatchedTokenArray *array )
{
  MatchedTokensGenerator *generator = malloc( sizeof( MatchedTokensGenerator ) );
  if ( generator == NULL )
  {
    perror( "Error allocating memory for MatchedTokensGenerator" );
    exit( EXIT_FAILURE );
  }
  generator->tokens = array;
  generator->index = 0;

  if ( PARSER_DEBUG )
  {
    printf( " > [PARSER_DEBUG] init : %zu tokens loaded.\n", array->length );
  }

  return generator;
}


/* Consumes and returns one token; exits if called past the end of input. */
LexerMatchedToken GeneratorNextToken( MatchedTokensGenerator *generator )
{
  if ( generator->index < generator->tokens->length )
  {
    if ( PARSER_DEBUG )
    {
      printf( " > [PARSER_DEBUG] next : %s\n", LexerTokenName[generator->tokens->data[generator->index].token] );
    }
    return generator->tokens->data[generator->index++];
  }

  fprintf( stderr, "[PARSER ERROR] unexpected end of token stream\n" );
  exit( EXIT_FAILURE );
}


LexerMatchedToken GeneratorCurrentToken( MatchedTokensGenerator *generator )
{
  return generator->tokens->data[generator->index];
}

LexerMatchedToken *GeneratorPreviousToken( MatchedTokensGenerator *generator )
{
  return &generator->tokens->data[generator->index - 1];
}

/* If the current token matches, consume it and return 1; otherwise leave stream unchanged. */
int accept( LexerToken desired_token, MatchedTokensGenerator *generator )
{
  if ( desired_token == GeneratorCurrentToken( generator ).token )
  {
    if ( PARSER_DEBUG )
    {
      printf( " > [PARSER_DEBUG] accepting : %s\n", LexerTokenName[desired_token] );
    }
    GeneratorNextToken( generator );
    return 1;
  }
  return 0;
}

/* If the current token matches, consume it and return 1; otherwise give an error. */
int expect( LexerToken token, MatchedTokensGenerator *generator )
{
  if ( accept( token, generator ) )
  {
    if ( PARSER_DEBUG )
    {
      printf( " > [PARSER_DEBUG] Expected correctly found: %s\n", LexerTokenName[token] );
    }
    return 1;
  }
  printf( "[PARSER ERROR] Expected %s, found %s\n", LexerTokenName[token],
          LexerTokenName[GeneratorCurrentToken( generator ).token] );
  exit( EXIT_FAILURE );
}

/* AST helpers. */

/* Allocates an empty AST node (children initialized to NULL). */
AST *AST_init()
{
  AST *ast = malloc( sizeof( AST ) );
  if ( ast == NULL )
  {
    perror( "AST malloc error" );
    exit( EXIT_FAILURE );
  }
  ast->left = NULL;
  ast->right = NULL;
  ast->matched_token = NULL;
  return ast;
}


/* Creates an AST node with a token and up to two children. */
AST *AST_make_node( ParserMatchedToken *token, AST *left, AST *right )
{
  AST *ast = AST_init();
  ast->matched_token = token;
  ast->left = left;
  ast->right = right;
  return ast;
}

/* Recursive-descent parser. */

AST *_parse_fun( MatchedTokensGenerator *generator );

/* Parses an atom; see parser.h for the grammar. */
AST *term( MatchedTokensGenerator *generator )
{
  if ( PARSER_DEBUG )
  {
    printf( " > [PARSER_DEBUG] -> term() : at %zu (%s)\n", generator->index,
            LexerTokenName[GeneratorCurrentToken( generator ).token] );
  }

  AST *node = NULL;

  if ( accept( LEXER_TOKEN_INT, generator ) )
  {
    LexerMatchedToken *current_node = GeneratorPreviousToken( generator );
    ParserMatchedToken *ptok = parser_token_from_lexer_token( current_node );
    node = AST_make_node( ptok, NULL, NULL );

    if ( PARSER_DEBUG )
    {
      printf( " > [PARSER_DEBUG] term(): found INT\n" );
    }
  }
  else if ( accept( LEXER_TOKEN_VARIABLE, generator ) )
  {
    LexerMatchedToken *current_node = GeneratorPreviousToken( generator );
    ParserMatchedToken *ptok = parser_token_from_lexer_token( current_node );
    node = AST_make_node( ptok, NULL, NULL );

    if ( PARSER_DEBUG )
    {
      printf( " > [PARSER_DEBUG] term(): found VARIABLE\n" );
    }
  }
  else if ( accept( LEXER_TOKEN_LET, generator ) )
  {
    if ( PARSER_DEBUG )
    {
      printf( " > [PARSER_DEBUG] term(): found letin()\n" );
    }
    node = letin( generator );
  }
  else if ( accept( LEXER_TOKEN_LPAREN, generator ) )
  {
    node = _parse_fun( generator );
    expect( LEXER_TOKEN_RPAREN, generator );
  }
  else if ( accept( LEXER_TOKEN_FN, generator ) )
  {
    node = fn_parser( generator );
  }

  return node;
}

AST *fn_parser( MatchedTokensGenerator *generator )
{
  LexerMatchedToken *fn_token = &generator->tokens->data[generator->index - 1];

  expect( LEXER_TOKEN_VARIABLE, generator );
  LexerMatchedToken *variable_token = &generator->tokens->data[generator->index - 1];

  expect( LEXER_TOKEN_ARROW, generator );

  AST *asign = _parse_fun( generator );

  ParserMatchedToken *fn_ptok = parser_token_from_lexer_token( fn_token );
  ParserMatchedToken *var_ptok = parser_token_from_lexer_token( variable_token );

  AST *var_node = AST_make_node( var_ptok, NULL, NULL );
  AST *fun_node = AST_make_node( fn_ptok, var_node, asign );
  return fun_node;
}

AST *letin( MatchedTokensGenerator *generator )
{
  if ( PARSER_DEBUG )
  {
    printf( " > [PARSER_DEBUG] -> letin() : at %zu (%s)\n", generator->index,
            LexerTokenName[GeneratorCurrentToken( generator ).token] );
  }

  LexerMatchedToken *let_token = &generator->tokens->data[generator->index - 1];

  expect( LEXER_TOKEN_VARIABLE, generator );
  LexerMatchedToken *var_token = &generator->tokens->data[generator->index - 1];

  expect( LEXER_TOKEN_EQUALS, generator );
  AST *value_expr = _parse_fun( generator );

  expect( LEXER_TOKEN_IN, generator );
  LexerMatchedToken *in_token = &generator->tokens->data[generator->index - 1];

  AST *body = _parse_fun( generator );

  ParserMatchedToken *let_ptok = parser_token_from_lexer_token( let_token );
  ParserMatchedToken *var_ptok = parser_token_from_lexer_token( var_token );
  ParserMatchedToken *in_ptok = parser_token_from_lexer_token( in_token );

  AST *var_node = AST_make_node( var_ptok, NULL, NULL );
  AST *let_node = AST_make_node( let_ptok, var_node, value_expr );
  AST *in_node = AST_make_node( in_ptok, let_node, body );

  if ( PARSER_DEBUG )
  {
    printf( " > [PARSER_DEBUG] -> letin() : completed \n" );
  }

  return in_node;
}

/* Detects whether the next token starts an application argument. */
int token_can_start_atom( LexerToken t )
{
  return t == LEXER_TOKEN_INT || t == LEXER_TOKEN_VARIABLE || t == LEXER_TOKEN_LPAREN || t == LEXER_TOKEN_LET ||
    t == LEXER_TOKEN_FN;
}

/* Parses left-associative function application. */
AST *application( MatchedTokensGenerator *generator )
{
  AST *node = term( generator );

  while ( token_can_start_atom( GeneratorCurrentToken( generator ).token ) )
  {
    ParserMatchedToken *apply_token = parser_token_new( PARSER_TOKEN_APPLY, "APPLY" );

    AST *rhs = term( generator );
    node = AST_make_node( apply_token, node, rhs );
  }
  return node;
}

/* Parses left-associative addition expressions. */
AST *parse_sum( MatchedTokensGenerator *generator )
{
  AST *node = application( generator );

  while ( accept( LEXER_TOKEN_SUM, generator ) )
  {
    LexerMatchedToken *operator= & generator->tokens->data[generator->index - 1];

    ParserMatchedToken *op_ptok = parser_token_from_lexer_token( operator);

    AST *rhs = application( generator );
    node = AST_make_node( op_ptok, node, rhs );
  }

  return node;
}

AST *_parse_fun( MatchedTokensGenerator *generator )
{
  AST *ast = parse_sum( generator );
  return ast;
}

AST *parse_fun( LexerMatchedTokenArray *tokens )
{
  if ( PARSER_DEBUG )
  {
    printf( " > [PARSER_DEBUG] === Begin Parsing ===\n" );
  }

  MatchedTokensGenerator *generator = MatchedTokensGenerator_init( tokens );
  AST *ast = _parse_fun( generator );

  free( generator );

  if ( PARSER_DEBUG )
  {
    printf( " > [PARSER_DEBUG] === Parsing Finished ===\n" );
  }

  return ast;
}
/* AST printing and pretty printing. */

void AST_print( AST *ast, int depth )
{
  if ( ast == NULL )
    return;

  for ( int i = 0; i < depth; i++ )
  {
    printf( " " );
  }

  printf( "(%s :", ParserTokenName[ast->matched_token->token] );
  if ( ast->matched_token->string )
    printf( "%s", ast->matched_token->string );
  printf( ")\n" );

  AST_print( ast->left, depth + 1 );
  AST_print( ast->right, depth + 1 );
}

static void parser_matched_token_destroy( ParserMatchedToken *token )
{
  if ( !token )
    return;
  if ( token->string )
  {
    free( token->string );
  }
  free( token );
}

/* Recursively frees the AST and the parser tokens attached to each node. */
void AST_destroy( AST *ast )
{
  if ( !ast )
    return;

  AST_destroy( ast->left );
  AST_destroy( ast->right );

  parser_matched_token_destroy( ast->matched_token );

  free( ast );
}

void AST_print_no_new_line( AST *ast, int depth )
{
  (void) depth;

  if ( ast == NULL )
    return;

  printf( "(%s :", ParserTokenName[ast->matched_token->token] );
  if ( ast->matched_token->string )
    printf( "%s", ast->matched_token->string );
  printf( ")" );

  if ( ast->left && ast->right )
  {
    printf( "[" );
    AST_print_no_new_line( ast->left, depth + 1 );
    printf( "," );
    AST_print_no_new_line( ast->right, depth + 1 );
    printf( "]" );
  }
  else
  {
    AST_print_no_new_line( ast->left, depth + 1 );
    AST_print_no_new_line( ast->right, depth + 1 );
  }
}


/*
 * Pretty-printing support.
 */
typedef enum
{
  PREC_LOWEST = 0, /* let and function expressions */
  PREC_SUM = 1,    /* addition */
  PREC_APP = 2,    /* function application */
  PREC_ATOM = 3,   /* integers and variables */
} Precedence;

static Precedence ast_prec( const AST *ast )
{
  if ( !ast || !ast->matched_token )
    return PREC_LOWEST;

  switch ( ast->matched_token->token )
  {
    case PARSER_TOKEN_IN:
      return PREC_LOWEST;
    case PARSER_TOKEN_FN:
      return PREC_LOWEST;
    case PARSER_TOKEN_SUM:
      return PREC_SUM;
    case PARSER_TOKEN_APPLY:
      return PREC_APP;
    case PARSER_TOKEN_INT:
      return PREC_ATOM;
    case PARSER_TOKEN_VARIABLE:
      return PREC_ATOM;
    default:
      return PREC_LOWEST;
  }
}

typedef struct
{
  const char *reset;
  const char *kw;
  const char *id;
  const char *num;
  const char *op;
  const char *delim;
} PPStyle;

static const PPStyle PP_NO_COLOR = {
  .reset = "",
  .kw = "",
  .id = "",
  .num = "",
  .op = "",
  .delim = "",
};

static const PPStyle PP_COLOR = {
  .reset = "\033[0m",

  .kw = "\033[38;5;153m",
  .id = "\033[38;5;150m",
  .num = "\033[38;5;223m",
  .op = "\033[38;5;174m",
  .delim = "\033[38;5;146m",
};

static void pp_indent( FILE *out, int indent )
{
  for ( int i = 0; i < indent; i++ )
    fputc( ' ', out );
}

/*
  Pretty print rules:
  - let-in prints as:
      let x = <expr> in
        <body>
  - fn prints as:
      fn x => <body>
    (body is indented if it is "big")
  - application prints as:
      <lhs> <rhs>
    with parentheses around "big" rhs/lhs when needed.
*/
static void ast_pp( FILE *out, const AST *ast, int indent, Precedence parent_prec, const PPStyle *st );

static int is_big_expr( const AST *ast )
{
  if ( !ast || !ast->matched_token )
    return 0;
  ParserToken t = ast->matched_token->token;
  return ( t == PARSER_TOKEN_IN || t == PARSER_TOKEN_FN );
}

static void pp_parens_if_needed( FILE *out, const AST *child, int indent, Precedence parent_prec, const PPStyle *st,
                                 int force_parens )
{
  if ( !child )
    return;

  Precedence child_prec = ast_prec( child );


  /* Parentheses are required when a child has lower precedence. */
  int need = force_parens || ( child_prec < parent_prec );

  if ( need )
    fprintf( out, "%s(%s", st->delim, st->reset );
  ast_pp( out, child, indent, need ? PREC_LOWEST : parent_prec, st );
  if ( need )
    fprintf( out, "%s)%s", st->delim, st->reset );
}

static void ast_pp( FILE *out, const AST *ast, int indent, Precedence parent_prec, const PPStyle *st )
{
  if ( !ast || !ast->matched_token )
    return;

  ParserToken t = ast->matched_token->token;
  Precedence my_prec = ast_prec( ast );

  switch ( t )
  {
    case PARSER_TOKEN_INT:
      fprintf( out, "%s%s%s", st->num, ast->matched_token->string, st->reset );
      return;

    case PARSER_TOKEN_VARIABLE:
      fprintf( out, "%s%s%s", st->id, ast->matched_token->string, st->reset );
      return;

    case PARSER_TOKEN_SUM:
    {
      /* Addition is left-associative. */
      pp_parens_if_needed( out, ast->left, indent, my_prec, st, 0 );
      fprintf( out, " %s+%s ", st->op, st->reset );
      pp_parens_if_needed( out, ast->right, indent, my_prec, st, 0 );
      return;
    }

    case PARSER_TOKEN_APPLY:
    {
      /* Parenthesize big expressions and applications used as operands. */
      int lhs_force = is_big_expr( ast->left );
      int rhs_force = is_big_expr( ast->right ) ||
        ( ast->right && ast->right->matched_token && ast->right->matched_token->token == PARSER_TOKEN_APPLY );

      pp_parens_if_needed( out, ast->left, indent, my_prec, st, lhs_force );
      fputc( ' ', out );
      pp_parens_if_needed( out, ast->right, indent, PREC_APP, st, rhs_force );
      return;
    }

    case PARSER_TOKEN_FN:
    {
      /* FN nodes store the parameter on the left and the body on the right. */
      fprintf( out, "%sfn%s ", st->kw, st->reset );

      if ( ast->left && ast->left->matched_token )
      {
        fprintf( out, "%s%s%s ", st->id, ast->left->matched_token->string, st->reset );
      }
      else
      {
        fprintf( out, "%s<missing_param>%s ", st->id, st->reset );
      }

      fprintf( out, "%s=>%s ", st->op, st->reset );

      /* Indent compound function bodies. */
      if ( is_big_expr( ast->right ) )
      {
        fprintf( out, "\n" );
        pp_indent( out, indent + 1 );
        ast_pp( out, ast->right, indent + 1, PREC_LOWEST, st );
      }
      else
      {
        ast_pp( out, ast->right, indent, PREC_LOWEST, st );
      }
      return;
    }

    case PARSER_TOKEN_IN:
    {
      /* IN nodes hold a LET node on the left and the body on the right. */
      const AST *let_node = ast->left;
      const AST *body = ast->right;

      pp_indent( out, indent );
      fprintf( out, "%slet%s ", st->kw, st->reset );

      const AST *var_node = let_node ? let_node->left : NULL;
      const AST *value_expr = let_node ? let_node->right : NULL;

      if ( var_node && var_node->matched_token )
      {
        fprintf( out, "%s%s%s ", st->id, var_node->matched_token->string, st->reset );
      }
      else
      {
        fprintf( out, "%s<missing_var>%s ", st->id, st->reset );
      }

      fprintf( out, "%s=%s ", st->op, st->reset );

      /* Print the binding value on its own indented line. */
      fprintf( out, "\n" );
      pp_indent( out, indent + 1 );
      ast_pp( out, value_expr, indent + 1, PREC_LOWEST, st );
      fprintf( out, "\n" );

      /* Keep chained let expressions aligned. */
      if ( body && body->matched_token && body->matched_token->token == PARSER_TOKEN_IN )
      {
        pp_indent( out, indent );
        fprintf( out, "%sin%s ", st->kw, st->reset );
        ast_pp( out, body, indent, PREC_LOWEST, st );
      }
      else
      {
        pp_indent( out, indent );
        fprintf( out, "%sin%s\n", st->kw, st->reset );

        pp_indent( out, indent + 1 );
        ast_pp( out, body, indent + 1, PREC_LOWEST, st );
      }


      return;
    }

    default:
      /* Fallback for unsupported AST token kinds. */
      fprintf( out, "%s<unknown:%s>%s", st->delim, ParserTokenName[t], st->reset );
      return;
  }
}

void AST_print_pretty( AST *ast )
{
  ast_pp( stdout, ast, 0, PREC_LOWEST, &PP_NO_COLOR );
  printf( "\n" );
}

void AST_print_pretty_color( AST *ast )
{
  ast_pp( stdout, ast, 0, PREC_LOWEST, &PP_COLOR );
  printf( "\n" );
}
