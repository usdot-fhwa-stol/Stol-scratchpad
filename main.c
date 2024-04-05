#include <stdio.h>
#include <string.h>
#include "stol-utils.h"
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


int main()
{
   printf("File size: %lu", strlen(file));
   parse(file, strlen(file));
   return 0;
}

