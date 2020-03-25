
// ChatRoomClientDlg.h : header file
//

#pragma once

// default host: localhost, default port: 7890
#define DEFAULT_PORT L"7890"
#define DEFAULT_HOST L"localhost"

// CLogDlg dialog
class CUsernameDlg : public CDialogEx
{
public:
	CUsernameDlg();

	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_Username };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	// Implementation
protected:
	DECLARE_MESSAGE_MAP()
public:
	CString username;
};


// CChatRoomClientDlg dialog
class CChatRoomClientDlg : public CDialogEx
{
// Construction
public:
	CChatRoomClientDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CHATROOMCLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	HANDLE hConnThread;	// handle to the chat thread
	SOCKET sock;	// session socket
	// prepare for start chatting
	void prepareSession(SOCKET sock, wchar_t const* username);
	void cleanSession();
	void appendMsg(wchar_t const* msg);

	CButton connectButton;
	CButton sendButton;
	CEdit chatRoomEdit;
	CEdit contentEdit;
	CEdit serverIPEdit;
	CEdit serverPortEdit;
	afx_msg void OnBnClickedButtonConnect();
	afx_msg void OnBnClickedButtonSend();
};

