#ifndef DIRECTIVE_H
#define DIRECTIVE_H

struct direct {
  char name[128];
  int (*action)(char *arg, int len, char *file, char **nextbuf, statement_t **outpp);
};
int ifAction(char *arg, int len, char *file, char **nextbuf, statement_t **outpp);
int ifdefAction(char *arg, int len, char *file, char **nextbuf, statement_t **outpp);
int ifndefAction(char *arg, int len, char *file, char **nextbuf, statement_t **outpp);
int includeAction(char *arg, int len, char *file, char **nextbuf, statement_t **outpp);
int defineAction(char *arg, int len, char *file, char **nextbuf, statement_t **outpp);
int undefineAction(char *arg, int len, char *file, char **nextbuf, statement_t **outpp);
int pragmaAction(char *arg, int len, char *file, char **nextbuf, statement_t **outpp);
#endif
