
// ChatRoomServerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ChatRoomServer.h"
#include "ChatRoomServerDlg.h"
#include "afxdialogex.h"
#include "Server.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// logs dialog implementation

CLogDlg::CLogDlg() : CDialogEx(IDD_DIALOG_LOG){
}

void CLogDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_LOG, logEdit);
	DDX_Control(pDX, IDC_BUTTON_Clear, clearButton);
	DDX_Control(pDX, IDC_BUTTON_Close, closeButton);
}

void CLogDlg::appendMsg(wchar_t const* msg)
{
	CString buffer;
	CTime ctime = CTime::GetCurrentTime();

	// seek to the end
	int len = logEdit.GetWindowTextLength();
	logEdit.SetSel(len, len);	

	// get current time
	buffer = ctime.Format(L"[%m-%d-%Y %T] ");
	buffer += msg;
	buffer += L"\r\n";	// text lines in a multiline control are separated by '\r\n' character sequences

	logEdit.ReplaceSel(buffer);
}
// clear all logs
void CLogDlg::OnBnClickedButtonClear()
{
	logEdit.SetWindowText(L"");
}
// close log window, just hide it
void CLogDlg::OnBnClickedButtonClose()
{
	ShowWindow(SW_HIDE);
}

BEGIN_MESSAGE_MAP(CLogDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_Clear, &CLogDlg::OnBnClickedButtonClear)
	ON_BN_CLICKED(IDC_BUTTON_Close, &CLogDlg::OnBnClickedButtonClose)
END_MESSAGE_MAP()


// CChatRoomServerDlg dialog

CChatRoomServerDlg::CChatRoomServerDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_CHATROOMSERVER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CChatRoomServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON_Start, startButton);
	DDX_Control(pDX, IDC_EDIT_ServerPort, serverPortEdit);
	DDX_Control(pDX, IDC_LIST_clients, clientList);
	DDX_Control(pDX, IDC_EDIT_ServerIP, serverIPEdit);
}

BEGIN_MESSAGE_MAP(CChatRoomServerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_Start, &CChatRoomServerDlg::OnBnClickedButtonStart)
	ON_BN_CLICKED(IDC_MFCBUTTON_Logs, &CChatRoomServerDlg::OnBnClickedMfcbuttonLogs)
END_MESSAGE_MAP()

BOOL CChatRoomServerDlg::initClientList() {
	int headerSize = 3;
	wchar_t* headers[] = {
		_T("Username"),
		_T("IP Address"),
		_T("Port"),
	};
	int sizes[] = { 240, 120, 80};

	LVCOLUMN col;
	col.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
	col.fmt = LVCFMT_LEFT;

	for (int i = 0; i < headerSize; ++i) {
		col.pszText = headers[i];
		col.cx = sizes[i];

		if (clientList.InsertColumn(i, &col) == -1) {
			return FALSE;
		}
	}
	return TRUE;
}

void CChatRoomServerDlg::addClient(wchar_t const* username, wchar_t const* ip, int port)
{
	CString wUsername(username);
	CString portStr;
	portStr.Format(L"%d", port);

	// insert at the head of list
	clientList.InsertItem(0, wUsername);
	clientList.SetItemText(0, 1, ip);
	clientList.SetItemText(0, 2, portStr);
}

void CChatRoomServerDlg::removeClient(wchar_t const* username)
{
	LVFINDINFO fi = {};
	fi.flags = LVFI_STRING;
	fi.psz = username;

	int found = clientList.FindItem(&fi);
	if (found != -1) {
		clientList.DeleteItem(found);
	}
}

// CChatRoomServerDlg message handlers
BOOL CChatRoomServerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	SetDlgItemText(IDC_EDIT_ServerPort, DEFAULT_PORT);
	SetDlgItemText(IDC_EDIT_ServerIP, DEFAULT_HOST);

	if (initClientList() != TRUE) {
		return FALSE;
	}

	hLogDlg = new CLogDlg;
	if (hLogDlg->Create(IDD_DIALOG_LOG)) {
		return FALSE;
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CChatRoomServerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
		CDialogEx::OnSysCommand(nID, lParam);
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CChatRoomServerDlg::OnPaint()
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
HCURSOR CChatRoomServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

extern HANDLE hSockEvent;
void CChatRoomServerDlg::OnBnClickedButtonStart()
{
	int status = startButton.GetWindowTextLength();
	int ret = 0;
	DWORD exitCode;

	startButton.EnableWindow(FALSE);

	if (status == wcslen(L"START")) {	// start listening
		
		hServerThread = CreateThread(NULL, 0, startServer, (LPVOID)this, 0, NULL);
		if (hServerThread == NULL) {
			return;
		}
		else {
			startButton.SetWindowText(L"STOP");
		}
		startButton.EnableWindow(TRUE);
		return;
	}

	if (hServerThread != NULL && TerminateThread(hServerThread, NULL)) {	// terminate server
		// kind of idiot// just workaround for fixing unclean resources due to TerminateThread
		closesocket(sock);
		startButton.SetWindowText(L"START");
		clientList.DeleteAllItems();
		hLogDlg->appendMsg(L"Stopped the server");
	}
	
	startButton.EnableWindow(TRUE);
}

// Logs button handler
void CChatRoomServerDlg::OnBnClickedMfcbuttonLogs()
{
	RECT mDlgRect = {};
	RECT lDlgRect = {};
	
	if (hLogDlg->IsWindowVisible()) {	// hide it
		hLogDlg->ShowWindow(SW_HIDE);
	}
	else {
		// show the log window and reposition to the right of main dialog
		GetWindowRect(&mDlgRect);
		hLogDlg->GetWindowRect(&lDlgRect);

		hLogDlg->MoveWindow(mDlgRect.right, mDlgRect.top, lDlgRect.right - lDlgRect.left, lDlgRect.bottom - lDlgRect.top);
		hLogDlg->ShowWindow(SW_SHOW);
	}
}