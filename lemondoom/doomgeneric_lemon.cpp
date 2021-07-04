
#include <stdio.h>
#include <Lemon/GUI/Window.h>
#include <lemon/syscall.h>
#include <Lemon/Core/Keyboard.h>
#include <ctype.h>
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
    case 'w':
	case KEY_ARROW_UP:
		key = KEY_UPARROW;
		break;
    case 's':
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
	case 'a':
    case ',':
		key = KEY_STRAFE_L;
		break;
    case '.':
	case 'd':
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
	window = new Lemon::GUI::Window("LemonDOOM", {DOOMGENERIC_RESX, DOOMGENERIC_RESY});

	memset(s_KeyQueue, 0, KEYQUEUE_SIZE * sizeof(unsigned short));

	uint64_t seconds;
	uint64_t ms;
	syscall(SYS_UPTIME, &seconds, &ms, 0, 0, 0);
	msAtBoot = seconds * 1000 + ms;

	DG_ScreenBuffer = (uint32_t*)window->surface.buffer;
}

void DG_DrawFrame()
{
	Lemon::WindowServer::Instance()->Poll();

	Lemon::LemonEvent ev;
	while(window->PollEvent(ev)){
		switch(ev.event){
			case Lemon::EventKeyPressed:
				addKeyToQueue(1, ev.key);
				break;
			case Lemon::EventKeyReleased:
				addKeyToQueue(0, ev.key);
				break;
			case Lemon::EventWindowClosed:
				delete window;
				exit(0);
				break;
		}
	}

	window->SwapBuffers();
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
		window->SetTitle(title);
	}
}

void LemonMessageBox(char* msg){
	printf("Error: %s", msg);
}

}
