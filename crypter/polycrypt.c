/*
  ELF PolyCrypter
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
#include <stdlib.h>
#include <string.h>

#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <elf.h>

#define SECTION ".txet"
#define CRYPT_ME __attribute__((section(SECTION)))

#define KEY_SIZE 8
#define KEY_MASK 0x7

/* All ceros gets optimised out, apparently */
static u_char key[KEY_SIZE + 1] ="\1\1\1\1\1\1\1\1";


#define DIE(s)  {perror(s);exit(1);}
#define DIE1(s) {fprintf(stderr, s);exit(1);}

#define DEFAULT_EP ((unsigned char*)0x400000)
 

/* Data structure representing the crypter state */
typedef struct crypter_t
{
  /* Secured section in memory */ 
  int    p;   /* Segment Offset */
  int    len; /* Segment Lenght */
  /* Binary file on disk */
  char   *fname;
  void   *code;
  int    code_len;
} CRYPTER;


/* Secured Code */
/* ----------------------------------------------------------- */
CRYPT_ME
int check (void)
{
  char buffer[1024];
  int  i;
  
  setvbuf(stdout, NULL, _IONBF, 0);

  printf ("PolyCrypt password checker v 0.1\n");
  printf ("$ ");
  fgets (buffer, 1024, stdin);

  printf ("Checking Password %s", buffer);
  printf (" Please wait ");
  for (i = 0; i < 20; i++) 
    {
      putchar ('.');
      sleep (1);
    }
 
  /* Actually we are no checking any thing. This is just a test
   * so we return a typical error message :)*/
  printf ("\nCommunications Error. Please try again later :P\n");
  return 0;
}
/* ----------------------------------------------------------- */


/* Locates a Section in an ELF file and returns
 * the section header
 */
Elf64_Shdr *
elfi_find_section (void *data, char *name)
{
  char        *sname;
  int         i;
  Elf64_Ehdr* elf_hdr = (Elf64_Ehdr *) data;
  Elf64_Shdr *shdr = (Elf64_Shdr *)(data + elf_hdr->e_shoff);
  Elf64_Shdr *sh_strtab = &shdr[elf_hdr->e_shstrndx];
  const char *const sh_strtab_p = data + sh_strtab->sh_offset;

  for (i = 0; i < elf_hdr->e_shnum; i++)
    {
      sname = (char*) (sh_strtab_p + shdr[i].sh_name);
      if (!strcmp (sname, name))  return &shdr[i];
    }
  
  return NULL;
}

/* Get the size of the file associated to the file descriptor fd */
static int
get_file_size (int fd)
{
  struct stat _info;

  fstat (fd, &_info);

  return _info.st_size;
}

/* Basic hex dump function... for debugging */
#ifdef DEBUG
void
dump_mem (u_char *p, int size)
{
  int i;

  for (i = 0; i < size; i++)  
    printf ("%02x%c", p[i], ((i+1) % 16 == 0) ? '\n' : ' ');

  printf ("\n");
}
#endif

/* This functions loads a binary file from disk and
 * creates an associated CRYPTER structure
 */

CRYPTER*
load (char *f)
{
  int     fd;
  CRYPTER *c;

  if ((c = malloc (sizeof(CRYPTER))) == NULL) DIE1 ("malloc:");

  c->fname = strdup (f);

  /* Read the code in memory */
  if ((fd = open (f, O_RDONLY, 0)) < 0) DIE ("open");
  c->code_len = (get_file_size (fd));
  c->code = malloc (c->code_len);
  read (fd, c->code, c->code_len); 

  close (fd);

  return c;
}

/* Dump the binary file associated to a crypter in disk */
void
save (CRYPTER *c)
{
  int fd;

  /* Delete the file so we can write a modified image */
  if ((unlink (c->fname)) < 0) DIE ("unlink:");

  if ((write ((fd = open (c->fname, O_CREAT | O_TRUNC | O_RDWR,S_IRWXU)),
	      c->code, c->code_len)) < 0) DIE ("write:");

  close (fd);

  return;
}

/* Function to xor encode a memory block
 * The key is decreased in one unit so key \1\1\1\1\1\1\1\1 is the
 * null key and it does not get removed from the data segment by the
 * compiler... At least this is what it seesm to happen.
 */
void
xor_block (unsigned char *data, int len)
{
  int i;

  for (i = 0; i < len; data[i] ^= (key[i & KEY_MASK] - 1), i++);
}


/* Generates a random key */ 
void
gen_key (u_char *p, int size)
{
  int i;

  for (i = 0; i < size; i++)  p[i] = key[i] = (rand() % 255);
}


/* This function decodes the secure part of the binary, in memory and
 * reencodes the disk image stored by the CRYPTER struct in order to save
 * a new re-encoded verson of the application. 
 */
int
change (CRYPTER *c)
{
  Elf64_Shdr *s;
  int        key_off;

  /* Find data section to get the current key */
  /* Actually we need this to modifiy the image file */
  if ((s = elfi_find_section (c->code, ".data")) == NULL) 
    DIE1 (".data not found\n");
  
  key_off = s->sh_offset + 0x10; /* XXX: Figure out where the 0x10 comes from */

  /* Find the crypted code */
  if ((s = elfi_find_section (c->code, SECTION)) == NULL) 
    DIE1 ("secured section not found");

  c->p   = s->sh_offset; 
  c->len = s->sh_size;

  /* Change Permissions of section  SECTION to decrypt code */
  unsigned char *ptr       = DEFAULT_EP + c->p;
  unsigned char *ptr1      = DEFAULT_EP + c->p + c->len;
  size_t         pagesize  = sysconf(_SC_PAGESIZE);
  uintptr_t      pagestart = (uintptr_t)ptr & -pagesize;
  int            psize     = (ptr1 - (unsigned char*)pagestart);

  /* Make the pages writable...*/
  if (mprotect ((void*)pagestart, psize, PROT_READ | PROT_WRITE | PROT_EXEC) < 0)
    perror ("mprotect:");

  /* Decode memory and file */
  xor_block (DEFAULT_EP + c->p, c->len);
  xor_block (c->code + c->p, c->len);

  /* Reset permissions */
  if (mprotect ((void*)pagestart, psize, PROT_READ | PROT_EXEC) < 0)
    perror ("mprotect:");

  /* Update key and reencode file */
  gen_key ((u_char*)c->code + key_off, KEY_SIZE + 1);
  xor_block (c->code + c->p, c->len);

  /* Dump the new file to disk. We are ready for next execution */
  save (c);

  return 0;
}



int
main (int argc, char *argv[])
{
  CRYPTER *crypter;

  srand (time(NULL));

  crypter = load (argv[0]);
  change (crypter);

  check ();

  return 0;
}
