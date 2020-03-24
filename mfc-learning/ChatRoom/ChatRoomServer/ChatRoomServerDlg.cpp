
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

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
public:
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
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
		_T("No."),
		_T("IP Address"),
		_T("Port"),
	};
	int sizes[] = { 80, 200, 80};

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

// CChatRoomServerDlg message handlers
BOOL CChatRoomServerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

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

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CChatRoomServerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
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

void CChatRoomServerDlg::OnBnClickedButtonStart()
{
	int status = startButton.GetWindowTextLength();
	int ret = 0;
	
	// disable first to avoid multiple instances
	startButton.EnableWindow(FALSE);

	if (status == wcslen(L"START")) {	// start listening
		hServerThread = CreateThread(NULL, 0, startServer, (LPVOID)this, 0, NULL);
		if (hServerThread == NULL) {
			MessageBox(L"Cannot start server", L"ERROR");
		}
		else {
			startButton.SetWindowText(L"STOP");
		}
		startButton.EnableWindow(TRUE);
		return;
	}

	if (hServerThread != NULL && TerminateThread(hServerThread, NULL)) {
			startButton.SetWindowText(L"START");
	}
	startButton.EnableWindow(TRUE);
}

// logs button handler
void CChatRoomServerDlg::OnBnClickedMfcbuttonLogs()
{

}
