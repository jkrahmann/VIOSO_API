
// TrackerSimDlg.h : header file
//

#pragma once
#include "../VIOSOWarpBlend/socket.h"


// CTrackerSimDlg dialog
class CTrackerSimDlg : public CDialog
{
// Construction
public:
	CTrackerSimDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_TRACKERSIM_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;
	bool m_bDirty;
	CString m_filename;
	bool m_bSending;
	bool m_bPlaying;
	HACCEL m_hAccel;
	Socket m_sock;

	// control values
	unsigned int m_port;
	float m_xpos;
	float m_ypos;
	float m_zpos;
	float m_xdir;
	float m_ydir;
	float m_zdir;
	int m_sampleFreq;
	int m_sendFreq;
	DWORD m_tick;
	float m_last[6];
	float m_curr[6];
	CListBox m_list;
	CStatic m_staticOut;
	CButton m_ctrlPosXY;
	CButton m_ctrlPosZ;
	CButton m_ctrlDirXY;
	CButton m_ctrlDirZ;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage( MSG* pMsg );

	bool writeToFile();
	bool readFromFile();
	void UpdateFromListItem( int sel );

	DECLARE_MESSAGE_MAP()

	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnClickedButtonOk();
	afx_msg void OnUpdateButtonOk( CCmdUI *pCmdUI );
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnUpdateButtonAdd( CCmdUI *pCmdUI );
	afx_msg void OnBnClickedButtonDel();
	afx_msg void OnUpdateButtonDel( CCmdUI *pCmdUI );
	afx_msg void OnDestroy();
public:
	afx_msg void OnFileOpen();
	afx_msg void OnFileSave();
	afx_msg void OnUpdateFileSave( CCmdUI *pCmdUI );
	afx_msg void OnFileSaveas();
protected:
	afx_msg LRESULT OnKickidle( WPARAM wParam, LPARAM lParam );
	afx_msg void OnInitMenuPopup( CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu );
public:
	afx_msg void OnBnClickedButtonSend();
	afx_msg void OnUpdateButtonSend( CCmdUI *pCmdUI );
	afx_msg void OnTimer( UINT_PTR nIDEvent );
	afx_msg void OnBnClickedButtonPlay();
	afx_msg void OnUpdateButtonPlay( CCmdUI *pCmdUI );
	afx_msg void OnBnClickedButtonStop();
	afx_msg void OnUpdateButtonStop( CCmdUI *pCmdUI );
	afx_msg void OnLbnSelchangeList();
	afx_msg void OnUpdateRangeVals( CCmdUI *pCmdUI );
	afx_msg void OnEnChangeEditRange( UINT nID );
	afx_msg void OnIdcancel();
	afx_msg void OnIdok();
	afx_msg void OnEditSampleFreq();
	afx_msg void OnEditSendFreq();
	afx_msg void OnStnClickedStaticPosXy();
	afx_msg void OnStnClickedStaticPosz();
	afx_msg void OnStnClickedStaticDirXy();
	afx_msg void OnStnClickedStaticDirz();
};
