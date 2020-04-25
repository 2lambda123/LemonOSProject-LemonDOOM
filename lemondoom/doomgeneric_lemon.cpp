
#include <stdio.h>
#include <gfx/window/window.h>
#include <lemon/syscall.h>
#include <lemon/keyboard.h>
#include <lemon/ipc.h>
#include <ctype.h>

#define KEY_ARROW_UP 266
#define KEY_ARROW_LEFT 267
#define KEY_ARROW_RIGHT 268
#define KEY_ARROW_DOWN 269

#define _KEY_ESCAPE 27

#define KEYQUEUE_SIZE 16

static unsigned short s_KeyQueue[KEYQUEUE_SIZE];
static unsigned int s_KeyQueueWriteIndex = 0;
static unsigned int s_KeyQueueReadIndex = 0;

Window* window = nullptr;
win_info_t windowInfo;

void memcpy_optimized(void* dest, void* src, size_t count);

extern "C"{
#include "doomkeys.h"

#include "doomgeneric.h"

static unsigned char convertToDoomKey(unsigned int key)
{
	switch (key)
	{
	case '\n':
		key = KEY_ENTER;
		break;
	case _KEY_ESCAPE:
		key = KEY_ESCAPE;
		break;
	case KEY_ARROW_LEFT:
		key = KEY_LEFTARROW;
		break;
	case KEY_ARROW_RIGHT:
		key = KEY_RIGHTARROW;
		break;
	case KEY_ARROW_UP:
		key = KEY_UPARROW;
		break;
	case KEY_ARROW_DOWN:
		key = KEY_DOWNARROW;
		break;
	case /*KEY_CONTROL*/ 'z':
		key = KEY_FIRE;
		break;
	case ' ':
		key = KEY_USE;
		break;
	case /*KEY_SHIFT*/ 'x':
		key = KEY_RSHIFT;
		break;
	default:
		key = tolower(key);
		break;
	}

	return key;
}

static void addKeyToQueue(int pressed, unsigned int keyCode)
{
	unsigned char key = convertToDoomKey(keyCode);

	unsigned short keyData = (pressed << 8) | key;

	s_KeyQueue[s_KeyQueueWriteIndex] = keyData;
	s_KeyQueueWriteIndex++;
	s_KeyQueueWriteIndex %= KEYQUEUE_SIZE;
}

void DG_Init()
{
	windowInfo.x = windowInfo.y = 50;
	windowInfo.width = DOOMGENERIC_RESX;
	windowInfo.height = DOOMGENERIC_RESY;
	strcpy(windowInfo.title, "LemonDOOM");

	window = CreateWindow(&windowInfo);

	memset(s_KeyQueue, 0, KEYQUEUE_SIZE * sizeof(unsigned short));
}

void DG_DrawFrame()
{
	ipc_message_t msg;
	while(ReceiveMessage(&msg)){
		switch(msg.msg){
			case WINDOW_EVENT_KEY:
				addKeyToQueue(1, msg.data);
				break;
			case WINDOW_EVENT_KEYRELEASED:
				addKeyToQueue(0, msg.data);
				break;
			case WINDOW_EVENT_CLOSE:
				DestroyWindow(window);
				exit(0);
				break;
		}
	}

	memcpy_optimized(window->surface.buffer, DG_ScreenBuffer, DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);
	_PaintWindow(window->handle, &window->surface);
}

void DG_SleepMs(uint32_t ms)
{
	syscall(SYS_NANO_SLEEP, ms * 1000000, 0, 0, 0, 0);
}

uint32_t DG_GetTicksMs()
{
	uint64_t seconds;
	uint64_t ms;
	syscall(SYS_UPTIME, &seconds, &ms, 0, 0, 0);
	return seconds * 1000 + ms;
}

int DG_GetKey(int* pressed, unsigned char* doomKey)
{
	if (s_KeyQueueReadIndex == s_KeyQueueWriteIndex)
	{
		//key queue is empty

		return 0;
	}
	else
	{
		unsigned short keyData = s_KeyQueue[s_KeyQueueReadIndex];
		s_KeyQueueReadIndex++;
		s_KeyQueueReadIndex %= KEYQUEUE_SIZE;

		*pressed = keyData >> 8;
		*doomKey = keyData & 0xFF;

		return 1;
	}
}

void DG_SetWindowTitle(const char * title)
{
	strcpy(windowInfo.title, title);

	if(window){
		_DestroyWindow(window->handle);
		window->handle = _CreateWindow(&windowInfo);
	}
}

}