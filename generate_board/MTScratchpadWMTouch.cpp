// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.

// MTScratchpadWMTouch application.
// Description:
//  Inside the application window, user can draw using multiple fingers
//  at the same time. The trace of each finger is drawn using different
//  color. The primary finger trace is always drawn in black, and the
//  remaining traces are drawn by rotating through the following colors:
//  red, blue, green, magenta, cyan and yellow.
//
// Purpose:
//  This sample demonstrates handling of the multi-touch input inside
//  a Win32 application using WM_TOUCH window message:
//  - Registering a window for multi-touch using RegisterTouchWindow,
//    IsTouchWindow.
//  - Handling WM_TOUCH messages and unpacking their parameters using
//    GetTouchInputInfo and CloseTouchInputHandle; reading touch contact
//    data from the TOUCHINPUT structure.
//  - Unregistering a window for multi-touch using UnregisterTouchWindow.
//  In addition, the sample also shows how to store and draw strokes
//  entered by user, using the helper classes CStroke and
//  CStrokeCollection.
//
// MTScratchpadWMTouch.cpp : Defines the entry point for the application.

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <assert.h>
#include <string>
#include <atlstr.h>
#define ASSERT assert

#include "resource.h"
#include "Stroke.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE g_hInst;                              // Current module instance
WCHAR g_wszTitle[MAX_LOADSTRING];               // The title bar text
WCHAR g_wszWindowClass[MAX_LOADSTRING];         // The main window class name
CStrokeCollection g_StrkColFinished;            // Finished strokes, the finger has been lifted
CStrokeCollection g_StrkColDrawing;             // Strokes that are currently being drawn
int g_titleBarHeight = 0;
FILE* g_fp = fopen("generate.txt", "w+");

static inline void dprintf(const char* format, ...)
{
    va_list vlArgs = NULL;

    va_start(vlArgs, format);
    size_t nLen = _vscprintf(format, vlArgs) + 1;
    char* strBuffer = new char[nLen];
    _vsnprintf_s(strBuffer, nLen, nLen, format, vlArgs);
    va_end(vlArgs);

    OutputDebugStringA(strBuffer);

    delete[] strBuffer;
}

///////////////////////////////////////////////////////////////////////////////
// Drawing and WM_TOUCH helpers

// Returns color for the newly started stroke.
// in:
//      bPrimaryContact     boolean, whether the contact is the primary contact
// returns:
//      COLORREF, color of the stroke
COLORREF GetTouchColor(bool bPrimaryContact)
{
    static int g_iCurrColor = 0;    // Rotating secondary color index
    static COLORREF g_arrColor[] =  // Secondary colors array
    {
        RGB(255, 0, 0),             // Red
        RGB(0, 255, 0),             // Green
        RGB(0, 0, 255),             // Blue
        RGB(0, 255, 255),           // Cyan
        RGB(255, 0, 255),           // Magenta
        RGB(255, 255, 0)            // Yellow
    };

    COLORREF color;
    bPrimaryContact = false;
    if (bPrimaryContact)
    {
        // The primary contact is drawn in black.
        color = RGB(0, 0, 0);         // Black
    }
    else
    {
        // Take current secondary color.
        color = g_arrColor[g_iCurrColor];

        // Move to the next color in the array.
        g_iCurrColor = (g_iCurrColor + 1) % (sizeof(g_arrColor) / sizeof(g_arrColor[0]));
    }

    return color;
}

// Extracts contact point in client area coordinates (pixels) from a TOUCHINPUT
// structure. TOUCHINPUT structure uses 100th of a pixel units.
// in:
//      hWnd        window handle
//      ti          TOUCHINPUT structure (info about contact)
// returns:
//      POINT with contact coordinates
POINT GetTouchPoint(HWND hWnd, const TOUCHINPUT& ti)
{
    POINT pt;
    pt.x = ti.x / 100;
    pt.y = ti.y / 100;
    ScreenToClient(hWnd, &pt);
    return pt;
}

inline int GetTouchContactID(const TOUCHINPUT& ti)
{
    return ti.dwID;
}

void OnTouchDownHandler(HWND hWnd, const TOUCHINPUT& ti, char* info)
{
    POINT pt = GetTouchPoint(hWnd, ti);
    int iCursorId = GetTouchContactID(ti);
    ASSERT(g_StrkColDrawing.FindStrokeById(iCursorId) == -1);

    CStroke* pStrkNew = new CStroke;
    pStrkNew->Add(pt);
    pStrkNew->SetColor(GetTouchColor((ti.dwFlags & TOUCHEVENTF_PRIMARY) != 0));
    pStrkNew->SetId(iCursorId);

    g_StrkColDrawing.Add(pStrkNew);
    // 要在g_StrkColDrawing.Add之后打印，不然ID是-1
    sprintf(info, "%s{LeftDown %d,%d %d}", info, pt.x, pt.y + g_titleBarHeight, g_StrkColDrawing.FindStrokeById(iCursorId));
}

void OnTouchMoveHandler(HWND hWnd, const TOUCHINPUT& ti, char* info)
{
    int iCursorId = GetTouchContactID(ti);
    int iStrk = g_StrkColDrawing.FindStrokeById(iCursorId);
    ASSERT((iStrk >= 0) && (iStrk < g_StrkColDrawing.Count()));

    POINT pt;
    pt = GetTouchPoint(hWnd, ti);
    int id = iStrk + g_StrkColFinished.Count();
    sprintf(info, "%s{MoveTo %d,%d %d}", info, pt.x, pt.y + g_titleBarHeight, id);

    g_StrkColDrawing[iStrk]->Add(pt);

    HDC hDC = GetDC(hWnd);
    g_StrkColDrawing[iStrk]->DrawLast(hDC);
    ReleaseDC(hWnd, hDC);
}

void OnTouchUpHandler(HWND hWnd, const TOUCHINPUT& ti, char* info)
{
    int iCursorId = GetTouchContactID(ti);
    POINT pt;
    pt = GetTouchPoint(hWnd, ti);
    int iStrk = g_StrkColDrawing.FindStrokeById(iCursorId);
    ASSERT((iStrk >= 0) && (iStrk < g_StrkColDrawing.Count()));

    g_StrkColFinished.Add(g_StrkColDrawing[iStrk]);

    // 要在g_StrkColFinished之后添加，不然ID全是0
    sprintf(info, "%s{LeftUp %d,%d %d}", info, pt.x, pt.y + g_titleBarHeight, g_StrkColFinished.FindStrokeById(iCursorId));
    g_StrkColDrawing.Remove(iStrk);
    InvalidateRect(hWnd, NULL, FALSE);
}

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(HINSTANCE hInstance,
    HINSTANCE /* hPrevInstance */,
    LPWSTR    /* lpCmdLine */,
    int       nCmdShow)
{
    MSG msg;

    LoadString(hInstance, IDS_APP_TITLE, g_wszTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_MTSCRATCHPADWMTOUCH, g_wszWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, NULL, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = 0;
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = g_wszWindowClass;
    wcex.hIconSm = 0;

    return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd;

    g_hInst = hInstance;

    hWnd = CreateWindow(g_wszWindowClass, g_wszTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
    if (!hWnd)
    {
        return FALSE;
    }

    if (!RegisterTouchWindow(hWnd, 0))
    {
        MessageBox(hWnd, L"Cannot register application window for multi-touch input", L"Error", MB_OK);
        return FALSE;
    }
    ASSERT(IsTouchWindow(hWnd, NULL));

    ShowWindow(hWnd, SW_SHOWMAXIMIZED);
    UpdateWindow(hWnd);

    TITLEBARINFOEX info;
    info.cbSize = sizeof(TITLEBARINFOEX);
    SendMessage(hWnd, WM_GETTITLEBARINFOEX, 0, (LPARAM)&info);
    g_titleBarHeight = info.rcTitleBar.bottom - info.rcTitleBar.top;

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId;
    PAINTSTRUCT ps;
    HDC hdc;
    
    switch (message)
    {
    case WM_COMMAND:
        wmId = LOWORD(wParam);
        switch (wmId)
        {
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;

    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        g_StrkColFinished.Draw(hdc);
        g_StrkColDrawing.Draw(hdc);
        EndPaint(hWnd, &ps);
        break;
    case WM_TOUCH:
    {
        unsigned int numInputs = (unsigned int)wParam;
        TOUCHINPUT* ti = new TOUCHINPUT[numInputs];
        if (ti == NULL)
        {
            break;
        }
        if (GetTouchInputInfo((HTOUCHINPUT)lParam, numInputs, ti, sizeof(TOUCHINPUT)))
        {
            char info[MAX_PATH];
            ZeroMemory(info, MAX_PATH);
            for (unsigned int i = 0; i < numInputs; ++i)
            {
                if (ti[i].dwFlags & TOUCHEVENTF_DOWN)
                {
                    OnTouchDownHandler(hWnd, ti[i], info);
                }
                else if (ti[i].dwFlags & TOUCHEVENTF_MOVE)
                {
                    OnTouchMoveHandler(hWnd, ti[i], info);
                }
                else if (ti[i].dwFlags & TOUCHEVENTF_UP)
                {
                    OnTouchUpHandler(hWnd, ti[i], info);
                }
            }
			sprintf(info, "%s{Delay 10}\n", info/*, numInputs*/);
            fprintf(g_fp, info);
            fflush(g_fp);
        }
        CloseTouchInputHandle((HTOUCHINPUT)lParam);
        delete[] ti;
    }
    break;

    case WM_DESTROY:
        if (!UnregisterTouchWindow(hWnd))
        {
            MessageBox(NULL, L"Cannot unregister application window for touch input", L"Error", MB_OK);
        }
        ASSERT(!IsTouchWindow(hWnd, NULL));
        {
            int i;
            for (i = 0; i < g_StrkColDrawing.Count(); ++i)
            {
                delete g_StrkColDrawing[i];
            }
            for (i = 0; i < g_StrkColFinished.Count(); ++i)
            {
                delete g_StrkColFinished[i];
            }
        }
        fclose(g_fp);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}