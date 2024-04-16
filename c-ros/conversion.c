#include "conversion.h"

#define EMIT_VAR(c_var, ros_var, c_code, ros_code) \
    do { \
       struct expr *next = (struct expr *) calloc(1, sizeof(struct expr)); \
       if (next == NULL) { \
          exit(-1); \
       }\
       expr->value = (void *) malloc(strlen(c_var) + 2);
       if (expr->value == NULL) {
          exit(-1);
       }
       sprintf((char *) expr->value, ".%s", c_var); \
       if (c_code == NULL) \
          c_code = next; \
       else { \
          struct expr *exp = c_code; \
          while(exp->next) { \
             exp = exp->next; \
          } \
          exp->next = next; \

       }
       struct expr *next = (struct expr *) calloc(1, sizeof(struct expr)); \
       if (next == NULL) { \
          exit(-1); \
       }\
       expr->value = (void *) malloc(strlen(c_var) + 2);
       if (expr->value == NULL) {
          exit(-1);
       }
       sprintf((char *) expr->value, ".%s", ros_var); \
       if (ros_code == NULL) \
          ros_code = next; \
       else { \
          struct expr *exp = ros_code; \
          while(exp->next) { \
             exp = exp->next; \
          } \
          exp->next = next; \
       } \
    } while(0); 

char dir[1024];

int create_function(FILE *cfile, struct node **start, bool ros_to_c)
{
    struct node *next = *start;
    struct variable *var = start->children;
    struct expr *c_code = NULL, *ros_code = NULL;
    struct expr *function;
    char filename[1024];
    var = next->children;
 
    
    while(var ) {
       if (var->leaf) {
          if (var->next) {
             // program error
             if (c_code)
                free_code(c_code);
             if (ros_code)
                free_code(ros_code);
             return -1;
          }
          //Emit the final code here
          EMIT_VAR(var->c_var, var->ros_var, c_code, ros_code); 
       }
       if (strlen(var->c_presence_var)) {
          do_presence(var, &c_code, &ros_code);
          var = var->next;
          continue; 
       }
       EMIT_VAR(var->c_var, var->ros_var, c_code, ros_code); 
       var = var->next;
    }
    //uper_encode() ;

    if (ros_to c) {
       fprintf(fout, "%s", (char *) c_code->value);
       struct expr *exp = c_code->next;
       while(exp) {
          fprintf(fout, ".%s", (char *) exp->value);
          exp = exp->next; 
       }
       fprintf(fout, " = %s", (char *) ros_code->value);
       exp = ros_code->next; 
       while(exp) {
          fprintf(fout, ".%s", (char *) exp->value);
          exp = exp->next; 
       }
    } else {
       fprintf(fout, "%s", (char *) ros_code->value);
       exp = ros_code->next; 
       while(exp) {
          fprintf(fout, ".%s", (char *) exp->value);
          exp = exp->next; 
       }
       fprintf(fout, " = %s", (char *) c_code->value);
       struct expr *exp = c_code->next;
       while(exp) {
          fprintf(fout, ".%s", (char *) exp->value);
          exp = exp->next; 
       }
    }
    fprintf(fout, ";\n");
    close_function(next);
    next = next->next;
    fclose(cfile);
    cfile = NULL;
    //Create .h file here to declare this function
    return 1;
}

int create_for(FILE *cfile, struct node **start, bool ros_to_c, int indent_step)
{
    struct node *next = *start;
    struct variable *var = start->children;
    struct expr *c_code = NULL, *ros_code = NULL;
    struct expr *function;
    char filename[1024];
    var = next->children;
 
    for (int i = 0; i < ident_count; i++) {
       fprintf(fout,   " ");
    }
    fprintf(fout, "for (");
    for (i = 0; i < 3 && next; i++) {
       var = next->children;
       while(var ) {
          if (var->leaf) {
             if (var->next) {
                // program error
                if (c_code)
                   free_code(c_code);
                if (ros_code)
                   free_code(ros_code);
                return -1;
             }
             //Emit the final code here
             EMIT_VAR(var->c_var, var->ros_var, c_code, ros_code); 
          }
       }
       next = next->next;
   } 
   fprintf(fout, "( {\n");
   indent_count += INDENT_STEP;
   while(next) {
       struct variable *var = next->children;

       if (strlen(var->c_presence_var)) {
          do_presence(var, &c_code, &ros_code);
          var = var->next;
          continue; 
       }
       EMIT_VAR(var->c_var, var->ros_var, c_code, ros_code); 
       var = var->next;
    }
    //uper_encode() ;

    if (ros_to c) {
       fprintf(fout, "%s", (char *) c_code->value);
       struct expr *exp = c_code->next;
       while(exp) {
          fprintf(fout, ".%s", (char *) exp->value);
          exp = exp->next; 
       }
       fprintf(fout, " = %s", (char *) ros_code->value);
       exp = ros_code->next; 
       while(exp) {
          fprintf(fout, ".%s", (char *) exp->value);
          exp = exp->next; 
       }
    } else {
       fprintf(fout, "%s", (char *) ros_code->value);
       exp = ros_code->next; 
       while(exp) {
          fprintf(fout, ".%s", (char *) exp->value);
          exp = exp->next; 
       }
       fprintf(fout, " = %s", (char *) c_code->value);
       struct expr *exp = c_code->next;
       while(exp) {
          fprintf(fout, ".%s", (char *) exp->value);
          exp = exp->next; 
       }
    }
    fprintf(fout, ";\n");
    indent_step -= INDENT_STEP;
    for (i = 0 ;i < indent_count; i++)
       fprintf(fout, " ");
    fprintf(fout, "}\n");
    *start  = next
    fclose(cfile);
    cfile = NULL;
    return 1;
}

int emit_code(struct node *start, bool ros_to_c)
{
    struct node *next = start;
    struct variable *var = start->children;
    struct expr *c_code = NULL, *ros_code = NULL;
    struct expr *function;
    char filename[1024];
    int num_indent = 0;

    while(next) {
       sprintf(filename, "%s/%s.c", dir, start->name);
       FILE *cfile = fopen(filename, "a+");

       if (cfile == NULL) {
          fprintf(stderr, "could not create file %s", filename);
          return -1.
       }

       // SensorDataSharingMessage( ...) 
       if (next->type == FUNCTION_DEF) {
          create_function(cfile, &next, ros_to_c);  
       } else if (start->type == FOR_DEF) {
          create_for(cfile, &next, ros_to_c);
          continue;
       } else if (start->type == WHILE_DEF) {
          create_while(cfile, &next, ros_to_c);
          continue;
       } else if (start->type == DO_DEF) {
          create_do(cfile, &next, ros_to_c);
          continue;
       } else if (start->type == IF_DEF) {
          create_if(cfile, &ifstatement, &next);
          continue;
       }
    }
    return 0;
}
