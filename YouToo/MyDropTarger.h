#pragma once

class MyDropTarget : public COleDropTarget {
public:
	MyDropTarget(UINT wndMessage);
	DROPEFFECT OnDragEnter(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point) override;
	DROPEFFECT OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)override;
	void OnDragLeave(CWnd* pWnd) override;
	BOOL OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point) override;

	const auto& GetText() const { return droppedText; }
	void Enable(bool en) { enabled = en; }
private:
	UINT wndMessage;
	CString droppedText;
	bool isText = false;
	bool enabled = true;
};

