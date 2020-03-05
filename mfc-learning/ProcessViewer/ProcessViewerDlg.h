
// ProcessViewerDlg.h : header file
//

#pragma once

#define ListHeaderSize 5
#define BUFSIZE 1024

// CProcessViewerDlg dialog
class CProcessViewerDlg : public CDialogEx
{
// Construction
public:
	CProcessViewerDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_PROCESSVIEWER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	// user data
	int listHeaderIDs[ListHeaderSize] = { IDS_ProcessName, IDS_ProcessID, IDS_CommandLine, IDS_ImagePath, IDS_ImageType};
	int listHeaderIndexOf(int fieldID);
	int listHeaderSizes[ListHeaderSize] = { 100, 80, 300, 300 , 80};
	CListCtrl processList;

	afx_msg void OnBnClickedButtonRefresh();
	void refreshProcessList();
	afx_msg void OnSize(UINT nType, int cx, int cy);
};
