#include<stdio.h>
#include "globals.h"

int add_pointerval(int a, int *b)
{
   int *p,*q;
   if(a) p=b;
   else p=&a;
   q=b;
   *b=*p+*q;
   return *b;
}

int add_pointerval2(int a)
{
  int *p,*q;
  p=&a;
  q=p;
  return *p+*q;
}


int task_run(int a)
{

  int b=start_val;
  int sum1=add_pointerval(a,&b);
  int sum2=add_pointerval2(&a);
  return sum1+sum2;
}
