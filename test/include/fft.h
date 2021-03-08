#define M_PI 3.1415926535897932384
#define MAX 200

int intlog(int N)    /*function to calculate the log2(.) of int numbers*/
{
  int k = N, i = 0;
  while(k) {
    k >>= 1;
    i++;
  }
  return i - 1;
}

int check(int n)    //checking if the number of element is a power of 2
{
  return n > 0 && (n & (n - 1)) == 0;
}

int reverse(int N, int n)    //calculating revers number
{
  int j, p = 0;
  for(j = 1; j <= intlog(N); j++) {
    if(n & (1 << (intlog(N) - j)))
      p |= 1 << (j - 1);
  }
  return p;
}

void ordina(double* f1, int N) //using the reverse order in the array
{
  double f2[MAX];
  for(int i = 0; i < N; i++)
    f2[i] = f1[reverse(N, i)];
  for(int j = 0; j < N; j++)
    f1[j] = f2[j];
}

void transform(double* f, int N);
void FFT(double* f, int N, double d);
