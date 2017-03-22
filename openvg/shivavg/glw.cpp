#include <stdio.h>
#include <stdlib.h>
#include <process.h>
//#include <GL/GL.h>
#include <windows.h>
#include <windowsx.h>
#include <vector>
#include "input.h"
#include <iostream>

#pragma comment(lib, "opengl32")
#pragma comment(lib, "glu32")
#include <gl/gl.h>
//#include <GL/wglext.h>
//#include "GL/glwew.h"
//#include <gl/glu.h>

extern "C" bool vgCreateContextSH(int width, int height);

#ifdef _DEBUG
bool TRACE(TCHAR *format, ...)
{
	TCHAR buffer[1000];

	va_list argptr;
	va_start(argptr, format);
	wvsprintf(buffer, format, argptr);
	va_end(argptr);

	OutputDebugString(buffer);

	return true;
}
#else
#define TRACE(TCHAR *format, ...) { }
#endif

#ifndef WGL_NUMBER_PIXEL_FORMATS_ARB
#define WGL_NUMBER_PIXEL_FORMATS_ARB            0x2000
#define WGL_DRAW_TO_WINDOW_ARB                  0x2001
#define WGL_DRAW_TO_BITMAP_ARB                  0x2002
#define WGL_ACCELERATION_ARB                    0x2003
#define WGL_NEED_PALETTE_ARB                    0x2004
#define WGL_NEED_SYSTEM_PALETTE_ARB             0x2005
#define WGL_SWAP_LAYER_BUFFERS_ARB              0x2006
#define WGL_SWAP_METHOD_ARB                     0x2007
#define WGL_NUMBER_OVERLAYS_ARB                 0x2008
#define WGL_NUMBER_UNDERLAYS_ARB                0x2009
#define WGL_TRANSPARENT_ARB                     0x200A
#define WGL_TRANSPARENT_RED_VALUE_ARB           0x2037
#define WGL_TRANSPARENT_GREEN_VALUE_ARB         0x2038
#define WGL_TRANSPARENT_BLUE_VALUE_ARB          0x2039
#define WGL_TRANSPARENT_ALPHA_VALUE_ARB         0x203A
#define WGL_TRANSPARENT_INDEX_VALUE_ARB         0x203B
#define WGL_SHARE_DEPTH_ARB                     0x200C
#define WGL_SHARE_STENCIL_ARB                   0x200D
#define WGL_SHARE_ACCUM_ARB                     0x200E
#define WGL_SUPPORT_GDI_ARB                     0x200F
#define WGL_SUPPORT_OPENGL_ARB                  0x2010
#define WGL_DOUBLE_BUFFER_ARB                   0x2011
#define WGL_STEREO_ARB                          0x2012
#define WGL_PIXEL_TYPE_ARB                      0x2013
#define WGL_COLOR_BITS_ARB                      0x2014
#define WGL_RED_BITS_ARB                        0x2015
#define WGL_RED_SHIFT_ARB                       0x2016
#define WGL_GREEN_BITS_ARB                      0x2017
#define WGL_GREEN_SHIFT_ARB                     0x2018
#define WGL_BLUE_BITS_ARB                       0x2019
#define WGL_BLUE_SHIFT_ARB                      0x201A
#define WGL_ALPHA_BITS_ARB                      0x201B
#define WGL_ALPHA_SHIFT_ARB                     0x201C
#define WGL_ACCUM_BITS_ARB                      0x201D
#define WGL_ACCUM_RED_BITS_ARB                  0x201E
#define WGL_ACCUM_GREEN_BITS_ARB                0x201F
#define WGL_ACCUM_BLUE_BITS_ARB                 0x2020
#define WGL_ACCUM_ALPHA_BITS_ARB                0x2021
#define WGL_DEPTH_BITS_ARB                      0x2022
#define WGL_STENCIL_BITS_ARB                    0x2023
#define WGL_AUX_BUFFERS_ARB                     0x2024
#define WGL_NO_ACCELERATION_ARB                 0x2025
#define WGL_GENERIC_ACCELERATION_ARB            0x2026
#define WGL_FULL_ACCELERATION_ARB               0x2027
#define WGL_SWAP_EXCHANGE_ARB                   0x2028
#define WGL_SWAP_COPY_ARB                       0x2029
#define WGL_SWAP_UNDEFINED_ARB                  0x202A
#define WGL_TYPE_RGBA_ARB                       0x202B
#define WGL_TYPE_COLORINDEX_ARB                 0x202C
#define WGL_SAMPLE_BUFFERS_ARB                  0x2041
#define WGL_SAMPLES_ARB                         0x2042
#endif

#ifndef WGL_ARB_create_context
#define WGL_CONTEXT_DEBUG_BIT_ARB      0x00000001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002
#define WGL_CONTEXT_MAJOR_VERSION_ARB  0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB  0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB    0x2093
#define WGL_CONTEXT_FLAGS_ARB          0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB   0x9126
#define ERROR_INVALID_VERSION_ARB      0x2095
#endif

#define GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB               0x8242

typedef HGLRC(WINAPI * PFNWGLCREATECONTEXTATTRIBSARBPROC) (HDC hDC,
	HGLRC
	hShareContext,
	const int
	*attribList);

typedef bool (WINAPI * PFNWGLCHOOSEPIXELFORMATPROC) (HDC, const int *, const float *, unsigned int, int *, unsigned int *);

typedef GLvoid(APIENTRY *GLDEBUGPROCARB)(GLenum source, GLenum type,
	GLuint id, GLenum severity, GLsizei length,
	const char* message, GLvoid* userParam);

typedef GLvoid(APIENTRY * PFGLDEBUGMESSAGECALLBACKARB)(GLDEBUGPROCARB callback, GLvoid* userParam);

PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
PFNWGLCHOOSEPIXELFORMATPROC       wglChoosePixelFormatARB;
PFGLDEBUGMESSAGECALLBACKARB       glDebugMessageCallbackARB;



// ARB_create_context_profile
#define	WGL_CONTEXT_CORE_PROFILE_BIT_ARB   0x00000001	
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB  0x00000002	
#define	WGL_CONTEXT_ES2_PROFILE_BIT_EXT   0x00000004


	void init_gl()
	{
		PIXELFORMATDESCRIPTOR pfd;
		ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));

		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;

		HWND wnd = CreateWindow("STATIC", "", 0, 0, 0, 1, 1, 0, 0, GetModuleHandle(0), 0);
		HDC dc = GetDC(wnd);

		int pf = ChoosePixelFormat(dc, &pfd);
		SetPixelFormat(dc, pf, &pfd);

		HGLRC rc = wglCreateContext(dc);

		wglMakeCurrent(dc, rc);
		//if (glewInit() != GLEW_OK) {
		//	TRACE("Failed to initiate glew\n");
		//}
		std::cout << "OpenGL Version : " << glGetString(GL_VERSION) << std::endl;

		//MessageBoxA(0, (char*)glGetString(GL_VERSION), "OPENGL VERSION", 0);

		wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATPROC)wglGetProcAddress("wglChoosePixelFormatARB");
		wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

		if (wglGetCurrentContext() == NULL) {
			MessageBoxA(0, (char*)glGetString(GL_VERSION), "Opengl context lost", 0);
		}
		if (wglChoosePixelFormatARB == NULL) {
			// 
			MessageBoxA(0, (char*)glGetString(GL_VERSION), "wglChoosePixelFormatARB NULL :-P", 0);

		}

		

		wglMakeCurrent(0, 0);

		wglDeleteContext(rc);
		ReleaseDC(wnd, dc);
		DestroyWindow(wnd);
	}

#define GL_DEBUG_SOURCE_API             5
#define GL_DEBUG_SOURCE_WINDOW_SYSTEM   6
#define GL_DEBUG_SOURCE_SHADER_COMPILER 7
#define GL_DEBUG_SOURCE_THIRD_PARTY     8
#define GL_DEBUG_SOURCE_APPLICATION     9
#define GL_DEBUG_SOURCE_OTHER           10


	const char* gl_dbg_source(GLenum source)
	{
		switch (source) {
		case GL_DEBUG_SOURCE_API:
			return "API";
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
			return "WINDOW_SYSTEM";
		case GL_DEBUG_SOURCE_SHADER_COMPILER:
			return "SHADER_COMPILER";
		case GL_DEBUG_SOURCE_THIRD_PARTY:
			return "THIRD_PARTY";
		case GL_DEBUG_SOURCE_APPLICATION:
			return "APPLICATION";
		case GL_DEBUG_SOURCE_OTHER:
			return "OTHER";
		}
		return "UNKNOWN";
	}

	static const char* TypeStrings[] =
	{
		"Error",                // GL_DEBUG_TYPE_ERROR
		"Deprecated",           // GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR
		"UndefinedBehavior",    // GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR
		"Portability",          // GL_DEBUG_TYPE_PORTABILITY
		"Performance",          // GL_DEBUG_TYPE_PERFORMANCE
		"Other"                 // GL_DEBUG_TYPE_OTHER
	};

#define GL_DEBUG_TYPE_ERROR 5
#define GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR 6
#define GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR 7
#define GL_DEBUG_TYPE_PORTABILITY 8
#define GL_DEBUG_TYPE_PERFORMANCE 9
#define GL_DEBUG_TYPE_OTHER 10


	const char* gl_dbg_type(GLenum type)
	{
		switch (type) {
		case GL_DEBUG_TYPE_ERROR:
			return "ERROR";
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			return "DEPRECATED_BEHAVIOR";
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
			return "UNDEFINED_BEHAVIOR";
		case GL_DEBUG_TYPE_PORTABILITY:
			return "PORTABILITY";
		case GL_DEBUG_TYPE_PERFORMANCE:
			return "PERFORMANCE";
		case GL_DEBUG_TYPE_OTHER:
			return "OTHER";
		}
		return "UNKNOWN";
	}
	/*
	(GLenum source, GLenum type,
	GLuint id, GLenum severity, GLsizei length,
	const char* message, GLvoid* userParam);
	*/
	void APIENTRY gl_debug_msg_proc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char *message, void *param)
	{
		param;
		length;
		severity;
		id;
		TRACE("OPENGL: [%s][%s] %s\n", gl_dbg_source(source), gl_dbg_type(type), message);
	}


	LRESULT WINAPI wnd_proc(HWND wnd, UINT msg, WPARAM wpar, LPARAM lpar);

	struct System_context
	{
		HGLRC glrc;
		HWND wnd;
		HDC dc;
		Input_handler* input_handler;
		uint8_t keys[256];
		bool _ui_key;

		//std::vector<DBG_ui::Event> _ui_events;

		void init()
		{
			input_handler = 0;
			init_keytable();

			//_ui_events.reserve(40);

			_ui_key = 0;

			WNDCLASS wc = { 0 };
			wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
			wc.lpfnWndProc = wnd_proc;
			wc.hInstance = GetModuleHandle(0);
			wc.hIcon = LoadIcon(0, IDI_WINLOGO);
			wc.hCursor = LoadCursor(0, IDC_ARROW);
			wc.lpszClassName = "shiva_wc";

			if (!RegisterClass(&wc)) {
				TRACE("Failed to register window class\n");
				return;
			}

			init_gl();

			DWORD style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW;
			DWORD xstyle = WS_EX_APPWINDOW;

			PIXELFORMATDESCRIPTOR pfd = {
				sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
				PFD_TYPE_RGBA, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, PFD_MAIN_PLANE, 0, 0, 0, 0
			};



			if (0/*fulscreen*/) {
				style = WS_POPUP | WS_MAXIMIZE;
				xstyle |= WS_EX_TOPMOST;

				DEVMODE deviceMode = { 0 };
				deviceMode.dmSize = sizeof(DEVMODE);
				EnumDisplaySettings(0, ENUM_CURRENT_SETTINGS, &deviceMode);

				if (ChangeDisplaySettings(&deviceMode, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) {
					TRACE("Failed to set fullscreen window");
				}
			}

			RECT wr = { 0, 0, 800, 600 };
			AdjustWindowRectEx(&wr, style, 0, xstyle);
			int w = wr.right - wr.left;
			int h = wr.bottom - wr.top;

			wnd = CreateWindowEx(xstyle, wc.lpszClassName, "OpenVG", style, 10, 10, w, h, 0, 0, wc.hInstance, 0);
			if (!wnd) {
				TRACE("Failed to create system window!\n");
				return;
			}

			dc = GetDC(wnd);

			int pixelFormat;
			BOOL valid;
			UINT numFormats;
			int iAttributes[] = {
				WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
				WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
				WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
				WGL_COLOR_BITS_ARB, 24,
				WGL_ALPHA_BITS_ARB, 8,
				WGL_DEPTH_BITS_ARB, 24,
				WGL_STENCIL_BITS_ARB, 8,
				WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
				0, 0 };

			    // WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
				// WGL_SAMPLES_ARB, 4,                        // Check For 4x Multisampling


			if (wglChoosePixelFormatARB == NULL)
			{
				if (wglGetCurrentContext() == NULL) {
					// 
					MessageBoxA(0, (char*)glGetString(GL_VERSION), "Context lost", 0);

				}


				MessageBoxA(0, (char*)glGetString(GL_VERSION), "Cannot find extension, wglChoosePixelFormatARB", 0);
			}
			else
			{

				valid = wglChoosePixelFormatARB(dc, iAttributes, 0, 1, &pixelFormat, &numFormats);

				int pf = ChoosePixelFormat(dc, &pfd);
				BOOL ok = SetPixelFormat(dc, pixelFormat, &pfd);

			}

			int attr[] = {
				WGL_CONTEXT_MAJOR_VERSION_ARB, 1,
				WGL_CONTEXT_MINOR_VERSION_ARB, 2,
				WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
				WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
				WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
				0
			};

			                                                                 
			if (wglCreateContextAttribsARB == NULL)
			{
				MessageBoxA(0, (char*)glGetString(GL_VERSION), "Cannot find extension, wglCreateContextAttribsARB", 0);
				return;
			}


			glrc = wglCreateContextAttribsARB(dc, 0, attr);

			/*
			for (int i = 0; i < 2; ++i) {
			GLuint swap_group = 1;
			if (!wglJoinSwapGroupNV(context.channels[i].dc, swap_group)) {
			TRACE("ERROR: failed to join sawp group");
			}
			if (!wglBindSwapBarrierNV(swap_group, 1)) {
			TRACE("ERROR: failed to bind sawp barrier");
			}
			}
			*/

			// set channel 0 to current so graphics can initiate correctly
			wglMakeCurrent(dc, glrc);

			glDebugMessageCallbackARB = (PFGLDEBUGMESSAGECALLBACKARB) wglGetProcAddress("glDebugMessageCallbackARB");
			if (glDebugMessageCallbackARB == NULL)
			{
				MessageBoxA(0, (char*)glGetString(GL_VERSION), "Unimplemented glDebugMessageCallbackARB", 0);
			}
			else
			{
				glDebugMessageCallbackARB(&gl_debug_msg_proc, 0);
				glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
			}

			ShowWindow(wnd, SW_SHOW);
			UpdateWindow(wnd);

			vgCreateContextSH(800, 600);
		}

		void init_keytable()
		{
			memset(keys, 0xFF, 256);

			keys[0x20] = KEY_SPACE;

			keys[0x30] = KEY_0;
			keys[0x31] = KEY_1;
			keys[0x32] = KEY_2;
			keys[0x33] = KEY_3;
			keys[0x34] = KEY_4;
			keys[0x35] = KEY_5;
			keys[0x36] = KEY_6;
			keys[0x37] = KEY_7;
			keys[0x38] = KEY_8;
			keys[0x39] = KEY_9;

			keys[0x41] = KEY_A;
			keys[0x42] = KEY_B;
			keys[0x43] = KEY_C;
			keys[0x44] = KEY_D;
			keys[0x45] = KEY_E;
			keys[0x46] = KEY_F;
			keys[0x47] = KEY_G;
			keys[0x48] = KEY_H;
			keys[0x49] = KEY_I;
			keys[0x4A] = KEY_J;
			keys[0x4B] = KEY_K;
			keys[0x4C] = KEY_L;
			keys[0x4D] = KEY_M;
			keys[0x4E] = KEY_N;
			keys[0x4F] = KEY_O;
			keys[0x50] = KEY_P;
			keys[0x51] = KEY_Q;
			keys[0x52] = KEY_R;
			keys[0x53] = KEY_S;
			keys[0x54] = KEY_T;
			keys[0x55] = KEY_U;
			keys[0x56] = KEY_V;
			keys[0x57] = KEY_W;
			keys[0x58] = KEY_X;
			keys[0x59] = KEY_Y;
			keys[0x5A] = KEY_Z;

			keys[0x70] = KEY_F1;
			keys[0x71] = KEY_F2;
			keys[0x72] = KEY_F3;
			keys[0x73] = KEY_F4;
			keys[0x74] = KEY_F5;
			keys[0x75] = KEY_F6;
			keys[0x76] = KEY_F7;
			keys[0x77] = KEY_F8;
			keys[0x78] = KEY_F9;
			keys[0x79] = KEY_F10;
			keys[0x7A] = KEY_F11;
			keys[0x7B] = KEY_F12;

			keys[0xA0] = KEY_LSHIFT;
			keys[0xA1] = KEY_RSHIFT;
			keys[0xA2] = KEY_LCTRL;
			keys[0xA3] = KEY_RCTRL;
		}
	};

	static System_context context;

	uint8_t translate_key(WPARAM wp, LPARAM lp)
	{
		uint8_t vk;
		switch (wp) {
		case VK_SHIFT:
			vk = MapVirtualKey((lp & 0x00ff0000) >> 16, MAPVK_VSC_TO_VK_EX);
			break;
		case VK_CONTROL:
			vk = (lp & 0x01000000) ? VK_RCONTROL : VK_LCONTROL;
			break;
		default:
			vk = wp;
		}
		return context.keys[vk];
	}

	LRESULT WINAPI wnd_proc(HWND wnd, UINT msg, WPARAM wpar, LPARAM lpar)
	{
		//DBG_ui::Event uiev;

		switch (msg) {
		case WM_CREATE:
			//myapp = reinterpret_cast<base::app*>(
			//	reinterpret_cast<const CREATESTRUCT*>(lParam)->lpCreateParams);
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		case WM_PAINT:
			ValidateRect(wnd, 0);
			break;
		case WM_MOUSEMOVE:
			if (context.input_handler) {
				context.input_handler->mouse(GET_X_LPARAM(lpar), GET_Y_LPARAM(lpar));
			}
			return 0;
		case WM_KEYDOWN:
			if (wpar == VK_F12 && context._ui_key == false) {
				context._ui_key = true;
			}
			else if (context.input_handler) {
				context.input_handler->key(translate_key(wpar, lpar), 1);
			}
			return 0;
		case WM_KEYUP:

			if (wpar == VK_F12 && context._ui_key == true) {
				context._ui_key = false;
			}
			else if (context.input_handler) {
				context.input_handler->key(translate_key(wpar, lpar), 0);
			}
			return 0;
		case WM_CHAR:
			return 0;
		case WM_LBUTTONDOWN:
			SetCapture(wnd);
			if (context.input_handler) {
				context.input_handler->button(0, 1);
			}
			return 0;
		case WM_LBUTTONUP:
			ReleaseCapture();

			if (context.input_handler) {
				context.input_handler->button(0, 0);
			}
			return 0;
		case WM_RBUTTONDOWN:
			SetCapture(wnd);
			if (context.input_handler) {
				context.input_handler->button(1, 1);
			}
			return 0;
		case WM_RBUTTONUP:
			ReleaseCapture();
			if (context.input_handler) {
				context.input_handler->button(1, 0);
			}
			return 0;
		case WM_MBUTTONDOWN:
			SetCapture(wnd);
			if (context.input_handler) {
				context.input_handler->button(2, 1);
			}
			return 0;
		case WM_MBUTTONUP:
			ReleaseCapture();
			if (context.input_handler) {
				context.input_handler->button(2, 0);
			}
			return 0;
		case WM_ACTIVATEAPP:
			//myapp->activated(wParam == TRUE);
			break;
		}

		return DefWindowProc(wnd, msg, wpar, lpar);
	}

	extern "C" void init_gls()
	{
		context.init();
	}

	extern "C" void shutdown_gls()
	{

	}

	extern "C" void make_current_gls()
	{
		wglMakeCurrent(context.dc, context.glrc);
	}

	extern "C" void swap_buffers_gls()
	{
		SwapBuffers(context.dc);
	}

	extern "C" bool process_events_gls()
	{
		MSG msg = { 0 };
		bool exit = false;
		while (PeekMessage(&msg, 0, 0U, 0U, PM_REMOVE) != 0) {
			if (msg.message == WM_QUIT) { exit = true; }
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return exit;
	}

	extern "C" void set_input_handler(Input_handler* handler)
	{
		context.input_handler = handler;
	}
