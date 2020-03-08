#include <Windows.h>
#include <cstdio>

#define _T TEXT

wchar_t const* wcName = _T("SampleWindowClass");

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_CLOSE:
	{
		DestroyWindow(hwnd);

		break;
	}
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		break;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		// All painting occurs here, between BeginPaint and EndPaint.

		FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

		EndPaint(hwnd, &ps);
		break;
	}

	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	return 0;
}

/*
	hInstance: handle to this module (other words, the base address of module)
	hPrevInstance: no meaning
	pCmdLine: pointer to commandlines (PWSTR - pointer to widestring)
	nCmdShow: how to show the app window (shown normally, minimized, maximized)
	WINAPI: __stdcall (calling convention)
*/
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {

	WNDCLASS wc = {};
	
	wc.lpszClassName = wcName;
	wc.hInstance = hInstance;
	wc.lpfnWndProc = WindowProc;

	if (!RegisterClass(&wc)) {
		MessageBox(NULL, _T("ERROR: cannot register the window class"), _T("ERROR"), MB_OK);
		return 1;
	}

	HWND hWnd = CreateWindowEx(
		NULL,	// Optional window styles
		wcName,	// Window class
		_T("Just a blank canvas"),	// Window text
		WS_OVERLAPPEDWINDOW,	// Window style
		// Size and position
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL,       // Parent window    
		NULL,       // Menu
		hInstance,  // Instance handle
		NULL        // Additional application data
	);
	if (hWnd == NULL) {
		MessageBox(NULL, _T("ERROR: cannot create the app window"), _T("ERROR"), MB_OK);
		return 1;
	}

	ShowWindow(hWnd, nCmdShow);


	MSG msg = { };
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}