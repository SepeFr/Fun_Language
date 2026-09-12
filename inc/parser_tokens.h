#ifndef PARSER_TOKENS_H
#define PARSER_TOKENS_H

/*
 * Lexer tokens represent raw symbols, keywords, and identifiers. Parser tokens
 * represent the constructs stored in the syntax tree.
 */

typedef enum
{
  PARSER_TOKEN_INT,
  PARSER_TOKEN_SUM,
  PARSER_TOKEN_LET,
  PARSER_TOKEN_EQUALS,
  PARSER_TOKEN_IN,
  PARSER_TOKEN_LPAREN,
  PARSER_TOKEN_RPAREN,
  PARSER_TOKEN_FN,
  PARSER_TOKEN_APPLY,
  PARSER_TOKEN_VARIABLE,
  _NUMER_OF_PARSER_TOKENS, /* Number of parser token kinds. */
} ParserToken;

#endif                     // !PARSER_TOKENS_H
