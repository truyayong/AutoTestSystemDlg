// AutoTestSystemDlgDlg.cpp : implementation file
//

#include "stdafx.h"
#include "AutoTestSystemDlg.h"
#include "AutoTestSystemDlgDlg.h"
#include "Markup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MKID(a,b,c,d) ((a) | ((b) << 8) | ((c) << 16) | ((d) << 24))


#define ID_STAT MKID('S','T','A','T')
#define ID_LIST MKID('L','I','S','T')
#define ID_ULNK MKID('U','L','N','K')
#define ID_SEND MKID('S','E','N','D')
#define ID_RECV MKID('R','E','C','V')
#define ID_DENT MKID('D','E','N','T')
#define ID_DONE MKID('D','O','N','E')
#define ID_DATA MKID('D','A','T','A')
#define ID_OKAY MKID('O','K','A','Y')
#define ID_FAIL MKID('F','A','I','L')
#define ID_QUIT MKID('Q','U','I','T')
#define SYNC_DATA_MAX (64*1024)

#define ERR 1
#define SUCCESS 0

#define TMP_BITMAP_DATA 6000
#define SOCK_REC_VNORMAL_LEN 7
/////////////////////////////////////////////////////////////////////////////

//define global function

CString ConvertAdbCommand(CString& s);
UINT  ScreenCaping( LPVOID pParam );//function for screencap thread
UINT  ExecuteXml( LPVOID pParam );//function for ExecuteXml thread
void checkIsRecord(BOOL flag,CString command);
UINT  sendFile( LPVOID pParam );//function for adb send file thread
UINT  pullFile( LPVOID pParam );//function for adb pull file thread


//define class


//bitmap head info
class BfInfo
{
public:
	unsigned int version;
	unsigned int bpp;
	unsigned int size;
	unsigned int width;
	unsigned int height;
	unsigned int red_offset;
	unsigned int red_length;
	unsigned int blue_offset;
	unsigned int blue_length;
	unsigned int green_offset;
	unsigned int green_length;
	unsigned int alpha_offset;
	unsigned int alpha_length;
};

//adb msg struct
typedef union {
	unsigned id;
	struct {
		unsigned id;
		unsigned namelen;
	} req;
	struct {
		unsigned id;
		unsigned mode;
		unsigned size;
		unsigned time;
	} stat;
	struct {
		unsigned id;
		unsigned mode;
		unsigned size;
		unsigned time;
		unsigned namelen;
	} dent;
	struct {
		unsigned id;
		unsigned size;
	} data;
	struct {
		unsigned id;
		unsigned msglen;
	} status;    
} syncmsg;






//define variable
BOOL isGetScreenSize = FALSE;
BOOL isRecord=FALSE;//xml record switch
CString recordXmlPath="";
BOOL forDebug = FALSE;

const int pointX=24;
const int pointY=10;
int ScreenHeight=0;
int ScreenWidth=0;

int mGlobalCountTime = GetTickCount(); //calculate every step runtime


const int byteBitConst = 8;

//convert string to adb command
CString ConvertAdbCommand(CString& s)
{
	CString tm;
	tm.Format("%.4X",strlen(s));
	return tm+s;
} 

UINT  ScreenCaping( LPVOID pParam )//function for screencap thread
{
	CAutoTestSystemDlgDlg* pThis = (CAutoTestSystemDlgDlg*)pParam;
	if (NULL == pThis) {
		return ERR;
	}

	//  get the static for display the screen mem
	CWnd *hwnd1 = pThis->GetDlgItem(IDC_SCREEN);
	if (NULL == hwnd1) {
		return ERR;
	}
	HDC hdc1 = hwnd1->GetDC()->m_hDC;
	RECT rect; 

	CSocket m_AdbSocket;
	m_AdbSocket.Create();
	m_AdbSocket.Connect("127.0.0.1",5037);

	//init adb and send message to it
	CString command  = "host:transport-usb";
	CString adbcommand = ConvertAdbCommand(command);
	m_AdbSocket.Send(adbcommand,strlen(adbcommand));
	char RecvCon[30];
	int  RecvSz =m_AdbSocket.Receive(RecvCon,sizeof RecvCon,0);
	RecvCon[RecvSz]=NULL;  

	int countBuf=0;
	BfInfo bf;
	command = "framebuffer:";
	adbcommand = ConvertAdbCommand(command);
	m_AdbSocket.Send(adbcommand,strlen(adbcommand));
	memset(RecvCon,0,sizeof RecvCon);
	RecvSz = m_AdbSocket.Receive(RecvCon,sizeof RecvCon,0);
	countBuf =m_AdbSocket.Receive(&bf,sizeof bf,0);


	//set the BITMAPFILEHEADER
	BITMAPFILEHEADER bitmapFileHeader;
	bitmapFileHeader.bfSize = (DWORD)sizeof(BITMAPFILEHEADER)+(DWORD)sizeof(BITMAPINFOHEADER)+(DWORD)((bf.width*32+31)/32)*4*bf.height;
	bitmapFileHeader.bfType = (WORD)0x4D42;
	bitmapFileHeader.bfReserved1 = 0;
	bitmapFileHeader.bfReserved2 = 0;
	bitmapFileHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER)+(DWORD)sizeof(BITMAPINFOHEADER);
	//set the BITMAPINFOHEADER
	BITMAPINFOHEADER bitmapInfoHeader;
	bitmapInfoHeader.biHeight = (LONG)bf.height;
	bitmapInfoHeader.biWidth = (LONG)bf.width;

	ScreenHeight = bitmapInfoHeader.biHeight;//get the screen height
	ScreenWidth = bitmapInfoHeader.biWidth;//get the screen widgh

	bitmapInfoHeader.biSize = (DWORD)sizeof(BITMAPINFOHEADER);
	bitmapInfoHeader.biPlanes = 1;
	bitmapInfoHeader.biCompression = BI_RGB;
	bitmapInfoHeader.biSizeImage = 0;
	bitmapInfoHeader.biClrUsed = 0;
	bitmapInfoHeader.biBitCount = (WORD)bf.bpp;
	bitmapInfoHeader.biClrImportant = 0;
	bitmapInfoHeader.biXPelsPerMeter = 0;
	bitmapInfoHeader.biYPelsPerMeter = 0;  
	//receive bitmap data from adb
	char* MBuf = (char*)malloc(bitmapFileHeader.bfSize);
	if (NULL == MBuf) {
		return ERR;
	}
	memset(MBuf,0,bitmapFileHeader.bfSize);
	memcpy(MBuf,&bitmapFileHeader,sizeof(BITMAPFILEHEADER));
	memcpy(MBuf+sizeof(BITMAPFILEHEADER),&bitmapInfoHeader,sizeof(BITMAPINFOHEADER));
	char* bmDataStart = MBuf+sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
	char* bmDataEnd = MBuf+sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+bf.size;
	char BmTempBuf[TMP_BITMAP_DATA];
	int currentRECV = 0;    
	while (currentRECV<bf.size) {
		int retCount =m_AdbSocket.Receive(BmTempBuf,sizeof BmTempBuf,0);
		if(retCount<0)retCount=0;
		memcpy(bmDataStart+currentRECV,BmTempBuf,retCount);
		currentRECV+=retCount;
	} 
	//socket complete it's task
	if (NULL == m_AdbSocket) {
		m_AdbSocket.Close();
	}
	//reversal the bitmap data , put bitmapbuffer bottom data to top by row
	int bmWidthBits = bf.width*bf.bpp/byteBitConst;
	char* rowBuf = new char[bmWidthBits];
	int rowNumConst = bmWidthBits;
	if (0 == rowNumConst) {
		return ERR;
	}
	int rowCount = 0;
	for (rowCount=rowNumConst;rowCount<=bf.height/2*rowNumConst;rowCount+=rowNumConst)
	{
		memcpy(rowBuf,bmDataStart+rowCount-rowNumConst,rowNumConst);
		memcpy(bmDataStart+rowCount-rowNumConst,bmDataEnd-rowCount,rowNumConst);
		memcpy(bmDataEnd-rowCount,rowBuf,rowNumConst);
	} 
	//draw the bitmap data to pc screen
	hwnd1->GetClientRect(&rect);
	SetStretchBltMode( hdc1,HALFTONE);
	int nRet = StretchDIBits(hdc1,0,0,
		rect.right-rect.left,rect.bottom-rect.top,
		0,0,bf.width,bf.height,(LPBYTE)bmDataStart,
		(LPBITMAPINFO)&bitmapInfoHeader,DIB_RGB_COLORS,SRCCOPY); 

	if (NULL != rowBuf || NULL != MBuf) {
		delete rowBuf;
		rowBuf = NULL;
		free(MBuf);
		MBuf = NULL; 
		return SUCCESS;
	} else {
		return ERR;
	}

}

// CAboutDlg dialog used for App About
class CAboutDlg : public CDialog
{
public:
	CAboutDlg();
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAutoTestSystemDlgDlg dialog

CAutoTestSystemDlgDlg::CAutoTestSystemDlgDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAutoTestSystemDlgDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAutoTestSystemDlgDlg)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CAutoTestSystemDlgDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAutoTestSystemDlgDlg)
	DDX_Control(pDX, IDC_PROGRESS1, m_Progress);
	DDX_Control(pDX, IDC_TEXT, m_Text);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAutoTestSystemDlgDlg, CDialog)
	//{{AFX_MSG_MAP(CAutoTestSystemDlgDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_SCREENSHOT, OnScreenshot)
	ON_BN_CLICKED(IDC_CONTROL, OnControl)
	ON_BN_CLICKED(IDC_MENU, OnMenu)
	ON_BN_CLICKED(IDC_HOME, OnHome)
	ON_BN_CLICKED(IDC_BACK, OnBack)
	ON_BN_CLICKED(IDC_WAKE, OnWake)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_BN_CLICKED(IDC_EXEXML, OnExexml)
	ON_BN_CLICKED(IDC_RECXML, OnRecxml)
	ON_BN_CLICKED(IDC_HALT, OnHalt)
	ON_BN_CLICKED(IDC_SENDTEXT, OnSendtext)
	ON_BN_CLICKED(IDC_PUSHFILE, OnPushfile)
	ON_BN_CLICKED(IDC_PULLFILE, OnPullfile)
	ON_BN_CLICKED(IDC_COMPARE, OnCompare)
	ON_BN_CLICKED(IDC_START, OnStart)
	ON_BN_CLICKED(IDC_UNCONTROL, OnUncontrol)
	ON_BN_CLICKED(IDC_STOP, OnStop)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAutoTestSystemDlgDlg message handlers

BOOL CAutoTestSystemDlgDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
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
	GetDlgItem(IDC_START)->EnableWindow(TRUE);
	GetDlgItem(IDC_STOP)->EnableWindow(FALSE);
	GetDlgItem(IDC_SCREENSHOT)->EnableWindow(FALSE);

	GetDlgItem(IDC_CONTROL)->EnableWindow(FALSE);
	GetDlgItem(IDC_UNCONTROL)->EnableWindow(FALSE);

	GetDlgItem(IDC_HOME)->EnableWindow(FALSE);
	GetDlgItem(IDC_MENU)->EnableWindow(FALSE);
	GetDlgItem(IDC_WAKE)->EnableWindow(FALSE);
	GetDlgItem(IDC_BACK)->EnableWindow(FALSE);

	GetDlgItem(IDC_RECXML)->EnableWindow(FALSE);
	GetDlgItem(IDC_HALT)->EnableWindow(FALSE);
	GetDlgItem(IDC_EXEXML)->EnableWindow(FALSE);

	GetDlgItem(IDC_SENDTEXT)->EnableWindow(FALSE);

	GetDlgItem(IDC_PUSHFILE)->EnableWindow(FALSE);
	GetDlgItem(IDC_PULLFILE)->EnableWindow(FALSE);
	GetDlgItem(IDC_COMPARE)->EnableWindow(FALSE);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CAutoTestSystemDlgDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CAutoTestSystemDlgDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

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
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CAutoTestSystemDlgDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CAutoTestSystemDlgDlg::OnScreenshot() 
{
	// TODO: Add your control notification handler code here
	// thread for screencap
	HANDLE hThread1;
	DWORD ThreadID1;

	hThread1=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)ScreenCaping,
		this,0,&ThreadID1);
	if (NULL == hThread1) {
		return ;
	}
	CloseHandle(hThread1);
	if (!isGetScreenSize)
	{
		isGetScreenSize = TRUE;
		GetDlgItem(IDC_CONTROL)->EnableWindow(TRUE);
		GetDlgItem(IDC_UNCONTROL)->EnableWindow(FALSE);
	}
}

void CAutoTestSystemDlgDlg::OnControl() 
{
	// TODO: Add your control notification handler code here
	int nTimeout = 1000;//timeout 1s
	setsockopt(m_MonkeySocket,SOL_SOCKET,SO_SNDTIMEO,(char*)&nTimeout,sizeof nTimeout);
	setsockopt(m_MonkeySocket,SOL_SOCKET,SO_RCVTIMEO,(char*)&nTimeout,sizeof nTimeout);
	if(m_MonkeySocket.Create()){ 
		m_MonkeySocket.Connect("127.0.0.1",5037);

		//init adb and send message to it
		CString command  = "host:transport-usb";
		CString adbcommand = ConvertAdbCommand(command);
		m_MonkeySocket.Send(adbcommand,strlen(adbcommand));
		char RecvCon[5];
		int  RecvSz = m_MonkeySocket.Receive(RecvCon,sizeof RecvCon,0);
		if (RecvSz<=0||RecvCon[0]=='F')
		{
			AfxMessageBox("socket connect error!");
			m_MonkeySocket.Close();
			return;
		}

		RecvCon[RecvSz] = NULL;  
		int countBuf = 0;
		command = "shell:";
		adbcommand = ConvertAdbCommand(command);
		m_MonkeySocket.Send(adbcommand,strlen(adbcommand));
		memset(RecvCon,0,sizeof RecvCon);
		RecvSz =m_MonkeySocket.Receive(RecvCon,sizeof RecvCon,0);
		if (RecvSz<=0||RecvCon[0]=='F')
		{
			AfxMessageBox("socket connect error!");
			m_MonkeySocket.Close();
			return;
		}
		// countBuf =m_MonkeySocket.Receive(&bf,sizeof bf,0);
		AfxMessageBox("control success!");

		GetDlgItem(IDC_CONTROL)->EnableWindow(FALSE);
		GetDlgItem(IDC_UNCONTROL)->EnableWindow(TRUE);

		GetDlgItem(IDC_HOME)->EnableWindow(TRUE);
		GetDlgItem(IDC_MENU)->EnableWindow(TRUE);
		GetDlgItem(IDC_WAKE)->EnableWindow(TRUE);
		GetDlgItem(IDC_BACK)->EnableWindow(TRUE);

		GetDlgItem(IDC_RECXML)->EnableWindow(TRUE);
		GetDlgItem(IDC_HALT)->EnableWindow(FALSE);
		GetDlgItem(IDC_EXEXML)->EnableWindow(FALSE);

		GetDlgItem(IDC_SENDTEXT)->EnableWindow(TRUE);

		GetDlgItem(IDC_PUSHFILE)->EnableWindow(TRUE);
		GetDlgItem(IDC_PULLFILE)->EnableWindow(TRUE);
		GetDlgItem(IDC_COMPARE)->EnableWindow(TRUE);
	}
}

void CAutoTestSystemDlgDlg::OnMenu() 
{
	// TODO: Add your control notification handler code here
	CString touchCommand;
	touchCommand = "input keyevent KEYCODE_MENU\n";
	checkIsRecord(isRecord,touchCommand);//check record

	m_MonkeySocket.Send(touchCommand,touchCommand.GetLength(),0);
}

void CAutoTestSystemDlgDlg::OnHome() 
{
	// TODO: Add your control notification handler code here
	CString touchCommand;

	touchCommand = "input keyevent KEYCODE_HOME\n";
	checkIsRecord(isRecord,touchCommand);//check record

	m_MonkeySocket.Send(touchCommand,touchCommand.GetLength(),0);
}

void CAutoTestSystemDlgDlg::OnBack() 
{
	// TODO: Add your control notification handler code here
	CString touchCommand;

	touchCommand = "input keyevent KEYCODE_BACK\n";
	checkIsRecord(isRecord,touchCommand);//check record

	m_MonkeySocket.Send(touchCommand,touchCommand.GetLength(),0);
}

void CAutoTestSystemDlgDlg::OnWake() 
{
	// TODO: Add your control notification handler code here
	CString touchCommand;

	touchCommand = "input keyevent KEYCODE_POWER\n";
	checkIsRecord(isRecord,touchCommand);//check record

	m_MonkeySocket.Send(touchCommand,touchCommand.GetLength(),0);
	//m_Command.SetWindowText(touchCommand);

	char recv[3] = {0};
	m_MonkeySocket.Receive(recv,2,0);
	recv[2] = NULL;
}

void CAutoTestSystemDlgDlg::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	CRect rect;
	HWND hwnd;
	GetDlgItem(IDC_SCREEN)->GetClientRect(rect);
	point.x=point.x-pointX;
	point.y=point.y-pointY;
	if (point.x>=rect.left&&point.x<=rect.right&&point.y<=rect.bottom&&point.y>=rect.top)
	{
		double rateX = (double)ScreenWidth/(rect.right-rect.left);
		double rateY = (double)ScreenHeight/(rect.bottom-rect.top);
		int X = rateX*(point.x-rect.left);
		int Y = rateY*(point.y-rect.top);
		CString touchCommand;
		mPoint.x = X;
		mPoint.y = Y;

	}
	CDialog::OnLButtonDown(nFlags, point);
}

void CAutoTestSystemDlgDlg::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	CRect rect;
	HWND hwnd;
	GetDlgItem(IDC_SCREEN)->GetClientRect(rect);
	point.x=point.x-pointX;
	point.y=point.y-pointY;
	if (point.x>=rect.left&&point.x<=rect.right&&point.y<=rect.bottom&&point.y>=rect.top)
	{
		double rateX = (double)ScreenWidth/(rect.right-rect.left);
		double rateY = (double)ScreenHeight/(rect.bottom-rect.top);
		int X = rateX*(point.x-rect.left);
		int Y = rateY*(point.y-rect.top);
		CString touchCommand;
		touchCommand.Format("input swipe %d %d %d %d\n",mPoint.x,mPoint.y,X,Y);//X+(X-mPoint.x),Y+(Y-mPoint.y));
		checkIsRecord(isRecord,touchCommand);//check record

		m_MonkeySocket.Send(touchCommand,touchCommand.GetLength(),0);
	}
	// m_Command.SetWindowText(touchCommand);
	CDialog::OnLButtonUp(nFlags, point);
}

UINT  ExecuteXml( LPVOID pParam )
{
	CAutoTestSystemDlgDlg* pThis = (CAutoTestSystemDlgDlg*)pParam;//get this pointer
	pThis->GetDlgItem(IDC_EXEXML)->EnableWindow(FALSE);
	pThis->GetDlgItem(IDC_HALT)->EnableWindow(FALSE);
	pThis->GetDlgItem(IDC_RECXML)->EnableWindow(FALSE);

	//get xml path
	CFileDialog xmldlg(TRUE,NULL,NULL,OFN_HIDEREADONLY,"All Files(*.*)|*.xml||");
	if (IDOK!=xmldlg.DoModal())
	{
		pThis->GetDlgItem(IDC_EXEXML)->EnableWindow(TRUE);
		pThis->GetDlgItem(IDC_HALT)->EnableWindow(FALSE);
		pThis->GetDlgItem(IDC_RECXML)->EnableWindow(TRUE);
		return 0;
	}
	CString execXmlPath = "";
	execXmlPath = xmldlg.GetPathName();

	// create a xml
	CMarkup xml;
	if(!xml.Load(execXmlPath))
	{
		AfxMessageBox("load "+execXmlPath+" fail!");
		pThis->GetDlgItem(IDC_EXEXML)->EnableWindow(TRUE);
		pThis->GetDlgItem(IDC_HALT)->EnableWindow(FALSE);
		pThis->GetDlgItem(IDC_RECXML)->EnableWindow(TRUE);
		return 0;
	}
	//parse xml
	xml.ResetPos();
	BOOL flag;
	flag = xml.FindElem(_T("actions"));
	CString touchCommand;
	xml.IntoElem();



	while (flag)
	{
		flag = xml.FindElem(_T("action"));
		int circle = atoi(xml.GetAttrib("circle"));
		int runtime = atoi(xml.GetAttrib("runtime"));

		if (!flag)
		{
			pThis->GetDlgItem(IDC_EXEXML)->EnableWindow(TRUE);
			pThis->GetDlgItem(IDC_HALT)->EnableWindow(FALSE);
			pThis->GetDlgItem(IDC_RECXML)->EnableWindow(TRUE);
			break;
		}
		touchCommand = xml.GetData();
		for(int i=0;i<circle;i++){

			Sleep(runtime);
			pThis->m_MonkeySocket.Send(touchCommand,touchCommand.GetLength(),0);

			//pThis->m_Command.SetWindowText(touchCommand);
		} 
	}  
	AfxMessageBox("finish");
	pThis->GetDlgItem(IDC_EXEXML)->EnableWindow(TRUE);
	pThis->GetDlgItem(IDC_HALT)->EnableWindow(FALSE);
	pThis->GetDlgItem(IDC_RECXML)->EnableWindow(TRUE);
	return 0;
}

void CAutoTestSystemDlgDlg::OnExexml() 
{
	// TODO: Add your control notification handler code here
	//Excute thread
	HANDLE hThreadExeXml;
	DWORD ThreadID;

	hThreadExeXml=CreateThread(NULL,
		0,
		(LPTHREAD_START_ROUTINE)ExecuteXml,
		this,0,&ThreadID);

	CloseHandle(hThreadExeXml);
}

void CAutoTestSystemDlgDlg::OnRecxml() 
{
	// TODO: Add your control notification handler code here
	CFileDialog xmldlg(FALSE,".xml",NULL,OFN_CREATEPROMPT | OFN_PATHMUSTEXIST,"All files (*.xml)|*.xml|*.*||");
	if (IDOK!=xmldlg.DoModal())
	{
		return;
	}
	recordXmlPath = xmldlg.GetPathName();
	//save the record xml
	mGlobalCountTime = GetTickCount();
	CMarkup xml;
	xml.SetDoc("<?xml version=\"1.0\" ?>\r\n");
	xml.AddElem("actions");
	xml.Save(recordXmlPath);
	isRecord = TRUE;//recording
	GetDlgItem(IDC_HALT)->EnableWindow(TRUE);
	GetDlgItem(IDC_RECXML)->EnableWindow(FALSE);
	GetDlgItem(IDC_EXEXML)->EnableWindow(FALSE);
}

void CAutoTestSystemDlgDlg::OnHalt() 
{
	// TODO: Add your control notification handler code here
	isRecord = FALSE;//stop record
	GetDlgItem(IDC_HALT)->EnableWindow(FALSE);
	GetDlgItem(IDC_RECXML)->EnableWindow(TRUE);
	GetDlgItem(IDC_EXEXML)->EnableWindow(TRUE);
}
void checkIsRecord(BOOL flag,CString command)
{
	if (flag)
	{
		CMarkup xml;
		xml.Load(recordXmlPath);
		xml.ResetPos();
		xml.FindElem("actions");
		xml.IntoElem();
		xml.AddElem("action");
		xml.SetData(command);
		xml.SetAttrib("circle",1);//execute once

		int timeCount = GetTickCount()-mGlobalCountTime;
		mGlobalCountTime = GetTickCount();
		xml.SetAttrib("runtime",timeCount);//one step runningtime
		xml.Save(recordXmlPath);
	}
}

void CAutoTestSystemDlgDlg::OnSendtext() 
{
	// TODO: Add your control notification handler code here
	CString text = "";
	m_Text.GetWindowText(text);
	CString sendCommand;
	sendCommand = "input text "+text+"\n";
	checkIsRecord(isRecord,sendCommand);//check record
	m_MonkeySocket.Send(sendCommand,sendCommand.GetLength()+text.GetLength(),0);
	// m_Command.SetWindowText(sendCommand);

	char recv[3] = {0};
	m_MonkeySocket.Receive(recv,2,0);
	recv[2] = NULL;
}

UINT  sendFile( LPVOID pParam ){
	CAutoTestSystemDlgDlg* pThis = (CAutoTestSystemDlgDlg*)pParam;
	pThis->GetDlgItem(IDC_PUSHFILE)->EnableWindow(FALSE);
	pThis->GetDlgItem(IDC_PULLFILE)->EnableWindow(FALSE);
	pThis->GetDlgItem(IDC_COMPARE)->EnableWindow(FALSE);
	CFileDialog fileDlgOpen(TRUE,".*",NULL,OFN_CREATEPROMPT | OFN_PATHMUSTEXIST,"All files (*.*)|*.*|*.*||");
	CString PCpathStr = ""; 

	if (IDOK != fileDlgOpen.DoModal())
	{
		pThis->GetDlgItem(IDC_PUSHFILE)->EnableWindow(TRUE);
		pThis->GetDlgItem(IDC_PULLFILE)->EnableWindow(TRUE);
		pThis->GetDlgItem(IDC_COMPARE)->EnableWindow(TRUE);
		return 0;
	}
	PCpathStr = fileDlgOpen.GetPathName();

	CString DEVpathStr;
	pThis->GetDlgItemText(IDC_DEVPATH,DEVpathStr);
	if("" == DEVpathStr){
		pThis->GetDlgItem(IDC_PUSHFILE)->EnableWindow(TRUE);
		pThis->GetDlgItem(IDC_PULLFILE)->EnableWindow(TRUE);
		pThis->GetDlgItem(IDC_COMPARE)->EnableWindow(TRUE);
		return 0;
	}

	CSocket m_AdbSocket;
	if(!m_AdbSocket.Create()){
		pThis->GetDlgItem(IDC_PUSHFILE)->EnableWindow(TRUE);
		pThis->GetDlgItem(IDC_PULLFILE)->EnableWindow(TRUE);
		pThis->GetDlgItem(IDC_COMPARE)->EnableWindow(TRUE);
		return 0;
	}
	m_AdbSocket.Connect("127.0.0.1",5037);

	//init adb and send message to it
	CString command  = "host:transport-usb";
	CString adbresult = ConvertAdbCommand(command);
	m_AdbSocket.Send(adbresult,strlen(adbresult));
	char RecvCon[5];
	int  RecvSz =m_AdbSocket.Receive(RecvCon,sizeof RecvCon,0);
	RecvCon[RecvSz]=NULL;  

	//into sync mode
	command = "sync:";
	adbresult = ConvertAdbCommand(command);
	m_AdbSocket.Send(adbresult,strlen(adbresult));
	char RecvContest[SOCK_REC_VNORMAL_LEN] = {0};
	RecvSz =m_AdbSocket.Receive(RecvContest,sizeof RecvContest,0);
	RecvCon[RecvSz]=NULL;  

	//send "SEND" syncmsg
	command = "SEND:";
	char* file = (LPSTR)(LPCTSTR)DEVpathStr;
	int filelen = strlen(file);
	syncmsg msg;
	msg.req.id = ID_SEND;
	msg.req.namelen = filelen;

	m_AdbSocket.Send(&msg.req,sizeof msg.req);
	m_AdbSocket.Send(file,strlen(file));


	//send file data
	pThis->m_Progress.SetRange(0,100);//Progress set 100% is over
	pThis->m_Progress.SetStep(1);//1% a step
	pThis->m_Progress.SetPos(0);//start postion is 0%
	CFile LocalFile(PCpathStr,CFile::modeCreate|CFile
		::modeNoTruncate|CFile::modeReadWrite);
	if (NULL == LocalFile)
	{
		pThis->GetDlgItem(IDC_PULLFILE)->EnableWindow(TRUE);
		pThis->GetDlgItem(IDC_PUSHFILE)->EnableWindow(TRUE);
		pThis->GetDlgItem(IDC_COMPARE)->EnableWindow(TRUE);
		m_AdbSocket.Close();
		return SUCCESS;
	}

	syncmsg sendmsg;
	char*  buffer = (char*)malloc(SYNC_DATA_MAX);
	if (buffer == NULL) {
		return ERR;
	}

	int LocalFileLen = LocalFile.GetLength();
	int ReadSumLen = 0;
	int PerDataLen = 0;
	while(LocalFileLen>ReadSumLen){
		int PerDataSumLen=0,PerDataSendlen = 0;
		if (LocalFileLen-ReadSumLen>=SYNC_DATA_MAX)
		{
			PerDataLen = SYNC_DATA_MAX;
		}else
		{
			PerDataLen = LocalFileLen-ReadSumLen;
		}
		sendmsg.data.id = ID_DATA;
		sendmsg.data.size = PerDataLen;
		m_AdbSocket.Send(&sendmsg.data, sizeof sendmsg.data, 0);

		//check send data can or not generate a msg
		while(PerDataSumLen < PerDataLen){
			int ReadSz=LocalFile.Read(buffer, PerDataLen-PerDataSumLen);
			PerDataSumLen+=ReadSz;
		}

		//check whether data is send all
		while(PerDataSendlen < PerDataLen){
			int SendSz = m_AdbSocket.Send(buffer+PerDataSendlen, PerDataLen, 0);
			PerDataSendlen+=SendSz;
			pThis->m_Progress.StepIt();
		}

		ReadSumLen+=PerDataLen;
		int pos = pThis->m_Progress.GetPos();
		if(pos == 100) pThis->m_Progress.SetPos(0);
	}


	//send "DONE" syncmsg tell the device data sending over 
	syncmsg Donemsg;
	Donemsg.data.id = ID_DONE;
	int ret = m_AdbSocket.Send(&Donemsg.req, sizeof Donemsg.req, 0);


	//device recv syncmsg "OKAY"
	syncmsg OKmsg;
	m_AdbSocket.Receive(&OKmsg.stat, sizeof OKmsg.stat, 0);
	if (OKmsg.stat.id == ID_OKAY)
	{
		AfxMessageBox("Send Success!");
	}
	pThis->m_Progress.SetPos(0);
	pThis->isFileCompare = FALSE;
	LocalFile.Close();
	free(buffer);
	m_AdbSocket.Close();
	pThis->GetDlgItem(IDC_PULLFILE)->EnableWindow(TRUE);
	pThis->GetDlgItem(IDC_PUSHFILE)->EnableWindow(TRUE);
	pThis->GetDlgItem(IDC_COMPARE)->EnableWindow(TRUE);
	return 0;
}

void CAutoTestSystemDlgDlg::OnPushfile() 
{
	// TODO: Add your control notification handler code here
	HANDLE hThread;
	DWORD ThreadID;

	hThread=CreateThread(NULL,
		0,(LPTHREAD_START_ROUTINE)sendFile,this,
		0,&ThreadID);

	CloseHandle(hThread);
}


UINT  pullFile( LPVOID pParam )//function for adb pull file thread
{


	CAutoTestSystemDlgDlg* pThis = (CAutoTestSystemDlgDlg*)pParam;
	pThis->GetDlgItem(IDC_PULLFILE)->EnableWindow(FALSE);
	pThis->GetDlgItem(IDC_PUSHFILE)->EnableWindow(FALSE);
	pThis->GetDlgItem(IDC_COMPARE)->EnableWindow(FALSE);
	CFileDialog fileDlgSave(FALSE,".*", NULL,OFN_CREATEPROMPT | OFN_PATHMUSTEXIST, "All files (*.*)|*.*|*.*||");
	CFileDialog fileDlgOpen(TRUE,".*", NULL,OFN_CREATEPROMPT | OFN_PATHMUSTEXIST, "All files (*.*)|*.*|*.*||");
	CString PCpathStr = ""; 


	//Compare or PullFile?
	if(pThis->isFileCompare){   
		if (IDOK!=fileDlgOpen.DoModal())
		{
			pThis->GetDlgItem(IDC_PULLFILE)->EnableWindow(TRUE);
			pThis->GetDlgItem(IDC_PUSHFILE)->EnableWindow(TRUE);
			pThis->GetDlgItem(IDC_COMPARE)->EnableWindow(TRUE);
			return 0;
		}
		PCpathStr = fileDlgOpen.GetPathName();
	}else{   
		if(IDOK != fileDlgSave.DoModal())
		{
			pThis->GetDlgItem(IDC_PULLFILE)->EnableWindow(TRUE);
			pThis->GetDlgItem(IDC_PUSHFILE)->EnableWindow(TRUE);
			pThis->GetDlgItem(IDC_COMPARE)->EnableWindow(TRUE);
			return 0;
		}
		PCpathStr = fileDlgSave.GetPathName();
	}




	CFile LocalFile(PCpathStr,CFile::modeCreate|CFile
		::modeNoTruncate|CFile::modeReadWrite);
	if (NULL == LocalFile)
	{
		pThis->GetDlgItem(IDC_PULLFILE)->EnableWindow(TRUE);
		pThis->GetDlgItem(IDC_PUSHFILE)->EnableWindow(TRUE);
		pThis->GetDlgItem(IDC_COMPARE)->EnableWindow(TRUE);
		return 0;
	}

	CString DEVpathStr;
	pThis->GetDlgItemText(IDC_DEVPATH,DEVpathStr);


	CSocket m_AdbSocket;
	if (!m_AdbSocket.Create()) {
		pThis->GetDlgItem(IDC_PULLFILE)->EnableWindow(TRUE);
		pThis->GetDlgItem(IDC_PUSHFILE)->EnableWindow(TRUE);
		pThis->GetDlgItem(IDC_COMPARE)->EnableWindow(TRUE);
		return 0;
	}
	m_AdbSocket.Connect("127.0.0.1",5037);

	//init adb and send message to device
	CString command  = "host:transport-usb";
	CString adbresult = ConvertAdbCommand(command);
	m_AdbSocket.Send(adbresult,strlen(adbresult));
	char RecvCon[SOCK_REC_VNORMAL_LEN] = {0};
	int  RecvSz =m_AdbSocket.Receive(RecvCon,sizeof RecvCon,0);
	RecvCon[RecvSz]=NULL;  


	//into sync mode
	command = "sync:";
	adbresult = ConvertAdbCommand(command);
	m_AdbSocket.Send(adbresult,strlen(adbresult));
	char RecvContest[SOCK_REC_VNORMAL_LEN] = {0};
	RecvSz = m_AdbSocket.Receive(RecvContest,sizeof RecvContest,0);
	RecvCon[RecvSz] = NULL;  

	//send syncmsg "RECV" to device
	command = "RECV:";
	char* file = (LPSTR)(LPCTSTR)DEVpathStr;

	int filelen = strlen(file);

	syncmsg msg;
	msg.req.id = ID_RECV;
	msg.req.namelen = filelen;

	m_AdbSocket.Send(&msg.req,sizeof msg.req);
	m_AdbSocket.Send(file,strlen(file));


	//recv data from device
	pThis->m_Progress.SetRange(0,100);
	pThis->m_Progress.SetStep(1);
	pThis->m_Progress.SetPos(0);
	syncmsg recvmsg;
	char*  buffer = (char*)malloc(SYNC_DATA_MAX);
	char* compareBuffer = (char*)malloc(SYNC_DATA_MAX);
	if (NULL == buffer || NULL == compareBuffer) {
		return ERR;
	}
	while(TRUE){
		RecvSz =m_AdbSocket.Receive(&recvmsg.data,sizeof recvmsg.data,0); 
		int dataLen = recvmsg.data.size;
		if(recvmsg.data.id == ID_DATA)
		{
			pThis->m_Progress.StepIt();
			int RECVedBufSz = 0;
			while(RECVedBufSz<dataLen){
				RecvSz = 0;
				RecvSz =m_AdbSocket.Receive(buffer,dataLen-RECVedBufSz,0); 
				buffer+=RecvSz;
				if (RecvSz<0)
				{
					AfxMessageBox("Socket error!");
					LocalFile.Close();
					free(buffer);
					free(compareBuffer);
					m_AdbSocket.Close();
					return -1;
				}
				RECVedBufSz = RECVedBufSz + RecvSz;
			}


			buffer-=dataLen;
			//compare data
			if(pThis->isFileCompare){
				int LocalFileReadSz=LocalFile.Read(compareBuffer,dataLen);
				if(LocalFileReadSz<recvmsg.data.size||strncmp(buffer,compareBuffer,dataLen)){
					AfxMessageBox("file diff");
					break;
				}
			}else{
				LocalFile.Write(buffer,dataLen);
			}
			int pos = pThis->m_Progress.GetPos();
			if(pos == 100)
				pThis->m_Progress.SetPos(0);
		}

		if (recvmsg.data.id == ID_DONE)
		{
			AfxMessageBox("Done Success!");
			break;
		}
	}
	pThis->m_Progress.SetPos(0);
	pThis->isFileCompare = FALSE;
	LocalFile.Close();
	free(buffer);
	free(compareBuffer);
	m_AdbSocket.Close();
	pThis->GetDlgItem(IDC_PULLFILE)->EnableWindow(TRUE);
	pThis->GetDlgItem(IDC_PUSHFILE)->EnableWindow(TRUE);
	pThis->GetDlgItem(IDC_COMPARE)->EnableWindow(TRUE);
	return 0;
}

void CAutoTestSystemDlgDlg::OnPullfile() 
{
	HANDLE hThread;
	DWORD ThreadID;

	hThread=CreateThread(NULL,0,
		(LPTHREAD_START_ROUTINE)pullFile,this,0,&ThreadID);

	CloseHandle(hThread);
}

//compare pc file and smartphone file
void CAutoTestSystemDlgDlg::OnCompare() 
{
	isFileCompare = TRUE;
	HANDLE hThread;
	DWORD ThreadID;

	hThread=CreateThread(NULL,0,
		(LPTHREAD_START_ROUTINE)pullFile,this,0,
		&ThreadID);

	CloseHandle(hThread);
}

void CAutoTestSystemDlgDlg::OnStart() 
{
	GetDlgItem(IDC_START)->EnableWindow(FALSE);
	GetDlgItem(IDC_STOP)->EnableWindow(TRUE);

	GetDlgItem(IDC_SCREENSHOT)->EnableWindow(TRUE);
}

void CAutoTestSystemDlgDlg::OnUncontrol() 
{
	m_MonkeySocket.Close();
	isGetScreenSize = FALSE;
	GetDlgItem(IDC_CONTROL)->EnableWindow(TRUE);
	GetDlgItem(IDC_UNCONTROL)->EnableWindow(FALSE);

	GetDlgItem(IDC_HOME)->EnableWindow(FALSE);
	GetDlgItem(IDC_MENU)->EnableWindow(FALSE);
	GetDlgItem(IDC_WAKE)->EnableWindow(FALSE);
	GetDlgItem(IDC_BACK)->EnableWindow(FALSE);

	GetDlgItem(IDC_RECXML)->EnableWindow(FALSE);
	GetDlgItem(IDC_HALT)->EnableWindow(FALSE);
	GetDlgItem(IDC_EXEXML)->EnableWindow(FALSE);

	GetDlgItem(IDC_SENDTEXT)->EnableWindow(FALSE);

	GetDlgItem(IDC_PUSHFILE)->EnableWindow(FALSE);
	GetDlgItem(IDC_PULLFILE)->EnableWindow(FALSE);
	GetDlgItem(IDC_COMPARE)->EnableWindow(FALSE);
}

void CAutoTestSystemDlgDlg::OnStop() 
{
	// TODO: Add your control notification handler code here
	if (GetDlgItem(IDC_UNCONTROL)->IsWindowEnabled())
	{
		AfxMessageBox("Please Uncontrol");
		return ;
	}
	isGetScreenSize = FALSE;
	GetDlgItem(IDC_START)->EnableWindow(TRUE);
	GetDlgItem(IDC_STOP)->EnableWindow(FALSE);
	GetDlgItem(IDC_SCREENSHOT)->EnableWindow(FALSE);
	GetDlgItem(IDC_CONTROL)->EnableWindow(FALSE);
}
