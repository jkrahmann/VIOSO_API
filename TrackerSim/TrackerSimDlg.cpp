
// TrackerSimDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TrackerSim.h"
#include "TrackerSimDlg.h"
#include <fstream>
#include <atlpath.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
	virtual void OnCancel();
	virtual void OnOK();
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CTrackerSimDlg dialog




CTrackerSimDlg::CTrackerSimDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTrackerSimDlg::IDD, pParent)
	, m_bDirty( false )
	, m_bSending( false )
	, m_bPlaying( false )
	, m_sampleFreq( 1000 )
	, m_sendFreq( 100 )
	, m_port( 1998 )
	, m_xpos( 0 )
	, m_ypos( 0 )
	, m_zpos( 0 )
	, m_xdir( 0 )
	, m_ydir( 0 )
	, m_zdir( 0 )
	, m_hAccel( 0 )
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTrackerSimDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange( pDX );
	DDX_Text( pDX, IDC_EDIT_SAMPLEFREQ, m_sampleFreq );
	DDX_Text( pDX, IDC_EDIT_PORT, m_port );
	DDX_Text( pDX, IDC_EDIT_XPOS, m_xpos );
	DDX_Text( pDX, IDC_EDIT_YPOS, m_ypos );
	DDX_Text( pDX, IDC_EDIT_ZPOS, m_zpos );
	DDX_Text( pDX, IDC_EDIT_XDIR, m_xdir );
	DDX_Text( pDX, IDC_EDIT_YDIR, m_ydir );
	DDX_Text( pDX, IDC_EDIT_ZDIR, m_zdir );
	DDX_Text( pDX, IDC_EDIT_SENDFREQ, m_sendFreq );
	DDX_Control( pDX, IDC_LIST1, m_list );
	DDX_Control( pDX, IDC_STATIC_OUT, m_staticOut );
	DDX_Control( pDX, IDC_STATIC_POSXY, m_ctrlPosXY );
	DDX_Control( pDX, IDC_STATIC_POSZ, m_ctrlPosZ );
	DDX_Control( pDX, IDC_STATIC_DIRXY, m_ctrlDirXY );
	DDX_Control( pDX, IDC_STATIC_DIRZ, m_ctrlDirZ );
}

BEGIN_MESSAGE_MAP( CTrackerSimDlg, CDialog )
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE( WM_KICKIDLE, &CTrackerSimDlg::OnKickidle )
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED( IDC_BUTTON_OK, &CTrackerSimDlg::OnBnClickedButtonOk )
	ON_UPDATE_COMMAND_UI( IDC_BUTTON_OK, &CTrackerSimDlg::OnUpdateButtonOk )
	ON_BN_CLICKED( IDC_BUTTON_ADD, &CTrackerSimDlg::OnBnClickedButtonAdd )
	ON_UPDATE_COMMAND_UI( IDC_BUTTON_ADD, &CTrackerSimDlg::OnUpdateButtonAdd )
	ON_BN_CLICKED( IDC_BUTTON_DEL, &CTrackerSimDlg::OnBnClickedButtonDel )
	ON_UPDATE_COMMAND_UI( IDC_BUTTON_DEL, &CTrackerSimDlg::OnUpdateButtonDel )
	ON_COMMAND( ID_FILE_OPEN, &CTrackerSimDlg::OnFileOpen )
	ON_COMMAND( ID_FILE_SAVES, &CTrackerSimDlg::OnFileSave )
	ON_UPDATE_COMMAND_UI( ID_FILE_SAVES, &CTrackerSimDlg::OnUpdateFileSave )
	ON_COMMAND( ID_FILE_SAVEAS, &CTrackerSimDlg::OnFileSaveas )
	ON_COMMAND( ID_FILE_EXIT, &CTrackerSimDlg::OnIdcancel )
	ON_BN_CLICKED( IDC_BUTTON_SEND, &CTrackerSimDlg::OnBnClickedButtonSend )
	ON_UPDATE_COMMAND_UI( IDC_BUTTON_SEND, &CTrackerSimDlg::OnUpdateButtonSend )
	ON_WM_TIMER()
	ON_BN_CLICKED( IDC_BUTTON_PLAY, &CTrackerSimDlg::OnBnClickedButtonPlay )
	ON_UPDATE_COMMAND_UI( IDC_BUTTON_PLAY, &CTrackerSimDlg::OnUpdateButtonPlay )
	ON_BN_CLICKED( IDC_BUTTON_STOP, &CTrackerSimDlg::OnBnClickedButtonStop )
	ON_UPDATE_COMMAND_UI( IDC_BUTTON_STOP, &CTrackerSimDlg::OnUpdateButtonStop )
	ON_LBN_SELCHANGE( IDC_LIST1, &CTrackerSimDlg::OnLbnSelchangeList )
	ON_UPDATE_COMMAND_UI_RANGE( IDC_EDIT_XPOS, IDC_EDIT_ZDIR, &CTrackerSimDlg::OnUpdateRangeVals )
	ON_CONTROL_RANGE( EN_CHANGE, IDC_EDIT_SAMPLEFREQ, IDC_EDIT_ZDIR, &CTrackerSimDlg::OnEnChangeEditRange )
	ON_COMMAND( IDCANCEL, &CTrackerSimDlg::OnIdcancel )
	ON_COMMAND( IDOK, &CTrackerSimDlg::OnIdok )
	ON_COMMAND( IDC_EDIT_SAMPLEFREQ, &CTrackerSimDlg::OnEditSampleFreq )
	ON_COMMAND( IDC_EDIT_SAMPLEFREQ, &CTrackerSimDlg::OnEditSendFreq )
	ON_STN_CLICKED( IDC_STATIC_POSXY, &CTrackerSimDlg::OnStnClickedStaticPosXy )
	ON_STN_CLICKED( IDC_STATIC_POSZ, &CTrackerSimDlg::OnStnClickedStaticPosz )
	ON_STN_CLICKED( IDC_STATIC_DIRXY, &CTrackerSimDlg::OnStnClickedStaticDirXy )
	ON_STN_CLICKED( IDC_STATIC_DIRZ, &CTrackerSimDlg::OnStnClickedStaticDirz )
END_MESSAGE_MAP()


// CTrackerSimDlg message handlers

BOOL CTrackerSimDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
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
	CMenu other;
	other.LoadMenu( IDC_TRACKERSIM );
	SetMenu( &other );

	m_hAccel = LoadAccelerators( AfxGetResourceHandle(), MAKEINTRESOURCE( IDR_ACCELERATOR1 ) );

	ifStartSockets()
	{
		m_sock = Socket( SOCK_DGRAM, true, IPPROTO_UDP, false, 0, 0 );
	}


	//CString s = GetCommandLine();
	//if( !s.IsEmpty() )
	//{
	//	m_filename = s;
	//	readFromFile();
	//}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

BOOL CTrackerSimDlg::PreTranslateMessage( MSG* pMsg )
{
	// TODO: Add your specialized code here and/or call the base class
	if( m_hAccel && ::TranslateAccelerator( m_hWnd, m_hAccel, pMsg ) )
	{
		return TRUE;
	}
	else if( WM_MOUSEHWHEEL == pMsg->message && m_hWnd != pMsg->hwnd )
	{
		UINT ctrlId = ::GetDlgCtrlID( pMsg->hwnd );
		if( IDC_EDIT_SENDFREQ <= ctrlId && ctrlId <= IDC_EDIT_ZDIR )
		{
			int i = 0;
		}
	}
	return CDialog::PreTranslateMessage( pMsg );
}

void CTrackerSimDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	if( IDCLOSE == nID )
	{
		int i = 0;
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CTrackerSimDlg::OnPaint()
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
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CTrackerSimDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

afx_msg LRESULT CTrackerSimDlg::OnKickidle( WPARAM wParam, LPARAM lParam )
{
	UpdateDialogControls( this, FALSE );
	return 0;
}

void CTrackerSimDlg::OnInitMenuPopup( CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu )
{
	CDialog::OnInitMenuPopup( pPopupMenu, nIndex, bSysMenu );

	UINT i;
	CCmdUI ui;
	UINT const c = pPopupMenu->GetMenuItemCount();

	for( i = c - 1; i > c; i-- )
	{
		ui.m_nID = pPopupMenu->GetMenuItemID( i );
		ui.m_nIndex = i;
		ui.m_nIndexMax = c;
		ui.m_pMenu = pPopupMenu;
		if( -1 != ui.m_nID )
			CWnd::OnCmdMsg( (UINT)ui.m_nID, CN_UPDATE_COMMAND_UI, &ui, NULL );
	}
}

void CTrackerSimDlg::OnBnClickedButtonOk()
{
	int sel = m_list.GetCurSel();
	if( m_bPlaying || -1 == sel )
		return;
	CString s;
	s.Format( "%5.3f %5.3f %5.3f %5.3f %5.3f %5.3f", m_xpos, m_ypos, m_zpos, m_xdir, m_ydir, m_zdir );
	m_list.InsertString( sel, s );
	m_list.DeleteString( sel + 1 );
	m_bDirty = true;
}

void CTrackerSimDlg::OnUpdateButtonOk( CCmdUI *pCmdUI )
{
	if( m_bPlaying || -1 == m_list.GetCurSel() )
	{
		pCmdUI->Enable( FALSE );
	}
	else
	{
		pCmdUI->Enable( TRUE );
	}
}

void CTrackerSimDlg::OnBnClickedButtonAdd()
{
	if( m_bPlaying )
		return;
	CString s;
	s.Format( "%5.3f %5.3f %5.3f %5.3f %5.3f %5.3f", m_xpos, m_ypos, m_zpos, m_xdir, m_ydir, m_zdir );
	m_list.InsertString( m_list.GetCurSel(), s );
	m_bDirty = true;
}

void CTrackerSimDlg::OnUpdateButtonAdd( CCmdUI *pCmdUI )
{
	if( m_bPlaying )
	{
		pCmdUI->Enable( FALSE );
	}
	else
	{
		pCmdUI->Enable( TRUE );
	}
}

void CTrackerSimDlg::OnBnClickedButtonDel()
{
	int sel = m_list.GetCurSel();
	if( m_bPlaying || -1 == sel )
		return;
	m_list.DeleteString( sel );
	m_bDirty = true;
}

void CTrackerSimDlg::OnUpdateButtonDel( CCmdUI *pCmdUI )
{
	if( m_bPlaying || -1 == m_list.GetCurSel() )
	{
		pCmdUI->Enable( FALSE );
	}
	else
	{
		pCmdUI->Enable( TRUE );
	}
}


void CTrackerSimDlg::OnDestroy()
{
	CDialog::OnDestroy();
}

bool CTrackerSimDlg::readFromFile()
{
	CString line;
	std::ifstream s( m_filename );
	if( s.bad() )
		return false;
	m_list.ResetContent();
	while( !s.bad() && !s.eof() )
	{
		s.getline( line.GetBufferSetLength( MAX_PATH ), MAX_PATH );
		line.ReleaseBuffer();
		if( !line.IsEmpty() )
			m_list.InsertString( -1, line );
	}
	m_bDirty = FALSE;
	return true;
}

void CTrackerSimDlg::OnFileOpen()
{
	if( m_bDirty )
	{
		INT_PTR res = AfxMessageBox( IDS_SAVE_BEFORE_LOAD, MB_YESNOCANCEL );
		if( IDYES == res )
		{
			OnFileSave();
		}
		else if( IDCANCEL == res )
		{
			return;
		}
	}
	CString str;
	CFileDialog fileDlg( TRUE, NULL, NULL, OFN_EXPLORER | OFN_LONGNAMES | OFN_FILEMUSTEXIST, _T( "text file|*.txt|all files|*.*||" ), this, 0 );
	fileDlg.m_ofn.lpstrTitle = _T( "Load tracker data" );
	fileDlg.m_ofn.nMaxFile = MAX_PATH;
	fileDlg.m_ofn.lpstrFile = str.GetBufferSetLength( MAX_PATH );
	fileDlg.m_ofn.nFilterIndex = 0;
	if( IDOK != fileDlg.DoModal() )
	{
		return;
	}
	m_filename = fileDlg.m_ofn.lpstrFile;
	str.ReleaseBuffer();

	readFromFile();
}

bool CTrackerSimDlg::writeToFile()
{
	if( m_filename.IsEmpty() )
		return false;
	std::ofstream s( m_filename, std::ios::trunc );
	const int m = m_list.GetCount();
	for( int i = 0; i != m; i++ )
	{
		CString line;
		m_list.GetText( i, line );
		s << line << std::endl;
	}
	m_bDirty = false;
	return true;
}

void CTrackerSimDlg::OnFileSave()
{
	if( m_filename.IsEmpty() )
	{
		OnFileSaveas();
	}
	else
	{
		writeToFile();
	}
}


void CTrackerSimDlg::OnUpdateFileSave( CCmdUI *pCmdUI )
{
	if( m_bDirty )
	{
		pCmdUI->Enable( FALSE );
	}
	else
	{
		pCmdUI->Enable( TRUE );
	}
}


void CTrackerSimDlg::OnFileSaveas()
{
	CString str;
	CFileDialog fileDlg( FALSE, NULL, NULL, OFN_EXPLORER | OFN_LONGNAMES, _T( "text file|*.txt|all files|*.*||" ), this, 0 );
	fileDlg.m_ofn.lpstrTitle = _T( "Save tracker data" );
	fileDlg.m_ofn.nMaxFile = MAX_PATH;
	fileDlg.m_ofn.lpstrFile = str.GetBufferSetLength( MAX_PATH );
	fileDlg.m_ofn.nFilterIndex = 0;
	if( IDOK == fileDlg.DoModal() )
	{
		CPath p( fileDlg.m_ofn.lpstrFile );
		if( -1 == p.FindExtension() && 0 == fileDlg.m_ofn.nFilterIndex )
		{
			p.Append( ".txt" );
		}
		m_filename = p.m_strPath;
		writeToFile();
	}
	str.ReleaseBuffer();
}

void CTrackerSimDlg::OnBnClickedButtonSend()
{
	if( m_bSending )
	{
		m_bSending = false;
		KillTimer( 99 );
	}
	else
	{
		m_tick = ::GetTickCount();
		m_last[0] = m_xpos;
		m_last[1] = m_ypos;
		m_last[2] = m_zpos;
		m_last[3] = m_xdir;
		m_last[4] = m_ydir;
		m_last[5] = m_zdir;
		memcpy( m_curr, m_last, sizeof( m_curr ) );
		SetTimer( 99, m_sendFreq, NULL );
		m_bSending = true;
	}
}

void CTrackerSimDlg::OnUpdateButtonSend( CCmdUI *pCmdUI )
{
	if( !m_bSending )
	{
		pCmdUI->SetText( "Send" );
	}
	else
	{
		pCmdUI->SetText( "Stop" );
	}
}



void CTrackerSimDlg::OnTimer( UINT_PTR nIDEvent )
{
	// TODO: Add your message handler code here and/or call default
	DWORD tick = ::GetTickCount();
	DWORD delta = tick - m_tick;
	if( 99 == nIDEvent )
	{
		CString str;
		if( (DWORD)m_sampleFreq > delta ) // interpolating, when playing
		{
			float f = (float)delta / m_sampleFreq;
			m_curr[0] = ( 1.0f - f ) * m_last[0] + f * m_xpos;
			m_curr[1] = ( 1.0f - f ) * m_last[1] + f * m_ypos;
			m_curr[2] = ( 1.0f - f ) * m_last[2] + f * m_zpos;
			m_curr[3] = ( 1.0f - f ) * m_last[3] + f * m_xdir;
			m_curr[4] = ( 1.0f - f ) * m_last[4] + f * m_ydir;
			m_curr[5] = ( 1.0f - f ) * m_last[5] + f * m_zdir;
			str.Format( "%5.3f %5.3f %5.3f %5.3f %5.3f %5.3f", m_curr[0], m_curr[1], m_curr[2], m_curr[3], m_curr[4], m_curr[5] );
		}
		else
		{
			str.Format( "%5.3f %5.3f %5.3f %5.3f %5.3f %5.3f", m_xpos, m_ypos, m_zpos, m_xdir, m_ydir, m_zdir );
		}
		m_staticOut.SetWindowText( str );

		m_sock.sendDatagram( str, str.GetLength(), SocketAddress::broadcast( (unsigned short)m_port ), true );
	}
	else if( 100 == nIDEvent )
	{
		if( delta > ( DWORD )m_sampleFreq )
		{
			// read next line or loop
			int sel = m_list.GetCurSel() + 1;
			if( m_list.GetCount() <= sel )
			{
				sel = 0;
			}

			m_list.SetCurSel( sel );
			UpdateFromListItem( sel );
		}

	}
	else
	{
		CDialog::OnTimer( nIDEvent );
	}
}


void CTrackerSimDlg::OnBnClickedButtonPlay()
{
	if( -1 == m_list.GetCurSel() )
		m_list.SetCurSel( 0 );
	SetTimer( 100, m_sampleFreq, NULL );
	m_bPlaying = true;
}

void CTrackerSimDlg::OnUpdateButtonPlay( CCmdUI *pCmdUI )
{
	if( m_bPlaying || 0 == m_list.GetCount() )
	{
		pCmdUI->Enable( FALSE );
	}
	else
	{
		pCmdUI->Enable( TRUE );
	}
}

void CTrackerSimDlg::OnBnClickedButtonStop()
{
	m_bPlaying = false;
	KillTimer( 100 );
}

void CTrackerSimDlg::OnUpdateButtonStop( CCmdUI *pCmdUI )
{
	if( m_bPlaying )
	{
		pCmdUI->Enable( TRUE );
	}
	else
	{
		pCmdUI->Enable( FALSE );
	}
}

void CTrackerSimDlg::UpdateFromListItem( int sel )
{
	m_tick = ::GetTickCount();

	m_last[0] = m_xpos;
	m_last[1] = m_ypos;
	m_last[2] = m_zpos;
	m_last[3] = m_xdir;
	m_last[4] = m_ydir;
	m_last[5] = m_zdir;

	CString s;
	m_list.GetText( sel, s );
	sscanf_s( s, "%f %f %f %f %f %f", &m_xpos, &m_ypos, &m_zpos, &m_xdir, &m_ydir, &m_zdir );
	UpdateData( FALSE );
}

void CTrackerSimDlg::OnLbnSelchangeList()
{
	UpdateFromListItem( m_list.GetCurSel() );
}

void CTrackerSimDlg::OnUpdateRangeVals( CCmdUI * pCmdUI )
{
	if( m_bPlaying )
	{
		pCmdUI->Enable( FALSE );
		return;
	}
	pCmdUI->Enable( TRUE );
}


void CTrackerSimDlg::OnEnChangeEditRange( UINT nID )
{
	m_tick = ::GetTickCount();
	m_last[0] = m_xpos;
	m_last[1] = m_ypos;
	m_last[2] = m_zpos;
	m_last[3] = m_xdir;
	m_last[4] = m_ydir;
	m_last[5] = m_zdir;
	UpdateData( TRUE );
	if( IDC_EDIT_SAMPLEFREQ == nID ||
		IDC_EDIT_SENDFREQ )
	{
		PostMessage( WM_COMMAND, nID, 0 );
	}
}


void CAboutDlg::OnCancel()
{
	// TODO: Add your specialized code here and/or call the base class

	CDialog::OnCancel();
}


void CAboutDlg::OnOK()
{
	// TODO: Add your specialized code here and/or call the base class

	CDialog::OnOK();
}


void CTrackerSimDlg::OnIdcancel()
{
	if( m_bDirty )
	{
		UINT_PTR res = AfxMessageBox( IDS_SAVE_BEFORE_CLOSE, MB_YESNOCANCEL );
		if( IDYES == res )
		{
			OnFileSave();
		}
		else if( IDCANCEL == res )
		{
			return;
		}
	}
	OnCancel();
}

void CTrackerSimDlg::OnIdok()
{
 // we just do exactly nothing
}

void CTrackerSimDlg::OnEditSampleFreq()
{
	if( m_bPlaying )
	{
		KillTimer( 100 );
		SetTimer( 100, m_sampleFreq, NULL );
	}
}

void CTrackerSimDlg::OnEditSendFreq()
{
	if( m_bSending )
	{
		KillTimer( 99 );
		SetTimer( 99, m_sendFreq, NULL );
	}

}


void CTrackerSimDlg::OnStnClickedStaticPosXy()
{
	CPoint p;
	GetCursorPos( &p );
	CRect r;
	m_ctrlPosXY.GetWindowRect( &r );
	float shiftX = float( p.x - r.left ) / float( r.right - r.left ) - 0.5f;
	float shiftY = 0.5f - float( p.y - r.top ) / float( r.bottom - r.top );
	m_xpos += shiftX;
	m_ypos += shiftY;
	UpdateData( FALSE );
}


void CTrackerSimDlg::OnStnClickedStaticPosz()
{
	CPoint p;
	GetCursorPos( &p );
	CRect r;
	m_ctrlPosZ.GetWindowRect( &r );
	float shiftZ = 0.5f - float( p.y - r.top ) / float( r.bottom - r.top );
	m_zpos += shiftZ;
	UpdateData( FALSE );
}

void CTrackerSimDlg::OnStnClickedStaticDirXy()
{
	CPoint p;
	GetCursorPos( &p );
	CRect r;
	m_ctrlDirXY.GetWindowRect( &r );
	float shiftX = float( p.x - r.left ) / float( r.right - r.left ) - 0.5f;
	float shiftY = 0.5f - float( p.y - r.top ) / float( r.bottom - r.top );
	m_xdir += shiftX;
	m_ydir += shiftY;
	UpdateData( FALSE );
}


void CTrackerSimDlg::OnStnClickedStaticDirz()
{
	CPoint p;
	GetCursorPos( &p );
	CRect r;
	m_ctrlDirZ.GetWindowRect( &r );
	float shiftZ = 0.5f - float( p.y - r.top ) / float( r.bottom - r.top );
	m_zdir += shiftZ;
	UpdateData( FALSE );
}
