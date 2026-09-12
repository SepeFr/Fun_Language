#ifndef LEXXER_TOKENS_H
#define LEXXER_TOKENS_H

/* Values are contiguous so they can index lexer tables. */
typedef enum
{
  LEXER_TOKEN_INT,
  LEXER_TOKEN_SUM,
  LEXER_TOKEN_LET,
  LEXER_TOKEN_EQUALS,
  LEXER_TOKEN_IN,
  LEXER_TOKEN_LPAREN,
  LEXER_TOKEN_RPAREN,
  LEXER_TOKEN_FN,
  LEXER_TOKEN_ARROW,
  LEXER_TOKEN_WHITESPACE,
  LEXER_TOKEN_APPLY,
  LEXER_TOKEN_VARIABLE,
  LEXER_TOKEN_EOF,
  _NUMER_OF_LEXER_TOKENS, /* Number of lexer token kinds. */
} LexerToken;

extern const char *LexerTokenName[_NUMER_OF_LEXER_TOKENS];

#endif /* LEXXER_TOKENS_H */
