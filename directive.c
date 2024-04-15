#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stol-utils.h"
#include "stol-errors.h"
#include "macro.h"
#include "directive.h"

struct direct cdirect[] = {
  {"#if", ifAction},
  {"#ifdef", ifdefAction},
  {"#ifndef", ifndefAction},
  {"#include", includeAction},
  {"#define", defineAction},
  {"#undefine", undefineAction},
  {"#pragma", pragmaAction},
};
extern char outbuf[];
int ifAction(char *buf, int len, char *file, char **nextbuf, statement_t **outpp) 
{
  char *orig = buf, *origbuf = buf, *beginbuf, *endbuf;
  int i, j, result;
  int outlen = 0;
  int eval = 0;
  token_t t = {0}, *next, *outt;
  statement_t *newst = NULL, *stmt, *orst = *outpp;
  char *newline;
  char *nextline = NULL;
  int newlen = 0, found;
  

  stmt = orst;
  j = get_line(outbuf, buf, len);
  newlen = j;
  buf += j;
  len -= j;

  outt = NULL;
  i = parse_if_macro(outbuf, newlen, &outt);
  if (i > 0) {
    result = 0;
    i = eval_if_macro(outt, &result);
    if (i <= 0)
       return i;
    beginbuf = buf;
    if (result > 0) {
       //#if true
       while(len > 0) {
          i = get_token(outbuf, buf, len, &t);
          if (i > 0) {
             if (strcmp(outbuf, "#elseif") == 0) {
                i = parse(beginbuf, buf-beginbuf, file, outpp);
                j = get_line(outbuf, buf, len);
                if (j > 0) {
                   buf += j;
                   len -= j;
                }
                while(j > 0) {
                   j = get_token(outbuf, buf, len, &t);
                   if (j > 0) {
                     len -= j;
                     buf += j;
                     outlen += j;
                     if (strcmp(outbuf, "#endif") == 0) {
                        j = get_line(outbuf, buf, len);
                        if (j > 0) {
                           buf += j;
                           len -= j;
                           outlen += j;
                           *nextbuf  =buf;
                           return outlen;
                        } 
                     }
                   }
                }
             }
          }
       }
    } else {
       //#if false
       while(len > 0) {
          i = get_token(outbuf, buf, len, &t);
          if (i > 0) {
             if (strcmp(outbuf, "#elseif") == 0) {
                i = get_line(outbuf, buf, len); 
                if (i < 0) {
                   errorPrint(&t, NO_POUND_ENDIF_ERR);
                   return NO_POUND_ENDIF_ERR;
                }
                buf += i;  
                len -= i;
                beginbuf = buf;
                while(len > 0) {
                   i = get_line(outbuf, buf, len); 
                   if (i < 0) {
                      errorPrint(&t, NO_POUND_ENDIF_ERR);
                      return NO_POUND_ENDIF_ERR;
                   }
                   endbuf  = buf;
                   i = get_line (outbuf, buf, len);
                   len -= i;
                   outlen += i;
                   buf += i;
                   if (strcmp(outbuf, "#endif") == 0 || strcmp(outbuf, "#else") == 0) {
                      j = parse(beginbuf, buf-beginbuf, file, outpp);
                   }
                   return outlen;
                }
             }
          }
       }
    }
  } else {
    //skip to just below #else or #endif
    buf = nextline;
    int j = 0;
    len -= newlen;
    found = 0;
    while(j < len) {
      i = get_token(outbuf, buf, j, &t);
      if (i > 0) {
        if (t.id == POUND_ELSEIF || t.id == POUND_ENDIF) {
           found = 1;
           break;
        }
        j += i;
      } else
        break;
    }
    if (found == 1) {
      if (t.id == POUND_ELSEIF) {
         i = get_line(outbuf, buf, len);
         len -= i;
         buf += i;
         outlen += i;
         beginbuf = buf;
         while(len > 0) {
            i = get_token(outbuf, buf, len, &t);
            if (t.id == POUND_ENDIF) {
              i = parse(beginbuf, buf-beginbuf, file, outpp);
              outlen += i;
              return outlen;
            }
            buf += i;
            len -= i;
            outlen += i;
         }
         if (len <= 0) {
            errorPrint(&t, NO_POUND_ENDIF_ERR);
            return NO_POUND_ENDIF_ERR;
         }
      } 
      //Found endif
      buf += i;
      len -= i;
      while(len > 0) {
        if (buf[0] != '\n' && buf[0] != '\v' && buf[0] != '\r') {
           buf++;
           len--;
        } else {
           buf++;
           len--;
           break;
        }
      }
      *nextbuf = buf;
      return (buf - origbuf);
    }
    len  = j;
    return i;
  }
  return i;
}

int ifdefAction(char *buf, int len, char *file, char **nextbuf, statement_t **outpp) 
{
   return -1;
}

int ifndefAction(char *buf, int len, char *file, char **nextbuf, statement_t **outpp) 
{
   return -1;
}

int includeAction(char *buf, int len, char *file, char **nextbuf, statement_t **outpp) 
{
   return -1;
}

int undefineAction(char *buf, int len, char *file, char **nextbuf, statement_t **outpp) 
{
   return -1;
}

int pragmaAction(char *buf, int len, char *file, char **nextbuf, statement_t **outpp) 
{
   return -1;
}

char scratch[1024];
int defineAction(char *buf, int len, char *file, char **nextbuf, statement_t **outpp) 
{
   int i, j, newst = 0, outlen = 0;
   token_t *t, *orig = NULL, token;
   statement_t *stmt = *outpp;

   if (stmt == NULL) {
      stmt = (statement_t *) calloc(1, sizeof(statement_t));
      if (stmt == NULL) {
         printf("\nError: could not allocate memory for statement at %d in file %s", __LINE__, __FILE__);
         return NO_MEM_ERR;
      }
      stmt->filename = (char *) malloc(strlen(file)+1);
      if (stmt->filename == NULL) {
         printf("\nError: could not allocate memory for statement at %d in file %s", __LINE__, __FILE__);
         free(stmt);
         return NO_MEM_ERR;
      }
      strcpy(stmt->filename, file);
      *outpp = stmt;
      newst = 1;
   }
   i = get_line(outbuf, buf, len);
   if (i <= 0) {
      printf("\nError:End of file %s", file);
      return SYNTAX_ERR;
   }
   buf += i;
   len -= i;
   outlen += i; 
   j = 0;
   while(outlen > 0) {
      i = get_token(scratch, outbuf + j, outlen, &token);
      if (i <= 0) {
         printf("\nSyntax error: %s line %d", __FILE__, __LINE__);
         return SYNTAX_ERR;
      }
      outlen -= i;
      j += i;
      t  = stmt->token;
      if (newst) {
         t = (token_t *) calloc(1, sizeof(token_t));
         if (t == NULL) {
            free(stmt->filename);
            free(stmt);
            *outpp = NULL;
         }
         stmt->token = t;
         orig = t;
         stmt->token[0] = token;
         continue;
      }
      t = stmt->token;
      while(t->next)
         t = t->next;
      t->next = (token_t *)calloc(1, sizeof(token_t));
      if (t->next == NULL) {
         errorPrint(t, NO_MEM_ERR);
         return NO_MEM_ERR;
      }
      t = t->next;
      if (orig == NULL)
         orig = t;
      t[0] = token; 
   }
   t = orig;

   while(t) {
      if (t->id == INTEGER || t->id == FLOAT || t->id == CHAR || t->id == LONG || t->id == DOUBLE) {
         errorPrint(t, PREDEFINED_ERR);
         return outlen;
      }
      if (t->id == IDENTIFIER) {
         struct macro *m;
         if ((m = findMacro((char *) t->value))) {
           errorPrint(t, REDEFINED_ERR);
           char *ptr = realloc(m->value, strlen((char *)t->value)+1);
           if (ptr == NULL) {
              errorPrint(t, NO_MEM_ERR);
              return NO_MEM_ERR;
           }
           strcpy(m->value, t->value);
           continue;
         }
         addMacro(t->value, t->next->value, file, t->line, t->col);
         break; // one define, one macro
      }
   }
   return outlen;
}
