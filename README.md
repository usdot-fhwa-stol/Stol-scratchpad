# Stol-scratch
C-to-ros conversion
This is branch under-develop.
This code needs work before it can work. TAll the .c files compile clean, but there are undefined functions that need to be added. The link fail.

This software is written to demonstrate how to compile C source and create a linked list of C statements including functions.
To compile each file, do this

gcc -c <file.c>

To link with main.c do this
gcc -o main stol-utils.o stol-errors.o eval.o expr.o directive.o main.c
