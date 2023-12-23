#pragma once

#include "MyDropTarger.h"

#include <thread>
#include <optional>

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
	afx_msg LRESULT OnEndThread(WPARAM wParam, LPARAM lParam);
	void AddLog(const CString& text, bool discardPrevious = false);
	void DownloadInThread(CString id, CString fileName); // executed in thread
	void EnableControls(bool en);

	DECLARE_MESSAGE_MAP()
	virtual void OnOK();
	virtual void OnCancel();

	MyDropTarget m_dropTarget;
	CEdit m_URL;
	CEdit m_status;

	HANDLE m_stopEvent = nullptr;
	std::optional<std::jthread> m_thread;
	bool m_closeOnFinish = false;
};
