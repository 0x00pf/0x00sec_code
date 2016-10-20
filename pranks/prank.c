
/*
  CyberPranks
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <linux/uinput.h>

#define die(str, args...) do { \
        perror(str); \
        exit(EXIT_FAILURE); \
    } while(0)

int
move_mouse (int fd, int dx, int dy)
{
  struct input_event     ev;

  memset(&ev, 0, sizeof(struct input_event));
  ev.type = EV_REL;
  ev.code = REL_X;
  ev.value = dx;
  if (write (fd, &ev, sizeof(struct input_event)) < 0)
    die("error: write");

  memset(&ev, 0, sizeof(struct input_event));
  ev.type = EV_REL;
  ev.code = REL_Y;
  ev.value = dy;
  if (write (fd, &ev, sizeof(struct input_event)) < 0)
    die("error: write");
  
  memset(&ev, 0, sizeof(struct input_event));
  ev.type = EV_SYN;
  ev.code = 0;
  ev.value = 0;
  if(write(fd, &ev, sizeof(struct input_event)) < 0)
    die("error: write");
  
  usleep(15000);
  
  return 0;
}

int
click (int fd)
{
  struct input_event     ev;

  memset(&ev, 0, sizeof(struct input_event));
  ev.type = EV_KEY;
  ev.code = BTN_LEFT;
  ev.value = 1;
  if (write (fd, &ev, sizeof(struct input_event)) < 0)
    die("error: write");

  usleep (500000);

  memset(&ev, 0, sizeof(struct input_event));
  ev.type = EV_KEY;
  ev.code = BTN_LEFT;
  ev.value = 0;
  if (write (fd, &ev, sizeof(struct input_event)) < 0)
    die("error: write");

  
  memset(&ev, 0, sizeof(struct input_event));
  ev.type = EV_SYN;
  ev.code = 0;
  ev.value = 0;
  if(write(fd, &ev, sizeof(struct input_event)) < 0)
    die("error: write");
  
  usleep(15000);
  
  return 0;
}

int
main(void)
{
    int                    fd;
    struct uinput_user_dev uidev;
    struct input_event     ev;
    int                    dx, dy;
    int                    i;

    /* Open the device */
    if ((fd = open ("/dev/uinput", O_WRONLY | O_NONBLOCK)) < 0) die ("error: open");

    /* We want to produce key events... left button click*/
    if(ioctl(fd, UI_SET_EVBIT,  EV_KEY) < 0)    die ("error: ioctl");
    if(ioctl(fd, UI_SET_KEYBIT, BTN_LEFT) < 0)  die ("error: ioctl");

    /* And we also want to produce mouse events */
    if(ioctl(fd, UI_SET_EVBIT,  EV_REL) < 0)    die ("error: ioctl");
    if(ioctl(fd, UI_SET_RELBIT, REL_X) < 0)     die ("error: ioctl");
    if(ioctl(fd, UI_SET_RELBIT, REL_Y) < 0)     die ("error: ioctl");

    /* Time to register our virtual device */
    memset (&uidev, 0, sizeof(uidev));
    snprintf (uidev.name, UINPUT_MAX_NAME_SIZE, "uinput-sample");

    uidev.id.bustype = BUS_USB;
    uidev.id.vendor  = 0x1;
    uidev.id.product = 0x1;
    uidev.id.version = 1;

    if (write (fd, &uidev, sizeof(uidev)) < 0) die("error: write");
    if (ioctl(fd, UI_DEV_CREATE) < 0) die("error: ioctl");
    /* We are done! Fun starts */


    sleep(2);


    while (1)
      {
        for (i = 0; i < 100; i++)
	  {
	    move_mouse (fd, rand() % 10 -5 , rand()%10 - 5);
	  }
	sleep (2);
	click (fd);
	sleep (5);
      }

    sleep(2);

    if (ioctl (fd, UI_DEV_DESTROY) < 0) die ("error: ioctl");

    close (fd);

    return 0;
}
