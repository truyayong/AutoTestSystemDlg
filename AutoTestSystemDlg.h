// AutoTestSystemDlg.h : main header file for the AUTOTESTSYSTEMDLG application
//

#if !defined(AFX_AUTOTESTSYSTEMDLG_H__67078F13_B4F8_49AD_91C3_59AE4ECCAEF7__INCLUDED_)
#define AFX_AUTOTESTSYSTEMDLG_H__67078F13_B4F8_49AD_91C3_59AE4ECCAEF7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CAutoTestSystemDlgApp:
// See AutoTestSystemDlg.cpp for the implementation of this class
//

class CAutoTestSystemDlgApp : public CWinApp
{
public:
	CAutoTestSystemDlgApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAutoTestSystemDlgApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CAutoTestSystemDlgApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AUTOTESTSYSTEMDLG_H__67078F13_B4F8_49AD_91C3_59AE4ECCAEF7__INCLUDED_)
