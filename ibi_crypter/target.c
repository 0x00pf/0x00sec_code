#include <stdio.h>
#include "stub.h"

#define CRYPT_ME __attribute__((section(".secure")))

// This function is crypted
CRYPT_ME int
check_key (unsigned char *str)
{
  int  i;
  unsigned char  *p = str;

  while (*p) {*p -= '0'; p++;};
  if (str[0] + str[1] != 5) return 1;
  if (str[2] * str[3] != 10) return 1;
  return 0;
}


int
main (int argc, char*argv[])
{
  _stub (check_key); // Setup run environment
  printf ("Code is %s\n", check_key (argv[1]) ? "INCORRECT": "CORRECT");
}
