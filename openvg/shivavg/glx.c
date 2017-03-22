#include <inttypes.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <stdarg.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xatom.h>
#include<GL/gl.h>
#include<GL/glx.h>
//#include <Xm/MwmUtil.h>
#include "input.h"
#include "VG/openvg.h"
#include<stdlib.h>
#include "shgl.h"

//#include "shContext.h"
 VGboolean vgCreateContextSH(int width, int height);
 void vgResizeSurfaceSH(int width, int height);


    Display                 *dpy;
    Window                  root;

    XVisualInfo             *vi;

    XSetWindowAttributes    swa;
    Window                  win;
    GLXContext              glc;
    XWindowAttributes       gwa;
    XEvent                  xev;

    XSizeHints hints;


    uint32_t *width_ptr=NULL;
    uint32_t *heigth_ptr=NULL;


    Input_handler* input_handler=NULL;


    void mouseH(int32_t x, int32_t y) {};
    void buttonH(uint8_t btn, VGboolean state) {};
    void keyH(key_code key, VGboolean state) {};



    // Se also, https://www.opengl.org/wiki/Programming_OpenGL_in_Linux:_GLX_and_Xlib
    // http://www-f9.ijs.si/~matevz/docs/007-2392-003/sgi_html/ch03.html#LE54269-PARENT

    void init_gls(uint32_t *width,uint32_t *heigth)
    {
        GLint                   att[] = { GLX_RGBA, GLX_DOUBLEBUFFER,GLX_DEPTH_SIZE, 8,GLX_STENCIL_SIZE,8, None }; //
        Colormap                cmap;

        width_ptr=width;
        heigth_ptr=heigth;


        input_handler=(Input_handler *)malloc(sizeof(Input_handler));
        input_handler->button=buttonH;
        input_handler->key=keyH;
        input_handler->mouse=mouseH;


        XInitThreads();

        dpy = XOpenDisplay(NULL);

        if(dpy == NULL) {
           printf("\n\tcannot connect to X server\n\n");
               exit(0);
        }

        root = DefaultRootWindow(dpy);

        vi = glXChooseVisual(dpy, 0, att);

        if(vi == NULL) {
           printf("\n\tno appropriate visual found\n\n");
               exit(0);
        }
        else {
           //printf("\n\tvisual %p selected\n", (void *)vi->visualid); // %p creates hexadecimal output like in glxinfo
           //printf("\n\tvisual %p found\n", (void *)vi->visual); // %p creates hexadecimal output like in glxinfo
        }


        cmap = XCreateColormap(dpy, RootWindow(dpy, vi->screen ), vi->visual, AllocNone);
        //printf("\n\tColormap %d created\n", cmap); 


        swa.colormap = cmap;
        swa.border_pixel = 0;
        swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |  PointerMotionMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask;
        // Override window manager
        swa.override_redirect = True;


        // Find the size of the screen
        Window testw = DefaultRootWindow(dpy);
        XWindowAttributes getWinAttr;
        XGetWindowAttributes(dpy, testw, &getWinAttr);

        win = XCreateWindow(dpy, RootWindow(dpy, vi->screen ), 0, 0, *width, *heigth, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);

#ifdef FULLSCREEN
	
        //win = XCreateWindow(dpy, RootWindow(dpy, vi->screen ), 0, 0, getWinAttr.width, getWinAttr.height, 0, vi->depth, InputOutput, vi->visual, CWColormap | CWEventMask, &swa);

         *width = getWinAttr.widt;
         *height =  getWinAttr.height;


        // Attempt to sett fullscreen, Tell windowmanager

        Atom wm_state   = XInternAtom (dpy, "_NET_WM_STATE", true );
        Atom wm_fullscreen = XInternAtom (dpy, "_NET_WM_STATE_FULLSCREEN", true );

        XChangeProperty(dpy, win, wm_state, XA_ATOM, 32,
                        PropModeReplace, (unsigned char *)&wm_fullscreen, 1);

        XWMHints wmHints;


        hints.flags = USSize;
        hints.width = getWinAttr.width;
        hints.height = getWinAttr.height;

        wmHints.flags = InputHint;
        wmHints.input = True;
        char* argv[]={""};
        int argc =1;

        XmbSetWMProperties (dpy, win, "OPENVG", "OPENVG", argv, argc, &hints, &wmHints, NULL);



        // Try to set decorations to none, FULLSCREEN!
        Atom    property;
        PropMwmHints hints;


        property = XInternAtom(dpy,"_MOTIF_WM_HINTS",True);
        hints.flags |= MWM_HINTS_DECORATIONS;
        hints.decorations=0;

        if (property) {

           XChangeProperty(dpy,win,property,property,32,PropModeReplace,(unsigned char *)&hints,PROP_MOTIF_WM_HINTS_ELEMENTS);
        }

#endif
	
        XMapWindow(dpy, win);
        XStoreName(dpy, win, "OpenVG");


        // --------------
#if 0
        // Since OpenGL3 a new extension called ARB_create_context[1][2] gives a new API to create an OpenGL contex
        // https://sidvind.com/wiki/Opengl/windowless


        typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
        typedef Bool (*glXMakeContextCurrentARBProc)(Display*, GLXDrawable, GLXDrawable, GLXContext);
        static glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
        static glXMakeContextCurrentARBProc glXMakeContextCurrentARB = 0;

        int main(int argc, const char* argv[]){
                static int visual_attribs[] = {
                        None
                };
                int context_attribs[] = {
                        GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
                        GLX_CONTEXT_MINOR_VERSION_ARB, 3,
                        None
                };

                Display* dpy = XOpenDisplay(0);
                int fbcount = 0;
                GLXFBConfig* fbc = NULL;
                GLXContext ctx;
                GLXPbuffer pbuf;

                /* open display */
                if ( ! (dpy = XOpenDisplay(0)) ){
                        fprintf(stderr, "Failed to open display\n");
                        exit(1);
                }

                /* get framebuffer configs, any is usable (might want to add proper attribs) */
                if ( !(fbc = glXChooseFBConfig(dpy, DefaultScreen(dpy), visual_attribs, &fbcount) ) ){
                        fprintf(stderr, "Failed to get FBConfig\n");
                        exit(1);
                }

                /* get the required extensions */
                glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)glXGetProcAddressARB( (const GLubyte *) "glXCreateContextAttribsARB");
                glXMakeContextCurrentARB = (glXMakeContextCurrentARBProc)glXGetProcAddressARB( (const GLubyte *) "glXMakeContextCurrent");
                if ( !(glXCreateContextAttribsARB && glXMakeContextCurrentARB) ){
                        fprintf(stderr, "missing support for GLX_ARB_create_context\n");
                        XFree(fbc);
                        exit(1);
                }

                /* create a context using glXCreateContextAttribsARB */
                if ( !( ctx = glXCreateContextAttribsARB(dpy, fbc[0], 0, True, context_attribs)) ){
                        fprintf(stderr, "Failed to create opengl context\n");
                        XFree(fbc);
                        exit(1);
                }

                /* create temporary pbuffer */
                int pbuffer_attribs[] = {
                        GLX_PBUFFER_WIDTH, 800,
                        GLX_PBUFFER_HEIGHT, 600,
                        None
                };
                pbuf = glXCreatePbuffer(dpy, fbc[0], pbuffer_attribs);

                XFree(fbc);
                XSync(dpy, False);

                /* try to make it the current context */
                if ( !glXMakeContextCurrent(dpy, pbuf, pbuf, ctx) ){
                        /* some drivers does not support context without default framebuffer, so fallback on
                         * using the default window.
                         */
                        if ( !glXMakeContextCurrent(dpy, DefaultRootWindow(dpy), DefaultRootWindow(dpy), ctx) ){
                                fprintf(stderr, "failed to make current\n");
                                exit(1);
                        }
                }

                /* try it out */
                //printf("vendor  : %s\n", (const char*)glGetString(GL_VENDOR));
                printf("version : %s\n", (const char*)glGetString(GL_VERSION));

#endif



        glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);


        /* Connect context to the window */
        glXMakeCurrent(dpy, win, glc);


        XSync(dpy, False);

        //GLenum error = glewInit();
        //if (error != GLEW_OK){
        //   printf("error with glew init()\n");
        //}else{
        //    printf("glew is ok\n\n");
        //}


        //printf("vendor  : %s\n", (const char*)glGetString(GL_VENDOR));
        //printf("version : %s\n", (const char*)glGetString(GL_VERSION));


        vgCreateContextSH(*width,*heigth);


    };

    void swap_buffers()
    {
       glXSwapBuffers(dpy, win);
    }

    void shutdownVG()
    {
        glXMakeCurrent(dpy, None, NULL);
        glXDestroyContext(dpy, glc);
        XDestroyWindow(dpy, win);
        XCloseDisplay(dpy);
    };


    // Translate an X11 key code to a input key code.
    //
    int translateKey(int scancode)
    {
        int keySym;

        // Valid key code range is  [8,255], according to the Xlib manual
        if (scancode < 8 || scancode > 255)
            return(0); // KEY_UNKNOWN

    #if USE_NUMERIC_KEYS

        if (x11.xkb.available)
        {
            // Try secondary keysym, for numeric keypad keys
            // Note: This way we always force "NumLock = ON", which is intentional
            // since the returned key code should correspond to a physical
            // location.
            keySym = XkbKeycodeToKeysym(dpy, scancode, 0, 1);
            switch (keySym)
            {
                case XK_KP_0:           return KEY_KP_0;
                case XK_KP_1:           return KEY_KP_1;
                case XK_KP_2:           return KEY_KP_2;
                case XK_KP_3:           return KEY_KP_3;
                case XK_KP_4:           return KEY_KP_4;
                case XK_KP_5:           return KEY_KP_5;
                case XK_KP_6:           return KEY_KP_6;
                case XK_KP_7:           return KEY_KP_7;
                case XK_KP_8:           return KEY_KP_8;
                case XK_KP_9:           return KEY_KP_9;
                case XK_KP_Separator:
                case XK_KP_Decimal:     return KEY_KP_DECIMAL;
                case XK_KP_Equal:       return KEY_KP_EQUAL;
                case XK_KP_Enter:       return KEY_KP_ENTER;
                default:                break;
            }

            // Now try primary keysym for function keys (non-printable keys). These
            // should not be layout dependent (i.e. US layout and international
            // layouts should give the same result).
            keySym = XkbKeycodeToKeysym(dpy, scancode, 0, 0);
        }
    #endif


      int dummy;
      KeySym* keySyms;

      keySyms = XGetKeyboardMapping(dpy, scancode, 1, &dummy);
      keySym = keySyms[0];
      XFree(keySyms);


        switch (keySym)
        {
            case XK_Escape:         return KEY_ESC;
            case XK_Tab:            return KEY_TAB;
            case XK_Shift_L:        return KEY_LSHIFT;
            case XK_Shift_R:        return KEY_RSHIFT;
            case XK_Control_L:      return KEY_LCTRL;
            case XK_Control_R:      return KEY_RCTRL;
            case XK_Meta_L:
            case XK_Alt_L:          return KEY_ALT;
            case XK_Mode_switch: // Mapped to Alt_R on many keyboards
            case XK_ISO_Level3_Shift: // AltGr on at least some machines
            case XK_Meta_R:
            case XK_Alt_R:          return KEY_ALT;
            //case XK_Super_L:        return KEY_LEFT_SUPER;
            //case XK_Super_R:        return KEY_RIGHT_SUPER;
            //case XK_Menu:           return KEY_MENU;
            //case XK_Num_Lock:       return KEY_NUM_LOCK;
            //case XK_Caps_Lock:      return KEY_CAPS_LOCK;
            //case XK_Print:          return KEY_PRINT_SCREEN;
            //case XK_Scroll_Lock:    return KEY_SCROLL_LOCK;
            //case XK_Pause:          return KEY_PAUSE;
            //case XK_Delete:         return KEY_DELETE;
            //case XK_BackSpace:      return KEY_BACKSPACE;
            case XK_Return:         return KEY_RETURN;
            //case XK_Home:           return KEY_HOME;
            //case XK_End:            return KEY_END;
            //case XK_Page_Up:        return KEY_PAGE_UP;
            //case XK_Page_Down:      return KEY_PAGE_DOWN;
            //case XK_Insert:         return KEY_INSERT;
            case XK_Left:           return KEY_LEFT;
            case XK_Right:          return KEY_RIGHT;
            case XK_Down:           return KEY_DOWN;
            case XK_Up:             return KEY_UP;
            case XK_F1:             return KEY_F1;
            case XK_F2:             return KEY_F2;
            case XK_F3:             return KEY_F3;
            case XK_F4:             return KEY_F4;
            case XK_F5:             return KEY_F5;
            case XK_F6:             return KEY_F6;
            case XK_F7:             return KEY_F7;
            case XK_F8:             return KEY_F8;
            case XK_F9:             return KEY_F9;
            case XK_F10:            return KEY_F10;
            case XK_F11:            return KEY_F11;
            case XK_F12:            return KEY_F12;
            //case XK_F13:            return KEY_F13;
            //case XK_F14:            return KEY_F14;
            //case XK_F15:            return KEY_F15;
            //case XK_F16:            return KEY_F16;
            //case XK_F17:            return KEY_F17;
            //case XK_F18:            return KEY_F18;
            //case XK_F19:            return KEY_F19;
            //case XK_F20:            return KEY_F20;
            //case XK_F21:            return KEY_F21;
            //case XK_F22:            return KEY_F22;
            //case XK_F23:            return KEY_F23;
            //case XK_F24:            return KEY_F24;
            //case XK_F25:            return KEY_F25;

            // Numeric keypad
            //case XK_KP_Divide:      return KEY_KP_DIVIDE;
            //case XK_KP_Multiply:    return KEY_KP_MULTIPLY;
            //case XK_KP_Subtract:    return KEY_KP_SUBTRACT;
            //case XK_KP_Add:         return KEY_KP_ADD;

            // These should have been detected in secondary keysym test above!
            //case XK_KP_Insert:      return KEY_KP_0;
            //case XK_KP_End:         return KEY_KP_1;
            //case XK_KP_Down:        return KEY_KP_2;
            //case XK_KP_Page_Down:   return KEY_KP_3;
            //case XK_KP_Left:        return KEY_KP_4;
            //case XK_KP_Right:       return KEY_KP_6;
            //case XK_KP_Home:        return KEY_KP_7;
            //case XK_KP_Up:          return KEY_KP_8;
            //case XK_KP_Page_Up:     return KEY_KP_9;
            //case XK_KP_Delete:      return KEY_KP_DECIMAL;
            //case XK_KP_Equal:       return KEY_KP_EQUAL;
            //case XK_KP_Enter:       return KEY_KP_ENTER;

            // Last resort: Check for printable keys (should not happen if the XKB
            // extension is available). This will give a layout dependent mapping
            // (which is wrong, and we may miss some keys, especially on non-US
            // keyboards), but it's better than nothing...
            case XK_a:              return KEY_A;
            case XK_b:              return KEY_B;
            case XK_c:              return KEY_C;
            case XK_d:              return KEY_D;
            case XK_e:              return KEY_E;
            case XK_f:              return KEY_F;
            case XK_g:              return KEY_G;
            case XK_h:              return KEY_H;
            case XK_i:              return KEY_I;
            case XK_j:              return KEY_J;
            case XK_k:              return KEY_K;
            case XK_l:              return KEY_L;
            case XK_m:              return KEY_M;
            case XK_n:              return KEY_N;
            case XK_o:              return KEY_O;
            case XK_p:              return KEY_P;
            case XK_q:              return KEY_Q;
            case XK_r:              return KEY_R;
            case XK_s:              return KEY_S;
            case XK_t:              return KEY_T;
            case XK_u:              return KEY_U;
            case XK_v:              return KEY_V;
            case XK_w:              return KEY_W;
            case XK_x:              return KEY_X;
            case XK_y:              return KEY_Y;
            case XK_z:              return KEY_Z;
            case XK_1:              return KEY_1;
            case XK_2:              return KEY_2;
            case XK_3:              return KEY_3;
            case XK_4:              return KEY_4;
            case XK_5:              return KEY_5;
            case XK_6:              return KEY_6;
            case XK_7:              return KEY_7;
            case XK_8:              return KEY_8;
            case XK_9:              return KEY_9;
            case XK_0:              return KEY_0;
            case XK_space:          return KEY_SPACE;
            case XK_minus:          return KEY_MINUS;
            //case XK_equal:          return KEY_EQUAL;
            //case XK_bracketleft:    return KEY_LEFT_BRACKET;
            //case XK_bracketright:   return KEY_RIGHT_BRACKET;
            //case XK_backslash:      return KEY_BACKSLASH;
            //case XK_semicolon:      return KEY_SEMICOLON;
            //case XK_apostrophe:     return KEY_APOSTROPHE;
            //case XK_grave:          return KEY_GRAVE_ACCENT;
            //case XK_comma:          return KEY_COMMA;
            //case XK_period:         return KEY_PERIOD;
            //case XK_slash:          return KEY_SLASH;
            //case XK_less:           return KEY_WORLD_1; // At least in some layouts...
            default:                break;
        }

        // No matching translation was found
        return 0 ; /* KEY_UNKNOWN */;
    }




    VGboolean process_events() {
        VGboolean ret=VG_FALSE;

        while( XPending(dpy) ) {
            XNextEvent(dpy, &xev);

            switch (xev.type)
            {
               case Expose:
                    XGetWindowAttributes(dpy, win, &gwa);
                    glViewport(0, 0, gwa.width, gwa.height);
                    if (width_ptr)
                    {
                        *width_ptr=gwa.height;
                    }
                    if (heigth_ptr) {
                        *heigth_ptr=gwa.height;
                    }

                    vgResizeSurfaceSH(gwa.width,gwa.height);
                    glXSwapBuffers(dpy, win);
                break;

                case ConfigureNotify:
                        glViewport(0, 0, xev.xconfigure.width,
                        xev.xconfigure.height);
                        if (width_ptr)
                        {
                            *width_ptr=xev.xconfigure.width;
                        }
                        if (heigth_ptr) {
                            *heigth_ptr=xev.xconfigure.height;
                        }


                        vgResizeSurfaceSH(xev.xconfigure.width,xev.xconfigure.height);
                        glXMakeCurrent(dpy, win, glc);
                break;
               case KeyRelease:
                  {
                      int mykey = translateKey(xev.xkey.keycode);
                      if (input_handler) input_handler->key(mykey,VG_FALSE);
                      printf ("Got keyrelease\n" );
                  }
               break;
               case KeyPress:
                  {
                     int mykey = translateKey(xev.xkey.keycode);
                     if (input_handler) input_handler->key(mykey,VG_TRUE);
                     printf ("Got keypress %c\n" , xev.xkey.keycode);
                     if (mykey == KEY_ESC)
                     {
                         glXMakeCurrent(dpy, None, NULL);
                         glXDestroyContext(dpy, glc);
                         XDestroyWindow(dpy, win);
                         XCloseDisplay(dpy);
                         //exit(0);
                     }

                  }
               break;

            case ButtonPress:

                if (xev.xbutton.button == Button1)
                {
                    if (input_handler) input_handler->button(0,VG_TRUE);
                     //printf ("Got button 1\n" );
                }
                else if (xev.xbutton.button == Button2)
                {
                    if (input_handler) input_handler->button(2,VG_TRUE);
                     //printf ("Got button 2\n" );
                }
                else if (xev.xbutton.button == Button3)
                {
                    if (input_handler) input_handler->button(1,VG_TRUE);
                    //printf ("Got button 3\n" );
                }
                break;
                break;
            case ButtonRelease:
                if (xev.xbutton.button == Button1)
                {
                    if (input_handler) input_handler->button(0,VG_FALSE);
                     //printf ("Rel button 1\n" );
                }
                else if (xev.xbutton.button == Button2)
                {
                    if (input_handler) input_handler->button(2,VG_FALSE);
                     //printf ("Rel button 2\n" );
                }
                else if (xev.xbutton.button == Button3)
                {
                    if (input_handler) input_handler->button(1,VG_FALSE);
                    //printf ("Rel button 3\n" );
                }
                break;



            case MotionNotify:
                {
                    const int x = xev.xmotion.x;
                    const int y = xev.xmotion.y;

                    //printf ("Motion %d,%d\n",x,y );
                    if (input_handler) input_handler->mouse(x,y);

                }
            break;
            default:
                break;

            }

        }

        return ret;
    }







#if !defined(NOTRACE)


    void trace_handler(const char *file, const char *func, int line, const char *msg, ...)
    {
        static char bf[4096] = { 0 };

        va_list args;
        //va_start(args, msg);
        //vfprintf(stderr, msg, args);
        //va_end(args);

        va_start(args, msg);
        vsnprintf(bf, 4095, msg, args);
        va_end(args);

        bf[4095] = 0;

        printf("%s",bf);
        //trlog::write_msg(file, func, line, bf);
    }
#endif

#if !defined(NOASSERT)
    void dbgbrk()
    {
#ifdef WIN32
        __asm {int 3};
#else
        raise(SIGINT);
#endif
    }

    VGboolean assert_handler(const char *file, const char *func, int line, const char *exp)
    {
        static char bf1[1024],bf2[1024],bf3[1024];
        Window mywindow; char text[10];
        XSizeHints myhint;  KeySym mykey;
        GC mygc;  int done; ; int i;  XEvent myevent;
        Display *mydisplay;

        bf1[1023] = 0;
        bf2[1023] = 0;
        bf3[1023] = 0;

        snprintf(bf1,1023,"Assertion failed expr: %s", exp);
        snprintf(bf2,1023,"   file: %s", file);
        snprintf(bf3,1023,"   func: %s line: %d", func, line);

        //if (MessageBox(0, bf, "assertion failed", MB_RETRYCANCEL | MB_ICONERROR | MB_TASKMODAL) == IDCANCEL) { return 0; }
        unsigned long myforeground, mybackground;
        printf("%s\n%s\n%s\n",bf1,bf2,bf3);

        mydisplay = XOpenDisplay("");  // mydisplay could have been context.dpy
        int myscreen = DefaultScreen(mydisplay);
        mybackground = WhitePixel(mydisplay,myscreen);
        myforeground = BlackPixel(mydisplay,myscreen);


        /* Suggest where to position the window: */
        myhint.x = 200;
        myhint.y = 200;
        myhint.width = 400;
        myhint.height = 150;
        myhint.flags = PPosition | PSize;
        /* Create a window - not displayed yet however: */
        mywindow = XCreateSimpleWindow(mydisplay,DefaultRootWindow(mydisplay), myhint.x,myhint.y,myhint.width,myhint.height,5,myforeground,mybackground);


        const char *hello = "Assert failed, press q";
        const char *hi="Press q to quit";
        int argc=1;
        const char *argv[]={"None","Empty",0};

        XSetStandardProperties(mydisplay,mywindow,hello,hello,None,(char **)argv,argc,&myhint);

        mygc = XCreateGC(mydisplay,mywindow,0,0);
        XSetBackground(mydisplay,mygc,mybackground);
        XSetForeground(mydisplay,mygc,myforeground);
          /* Select input devices to listen to: */
        XSelectInput(mydisplay,mywindow,ButtonPressMask|KeyPressMask|ExposureMask);
         /* Actually display the window: */
        XMapRaised(mydisplay,mywindow);

               /* Main Event Loop */
           done = 0;
           while (done==0) {
               XNextEvent(mydisplay,&myevent);
               switch(myevent.type) {
               case Expose: /* Repaint window on expose */
                   if (myevent.xexpose.count==0)
                       XDrawImageString(myevent.xexpose.display,myevent.xexpose.window,
                               mygc,30,50,bf1,strlen(bf1));

                   XDrawImageString(myevent.xexpose.display,myevent.xexpose.window,
                           mygc,30,70,bf2,strlen(bf2));

                   XDrawImageString(myevent.xexpose.display,myevent.xexpose.window,
                           mygc,30,90,bf3,strlen(bf3));

                   break;
                   //case MappingNotify: /* Process keyboard mapping changes: */
                   //XRefreshKeyboardMapping(&myevent);
                   //break;
               case ButtonPress: /* Process mouse click - output Hi! at mouse: */

                     XDrawImageString(myevent.xbutton.display,myevent.xbutton.window,
                        mygc,myevent.xbutton.x,myevent.xbutton.y,hi,strlen(hi));
                     done=1;
                   break;
               case KeyPress: /* Process key press - quit on q: */
                   {
                       int dummy;
                       KeySym* keySyms;
                       keySyms = XGetKeyboardMapping(mydisplay, myevent.xkey.keycode, 1, &dummy);
                       mykey = keySyms[0];
                       XFree(keySyms);
                       if (myevent.xkey.keycode==XK_q)  done = 1;
                       if (myevent.xkey.keycode==XK_Escape)  done = 1;
                   }
                   break;
               }
           }

           printf("DONE!!!\n");

           XUnmapWindow(mydisplay,mywindow);
           XDestroySubwindows(mydisplay,mywindow);
           XDestroyWindow(mydisplay,mywindow);
           XFreeGC(mydisplay,mygc);

        return 1;
    }
#endif





void make_current()
{
    //glfwMakeContextCurrent(window);

}

void swap_buffers_gls()
{
   swap_buffers();
}

VGboolean process_events_gls()
{
    return(process_events());
}






