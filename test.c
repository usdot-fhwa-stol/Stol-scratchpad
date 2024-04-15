#include<stdio.h>

int main()
{
  struct test {
    int i;
    struct inner {
      int j;
    } histest;
  } mytest;
  int inner() {
     struct test {
       int i;
       struct inner {
         int j;
       } histest;
     } mytest;
     mytest.histest.j = 2;
     printf("Calling inner function %d\n", mytest.histest.j);
  }
  inner();
  mytest.i  =1;
  mytest.histest.j  =1;
  printf("My int %d, %d", mytest.i, sizeof("myname"));
  return 0;
}
