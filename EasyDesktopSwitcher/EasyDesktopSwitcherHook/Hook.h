#include <Windows.h>

#ifdef EASYDESKTOPSWITCHERHOOK_EXPORTS
#define HOOK_API __declspec(dllexport)
#else
#define HOOK_API __declspec(dllimport)
#endif

// the callback function is called on mouse messages
extern "C" HOOK_API LRESULT CALLBACK mouseHookProc(
    _In_ int    nCode,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
);