#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "stol-utils.h"
#include "stol-errors.h"
#include "eval.h"

struct id_table *idTable[256];

int add_id(char *name, statement_t *stmt, int scope, int type, int size, int dimension, int quantity, int level, int immutable)
{
   struct id_table *next, *id = idTable[(unsigned)name[0]];

   if (id == NULL) {
      id = (struct id_table *) calloc(1, sizeof(struct id_table));
      if (id == NULL) {
         return NO_MEM_ERR;
      }
      strncpy(id->name, name, 128);
      id->scope = scope;
      if (scope == SC_LOCAL) {
         id->block = stmt->block;
      }
      id->size = size;
      id->quantity = quantity;
      id->type = type;
      id->dimension = dimension;
      id->level = level;
      id->immutable = immutable;
      idTable[(unsigned char)name[0]] = id;
      return 1;
   }
   if (find_id(name, stmt, scope)) {
      errorPrint(stmt->token, REDEFINED_ERR);
      return REDEFINED_ERR;
   }
   while(id->next)
      id = id->next;
   next = (struct id_table *) calloc(1, sizeof(struct id_table));
   if (next == NULL) {
         return NO_MEM_ERR;
   }
   strcpy(next->name, name);
   next->level = level;
   next->type = type;
   next->size = size;
   next->quantity = quantity;
   next->immutable = immutable;
   next->dimension = dimension;
   next->scope = scope;
   id->next = next;
   return 1;
}
int find_id(char *name, statement_t *stmt, int scope)
{
   struct id_table *id = idTable[(unsigned)name[0]];

   if (id == NULL)
      return 0; 
   while(id) {
      if (strncmp(id->name, name, 128) == 0) {
         if (id->scope == SC_LOCAL) {
            if (stmt->block == id->block)
               return 1;
         } else
            return 1;
      }
      id = id->next;
   }
   return 0;
}

int delete_id(char *name, statement_t *stmt, int scope)
{
   struct id_table *prev, *id = idTable[(unsigned)name[0]];
   int found = 0;

   if (id == NULL)
      return 0;
   prev = id; 
   while(id) {
      if (strncmp(id->name, name, 128) == 0) {
         if (id->scope == SC_LOCAL) {
            if (stmt->block == id->block) {
               found = 1;
               break;;
            }
         } else {
            found = 1;
            break;
         }
      }
      prev = id;
      id = id->next;
   }
   if (found == 1) {
      if (prev == id)
         idTable[(unsigned char) name[0]] = id->next;
      else
         prev->next  = id->next;
      free(id);
   }
   return found;
}

int isIdDefined(token_t *t, statement_t *stmt)
{
   if (find_id(t->value, stmt, SC_GLOBAL) || find_id(t->value, stmt, SC_LOCAL))
      return 1;
   return 0;
}

add_param();
int isFuncDef(statement_t *stmt)
{
   int bropen = 0, brclose = 0, blopen = 0, blclose = 0;
   int retid = 0, id = 0;
   token_t *t = stmt->token, *orig;

   if (t->next == NULL) {
      return 0;
   }
   orig = t;
   if (t->id == IDENTIFIER) {
       if (t->next->id == IDENTIFIER) {
           retid = 1;
           id = 1;
       } 
   }
   else if (t->id == INTEGER || 
         t->id == CHAR || t->id == LONG || t->id == INT64 || t->id == ULONG ||
         t->id == UINTEGER || t->id == UCHAR || t->id == SHORT || t->id == USHORT ||
         t->id == FLOAT || t->id == DOUBLE) {
      if (t->next->id == IDENTIFIER) {
        retid = 1;
        id = 1;
      } else {
        errorPrint(t->next, SYNTAX_ERR);
        return 0;
      }
   } else if (t->id == UNSIGNED || t->id == SIGNED) {
       token_t *next = t->next;
       if(next->id == INTEGER || 
         next->id == CHAR || next->id == LONG || next->id == INT64 ||
         next->id == SHORT || 
         next->id == FLOAT || next->id == DOUBLE)
       {
         if (!next->next) {
            errorPrint(next, SYNTAX_ERR);
            return 0;
         }
         next = next->next;
         while(next->id == STAR)
           next = next->next;
         if (!next) {
           errorPrint(next, SYNTAX_ERR);
           return 0;
         }
         
         if (next->id != IDENTIFIER) {
            errorPrint(next, SYNTAX_ERR);
            return 0;
         }
         retid = 1;
         id = 1;
     }
   }
   t = orig;
   while(t) {
     if (t->id == BRACKET_OPEN) {
        bropen++;
     } else if (t->id == BRACKET_CLOSE) {
        brclose++;
     } else if (t->id == BLOCK_BEGIN) {
        blopen++;
        break;
     }
     t = t->next;
   }
   if (bropen == 0 || bropen != brclose) {
     if(bropen < brclose) {
       errorPrint(orig, NO_BROPEN_ERR); 
     } else {
       errorPrint(orig, NO_BRCLOSE_ERR); 
     }
     return 0;
   }
   if (t == NULL || t->id != BLOCK_BEGIN)
     return 0;
   while(t->next) {
     t = t->next;
   }
   if (t->id != BLOCK_END) {
     errorPrint(t, NO_BLCLOSE_ERR);
     return 0;
   }
   return 1;
}

int isFuncDecl(statement_t *stmt)
{
   int bropen = 0, brclose = 0, blopen = 0, blclose = 0, semicolon = 0;
   int retid = 0, id = 0;
   token_t *t = stmt->token, *orig;

   orig = t;
   if (t->next == NULL) {
      return 0;
   }
   if (t->id == IDENTIFIER) {
       if (t->next->id == IDENTIFIER) {
           retid = 1;
           id = 1;
       } 
   }
   else if (t->id == INTEGER || 
         t->id == CHAR || t->id == LONG || t->id == INT64 || t->id == ULONG ||
         t->id == UINTEGER || t->id == UCHAR || t->id == SHORT || t->id == USHORT ||
         t->id == FLOAT || t->id == DOUBLE) {
      if (t->next->id == IDENTIFIER) {
        retid = 1;
        id = 1;
      } else {
        errorPrint(t->next, SYNTAX_ERR);
        return 0;
      }
   } else if (t->id == UNSIGNED || t->id == SIGNED) {
       token_t *next = t->next;
       if(next->id == INTEGER || 
         next->id == CHAR || next->id == LONG || next->id == INT64 ||
         next->id == SHORT || 
         next->id == FLOAT || next->id == DOUBLE)
       {
         if (!next->next) {
            errorPrint(next, SYNTAX_ERR);
            return 0;
         }
         next = next->next;
         while(next->id == STAR)
           next = next->next;
         if (!next) {
           errorPrint(next, SYNTAX_ERR);
           return 0;
         }
         
         if (next->id != IDENTIFIER) {
            errorPrint(next, SYNTAX_ERR);
            return 0;
         }
         retid = 1;
         id = 1;
     }
   }
   t = orig;
   while(t) {
     if (t->id == BRACKET_OPEN) {
        bropen++;
     } else if (t->id == BRACKET_CLOSE) {
        brclose++;
     } else if (t->id == BLOCK_BEGIN) {
        blopen++;
        break;
     }
     t = t->next;
   }
   if (bropen == 0 || bropen != brclose) {
     if(bropen < brclose) {
       errorPrint(orig, NO_BROPEN_ERR); 
     } else {
       errorPrint(orig, NO_BRCLOSE_ERR); 
     }
     return 0;
   }
   if (t == NULL || t->id == BLOCK_BEGIN)
     return 0;
   semicolon = 0;
   while(t->next) {
     if (semicolon == 1) {
        errorPrint(t, SYNTAX_ERR);
        return 0;
     }
     if (t->id == SEMICOLON)
       semicolon = 1;
     t = t->next;
   }
   if (semicolon == 0) {
     errorPrint(t, SYNTAX_ERR);
     return 0;
   }
   return 1;
}

int isVarDef(statement_t *stmt)
{
  token_t *t = stmt->token, *orig;

  orig = t;
  if (t == NULL || t->next == NULL || t->next->next == NULL)
    return 0;
  if (!isTypedefed(t)) {
     errorPrint(t, UNDEFINED_TYPE_ERR);
     return 0;
  }
  do {
     if (t->next == NULL || t->next->id != IDENTIFIER) {
        errorPrint(t->next, NO_IDENTIFIER_ERR);
        return 0;
     }
     if (isDeclared(t->next)) {
        errorPrint(t->next, REDEFINED_ERR);
        return 0;
     }
     if (t->next->next == NULL) {
        errorPrint(t->next, NO_SEMICOLON_ERR);
        return 0;
     }
     t = t->next->next;
     if (t->id == SEMICOLON) {
        return 1;
     }
     if (t->id == ASSIGNMENT) {
        token_t *end;
        memset(&end, 0, sizeof(end));
        if (isStructCompatible((char *) orig->value, t->next, &end)) {
           if (end->id != COMMA && end->id != SEMICOLON) {
              errorPrint(end, NO_SEMICOLON_ERR);
              return 0;
           }
           if (end->id == SEMICOLON) {
              if(end->next == NULL)
                 return 1;
              else
                 return 0;
           }
           t = end;
           //Need to process next definition of the typedef orig->id
           //TBD
           continue;
        }
          
     }
  } while(t);
}

int isVarDecl(statement_t *stmt)
{
  token_t *t = stmt->token, *orig;

  orig = t;
  if (t == NULL || t->next == NULL || t->next->next == NULL)
    return 0;
  if (t->id != EXTERN)
    return 0;
  t = t->next;
  if (!isTypedefed(t)) {
     errorPrint(t, UNDEFINED_TYPE_ERR);
     return 0;
  }
  if (t->id == UNSIGNED || t->id == SIGNED) {
     t = t->next;
     if (t->id == UINTEGER || t->id == ULONG || t->id == UCHAR || t->id == USHORT)
        return 0;
     if (t->id == LONG && t->next->id == LONG)
        t = t->next;
  }
  while(t) {
     token_t *idt;
     if (t->next == NULL || t->next->id != IDENTIFIER) {
        errorPrint(t->next, NO_IDENTIFIER_ERR);
        return 0;
     }
     idt = t->next;
     if (t->next->next == NULL) {
        errorPrint(t->next, NO_SEMICOLON_ERR);
        return 0;
     }
     t = t->next->next;
     if (t->id == SEMICOLON) {
        return 1;
     }
     if (t->id == COMMA) {
        continue;
     }
     if (t->id == ASSIGNMENT) {
        token_t *end;
        memset(&end, 0, sizeof(end));
        if (isStructCompatible((char *) idt->value, t->next, &end)) {
           if (end->id != COMMA && end->id != SEMICOLON) {
              errorPrint(end, NO_SEMICOLON_ERR);
              return 0;
           }
           if (end->id == SEMICOLON) {
              if(end->next == NULL)
                 return 1;
              else
                 return 0;
           }
           t = end;
           //Need to process next definition of the typedef orig->id
           //TBD
           continue;
        } else
           return 0;
     }
     break;
  }
  return 0;
}

int evalIntegerDef(token_t *t, statement_t **outp)
{
    token_t *orig = t, *tt;
    int error = 0;
    int depth = 0;
    int fDef = 0, fDecl = 0, vDef = 0, vDecl = 0;
    int bropen = 0, blopen = 0, brclose = 0, blclose = 0;

    if (isFuncDef(*outp))
       fDef = 1;
    if (isVarDef(*outp))
       vDef = 1;
    if (isFuncDecl(*outp))
       fDecl = 1;
    if (isVarDecl(*outp))
       vDecl = 1;
     
    while(t) {
       depth++;
       if (t->id == BRACKET_OPEN)
          bropen = 1;
       else if (t->id == BRACKET_CLOSE)
          brclose = 1;
       else if (t->id == BLOCK_BEGIN)
          blopen = 1;
       else if (t->id == BLOCK_END)
          blclose = 1;
       t = t->next;
    }
    t = orig;
    tt = (token_t *) malloc(depth *sizeof(token_t));
    if (tt == NULL) {
       printf("\nNo memory allocating token tables in %s", __func__);
       return NO_MEM_ERR;
    }
    for (int i = 0; i < depth ; ) {
      tt[i] = t[0];
      t = t->next;
      i++;
    }

    if (bropen == 0 && brclose == 0) {
      if (blopen == 0 && blclose == 0)
        return evalIntVar(*outp); //Definitely a variable declaration
      while(t) {
        if (t->id == BLOCK_BEGIN) {
           errorPrint(t, NO_BLOPEN_ERR);
           return SYNTAX_ERR;
        }
        else if (t->id == BLOCK_END) {
           errorPrint(t, NO_BLCLOSE_ERR);
           return SYNTAX_ERR;
        }
        t = t->next;
      }
    } else if (bropen == 1 && brclose == 1) {
      if (blopen == 1) {
         if(blclose == 1) {
            evalIntFuncDef(*outp);
         } else {
            errorPrint(t, NO_BLCLOSE_ERR);
            return SYNTAX_ERR;
         }
      } else if (blclose == 1) {
            errorPrint(t, NO_BLOPEN_ERR);
            return SYNTAX_ERR;
      }
      evalFuncDecl(*outp);
    } else if (bropen) {
      errorPrint(t, NO_BRCLOSE_ERR);
      return SYNTAX_ERR;
    }
    errorPrint(t, NO_BROPEN_ERR);
    return SYNTAX_ERR;
} 

int evalUnsignedDef(token_t *t, statement_t **outp)
{
    token_t *orig = t, *tt;
    int error = 0;
    int depth = 0;
    int fDef = 0, fDecl = 0, vDef = 0, vDecl = 0;
    int bropen = 0, blopen = 0, brclose = 0, blclose = 0;

    if (isFuncDef(*outp))
       fDef = 1;
    if (isVarDef(*outp))
       vDef = 1;
    if (isFuncDecl(*outp))
       fDecl = 1;
    if (isVarDecl(*outp))
       vDecl = 1;
     
    while(t) {
       depth++;
       if (t->id == BRACKET_OPEN)
          bropen = 1;
       else if (t->id == BRACKET_CLOSE)
          brclose = 1;
       else if (t->id == BLOCK_BEGIN)
          blopen = 1;
       else if (t->id == BLOCK_END)
          blclose = 1;
       t = t->next;
    }
    t = orig;
    tt = (token_t *) malloc(depth* sizeof(token_t));
    if (tt == NULL) {
       printf("\nNo memory allocating token tables in %s", __func__);
       return NO_MEM_ERR;
    }
    for (int i = 0; i < depth ; ) {
      tt[i] = t[0];
      t = t->next;
      i++;
    }

    if (bropen == 0 && brclose == 0) {
      if (blopen == 0 && blclose == 0)
        return evalUnsignedVar(*outp); //Definitely a variable declaration
      while(t) {
        if (t->id == BLOCK_BEGIN) {
           errorPrint(t, NO_BLOPEN_ERR);
           return SYNTAX_ERR;
        }
        else if (t->id == BLOCK_END) {
           errorPrint(t, NO_BLCLOSE_ERR);
           return SYNTAX_ERR;
        }
        t = t->next;
      }
    } else if (bropen == 1 && brclose == 1) {
      if (blopen == 1) {
         if(blclose == 1) {
            evalUnsignedFuncDef(*outp);
         } else {
            errorPrint(t, NO_BLCLOSE_ERR);
            return SYNTAX_ERR;
         }
      } else if (blclose == 1) {
            errorPrint(t, NO_BLOPEN_ERR);
            return SYNTAX_ERR;
      }
      evalFuncDecl(*outp);
    } else if (bropen) {
      errorPrint(t, NO_BRCLOSE_ERR);
      return SYNTAX_ERR;
    }
    errorPrint(t, NO_BROPEN_ERR);
    return SYNTAX_ERR;
} 

int evalCharDef(token_t *t, statement_t **outp)
{
    token_t *orig = t, *tt;
    int i, error = 0;
    int depth = 0;
    int fDef = 0, fDecl = 0, vDef = 0, vDecl = 0;
    int bropen = 0, blopen = 0, brclose = 0, blclose = 0;
    statement_t *stmt = *outp;

    if (isFuncDef(stmt))
       fDef = 1;
    if (isVarDef(stmt))
       vDef = 1;
    if (isFuncDecl(stmt))
       fDecl = 1;
    if (isVarDecl(stmt))
       vDecl = 1;
     
    while(t) {
       depth++;
       if (t->id == BRACKET_OPEN)
          bropen = 1;
       else if (t->id == BRACKET_CLOSE)
          brclose = 1;
       else if (t->id == BLOCK_BEGIN)
          blopen = 1;
       else if (t->id == BLOCK_END)
          blclose = 1;
       t = t->next;
    }
    t = orig;
    tt = (token_t *) malloc(depth * sizeof(token_t));
    if (tt == NULL) {
       printf("\nNo memory allocating token tables in %s", __func__);
       return NO_MEM_ERR;
    }
    for (i = 0; i < depth ; ) {
      tt[i] = t[0];
      t = t->next;
      i++;
    }

    if (bropen == 0 && brclose == 0) {
      if (blopen == 0 && blclose == 0)
        return evalCharVar(stmt); //Definitely a variable declaration
      while(t) {
        if (t->id == BLOCK_BEGIN) {
           errorPrint(t, NO_BLOPEN_ERR);
           return SYNTAX_ERR;
        }
        else if (t->id == BLOCK_END) {
           errorPrint(t, NO_BLCLOSE_ERR);
           return SYNTAX_ERR;
        }
        t = t->next;
      }
    } else if (bropen == 1 && brclose == 1) {
      if (blopen == 1) {
         if(blclose == 1) {
            i = evalCharFuncDef(stmt);
            return i;
         } else {
            errorPrint(t, NO_BLCLOSE_ERR);
            return SYNTAX_ERR;
         }
      } else if (blclose == 1) {
            errorPrint(t, NO_BLOPEN_ERR);
            return SYNTAX_ERR;
      }
      evalFuncDecl(stmt);
    } else if (bropen) {
      errorPrint(t, NO_BRCLOSE_ERR);
      return SYNTAX_ERR;
    }
    errorPrint(t, NO_BROPEN_ERR);
    return SYNTAX_ERR;
} 


int abcd(token_t *orig, statement_t *stmt)
{
    token_t *t = orig, *tt;
    int i, error = 0, successive = 0, depth;

    depth = 0;
    for (;t; t = t->next) {
      depth++;
    } 
    t = orig;
    tt = (token_t *) malloc(depth * sizeof(token_t));
    if (tt == NULL) {
       return NO_MEM_ERR;
    }
    for (i = 0; i < depth; i++) {
       tt[i] = *t;
       t = t->next;
    } 

    for (i = 0; i < depth-1; i++) {
      if (tt[i].id == LONG) {
        if(tt[i+1].id == LONG) {
          tt[i].id = INT64;
          for (int j = i+1; j < depth-1; j++)
              tt[j] = tt[j+1];
          depth--;
        }
      }
    }
    for (i = 0; i < depth-1; i++) {
      if (tt[i].id == UNSIGNED) {
        if(tt[i+1].id == CHAR || tt[i+1].id == LONG || tt[i+1].id == INTEGER || tt[i+1].id == INT64) {
          if(tt[i+1].id == CHAR)
            tt[i].id = UCHAR;
          else if (tt[i+1].id == INTEGER)
            tt[i].id = UINTEGER;
          else if (tt[i+1].id == LONG)
            tt[i].id = ULONG;
          else
            tt[i].id = UINT64;
          for (int j = i+1; j < depth-1; j++)
              tt[j] = tt[j+1];
          depth--;
        } else
          tt[i].id = UINTEGER;
      }
    }
    if (tt[0].id == IDENTIFIER || tt[0].id == INTEGER ||
          tt[0].id == UINTEGER || tt[0].id == LONG || tt && tt[1].id == BRACKET_OPEN) {
      i = 2;
      int fault = 0;
      while (i < depth) {
         if (tt[i].id == BRACKET_CLOSE) {
            if (successive) {
               successive--;
               continue;
            }
            break;
         }
         if (tt[i].id == BRACKET_OPEN) {
           successive++;
           continue;
         }
         if (tt[i].id == IDENTIFIER) {
           if (i < depth -1 && tt[i+1].id == IDENTIFIER) {
             if (i < depth-2 && (tt[i+2].id == COMMA || 
                 tt[i].id == BRACKET_CLOSE)) {
                i++;
             }
          }
        }
      }
    }
    if (t->id == IDENTIFIER && 
           (t->next && t->next->id == BRACKET_OPEN) && 
           (t->next->next && t->next->next->id == BRACKET_CLOSE)) {
        char *name = (char *) malloc(strlen((char *) t->value)+1);
        if (name == NULL) {
           return NO_MEM_ERR;
        }
        strcpy(name, (char *) t->value);
        token_t *next = t->next;
        if (next->id == BRACKET_OPEN) {
           int found = 0;
           int successive = 0;
           while(next->next) {
              if (next->id == BRACKET_OPEN) {
                  successive++;
                  next = next->next;
                  continue;
              }
              if (next->id == BRACKET_CLOSE) {
                  if(successive) {
                    successive--;
                    next = next->next;
                    continue;
                 }
                 found = 1;
                 break;
              }
              if (next->id == IDENTIFIER) {
                 token_t *n = find_identifier((char *) next->value, stmt, SC_LOCAL);
                 if (n) {
                    printf("\nRedefinition of prameter %s in function %s", (char *) next->value, name);
                    error = 1;
                 } else
                    add_param(next, stmt, SC_LOCAL);
              }
            }
        }
    }
}

int isFuncPointerDefined(token_t *t, statement_t *stmt)
{
    int index = ((unsigned char *)(t->value))[0];
    struct id_table *id = idTable[index];

    while(id) {
       if (id->type == TYPE_FUNCTION_POINTER) {
          if (strcmp((char *) t->value, id->name) == 0) {
             return 1;
          }
       }
       id = id->next;
    }
    return 0;  
}

int evalExprDef(token_t *t, statement_t *stmt)
{
   int i;
   int bracket_open = 0;
   token_t *end, *idt = NULL, *parmt, *prev;
   token_t *tdeft;

   if (!isTypedefed(t)) {
     //Check case:
     //  func(...){}
     if ((i = evalFuncDef(stmt)) <= 0) {
        errorPrint(t, UNDEFINED_TYPE_ERR);
        return 0;
     }
     return i;
   }
   tdeft = t;
   prev = t;
   if (t->next == NULL) {
     errorPrint(t, NO_IDENTIFIER_ERR);
     return NO_IDENTIFIER_ERR;
   }
   t = t->next;
   while(t) {
     int pointer_level = 0;
     int function_pointer_level = 0;

     while (t) {
       if (t->id == STAR) {
          pointer_level++;
          prev = t;
          t = t->next;
          continue;
       }
       break;
     }
     if (t == NULL) {
       errorPrint(prev, NO_IDENTIFIER_ERR);
       return NO_IDENTIFIER_ERR;
     }
     if(t->id == BRACKET_OPEN) {
       //case int (*name)();
       bracket_open++;
       if (t->next == NULL) {
          errorPrint(t, NO_BRCLOSE_ERR);
          return NO_BRCLOSE_ERR;
       }
       prev = t;
       t = t->next;
        
       while (t) {
          if (t->id == STAR) {
             function_pointer_level++;
             t = t->next;
          }
          break;
       }
       if (t == NULL) {
          //case int (*;
          errorPrint(prev, NO_IDENTIFIER_ERR);
          return NO_IDENTIFIER_ERR;
       }
       if (t->id != IDENTIFIER ) {
          //case int (*name)()
          errorPrint(t, NO_IDENTIFIER_ERR);
          return NO_IDENTIFIER_ERR;
       }
       idt = t;
       t = t->next;
       if (t == NULL || t->id != BRACKET_CLOSE) {
         //case int (*name)
         errorPrint(idt, NO_BRCLOSE_ERR);
         return NO_IDENTIFIER_ERR;
       }
       prev = t;
       t = t->next;
      
       if (t == NULL || t->id != BRACKET_OPEN) {
         //case int (*)(
         errorPrint(prev, NO_BRCLOSE_ERR);
         return NO_IDENTIFIER_ERR;
       }
       prev = t;
       end = t;
       parmt = t;
       i = evalFuncParamDecl(t, &end);
       if (i <= 0) {
         return i;
       }
       t = end;
       if (t == NULL || t->id != SEMICOLON) {
         errorPrint(prev, NO_SEMICOLON_ERR);
         return NO_SEMICOLON_ERR;
       }

       if (isFuncPointerDefined(idt, stmt)) {
         errorPrint(prev, REDEFINED_ERR);
         return REDEFINED_ERR;
       }
       i = defineFuncPointer(tdeft, idt, parmt, pointer_level, function_pointer_level);
       if (i < 0) {
         return i;
       } 
       return i;
     } else if (t->id == IDENTIFIER) {
       idt = t;
       prev = t;
       t = t->next;
       if (t == NULL) {
          errorPrint(prev, NO_SEMICOLON_ERR);
          return NO_SEMICOLON_ERR;
       }
       if (t->id == ASSIGNMENT) {
          if (isIdDefined(idt, stmt)) {
            errorPrint(idt, REDEFINED_ERR);
            return REDEFINED_ERR;
          }
          prev = t;
          t = t->next;
          parmt = t;
          i = evalParam(tdeft, idt, parmt, pointer_level, &end);
          if (i < 0) {
            return i;
          } 
          i = defineId(tdeft, idt, parmt, pointer_level);
          if (i < 0) {
            return i;
          }
          t = end;
       }
       else if (t->id == SQUARE_OPEN) {

       }
       if (t->id == COMMA) {
          t = t->next;
       }
     } else {
       errorPrint(t, NO_IDENTIFIER_ERR);
       return NO_IDENTIFIER_ERR;
     }
   }
   if (t) {
      errorPrint(t, NO_SEMICOLON_ERR);
      return NO_SEMICOLON_ERR;
   }
   return 1;
}

int evaluateStatement(statement_t *stmt)
{
   token_t *t = stmt->token;
   statement_t *out;
   int i;

   out = stmt;
   if (t) {
     if (t->id == INTEGER) {
         i = evalIntegerDef(t, &out);
         if (i < 0) {
            printf("\nError in file %s at line %d:%d", out->filename, t->line, t->col);
         }
          return i;
     }
     if (t->id == UNSIGNED) {
         i = evalUnsignedDef(t, &out);
         if (i < 0) {
            printf("\nError in file %s at line %d:%d", out->filename, t->line, t->col);
         }
         return i;
     }
     if (t->id == LONG) {
         i = evalLongDef(t, out);
         if (i < 0) {
            printf("\nError in file %s at line %d:%d", out->filename, t->line, t->col);
         }
         return i;
     }
     if (t->id == BLOCK_BEGIN) {
         i = evalBlockDef(t, out);
         if (i < 0) {
            printf("\nError in file %s at line %d:%d", out->filename, t->line, t->col);
         }
         return i;
     }

     if (t->id == BRACKET_OPEN) {
         //statement cannot begin with a bracket
         errorPrint(t, BROPEN_ERR);
         return 0;
     }
     if (t->id == IDENTIFIER) { // A case where user-defined types are starting
                                // statement
         i = evalExprDef(t, out);
         if (i < 0) {
            printf("\nError in file %s at line %d:%d", out->filename, t->line, t->col);
         }
         return i;
     }
   }
   return -1; //SYNTAX_ERR
}
int eval_expr(statement_t **stmt, int *result)
{
   int i;
   token_t *t = stmt[0]->token;

   if (!t)
      return 0;
   if (t->id == INTEGER || t->id == LONG || t->id == UINTEGER || 
        t->id == ULONG || t->id == CHAR || t->id == UCHAR || t->id == FLOAT ||
        t->id == DOUBLE || t->id == IDENTIFIER) {
      i = evalTypedef(t, stmt);
      *result = i;
      return i;
   }
   return 0;
}
int eval_bracket_expr(token_t *t, statement_t **stmt, int *result)
{
   int i;
   if (!t)
      return 0;
   if (t->next == NULL) {
      if (t->id == IDENTIFIER) {
         if (isIdDefined(t->value, *stmt)) {

         }
      }
   }
   if (t->id == INTEGER || t->id == LONG || t->id == UINTEGER || 
        t->id == ULONG || t->id == CHAR || t->id == UCHAR || t->id == FLOAT ||
        t->id == DOUBLE || t->id == IDENTIFIER) {
      i = evalTypedef(t, stmt);
      *result = i;
      return i;
   }
   return 0;
}
int evaluate(statement_t *root)
{
   statement_t *stmt = root;

   while(stmt) {
      evaluateStatement(stmt);
      stmt = stmt->next;
   } 
}

