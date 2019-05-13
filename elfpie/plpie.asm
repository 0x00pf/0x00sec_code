; ELFPIE
;  Copyright (c) 2019 picoFlamingo
;
;This program is free software: you can redistribute it and/or modify
;it under the terms of the GNU General Public License as published by
;the Free Software Foundation, either version 3 of the License, or
;(at your option) any later version.
;
;This program is distributed in the hope that it will be useful,
;but WITHOUT ANY WARRANTY; without even the implied warranty of
;MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;GNU General Public License for more details.
;
;You should have received a copy of the GNU General Public License
;along with this program.  If not, see <http://www.gnu.org/licenses/>.


section .text
        global _start
	global _label

_start:
	  ;; save cpu state
	push rax	
	push rdi	
	push rsi
	push rdx
	
        mov rax,1       	; [1] - sys_write
        mov rdi,1       	; 0 = stdin / 1 = stdout / 2 = stderr
        lea rsi,[rel msg]     	; pointer(mem address) to msg (*char[])
        mov rdx, msg_end - msg  ; msg size
        syscall         	; calls the function stored in rax


	;; restore cpu state
	pop rdx
	pop rsi
	pop rdi
	pop rax

	_label:	
	;; mov rax, 0x11111111
	;; jmp rax
	jmp 0x1000

align 8
        msg     db 'This file has been infected for 0x00SEC',0x0a,0
	msg_end db 0x0
