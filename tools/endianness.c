#include <stdio.h>
#include <stdint.h>

// Detects if processor is little- or big-endian
int main(){
  uint16_t i = 1;
  char *p = (char *)&i;

  if (p[0] == 1)
    printf("LITTLE\n");
  else
    printf("BIG\n");
}
