#include "lexer_regex.h"
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "lexer_tokens.h"


/*
 * Regex-based lexer implementation.
 */

/* Compiled regular expressions indexed by LexerToken. */
regex_t LexerRegexes[_NUMER_OF_LEXER_TOKENS];

const char IntRegexPattern[] = "^[0-9]+";
const char VariableRegexPattern[] = "^[a-zA-Z_][a-zA-Z0-9_]*";
const char SumRegexPattern[] = "^\\+";
const char LetRegexPattern[] = "^let\\b";
const char EqualsRegexPattern[] = "^=";
const char InRegexPattern[] = "^in\\b";
const char WhitespaceRegexPattern[] = "^[[:space:]]+";
const char LParenRegexPattern[] = "^\\(";
const char RParenRegexPattern[] = "^\\)";
const char FnRegexPattern[] = "^fn\\b";
const char ArrowRegexPattern[] = "^=>";
const char NotMatchAnything[] = "^\\b\\B";

const char *LexerTokenName[_NUMER_OF_LEXER_TOKENS] = {
  "TOKEN_INT", "TOKEN_SUM",   "TOKEN_LET",        "TOKEN_EQUALS", "TOKEN_IN",       "TOKEN_LPAREN", "TOKEN_RPAREN",
  "TOKEN_FN",  "TOKEN_ARROW", "TOKEN_WHITESPACE", "TOKEN_APPLY",  "TOKEN_VARIABLE", "TOKEN_EOF" };

void lexer_init()
{
  regcomp( &LexerRegexes[LEXER_TOKEN_INT], IntRegexPattern, REG_EXTENDED );
  regcomp( &LexerRegexes[LEXER_TOKEN_VARIABLE], VariableRegexPattern, REG_EXTENDED );
  regcomp( &LexerRegexes[LEXER_TOKEN_SUM], SumRegexPattern, REG_EXTENDED );
  regcomp( &LexerRegexes[LEXER_TOKEN_LET], LetRegexPattern, REG_EXTENDED );
  regcomp( &LexerRegexes[LEXER_TOKEN_EQUALS], EqualsRegexPattern, REG_EXTENDED );
  regcomp( &LexerRegexes[LEXER_TOKEN_IN], InRegexPattern, REG_EXTENDED );
  regcomp( &LexerRegexes[LEXER_TOKEN_LPAREN], LParenRegexPattern, REG_EXTENDED );
  regcomp( &LexerRegexes[LEXER_TOKEN_RPAREN], RParenRegexPattern, REG_EXTENDED );
  regcomp( &LexerRegexes[LEXER_TOKEN_FN], FnRegexPattern, REG_EXTENDED );
  regcomp( &LexerRegexes[LEXER_TOKEN_ARROW], ArrowRegexPattern, REG_EXTENDED );
  regcomp( &LexerRegexes[LEXER_TOKEN_WHITESPACE], WhitespaceRegexPattern, REG_EXTENDED );
  regcomp( &LexerRegexes[LEXER_TOKEN_APPLY], NotMatchAnything, REG_EXTENDED );
  regcomp( &LexerRegexes[LEXER_TOKEN_EOF], NotMatchAnything, REG_EXTENDED );
}

/*
 * Matching priority order.
 * Keywords must be considered before identifiers, so "let" becomes LET rather
 * than VARIABLE.
 */
static const int ORDER[] = { LEXER_TOKEN_WHITESPACE, LEXER_TOKEN_INT,   LEXER_TOKEN_LET,     LEXER_TOKEN_IN,
                             LEXER_TOKEN_FN,         LEXER_TOKEN_ARROW, LEXER_TOKEN_EQUALS,  LEXER_TOKEN_LPAREN,
                             LEXER_TOKEN_RPAREN,     LEXER_TOKEN_SUM,   LEXER_TOKEN_VARIABLE };

/* Tokenizes input from left to right and appends an EOF token. */
LexerMatchedTokenArray *lexer_lex( const char *input_text )
{
  LexerMatchedTokenArray *matched_tokens = LexerMatchedTokenArray_create();
  int start_offset = 0;

  if ( LEXER_DEBUG )
  {
    printf( "Parsing String %s\n", input_text );
  }

  while ( *input_text != '\0' )
  {
    regmatch_t match;
    int token = _NUMER_OF_LEXER_TOKENS;

    int matched = 0;

    for ( size_t k = 0; k < sizeof ORDER / sizeof ORDER[0]; ++k )
    {
      int i = ORDER[k];
      if ( regexec( &LexerRegexes[i], input_text, 1, &match, 0 ) == 0 )
      {
        token = i;
        matched = 1;
        break;
      }
    }


    /* No matching expression means the remaining input is invalid. */
    if ( !matched || match.rm_eo <= 0 )
    {
      fprintf( stderr, "Lexer error: cannot tokenize starting at: '%s'\n", input_text );
      exit( EXIT_FAILURE );
    }
    char *string = strndup( input_text, match.rm_eo );

    if ( LEXER_DEBUG )
    {
      start_offset += match.rm_so;
      printf( " > [LEXER_DEBUG] Token : `%s`, type : `%s`, start_offset : %d, "
              "end_offset : %d\n",
              string, LexerTokenName[token], start_offset, start_offset + match.rm_eo );
    }
    input_text = input_text + match.rm_eo;

    LexerMatchedToken matched_token = { .token = token, .string = string };

    LexerMatchedTokenArray_push( matched_tokens, matched_token );
  }

  LexerMatchedToken token_eof = { .token = LEXER_TOKEN_EOF, .string = "\0" };
  LexerMatchedTokenArray_push( matched_tokens, token_eof );
  return matched_tokens;
}


LexerMatchedTokenArray *lexer_filter_whitespace( const LexerMatchedTokenArray *arr )
{
  LexerMatchedTokenArray *new_array = LexerMatchedTokenArray_create();

  for ( int i = 0; i < arr->length; i++ )
  {
    if ( arr->data[i].token != LEXER_TOKEN_WHITESPACE )
    {
      LexerMatchedTokenArray_push( new_array, arr->data[i] );
    }
  }

  return new_array;
}

void lexer_matched_token_array_destroy( LexerMatchedTokenArray *arr )
{
  if ( !arr )
    return;

  /* Token strings are owned by this array. */
  for ( size_t i = 0; i < arr->length; i++ )
  {
    if ( arr->data[i].string && arr->data[i].token != LEXER_TOKEN_EOF )
    {
      free( arr->data[i].string );
    }
  }

  /* Release the backing array and its container. */
  free( arr->data );
  free( arr );
}

void lexer_cleanup()
{
  /* Release every compiled regular expression. */
  for ( int i = 0; i < _NUMER_OF_LEXER_TOKENS; i++ )
  {
    regfree( &LexerRegexes[i] );
  }
}

/*
 * Destroys only the array storage (not the strings).
 */

void lexer_matched_token_array_destroy_no_strings( LexerMatchedTokenArray *arr )
{
  if ( !arr )
    return;

  /* Strings are borrowed from the unfiltered token array. */
  free( arr->data );
  free( arr );
}
