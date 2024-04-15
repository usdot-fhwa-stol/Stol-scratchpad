#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stol-utils.h"
#include "stol-errors.h"

char *file = "char t1 = 0;\n" \
                 "struct myfirst { \n" \
                 " char t1;\n" \
                 " int t2;\n" \
                 " struct myfirst *next;\n" \
                 "};\n" \
             "int t2 = 0.1;\n" \
             "int add(int t1, int t2) { \n" \
             "     t2 = t1 + t2;\n" \
             "     return t2;\n" \
             "}\n";


struct compile comp;
int main()
{
   statement_t *stmt = NULL;

   stmt = (statement_t *) calloc(1, sizeof(statement_t));

   if (stmt == NULL) {
      return NO_MEM_ERR;
   }
   stmt->filename = (char *) malloc(32);
   if (stmt->filename == NULL) {
      free(stmt);
      return NO_MEM_ERR;
   }
   strcpy(stmt->filename, "main.c");
   printf("File size: %lu", strlen(file));
   memset(&comp, 0, sizeof(comp));
   parse(file, strlen(file), "main.c", &stmt);
   return 0;
}

