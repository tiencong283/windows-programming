#include <Windows.h>
#include <errno.h>
#include "ErrorShowDialog.h"

#define BUFSIZE 1024
#define INVALID_NUM_STR (1 << 28)

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {

	DialogBox(NULL, MAKEINTRESOURCE(IDD_DIALOG_ErrorShow), NULL, dlgWinProc);
	return 0;
}

long convertStrToLong(wchar_t* s, int base) {
	long num = 0;
	wchar_t* p = s;

	SetLastError(0);
	num = wcstol(s, &p, base);
	if (*p != _T('\x00') || errno == ERANGE) {
		SetLastError(INVALID_NUM_STR);
	}
	return num;
}

static HWND hLookupButton = NULL;
BOOL CALLBACK dlgWinProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) {

	wchar_t buffer[BUFSIZE] = {};
	long errorCode = 0;
	wchar_t* formatted = NULL; // Buffer that gets the error message string

	switch (message) {
	case WM_INITDIALOG:
	{
		// set text limit 
		SendDlgItemMessage(hwndDlg, IDC_EDIT_ErrorCode, EM_SETLIMITTEXT, BUFSIZE -1, NULL);

		// disable lookup button
		hLookupButton = GetDlgItem(hwndDlg, IDC_BUTTON_Lookup);
		EnableWindow(hLookupButton, FALSE);
		return TRUE;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam)) {
		case IDC_EDIT_ErrorCode:
			if (HIWORD(wParam) == EN_CHANGE) {
				DWORD len = SendDlgItemMessage(hwndDlg, IDC_EDIT_ErrorCode, WM_GETTEXTLENGTH, NULL, NULL);
				if (len > 0){
					EnableWindow(hLookupButton, TRUE);
				}
				else {
					EnableWindow(hLookupButton, FALSE);
				}
			}
			return TRUE;
		case IDC_BUTTON_Lookup:
		{
			DWORD len = SendDlgItemMessage(hwndDlg, IDC_EDIT_ErrorCode, WM_GETTEXTLENGTH, NULL, NULL);
			if (len > 0) {
				// GetDlgItemInt alternative
				buffer[GetDlgItemText(hwndDlg, IDC_EDIT_ErrorCode, buffer, BUFSIZE - 1)] = _T('\x00');
				errorCode = convertStrToLong(buffer, 10);
				if (GetLastError() == INVALID_NUM_STR) {
					errorCode = convertStrToLong(buffer, 16);
				}
				if (GetLastError() == INVALID_NUM_STR || errorCode < 0) {
					SetDlgItemText(hwndDlg, IDC_STATIC_ErrorText, _T("Invalid error code. It must be numeric string and in valid range"));
					return TRUE;
				}

				int nread = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL, errorCode, 0, (LPTSTR)&formatted, 0, NULL);
				if (nread == 0) {
					SendDlgItemMessage(hwndDlg, IDC_STATIC_ErrorText, WM_SETTEXT, NULL, (LPARAM)_T("Cannot find the error code message"));
					return TRUE;
				}
				formatted[nread] = _T('\x00');
				SetDlgItemText(hwndDlg, IDC_STATIC_ErrorText, formatted);
				
				LocalFree(formatted);
			}
			return TRUE;
		}
		}
	}
	case WM_CLOSE:	// events from Close button
	{
		int res = MessageBox(hwndDlg, _T("Do you want to close?"), _T("Closing"), MB_YESNO | MB_ICONQUESTION);
		if (res == IDYES) {
			EndDialog(hwndDlg, 0);
		}
		return TRUE;
	}
	default:
		return FALSE;
	}
}