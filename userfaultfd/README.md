# Copying code on Read Only memory without using mprotect

When a program needs to generate code and run it, several steps are required. The program needs to allocate memory with read, write, and execution permissions, and perhaps remove the write permission afterward to resemble a normal program. Alternatively, it has to allocate memory with read and write permissions, copy the code there, and then change the permissions to enable read and execution in order to be able to actually run the code. This last case is necessary when W^X protection is enabled.

W^X means that if write permission is enabled, execution cannot be, and if execution permission is enabled, write permission should be disabled. The symbol ^ usually represents the `xor` operation, or exclusive OR, which is why it's often known as W^X. This protection prevents setting a memory block with all permissions, allowing a program to copy data into that memory block and run it immediately. This forces the attacker to use two consecutive calls to `mprotect`, which is usually considered suspicious.

Alternatively, calling `mmap` with all permissions set is also considered suspicious.

So, what can we do? Let me introduce the `userfaultfd` system call.


# The `userfaultfd` system call

This system call was introduced in kernel 4.3, so it's a relatively recent addition. It allows us to control the page fault mechanism from user space. This system call enables features like live migration of VMs, garbage collectors, and snapshotting. It can also be used to implement a custom swap system or retrieve code from a remote server whenever it's needed.

It is a very powerful and versatile system call that deserves attention. However, it can be abused to get executable code into memory without manipulatingplaying with strange permission combinations. But let's start by learning how it works.

When you `mmap` a memory block in your program, you are not immediately reserving physical memory. Instead, you're reserving a virtual memory address range, or rather, a set of entries in the memory page table. Whenever the program tries to access that virtual memory address range, a page fault will occur because that memory range doesn't have any physical memory associated with it yet. The kernel captures that page fault and allocates a physical memory page for those virtual addresses. Now that everything is fine, the program can continue.

When using `userfaultfd`, we can take on the role of the kernel in this process. When a page fault occurs, instead of giving control to the kernel and letting it deal with the missing page, our application gets control, allowing us to decide what content we want on those pages.

The cool thing about this process is that our page fault user space code can write into the page despite the permissions associated with it when it was `mmapped`. 

# Use cases

Before showing you the code, let me introduce a couple of use cases. I had briefly mentioned in the introduction but it worth to elaborate them a littl bit. 

A traditional crypter will have a program header in the ELF file containing the encrypted code. Generally, if the program header has all three permissions, that would be suspicious, so it will likely have read and write permissions. That has the advantage that static analysis tools won't look into that part of the program immediately.

When the program starts, the `stub` gets executed and needs to write into the memory where the encrypted program header was loaded. If the program header has read and write permission, it can do that straight away, but then it will need to run `mprotect` to give it execution permission. Otherwise, the code cannot be executed.

Another case may be malware that gets code from a C2 server to execute. The code isn't installed in the malware itself, so analysis of the binary won't reveal much about what the malware does. Let's assume the W^X protection is in place, so the malware has to allocate memory with read and write permissions (it cannot give all three permissions to the memory block), then download the code and copy it into that region. Finally, it has to remove the write permission and add the execution permission using `mprotect`.

Overall, the use of `mprotect` is something to monitor because you usually don't need to use that system call. Normally, whatever permissions you need, you just give them when allocating the memory with `mmap`, and it’s uncommon to have to change them afterward. Moreover, giving execution permissions at runtime, even when it may be completely legitimate, is also considered suspicious because normal programs don't self-modify or get code to execute in non-standard ways, i.e., not using functions like `execve` or `dlopen/dlsym`.

# PoC, Copying and Running Code Without Changing Permissions

I'll show the code in pieces, explaining each part. You can just copy each piece and paste them all together in a simple file to get the final code, or head to my GitHub page and grab it from there.

https://github.com/0x00pf/0x00sec_code/userfaultfd

```C
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <linux/userfaultfd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <poll.h>

unsigned char x86_hello_bin[] = {
  0xb8, 0x01, 0x00, 0x00, 0x00, 0xbf, 0x01, 0x00, 0x00, 0x00, 0x48, 0x8d,
  0x35, 0x13, 0x00, 0x00, 0x00, 0xba, 0x0d, 0x00, 0x00, 0x00, 0x0f, 0x05,
  0xb8, 0x3c, 0x00, 0x00, 0x00, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x05,
  0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x57, 0x6f, 0x72, 0x6c, 0x64, 0x21,
  0x0a
};
unsigned int x86_hello_bin_len = 49;

```

Nothing special here, just the headers needed to compile the PoC and the shell code we’ll use for the demonstration—in this case, the usual `Hello, world!` binary for Intel `x86_64`. I got it from Chapter 8 of [Heavy Wizardry 101](https://nostarch.com/heavy-wizardry-101) ;) . You can write your own, but it was a got opportunity to promote my book XD.

Next, we create and initialize the `userfaultfd`.

```C
int main(void)
{
  int    uffd; 
  
  if ((uffd = syscall(SYS_userfaultfd, O_CLOEXEC | O_NONBLOCK | UFFD_USER_MODE_ONLY)) == -1) return 1;
  
  struct uffdio_api api;
  memset (&api,0, sizeof(struct uffdio_api));
  api.api = UFFD_API;
  
  if (ioctl(uffd, UFFDIO_API, &api) == -1) return 2;
```

There is no libC wrapper for this system call, so we have to use the general `syscall` function from GNU LibC to call it. The flag `UFFD_USER_MODE_ONLY` allows us to use the system call as a regular user, otherwise, depending on the kernel configuration we may need to run the test program as root (actuall `CAP_SYS_PTRACE` is needed).

Then we set up the API we want to use with an `ioctl` call, and we’re good to go.

Next, we allocate memory and register our user space page fault handler for that memory range.

```C
  size_t pagesize = sysconf(_SC_PAGE_SIZE);
  char *code = mmap(NULL, pagesize, PROT_READ | PROT_EXEC,
		      MAP_PRIVATE | MAP_ANONYMOUS, -1,0);
  
  struct uffdio_register reg;
  reg.range.start = (unsigned long)code;
  reg.range.len = pagesize;
  reg.mode = UFFDIO_REGISTER_MODE_MISSING;
  
   if (ioctl(uffd, UFFDIO_REGISTER, &reg) == -1) return 3;
```

Note how we’re using standard permissions with `mmap`: read and execute. No write permission, no all permissions set. 

The registration of the address range is done using another `ioctl`. For this demonstration, we’re using a single page for the sake of simplicity, but you can apply this technique to multiple pages in the exact same way—just change the sizes and make sure that the addresses are page-aligned. The mode can take the following values:

- `UFFDIO_REGISTER_MODE_MISSING` — handle missing-page faults.
- `UFFDIO_REGISTER_MODE_WP` — handle write-protection faults.
- `UFFDIO_REGISTER_MODE_MINOR` — handle minor faults (supported only on some backing types and kernel versions. Forget about it for now.).

In our case, `MODE_MISSING` is the one we want because the page is missing until we add it.

Now, we prepare our payload to be copied.

```C
  char *buf = aligned_alloc(pagesize, pagesize);
  memcpy (buf, x86_hello_bin, x86_hello_bin_len);
```

All this mechanism works on memory pages, so we need to allocate memory that is be page-aligned. As I said before, we’re using a single page for simplicity, so the size we ask `aligned_alloc` for is the same as the page size. Once we have that page, we copy over the code we want to run. 

In a real-world program, you may likely read the code from a file or from the network, but the process is the same. You can also store the code inside the ELF and make sure it gets loaded in memory in its own page—playing with the ELF structures. Well, there are a lot of options to have fun with this.

Finally, we just copy the code over using one more `ioctl`, actually this assign physical memory to the virtual memory block we allocated and also initializes it.

```C
  struct uffdio_copy copy;
  memset (&copy, 0, sizeof (struct uffdio_copy));
  copy.src = (unsigned long) buf; 
  copy.dst = (unsigned long) code;
  copy.len = pagesize;
  if (ioctl(uffd, UFFDIO_COPY, &copy) == -1) return 4;
 ```
 Pretty straightforward. The `UFFDIO_COPY` `ioctl` copies `copy.len` bytes from `copy.src` into `copy.dst`. This is the operation that happens no matter what the permissions of that page are, because this is intended to be used to manage page faults—we should be able to always write into the associated virtual address range to be able to perform such an operation.

And then we just run the code:

```C
  ((int(*)())code)();
```

Yes, the casting is a bit ugly. Be free to define your own types to make it look nicer.


# `userfaultfd`. FD stands for file descriptor.

Yes, the `userfaultfd` creates a File Descriptor to deal with page FAULTs in USER space. What does this mean? Well, that we can actually get events on that file descriptor, and that is the intended way to use this system call. Another thread or the main loop of the application will `select`, `poll`, or just `read` on that file descriptor. Whenever something happens (normally a page fault, but there are a few other events that may happen) the file descriptor gets an event to read from that file descriptor with the relevant information, such as the virtual address that was producing the page fault.

This opens the possibility of interesting runtime code modification options.

# SUMMARY

I just stumbled upon this system call by accident, and it looked like it could be used for this purpose, and it was. I think this is interesting for programs secured with crypters and getting code from memory—those usually have to use funny permissions or call to `mprotect`, which may look suspicious. As usual, any comment, correction and feedback in general is welcomed!

