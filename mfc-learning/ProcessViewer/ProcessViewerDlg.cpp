
// ProcessViewerDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "ProcessViewer.h"
#include "ProcessViewerDlg.h"
#include "afxdialogex.h"
#include "Resource.h"

#include "CompAPI.h"
#include "tlhelp32.h"
#include "winternl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// function pointers for some dynamic linking functions

typedef NTSTATUS
(NTAPI* NtQueryInformationProcess_t)(
	IN HANDLE ProcessHandle,
	IN PROCESSINFOCLASS ProcessInformationClass,
	OUT PVOID ProcessInformation,
	IN ULONG ProcessInformationLength,
	OUT PULONG ReturnLength OPTIONAL
	);

typedef BOOL(WINAPI* IsWow64Process_t) (HANDLE, PBOOL);

NtQueryInformationProcess_t NtQueryInformationProcess_proc;
IsWow64Process_t IsWow64Process_proc;


// CProcessViewerDlg dialog


CProcessViewerDlg::CProcessViewerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PROCESSVIEWER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CProcessViewerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_Process, processList);
}

BEGIN_MESSAGE_MAP(CProcessViewerDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_Refresh, &CProcessViewerDlg::OnBnClickedButtonRefresh)
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CProcessViewerDlg message handlers

BOOL CProcessViewerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	//--------------------------------------------------------------------------------------------//

	// load NtQueryInformationProcess
	HMODULE hLib = LoadLibrary(_T("ntdll.dll"));
	if (hLib == NULL) {
		return FALSE;
	}
	NtQueryInformationProcess_proc = (NtQueryInformationProcess_t)GetProcAddress(hLib, "NtQueryInformationProcess");
	if (NtQueryInformationProcess_proc == NULL) {
		return FALSE;
	}

	// load IsWow64Process
	hLib = LoadLibrary(_T("kernel32"));
	if (hLib == NULL) {
		return FALSE;
	}
	IsWow64Process_proc = (IsWow64Process_t)GetProcAddress(hLib, "IsWow64Process");
	if (IsWow64Process_proc == NULL) {
		return FALSE;
	}


	// add column headers (little complex just for practicing String resource)
	TCHAR buffer[BUFSIZE];

	HINSTANCE hModule = GetModuleHandle(NULL);
	if (hModule == NULL) {
		return FALSE;
	}
	LVCOLUMN col;

	col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	col.fmt = LVCFMT_LEFT;

	for (int i = 0; i < ListHeaderSize; ++i) {
		buffer[LoadString(hModule, listHeaderIDs[i], buffer, BUFSIZE - 1)] = NULL;

		col.cx = listHeaderSizes[i];
		col.pszText = buffer;
		processList.InsertColumn(i, &col);
	}
	processList.SetItemCount(4096);
	refreshProcessList();
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CProcessViewerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CProcessViewerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CProcessViewerDlg::OnBnClickedButtonRefresh()
{
	refreshProcessList();
}

// --------------------------------------------------------------------- //

// for access header index quickly
int CProcessViewerDlg::listHeaderIndexOf(int fieldID)
{
	for (int i = 0; i < ListHeaderSize; ++i) {
		if (listHeaderIDs[i] == fieldID)
			return i;
	}
	return -1;
}

// set image path of a process
void setImagePath(int procID, CString& imagePath)
{
	MODULEENTRY32 module;

	if (procID <= 0) {	// 0 -> system process
		return;
	}

	// take snapshot of all modules of a process
	HANDLE hModuleList = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, procID);
	if (hModuleList == INVALID_HANDLE_VALUE) {
		return;
	}
	// must set the dwSize member of MODULEENTRY32 to the size of the structure
	module.dwSize = sizeof(MODULEENTRY32);

	if (!Module32First(hModuleList, &module)) {
		return;
	}

	imagePath = module.szExePath;
	CloseHandle(hModuleList);
}

// set the commandline of a process
void setCommandline64(int procID, CString& commandline)
{
	PROCESS_BASIC_INFORMATION procInfo;
	unsigned long retlen = 0;

	// obtain process handle
	HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, procID);
	if (hProc == NULL) {
		return;
	}
	// get PEB base address
	if (NtQueryInformationProcess_proc(hProc, ProcessBasicInformation, &procInfo,
		sizeof(procInfo), &retlen) != 0 || retlen != sizeof(procInfo)) {
		return;
	}

	// remote process memory reading
	PEB procPEB;
	RTL_USER_PROCESS_PARAMETERS procParams;
	SIZE_T nread;

	// read ProcessParameters pointer
	if (!ReadProcessMemory(hProc, procInfo.PebBaseAddress, &procPEB, sizeof(procPEB), &nread) || nread != sizeof(procPEB)) {
		return;
	}
	// read process parameters
	if (!ReadProcessMemory(hProc, procPEB.ProcessParameters, &procParams, sizeof(procParams), &nread)
		|| nread != sizeof(procParams)) {
		return;
	}
	int bufSize = procParams.CommandLine.Length;
	wchar_t* buffer = commandline.GetBufferSetLength(bufSize);
	buffer[bufSize] = '\x00\x00';

	// read commandline
	if (!ReadProcessMemory(hProc, procParams.CommandLine.Buffer, buffer, bufSize*2, &nread) || nread != bufSize*2) {
		return;
	}

	CloseHandle(hProc);
}

// set the commandline of a process
// https://github.com/nihilus/ScyllaHide/blob/master/InjectorCLI/RemotePebHider.cpp#L58
void setCommandline(int procID, CString& cmdl, int isX86)
{
	PROCESS_BASIC_INFORMATION procInfo;
	unsigned long retlen = 0;

	// obtain process handle
	HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, procID);
	if (hProc == NULL) {
		return;
	}
	// get PEB base address
	if (NtQueryInformationProcess_proc(hProc, ProcessBasicInformation, &procInfo,
		sizeof(procInfo), &retlen) != 0 || retlen != sizeof(procInfo)) {
		return;
	}

	PVOID bufferAt = NULL;
	__int16 bufSize = 0;
	__int64 procParamsAddr = 0;
	SIZE_T nread;

	if (isX86 == 1) {	// 32bit
		RTL_USER_PROCESS_PARAMETERS32 procParams;

		// read ProcessParameters addr
		// WOW64 processes have 2 PEBs (PEB64 after PEB32 offset 0x1000)
		if (!ReadProcessMemory(hProc, (PVOID)((__int64)procInfo.PebBaseAddress + 0x1000 + 0x10), &procParamsAddr, sizeof(__int32), &nread) || nread != sizeof(__int32)) {
			return;
		}

		// read process parameters
		if (!ReadProcessMemory(hProc, (PVOID)procParamsAddr, &procParams, sizeof(procParams), &nread)
			|| nread != sizeof(procParams)) {
			return;
		}

		bufSize = procParams.CommandLine.Length;
		bufferAt = (PVOID)procParams.CommandLine.Buffer;
	}
	else {
		// remote process memory reading
		RTL_USER_PROCESS_PARAMETERS procParams;

		// read ProcessParameters pointer
		if (!ReadProcessMemory(hProc, (PVOID)((__int64)procInfo.PebBaseAddress + 0x20), &procParamsAddr, sizeof(__int64), &nread) || nread != sizeof(__int64)) {
			return;
		}

		PEB;
		// read process parameters
		if (!ReadProcessMemory(hProc, (PVOID)procParamsAddr, &procParams, sizeof(procParams), &nread)
			|| nread != sizeof(procParams)) {
			return;
		}
		bufSize = procParams.CommandLine.Length;
		bufferAt = (PVOID)procParams.CommandLine.Buffer;
	}

	wchar_t* buffer = cmdl.GetBufferSetLength(bufSize);
	buffer[bufSize] = '\x00\x00';

	// read commandline
	if (!ReadProcessMemory(hProc, bufferAt, buffer, bufSize*2, &nread) || nread != bufSize*2) {
		return;
	}

	CloseHandle(hProc);
}

// return 1 if is the process with procID is 32bit (on both x64, x86)
int getProcessType(int procID) {
	BOOL isWow64;
	SYSTEM_INFO sysInfo;

	GetNativeSystemInfo(&sysInfo);
	// obtain process handle
	HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, procID);
	if (hProc == NULL) {
		return -1;
	}
	IsWow64Process_proc(hProc, &isWow64);

	// isWow64 set to 1 if the process is running under WOW64 on an Intel64 or x64 processor
	// If the process is running under 32-bit Windows, WOW64 is set to FALSE
	if(sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL){
		isWow64 = 1;
	}
	else if (sysInfo.wProcessorArchitecture != PROCESSOR_ARCHITECTURE_AMD64) {
		isWow64 = -1;	// only consider on intel arch
	}

	CloseHandle(hProc);
	return isWow64;
}

// fulfill the process list 
void CProcessViewerDlg::refreshProcessList()
{
	PROCESSENTRY32 proc;

	processList.DeleteAllItems();

	// take snapshot of all processes in the system
	HANDLE hProcList = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcList == INVALID_HANDLE_VALUE) {
		MessageBox(_T("cannot take snapshot of processes"), _T("ERROR"), MB_ICONERROR | MB_OK);
		return;
	}
	// must set the dwSize member of PROCESSENTRY32 to the size of the structure
	proc.dwSize = sizeof(proc);

	if (!Process32First(hProcList, &proc)) {
		return;
	}

	int item_i = 0;
	do {
		int procID = proc.th32ProcessID;
		CString fmtProcID;
		CString imagePath = _T("-");
		CString imageType = _T("-");
		CString commandline = _T("-");
		int isX86 = getProcessType(procID);

		fmtProcID.Format(_T("%d"), proc.th32ProcessID);
		setImagePath(procID, imagePath);
		setCommandline(procID, commandline, isX86);

		imageType = isX86 == 1 ? _T("32 bit") :
			(isX86 == 0 ? _T("64 bit") : _T("-"));

		processList.InsertItem(item_i, proc.szExeFile);
		processList.SetItemText(item_i, listHeaderIndexOf(IDS_ProcessID), fmtProcID);
		processList.SetItemText(item_i, listHeaderIndexOf(IDS_ImagePath), imagePath);
		processList.SetItemText(item_i, listHeaderIndexOf(IDS_CommandLine), commandline);
		processList.SetItemText(item_i, listHeaderIndexOf(IDS_ImageType), imageType);

		item_i += 1;
	} while (Process32Next(hProcList, &proc));

	CloseHandle(hProcList);
}



void CProcessViewerDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	// remains: How to resize listing while dialog resized

}
