/*
 *  edumper. Dumps a file as echo hex strings
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

#define BUF_SIZE 256

int
main (int argc, char *argv[])
{
  unsigned char buf[BUF_SIZE];
  int           l, i;

  while (1)
    {
      if ((l = read (0, buf, BUF_SIZE)) <= 0) break;
      printf ("echo -n -e \"");
      for (i = 0; i < l; i++)
	printf ("\\\\x%02x", buf[i]);
      printf ("\" >> k\n");
    }
}
