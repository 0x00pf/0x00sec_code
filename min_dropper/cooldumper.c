/*
 *  Cooldumper. Dumps file as echo hex strings on ptraced process
 *  Copyright (c) 2020 pico (@0x00pico at twitter)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
Run as:
$ cat binary_file | ./cooldumper PID
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ptrace.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <sys/user.h>
#include <sys/reg.h>

#define BUF_SIZE 256

int
cpy_str (pid_t _pid, char *str, unsigned long long int *p)
{
  int                    i;
  int                    len = strlen (str);
  int                    len1 = (len / 8) + 1;
  char                   *aux = malloc (len1 * 8);
  unsigned long long int *d = (unsigned long long int*)aux;

  printf ("!! Reallocating %d to %d bytes\n", len, len1);
  memset (aux, 0, len1);
  strcpy (d, str);
  
  for (i = 0; i < len1 + 1; i++)
    {
      if ((ptrace (PTRACE_POKEDATA, _pid, p, *d)) < 0) perror ("POKE Stack:");
      p++; d++;
    }
  free (aux);
  return len;
}

int
main (int argc, char *argv[])
{
  unsigned char           buf[BUF_SIZE], cmd[4096], *p;
  int                     l, i, slen;
  pid_t                   _pid;
  struct user_regs_struct regs;
  struct user_regs_struct regs_cpy;
  int                     status;
  long                    opcode, opcode1;

  
  _pid = atoi (argv[1]);

  printf ("+ Attaching to process %ld\n", _pid);
  if ((ptrace (PTRACE_ATTACH, _pid, NULL, NULL)) < 0)
    perror ("ptrace_attach:");
  
  printf ("%s", " ..... Waiting for process...\n");
  wait (&status);
  
   
  //Get registers
  printf ("+ Getting current registers state....\n");
  if ((ptrace (PTRACE_GETREGS, _pid, 0, &regs)) < 0) perror ("ptrace_get_regs:");

  printf ("  -> Process stopped at : %p\n", regs.rip);
  printf ("  -> Stack  at          : %p\n", regs.rsp);
 
  // Make a copy for restore state when we are done
  printf ("+ Current register set copied...\n");
  memcpy (&regs_cpy, &regs, sizeof (struct user_regs_struct));

  printf ("+ Retrieving opcode from (%llx) -> ", regs.rip);
  if ((opcode = ptrace (PTRACE_PEEKTEXT, _pid, regs.rip, 0)) < 0)
    perror ("retrieve opcode:");
  printf ("0x%lx\n", opcode);

  ptrace (PTRACE_POKETEXT, _pid, regs.rip, 0x050f050f050f050f); // Write syscall
  
  printf ("+ Pushing string into stack...\n");
  regs.rsp -= 2048; // Allocating 2048 bytes buffer

  while (1)
    {
      memset (cmd, 0, 4096);
      p = cmd;
      if ((l = read (0, buf, BUF_SIZE)) <= 0) break;
      
      p+= sprintf (p, "echo -n -e \"");
      for (i = 0; i < l; i++)
	p+=sprintf (p, "\\\\x%02x", buf[i]);
      p+=sprintf (p, "\" > k\n");
      printf ( "--------------------\n");
      printf ("%s", cmd);
      printf ( "--------------------\n");
       
      slen = cpy_str (_pid, cmd, regs.rsp);
       
      printf ("+ Preparing for syscall....\n");
      
      regs.rax = 1;        // Write
      regs.rdi = 5;        // socket
      regs.rsi = regs.rsp; // Buf
      regs.rdx= slen;
      
      if ((ptrace (PTRACE_SETREGS, _pid, 0, &regs)) < 0) perror ("ptrace_set_regs:");

      printf ("+ Run single step...\n");
      if ((ptrace (PTRACE_SINGLESTEP, _pid, 0,0)) < 0) perror ("Run syscallL");
      
      printf ("%s", "+ Waiting for process...\n");
      wait (&status);

      if ((ptrace (PTRACE_SINGLESTEP, _pid, 0,0)) < 0) perror ("Run syscallL");
      
      printf ("%s", "+ Waiting for process...\n");
      wait (&status);
  
    }  
  printf ("Restoring opcode att : %p\n", regs_cpy.rip);
  if ((ptrace (PTRACE_POKETEXT, _pid, regs_cpy.rip, opcode)) < 0) perror ("Restore opcode:");

  printf ("0x%lx\n", opcode);
  // Restore copy
  printf  ("+Restoting original registers\n");
  if ((ptrace (PTRACE_SETREGS, _pid, 0, &regs_cpy)) < 0) perror ("ptrace_set_regs:");
  
  printf ("Registers successfully restored\n");
  printf ("+ Deataching process...\n");
	 
  if ((ptrace (PTRACE_DETACH, _pid, NULL, NULL)) < 0) perror ("ptrace_deattach:");
  wait (&status);
  printf ("Releasing process...\n");

  return 0;
}
