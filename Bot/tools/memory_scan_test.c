#include <unistd.h>
#include <stdio.h>

/*
This program is used to test the scanning functionality i the sanitizer module of AntibIoTic.
The buffer that is allocated should be present somewhere in the executable, so the sanitizer will be able search for this value.
Note that the compiler may not store the buffer in one long sequence, so check if the binary contains the search sequence.
*/

int main(int argc, char **args){
  char buf[] = "\x0C\x43\x4C\x4B\x4F\x47\x22";
  printf("PID: %d\n", getpid());
  getchar();
}
