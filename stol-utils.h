#ifndef STOL_UTILS_H
#define STOL_UTILS_H

enum {
  RESERVED,
  CHAR,
  UCHAR,
  SHORT,
  USHORT,
  INTEGER,
  UINTEGER,
  LONG,
  ULONG,
  FLOAT,
  DOUBLE,
  INT64,
  UINT64,
  UNSIGNED,
  SIGNED,
  NUMBER, // 14
  EXPRESSION,
  IDENTIFIER, //16
  STRUCT,
  TYPEDEF,
  BLOCK_BEGIN,
  BLOCK_END,
  ENUM,
  UNION,
  STATEMENT,
  PLUS, // character +
  PLUSPLUS, // character ++
  MINUS,
  MINUSMINUS,
  STAR, // character *
  DIVISION, // character /
  COMMA,
  SEMICOLON, //31
  EQUAL_TO, // characters ==
  ASSIGNMENT, // character =, index 33
  STATIC,
  EXTERN,
  LEFT_SHIFT,
  RIGHT_SHIFT,
  LOGICAL_AND,
  LOGICAL_OR,
  MODULO,
  NOT,
  AMPERSAND,
  OR,
  XOR,
  TOGGLE,
  KEYWORD,
  RETURN,
  FUNCTION,
  BRACKET_OPEN,
  BRACKET_CLOSE,
  SQUARE_OPEN,
  SQUARE_CLOSE,
  POUND_IF,
  POUND_ELSE,
  POUND_ELSEIF,
  POUND_ENDIF,
};

enum {
  SC_LOCAL=1,
  SC_STATIC,
  SC_GLOBAL,
};

typedef unsigned int uint;
typedef struct token {
   uint   id;
   char   *filename;
   uint   line, col;
   void * value;
   struct token *next;
} token_t;
#if 1
typedef struct statement {
   uint   id;
   char *filename;
   void   *value;
   token_t *token;
   unsigned int  block[1];
   struct statement *next;
} statement_t;

struct name {
  char *myname;
  struct name *next;
};
typedef struct vardef {
   struct name *name[256];
} vardef_t;
struct compile {
   vardef_t *statdef;
   vardef_t  *macdef;
   vardef_t  *bldef;
   vardef_t  *gldef;
};
#endif
int get_token(char *outbuf, const char *inbuf, int len, token_t *token);
int get_line(char *outbuf, char *buf, int len);
int isValidNumber(char *outbuf);
int isValidInt(char *outbuf);
int isValidId(char *outbuf);
int parse(char *buf, int len, char *filename, statement_t **outpp);
int parse_typedef_int(char *buf, int len, int *outlen, token_t *out);
int parse_typedef_long(char *buf, int len, int *outlen, token_t *out);
int parse_typedef_struct(char *buf, int len, int *outlen, token_t *out);
int parse_typedef_struct_identifier(char *buf, int len, int *outlen, token_t *out);
int parse_typedef_struct_block(char *buf, int len, int *outlen, token_t *out);
int parse_enum_block(char *buf, int len, int *outlen, token_t *out);
int parse_struct_block(char *buf, int len, int *outlen, token_t *out);
int parse_struct_identifier(char *buf, int len, int *outlen, token_t *out);
int parse_enum_identifier(char *buf, int len, int *outlen, token_t *orig);
int parse_enum_identifier_equal(char *buf, int len, int *outlen, token_t *orig);
int parse_statement(char *buf, int len, int *outlen, token_t **outpp);
int parse_declaration(char *buf, int len, int *outlen, token_t **outpp);
int parse_long64(char *buf, int len, int *outlen, token_t *out);
int parse_long64_identifier(char *buf, int len, int *outlen, token_t *out);
int parse_ulong64(char *buf, int len, int *outlen, token_t *out);
int parse_ulong64_identifier(char *buf, int len, int *outlen, token_t *out);
int parse_ulong(char *buf, int len, int *outlen, token_t *out);
int parse_ulong_identifier(char *buf, int len, int *outlen, token_t *out);
int parse_long(char *buf, int len, int *outlen, token_t *out);
int parse_long_identifier(char *buf, int len, int *outlen, token_t *out);
int parse_uint(char *buf, int len, int *outlen, token_t *out);
int parse_uint_identifier(char *buf, int len, int *outlen, token_t *out);
int parse_int(char *buf, int len, int *outlen, token_t *out);
int parse_int_identifier(char *buf, int len, int *outlen, token_t *out);
int parse_int16(char *buf, int len, int *outlen, token_t *out);
int parse_int16_identifier(char *buf, int len, int *outlen, token_t *out);
int parse_uint16(char *buf, int len, int *outlen, token_t *out);
int parse_uint16_identifier(char *buf, int len, int *outlen, token_t *out);
int parse_int8(char *buf, int len, int *outlen, token_t *out);
int parse_int8_identifier(char *buf, int len, int *outlen, token_t *out);
int parse_uint8(char *buf, int len, int *outlen, token_t *out);
int parse_uint8_identifier(char *buf, int len, int *outlen, token_t *out);
int parse_float(char *buf, int len, int *outlen, token_t *out);
int parse_float_identifier(char *buf, int len, int *outlen, token_t *out);
int parse_double(char *buf, int len, int *outlen, token_t *out);
int parse_double_identifier(char *buf, int len, int *outlen, token_t *out);
int parse_static_int(char *buf, int len, int *outlen, token_t *out);
int parse_static_int_identifier(char *buf, int len, int *outlen, token_t *out);
int parse_static_uint(char *buf, int len, int *outlen, token_t *out);
int parse_static_unsigned(char *buf, int len, int *outlen, token_t *out);
int parse_static_uint_identifier(char *buf, int len, int *outlen, token_t *out);
int parse_static_long(char *buf, int len, int *outlen, token_t *out);
int parse_static_long_identifier(char *buf, int len, int *outlen, token_t *out);
int parse_static_ulong(char *buf, int len, int *outlen, token_t *out);
int parse_static_ulong_identifier(char *buf, int len, int *outlen, token_t *out);
int parse_static_int16(char *buf, int len, int *outlen, token_t *out);
int parse_static_int16_identifier(char *buf, int len, int *outlen, token_t *out);
int parse_static_uint16(char *buf, int len, int *outlen, token_t *out);
int parse_static_uint16_identifier(char *buf, int len, int *outlen, token_t *out);
int parse_static_int8(char *buf, int len, int *outlen, token_t *out);
int parse_static_int8_identifier(char *buf, int len, int *outlen, token_t *out);
int parse_static_uint8(char *buf, int len, int *outlen, token_t *out);
int parse_static_uint8_identifier(char *buf, int len, int *outlen, token_t *out);
int parse_static_double(char *buf, int len, int *outlen, token_t *out);
int parse_static_double_identifier(char *buf, int len, int *outlen, token_t *out);
int parse_static_float(char *buf, int len, int *outlen, token_t *out);
int parse_static_float_identifier(char *buf, int len, int *outlen, token_t *out);
void errorPrint(token_t *, int);
#endif
