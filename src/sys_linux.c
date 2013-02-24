#include <X11/X.h>    /* X11 constant (e.g. TrueColor) */
#include <X11/keysym.h>
#include <GL/glx.h>

#include <sys/time.h>
#include <stdarg.h>
#include <signal.h>

#define G_GL_EXT_IMPLEMENT
#include "land.h"
#include "gl/glxext.h"


GConfig conf = { NULL, 640, 480, 0, 15, NULL };

static unsigned int get_milliseconds() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

static void x11_hide_cursor( Display* display, Window root ){
    XGCValues xgc;
    XColor    col;

    xgc.function = GXclear;
    Pixmap cursormask = XCreatePixmap( display, root, 1, 1, 1 );
    GC gc = XCreateGC( display, cursormask, GCFunction, &xgc );
    XFillRectangle( display, cursormask, gc, 0, 0, 1, 1 );
    col.pixel = 0;
    col.red = 0;
    col.flags = 4;
    Cursor cursor = XCreatePixmapCursor( display, cursormask, cursormask, &col, &col, 0, 0 );
    XFreePixmap( display, cursormask );
    XFreeGC( display, gc );

    XDefineCursor( display, root, cursor );
}

static int keysym_to_key(KeySym key) {
    switch(key) {
        case 'a': return GK_A;
        case 's': return GK_S;
        case 'd': return GK_D;
        case 'w': return GK_W;
        case 'q': return GK_Q;
        case 'e': return GK_E;
        case XK_Up: return GK_UP;
        case XK_Down: return GK_DOWN;
        case XK_Left: return GK_LEFT;
        case XK_Right: return GK_RIGHT;
        case XK_Return: return GK_RETURN;
        case XK_Escape: return GK_ESCAPE;
        case XK_space: return GK_SPACE;
        default: return GK_UNKNOWN;
    }
}

int main(int argc, char** argv) {
    g_configure( &conf );
    if( !conf.title ) conf.title = strdup("Myr default");

    int attrib[18] = {
        GLX_RGBA,
        GLX_DEPTH_SIZE, 24,
        GLX_DOUBLEBUFFER, None
    };

    if( conf.flags & GC_MULTISAMPLING ){
        attrib[4] = GLX_SAMPLE_BUFFERS; attrib[5] = 1;
        attrib[6] = GLX_SAMPLES; attrib[7] = 4;
        attrib[8] = None;
    }

    Display* dpy = XOpenDisplay( NULL );
    int screen = DefaultScreen( dpy );
    Window root = RootWindow( dpy, screen );

    int dummy;
    if( !glXQueryExtension( dpy, &dummy, &dummy ) )
        g_fatal_error("X server has no OpenGL GLX extension");

    XVisualInfo *visinfo = glXChooseVisual( dpy, DefaultScreen(dpy), attrib );

    XSetWindowAttributes attr;
    attr.background_pixel = 0;
    attr.border_pixel = 0;
    attr.colormap = XCreateColormap(dpy, root, visinfo->visual, AllocNone);
    attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask | KeyReleaseMask |
                      PointerMotionMask | ButtonPressMask | ButtonReleaseMask;

    Window win = XCreateWindow(
        dpy, root, 0, 0,
        conf.width, conf.height, 0,
        visinfo->depth,
        InputOutput,
        visinfo->visual,
        CWBackPixel | CWBorderPixel | CWColormap | CWEventMask,
        &attr
    );

    XMapWindow( dpy, win );

    // set fullscreen if requested
    if( conf.flags & GC_FULLSCREEN ){
      XEvent xev;
      Atom wm_state = XInternAtom( dpy, "_NET_WM_STATE", False);
      Atom fullscreen = XInternAtom( dpy, "_NET_WM_STATE_FULLSCREEN", False);

      memset(&xev, 0, sizeof(xev));
      xev.type = ClientMessage;
      xev.xclient.window = win;
      xev.xclient.message_type = wm_state;
      xev.xclient.format = 32;
      xev.xclient.data.l[0] = 1;
      xev.xclient.data.l[1] = fullscreen;
      xev.xclient.data.l[2] = 0;

      XSendEvent( dpy, DefaultRootWindow(dpy), False, SubstructureNotifyMask, &xev);

      XWindowAttributes xwa;
      XGetWindowAttributes(dpy, DefaultRootWindow(dpy), &xwa);
      conf.width = xwa.width;
      conf.height = xwa.height;
    }

    if( conf.flags & GC_HIDE_CURSOR ) x11_hide_cursor( dpy, win );
//    if( conf.flags & FL_CLIP_CURSOR )
//      XGrabPointer( dpy, win, False, ButtonPressMask | ButtonReleaseMask,
//                     GrabModeAsync, GrabModeAsync, win, None, CurrentTime );


    GLXContext glcontext;
    if( conf.flags & GC_CORE_PROFILE ) {
        GLXContext tempContext = glXCreateContext(dpy, visinfo, NULL, True);
        PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribs = (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddress((GLubyte*)"glXCreateContextAttribsARB");
        if( !glXCreateContextAttribs )
            g_fatal_error("Your platform does not support OpenGL 3.0.\nTry removing GC_CORE_PROFILE flag.\n");

        int fbcount = 0;
        GLXFBConfig *framebufferConfig = glXChooseFBConfig(dpy, screen, 0, &fbcount);
        if( !framebufferConfig ) {
            g_fatal_error("Can't create a framebuffer for OpenGL 3.0.\n");
        } else {
            int attribs[] = {
                GLX_CONTEXT_MAJOR_VERSION_ARB, conf.gl_version/10,
                GLX_CONTEXT_MINOR_VERSION_ARB, conf.gl_version%10,
                GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
                0
            };
            glcontext = glXCreateContextAttribs(dpy, framebufferConfig[0], NULL, True, attribs);
            glXMakeCurrent(dpy, 0, 0);
            glXDestroyContext(dpy, tempContext);
        }
    } else {
        glcontext = glXCreateContext(dpy, visinfo, NULL, True);
    }

    glXMakeCurrent(dpy, win, glcontext);

    // Reset OpenGL error state:
    glGetError();

    g_debug_str("OpenGL Version: %s\n", glGetString(GL_VERSION));

    g_init_gl_extensions();
    g_initialize( conf.width, conf.height, conf.data );
    XStoreName( dpy, win, conf.title );


    // -------------------
    // Start the Game Loop
    // -------------------

    unsigned int previousTime = get_milliseconds();
    GEvent e;
    int done = 0;
    while( !done ) {

        if( glGetError() != GL_NO_ERROR ) g_fatal_error("OpenGL error.\n");

        if( XPending(dpy) ){
            XEvent event;

            XNextEvent(dpy, &event);
            switch (event.type) {
                case ButtonPress:
                case ButtonRelease:
                    e.type = (event.type == ButtonPress ? GE_MOUSEDOWN : GE_MOUSEUP);
                    e.x = event.xmotion.x;
                    e.y = event.xmotion.y;
                    g_handle_event( &e, conf.data );
                    break;

                case MotionNotify:
//                    if( RELATIVE ){
//                        int x = event.xmotion.x - last_x;
//                        int y = event.xmotion.y - last_y;
//                        if( !x && !y ) break;
//                        mouse_cb( x, y, TRE_MOUSE_MOVE, mouse_data );
//                        XWarpPointer( display, None, window, 0,0,0,0, last_x, last_y );
//                    } else {
//                        mouse_cb( event.xmotion.x, event.xmotion.y, TRE_MOUSE_MOVE, mouse_data );
//                    }

                    e.type = GE_MOUSEMOVE;
                    e.value = 0;
                    e.x = event.xmotion.x;
                    e.y = event.xmotion.y;
                    g_handle_event( &e, conf.data );
                    break;

                case KeyRelease: //fallthrough
                case KeyPress:
                    if( (conf.flags & GC_IGNORE_KEYREPEAT) && XEventsQueued(dpy, QueuedAfterReading) ) {
                        XEvent a; //ahead
                        XPeekEvent( dpy, &a);
                        if( a.type == KeyPress && a.xkey.window == event.xkey.window
                            && a.xkey.keycode == event.xkey.keycode && a.xkey.time == event.xkey.time) {
                            // Pop off the repeated KeyPress and ignore the auto repeated KeyRelease/KeyPress pair.
                            XNextEvent( dpy, &event);
                            break;
                        }
                    }
                    e.type = (event.type == KeyPress ? GE_KEYDOWN : GE_KEYUP);
                    e.value = keysym_to_key(XLookupKeysym(&event.xkey, 0));
                    if( !g_handle_event(&e, conf.data) ) done = 1;
                    break;
            }

        }

        unsigned int currentTime = get_milliseconds();
        unsigned int deltaTime = currentTime - previousTime;
        previousTime = currentTime;

        g_update( deltaTime, conf.data );
        g_render( conf.data );
        glXSwapBuffers( dpy, win );
    }

    g_free( conf.title );
    g_cleanup( conf.data );
    return 0;
}

#define ERR_STR_LEN 1024

void g_debug_str(const char* str, ...) {
    va_list a;
    va_start(a, str);

    char msg[ERR_STR_LEN] = {0};
    vsnprintf(msg, ERR_STR_LEN, str, a);
    fputs(msg, stderr);
}

void g_fatal_error(const char* str, ...) {
    va_list a;
    va_start(a, str);

    char msg[ERR_STR_LEN] = {0};
    vsnprintf(msg, ERR_STR_LEN, str, a);
    fputs(msg, stderr);
    exit(1);
}

// void g_debug_wstr(const wchar_t* wstr, ...) {
//     va_list a;
//     va_start(a, wstr);

//     wchar_t msg[ERR_STR_LEN] = {0};
//     vswprintf(msg, ERR_STR_LEN, wstr, a);
//     fputws(msg, stderr);
// }

// void g_fatal_werror(const wchar_t* wstr, ...) {
//     fwide(stderr, 1);

//     va_list a;
//     va_start(a, wstr);

//     wchar_t msg[ERR_STR_LEN] = {0};
//     vswprintf(msg, ERR_STR_LEN, wstr, a);
//     fputws(msg, stderr);
//     exit(1);
// }
