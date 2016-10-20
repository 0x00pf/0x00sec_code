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

/* Helper functions */
static int 
get_file_size (int fd)
{
  struct stat _info;
  
  fstat (fd, &_info);

  return _info.st_size;
}

Elf64_Shdr *
elfi_find_section (void *data, char *name)
{
  char        *sname;
  int         i;
  Elf64_Ehdr* elf_hdr = (Elf64_Ehdr *) data;
  Elf64_Shdr *shdr    = (Elf64_Shdr *)(data + elf_hdr->e_shoff);
  Elf64_Shdr *sh_strtab = &shdr[elf_hdr->e_shstrndx];
  const char *const sh_strtab_p = data + sh_strtab->sh_offset;
   
  printf ("+ %d section in file. Looking for section '%s'\n", 
	  elf_hdr->e_shnum, name);
  
  for (i = 0; i < elf_hdr->e_shnum; i++)
    {
      sname = (char*) (sh_strtab_p + shdr[i].sh_name);
      if (!strcmp (sname, name))  return &shdr[i];
    }
  
  return NULL;
}

void
xor_block (unsigned char *data, int len)
{
  int i;
  for (i = 0; i < len; data[i] ^= key[i & KEY_MASK], i++);
}

void
encode (char *f)
{
  int            fd, len;
  unsigned char *p;
  Elf64_Shdr    *s;

  printf ("+ Encoding\n");
  if ((fd = open (f, O_RDWR, 0)) < 0) DIE ("open");
  len = (get_file_size (fd));
  if ((p = mmap (0, len, 
		 PROT_READ | PROT_WRITE, 
		 MAP_SHARED, fd, 0)) == MAP_FAILED) DIE ("mmap");

  if ((s = elfi_find_section (p, ".secure")) == NULL)
    {
      fprintf (stderr, "- No secure section found... Done\n");
      close (fd);
      exit (1);
    }

  xor_block (p + s->sh_offset, s->sh_size);

  /* Store Offset and size */
  *((int*)(p + 0x09)) = s->sh_offset;
  *((short*)(p + 0x0d)) = s->sh_size;
  printf ("+ Offsets: 0x%x, 0x%x\n", 
	  (unsigned int)s->sh_offset, (unsigned int)s->sh_size);
  close (fd);

  return;
}


int
main (int argc, char *argv[])
{
  if (argc != 2) {fprintf (stderr,"Invalid Number of Params\n");exit(1);}
    
  encode (argv[1]);
  return 0;
}
 
