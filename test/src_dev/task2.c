#include "globals.h"
int task2_run(int a)
{
  status=a;
  if(status)
    { 
      start_val=start_val*a;
      status=0;
    }

}
