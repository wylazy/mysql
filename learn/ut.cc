#include <stdio.h>

#include <ut0ut.h>
#include <ut0mem.h>

#define SIZE 1024
int main() {

  byte * block = (byte *)ut_malloc(SIZE);

  for (int i = 0; i < SIZE-1; i++) {
     block[i] = 'a' + i%26;
  }

  block[SIZE-1] = 0;
  printf("%s\n", block);
  return 0;
}
