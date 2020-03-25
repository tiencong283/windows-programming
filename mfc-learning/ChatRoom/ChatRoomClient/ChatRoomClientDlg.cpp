
// ChatRoomClientDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "ChatRoomClient.h"
#include "ChatRoomClientDlg.h"
#include "Client.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// logs dialog implementation
BEGIN_MESSAGE_MAP(CUsernameDlg, CDialogEx)
END_MESSAGE_MAP()

CUsernameDlg::CUsernameDlg() : CDialogEx(IDD_DIALOG_Username), username(_T(""))
{
}
void CUsernameDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_Username, username);
}

// CChatRoomClientDlg dialog

CChatRoomClientDlg::CChatRoomClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CHATROOMCLIENT_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CChatRoomClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON_Connect, connectButton);
	DDX_Control(pDX, IDC_BUTTON_Send, sendButton);
	DDX_Control(pDX, IDC_EDIT_ChatRoom, chatRoomEdit);
	DDX_Control(pDX, IDC_EDIT_Content, contentEdit);
	DDX_Control(pDX, IDC_EDIT_ServerIP, serverIPEdit);
	DDX_Control(pDX, IDC_EDIT_ServerPort, serverPortEdit);
}

BEGIN_MESSAGE_MAP(CChatRoomClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_Connect, &CChatRoomClientDlg::OnBnClickedButtonConnect)
	ON_BN_CLICKED(IDC_BUTTON_Send, &CChatRoomClientDlg::OnBnClickedButtonSend)
END_MESSAGE_MAP()


// CChatRoomClientDlg message handlers

BOOL CChatRoomClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
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

	
	// user's initialization
	serverIPEdit.SetWindowText(DEFAULT_HOST);
	serverPortEdit.SetWindowText(DEFAULT_PORT);


	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CChatRoomClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
		CDialogEx::OnSysCommand(nID, lParam);
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CChatRoomClientDlg::OnPaint()
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
HCURSOR CChatRoomClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


// when connected -> change button to DISCONNECT
void CChatRoomClientDlg::OnBnClickedButtonConnect()
{
	int status = connectButton.GetWindowTextLength();
	int ret = 0;

	// disable first to avoid multiple instances
	connectButton.EnableWindow(FALSE);
	if (status == wcslen(L"CONNECT")) {	// start listening
		hConnThread = CreateThread(NULL, 0, startConnection, (LPVOID)this, 0, NULL);
		if (hConnThread == NULL) {
			MessageBox(L"Cannot start server", L"ERROR");
		}
		else {
			connectButton.SetWindowText(L"DISCONNECT");
		}
		connectButton.EnableWindow(TRUE);
		return;
	}

	if (hConnThread != NULL && TerminateThread(hConnThread, NULL)) {
		cleanSession();
		connectButton.SetWindowText(L"CONNECT");
	}
	connectButton.EnableWindow(TRUE);
}

void CChatRoomClientDlg::prepareSession(SOCKET sock, wchar_t const* username)
{
	CString session;
	
	this->sock = sock;
	session.Format(L"Session: %s", username);
	SetDlgItemText(IDC_STATIC_Session, session);
	sendButton.EnableWindow(TRUE);
	contentEdit.EnableWindow(TRUE);
}

void CChatRoomClientDlg::cleanSession()
{
	closesocket(sock);
	sendButton.EnableWindow(FALSE);
	contentEdit.EnableWindow(FALSE);
}

void CChatRoomClientDlg::appendMsg(wchar_t const* msg)
{
	CString buffer;
	CTime ctime = CTime::GetCurrentTime();

	// seek to the end
	int len = chatRoomEdit.GetWindowTextLength();
	chatRoomEdit.SetSel(len, len);

	// get current time
	buffer += msg;
	buffer += L"\r\n";	// text lines in a multiline control are separated by '\r\n' character sequences

	chatRoomEdit.ReplaceSel(buffer);
}

// send msg
void CChatRoomClientDlg::OnBnClickedButtonSend()
{
	CString msg;
	CString echo;
	contentEdit.GetWindowText(msg);
	int len = msg.GetLength();
	if (len > 0){
		echo.Format(L"[You] %s", msg);
		appendMsg(echo);
		sendWideChars(sock, msg, len);
		contentEdit.SetWindowText(L"");
	}
}
