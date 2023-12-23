
// YouTooDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "YouToo.h"
#include "YouTooDlg.h"
#include "afxdialogex.h"

#include <afxinet.h>

#include <memory>
#include <vector>
#include <string>
#include <string_view>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


/*
download mpeg https://ffmpeg.org/download.html#build-windows
-> https://github.com/BtbN/FFmpeg-Builds/releases
-> unpack, add 'bin' to PATH

pip install yt-dlp

command like
yt-dlp -x --audio-format mp3 -o fino_al_giorno_nuovo.mp3 -- KkpmgBuc8XQ

https://github.com/ytdl-org/youtube-dl
https://github.com/yt-dlp/yt-dlp

// https://www.youtube.com/watch?v=23yNGer9Wqs
// https://youtu.be/q6EHd8T8Xgo


*/

namespace {
	UINT fmtMozUrl = 0;
	UINT fmtUniformResourceLocatorW = 0;
	UINT fmtUniformResourceLocator = 0;

	CRect GetMonitorRect(HWND hWnd) {
		HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO monitorInfo{ .cbSize = sizeof(MONITORINFO) };

		if (GetMonitorInfo(hMonitor, &monitorInfo))
			return monitorInfo.rcMonitor;

		return {};
	}

	CStringW fromUtf8(std::string_view utf8)
	{
		int utf16Length = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), nullptr, 0);
		CStringW ret;
		if (utf16Length <= 0)
			return ret;
		// Convert UTF-8 to UTF-16
		MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), ret.GetBufferSetLength(utf16Length), utf16Length);
		return ret;
	}

	std::vector<char> LoadFileFromURL(const CString& strURL, CString* err)
	{
		CInternetSession session(_T("MyApp"));
		CHttpFile* pFile = nullptr;
		std::vector<char> ret;

		try {
			pFile = static_cast<CHttpFile*>(session.OpenURL(strURL));
			if (pFile != nullptr) {
				char buffer[1024];
				UINT bytesRead = 0;
				while ((bytesRead = pFile->Read(buffer, sizeof(buffer))) > 0) {
					ret.insert(ret.end(), buffer, buffer + bytesRead);
				}

			}
		}
		catch (CInternetException* pEx) {
			TCHAR szError[256];
			pEx->GetErrorMessage(szError, 256);
			err->SetString(szError);
			pEx->Delete();
		}

		if (pFile != nullptr) {
			pFile->Close();
			delete pFile;
		}
		return ret;
	}

	CString GetTitleFrom(const CString& url, CString* err)
	{
		std::vector<char> content = LoadFileFromURL(url, err);
		CString title;
		if (content.empty())
			return title;
		std::string_view cc{ content.data(), content.size() };
		if (auto p1 = cc.find("<title>"); p1 != std::string_view::npos) {
			p1 += 7;
			if (auto p2 = cc.find("</title>", p1); p2 != std::string_view::npos) {
				auto tit = cc.substr(p1, p2 - p1);
				if (tit.ends_with(" - YouTube"))
					tit.remove_suffix(10);
				title = fromUtf8(tit);
			}
		}

		if (title.IsEmpty())
			err->SetString(L"Failed to find the title");
		return title;
	}

	CString MakeFileName(CString title)
	{
		title.Replace('|', '-');
		title.Replace(':', '.');
		title.Replace('*', '_');
		title.Replace('?', '_');
		title.Replace('<', '_');
		title.Replace('>', '_');
		title.Replace('/', '_');
		title.Replace('\\', '_');
		return title;
	}

	bool isIdChar(wchar_t c)
	{
		return (c >= '0' && c <= '9')
			|| (c >= 'A' && c <= 'Z')
			|| (c >= 'a' && c <= 'z')
			|| c == '-'
			|| c == '_';
	}

	CString ExtractVideoId(const  CString& url)
	{
		// find 11 chars in {0-9, A-Z, a-z, -, _}
		for (int i = 0; i < url.GetLength(); ) {
			bool maybeId = isIdChar(url[i]);
			int start = i++;
			while (i < url.GetLength() && maybeId == isIdChar(url[i]))
				++i;

			auto deb = url.Mid(start, i - start);
			if (maybeId && i - start == 11)
				return url.Mid(start, i - start);
		}
		return {};
	}

	bool TouchFile(const CString& file)
	{
		SYSTEMTIME systemTime;
		GetSystemTime(&systemTime);
		FILETIME fileTime;
		SystemTimeToFileTime(&systemTime, &fileTime);

		// Open the file
		HANDLE hFile = CreateFile(file, FILE_WRITE_ATTRIBUTES, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE) 
			return false;

		// Set the file times (created, modified, accessed) to the current time
		bool res = SetFileTime(hFile, &fileTime, &fileTime, &fileTime);
		CloseHandle(hFile);
		return res;
	}

	bool DownloadAudio(const CString& vidId, const CString& file)
	{
		STARTUPINFO startupInfo{ .cb = sizeof(STARTUPINFO) };
		PROCESS_INFORMATION processInfo;
		//yt-dlp -x --audio-format mp3 -o fino_al_giorno_nuovo.mp3 -- KkpmgBuc8XQ
		CString cmdLine = L"yt-dlp -x --audio-format mp3 -o \"" + file + "\" -- " + vidId;
		if (!CreateProcess(
			nullptr,             // No module name (use command line)
			cmdLine.GetBuffer(), // Command line
			nullptr,             // Process handle not inheritable
			nullptr,             // Thread handle not inheritable
			FALSE,               // Set handle inheritance to FALSE
			0,                   // No creation flags
			nullptr,             // Use parent's environment block
			nullptr,             // Use parent's starting directory
			&startupInfo,        // Pointer to STARTUPINFO structure
			&processInfo))       // Pointer to PROCESS_INFORMATION structure
			return false;

		// Wait for the process to complete
		WaitForSingleObject(processInfo.hProcess, INFINITE);

		// Clean up handles
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);

		return TouchFile(file);
	}

}

// CYouTooDlg dialog

#define WM_DROPPED_TEXT (WM_APP + 5)

BEGIN_MESSAGE_MAP(CYouTooDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(WM_DROPPED_TEXT, OnDroppedText)
	ON_BN_CLICKED(IDC_BUTTON1, &CYouTooDlg::OnBnClickedGo)
END_MESSAGE_MAP()

CYouTooDlg::CYouTooDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_YOUTOO_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CYouTooDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_URL);
	DDX_Control(pDX, IDC_EDIT2, m_status);
}

BOOL CYouTooDlg::OnInitDialog()
{
	fmtMozUrl = RegisterClipboardFormat(L"text/x-moz-url");
	fmtUniformResourceLocatorW = RegisterClipboardFormat(L"UniformResourceLocatorW");
	fmtUniformResourceLocator = RegisterClipboardFormat(L"UniformResourceLocator");

	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon


	m_dropTarget.Register(this);

	CRect mon = GetMonitorRect(m_hWnd);
	CRect dlg;
	GetWindowRect(&dlg);
	dlg.InflateRect(dlg.Width() * 3 / 8, dlg.Height() * 1 / 4);
	if(dlg.left < 0)
		dlg.OffsetRect(-dlg.left, 0);
	else if (dlg.right > mon.right)
		dlg.OffsetRect(mon.right - dlg.right, 0);
	if(dlg.top < 0)
		dlg.OffsetRect(0, -dlg.top);
	else if (dlg.bottom > mon.bottom)
		dlg.OffsetRect(0, mon.bottom - dlg.bottom);

	MoveWindow(&dlg);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CYouTooDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	CDialogEx::OnSysCommand(nID, lParam);
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CYouTooDlg::OnPaint()
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
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CYouTooDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

LRESULT CYouTooDlg::OnDroppedText(WPARAM wParam, LPARAM lParam) {
	// Handle the custom message here
	// wParam and lParam can carry additional data if needed
	auto txt = m_dropTarget.GetText();
	//if (txt.Find(L"//") < 0)
	//	txt = L"https://" + txt;
	m_URL.SetWindowText(txt);
	OnBnClickedGo();
	// Return a value if necessary
	return 0;
}



void CYouTooDlg::OnBnClickedGo()
{
	// TODO: Add your control notification handler code here
	CString url;
	m_URL.GetWindowText(url);
	AddLog(L"", true);
	CString id = ExtractVideoId(url);
	if (id.IsEmpty()) {
		AddLog(L"Video ID not found in URL");
		return;
	}
	CString normUrl = L"https://www.youtube.com/watch?v=" + id;
	AddLog(L"Check URL " + normUrl);
	m_status.UpdateWindow();

	CString err;
	CString title = GetTitleFrom(normUrl, &err);
	if (title.IsEmpty()) {
		AddLog(err);
		return;
	}
	AddLog(L"Title: " + title);

	CString fileName = MakeFileName(title) + L".mp3";
	CFileDialog fd(FALSE, L"mp3", fileName,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST,
		L"mp3 files|*.mp3|All files|*.*||", this);
	if (fd.DoModal() != IDOK)
		return;

	fileName = fd.GetPathName();
	AddLog(L"Loading " + fileName + L"...");
	bool res = DownloadAudio(id, fileName);
	AddLog(res? L"done" : L"error");
}

void CYouTooDlg::AddLog(const CString& text, bool discardPrevious)
{
	int len = m_status.GetWindowTextLength();
	m_status.SetSel(len, len, TRUE);
	if(!len || discardPrevious)
		m_status.SetWindowText(text);
	else
		m_status.ReplaceSel(L"\r\n" + text);
}


/// ////////////////////////////////////////////////////

inline DROPEFFECT MyDropTarget::OnDragEnter(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	//pDataObject->BeginEnumFormats();
	//FORMATETC fe;
	//std::vector<std::wstring> fmts;
	//while (pDataObject->GetNextFormat(&fe)) {
	//	wchar_t buf[200];
	//	if (GetClipboardFormatName(fe.cfFormat, buf, 200) > 0)
	//		fmts.push_back(buf);
	//	int g = 0;
	//}

	isText = pDataObject->IsDataAvailable(CF_UNICODETEXT)
		|| pDataObject->IsDataAvailable(fmtMozUrl)
		|| pDataObject->IsDataAvailable(fmtUniformResourceLocatorW)
		|| pDataObject->IsDataAvailable(fmtUniformResourceLocator);
	return isText ? DROPEFFECT_COPY : DROPEFFECT_NONE;
}

inline DROPEFFECT MyDropTarget::OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	return isText ? DROPEFFECT_COPY : DROPEFFECT_NONE;
}

inline void MyDropTarget::OnDragLeave(CWnd* pWnd)
{
	isText = false;
}

inline BOOL MyDropTarget::OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point)
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
			if(fmt == fmtUniformResourceLocator)
				droppedText = fromUtf8((LPCSTR)lpszText) ;
			else
				droppedText = lpszText;
			GlobalUnlock(hGlobal);
			::PostMessage(m_hWnd, WM_DROPPED_TEXT, 0, 0);
		}
		GlobalFree(hGlobal);
		break;
	}
	return TRUE;
}


void CYouTooDlg::OnOK()
{
	// just ignore
}
