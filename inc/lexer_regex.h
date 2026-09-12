#ifndef LEXXER_REGEXS_H
#define LEXXER_REGEXS_H

#include <regex.h>
#include "dynamic_array.h"
#include "lexer_tokens.h"

/*
 * Regex-based lexer for the Fun language.
 *
 * The lexer scans the input string from left to right, matching the longest
 * prefix that fits one of the supported token regexes.
 *
 * The result is a dynamic array of (token kind, matched string) pairs.
 * Whitespace is represented explicitly.
 */


typedef struct LexerMatchedToken
{
  LexerToken token;
  char *string;
} LexerMatchedToken;

/* Compiled regular-expression table. */
extern regex_t LexerRegexes[_NUMER_OF_LEXER_TOKENS];

/* Patterns used to initialize the lexer table. */
extern const char IntRegexPattern[];
extern const char VariableRegexPattern[];
extern const char SumRegexPattern[];
extern const char LetRegexPattern[];
extern const char EqualsRegexPattern[];
extern const char InRegexPattern[];
extern const char WhitespaceRegexPattern[];
extern const char NotMatchAnything[];

DEFINE_DYNAMIC_ARRAY( LexerMatchedTokenArray, LexerMatchedToken );

/* Compiles the regular expressions used by the lexer. */
void lexer_init();

/* Tokenizes input into an array that includes whitespace and an EOF token. */
LexerMatchedTokenArray *lexer_lex( const char *string );

/*
 * Produces a new token array that skips whitespace tokens.
 *
 * The returned array borrows its strings from arr. Destroy it with
 * lexer_matched_token_array_destroy_no_strings().
 */
LexerMatchedTokenArray *lexer_filter_whitespace( const LexerMatchedTokenArray *arr );

/* Frees an owning token array and its matched strings. */
void lexer_matched_token_array_destroy( LexerMatchedTokenArray *arr );

/* Releases compiled regular-expression resources. */
void lexer_cleanup( void );

/*
 * Frees only token-array storage, not token strings. Use this for arrays
 * returned by lexer_filter_whitespace().
 */
void lexer_matched_token_array_destroy_no_strings( LexerMatchedTokenArray *arr );

#endif // !LEXXER_REGEXS_H
