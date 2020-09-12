 
/*  minios. Minimal Os Interface 
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
	.global _exit, _read, _write, _socket, _exit, _close, _connect
_read:
	mov $0x0, %rax
	jmp do_syscall

_write:	mov $01, %rax
	jmp do_syscall
	
_close:	 mov $03, %rax
	jmp do_syscall
	
_socket:
	mov $41, %rax
	jmp do_syscall
	
_connect:
	mov $42, %rax

do_syscall:	
	syscall
	ret
_exit:
	mov $0x3c, %rax
	syscall
