/*
  IBI Linux Crypter. JIT Crypter PoC
  Copyright (c) 2016 picoFlamingo

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
