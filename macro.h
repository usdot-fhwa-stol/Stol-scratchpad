#ifndef MACRO_H
#define MACRO_H

struct macro {
  char name[256];
  char *value;
  struct macro *next;
  char *filename;
  int line, col;
};

int addMacro(char *name, char *var, char *filename, int line, int col);
struct macro *findMacro(char *name);
void freeMacroEntry(struct macro *first);
int deleteMacro(char *name);
int parse_if_macro(char *buf, int len, token_t **outp);
int eval_if_macro(token_t *t, int *result);
int eval_bracket_macro(token_t *t, token_t **newt, int *result);
#endif
