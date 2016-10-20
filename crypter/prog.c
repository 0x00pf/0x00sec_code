/*
  Simple Linux Crypter
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

/*************************************************************
  Read the post at 0x00sec.org
  https://0x00sec.org/t/a-simple-linux-crypter/537
**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <elf.h> 

#define KEY_MASK 0x7
static char key[8] ="ABCDEFGH";

#define DIE(s) {perror(s);exit(1);}
#define DEFAULT_EP ((unsigned char*)0x400000)

#define CRYPT_ME __attribute__((section(".secure")))

void
xor_block (unsigned char *data, int len)
{
  int i;

  for (i = 0; i < len; data[i] ^= key[i & KEY_MASK], i++);
}


CRYPT_ME int
secure_main (int argc, char *argv[])
{
  printf ("This was crypted!\n");
  getchar ();

  return 0;
}

void
uncrypt ()
{
  int   p   = *((int *)(DEFAULT_EP + 0x09));
  int   len = *((short *)(DEFAULT_EP + 0xd));

  /* Change Permissions */
  unsigned char *ptr       = DEFAULT_EP + p;
  unsigned char *ptr1      = DEFAULT_EP + p + len;
  size_t         pagesize  = sysconf (_SC_PAGESIZE);
  uintptr_t      pagestart = (uintptr_t)ptr & -pagesize;
  int            psize     = (ptr1 - (unsigned char*)pagestart);

  /* Make the pages writable...*/
  if (mprotect ((void*)pagestart, psize, PROT_READ | PROT_WRITE | PROT_EXEC) < 0)
    DIE ("mprotect:");

  xor_block (DEFAULT_EP + p, len);

  if (mprotect ((void*)pagestart, psize, PROT_READ | PROT_EXEC) < 0)     
    DIE ("mprotect:");

  printf ("+ Ready to run!\n");
}

int
main (int argc, char *argv[])
{
  uncrypt ();
  secure_main (argc, argv);
  
  return 0;
}
