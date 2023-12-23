#include "pch.h"
#include "MyDropTarger.h"
#include "CommonFunc.h"

namespace {
	UINT fmtMozUrl = 0;
	UINT fmtUniformResourceLocatorW = 0;
	UINT fmtUniformResourceLocator = 0;

	void InitCbFormats() {
		fmtMozUrl = RegisterClipboardFormat(L"text/x-moz-url");
		fmtUniformResourceLocatorW = RegisterClipboardFormat(L"UniformResourceLocatorW");
		fmtUniformResourceLocator = RegisterClipboardFormat(L"UniformResourceLocator");
	}
}

MyDropTarget::MyDropTarget(UINT wndMessage)
	: wndMessage(wndMessage)
{
	InitCbFormats();
}

DROPEFFECT MyDropTarget::OnDragEnter(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	// see which formats are available when dragged
	//pDataObject->BeginEnumFormats();
	//FORMATETC fe;
	//std::vector<std::wstring> fmts;
	//while (pDataObject->GetNextFormat(&fe)) {
	//	wchar_t buf[200];
	//	if (GetClipboardFormatName(fe.cfFormat, buf, 200) > 0)
	//		fmts.push_back(buf);
	//	int g = 0;
	//}

	if (!enabled)
		isText = false;
	else {
		isText = pDataObject->IsDataAvailable(CF_UNICODETEXT)
			|| pDataObject->IsDataAvailable(fmtMozUrl)
			|| pDataObject->IsDataAvailable(fmtUniformResourceLocatorW)
			|| pDataObject->IsDataAvailable(fmtUniformResourceLocator);
	}
	return isText ? DROPEFFECT_COPY : DROPEFFECT_NONE;
}

DROPEFFECT MyDropTarget::OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	return isText ? DROPEFFECT_COPY : DROPEFFECT_NONE;
}

void MyDropTarget::OnDragLeave(CWnd* pWnd)
{
	isText = false;
}

BOOL MyDropTarget::OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point)
{
	droppedText.Empty();
	for (auto fmt : { fmtMozUrl, fmtUniformResourceLocatorW, fmtUniformResourceLocator, (UINT)CF_UNICODETEXT }) {
		HGLOBAL hGlobal = pDataObject->GetGlobalData(CF_UNICODETEXT);
		if (hGlobal == nullptr)
			continue;
		LPCTSTR lpszText = static_cast<LPCTSTR>(GlobalLock(hGlobal));
		if (lpszText != nullptr) {
			// Handle the dropped text (URL)
			// For example: MessageBox(pWnd->GetSafeHwnd(), lpszText, _T("Dropped URL"), MB_OK);
			if (fmt == fmtUniformResourceLocator)
				droppedText = fromUtf8((LPCSTR)lpszText);
			else
				droppedText = lpszText;
			GlobalUnlock(hGlobal);
			::PostMessage(m_hWnd, wndMessage, 0, 0);
		}
		GlobalFree(hGlobal);
		break;
	}
	return TRUE;
}

