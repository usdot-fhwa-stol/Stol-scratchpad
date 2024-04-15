#include <stdio.h>
#include "stol-errors.h"
#include "stol-utils.h"


void errorPrint(token_t *t, int cause)
{
   switch(cause)
   {
   case NO_IDENTIFIER_ERR:
      printf("\nError: Expected identifier at %d:%d", t->line, t->col);
      break;
 
   case NO_BROPEN_ERR:
      printf("\nError: Expected '(' around %d:%d", t->line, t->col);
      break;
 
   case NO_BRCLOSE_ERR:
      printf("\nError: Expected ')' around %d:%d", t->line, t->col);
      break;
 
   case BROPEN_ERR:
      printf("\nError: Unexpected '(' around %d:%d", t->line, t->col);
      break;
 
   case NO_BLOPEN_ERR:
      printf("\nError: Expected '{' around %d:%d", t->line, t->col);
      break;
 
   case NO_BLCLOSE_ERR:
      printf("\nError: Expected '}' around %d:%d", t->line, t->col);
      break;
 
   case NO_SEMICOLON_ERR:
      printf("\nError: Expected ';' around %d:%d", t->line, t->col);
      break;
 
   case UNDEFINED_TYPE_ERR:
      printf("\nError: %s undefined type around %d:%d", (char *) t->value, t->line, t->col);
      break;
 
   case REDEFINED_ERR:
      printf("\nError: %s redefined around %d:%d", (char *) t->value, t->line, t->col);
      break;
 
   case SYNTAX_ERR:
      printf("\nError: syntax error around %d:%d", t->line, t->col);
      break;

   default:
      printf("\nError: undefined error around %d:%d", t->line, t->col);
      break;

       
   }
   return;
}
