
// FileScannerDlg.h : header file
//

#pragma once


// CFileScannerDlg dialog
class CFileScannerDlg : public CDialogEx
{
// Construction
public:
	CFileScannerDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FILESCANNER_DIALOG };
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
	// user controls
	CEdit directoryPathEdit;
	CEdit fileNameEdit;
	CButton matchCaseCheck;
	CButton matchWholeCheck;
	CListCtrl fileList;

	// support funtions
	void browseForDir(BOOL recursive, CString root, CString dirPath, CString fileSearchName, BOOL filtered, BOOL matchCase, BOOL matchWhole);

	// user handlers
	afx_msg void OnBnClickedButtonFind();
	int initFileList();	// initialize file listing
	afx_msg void OnBnClickedButtonBrowse();
	CButton recursiveCheck;
};
