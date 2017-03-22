
#pragma once

#include "VG/openvg.h"
#include<inttypes.h>


	typedef enum 
	{
		KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
		KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M,
		KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,
		KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
                KEY_ESC, KEY_TAB, KEY_BACK, KEY_LSHIFT, KEY_RSHIFT, KEY_LCTRL, KEY_RCTRL, KEY_ALT, KEY_SPACE,  KEY_RETURN,
                KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_PLUS, KEY_MINUS
	} key_code;

       typedef void (MOUSEHANDLER) (int32_t x, int32_t y);
       typedef void (BUTTONHANDLER) (uint8_t btn, VGboolean state);
       typedef void (KEYHANDLER) (key_code keyc, VGboolean state);



        typedef struct
	{
        MOUSEHANDLER*  mouse;
        BUTTONHANDLER* button;
        KEYHANDLER*    key;

        //virtual void mouse(int32_t x, int32_t y) {}
        //virtual void button(uint8_t btn, bool state) {}
        //virtual void key(uint8_t key, bool state) {}
        } Input_handler;

