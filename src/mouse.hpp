#pragma once
#include "stdafx.h"

void MoveMouse(int x, int y)
{
	DWORD screenWidth = GetSystemMetrics(SM_CXSCREEN) - 1;
	DWORD screenHeight = GetSystemMetrics(SM_CYSCREEN) - 1;
	DWORD dx = x * (65535 / screenWidth);
	DWORD dy = y * (65535 / screenHeight);
	//´òÓ¡dx,dy×ø±ê
	OutputDebugStringA(std::to_string(dx).c_str());
	OutputDebugStringA(",");
	OutputDebugStringA(std::to_string(dy).c_str());
	OutputDebugStringA("\n");

	mouse_event(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE, dx, dy, 0, 0);
}

void MouseDown(int x, int y)
{
	DWORD screenWidth = GetSystemMetrics(SM_CXSCREEN) - 1;
	DWORD screenHeight = GetSystemMetrics(SM_CYSCREEN) - 1;
	DWORD dx = x * (65535 / screenWidth);
	DWORD dy = y * (65535 / screenHeight);
	mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_ABSOLUTE, dx, dy, 0, 0);
}

void MouseUp(int x, int y)
{
	DWORD screenWidth = GetSystemMetrics(SM_CXSCREEN) - 1;
	DWORD screenHeight = GetSystemMetrics(SM_CYSCREEN) - 1;
	DWORD dx = x * (65535 / screenWidth);
	DWORD dy = y * (65535 / screenHeight);
	mouse_event(MOUSEEVENTF_LEFTUP | MOUSEEVENTF_ABSOLUTE, dx, dy, 0, 0);
}