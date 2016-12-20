#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <sys/user.h>
#include <sys/reg.h>

#include "stub.h"



#ifdef DEBUG
#define DPRINTF(...) fprintf( stderr, __VA_ARGS__ );
#else
#define DPRINTF(...) 
#endif
/* 64 bits constants */
#if 1
#define OPCODE_MASK 0xffffffffffffff00
#define KEY         0x4242424242424242
#else
#define OPCODE_MASK 0xffffff00
#define KEY         0x42424242
#endif

// XOR encoding data
#define KEY_MASK    7
static char key[8] ="\x42\x42\x42\x42\x42\x42\x42\x42";


// Helper macros
#define XOR(op1) {int _i; unsigned char *_p; for (_p = (unsigned char*) &op1, _i = 0; _i < 8; _p[_i] ^= key[_i & KEY_MASK], _i++);}

#define PERROR(s) {perror(s); exit (1);}

pid_t _pid;


// Get this values from readelf... Should get them from the ELF itself
// just being lazy
static void *secure_ptr = (void*)0x0400e22;
static int   secure_len = 0x0e22;

// Helper function for debugging... not used
void
_dump_opcodes (unsigned char *p, int len)
{
  int i;
  for (i = 0; i < len; i++) printf ("%02x ", p[i]);
  printf ("\n");
}

int
_stub (void *ep)
{
  void                     *bp_ip;
  long                    ip1, op1, op2;
  struct user_regs_struct regs;
  int                     status, cnt;

  printf ("%s", "0x00pf IbI Crypter Stub\n");

  // Start debugging!!!
  if ((_pid = fork ()) < 0) PERROR("fork:");

  if (_pid == 0) return 0;  // Child process just keeps running
  else
    {
      // Father starts debugging child
      if ((ptrace (PTRACE_ATTACH, _pid, NULL, NULL)) < 0) PERROR ("ptrace_attach:");
      printf ("%s", "+ Waiting for process...\n");
      wait (&status);

      bp_ip = ep;

      // Set breakpoint at get there...
      op1 = ptrace (PTRACE_PEEKTEXT, _pid, bp_ip);
      DPRINTF("BP: %p 1 Opcode: %lx\n", bp_ip, op1);
      if (ptrace (PTRACE_POKETEXT, _pid, bp_ip, 
		  (op1 & 0xFFFFFFFFFFFFFF00) | 0xcc) < 0) PERROR ("ptrace_poke:");

      // Run until breakpoint is reached.
      if (ptrace (PTRACE_CONT, _pid, 0, 0) < 0) PERROR("ptrace_cont:");
      wait (&status);
      
      ptrace (PTRACE_GETREGS, _pid, 0, &regs);
      DPRINTF ("Breakpoint reached: RIP: %llx\n", regs.rip);
      regs.rip--;
      ptrace (PTRACE_SETREGS, _pid, 0, &regs);

      // REstore opcode
      ptrace (PTRACE_POKETEXT, _pid, bp_ip, op1);

      // Start step by step debugging
      ip1 = (long) ep;
      cnt = 0;
      while (WIFSTOPPED (status))
	{
	  cnt ++;
	  // Read up to 16 bytes to get the longest instruction possible
	  // Decode and write back the decoded code to execute it
	  op1 = ptrace (PTRACE_PEEKTEXT, _pid, ip1);
	  op2 = ptrace (PTRACE_PEEKTEXT, _pid, ip1 + 8);
	  DPRINTF ("%lx :: OPCODES : %lx %lx\n", ip1, op1, op2);
	  
	  XOR(op1);
	  XOR(op2);

	  DPRINTF ("%lx :: DOPCODES: %lx %lx\n", ip1, op1, op2);

	  ptrace (PTRACE_POKETEXT, _pid, ip1, op1);
	  ptrace (PTRACE_POKETEXT, _pid, ip1 + 8, op2);

	  /* Make the child execute another instruction */
	  if (ptrace(PTRACE_SINGLESTEP, _pid, 0, 0) < 0) PERROR ("ptrace_singlestep:");
	  wait(&status);
	  
	  // Re-encode the instruction just executed so we do not have
	  // to count how many bytes got executed
	  XOR(op1);
	  XOR(op2);

	  ptrace (PTRACE_POKETEXT, _pid, ip1, op1);
	  ptrace (PTRACE_POKETEXT, _pid, ip1 + 8, op2);

	  // Get the new IP
	  ptrace (PTRACE_GETREGS, _pid, 0, &regs);
	  ip1 = regs.rip;

	  // If code is outside .secure section we stop debugging 
	  if ((void*)ip1 < secure_ptr || (void*)ip1 > secure_ptr + secure_len)
	    {
	      printf ("Leaving .secure section... %d instructions executed\n", cnt);
	      break;
	    }
	}

      ptrace (PTRACE_CONT, _pid, 0, 0);
      wait (&status);      
    }

  printf ("DONE\n");
  exit (1);
}

// http://eli.thegreenplace.net/2011/01/27/how-debuggers-work-part-2-breakpoints
// http://mainisusuallyafunction.blogspot.nl/2011/01/implementing-breakpoints-on-x86-linux.html
// http://www.alexonlinux.com/how-debugger-works
//http://eli.thegreenplace.net/2011/01/23/how-debuggers-work-part-1
