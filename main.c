#include <stdio.h>
#include <string.h>
#include "stol-utils.h"

//The following is a repesentation of C code in a file.
//For easiness and for testing purposes, instead of opening a C file and
//loading it into memory, this static string makes life simpler.
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

   //Start the parse of the loaded file
   parse(file, strlen(file));
   return 0;
}

