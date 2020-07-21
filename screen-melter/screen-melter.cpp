// Modified by weyh
// Inspired by Napalm/MetalHead (http://www.rohitab.com/discuss/index.php?showtopic=23191)

#include "framework.h"
#include "screen-melter.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <string>

#include "CLI11.hpp"

int	meltWidth = 150,
	meltHeight = 15,
	interval = 1;
int screenX, screenY, screenW, screenH;
HHOOK keyboardHhook;

LRESULT CALLBACK KeyboardHook(int code, WPARAM wParam, LPARAM lParam)
{
	KBDLLHOOKSTRUCT* kbd = (KBDLLHOOKSTRUCT*)lParam;

	if(code < 0
	   || (kbd->flags & 0x10) // ignore injected events
	   ) return CallNextHookEx(keyboardHhook, code, wParam, lParam);

	if(WM_KEYDOWN == wParam)
		Debug::WriteLog("WM_KEYDOWN");

	return 1; // by default swallow the keys
}

LRESULT WINAPI MelterProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg)
	{
		case WM_CREATE:
			{
				HDC hdcDesktop = GetDC(HWND_DESKTOP);
				HDC hdcWindow = GetDC(hWnd);
				BitBlt(hdcWindow, screenX, screenY, screenW, screenH, hdcDesktop, 0, 0, SRCCOPY);
				ReleaseDC(hWnd, hdcWindow);
				ReleaseDC(HWND_DESKTOP, hdcDesktop);
				SetTimer(hWnd, 0, interval, NULL);
				ShowWindow(hWnd, SW_SHOW);
			}
			return 0;
		case WM_ERASEBKGND:
			return 0;
		case WM_PAINT:
			ValidateRect(hWnd, NULL);
			return 0;
		case WM_TIMER:
			{
				HDC hdcWindow = GetDC(hWnd);
				int	x = (rand() % screenW) - (meltWidth / 2),
					y = (rand() % meltHeight),
					width = (rand() % meltWidth);
				BitBlt(hdcWindow, x, y, width, screenH, hdcWindow, x, 0, SRCCOPY);
				ReleaseDC(hWnd, hdcWindow);
			}
			return 0;
		case WM_CLOSE:
			#ifndef _DEBUG
			return 0; // disable closing
			#endif
		case WM_DESTROY:
			{
				KillTimer(hWnd, 0);
				PostQuitMessage(0);
			}
			return 0;
	}

	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmdLine, int nShowCmd)
{
	CLI::App appArgs {"Creates melting like effect on users screen."};

	std::string time = "0";
	appArgs.add_option("-t,--time", time, "Sleep time before visual effect (ms)");

	bool disableInput = false;
	bool disableKeyboard = false;
	bool disableMouse = false;

	appArgs.add_flag("-I,--disable_input", disableInput, "Disable user input.");
	appArgs.add_flag("-K,--disable_keyboard", disableKeyboard, "Disable user keyboard.");
	appArgs.add_flag("-M,--disable_mouse", disableMouse, "Disable user mouse.");

	CLI11_PARSE(appArgs, __argc, __argv);

	Debug::WriteLog("sleep_for: " + time);
	Debug::WriteLog("disable_input: " + BoolToString(disableInput));
	Debug::WriteLog("disable_keyboard: " + BoolToString(disableKeyboard));
	Debug::WriteLog("disable_mouse: " + BoolToString(disableMouse));

	std::this_thread::sleep_for(std::chrono::milliseconds(std::stoi(time)));

	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);

	screenX = GetSystemMetrics(SM_XVIRTUALSCREEN); // a |----------------------|
	screenY = GetSystemMetrics(SM_YVIRTUALSCREEN); // b |a:b       |		   |
	screenW = GetSystemMetrics(SM_CXVIRTUALSCREEN);// c |          |        c:d|
	screenH = GetSystemMetrics(SM_CYVIRTUALSCREEN);// d |----------------------|

	WNDCLASS wndClass = {0, MelterProc, 0, 0, hInstance,
		LoadIcon(NULL, IDI_APPLICATION), LoadCursor(NULL, NULL),
		(HBRUSH)(COLOR_BACKGROUND + 1), 0, L"Melter"};

	if(!RegisterClass(&wndClass))
		return MessageBoxA(HWND_DESKTOP, "Cannot register class!", NULL, MB_ICONERROR | MB_OK);

	HWND hWnd = CreateWindowA("Melter", NULL, WS_POPUP, screenX, screenY, screenW, screenH, HWND_DESKTOP, NULL, hInstance, NULL);
	if(!hWnd)
		return MessageBoxA(HWND_DESKTOP, "Cannot create window!", NULL, MB_ICONERROR | MB_OK);

	#ifndef _DEBUG
	if(disableKeyboard || disableInput)
		keyboardHhook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHook, hInstance, 0);

	if(disableMouse || disableInput)
		ShowCursor(false);
	#endif

	srand(GetTickCount());
	MSG Msg = {0};
	while(Msg.message != WM_QUIT)
	{
		if(PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
		#ifdef _DEBUG
		if(GetAsyncKeyState(VK_ESCAPE) & 0x8000)
			DestroyWindow(hWnd);
		#endif

		#ifndef _DEBUG
		if(disableMouse || disableInput)
			SetCursorPos(0, 0);
		#endif
	}

	return 0;
}
