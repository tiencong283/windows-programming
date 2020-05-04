#include <Windows.h>
#include <psapi.h>

#define BUFSIZE 1024
#define MAX_PROCESS 1024
#define MAX_TASKBAR 2

static HMODULE hHookLib = NULL;
static HOOKPROC pHookProc = NULL;

// process ID of explorer.exe
static DWORD explorerPid = 0;
// thread ID owner of the taskbar
static DWORD explorerTid = 0;

static HWND taskbarHandles[MAX_TASKBAR] = {NULL};
static int numOfTaskbarHanles = 0;

// print error dialog box
void printError(wchar_t const* msg) {
	MessageBox(NULL, msg, L"ERROR", MB_OK);
}

// enumerate all taskbar handles
BOOL CALLBACK enumWindowsProc(HWND hWnd, LPARAM lParam) {
	DWORD pid = 0;
	DWORD tid = 0;
	wchar_t className[BUFSIZE];

	// filter processID
	tid = GetWindowThreadProcessId(hWnd, &pid);
	if (pid != explorerPid) {
		return TRUE;
	}
	// filter "Shell_TrayWnd" || "Shell_SecondaryTrayWnd"
	if (!GetClassName(hWnd, className, BUFSIZE)) {
		return TRUE;
	}
	if (numOfTaskbarHanles < MAX_TASKBAR && wcsncmp(className, L"Shell_", wcslen(L"Shell_")) == 0) {
		explorerTid = tid;
		taskbarHandles[numOfTaskbarHanles++] = hWnd;
	}
	return numOfTaskbarHanles < MAX_TASKBAR;
}

// return process id of explorer.exe process or 0 if not found
DWORD findExplorerProc() {
	DWORD pidList[MAX_PROCESS];
	DWORD byteNeeded;
	DWORD numOfProc;
	HANDLE hProc = NULL;
	HMODULE hMod = NULL;
	wchar_t processName[MAX_PATH];

	if (!(EnumProcesses(pidList, sizeof(pidList), &byteNeeded))) {
		return 0;
	}
	if ((numOfProc = byteNeeded/sizeof(DWORD)) >= MAX_PROCESS) {	// pidList not enough
		return 0;
	}
	int pid;
	for (int i = 0; i < numOfProc; ++i) {
		pid = pidList[i];

		if (pid) {
			if (!(hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid))) {
				continue;
			}
			if (!EnumProcessModules(hProc, &hMod, sizeof(hMod), &byteNeeded)) {
				CloseHandle(hProc);
				continue;
			}
			if (!GetModuleBaseName(hProc, hMod, processName, MAX_PATH)) {
				CloseHandle(hProc);
				continue;
			}
			// MessageBox(NULL, processName, processName, MB_OK);
			if (_wcsicmp(processName, L"explorer.exe")) {
				CloseHandle(hProc);
				continue;
			}
			CloseHandle(hProc);
			return pid;
		}
	}
	return 0;
}

// do runtime load hook function
BOOL loadHookProc() {
	if ((hHookLib = LoadLibrary(L"EasyDesktopSwitcherHook.dll")) == NULL) {
		return FALSE;
	}
	if ((pHookProc = (HOOKPROC)GetProcAddress(hHookLib, "mouseHookProc")) == NULL) {
		return FALSE;
	}
	return TRUE;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	HHOOK hHook = NULL;

	if (!(explorerPid = findExplorerProc())) {
		printError(L"Cannot find explorer.exe process");
		return 1;
	}

	SetLastError(0);
	EnumWindows(enumWindowsProc, NULL);
	if (GetLastError()){
		printError(L"In EnumWindows");
		return 1;
	}

	// install WH_MOUSE hook
	if (!loadHookProc()) {
		printError(L"Cannot load the hook function from dll");
		return 1;
	}
	if ((hHook = SetWindowsHookEx(WH_MOUSE, pHookProc, hHookLib, explorerTid)) == NULL) {
		printError(L"In SetWindowsHookEx");
		return 1;
	}

	MSG msg = { };
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	UnhookWindowsHookEx(hHook);
	return 0;
}