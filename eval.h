#ifndef EVAL_H
#define EVAL_H

typedef struct param {
   int count;
   char **type;
   int *level;
}param_t;
struct id_table {
  char name[128];
  int  scope; 
  int  type; 
  int  level;
  void *block;
  int  dimension; //is it a static array or not
  int  immutable; //is it a static array or not
  int  quantity; // array size if array, default 1
  int  size; // ID size in bytes
  param_t param;
  struct id_table *next;
};

int add_id(char *name, statement_t *stmt, int scope, int type, int size, int dimension, int quantity, int level, int immutable);
int find_id(char *name, statement_t *stmt, int scope);
int delete_id(char *name, statement_t *stmt, int scope);

enum {
  TYPE_RESERVED,
  TYPE_REGULAR=1,
  TYPE_POINTER=2,
  TYPE_FUNCTION=4,
  TYPE_FUNCTION_POINTER=6, // 2|4
  TYPE_ARRAY=8,
};
enum {
  ARRAY_RESERVED,
  ARRAY_REGULAR=1,
  ARRAY_2DIMENSION=2,
  ARRAY_3DIMENSION=3,
  ARRAY_4DIMENSION=4,
  ARRAY_5DIMENSION=5,
  ARRAY_6DIMENSION=6,
  ARRAY_7DIMENSION=7,
  ARRAY_8DIMENSION=8,
  ARRAY_9DIMENSION=9,
};
int evalLongDef(token_t *t, statement_t *stmt);
int evalCharFuncDef(statement_t *stmt);
int evalIntFuncDef(statement_t *stmt);
int evalIntVar(statement_t *stmt);
int evalCharDef(token_t *t, statement_t **outp);
int evalCharVar(statement_t *stmt);
int evalUnsignedDef(token_t *t, statement_t **outp);
int evalUnsignedVar(statement_t *stmt);
int evalUnsignedFuncDef(statement_t *stmt);
int evalExprDef(token_t *t, statement_t *stmt);
int evalBlockDef(token_t *t, statement_t *stmt);
int evalFuncDef(statement_t *stmt);
int evalFuncDecl(statement_t *stmt);
int evalFuncParamDecl(token_t *t, token_t **outp);
int evalTypedef(token_t *t, statement_t **outp);
int eval_expr(statement_t **stmt, int *result);
int eval_bracket_expr(token_t *t, statement_t **stmt, int *result);
int defineId(token_t *type, token_t *ident, token_t *parmt, int pointer_level);
int defineFuncPointer(token_t *type, token_t *ident, token_t *parmt, int pointer_level, int function_pointer_level);
int evalParam(token_t *type, token_t *ident, token_t *parmt, int pointer_level, token_t **end );
int add_param(token_t *idt, statement_t *stmt, int scope);
token_t *find_identifier(char *id, statement_t *stmt, int scope);

int isIdDefined(token_t *idt, statement_t *stmt);
int isFuncPointerDefined(token_t *idt, statement_t *stmt);
int isFuncDecl(statement_t *outp);
int isVarDecl(statement_t *stmt);
int isVarDef(statement_t *stmt);
int isStructCompatible(char *name, token_t *arg, token_t **end);
int isTypedefed(token_t *t);
int isDeclared(token_t *t);
#endif
