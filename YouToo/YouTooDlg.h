
// YouTooDlg.h : header file
//

#pragma once

class MyDropTarget : public COleDropTarget {
public:
	DROPEFFECT OnDragEnter(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point) override;
	DROPEFFECT OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)override;
	void OnDragLeave(CWnd* pWnd) override;
	BOOL OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point) override;

	const auto& GetText() const { return droppedText; }
private:
	CString droppedText;
	bool isText = false;
};

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

	MyDropTarget m_dropTarget;
	CEdit m_URL;
	CEdit m_status;
	virtual void OnOK();
};
