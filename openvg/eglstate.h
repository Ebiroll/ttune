#include <inttypes.h>

typedef struct {
	// Screen dimentions
	uint32_t screen_width;
	uint32_t screen_height;

	// Window dimentions
	int32_t window_x;
	int32_t window_y;
	uint32_t window_width;
	uint32_t window_height;
  
	// OpenGL|ES objects
#ifdef __arm__
	// dispman window 
	DISPMANX_ELEMENT_HANDLE_T element;

	// EGL data

	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
#endif
} STATE_T;

#ifdef __arm__
extern void oglinit(STATE_T *);
extern void dispmanMoveWindow(STATE_T *, int, int);
extern void dispmanChangeWindowOpacity(STATE_T *, unsigned int);
#endif


