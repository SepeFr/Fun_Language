#ifndef PARSER_H
#define PARSER_H

#include "dynamic_array.h"
#include "lexer_regex.h"
#include "lexer_tokens.h"
#include "parser_tokens.h"

/*
 * Recursive-descent parser interface for the Fun language.
 *
 * The parser consumes lexer tokens and produces an abstract syntax tree (AST).
 *
 * Grammar:
 *   expr    := sum
 *   sum     := application ('+' application)*
 *   application := atom atom*
 *   atom    := INT | VAR | '(' expr ')' | let-in | fn
 *
 * Application is left-associative: f x y is parsed as (f x) y.
 */

/*
 * Parser-level token representation stored inside the AST.
 * The token kind and literal string are retained for printing and diagnostics.
 */
typedef struct ParserMatchedToken
{
  ParserToken token;
  char *string;
} ParserMatchedToken;

/* Dynamic array specialization for parser tokens. */
DEFINE_DYNAMIC_ARRAY( ParserMatchedTokenArray, ParserMatchedToken );

/*
 * Cursor over a lexer token stream. The parser advances it with accept() and
 * expect().
 */
typedef struct MatchedTokensGenerator
{
  LexerMatchedTokenArray *tokens;
  size_t index;
} MatchedTokensGenerator;

/* Creates a cursor positioned at the first token. */
MatchedTokensGenerator *MatchedTokensGenerator_init( LexerMatchedTokenArray *array );

/* Consumes and returns the current token. */
LexerMatchedToken GeneratorNextToken( MatchedTokensGenerator *generator );

/* Returns the current token without consuming it. */
LexerMatchedToken GeneratorCurrentToken( MatchedTokensGenerator *generator );

/* Returns the token consumed by the most recent GeneratorNextToken() call. */
LexerMatchedToken *GeneratorPreviousToken( MatchedTokensGenerator *generator );

/*
 * Consumes the current token and returns 1 when it matches token. Otherwise,
 * returns 0 without advancing.
 */
int accept( LexerToken token, MatchedTokensGenerator *generator );

/*
 * Like accept(), but reports a parser error and exits when the token does not
 * match.
 */
int expect( LexerToken token, MatchedTokensGenerator *generator );


/*
 * Binary-tree representation of Fun expressions:
 *   - INT / VAR:     leaf nodes
 *   - SUM:           left + right
 *   - APPLY:         left right
 *   - FN:            left=param(VAR), right=body
 *   - IN:            left=LET node, right=body
 *   - LET node:      left=VAR, right=value expression
 */
typedef struct AST
{
  struct AST *left;
  struct AST *right;
  ParserMatchedToken *matched_token;
} AST;


/* Parses a complete expression from a lexer token array. */
AST *parse_fun( LexerMatchedTokenArray *tokens );

/* Parses a let expression after its LET token has been consumed. */
AST *letin( MatchedTokensGenerator *generator );

/* Parses a function expression after its FN token has been consumed. */
AST *fn_parser( MatchedTokensGenerator *generator );

/* Parse an atomic term: INT, VAR, parenthesized expr, let-in, or fn */
AST *term( MatchedTokensGenerator *generator );

/* Allocates an empty AST node. */
AST *AST_init();

/* Writes the AST tree with indentation. */
void AST_print( AST *ast, int depth );

/* Writes a compact, single-line AST representation. */
void AST_print_no_new_line( AST *ast, int depth );

/* Frees an AST and its associated parser tokens. */
void AST_destroy( AST *ast );

/* Reconstructs an expression with minimal parentheses. */
void AST_print_pretty( AST *ast );

/* Pretty-prints an expression using ANSI color sequences. */
void AST_print_pretty_color( AST *ast );

#endif // !PARSER_H
