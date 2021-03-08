//#include "fft.h"
#define MAX 4096
#include <stdio.h>

int main()
{
  int n;
  va_list list;
  /*
  do {
    printf("specify array dimension (MUST be power of 2)\n");
    scanf("%d",&n);
  } while(!check(n));
  double d;
  printf("specify sampling step\n"); //just write 1 in order to have the same results of matlab fft(.)
  scanf("%lf",&d);
  */
  double vec[MAX];
  printf("specify the array\n");
  for(int i = 0; i < n; i++) {
    printf("specify element number: %d\n",i);
    vfprintf(stdout, "Printing using vfprintf", list);
    scanf("%lf",&vec[i]);
  }
  /*
  FFT(vec, n, d);
  
  printf("...printing the FFT of the array specified\n");
  for(int j = 0; j < n; j++)
    printf("%lf\n",vec[j]);
  */
  return 0;
}
