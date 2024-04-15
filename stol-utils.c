#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "stol-utils.h"
#include "stol-errors.h"
#include "eval.h"
#include "directive.h"

char ros_name[1024];
char outbuf[1024];

char *keywords[] = {"static", "extern", "include", "define", "int", "long", "char", "short", "{", "}", "[", "]", "+", "-", "%", "/", "*", "--", "++", "<", "<=", ">" , ">="};
int isValidId(char *outbuf)
{
   if (strlen(outbuf) == 0)
       return 0;
   if (outbuf[0] >= '0' && outbuf[0] <= '9')
       return 0;
   if (isspace(outbuf[0]) || outbuf[0] == '\b' || outbuf[0] == '\a')
       return 0;
   return 1;
}

int isValidInt(char *outbuf)
{
   int i, flag, decimal = 0, negative = 0, positive = 0;
   if (strlen(outbuf) == 0)
       return 0;
   
   for (i = 0, flag = 1; i < strlen(outbuf); i++) {
       if (outbuf[i] >= '0' && outbuf[i] <=  '9')
          continue;
       if (decimal == 0 && outbuf[i] == '.') {
          decimal  = 1;
          continue;
       }
       flag = 0;
       break;
   }
   if (flag == 1)
       return 1;
   if (outbuf[0] != 0 && outbuf[0] != 'x' && outbuf[0] != 'X')
       return 0;
   for (i = 0, flag = 1; i < strlen(outbuf); i++) {
       if ((outbuf[i] >= '0' && outbuf[i] <=  '9') ||
           (outbuf[i] >= 'a' && outbuf[i] <= 'f') ||
           (outbuf[i] >= 'A' && outbuf[i] <= 'F'))
          continue;
       flag = 0;
       return 0;
   }
   return 1;
}
int isValidNumber(char *outbuf)
{
   int i, flag, decimal = 0, negative = 0, positive = 0;
   if (strlen(outbuf) == 0)
       return 0;
   
   for (i = 0, flag = 1; i < strlen(outbuf); i++) {
       if (outbuf[i] >= '0' && outbuf[i] <=  '9')
          continue;
       if (decimal == 0 && outbuf[i] == '.') {
          decimal  = 1;
          continue;
       }
       if (i == 0 && negative == 0 && outbuf[i] == '-') {
          negative  = 1;
          continue;
       }
       flag = 0;
       break;
   }
   if (flag == 1)
       return 1;
   if (outbuf[0] != 0 && outbuf[0] != 'x' && outbuf[0] != 'X')
       return 0;
   for (i = 0, flag = 1; i < strlen(outbuf); i++) {
       if ((outbuf[i] >= '0' && outbuf[i] <=  '9') ||
           (outbuf[i] >= 'a' && outbuf[i] <= 'f') ||
           (outbuf[i] >= 'A' && outbuf[i] <= 'F'))
          continue;
       flag = 0;
       return 0;
   }
   return 1;
}

int isValidKeyword(char *outbuf)
{
    int i;

    for (i = 0; i < sizeof(keywords)/sizeof(char *); i++)
        if (strcmp(outbuf, keywords[i]) == 0)
           return 1;
    return 0;
}

static char filename[1024] = "";
static uint line = 1, col = 1;
int get_line(char *outbuf, char *inbuf, int len)
{
   int i;
   char *origout = outbuf;

   outbuf[0] = 0;
   for (i = 0; i < len; i++) {
      *outbuf++ = inbuf[i];
      col++;
      if (inbuf[i] == '\n' || inbuf[i] == '\r') {
         line++;
         if (i < len -1) {
             if ((inbuf[i+1] == '\r' || inbuf[i+1] == '\n') && inbuf[i+1] != inbuf[i]) {
                //Some editor adds \n\r or \r\n as a line
                //Some editor adds jusr '\n' or '\r'
                *outbuf++ = inbuf[i++];
             }
         }
         break; 
      }
   }
   *outbuf = 0;
   return i;
}
int get_token(char *outbuf, const const char *inbuf, int len, token_t *token)
{
   int i, backslash = 0, comment = 0, line_comment = 0, j = 0;
   char *origout = outbuf;
   const char *nextbuf = inbuf;

   outbuf[0] = 0;
   memset(token, 0, sizeof(token[0]));
   for (i = 0; i < len; i++) {
      if(inbuf[i] == '\n' || inbuf[i] == '\v') {
         line++;
         line_comment = 0;
         col = 1;
         continue;
      }
      if(inbuf[i] == '\r') {
         col = 1;
         continue;
      }
      if(inbuf[i] == '\b') {
         if(col > 1)
            col--;
         continue;
      }
      if (inbuf[i] == ' ' || inbuf[i] == '\t') {
         col++; 
         continue;
      }
      if (inbuf[i] == '\a')
         continue;
      if (inbuf[i] == '/') {
         if (i < len -1) {
            if (inbuf[i+1] == '*') {
               comment = 1;
               i++;
               continue;
            }

         }
         if (backslash == 0)
             backslash = 1;
         else {
             line_comment = 1;
             backslash = 0;
         }
         
      } else
         backslash = 0;
      if (comment == 1) {
         if (inbuf[i] == '*') {
            if (i < len -1 && inbuf[i+1] == '/') {
               comment = 0;
               i++;
               continue;
            }
         }
         continue;
      }
      if (line_comment == 1)
          continue;
      break;
   }
   if (i >= len)
      return i;
   token->line = line;
   token->col = col;
   comment = 0;
   backslash = 0;
   line_comment = 0;
   for (; i < len; i++) {
      if (comment == 1) {
         if (inbuf[i] == '*') {
            if (i < len-1) {
               if (inbuf[i+1] == '/') {
                  comment = 0;
                  continue;
               } else
                  continue;
            }
         }
      } else if (line_comment == 1) {
         if (inbuf[i] == '\n' || inbuf[i] == '\r' || inbuf[i] == '\v') {
             line_comment = 0;
             i++;
             if (j > 0) // got some token
                break;
         }
         continue;
      } else if (inbuf[i] == '/') {
         if (i < len -1) {
            if (inbuf[i+1] == '*') {
               comment = 1;
               backslash = 0;
               i++;
               continue;
            } 
            if (inbuf[i+1] == '/') {
               line_comment = 1;
               i++; //go to the next line to eat all the comment on this line
            }
         } 
      } else if (line_comment == 1) {
         if (inbuf[i] == '\n' || inbuf[i] == '\r' || inbuf[i] == '\v') {
             line_comment = 0;
             i++;
             if (j >0)
                break;
         }
      }
      if (comment == 1)
         continue;
      if (line_comment == 1 && j > 0)
         continue;
      if (inbuf[i] == ' ' || inbuf[i] == '\t' || inbuf[i] == '\n' || inbuf[i] == '\r' || inbuf[i] == '\v' || inbuf[i] == '\b' || inbuf[i] == '\a')
         break;
      *outbuf++ = inbuf[i];
      j++;
      col++;
      if (inbuf[i] == '+' || inbuf[i] == '-' || inbuf[i] == '<' || inbuf[i] == '>' || inbuf[i] == '=' || inbuf[i] == '&' || inbuf[i] == '|') {
         if (i < len -1) {
            if(inbuf[i] == inbuf[i+1]) {
               *outbuf++ = inbuf[i+1];
               i += 2; //two byte token
               break;
            }
            i++; //valid token
            break;
         }
      }
      if (inbuf[i] == '*' || inbuf[i] == '/' || inbuf[i] == '%' ) {
         i++;
         break;
      }
      if (inbuf[i] == ',' || inbuf[i] == ';' || inbuf[i] == '{' || inbuf[i] == '(' || inbuf[i] == '[' || inbuf[i] == '{' || inbuf[i] == ')' || inbuf[i] == ']' || inbuf[i] == ')') {
         i++;
         break;
      }
      if (i < len -1) {
         if (inbuf[i+1] == ',' || inbuf[i+1] == ';' || inbuf[i+1] == '{' || inbuf[i+1] == '(' || inbuf[i+1] == '[' || inbuf[i+1] == '{' || inbuf[i+1] == ')' || inbuf[i+1] == ']' || inbuf[i+1] == ')') {
            i++;
            break;
         }
      }
   }
   *outbuf = 0;
   if (strcmp(origout, ";") == 0)
      token->id = SEMICOLON;
   else if (strcmp(origout, ",") == 0)
      token->id = COMMA;
   else if (strcmp(origout, "==") == 0)
      token->id = EQUAL_TO;
   else if (strcmp(origout, "=") == 0)
      token->id = ASSIGNMENT;
   else if (strcmp(origout, "++") == 0)
      token->id = PLUSPLUS;
   else if (strcmp(origout, "--") == 0)
      token->id = MINUSMINUS;
   else if (strcmp(origout, "&&") == 0)
      token->id = LOGICAL_AND;
   else if (strcmp(origout, "&") == 0)
      token->id = AMPERSAND;
   else if (strcmp(origout, "|") == 0)
      token->id = OR;
   else if (strcmp(origout, "||") == 0)
      token->id = LOGICAL_OR;
   else if (strcmp(origout, "<<") == 0)
      token->id = LEFT_SHIFT;
   else if (strcmp(origout, ">>") == 0)
      token->id = RIGHT_SHIFT;
   else if (strcmp(origout, "+") == 0)
      token->id = PLUS;
   else if (strcmp(origout, "-") == 0)
      token->id = MINUS;
   else if (strcmp(origout, "*") == 0)
      token->id = STAR;
   else if (strcmp(origout, "/") == 0)
      token->id = DIVISION;
   else if (strcmp(origout, "%") == 0)
      token->id = MODULO;
   else if (strcmp(origout, "!") == 0)
      token->id = NOT;
   else if (strcmp(origout, "^") == 0)
      token->id = XOR;
   else if (strcmp(origout, "~") == 0)
      token->id = TOGGLE;
   else if (strcmp(origout, "(") == 0)
      token->id = BRACKET_OPEN;
   else if (strcmp(origout, ")") == 0)
      token->id = BRACKET_CLOSE;
   else if (strcmp(origout, "{") == 0)
      token->id = BLOCK_BEGIN;
   else if (strcmp(origout, "}") == 0)
      token->id = BLOCK_END;
   else if (strcmp(origout, "[") == 0)
      token->id = SQUARE_OPEN;
   else if (strcmp(origout, "]") == 0)
      token->id = SQUARE_CLOSE;
   else if (strcmp(origout, "unsigned") == 0)
      token->id = UNSIGNED;
   else if (strcmp(origout, "int") == 0)
      token->id = INTEGER;
   else if (strcmp(origout, "long") == 0)
      token->id = LONG;
   else if (strcmp(origout, "char") == 0)
      token->id = CHAR;
   else if (strcmp(origout, "float") == 0)
      token->id = FLOAT;
   else if (strcmp(origout, "double") == 0)
      token->id = DOUBLE;
   else if (strcmp(origout, "union") == 0)
      token->id = UNION;
   else if (strcmp(origout, "struct") == 0)
      token->id = STRUCT;
   else if (strcmp(origout, "typedef") == 0)
      token->id = TYPEDEF;
   else if (strcmp(origout, "static") == 0)
      token->id = STATIC;
   else if (isValidNumber(origout))
      token->id = NUMBER;
   else if (isValidId(origout))
      token->id = IDENTIFIER;
   else if (isValidKeyword(origout))
      token->id = KEYWORD;
   
   return i;
}

int parse_typedef_int(char *buf, int len, int *outlen, token_t *out)
{
   int i;
   token_t *next, *orig = out;
   int lenEaten = 0;
   token_t token;
   
   i = get_token(outbuf, buf, len, &token);
   if (i <= 0)
      return i;
   outlen[0] += i;
   lenEaten += i;
   buf += i;
   len -= i;
   if (isValidId(outbuf)) {
      next = (token_t *) calloc(1, sizeof(token_t));
      if (!next) 
        return -1;
      next->id = IDENTIFIER;
       next->line = token.line;
       next->col = token.col;
      next->line = token.line;
      next->col = token.col;
      next->value = calloc(1, i+1);
      if (next->value == NULL) {
         free(next);
         return NO_MEM_ERR;
      }
      strcpy((char *) next->value, outbuf);
      while(out->next)
         out = out->next;
      out->next = next;

      i = get_token(outbuf, buf, len, &token);
      if (i <= 0) {
         free(next->value); 
         free(next);
         return SYNTAX_ERR;
      }
      buf += i;
      len -= i;
      outlen[0] += i;
       lenEaten += i;

      if (strcmp(outbuf, ",") == 0) {
 
         next = (token_t *) calloc(1, sizeof(token_t));
         if (next == NULL) {
            return NO_MEM_ERR;
         }
         next->id = COMMA;
         next->line = token.line;
         next->col = token.col;
         next->line = token.line;
         next->col = token.col;
         while(out->next)
            out = out->next;
         out->next = next;
          
         i = parse_typedef_int(buf, len, outlen, orig);
         if (i > 0)
           lenEaten += i;
         return lenEaten;
      }
      if (strcmp(outbuf, ";") == 0) {
         next = (token_t *) calloc(1, sizeof(token_t));
         if (next == NULL) {
            return NO_MEM_ERR;
         }
         next->id = SEMICOLON;
         next->line = token.line;
         next->col = token.col;
         next->line = token.line;
         next->col = token.col;
         while(out->next)
            out = out->next;
         out->next = next;
         return parse_typedef_int(buf, len, outlen, orig);
      }
      return SYNTAX_ERR;
   }
   return -1;
}

int parse_typedef_long(char *buf, int len, int *outlen, token_t *out)
{
   return parse_typedef_int(buf, len, outlen, out);
}


int parse_typedef_uint(char *buf, int len, int *outlen, token_t *out)
{
   int i;
   token_t *next, *orig = out;
   int lenEaten = 0;
   token_t token;
   
   outlen[0] = 0;
   i = get_token(outbuf, buf, len, &token);
   if (i <= 0)
      return i;
   outlen[0] += i;
   lenEaten += i;
   buf += i;
   len -= i;
   if (isValidId(outbuf)) {
      next = (token_t *) calloc(1, sizeof(token_t));
      if (!next) 
        return -1;
      next->id = IDENTIFIER;
      next->line = token.line;
      next->col = token.col;
      next->line = token.line;
      next->col = token.col;
      next->value = calloc(1, i+1);
      if (next->value == NULL) {
         free(next);
         return NO_MEM_ERR;
      }
      strcpy((char *) next->value, outbuf);
      while(out->next)
         out = out->next;
      out->next = next;

      i = get_token(outbuf, buf, len, &token);
      if (i <= 0) {
         free(next->value); 
         free(next);
         return SYNTAX_ERR;
      }
      buf += i;
      len -= i;
      outlen[0] += i;
      lenEaten += i;
      if (strcmp(outbuf, ",") == 0) {
         next = (token_t *) calloc(1, sizeof(token_t));
         if (next == NULL) {
            return NO_MEM_ERR;
         }
         next->id = COMMA;
         next->line = token.line;
         next->col = token.col;
         while(out->next)
            out = out->next;
         out->next = next;
         return parse_typedef_int(buf, len, outlen, orig);
      }
      if (strcmp(outbuf, ";") == 0) {
         next = (token_t *) calloc(1, sizeof(token_t));
         if (next == NULL) {
            return NO_MEM_ERR;
         }
         next->id = SEMICOLON;
         next->line = token.line;
         next->col = token.col;
         while(out->next)
            out = out->next;
         out->next = next;
         return parse_typedef_int(buf, len, outlen, orig);
      }
      return SYNTAX_ERR;
   }
   return -1;
}
int parse_typedef_ulong(char *buf, int len, int *outlen, token_t *out)
{
   return parse_typedef_uint(buf, len, outlen, out);
}


int parse_typedef_uint16(char *buf, int len, int *outlen, token_t *out)
{
   int i;
   int lenEaten = 0;
   token_t *next, *orig = out;
   token_t token;
   
   outlen[0] = 0;
   i = get_token(outbuf, buf, len, &token);
   if (i <= 0)
      return i;
   outlen[0] += i;
   lenEaten += i;
   if (isValidId(outbuf)) {
      next = (token_t *) calloc(1, sizeof(token_t));
      if (!next) 
        return -1;
      next->id = IDENTIFIER;
      next->line = token.line;
      next->col = token.col;
      next->value = calloc(1, i+1);
      if (next->value == NULL) {
         free(next);
         return NO_MEM_ERR;
      }
      strcpy((char *) next->value, outbuf);
      while(out->next)
         out = out->next;
      out->next = next;

      i = get_token(outbuf, buf, len, &token);
      if (i <= 0) {
         free(next->value); 
         free(next);
         return SYNTAX_ERR;
      }
      buf += i;
      len -= i;
      lenEaten += i;
      outlen[0] += i;
      if (strcmp(outbuf, ",") == 0) {
         next = (token_t *) calloc(1, sizeof(token_t));
         if (next == NULL) {
            return NO_MEM_ERR;
         }
         next->id = COMMA;
         next->line = token.line;
         next->col = token.col;
         while(out->next)
            out = out->next;
         out->next = next;
         return parse_typedef_int(buf, len, outlen, orig);
      }
      if (strcmp(outbuf, ";") == 0) {
         next = (token_t *) calloc(1, sizeof(token_t));
         if (next == NULL) {
            return NO_MEM_ERR;
         }
         next->id = SEMICOLON;
         next->line = token.line;
         next->col = token.col;
         while(out->next)
            out = out->next;
         out->next = next;
         return parse_typedef_int(buf, len, outlen, orig);
      }
      return SYNTAX_ERR;
   }
   return -1;
}

int parse_typedef_int16(char *buf, int len, int *outlen, token_t *out)
{
   int i;
   int lenEaten = 0;
   token_t *next, *orig = out;
   token_t token;
   
   outlen[0] = 0;
   i = get_token(outbuf, buf, len, &token);
   if (i <= 0)
      return i;
   outlen[0] += i;
   lenEaten += i;
   buf += i;
   len -= i;
   if (isValidId(outbuf)) {
      next = (token_t *) calloc(1, sizeof(token_t));
      if (!next) 
        return -1;
      next->id = IDENTIFIER;
      next->line = token.line;
      next->col = token.col;
      next->value = calloc(1, i+1);
      if (next->value == NULL) {
         free(next);
         return NO_MEM_ERR;
      }
      strcpy((char *) next->value, outbuf);
      while(out->next)
         out = out->next;
      out->next = next;

      i = get_token(outbuf, buf, len, &token);
      if (i <= 0) {
         free(next->value); 
         free(next);
         return SYNTAX_ERR;
      }
      buf += i;
      len -= i;
      lenEaten += i;

      if (strcmp(outbuf, ",") == 0) {
         next = (token_t *) calloc(1, sizeof(token_t));
         if (next == NULL) {
            return NO_MEM_ERR;
         }
         next->id = COMMA;
         next->line = token.line;
         next->col = token.col;
         while(out->next)
            out = out->next;
         out->next = next;
         i = parse_typedef_int(buf, len, outlen, orig);
         if (i > 0)
            lenEaten += i;
         return lenEaten;
      }
      if (strcmp(outbuf, ";") == 0) {
         next = (token_t *) calloc(1, sizeof(token_t));
         if (next == NULL) {
            return NO_MEM_ERR;
         }
         next->id = SEMICOLON;
         next->line = token.line;
         next->col = token.col;
         while(out->next)
            out = out->next;
         out->next = next;
         return parse_typedef_int(buf, len, outlen, orig);
      }
      return SYNTAX_ERR;
   }
   return -1;
}

int parse_typedef_enum_block(char *buf, int len, int *outlen, token_t *out)
{
   int i;
   int lenEaten = 0;
   token_t *next, *orig = out;
   token_t token;
   
   do {
      i = get_token(outbuf, buf, len, &token);
      if (i <= 0)
         return SYNTAX_ERR;
      if (strcmp(outbuf, "}") == 0) {
         outlen[0] += i;
         len -= i;
         buf += i;
         lenEaten += i;
         next = (token_t *) calloc(1, sizeof(token_t));
         if (!next) 
            return -1;
         next->id = BLOCK_END;
         next->line = token.line;
         next->col = token.col;
         while(out->next)
            out = out->next;
         out->next = next;
         continue;
      }
      if (isValidId(outbuf)) {
         outlen[0] += i;
         len -= i;
         buf += i;
         lenEaten += i;
         next = (token_t *) calloc(1, sizeof(token_t));
         if (!next) 
            return -1;
         next->id = IDENTIFIER;
         next->line = token.line;
         next->col = token.col;
         next->value = calloc(1, strlen(outbuf)+1);
         if (next->value == NULL) {
            free(next);
            return NO_MEM_ERR;
         }
         strcpy((char *) next->value, outbuf);
         while(out->next)
            out = out->next;
         out->next = next;
         continue;
      }
      if (strcmp(outbuf, ";")) {
         outlen[0] += i;
         len -= i;
         buf += i;
         lenEaten += i;
         next = (token_t *) calloc(1, sizeof(token_t));
         if (!next) 
            return -1;
         next->id = SEMICOLON;
         next->line = token.line;
         next->col = token.col;
         while(out->next)
            out = out->next;
         out->next = next;
         return lenEaten;
      }
      i = parse_statement(buf, len, outlen, &orig);
      if (i <= 0)
        break;
      lenEaten += i;
      buf += i;
      len -= i;
   } while(len > 0);
   return SYNTAX_ERR;
}
int parse_typedef_enum_identifier(char *buf, int len, int *outlen, token_t *out)
{
   int i;
   int lenEaten = 0;
   token_t *next, *orig = out;
   token_t token;
   
   outlen[0] = 0;
   i = get_token(outbuf, buf, len, &token);
   if (i <= 0)
      return SYNTAX_ERR;
   outlen[0] += i;
   lenEaten += i;
   buf += i;
   len -= i;
   if (strcmp(outbuf, "{") == 0) {
      next = (token_t *) calloc(1, sizeof(token_t));
      if (!next) 
         return -1;
      next->id = BLOCK_BEGIN;
      next->line = token.line;
      next->col = token.col;
      while(out->next)
          out = out->next;
       out->next = next;
       i = parse_typedef_enum_block(buf, len, outlen, orig);
       if (i > 0) {
          lenEaten += i;
          buf += i;
          len -= i;
       }
       return lenEaten;
   }
   return SYNTAX_ERR;
}
int parse_typedef_enum(char *buf, int len, int *outlen, token_t *out)
{
   int i;
   int lenEaten = 0;
   token_t *next, *orig = out;
   token_t token;
   
   i = get_token(outbuf, buf, len, &token);
   if (i <= 0)
      return SYNTAX_ERR;
   outlen[0] += i;
   lenEaten += i;
   buf += i;
   len -= i;
   if (isValidId(outbuf)) {
      next = (token_t *) calloc(1, sizeof(token_t));
      if (!next) 
         return -1;
      next->id = IDENTIFIER;
      next->line = token.line;
      next->col = token.col;
      next->value = calloc(1, i+1);
      if (next->value == NULL) {
          free(next);
          return NO_MEM_ERR;
       }
       strcpy((char *) next->value, outbuf);
       while(out->next)
          out = out->next;
       out->next = next;
       i = parse_typedef_enum_identifier(buf, len, outlen, orig);
       if (i >= 0) {
         lenEaten += i;
         buf += i;
         outlen[0] += 0;
         len -= i;
         return lenEaten;
       }
       return SYNTAX_ERR;
   }
   if (strcmp(outbuf, "{") == 0) {
      next = (token_t *) calloc(1, sizeof(token_t));
      if (!next) 
         return -1;
      next->id = BLOCK_BEGIN;
      next->line = token.line;
      next->col = token.col;
      while(out->next)
          out = out->next;
       out->next = next;
       i = parse_typedef_enum_block(buf, len, outlen, orig);
       if (i >= 0) {
         lenEaten += i;
         buf += i;
         outlen[0] += 0;
         len -= i;
         return lenEaten;
       }
   }
   return SYNTAX_ERR;

}

int parse_typedef_union_block(char *buf, int len, int *outlen, token_t *out)
{
   int i;
   int lenEaten = 0;
   token_t *next, *orig = out;
   token_t token;
   
   do {
      i = get_token(outbuf, buf, len, &token);
      if (i <= 0)
         return SYNTAX_ERR;
      if (strcmp(outbuf, "}") == 0) {
         outlen[0] += i;
         len -= i;
         buf += i;
         lenEaten += i;
         next = (token_t *) calloc(1, sizeof(token_t));
         if (!next) 
            return -1;
         next->id = BLOCK_END;
         next->line = token.line;
         next->col = token.col;
         while(out->next)
            out = out->next;
         out->next = next;
         continue;
      }
      if (isValidId(outbuf)) {
         outlen[0] += i;
         len -= i;
         buf += i;
         lenEaten += i;
         next = (token_t *) calloc(1, sizeof(token_t));
         if (!next) 
            return -1;
         next->id = IDENTIFIER;
         next->line = token.line;
         next->col = token.col;
         next->value = calloc(1, strlen(outbuf)+1);
         if (next->value == NULL) {
            free(next);
            return NO_MEM_ERR;
         }
         strcpy((char *) next->value, outbuf);
         while(out->next)
            out = out->next;
         out->next = next;
         continue;
      }
      if (strcmp(outbuf, ";")) {
         outlen[0] += i;
         len -= i;
         buf += i;
         lenEaten += i;
         next = (token_t *) calloc(1, sizeof(token_t));
         if (!next) 
            return -1;
         next->id = SEMICOLON;
         next->line = token.line;
         next->col = token.col;
         while(out->next)
            out = out->next;
         out->next = next;
         return lenEaten;
      }
      i = parse_statement(buf, len, outlen, &orig);
      if (i <= 0)
        break;
      lenEaten += i;
      buf += i;
      len -= i;
      outlen[0] += i;
   } while(len > 0);
   return SYNTAX_ERR;
}

int parse_typedef_union_identifier(char *buf, int len, int *outlen, token_t *out)
{
   int i;
   int lenEaten = 0;
   token_t *next, *orig = out;
   token_t token;
   
   outlen[0] = 0;
   i = get_token(outbuf, buf, len, &token);
   if (i <= 0)
      return SYNTAX_ERR;
   outlen[0] += i;
   lenEaten += i;
   if (strcmp(outbuf, "{") == 0) {
      next = (token_t *) calloc(1, sizeof(token_t));
      if (!next) 
         return -1;
      next->id = BLOCK_BEGIN;
      next->line = token.line;
      next->col = token.col;
      while(out->next)
          out = out->next;
       out->next = next;
       i = parse_typedef_union_block(buf, len, outlen, orig);
       if (i >= 0) {
          lenEaten += i;
          return lenEaten;
       }
   }
   return SYNTAX_ERR;
}

int parse_typedef_union(char *buf, int len, int *outlen, token_t *out)
{
   int i;
   int lenEaten = 0;
   token_t *next, *orig = out;
   token_t token;
   
   i = get_token(outbuf, buf, len, &token);
   if (i <= 0)
      return SYNTAX_ERR;
   outlen[0] += i;
   lenEaten += i;
   if (isValidId(outbuf)) {
      next = (token_t *) calloc(1, sizeof(token_t));
      if (!next) 
         return -1;
      next->id = IDENTIFIER;
      next->line = token.line;
      next->col = token.col;
      next->value = calloc(1, i+1);
      if (next->value == NULL) {
          free(next);
          return NO_MEM_ERR;
       }
       strcpy((char *) next->value, outbuf);
       while(out->next)
          out = out->next;
       out->next = next;
       i = parse_typedef_struct_identifier(buf, len, outlen, orig);
       if (i >= 0) {
          lenEaten += i;
          return lenEaten;
       }
   }
   if (strcmp(outbuf, "{") == 0) {
      next = (token_t *) calloc(1, sizeof(token_t));
      if (!next) 
         return -1;
      next->id = BLOCK_BEGIN;
      next->line = token.line;
      next->col = token.col;
      while(out->next)
          out = out->next;
       out->next = next;
       i = parse_typedef_struct_block(buf, len, outlen, orig);
       if (i >= 0) {
          lenEaten += i;
          return lenEaten;
       }
   }
   return SYNTAX_ERR;
}

int parse_typedef_struct_block(char *buf, int len, int *outlen, token_t *out)
{
   int i;
   int lenEaten = 0;
   token_t *next, *orig = out;
   token_t token;
   
   do {
      i = get_token(outbuf, buf, len, &token);
      if (i <= 0)
         return SYNTAX_ERR;
      if (strcmp(outbuf, "}") == 0) {
         outlen[0] += i;
         len -= i;
         buf += i;
         lenEaten += i;
         next = (token_t *) calloc(1, sizeof(token_t));
         if (!next) 
            return -1;
         next->id = BLOCK_END;
         next->line = token.line;
         next->col = token.col;
         while(out->next)
            out = out->next;
         out->next = next;
         continue;
      }
      if (isValidId(outbuf)) {
         outlen[0] += i;
         len -= i;
         buf += i;
         lenEaten += i;
         next = (token_t *) calloc(1, sizeof(token_t));
         if (!next) 
            return -1;
         next->id = IDENTIFIER;
         next->line = token.line;
         next->col = token.col;
         next->value = calloc(1, strlen(outbuf)+1);
         if (next->value == NULL) {
            free(next);
            return NO_MEM_ERR;
         }
         strcpy((char *) next->value, outbuf);
         while(out->next)
            out = out->next;
         out->next = next;
         continue;
      }
      if (strcmp(outbuf, ";")) {
         outlen[0] += i;
         len -= i;
         buf += i;
         lenEaten += i;
         next = (token_t *) calloc(1, sizeof(token_t));
         if (!next) 
            return -1;
         next->id = SEMICOLON;
         next->line = token.line;
         next->col = token.col;
         while(out->next)
            out = out->next;
         out->next = next;
         return lenEaten;
      }
      i = parse_statement(buf, len, outlen, &orig);
      if (i <= 0)
        break;
      len -= i;
      lenEaten += i;   
   } while(len > 0);
   return SYNTAX_ERR;
}

int parse_typedef_struct_identifier(char *buf, int len, int *outlen, token_t *out)
{
   int i;
   int lenEaten = 0;
   token_t *next, *orig = out;
   token_t token;
   
   outlen[0] = 0;
   i = get_token(outbuf, buf, len, &token);
   if (i <= 0)
      return SYNTAX_ERR;
   outlen[0] += i;
   lenEaten += i;
   if (strcmp(outbuf, "{") == 0) {
      next = (token_t *) calloc(1, sizeof(token_t));
      if (!next) 
         return -1;
      next->id = BLOCK_BEGIN;
      next->line = token.line;
      next->col = token.col;
      while(out->next)
          out = out->next;
       out->next = next;
       i = parse_typedef_struct_block(buf, len, outlen, orig);
       if ( i >= 0) {
          lenEaten += i;
          return lenEaten; 
       }
   }
   return SYNTAX_ERR;
}
int parse_typedef_struct(char *buf, int len, int *outlen, token_t *out)
{
   int i;
   int lenEaten = 0;
   token_t *next, *orig = out;
   token_t token;
   
   i = get_token(outbuf, buf, len, &token);
   if (i <= 0)
      return SYNTAX_ERR;
   outlen[0] += i;
   lenEaten += i;
   if (isValidId(outbuf)) {
      next = (token_t *) calloc(1, sizeof(token_t));
      if (!next) 
         return -1;
      next->id = IDENTIFIER;
      next->line = token.line;
      next->col = token.col;
      next->value = calloc(1, strlen(outbuf)+1);
      if (next->value == NULL) {
         free(next);
         return NO_MEM_ERR;
      }
      strcpy((char *) next->value, outbuf);
       while(out->next)
          out = out->next;
       out->next = next;
       i = parse_typedef_struct_identifier(buf, len, outlen, orig);
       if (i >= 0) {
          lenEaten += i;
          return lenEaten;
       }
       return i;
   }
   if (strcmp(outbuf, "{") == 0) {
      next = (token_t *) calloc(1, sizeof(token_t));
      if (!next) 
         return -1;
      next->id = BLOCK_BEGIN;
      next->line = token.line;
      next->col = token.col;
      while(out->next)
          out = out->next;
       out->next = next;
       i = parse_typedef_struct_block(buf, len, outlen, orig);
       if (i >= 0) {
          lenEaten += i;
          return lenEaten;
       }
       return i;
   }
   return SYNTAX_ERR;
}


int parse_enum(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int i;
    int lenEaten = 0;
    token_t token;

    while(len > 0) {
       i = get_token(outbuf, buf, len, &token);
       if (i <= 0) {
          return SYNTAX_ERR;
       }
       if (strcmp(outbuf, "{") == 0) {
          len -= i;
          outlen[0] += i;
          buf += i; 
          lenEaten += i;
          next = (token_t *) calloc(1, sizeof(token_t));
          if (!next)
             return NO_MEM_ERR;
          next->id = BLOCK_BEGIN;
          next->line = token.line;
          next->col = token.col;
          while(out->next)
             out = out->next;
          i = parse_enum_block(buf, len, outlen, orig);
          if (i > 0) {
             lenEaten += i;
             continue;
          }
          return SYNTAX_ERR;
       }
       if (strcmp(outbuf, ",") == 0) {
          len -= i;
          outlen[0] += i;
          buf += i; 
          lenEaten += i;
          next = (token_t *) calloc(1, sizeof(token_t));
          if (!next)
             return NO_MEM_ERR;
          next->id = COMMA;
          next->line = token.line;
          next->col = token.col;
          while(out->next)
             out = out->next;
          out->next = next;
          continue;
       }
       if (strcmp(outbuf, ";") == 0) {
          len -= i;
          outlen[0] += i;
          buf += i; 
          lenEaten += i;
          next = (token_t *) calloc(1, sizeof(token_t));
          if (!next)
             return NO_MEM_ERR;
          next->id = SEMICOLON;
          next->line = token.line;
          next->col = token.col;
          while(out->next)
             out = out->next;
          out->next = next;
          return lenEaten;
       }
       if (isValidId(outbuf)) {
          len -= i;
          outlen[0] += i;
          buf += i; 
          lenEaten += i;
          next = (token_t *) calloc(1, sizeof(token_t));
          if (!next)
             return NO_MEM_ERR;
          next->id = IDENTIFIER;
          next->line = token.line;
          next->col = token.col;
          next->value = calloc(1, strlen(outbuf)+1);
          if (next->value == NULL) {
             free(next);
             return NO_MEM_ERR;
          }
          strcpy((char *) next->value, outbuf);
          while(out->next)
             out = out->next;
          out->next = next;
          continue;
       }
       return SYNTAX_ERR;
    }
    return SYNTAX_ERR;
}

int parse_struct_identifier(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int i;
    int lenEaten = 0;
    token_t token;
    
    i = get_token(outbuf, buf, len, &token);
    if (i <= 0) {
       return SYNTAX_ERR;
    }
    if (strcmp(outbuf, "{") == 0) {
       len -= i;
       outlen[0] += i;
       buf += i; 
       lenEaten += i;
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = BLOCK_BEGIN;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
          out = out->next;
       i = parse_struct_block(buf, len, outlen, orig);
       if (i >= 0) {
          lenEaten += i;
          return lenEaten;
       }
       return i;
       
    }
    if (strcmp(outbuf, ";") == 0) {
       len -= i;
       outlen[0] += i;
       buf += i; 
       lenEaten += i;
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = SEMICOLON;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
          out = out->next;
       return lenEaten;
    }
    return SYNTAX_ERR;
}

int parse_struct(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int i;
    int lenEaten = 0;
    token_t token;

    while(len > 0) {
       i = get_token(outbuf, buf, len, &token);
       if (i <= 0) {
          return SYNTAX_ERR;
       }
       if (strcmp(outbuf, "{") == 0) {
          len -= i;
          outlen[0] += i;
          buf += i; 
          lenEaten += i;
          next = (token_t *) calloc(1, sizeof(token_t));
          if (!next)
             return NO_MEM_ERR;
          next->id = BLOCK_BEGIN;
          next->line = token.line;
          next->col = token.col;
          while(out->next)
             out = out->next;
          out->next = next;
          i = parse_struct_block(buf, len, outlen, orig);
          if (i > 0) {
             lenEaten += i;
             return lenEaten;
          }
          continue;
       }
       if (strcmp(outbuf, "*") == 0) {
          len -= i;
          outlen[0] += i;
          buf += i; 
          lenEaten += i;
          next = (token_t *) calloc(1, sizeof(token_t));
          if (!next)
             return NO_MEM_ERR;
          next->id = STAR;
       next->line = token.line;
       next->col = token.col;
          while(out->next)
          out = out->next;
          out->next = next;
          continue;
       }
       if (strcmp(outbuf, ",") == 0) {
          len -= i;
          outlen[0] += i;
          buf += i; 
          lenEaten += i;
          next = (token_t *) calloc(1, sizeof(token_t));
          if (!next)
             return NO_MEM_ERR;
          next->id = SEMICOLON;
          next->line = token.line;
          next->col = token.col;
          while(out->next)
             out = out->next;
          out->next = next;
          continue;
       }
       if (strcmp(outbuf, ";") == 0) {
          len -= i;
          outlen[0] += i;
          buf += i; 
          lenEaten += i;
          next = (token_t *) calloc(1, sizeof(token_t));
          if (!next)
             return NO_MEM_ERR;
          next->id = SEMICOLON;
          next->line = token.line;
          next->col = token.col;
          while(out->next)
             out = out->next;
          out->next = next;
          return lenEaten;
       }
       if (isValidId(outbuf)) {
          len -= i;
          outlen[0] += i;
          buf += i; 
          lenEaten += i;
          next = (token_t *) calloc(1, sizeof(token_t));
          if (!next)
             return NO_MEM_ERR;
          next->id = IDENTIFIER;
          next->line = token.line;
          next->col = token.col;
          next->value = calloc(1, strlen(outbuf)+1);
          if (next->value == NULL) {
             free(next);
             return NO_MEM_ERR;
          }
          strcpy((char *) next->value, outbuf);
          while(out->next)
             out = out->next;
          out->next = next;
          continue;
       }
    }
    return SYNTAX_ERR;
}

int parse_struct_block(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int i;
    int lenEaten = 0;
    token_t token;


    i = get_token(outbuf, buf, len, &token);
    if (i <= 0) {
       return SYNTAX_ERR;
    }
    if (strcmp(outbuf, "}") == 0)
      return SYNTAX_ERR; // empty block
    do {     
       i = get_token(outbuf, buf, len, &token);
       if (i <= 0) {
          return SYNTAX_ERR;
       }
       if (strcmp(outbuf, "}") == 0) {
          buf += i;
          len -= i;
          outlen[0] += i;
          lenEaten += i;
          next = (token_t *) calloc(1, sizeof(token_t));
          if (next == NULL)
              return NO_MEM_ERR;
          next->id = BLOCK_END;
          next->line = token.line;
          next->col = token.col;
          while(out->next)
             out = out->next;
          out->next = next;
          continue;
       }
       if (strcmp(outbuf, ";") == 0) {
          next = (token_t *) calloc(1, sizeof(token_t));
          if (next == NULL)
              return NO_MEM_ERR;
          buf += i;
          len -= i;
          outlen[0] += i;
          lenEaten += i;
          next->id = SEMICOLON;
          next->line = token.line;
          next->col = token.col;
          while(out->next)
             out = out->next;
          out->next = next;
          return lenEaten;
       }
       if (strcmp(outbuf, ",") == 0) {
          next = (token_t *) calloc(1, sizeof(token_t));
          if (next == NULL)
              return NO_MEM_ERR;
          buf += i;
          len -= i;
          outlen[0] += i;
          lenEaten += i;
          next->id = COMMA;
          next->line = token.line;
          next->col = token.col;
          while(out->next)
             out = out->next;
          out->next = next;
          continue;
       }
       if (strcmp(outbuf, "{") == 0) {
          len -= i;
          outlen[0] += i;
          buf += i; 
          lenEaten += i;
          next = (token_t *) calloc(1, sizeof(token_t));
          if (!next)
             return NO_MEM_ERR;
          next->id = BLOCK_BEGIN;
          next->line = token.line;
          next->col = token.col;
          while(out->next)
             out = out->next;
          i = parse_struct_block(buf, len, outlen, orig);
          if ( i > 0)
             lenEaten += i;
          continue;
       }
       i = parse_declaration(buf, len, outlen, &orig);
       if (i <= 0)
          break;
       len -= i;
       buf += i;
       lenEaten += i;
    } while(len > 0); 
    return SYNTAX_ERR;
}

int parse_union(char *buf, int len, int *outlen, token_t *out)
{
    return parse_struct(buf, len, outlen, out);
}
 
int parse_enum_block(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int i;
    int lenEaten = 0;
    token_t token;


    i = get_token(outbuf, buf, len, &token);
    if (i <= 0) {
       return SYNTAX_ERR;
    }
    if (strcmp(outbuf, "}") == 0)
      return SYNTAX_ERR;
    do {
       i = get_token(outbuf, buf, len, &token);
       if (i <= 0) {
          return SYNTAX_ERR;
       }
       if (strcmp(outbuf, "}") == 0) {
          len -= i;
          lenEaten += i;
          buf += i;
          outlen[0] += i;
          next = (token_t *) calloc(1, sizeof(token_t));
          if (!next)
             return NO_MEM_ERR;
          next->id = SEMICOLON;
          next->line = token.line;
          next->col = token.col;
          while(out->next)
             out = out->next;
          out->next = next;
          return lenEaten;
       }
#if 1
       if (isValidId(outbuf)) {
          len -= i;
          outlen[0] += i;
          buf += i; 
          lenEaten += i;
          next = (token_t *) calloc(1, sizeof(token_t));
          if (!next)
             return NO_MEM_ERR;
          next->id = IDENTIFIER;
          next->line = token.line;
          next->col = token.col;
          next->value = calloc(1, strlen(outbuf)+1);
          if (next->value == NULL) {
             free(next);
             return NO_MEM_ERR;
          }
          strcpy((char *) next->value, outbuf);
          while(out->next)
             out = out->next;
          i = parse_enum_identifier(buf, len, outlen, orig);
          if ( i >= 0) {
             lenEaten += i;
             return lenEaten;
          }
          return i;
       }
#endif
       if (strcmp(outbuf, "}") == 0) {
          len -= i;
          outlen[0] += i;
          buf += i; 
          lenEaten += i;
          next = (token_t *) calloc(1, sizeof(token_t));
          if (!next)
             return NO_MEM_ERR;
          next->id = BLOCK_END;
          next->line = token.line;
          next->col = token.col;
          while(out->next)
             out = out->next;
          out->next = next;
       }
       if (strcmp(outbuf, ";") == 0) {
          len -= i;
          outlen[0] += i;
          buf += i; 
          lenEaten += i;
          next = (token_t *) calloc(1, sizeof(token_t));
          if (!next)
             return NO_MEM_ERR;
          next->id = SEMICOLON;
          next->line = token.line;
       next->col = token.col;
          while(out->next)
             out = out->next;
          out->next = next;
          return lenEaten; //this is hopefully end of enum definition
       }
       
    } while(len > 0); 
    return SYNTAX_ERR;
}

int parse_enum_identifier_equal(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int i;
    int lenEaten = 0;
    token_t token;

    while( len > 0) {
       i = get_token(outbuf, buf, len, &token);
       if (i <= 0) {
          return SYNTAX_ERR;
       }

       if (isValidInt(outbuf)) {
          len -= i;
          outlen[0] += i;
          buf += i; 
          lenEaten += i;
          next = (token_t *) calloc(1, sizeof(token_t));
          if (!next)
             return NO_MEM_ERR;
          next->id = INTEGER;
          next->line = token.line;
          next->col = token.col;
          while(out->next)
             out = out->next;
          continue;
        }
        if (isValidId(outbuf)) {
          len -= i;
          outlen[0] += i;
          buf += i; 
          lenEaten += i;
          next = (token_t *) calloc(1, sizeof(token_t));
          if (!next)
             return NO_MEM_ERR;
          next->id = IDENTIFIER;
          next->line = token.line;
          next->col = token.col;
          next->value = calloc(1, strlen(outbuf)+1);
          if (next->value == NULL) {
             free(next);
             return NO_MEM_ERR;
          }
          strcpy((char *) next->value, outbuf);
          while(out->next)
             out = out->next;
          out->next = next;
          continue;
       }
       if (strcmp(outbuf, ",") == 0) {
          len -= i;
          outlen[0] += i;
          buf += i; 
          lenEaten += i;
          next = (token_t *) calloc(1, sizeof(token_t));
          if (!next)
             return NO_MEM_ERR;
          next->id = COMMA;
          next->line = token.line;
          next->col = token.col;
          while(out->next)
             out = out->next;
          out->next = next;
          continue;
       }
       if (strcmp(outbuf, "}") == 0) {
          len -= i;
          outlen[0] += i;
          buf += i; 
       lenEaten += i;
          next = (token_t *) calloc(1, sizeof(token_t));
          if (!next)
             return NO_MEM_ERR;
          next->id = BLOCK_END;
          next->line = token.line;
          next->col = token.col;
          while(out->next)
             out = out->next;
          out->next = next;
          continue;
       }
       if (strcmp(outbuf, ";") == 0) {
          len -= i;
          outlen[0] += i;
          buf += i; 
          lenEaten += i;
          next = (token_t *) calloc(1, sizeof(token_t));
          if (!next)
             return NO_MEM_ERR;
          next->id = SEMICOLON;
          next->line = token.line;
          next->col = token.col;
          while(out->next)
             out = out->next;
          return lenEaten;
      }
    }
    return SYNTAX_ERR;
}

int parse_enum_identifier(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int i;
    int lenEaten = 0;
    token_t token;
    
    i = get_token(outbuf, buf, len, &token);
    if (i <= 0) {
       return SYNTAX_ERR;
    }
    if (strcmp(outbuf, "=") == 0) {
       len -= i;
       outlen[0] += i;
       buf += i; 
       lenEaten += i;
#if 1
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = ASSIGNMENT;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
          out = out->next;
       return parse_enum_identifier_equal(buf, len, outlen, orig);
#endif
    }
    if (strcmp(outbuf, ",") == 0) {
       len -= i;
       outlen[0] += i;
       buf += i; 
       lenEaten += i;
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = COMMA;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
          out = out->next;
       return  lenEaten; 
    }
    if (strcmp(outbuf, ";") == 0) {
       len -= i;
       outlen[0] += i;
       buf += i; 
       lenEaten += i;
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = SEMICOLON;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
          out = out->next;
       return lenEaten;
    }
    return SYNTAX_ERR;
}

int parse_typedef(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (strcmp(outbuf, "long") == 0) {
       len -= i;
       buf += i;
       lenEaten += i;
       (*outlen) += i;
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next) {
           return NO_MEM_ERR;
       }
       next->id = LONG;
       next->line = token.line;
       next->col = token.col;
       next->value = 0;
       next->next = NULL;
       while(out->next)
          out = out->next;
       out->next = next;
       i = get_token(outbuf, buf, len, &token);
       if (i <= 0)
          return -1;
       i = parse_typedef_long(buf, len, outlen, orig);
       if (i >= 0) {
          lenEaten += i;
          return lenEaten;
       }
       return i;
    }
    if (strcmp(outbuf, "enum") == 0) {
       len -= i;
       buf += i;
       outlen[0] += i;
       lenEaten += i;
       i = parse_typedef_enum(buf, len, outlen, orig);
       if (i >= 0) {
          lenEaten += i;
          outlen[0] += i;
          return lenEaten;
       }
       return i;
    } 
    if (strcmp(outbuf, "struct") == 0) {
       len -= i;
       buf += i;
       outlen[0] += i;
       lenEaten += i;
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next) {
           return -1;
       }
       next->id = STRUCT;
       next->line = token.line;
       next->col = token.col;
       next->value = 0;
       next->next = NULL;
       while(out->next)
          out = out->next;
       out->next = next;
       i = parse_typedef_struct(buf, len, outlen, orig);
       if (i <= 0)
          return SYNTAX_ERR;
       outlen[0] += i;
       lenEaten += i;
       return lenEaten;
    } 
    if (strcmp(outbuf, "union") == 0) {
       len -= i;
       buf += i;
       lenEaten += i;
       outlen[0] += i;
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next) {
           return -1;
       }
       next->id = UNION;
       next->line = token.line;
       next->col = token.col;
       next->value = 0;
       next->next = NULL;
       while(out->next)
          out = out->next;
       out->next = next;
       i = parse_typedef_union(buf, len, outlen, orig);
       if (i <= 0)
          return SYNTAX_ERR;
       outlen[0] += i;
       lenEaten += i;
       return lenEaten;
    } 
    return 0;
}

int parse_block(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int i;
    int lenEaten = 0;
    token_t token;


    i = get_token(outbuf, buf, len, &token);
    if (i <= 0) {
       return SYNTAX_ERR;
    }
    if (token.id == BLOCK_END)
      return SYNTAX_ERR; // empty block
    do {     
       i = get_token(outbuf, buf, len, &token);
       if (i <= 0) {
          return SYNTAX_ERR;
       }
       if (token.id == BLOCK_END) {
          buf += i;
          len -= i;
          outlen[0] += i;
          lenEaten += i;
          next = (token_t *) calloc(1, sizeof(token_t));
          if (next == NULL)
              return NO_MEM_ERR;
          next->id = BLOCK_END;
          next->line = token.line;
          next->col = token.col;
          while(out->next)
             out = out->next;
          out->next = next;
          return lenEaten;
       }
       i = parse_statement(buf, len, outlen, &orig);
       if (i > 0) {
          buf += i;
          len -= i;
          outlen[0] += i;
          lenEaten += i;
          continue;
       }
       return i;
     } while(len > 0);
    return SYNTAX_ERR;
}

int parse_bracket_expr(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int i;
    int lenEaten = 0;
    token_t token;


    i = get_token(outbuf, buf, len, &token);
    if (i <= 0) {
       return SYNTAX_ERR;
    }
    if (token.id == BLOCK_END)
      return SYNTAX_ERR; // empty block
    do {     
       i = get_token(outbuf, buf, len, &token);
       if (i <= 0) {
          return SYNTAX_ERR;
       }
       if (token.id == BRACKET_CLOSE) {
          buf += i;
          len -= i;
          outlen[0] += i;
          lenEaten += i;
          next = (token_t *) calloc(1, sizeof(token_t));
          if (next == NULL)
              return NO_MEM_ERR;
          next->id = BRACKET_CLOSE;
          next->line = token.line;
          next->col = token.col;
          while(out->next)
             out = out->next;
          out->next = next;
          return lenEaten;
       }
       buf += i;
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       next = (token_t *) calloc(1, sizeof(token_t));
       if (next == NULL)
           return NO_MEM_ERR;
       next->id = token.id;
       next->line = token.line;
       next->col = token.col;
       next->value = calloc(1, strlen(outbuf)+1);
       if (next->value == NULL) {
           free(next); 
           return NO_MEM_ERR;
       }
       strcpy((char *) next->value, outbuf);   
       while(out->next)
          out = out->next;
       out->next = next;
       out = next;
       continue;
     } while(len > 0);
    return SYNTAX_ERR;
}

int parse_int_identifier(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;

    while(len > 0) {
       int i = get_token(outbuf, buf, len, &token);
       if (i <= 0)
          return i;
       if (strcmp(outbuf, ",") == 0) {
          len -= i;
          outlen[0] += i;
          lenEaten += i;
          buf += i;
       
          next = (token_t *) calloc(1, sizeof(token_t));
          if (!next)
             return NO_MEM_ERR;
          next->id = COMMA;
          next->line = token.line;
          next->col = token.col;
          while(out->next)
             out = out->next;
          out->next = next;
          i = parse_int(buf, len, outlen, orig);
          if (i > 0) {
            lenEaten += i;
            return lenEaten;
          }
          return i;
       }
       if (token.id == BRACKET_OPEN) {
          // matches '('
          len -= i;
          len -= i;
          outlen[0] += i;
          lenEaten += i;
          buf += i;
       
          next = (token_t *) calloc(1, sizeof(token_t));
          if (!next)
             return NO_MEM_ERR;
          next->id = BRACKET_OPEN;;
          next->line = token.line;
          next->col = token.col;
          while(out->next)
             out = out->next;
          out->next = next;
          out = out->next;
          i = parse_bracket_expr(buf, len, outlen, next);
          if (i > 0) {
             lenEaten += i;
             buf += i;
             len -= i;
             outlen[0] += i;
             continue;
          }
          return i;
       }
       if (token.id == BRACKET_CLOSE) {
          // matches ')'
          len -= i;
          len -= i;
          outlen[0] += i;
          lenEaten += i;
          buf += i;
       
          next = (token_t *) calloc(1, sizeof(token_t));
          if (!next)
             return NO_MEM_ERR;
          next->id = BRACKET_CLOSE;
          next->line = token.line;
          next->col = token.col;
          while(out->next)
             out = out->next;
          out->next = next;
          continue;
       }
       if (token.id == IDENTIFIER || token.id == INTEGER || token.id == CHAR || token.id == SHORT || token.id == UNSIGNED || token.id == FLOAT || token.id == DOUBLE || token.id == COMMA) {
          // matches ')'
          len -= i;
          len -= i;
          outlen[0] += i;
          lenEaten += i;
          buf += i;
       
          next = (token_t *) calloc(1, sizeof(token_t));
          if (!next)
             return NO_MEM_ERR;
          next->id = token.id;
          next->line = token.line;
          next->col = token.col;
          while(out->next)
             out = out->next;
          out->next = next;
          continue;
       }
       if (token.id == SEMICOLON) {
          len -= i;
          outlen[0] += i;
          lenEaten += i;
          buf += i;
       
          next = (token_t *) calloc(1, sizeof(token_t));
          if (!next)
             return NO_MEM_ERR;
          next->id = SEMICOLON;
          next->line = token.line;
          next->col = token.col;
          while(out->next)
             out = out->next;
          out->next = next;
          return lenEaten;
       }   
       if (strcmp(outbuf, "{") == 0) {
          len -= i;
          outlen[0] += i;
          lenEaten += i;
          buf += i;
       
          next = (token_t *) calloc(1, sizeof(token_t));
          if (!next)
             return NO_MEM_ERR;
          next->id = BLOCK_BEGIN;
          next->line = token.line;
          next->col = token.col;
          while(out->next)
             out = out->next;
          out->next = next;
          out = next;
          i = parse_block(buf, len, outlen, orig);
          if (i > 0) {
             //saw a function
             lenEaten += i;
             outlen[0] += i;
             return lenEaten;
          }
       }   
       if (strcmp(outbuf, "=") == 0) {
          len -= i;
          outlen[0] += i;
          lenEaten += i;
          buf += i;
       
          next = (token_t *) calloc(1, sizeof(token_t));
          if (!next)
             return NO_MEM_ERR;
          next->id = ASSIGNMENT;
          next->line = token.line;
          next->col = token.col;
          while(out->next)
            out = out->next;
          out->next = next;
          continue;
       }   
       if (isValidNumber(outbuf)) {
          len -= i;
          outlen[0] += i;
          lenEaten += i;
          buf += i;
       
          next = (token_t *) calloc(1, sizeof(token_t));
          if (!next)
             return NO_MEM_ERR;
          next->id = NUMBER;
          next->line = token.line;
          next->col = token.col;
          next->value = (token_t *) calloc(1, strlen(outbuf)+1);
          if (!next)
             return NO_MEM_ERR;
          strcpy((char *)next->value, outbuf);
          while(out->next)
               out = out->next;
          out->next = next;
          continue;
       }   
       if (isValidKeyword(outbuf)) {
          len -= i;
          buf += i;
          lenEaten += i;
          next = (token_t *) calloc(1, sizeof(token_t));
          if (!next)
             return NO_MEM_ERR;
          next->id = token.id;
          next->line = token.line;
          next->col = token.col;
          next->value = (token_t *) calloc(1, strlen(outbuf)+1);
          if (!next)
             return NO_MEM_ERR;
          strcpy((char *)next->value, outbuf);
          while(out->next)
               out = out->next;
          out->next = next;
          continue;
       }
    }
    SYNTAX_ERR;
}

int parse_int(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (isValidId(outbuf)) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = IDENTIFIER;
       next->line = token.line;
       next->col = token.col;
       next->value = calloc(1, strlen(outbuf)+1);
       if (next->value == NULL) {
          free(next);
          return NO_MEM_ERR;
       }
       strcpy((char *) next->value, outbuf);
       while(out->next)
          out = out->next;
       out->next = next;
       i = parse_int_identifier(buf, len, outlen, orig);
       if (i > 0)
          lenEaten += i;
       return lenEaten;
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    SYNTAX_ERR;

}

int parse_long_identifier(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;
    while(len > 0) { 
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (strcmp(outbuf, ",") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = COMMA;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       i = parse_long(buf, len, outlen, orig);
       if (i <= 0)
          return i;
       lenEaten += i;
       return lenEaten;
    }   
    if (strcmp(outbuf, ";") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = SEMICOLON;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return lenEaten;
    }   
    if (strcmp(outbuf, "=") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = ASSIGNMENT;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       continue;
    }   
    if (isValidNumber(outbuf)) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = NUMBER;
       next->line = token.line;
       next->col = token.col;
       next->value = (token_t *) calloc(1, strlen(outbuf)+1);
       if (!next)
          return NO_MEM_ERR;
       strcpy((char *)next->value, outbuf);
       while(out->next)
            out = out->next;
       out->next = next;
       continue;
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    }
    SYNTAX_ERR;
}

int parse_long(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (isValidId(outbuf)) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = IDENTIFIER;
       next->line = token.line;
       next->col = token.col;
       next->value = calloc(1, strlen(outbuf)+1);
       if (next->value == NULL) {
          free(next);
          return NO_MEM_ERR;
       }
       strcpy((char *) next->value, outbuf);
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_long_identifier(buf, len, outlen, orig);
    }
    if (strcmp(outbuf, "long") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = LONG;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_long64(buf, len, outlen, orig);
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }

    SYNTAX_ERR;
}

int parse_uint_identifier(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;

    while(len > 0) {
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (strcmp(outbuf, ",") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = COMMA;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_uint(buf, len, outlen, orig);
    }   
    if (strcmp(outbuf, ";") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = SEMICOLON;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return lenEaten;
    }   
    if (strcmp(outbuf, "=") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = ASSIGNMENT;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       continue;
    }   
    if (isValidNumber(outbuf)) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = NUMBER;
       next->line = token.line;
       next->col = token.col;
       next->value = (token_t *) calloc(1, strlen(outbuf)+1);
       if (!next)
          return NO_MEM_ERR;
       strcpy((char *)next->value, outbuf);
       while(out->next)
            out = out->next;
       out->next = next;
       continue;
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    SYNTAX_ERR;
    }
}
int parse_uint(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (isValidId(outbuf)) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = IDENTIFIER;
       next->line = token.line;
       next->col = token.col;
       next->value = calloc(1, strlen(outbuf)+1);
       if (next->value == NULL) {
          free(next);
          return NO_MEM_ERR;
       }
       strcpy((char *) next->value, outbuf);
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_uint_identifier(buf, len, outlen, orig);
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    SYNTAX_ERR;
}

int parse_ulong_identifier(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;


    while(len > 0) {
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (strcmp(outbuf, ",") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = COMMA;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_ulong(buf, len, outlen, orig);
    }   
    if (strcmp(outbuf, ";") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = SEMICOLON;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return lenEaten;
    }   
    if (strcmp(outbuf, "=") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = ASSIGNMENT;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       continue;
    }   
    if (isValidNumber(outbuf)) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = NUMBER;
       next->line = token.line;
       next->col = token.col;
       next->value = (token_t *) calloc(1, strlen(outbuf)+1);
       if (!next)
          return NO_MEM_ERR;
       strcpy((char *)next->value, outbuf);
       while(out->next)
            out = out->next;
       out->next = next;
       continue;
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    }
    SYNTAX_ERR;
}
int parse_ulong(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (isValidId(outbuf)) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = IDENTIFIER;
       next->line = token.line;
       next->col = token.col;
       next->value = calloc(1, strlen(outbuf)+1);
       if (next->value == NULL) {
          free(next);
          return NO_MEM_ERR;
       }
       strcpy((char *) next->value, outbuf);
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_ulong_identifier(buf, len, outlen, orig);
    }   
    if (strcmp(outbuf, "long")) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = LONG;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_ulong64(buf, len, outlen, orig);
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    SYNTAX_ERR;
}

int parse_long64_identifier(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;

    while(len > 0) {
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (strcmp(outbuf, ",") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = COMMA;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_long64(buf, len, outlen, orig);
    }   
    if (strcmp(outbuf, ";") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = SEMICOLON;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return lenEaten;
    }   
    if (strcmp(outbuf, "=") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = ASSIGNMENT;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       continue;
    }   
    if (isValidNumber(outbuf)) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = NUMBER;
       next->line = token.line;
       next->col = token.col;
       next->value = (token_t *) calloc(1, strlen(outbuf)+1);
       if (!next)
          return NO_MEM_ERR;
       strcpy((char *)next->value, outbuf);
       while(out->next)
            out = out->next;
       out->next = next;
       continue;
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    }
    SYNTAX_ERR;
}

int parse_long64(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (isValidId(outbuf)) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = IDENTIFIER;
       next->line = token.line;
       next->col = token.col;
       next->value = calloc(1, strlen(outbuf)+1);
       if (next->value == NULL) {
          free(next);
          return NO_MEM_ERR;
       }
       strcpy((char *) next->value, outbuf);
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_long64_identifier(buf, len, outlen, orig);
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    SYNTAX_ERR;
}
int parse_ulong64_identifier(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;

    while(len > 0) {
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (strcmp(outbuf, ",") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = COMMA;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_ulong64(buf, len, outlen, orig);
    }   
    if (strcmp(outbuf, ";") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = SEMICOLON;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return lenEaten;
    }   
    if (strcmp(outbuf, "=") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = ASSIGNMENT;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       continue;
    }   
    if (isValidNumber(outbuf)) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = NUMBER;
       next->line = token.line;
       next->col = token.col;
       next->value = (token_t *) calloc(1, strlen(outbuf)+1);
       if (!next)
          return NO_MEM_ERR;
       strcpy((char *)next->value, outbuf);
       while(out->next)
            out = out->next;
       out->next = next;
       continue;
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    }
    SYNTAX_ERR;
}
int parse_ulong64(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (isValidId(outbuf)) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = IDENTIFIER;
       next->line = token.line;
       next->col = token.col;
       next->value = calloc(1, strlen(outbuf)+1);
       if (next->value == NULL) {
          free(next);
          return NO_MEM_ERR;
       }
       strcpy((char *) next->value, outbuf);
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_ulong64_identifier(buf, len, outlen, orig);
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    SYNTAX_ERR;
}

int parse_int16_identifier(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;
   
    while(len > 0) {
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (strcmp(outbuf, ",") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = COMMA;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_int16(buf, len, outlen, orig);
    }   
    if (strcmp(outbuf, ";") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = SEMICOLON;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return lenEaten;
    }   
    if (strcmp(outbuf, "=") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = ASSIGNMENT;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       continue;
    }   
    if (isValidNumber(outbuf)) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = NUMBER;
       next->line = token.line;
       next->col = token.col;
       next->value = (token_t *) calloc(1, strlen(outbuf)+1);
       if (!next)
          return NO_MEM_ERR;
       strcpy((char *)next->value, outbuf);
       while(out->next)
            out = out->next;
       out->next = next;
       continue;
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    }
    SYNTAX_ERR;
}
int parse_int16(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (isValidId(outbuf)) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = IDENTIFIER;
       next->line = token.line;
       next->col = token.col;
       next->value = calloc(1, strlen(outbuf)+1);
       if (next->value == NULL) {
          free(next);
          return NO_MEM_ERR;
       }
       strcpy((char *) next->value, outbuf);
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_int16_identifier(buf, len, outlen, orig);
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    SYNTAX_ERR;
}

int parse_uint16_identifier(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;
    while(len > 0) {
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (strcmp(outbuf, ",") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = COMMA;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_uint16(buf, len, outlen, orig);
    }   
    if (strcmp(outbuf, ";") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = SEMICOLON;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return lenEaten;
    }   
    if (strcmp(outbuf, "=") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = ASSIGNMENT;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       continue;
    }   
    if (isValidNumber(outbuf)) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = NUMBER;
       next->line = token.line;
       next->col = token.col;
       next->value = (token_t *) calloc(1, strlen(outbuf)+1);
       if (!next)
          return NO_MEM_ERR;
       strcpy((char *)next->value, outbuf);
       while(out->next)
            out = out->next;
       out->next = next;
       continue;
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    }
    SYNTAX_ERR;
}
int parse_uint16(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (isValidId(outbuf)) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = IDENTIFIER;
       next->line = token.line;
       next->col = token.col;
       next->value = calloc(1, strlen(outbuf)+1);
       if (next->value == NULL) {
          free(next);
          return NO_MEM_ERR;
       }
       strcpy((char *) next->value, outbuf);
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_uint16_identifier(buf, len, outlen, orig);
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    SYNTAX_ERR;
}
int parse_int8_identifier(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;

    do {
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (strcmp(outbuf, ",") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = COMMA;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_int8(buf, len, outlen, orig);
    }   
    if (strcmp(outbuf, ";") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = SEMICOLON;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return lenEaten;
    }
    if (strcmp(outbuf, "=") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = ASSIGNMENT;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       continue;
    }   
    if (isValidNumber(outbuf)) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = NUMBER;
       next->line = token.line;
       next->col = token.col;
       next->value = (token_t *) calloc(1, strlen(outbuf)+1);
       if (!next->value)
          return NO_MEM_ERR;
       strcpy((char *)next->value, outbuf);
       while(out->next)
            out = out->next;
       out->next = next;
       continue;
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    } while (len > 0);
    SYNTAX_ERR;
}
int parse_int8(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (isValidId(outbuf)) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = IDENTIFIER;
       next->line = token.line;
       next->col = token.col;
       next->value = calloc(1, strlen(outbuf)+1);
       if (next->value == NULL) {
          free(next);
          return NO_MEM_ERR;
       }
       strcpy((char *) next->value, outbuf);
       while(out->next)
          out = out->next;
       out->next = next;
       i = parse_int8_identifier(buf, len, outlen, orig);
       if (i > 0) {
         lenEaten += i;
         return lenEaten;
       }
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    SYNTAX_ERR;
}

int parse_uint8_identifier(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;

    while(len > 0) {
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (strcmp(outbuf, ",") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = COMMA;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_uint8(buf, len, outlen, orig);
    }   
    if (strcmp(outbuf, ";") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = SEMICOLON;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return lenEaten;
    }   
    if (strcmp(outbuf, "=") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = ASSIGNMENT;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       continue;
    }   
    if (isValidNumber(outbuf)) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = NUMBER;
       next->line = token.line;
       next->col = token.col;
       next->value = (token_t *) calloc(1, strlen(outbuf)+1);
       if (!next)
          return NO_MEM_ERR;
       strcpy((char *)next->value, outbuf);
       while(out->next)
            out = out->next;
       out->next = next;
       continue;
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    }
    SYNTAX_ERR;
}

int parse_uint8(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (isValidId(outbuf)) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = IDENTIFIER;
       next->line = token.line;
       next->col = token.col;
       next->value = calloc(1, strlen(outbuf)+1);
       if (next->value == NULL) {
          free(next);
          return NO_MEM_ERR;
       }
       strcpy((char *) next->value, outbuf);
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_uint8_identifier(buf, len, outlen, orig);
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    SYNTAX_ERR;
}

int parse_float_identifier(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;

    while(len > 0) {
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (strcmp(outbuf, ",") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = COMMA;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_float(buf, len, outlen, orig);
    }   
    if (strcmp(outbuf, ";") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = SEMICOLON;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return lenEaten;
    }   
    if (strcmp(outbuf, "=") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = ASSIGNMENT;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       continue;
    }   
    if (isValidNumber(outbuf)) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = NUMBER;
       next->line = token.line;
       next->col = token.col;
       next->value = (token_t *) calloc(1, strlen(outbuf)+1);
       if (!next)
          return NO_MEM_ERR;
       strcpy((char *)next->value, outbuf);
       while(out->next)
            out = out->next;
       out->next = next;
       continue;
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    }
    SYNTAX_ERR;
}

int parse_float(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (isValidId(outbuf)) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = IDENTIFIER;
       next->line = token.line;
       next->col = token.col;
       next->value = calloc(1, strlen(outbuf)+1);
       if (next->value == NULL) {
          free(next);
          return NO_MEM_ERR;
       }
       strcpy((char *) next->value, outbuf);
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_float_identifier(buf, len, outlen, orig);
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    SYNTAX_ERR;
}
int parse_double_identifier(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;
    while(len > 0) {
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (strcmp(outbuf, ",") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = COMMA;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_double(buf, len, outlen, orig);
    }   
    if (strcmp(outbuf, ";") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = SEMICOLON;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return lenEaten;
    }   
    if (strcmp(outbuf, "=") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = ASSIGNMENT;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       continue;
    }   
    if (isValidNumber(outbuf)) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = NUMBER;
       next->line = token.line;
       next->col = token.col;
       next->value = (token_t *) calloc(1, strlen(outbuf)+1);
       if (!next)
          return NO_MEM_ERR;
       strcpy((char *)next->value, outbuf);
       while(out->next)
            out = out->next;
       out->next = next;
       continue;
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    }
    SYNTAX_ERR;
}
int parse_double(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (isValidId(outbuf)) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = IDENTIFIER;
       next->line = token.line;
       next->col = token.col;
       next->value = calloc(1, strlen(outbuf)+1);
       if (next->value == NULL) {
          free(next);
          return NO_MEM_ERR;
       }
       strcpy((char *) next->value, outbuf);
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_double_identifier(buf, len, outlen, orig);
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    SYNTAX_ERR;
}

int parse_unsigned(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (strcmp(outbuf, "int") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = INTEGER;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_uint(buf, len, outlen, orig);
    }
    if (strcmp(outbuf, "long") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = LONG;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_ulong(buf, len, outlen, orig);
    }
    if (isValidId(outbuf)) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = IDENTIFIER;
       next->line = token.line;
       next->col = token.col;
       next->value = calloc(1, strlen(outbuf)+1);
       if (next->value == NULL) {
          free(next);
          return NO_MEM_ERR;
       }
       strcpy((char *) next->value, outbuf);
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_uint_identifier(buf, len, outlen, orig);
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    SYNTAX_ERR;
}

int parse_static_int_identifier(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (strcmp(outbuf, ",") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = COMMA;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_static_int(buf, len, outlen, orig);
    }   
    if (strcmp(outbuf, ";") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = SEMICOLON;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return lenEaten;
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    SYNTAX_ERR;
}

int parse_static_uint_identifier(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (strcmp(outbuf, ",") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = COMMA;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_static_uint(buf, len, outlen, orig);
    }   
    if (strcmp(outbuf, ";") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = SEMICOLON;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return lenEaten;
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    SYNTAX_ERR;
}
int parse_static_int(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (isValidId(outbuf)) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = IDENTIFIER;
       next->line = token.line;
       next->col = token.col;
       next->value = calloc(1, strlen(outbuf)+1);
       if (next->value == NULL) {
          free(next);
          return NO_MEM_ERR;
       }
       strcpy((char *) next->value, outbuf);
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_static_int_identifier(buf, len, outlen, orig);
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    SYNTAX_ERR;
}
int parse_static_uint(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (isValidId(outbuf)) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = IDENTIFIER;
       next->line = token.line;
       next->col = token.col;
       next->value = calloc(1, strlen(outbuf)+1);
       if (next->value == NULL) {
          free(next);
          return NO_MEM_ERR;
       }
       strcpy((char *) next->value, outbuf);
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_static_uint_identifier(buf, len, outlen, orig);
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    SYNTAX_ERR;
}

int parse_static_unsigned(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (strcmp(outbuf, "int") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = INTEGER;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_static_uint_identifier(buf, len, outlen, orig);
    }   
    if (isValidId(outbuf)) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = IDENTIFIER;
       next->line = token.line;
       next->col = token.col;
       next->value = calloc(1, strlen(outbuf)+1);
       if (next->value == NULL) {
          free(next);
          return NO_MEM_ERR;
       }
       strcpy((char *) next->value, outbuf);
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_static_uint_identifier(buf, len, outlen, orig);
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    SYNTAX_ERR;
}
int parse_static_long_identifier(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (strcmp(outbuf, ",") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = COMMA;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_static_long(buf, len, outlen, orig);
    }   
    if (strcmp(outbuf, ";") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = SEMICOLON;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return lenEaten;
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    SYNTAX_ERR;
}
int parse_static_long(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (isValidId(outbuf)) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = IDENTIFIER;
       next->line = token.line;
       next->col = token.col;
       next->value = calloc(1, strlen(outbuf)+1);
       if (next->value == NULL) {
          free(next);
          return NO_MEM_ERR;
       }
       strcpy((char *) next->value, outbuf);
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_static_long_identifier(buf, len, outlen, orig);
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    SYNTAX_ERR;
}

int parse_static(char *buf, int len, int *outlen, token_t *out)
{
    token_t *next, *orig = out;
    int lenEaten = 0;
    token_t token;
    int i = get_token(outbuf, buf, len, &token);
    if (i <= 0)
       return i;
    if (strcmp(outbuf, "int") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = INTEGER;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_static_int(buf, len, outlen, orig);
    }
    if (strcmp(outbuf, "long") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = LONG;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_static_long(buf, len, outlen, orig);
    }
    if (strcmp(outbuf, "unsigned") == 0) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = UNSIGNED;
       next->line = token.line;
       next->col = token.col;
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_static_unsigned(buf, len, outlen, orig);
    }
    if (isValidId(outbuf)) {
       len -= i;
       outlen[0] += i;
       lenEaten += i;
       buf += i;
       
       next = (token_t *) calloc(1, sizeof(token_t));
       if (!next)
          return NO_MEM_ERR;
       next->id = INTEGER;
       next->line = token.line;
       next->col = token.col;

       next->next = (token_t *) calloc(1, sizeof(token_t));
       if (!next->next) {
          free(next);
          return NO_MEM_ERR;
       }
       next->next->id = IDENTIFIER;
       next->line = token.line;
       next->col = token.col;
       next->value = calloc(1, strlen(outbuf)+1);
       if (next->value == NULL) {
          free(next);
          return NO_MEM_ERR;
       }
       strcpy((char *) next->value, outbuf);
       while(out->next)
            out = out->next;
       out->next = next;
       return parse_static_int_identifier(buf, len, outlen, orig);
    }   
    if (isValidKeyword(outbuf)) {
       return KEYWORD_ERR;
    }
    SYNTAX_ERR;
}

int parse_statement(char *buf, int len, int *outlen, token_t **outpp)
{
    int i;
    int lenEaten = 0;
    token_t token;
    token_t *stmt = *outpp, *next, *prev;

    while(len > 0) {
      i = get_token(outbuf, buf, len, &token);
      if (i <= 0) {
        if (len == 0) {
           return 0;
        }
        return SYNTAX_ERR;
      }
      if (strlen(outbuf) == 0) {
        if (len == i)
          return i;  
      }
      if (stmt == NULL) {
         stmt = (token_t *) calloc(1, sizeof(token_t));
         if (stmt == NULL)
            return NO_MEM_ERR;
         *outpp = stmt;
      }
      buf += i;
      outlen[0] += i;
      lenEaten += i;
      len -= i;
      prev = stmt;
      while(stmt->next) {
        prev = stmt;
        stmt = stmt->next;
      }
      next = NULL;
      if (stmt->id) {
        prev  = stmt;
        next =  (token_t *) calloc(1, sizeof(token_t));
        if (next == NULL) {
           return NO_MEM_ERR;
        }
        stmt->next = next;
        stmt = stmt->next;
      }
      if (strcmp(outbuf, "typedef") == 0) {
          stmt->id = TYPEDEF;
          i = parse_typedef(buf, len, outlen, stmt);
          if (i > 0) {
             lenEaten += i;
             return lenEaten;
          }
          return i;
      }
      if (strcmp(outbuf, "enum") == 0) {
          stmt->id = ENUM;
          i = parse_enum(buf, len, outlen, stmt);
          if (i > 0) {
             lenEaten += i;
             return lenEaten;
          }
          return i;
      }
      if (strcmp(outbuf, "struct") == 0) {
          stmt->id = STRUCT;
          i = parse_struct(buf, len, outlen, stmt);
          if (i > 0) {
             lenEaten += i;
             return lenEaten;
          }
          return i;
      }
      if (strcmp(outbuf, "union") == 0) {
          stmt->id = UNION;
          i = parse_struct(buf, len, outlen, stmt);
          if (i > 0) {
             lenEaten += i;
             return lenEaten;
          }
          return i;
      }
      if (strcmp(outbuf, "int") == 0) {
          stmt->id = INTEGER;
          i = parse_int(buf, len, outlen, stmt);
          if (i > 0) {
             lenEaten += i;
             return lenEaten;
          }
          return i;
      }
      if (strcmp(outbuf, "long") == 0) {
          stmt->id = LONG;
          i = parse_long(buf, len, outlen, stmt);
          if (i > 0) {
             lenEaten += i;
             return lenEaten;
          }
          return i;
      }
      if (strcmp(outbuf, "char") == 0) {
          stmt->id = CHAR;
          i = parse_int8(buf, len, outlen, stmt);
          if (i > 0) {
             lenEaten += i;
             return lenEaten;
          }
          return i;
      }
      if (strcmp(outbuf, "short") == 0) {
          stmt->id = SHORT;
          i = parse_int16(buf, len, outlen, stmt);
          if (i > 0) {
             lenEaten += i;
             return lenEaten;
          }
          return i;
      }
      if (strcmp(outbuf, "float") == 0) {
          stmt->id = FLOAT;
          i = parse_float(buf, len, outlen, stmt);
          if (i > 0) {
             lenEaten += i;
             return lenEaten;
          }
          return i;
      }
      if (strcmp(outbuf, "double") == 0) {
          stmt->id = DOUBLE;
          i = parse_double(buf, len, outlen, stmt);
          if (i > 0) {
             lenEaten += i;
             return lenEaten;
          }
          return i;
      }
      if (strcmp(outbuf, "unsigned") == 0) {
          stmt->id = UNSIGNED;
          i = parse_unsigned(buf, len, outlen, stmt);
          if (i > 0) {
             lenEaten += i;
             return lenEaten;
          }
          return i;
      }
      if (strcmp(outbuf, "static") == 0) {
          stmt->id = STATIC;
          i = parse_static(buf, len, outlen, stmt);
           if (i > 0) {
              lenEaten += i;
              return lenEaten;
           }
           return i;
       }
       stmt->id = token.id;
       stmt->value = calloc(1, strlen(outbuf) +1);
       if (stmt->value == NULL) { 
           prev->next = NULL;
           if (next)
              free(next);
           return NO_MEM_ERR;
       }
       strcpy((char *) stmt->value, outbuf); 
       if (token.id == SEMICOLON)
          return lenEaten;
    }
    return SYNTAX_ERR;
}
int parse_declaration(char *buf, int len, int *outlen, token_t **outpp)
{
    int i;
    int lenEaten = 0;
    token_t token;

    i = get_token(outbuf, buf, len, &token);
    token_t *stmt = *outpp;
    if (i <= 0) {
      if (len == 0) {
         return 0;
      }
      return SYNTAX_ERR;
    }
    if (stmt == NULL) {
       stmt = (token_t *) calloc(1, sizeof(token_t));
       if (stmt == NULL)
          return NO_MEM_ERR;
       *outpp = stmt;
    }
    buf += i;
    outlen[0] += i;
    lenEaten += i;
    len -= i;
    while(stmt->next)
      stmt = stmt->next;
    if (stmt->id) {
      stmt->next = (token_t *) calloc(1, sizeof(token_t));
      if (stmt->next == NULL) {
         return NO_MEM_ERR;
      }
      stmt = stmt->next;
    }
    if (strcmp(outbuf, "typedef") == 0) {
        stmt->id = TYPEDEF;
        i = parse_typedef(buf, len, outlen, stmt);
        if (i > 0) {
          lenEaten += i;
          return lenEaten;
        }
        return i;
    }
    if (strcmp(outbuf, "enum") == 0) {
        stmt->id = ENUM;
        i = parse_enum(buf, len, outlen, stmt);
        if (i > 0) {
          lenEaten += i;
          return lenEaten;
        }
        return i;
    }
    if (strcmp(outbuf, "struct") == 0) {
        stmt->id = STRUCT;
        i = parse_struct(buf, len, outlen, stmt);
        if (i > 0) {
          lenEaten += i;
          return lenEaten;
        }
        return i;
    }
    if (strcmp(outbuf, "union") == 0) {
        stmt->id = UNION;
        i = parse_union(buf, len, outlen, stmt);
        if (i > 0) {
          lenEaten += i;
          return lenEaten;
        }
        return i;
    }
    if (strcmp(outbuf, "int") == 0) {
        stmt->id = INTEGER;
        i = parse_int(buf, len, outlen, stmt);
        if (i > 0) {
          lenEaten += i;
          return lenEaten;
        }
        return i;
    }
    if (strcmp(outbuf, "long") == 0) {
        stmt->id = LONG;
        i = parse_long(buf, len, outlen, stmt);
        if (i > 0) {
          lenEaten += i;
          return lenEaten;
        }
        return i;
    }
    if (strcmp(outbuf, "char") == 0) {
        stmt->id = CHAR;
        i = parse_int8(buf, len, outlen, stmt);
        if (i > 0) {
          lenEaten += i;
          return lenEaten;
        }
        return i;
    }
    if (strcmp(outbuf, "short") == 0) {
        stmt->id = SHORT;
        i = parse_int16(buf, len, outlen, stmt);
        if (i > 0) {
          lenEaten += i;
          return lenEaten;
        }
        return i;
    }
    if (strcmp(outbuf, "float") == 0) {
        stmt->id = FLOAT;
        i = parse_float(buf, len, outlen, stmt);
        if (i > 0) {
          lenEaten += i;
          return lenEaten;
        }
        return i;
    }
    if (strcmp(outbuf, "double") == 0) {
        stmt->id = DOUBLE;
        i = parse_double(buf, len, outlen, stmt);
        if (i > 0) {
          lenEaten += i;
          return lenEaten;
        }
        return i;
    }
    if (strcmp(outbuf, "unsigned") == 0) {
        stmt->id = UNSIGNED;
        i = parse_unsigned(buf, len, outlen, stmt);
        if (i > 0) {
          lenEaten += i;
          return lenEaten;
        }
        return i;
    }
    if (strcmp(outbuf, "static") == 0) {
        stmt->id = STATIC;
        i =  parse_static(buf, len, outlen, stmt);
        if (i > 0) {
          lenEaten += i;
          return lenEaten;
        }
        return i;
    }
    return SYNTAX_ERR;
}

void printParsedInfo(statement_t *stmt)
{
   if (stmt == NULL) {
      printf("\rNothing to print\n");
      return;
   }
   printf("\n");
   while(stmt) {
      token_t *t = stmt->token;
      while(t) {
        switch(t->id) {
        case IDENTIFIER:
          printf("%s ", (char *) t->value);
          break;
        case CHAR:
          printf("char ");
          break;
        case INTEGER:
          printf("int ");
          break;
        case LONG:
          printf("long ");
          break;
        case UNSIGNED:
          printf("unsigned ");
          break;
        case STRUCT:
          printf("struct ");
          break;
        case UNION:
          printf("union ");
          break;
        case TYPEDEF:
          printf("typedef ");
          break;
        case SEMICOLON:
          printf(";\n");
          break;
        case COMMA:
          printf(", ");
          break;
        case BLOCK_BEGIN:
          printf("{\n");
          break;
        case BLOCK_END:
          printf("}");
          break;
        case PLUS:
          printf("+ ");
          break;
        case PLUSPLUS:
          printf("++ ");
          break;
        case MINUS:
          printf("- ");
          break;
        case MINUSMINUS:
          printf("++ ");
          break;
        case DIVISION:
          printf("/ ");
          break;
        case STAR:
          printf("* ");
          break;
        case ASSIGNMENT:
          printf("= ");
          break;
        case NUMBER:
          printf("%s ", (char *) t->value);
          break;
        case BRACKET_OPEN:
          printf("( ");
          break;
        case BRACKET_CLOSE:
          printf(") ");
          break;
        case SQUARE_OPEN:
          printf("[");
          break;
        case SQUARE_CLOSE:
          printf("] ");
          break;

        default:
          printf("Unknown %d", stmt->id);
        }
        t = t->next;
     }
     stmt = stmt->next;
   }
   printf("\n");
}

void freeToken(token_t *t)
{
   while(t) {
     token_t *next  = t->next;
     if (t->value)
        free(t->value);
     if (t->filename)
        free(t->filename);
     free(t);
     t = next;
   }
}
void freeStatement(statement_t *stmt)
{
   if (stmt == NULL) {
      printf("\rNothing to print\n");
      return;
   }
   while(stmt) {
      token_t *t = stmt->token;
      statement_t *nstmt;
      if(t) { 
        freeToken(t);
      }
      free(stmt);
      stmt = nstmt;
  } 
}

int parse_expr(char *buf, int len, statement_t **outpp)
{
  int outlen = 0, eval = 0, result;
  statement_t *orst, *stmt = *outpp, *newst = NULL;
  token_t t = {0};
  char *nextbuf = buf;

  orst = *outpp;
  while(len > 0) {
     int i = get_token(outbuf, buf, len, &t);
     if (strcmp(outbuf, "#if") == 0) {
        char *nextbuf = buf;
        i = ifAction(buf, len, stmt->filename, &nextbuf, &orst);
        if (i > 0) {
           buf += i;
           len -= i;
           continue;
        } 
     }
     if (strcmp(outbuf, "#ifdef") == 0) {
        i = ifdefAction(buf, len, stmt->filename, &nextbuf, outpp);
        if (i > 0) {
           buf += i;
           len -= i;
           continue;
        } 
     }
     if (strcmp(outbuf, "#ifndef") == 0) {
        i = ifndefAction(buf, len, stmt->filename, &nextbuf, outpp);
        if (i > 0) {
           buf += i;
           len -= i;
           continue;
        } 
     }
     if (t.id == BRACKET_OPEN) {
        buf += i;
        len -= i;
        if (stmt == NULL) {
           newst = stmt =  (statement_t *) calloc(1, sizeof(statement_t));
           if (stmt == NULL)
              return NO_MEM_ERR;
           *outpp = stmt;
        }
        orst = stmt;
        while(stmt->next)
           stmt = stmt->next;
        if (stmt->id) {
           newst = stmt->next = (statement_t *) calloc(1, sizeof(statement_t));
           if (stmt->next == NULL) {
              return NO_MEM_ERR;
           } 
           stmt = stmt->next;
        }
        stmt->token = (token_t *) calloc(1, sizeof(token_t));
        if (stmt->token == NULL) {
           if (newst)
              free(newst);
           return NO_MEM_ERR;
        }
        stmt->token[0].id = t.id;
        i = parse_bracket_expr(buf, len, &outlen, stmt->token);
        if (i > 0) {
          buf += i;
          len -= i;
          eval = eval_bracket_expr(stmt->token, outpp, &result);
        }
        continue;
     } //if BRACKET_OPEN
     if (t.id == LOGICAL_OR || t.id == LOGICAL_AND || t.id == NOT) {
        buf += i;
        len -= i;
        i = parse_expr(buf, len, outpp);
        if (i > 0) {
          int result = 0;
          buf += i;
          len -= i;
          i = eval_expr(outpp, &result);
          if (i >= 0) {
             if (t.id == LOGICAL_OR)
             {
                if (result)
                   eval = 1;
             } else if(t.id == LOGICAL_AND) {
                if (eval && result)
                   eval = 1;
                 else
                   eval = 0;
             } else if (t.id == NOT) 
                   eval = !result;
          }
          else {
             printf("Evaluation error in file %s at line %d column %d", stmt->filename, stmt->token->line, stmt->token->col);
          }
        } else {
          printf("PArse error in file %s at line %d column %d", stmt->token->filename, stmt->token->line, stmt->token->col);
        }
     }
  }
  return 0;
}

int parse(char *buf, int len, char *filename, statement_t **outpp)
{
    int outlen = 0, i;
    statement_t *root = *outpp, *stmt = *outpp, *st;
    token_t   *t;
    token_t token;

    do {
       i = get_token(outbuf, buf, len, &token);
       if (i <= 0) {
          if (i == 0 || len == 0) {
             printParsedInfo(root);
             freeStatement(root);
             return 0;
          }
          return SYNTAX_ERR;
       }
       outlen = 0;
       t = NULL;
       i = parse_statement(buf, len, &outlen, &t);
       if (i > 0 && t) {
          st = (statement_t *) calloc(1, sizeof(statement_t));
          if (st == NULL) {
             if (t) {
                freeToken(t);
             }
             break;
          }
          st->token = t;
          st->filename = (char *) malloc(strlen(filename) + 1);
          if (st->filename == NULL) {
             freeToken(st->token);
             free(st);
             *outpp = NULL;
             break;
          }
          if (root == NULL) {
             root = st;
             *outpp = st;
          }
          else
             stmt->next = st;
          stmt = st;  
          buf += i;
          len -= i;
       } else if (i < 0) {
          printf("\nError met ");
          break;
       } else
          break;
    } while(1); 
    if(root) {
        printParsedInfo(root);
        freeStatement(root);
    }
    return SYNTAX_ERR;

}

