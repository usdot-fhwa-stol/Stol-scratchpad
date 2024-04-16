# Stol-scratch
C-to-C conversion
This program is meant to show a proof of concept of wrting a parser of a program
written in C to another language. As of now, this parser tokenize the code, parses the code and stores in a limked list. The linked list can be then evaluated to C language (or another language).
To compile the code, do the following in a terminal:
1. gcc -c stol-utils.c
2. gcc stol-utils.o main.c
