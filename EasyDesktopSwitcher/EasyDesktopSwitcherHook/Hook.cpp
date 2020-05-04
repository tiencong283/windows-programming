#include "Hook.h"
#include <iostream>

// simulate CTRL + Windows + RIGHT/LEFT ARROW
BOOL switchVirtualDesktop(BOOL direction) {
	INPUT keyEvents[6];
	int numOfEvents = 6;

	ZeroMemory(keyEvents, sizeof(keyEvents));
	// common properties
	for (int i = 0; i < numOfEvents; ++i) {
		keyEvents[i].type = INPUT_KEYBOARD;
	}
	// keys being pressed
	keyEvents[0].ki.wVk = keyEvents[3].ki.wVk = VK_CONTROL;
	keyEvents[1].ki.wVk = keyEvents[4].ki.wVk = VK_LWIN;
	keyEvents[2].ki.wVk = keyEvents[5].ki.wVk = direction ? VK_RIGHT : VK_LEFT;

	// keys being released
	keyEvents[3].ki.dwFlags = KEYEVENTF_KEYUP;
	keyEvents[4].ki.dwFlags = KEYEVENTF_KEYUP;
	keyEvents[5].ki.dwFlags = KEYEVENTF_KEYUP;

	if ((SendInput(numOfEvents, keyEvents, sizeof(INPUT)) != numOfEvents) || GetLastError()) {
		return FALSE;
	}
	return TRUE;
}


LRESULT CALLBACK mouseHookProc(
    _In_ int    nCode,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
) {
    LPMOUSEHOOKSTRUCT pHookInfo = LPMOUSEHOOKSTRUCT(lParam);
    if (nCode != 0 || wParam!=WM_MOUSEWHEEL) {
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }
	// There's no way to get the direction of mouse wheel
}