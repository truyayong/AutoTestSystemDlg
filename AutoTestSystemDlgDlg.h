// AutoTestSystemDlgDlg.h : header file
//

#if !defined(AFX_AUTOTESTSYSTEMDLGDLG_H__08D6AAC5_8C21_4B40_8B97_8E053FF36EF4__INCLUDED_)
#define AFX_AUTOTESTSYSTEMDLGDLG_H__08D6AAC5_8C21_4B40_8B97_8E053FF36EF4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CAutoTestSystemDlgDlg dialog

class CAutoTestSystemDlgDlg : public CDialog
{
// Construction
public:
	BOOL isFileCompare;
	CPoint mPoint;
	CSocket m_MonkeySocket;
	CAutoTestSystemDlgDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CAutoTestSystemDlgDlg)
	enum { IDD = IDD_AUTOTESTSYSTEMDLG_DIALOG };
	CProgressCtrl	m_Progress;
	CEdit	m_Text;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAutoTestSystemDlgDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CAutoTestSystemDlgDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnScreenshot();
	afx_msg void OnControl();
	afx_msg void OnMenu();
	afx_msg void OnHome();
	afx_msg void OnBack();
	afx_msg void OnWake();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnExexml();
	afx_msg void OnRecxml();
	afx_msg void OnHalt();
	afx_msg void OnSendtext();
	afx_msg void OnPushfile();
	afx_msg void OnPullfile();
	afx_msg void OnCompare();
	afx_msg void OnStart();
	afx_msg void OnUncontrol();
	afx_msg void OnStop();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AUTOTESTSYSTEMDLGDLG_H__08D6AAC5_8C21_4B40_8B97_8E053FF36EF4__INCLUDED_)
