#ifndef CONVERSION_H
#define CONVERSION_H

struct variable {
   char c_var[128];
   char ros_var[128];
   char c_presence_flag_macro[128];
   char c_presence_var[128]; // name of presence flag variable if optional
   bool c_leaf;
   bool ros_leaf;
   bool optional; // determines if this variable is valid in the current 
                  // instance
   bool c_pointer; // if it is a pointer, then optional flag in C structure
                   // determines if it is populated
   bool aset;
   bool ros_array;
   struct var *next;
};

struct node {
   char name[128];
   struct var *children;
   struct node *next;
   unsigned int count;
   unsigned int size;
   return_type_t *returnf;
};

struct expr {
   int type; // REGULAR, FOR, WHILE, DO_WHILE, IF
   unsigned long long maxval;
   unsigned long long minval;
   void *value;
   struct expr *subexpr;
   struct expr *next;
};
struct token {
   char cname[128];
   char rosname[128];
   struct token *ctok;
   struct token *rostok;
   struct node *parent;
};
#endif
