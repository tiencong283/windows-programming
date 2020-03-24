
// ChatRoomServerDlg.h : header file
//

#pragma once
// default host: localhost, default port: 7890
#define DEFAULT_PORT L"7890"
#define DEFAULT_HOST L"0.0.0.0"

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
	// add headers to list control
	BOOL initClientList();
	HANDLE hServerThread;

	CButton startButton;
	CEdit serverPortEdit;
	CListCtrl clientList;
	afx_msg void OnBnClickedButtonStart();
	CEdit serverIPEdit;
	afx_msg void OnBnClickedMfcbuttonLogs();
};


