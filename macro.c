#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stol-utils.h"
#include "stol-errors.h"
#include "macro.h"

struct macro *macroList[256] = {NULL};
extern char outbuf[];
int addMacro(char *name, char *var, char *filename, int line, int col)
{
   unsigned int index = (unsigned int)(unsigned char)name[0];
   struct macro *prev, *first = macroList[index];

   if (first == NULL) {
      first  = (struct macro *) malloc(sizeof(struct macro));
      if (first == NULL)
         return NO_MEM_ERR;
      strncpy(first->name, name, 128);
      first->value = (char *) malloc(strlen(var)+1);
      first->next = NULL;
      if (first->value == NULL) {
         free(first);
         return NO_MEM_ERR;
      }
      strcpy(first->value, var);
      macroList[index] = first;
      return 1;
   }
   prev = first;
   while(first) {
     if (strcmp(first->name, name) == 0) {
        char *file = first->filename;
        char *val = first->value;
        printf("\nWarning: Redefinition of the macro %s in file %s.", name, filename); 
        printf("\nPrevious definition in file %s line %d col %d", first->filename, first->line, first->col);
        file = (char *) realloc(file, strlen(filename)+1);
        val = (char *) realloc(val, strlen(var) + 1);
        if (val == NULL || file == NULL) {
             if (val)
               free(val);
             if(file)
               free(file);
             return NO_MEM_ERR;
        }
        first->filename =file;
        first->value = val;
        strcpy(first->filename, filename);
        strcpy(first->value, var); 
        return 1;
     } 
     prev = first;
     first = first->next;
   }
   first = (struct macro *) calloc(1, sizeof(struct macro));
   if (first == NULL)
     return NO_MEM_ERR;
   first->filename = (char *) malloc(strlen(filename) + 1);
   first->value = (char *) malloc(strlen(var) + 1);
   if (first->filename == NULL || first->value == NULL) {
     if (first->value)
        free(first->value);
     if (first->filename)
        free(first->filename);
     return NO_MEM_ERR;   
   }
   strncpy(first->name, name, 256);
   strcpy(first->filename, filename);
   strcpy(first->value, var);
   first->line = line;
   first->col = col;
   prev-> next = first;
   return 1;
}
struct macro *findMacro(char *name)
{
   unsigned int index = (unsigned int) (unsigned char) name[0];
   struct macro *first = macroList[index];

   while(first) {
     if (strncmp(first->name, name, 256) == 0) {
        return first;
     }
     first = first->next;
   }
   //Could not find the macro
   return NULL;
}

int parse_if_macro(char *buf, int len, token_t **outpp)
{
   token_t token, *t, *orig, *next;
   int i, outlen = 0;

   memset(&token, 0, sizeof(token));

   t = orig = NULL;
   while(len > 0) {
      i = get_token(outbuf, buf, len, &token);
      if (i <= 0) {
         printf("\nError while parsing #if ");
         return SYNTAX_ERR;
      }
      buf += i;
      len -= i;
      outlen += i;
      next = (token_t *) calloc(1, sizeof(token_t));
      if (next == NULL) {
         errorPrint(&token, NO_MEM_ERR);
         return NO_MEM_ERR;
      }
      next[0] = token;
      if (t == NULL) {
         orig  = t = next;
         continue;
      }
      t->next = next;
   }
   *outpp = orig;
   return outlen; 
}
int eval_bracket_macro(token_t *t, token_t **newt, int *result)
{
   token_t *orig, *next;
   int i, j, outlen = 0, val = 1;

   orig = t;
   if (t->id == BRACKET_CLOSE) {
      errorPrint(t, NO_STATEMENT_ERR);
      *newt = t;
      return NO_STATEMENT_ERR;
   }
   while(t) {
      if (t->id == BRACKET_CLOSE) {
         *newt = t;
         return val;
      }
      if (t->id == INTEGER) {
          val = atoi((char *)t->value);
          if (t->next == NULL) {
             *result = val;
             return val;
          }
          
          t = t->next;
          while(t) {
             if (t->id == BRACKET_CLOSE) {
                *newt = t;
                *result = val;
                return val;
             }
             if (t->id == LOGICAL_OR) {
                if (t->next == NULL) {
                   errorPrint(t, SYNTAX_ERR);
                   *newt = t;
                   return SYNTAX_ERR;
                }
                t = t->next;
                i = eval_if_macro(t, result);
                if (i < 0) {
                   *newt = t;
                   return i;
                }
                val |= *result;
                t = t->next;
                continue;
             }
             if (t->id == LOGICAL_AND) {
                if (t->next == NULL) {
                   errorPrint(t, SYNTAX_ERR);
                   *newt = t;
                   return SYNTAX_ERR;
                }
                t = t->next;
                i = eval_if_macro(t, result);
                if (i > 0) {
                   val &= *result;
                }
                t = t->next;
                continue;
             }
         }
         continue;
      }
      if (t->id == BRACKET_OPEN) {
         t = t->next;
         j = eval_bracket_macro(t, &next, result);
         if (j > 0) {
            if (next == NULL || next->id != BRACKET_CLOSE) {
               errorPrint(t, NO_BRCLOSE_ERR);
            }
            *newt = next;
            return result[0];
         }
      }
   }
   *newt = t;
   return val; 
}
int eval_if_macro(token_t *t, int *result)
{
   token_t *orig, *next;
   int i, j, outlen = 0, val = 1;

   orig = t;
   while(t) {
      if (t->id == INTEGER) {
          val = atoi((char *)t->value);
          if (t->next == NULL) {
             *result = val;
             return val;
          }
          
          t = t->next;
          while(t) {
             if (t->id == LOGICAL_OR) {
                if (t->next == NULL) {
                   errorPrint(t, SYNTAX_ERR);
                   return SYNTAX_ERR;
                }
                t = t->next;
                i = eval_if_macro(t, result);
                if (i < 0) {
                   return i;
                }
                val |= *result;
                t = t->next;
                continue;
             }
             if (t->id == LOGICAL_AND) {
                if (t->next == NULL) {
                   errorPrint(t, SYNTAX_ERR);
                   return SYNTAX_ERR;
                }
                t = t->next;
                i = eval_if_macro(t, result);
                if (i > 0) {
                   val &= *result;
                }
                t = t->next;
                continue;
             }
         }
         continue;
      }
      if (t->id == BRACKET_OPEN) {
         token_t *newt = NULL;
         t = t->next;
         j = eval_bracket_macro(t, &newt, result);
         if (j > 0) {
            if (newt == NULL || newt->id != BRACKET_CLOSE) {
               errorPrint(t, NO_BRCLOSE_ERR);
            }
            return *result;
         }
      }
   }
   return val; 
}

void freeMacroEntry(struct macro *first)
{
    if (first->value)
        free(first->value);
    if (first->filename)
        free(first->filename);
    free(first);
}

int deleteMacro(char *name)
{
   unsigned int index = (unsigned int) (unsigned char) name[0];
   struct macro *prev, *first = macroList[index];

   prev = first;
   while(first) {
     if (strncmp(first->name, name, 256) == 0) {
        if (prev == first) { // very first entry
           macroList[index] = first->next;
           first->next = NULL;
           freeMacroEntry(first);
           return 1;
        }
        prev->next = first->next;
        first->next = NULL;
        freeMacroEntry(first);
        return 1;
     }
     prev = first;
     first = first->next;
   }
   //Could not find the macro
   return 0;
}
