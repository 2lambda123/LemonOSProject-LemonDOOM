
#include <stdio.h>
#include <gfx/window/window.h>
#include <lemon/syscall.h>
#include <lemon/keyboard.h>
#include <lemon/ipc.h>
#include <ctype.h>
#include <gfx/window/messagebox.h>
#include <unistd.h>

#define KEY_ARROW_UP 266
#define KEY_ARROW_LEFT 267
#define KEY_ARROW_RIGHT 268
#define KEY_ARROW_DOWN 269

#define _KEY_ESCAPE 27

#define KEYQUEUE_SIZE 16

#define KEY_SHIFT 270
#define KEY_ALT 271
#define KEY_CONTROL 272

static unsigned short s_KeyQueue[KEYQUEUE_SIZE];
static unsigned int s_KeyQueueWriteIndex = 0;
static unsigned int s_KeyQueueReadIndex = 0;

Lemon::GUI::Window* window = nullptr;
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
	case KEY_CONTROL:
		key = KEY_FIRE;
		break;
	case ' ':
		key = KEY_USE;
		break;
	case KEY_SHIFT:
		key = KEY_RSHIFT;
		break;
	case ',':
		key = KEY_STRAFE_L;
		break;
	case '.':
		key = KEY_STRAFE_R;
		break;
	case KEY_ALT:
		key = KEY_LALT;
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

size_t msAtBoot;

void DG_Init()
{
	windowInfo.x = windowInfo.y = 50;
	windowInfo.width = DOOMGENERIC_RESX;
	windowInfo.height = DOOMGENERIC_RESY;
	strcpy(windowInfo.title, "LemonDOOM");

	window = Lemon::GUI::CreateWindow(&windowInfo);

	memset(s_KeyQueue, 0, KEYQUEUE_SIZE * sizeof(unsigned short));

	uint64_t seconds;
	uint64_t ms;
	syscall(SYS_UPTIME, &seconds, &ms, 0, 0, 0);
	msAtBoot = seconds * 1000 + ms;

	DG_ScreenBuffer = (uint32_t*)window->surface.buffer;
}

void DG_DrawFrame()
{
	ipc_message_t msg;
	while(Lemon::ReceiveMessage(&msg)){
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
	Lemon::GUI::SwapWindowBuffers(window);
	DG_ScreenBuffer = (uint32_t*)window->surface.buffer;
}

void DG_SleepMs(uint32_t ms)
{
	usleep(ms * 1000);
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
	if(window){
		strcpy(window->info.title, title);
		Lemon::GUI::UpdateWindow(window);
	}
}

void LemonMessageBox(char* msg){
	//Lemon::GUI::MessageBox(msg, MESSAGEBOX_OK); Stay light on dependencies

	printf("Error: %s", msg);
}

}