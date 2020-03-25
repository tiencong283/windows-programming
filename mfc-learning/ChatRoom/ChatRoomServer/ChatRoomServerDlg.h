
// ChatRoomServerDlg.h : header file
//

#pragma once
// default host: localhost, default port: 7890
#define DEFAULT_PORT L"7890"
#define DEFAULT_HOST L"0.0.0.0"

// CLogDlg dialog
class CLogDlg : public CDialogEx
{
public:
	CLogDlg();

	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_LOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	// Implementation
protected:
	DECLARE_MESSAGE_MAP()
public:
	CEdit logEdit;
	CButton clearButton;
	CButton closeButton;
	// add msg to the end of Edit
	void appendMsg(wchar_t const* msg);

	afx_msg void OnBnClickedButtonClear();
	afx_msg void OnBnClickedButtonClose();
};

// CChatRoomServerDlg dialog
class CChatRoomServerDlg : public CDialogEx
{
// Construction
public:
	CChatRoomServerDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CHATROOMSERVER_DIALOG };
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
	SOCKET sock;
	HANDLE hServerThread;
	CLogDlg* hLogDlg;
	
	// add headers to list control
	BOOL initClientList();
	void addClient(wchar_t const* username, wchar_t const* ip, int port);
	void removeClient(wchar_t const* username);

	CButton startButton;
	CEdit serverPortEdit;
	CEdit serverIPEdit;
	CListCtrl clientList;

	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnBnClickedMfcbuttonLogs();
};