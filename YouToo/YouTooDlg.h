#pragma once

#include "MyDropTarger.h"

#include <thread>

// CYouTooDlg dialog
class CYouTooDlg : public CDialogEx
{
	// Construction
public:
	CYouTooDlg(CWnd* pParent = nullptr);	// standard constructor

	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_YOUTOO_DIALOG };
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
	afx_msg LRESULT OnDroppedText(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedGo();
	void AddLog(const CString& text, bool discardPrevious = false);

	DECLARE_MESSAGE_MAP()
	virtual void OnOK();
	virtual void OnCancel();

	MyDropTarget m_dropTarget;
	CEdit m_URL;
	CEdit m_status;
};
