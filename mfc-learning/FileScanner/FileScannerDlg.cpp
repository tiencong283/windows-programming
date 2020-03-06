
// FileScannerDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "FileScanner.h"
#include "FileScannerDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#define BUFSIZE 1024

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


// CFileScannerDlg dialog



CFileScannerDlg::CFileScannerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_FILESCANNER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CFileScannerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_Directory, directoryPathEdit);
	DDX_Control(pDX, IDC_EDIT_FileName, fileNameEdit);
	DDX_Control(pDX, IDC_CHECK_MatchCase, matchCaseCheck);
	DDX_Control(pDX, IDC_CHECK_MatchWhole, matchWholeCheck);
	DDX_Control(pDX, IDC_LIST_Files, fileList);
	DDX_Control(pDX, IDC_CHECK_Recursive, recursiveCheck);
}

BEGIN_MESSAGE_MAP(CFileScannerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_Find, &CFileScannerDlg::OnBnClickedButtonFind)
	ON_BN_CLICKED(IDC_BUTTON_Browse, &CFileScannerDlg::OnBnClickedButtonBrowse)
END_MESSAGE_MAP()


// CFileScannerDlg message handlers

BOOL CFileScannerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

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

	// TODO: Add extra initialization here

	if (!initFileList()) {
		return FALSE;
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CFileScannerDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CFileScannerDlg::OnPaint()
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
HCURSOR CFileScannerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// ---------------------------------------------------------------------- //

void CFileScannerDlg::browseForDir(BOOL recursive, CString root, CString dirPath, CString fileSearchName, BOOL filtered, BOOL matchCase, BOOL matchWhole) {
	CString preservedDirPath = dirPath;

	// file enumeration
	WIN32_FIND_DATA ffd;
	HANDLE hFind;

	if (dirPath.GetAt(dirPath.GetLength() - 1) != '\\') {
		dirPath += "\\*";
	}
	else {
		dirPath += "*";
	}

	hFind = FindFirstFile(dirPath, &ffd);
	if (hFind == INVALID_HANDLE_VALUE) {
		return;
	}

	int item_i = 0;
	do {
		SYSTEMTIME createdAt;
		SYSTEMTIME modifiedAt;
		wchar_t fmtCreatedAt[BUFSIZE] = _T("-");
		wchar_t fmtModifiedAt[BUFSIZE] = _T("-");
		CString fmtSize = _T("-");

		CString fileName = CString(ffd.cFileName);

		// filter logic
		if (fileName == _T(".") || fileName == _T("..")) {	// ignore special files
			continue;
		}
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (recursive) {
				CString newRoot = root == "" ? fileName : root + "\\" + fileName;
				browseForDir(recursive, newRoot, preservedDirPath + "\\" + fileName,
					fileSearchName, filtered, matchCase, matchWhole);
			}
			continue;
		}

		if (filtered) {
			CString tmp = fileName;	// clone fileName to preserve its case
			if (!matchCase) {
				tmp.MakeLower();
			}

			// not matched
			if (matchWhole && fileSearchName != tmp) {	// match the whole
				continue;
			}
			else if (tmp.Find(fileSearchName) == -1) {	// match partly
				continue;
			}
		}

		// format time
		if (FileTimeToSystemTime(&ffd.ftCreationTime, &createdAt)) {
			GetDateFormatEx(LOCALE_NAME_SYSTEM_DEFAULT, NULL, &createdAt, NULL, fmtCreatedAt, BUFSIZE - 1, NULL);
			int len = wcslen(fmtCreatedAt);
			fmtCreatedAt[len] = _T(' ');

			GetTimeFormatEx(LOCALE_NAME_SYSTEM_DEFAULT, TIME_NOSECONDS, &createdAt, NULL, &fmtCreatedAt[len + 1], BUFSIZE - len - 2);
		}

		if (FileTimeToSystemTime(&ffd.ftLastWriteTime, &modifiedAt)) {
			GetDateFormatEx(LOCALE_NAME_SYSTEM_DEFAULT, NULL, &modifiedAt, NULL, fmtModifiedAt, BUFSIZE - 1, NULL);
			int len = wcslen(fmtModifiedAt);
			fmtModifiedAt[len] = _T(' ');

			GetTimeFormatEx(LOCALE_NAME_SYSTEM_DEFAULT, TIME_NOSECONDS, &modifiedAt, NULL, &fmtModifiedAt[len + 1], BUFSIZE - len - 2);
		}

		// file size
		__int64 fileSize = ffd.nFileSizeHigh << 32;
		fileSize += ffd.nFileSizeLow;

		fmtSize.Format(_T("%lld KB"), fileSize / 1024);

		
		fileList.InsertItem(item_i, root == "" ? fileName: root + "\\" + fileName);	// name
		fileList.SetItemText(item_i, 1, fmtSize);	// size
		fileList.SetItemText(item_i, 2, fmtModifiedAt);	// modification time
		fileList.SetItemText(item_i, 3, fmtCreatedAt);	// creation time

		item_i += 1;
	} while (FindNextFile(hFind, &ffd) != 0);

	// close the file search handle
	FindClose(hFind);
}


// main logic entrypoint
void CFileScannerDlg::OnBnClickedButtonFind()
{
	CString dirPath, fileSearchName;
	CFile dirFile;
	// search options
	BOOL matchCase = matchCaseCheck.GetCheck();
	BOOL matchWhole = matchWholeCheck.GetCheck();
	BOOL recursive = recursiveCheck.GetCheck();
	BOOL filtered = true;

	directoryPathEdit.GetWindowText(dirPath);
	fileNameEdit.GetWindowText(fileSearchName);

	// directory validation
	if (dirPath.GetLength() == 0 ) {
		MessageBox(_T("Please enter a directory"), _T("ERROR"), MB_ICONERROR | MB_OK);
		return;
	}
	CFileStatus status;
	if (!CFile::GetStatus(dirPath, status) || !(status.m_attribute & CFile::directory)) {
		MessageBox(_T("The file missing or not a directory"), _T("ERROR"), MB_ICONERROR | MB_OK);
		return;
	}
	if (fileSearchName.GetLength() == 0) {	// default list all files
		filtered = false;
	}
	if (filtered && !matchCase) {	// case-insesitive
		fileSearchName.MakeLower();
	}

	// clean listing
	fileList.DeleteAllItems();

	if (dirPath.GetLength() > MAX_PATH || recursive) {	// permit an extended-length path for a maximum total path length of 32,767 characters
		dirPath = _T("\\\\?\\") + dirPath;
	}
	browseForDir(recursive, _T(""), dirPath, fileSearchName, filtered, matchCase, matchWhole);
}

// initialize file listing
int CFileScannerDlg::initFileList()
{
	int headerSize = 4;
	wchar_t* headers[] = {
		_T("Name"),
		_T("Size"),
		_T("Date modified"),
		_T("Date created"),
	};
	int sizes[] = {180, 60, 120, 120};
	
	LVCOLUMN col;
	col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	col.fmt = LVCFMT_LEFT;
	
	for (int i = 0; i < headerSize; ++i) {
		col.cx = sizes[i];
		col.pszText = headers[i];
		if (fileList.InsertColumn(i, &col) == -1) {
			return 0;
		}
	}

	// prepares a list view control for adding a large number of items
	fileList.SetItemCount(1024);

	return 1;
}

// browse directory instead of manual typing
void CFileScannerDlg::OnBnClickedButtonBrowse()
{
	CFolderPickerDialog dirPickerDialog(NULL, OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST);
	OPENFILENAME& ofn = dirPickerDialog.GetOFN();

	ofn.lpstrTitle = _T("Choose a directory to search for");

	int response = dirPickerDialog.DoModal();
	if (response == IDOK) {
		directoryPathEdit.SetWindowText(dirPickerDialog.GetPathName());
	}
}
