/* A simple program to show how to set up an X window for OpenGL rendering.
 * X86 compilation: gcc -o -L/usr/X11/lib   main main.c -lGL -lX11
 * X64 compilation: gcc -o -L/usr/X11/lib64 main main.c -lGL -lX11
 */
#include <stdio.h>
#include <stdlib.h>

#include <GL/glx.h>    /* this includes the necessary X headers */
#include <GL/gl.h>

#include <X11/X.h>    /* X11 constant (e.g. TrueColor) */
#include <X11/keysym.h>

#define ONE					1
#define MAX(A, B)			((A > B) ? A : B)

static int dblBuf[]  = {GLX_RGBA, GLX_DEPTH_SIZE, 16, GLX_DOUBLEBUFFER, None};

Display   *p_display;
Window     win;
char c = ' ';

void fatalError(char *message)
{
  fprintf(stderr, "main: %s\n", message);
  exit(1);
}

void redraw(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBegin(GL_TRIANGLES);
        glColor3f(1, 0, 0);
        glVertex3f(10, 10, 0);
        glVertex3f(100, 10, 0);
        glVertex3f(10, 100, 0);
    glEnd();

    glXSwapBuffers(p_display, win);/* buffer swap does implicit glFlush */
}

int main(int argc, char **argv)
{
  XVisualInfo         *vi;
  Colormap             cmap;
  XSetWindowAttributes swa;
  GLXContext           cx;
  XEvent               event;
  GLboolean            needRedraw = GL_FALSE;
  int                  dummy;

  /*** (1) open a connection to the X server ***/

  p_display = XOpenDisplay(NULL);
  if (p_display == NULL)
    fatalError("could not open display");

  /*** (2) make sure OpenGL's GLX extension supported ***/

  if(!glXQueryExtension(p_display, &dummy, &dummy))
    fatalError("X server has no OpenGL GLX extension");

  /*** (3) find an appropriate visual ***/

  /* find an OpenGL-capable RGB visual with depth buffer */
  vi = glXChooseVisual(p_display, DefaultScreen(p_display), dblBuf);
  if (vi == NULL || vi->class != TrueColor)
  {
    fatalError("Double buffered trueColor visual required for this program");
  }

  /*** (4) create an OpenGL rendering context  ***/

  /* create an OpenGL rendering context */
  cx = glXCreateContext(p_display, vi, /* no shared dlists */ None,
                        /* direct rendering if possible */ GL_TRUE);
  if (cx == NULL)
    fatalError("could not create rendering context");

  /*** (5) create an X window with the selected visual ***/

  /* create an X colormap since probably not using default visual */
  cmap = XCreateColormap(p_display, RootWindow(p_display, vi->screen), vi->visual, AllocNone);
  swa.colormap = cmap;
  swa.border_pixel = 0;
  swa.event_mask = KeyPressMask    | ExposureMask
                 | ButtonPressMask | StructureNotifyMask;
  win = XCreateWindow(p_display, RootWindow(p_display, vi->screen), 0, 0,
                      300, 300, 0, vi->depth, InputOutput, vi->visual,
                      CWBorderPixel | CWColormap | CWEventMask, &swa);
  XSetStandardProperties(p_display, win, "main", "main", None,
                         argv, argc, NULL);

  /*** (6) bind the rendering context to the window ***/

  glXMakeCurrent(p_display, win, cx);

  /*** (7) request the X window to be displayed on the screen ***/

  XMapWindow(p_display, win);

  /*** (8) configure the OpenGL context for rendering ***/

  glEnable(GL_DEPTH_TEST); /* enable depth buffering */
  glDepthFunc(GL_LESS);    /* pedantic, GL_LESS is the default */
  glClearDepth(1.0);       /* pedantic, 1.0 is the default */

  /* frame buffer clears should be to black */
  glClearColor(1.0, 1.0, 1.0, 0.0);

  /* set up projection transform */
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, 300, 300, 0, 10, -1);
  glViewport(0, 0, 300, 300);

  /* Initialize model view */
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  /*** (9) dispatch X events ***/

  while (1)
  {
    do
    {
      XNextEvent(p_display, &event);
      switch (event.type)
      {

        case KeyPress:
        {
          KeySym     keysym;
          XKeyEvent *kevent;
          char       buffer[1];
          /* It is necessary to convert the keycode to a
           * keysym before checking if it is an escape */
          kevent = (XKeyEvent *) &event;
          if ((XLookupString((XKeyEvent *)&event,buffer,1,&keysym,NULL) == 1) && (keysym == (KeySym)XK_Escape))
            exit(0);
          break;
        }

        case ButtonPress:
          switch (event.xbutton.button)
          {
            case 1:
              break;
            case 2:
              break;
            case 3:
              break;
          }
          break;

        case ConfigureNotify:
            /* Reconfigure the projection matrix */
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0, event.xconfigure.width, event.xconfigure.height, 0, 10, -1);
            glViewport(0, 0, event.xconfigure.width, event.xconfigure.height);
            /* fall through... */
        case Expose:
            needRedraw = GL_TRUE;
            break;

      }
    } while(XPending(p_display)); /* loop to compress events */

    if (needRedraw)
    {
        redraw();
        needRedraw = GL_FALSE;
    }
  }

  return 0;
}
