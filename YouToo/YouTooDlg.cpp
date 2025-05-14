
// YouTooDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "YouToo.h"
#include "YouTooDlg.h"
#include "afxdialogex.h"
#include "CommonFunc.h"

#include <afxinet.h>

#include <memory>
#include <functional>
#include <fstream>
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

// very hi resolution https://www.youtube.com/watch?v=tUQmu7q0G-g
*/

namespace {
	CRect GetMonitorRect(HWND hWnd) {
		HMONITOR hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
		MONITORINFO monitorInfo{ .cbSize = sizeof(MONITORINFO) };

		if (GetMonitorInfo(hMonitor, &monitorInfo))
			return monitorInfo.rcMonitor;

		return {};
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
				while ((bytesRead = pFile->Read(buffer, sizeof(buffer))) > 0)
					ret.insert(ret.end(), buffer, buffer + bytesRead);
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

	int EntityCode(const CString& str)
	{
		if (str == L"&nbsp;")
			return 160;
		if (str == L"&lt;")
			return '<';
		if (str == L"&gt;")
			return '>';
		if (str == L"&amp;")
			return '&';
		if (str == L"&quot;")
			return '"';
		if (str == L"&apos;")
			return '\'';
		if (str == L"&copy;")
			return 169;
		return 0;
	}

	CString ReplaceHTMLEntities(CString str)
	{
		// convert entities as &#39; into characters
		for(int startPos = 0; startPos < str.GetLength(); ) {
			
			int entityStart = str.Find('&', startPos); // Find the next HTML entity starting from startPos
			if (entityStart == -1)
				break;
			
			int entityEnd = str.Find(';', entityStart); // Find the end of the HTML entity
			if (entityEnd == -1)  // bad html
				break;

			CString entity = str.Mid(entityStart, entityEnd - entityStart + 1);
			int charCode = str[entityStart + 1] == '#'
				? _ttoi(str.GetString() + entityStart + 2)
				: EntityCode(str.Mid(entityStart, entityEnd - entityStart + 1));

			if (!charCode) {
				startPos = entityEnd + 1;
				continue;
			}

			str.Delete(entityStart, entityEnd - entityStart);
			str.SetAt(entityStart, wchar_t(charCode));

			startPos = entityStart + 1;
		}

		return str;
	}

	CString GetTitleFrom(const CString& url, CString* err)
	{
		std::vector<char> content = LoadFileFromURL(url, err);
		CString title;
		if (content.empty())
			return title;
		//std::ofstream(L"page.txt", std::ios::binary).write(content.data(), content.size());
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
		// after </title>: <meta name="title" content="Bach - ....
		if (title.IsEmpty())
			err->SetString(L"Failed to find the title");

		return ReplaceHTMLEntities(title);
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
		title.Replace('\"', '\'');
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

	/*
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
	*/

	//void ReadOutput(HANDLE hOutputRead, HWND hEditControl) {
	void ReadOutput(HANDLE hOutputRead, std::function<void(const char*)> logFun) {
		char buffer[4096];
		DWORD dwRead;

		while (ReadFile(hOutputRead, buffer, sizeof(buffer), &dwRead, NULL) && dwRead) {
			buffer[dwRead] = '\0';
			logFun(buffer);
			//// Display the output in the edit control
			//int textLength = GetWindowTextLength(hEditControl);
			//SendMessage(hEditControl, EM_SETSEL, textLength, textLength);
			//SendMessage(hEditControl, EM_REPLACESEL, FALSE, (LPARAM)buffer);
		}
	}

	bool DownloadAudio(const CString& vidId, const CString& file, int format, HANDLE hStop, std::function<void(const char*)> logFun)
	{
		STARTUPINFO startupInfo{
			.cb = sizeof(STARTUPINFO),
			.dwFlags = STARTF_USESHOWWINDOW,
			.wShowWindow = SW_HIDE,
		};
		PROCESS_INFORMATION processInfo{};

		SECURITY_ATTRIBUTES saAttr{
			.nLength = sizeof(SECURITY_ATTRIBUTES),
			.lpSecurityDescriptor = NULL,
			.bInheritHandle = TRUE,
		};

		HANDLE hChildOutputRead = nullptr, hChildOutputWrite = nullptr;
		if (CreatePipe(&hChildOutputRead, &hChildOutputWrite, &saAttr, 0)) {
			SetHandleInformation(hChildOutputRead, HANDLE_FLAG_INHERIT, 0);
			startupInfo.hStdOutput = hChildOutputWrite;  // Redirect STDOUT to the write end of the pipe
			startupInfo.hStdError = hChildOutputWrite;   // Redirect STDERR to the write end of the pipe
			startupInfo.dwFlags |= STARTF_USESTDHANDLES;
		}
		auto closePipe = [&] {
			if (hChildOutputWrite)
				CloseHandle(hChildOutputWrite);
			if (hChildOutputRead)
				CloseHandle(hChildOutputRead);
			};

		//yt-dlp -x --audio-format mp3 -o fino_al_giorno_nuovo.mp3 -- KkpmgBuc8XQ
		// -x, --extract-audio    Convert video files to audio-only files (requires ffmpeg and ffprobe)
		// --audio-format FORMAT  Format to convert the audio to when -x is
		//                        used. (currently supported : best(default),
		//                        aac, alac, flac, m4a, mp3, opus, vorbis,
		//                        wav).You can specify multiple rules using
		//                        similar syntax as --remux - video

		/* video:
# Download and merge the best video-only format and the best audio-only format,
# or download the best combined format if video-only format is not available
$ yt-dlp -f "bv+ba/b"

		* 
yt-dlp -f "bv+ba/b" -o ich_ruf_zu -- FZUFZjuxmfU                          -> .webm
yt-dlp -f "bv+ba/b" -o ich_ruf_zu.mp4 -- FZUFZjuxmfU                      -> .mp4.webm
yt-dlp -f "bv+ba/b" --remux-video mp4 -o ich_ruf_zu4 -- FZUFZjuxmfU       -> .mp4, codec webm - iMovie doesn't accept
yt-dlp -f "bv+ba/b" --recode-video mp4 -o ich_ruf_zu5 -- FZUFZjuxmfU      -> .mp4, codec H264 - OK for iMovie

		separately ffmpeg convert:
		ffmpeg -i ich_ruf_zu1.webm -f mp4 ich_converted   -> makes mp4 file but without extension, extension must be added
		*/


		CString cmdLine = L"yt-dlp ";
		cmdLine += 
			format == 0? L"-x --audio-format mp3":
			format == 1 ? L"-f \"bv+ba/b\"":
			L"-f \"bv+ba/b\" --recode-video mp4";

		cmdLine += L" --no-mtime -o \"" + file + L"\" -- " + vidId;
		if (!CreateProcess(
			nullptr,             // No module name (use command line)
			cmdLine.GetBuffer(), // Command line
			nullptr,             // Process handle not inheritable
			nullptr,             // Thread handle not inheritable
			TRUE,                // Set handle inheritance to FALSE
			0,                   // No creation flags
			nullptr,             // Use parent's environment block
			nullptr,             // Use parent's starting directory
			&startupInfo,        // Pointer to STARTUPINFO structure
			&processInfo)) {      // Pointer to PROCESS_INFORMATION structure
			closePipe();
			return false;
		}

		std::jthread thReadPipe([&] { ReadOutput(hChildOutputRead, logFun); });
		// Wait for the process to complete or stop event
		HANDLE two[2] = { processInfo.hProcess, hStop };
		auto wRes = WaitForMultipleObjects(2, two, FALSE, INFINITE);
		if (wRes != WAIT_OBJECT_0) {
			// hStop was set
			TerminateProcess(processInfo.hProcess, 0);
		}

		// Clean up handles
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
		closePipe();
		thReadPipe.join();
		// "C:\Users\lukya\Downloads\Eugne Gigout Toccata en Si mineur - Olivier Penin, orgue Sainte Clotilde Paris VII.webm"
		//return TouchFile(file); // TODO remove Touch (due to --no-mtime)
		return true;
	}

	CString GetClipboardText(HWND hWnd)
	{
		CString text;

		if (::OpenClipboard(hWnd)) {
			HANDLE hClipboardData = ::GetClipboardData(CF_UNICODETEXT);
			if (hClipboardData != NULL) {
				LPWSTR lpClipboardText = static_cast<LPWSTR>(::GlobalLock(hClipboardData));
				if (lpClipboardText != NULL) {
					text = lpClipboardText;
					::GlobalUnlock(hClipboardData);
				}
			}
			::CloseClipboard();
		}

		return text;
	}
}

// CYouTooDlg dialog

#define WM_DROPPED_TEXT (WM_APP + 5)
#define WM_THREAD_FINISHED (WM_APP + 6)

BEGIN_MESSAGE_MAP(CYouTooDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(WM_DROPPED_TEXT, OnDroppedText)
	ON_MESSAGE(WM_THREAD_FINISHED, OnEndThread)
	ON_BN_CLICKED(IDC_BUTTON1, &CYouTooDlg::OnBnClickedGo)
	ON_BN_CLICKED(IDC_BUTTON2, &CYouTooDlg::OnBnClickedButton2)
END_MESSAGE_MAP()

CYouTooDlg::CYouTooDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_YOUTOO_DIALOG, pParent)
	, m_dropTarget(WM_DROPPED_TEXT)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CYouTooDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_URL);
	DDX_Control(pDX, IDC_EDIT2, m_status);
	DDX_Control(pDX, IDC_PROGRESS1, m_progress);
}

BOOL CYouTooDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_dropTarget.Register(this);

	CRect mon = GetMonitorRect(m_hWnd);
	CRect dlg;
	GetWindowRect(&dlg);
	dlg.InflateRect(dlg.Width() * 2 / 16, dlg.Height() * 1 / 4);
	if (dlg.left < 0)
		dlg.OffsetRect(-dlg.left, 0);
	else if (dlg.right > mon.right)
		dlg.OffsetRect(mon.right - dlg.right, 0);
	if (dlg.top < 0)
		dlg.OffsetRect(0, -dlg.top);
	else if (dlg.bottom > mon.bottom)
		dlg.OffsetRect(0, mon.bottom - dlg.bottom);

	MoveWindow(&dlg);
	m_URL.SetCueBanner(L"(can be dragged from the browser adddress bar)", TRUE);
	m_progress.SetRange(0, 100);
	CheckRadioButton(IDC_RADIO1, IDC_RADIO3, IDC_RADIO1);
	EnableControls(true);

	CString clbText = GetClipboardText(m_hWnd);
	CString id = ExtractVideoId(clbText);
	if (!id.IsEmpty())
		m_URL.SetWindowText(clbText);

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

	int format = GetCheckedRadioButton(IDC_RADIO1, IDC_RADIO3) - IDC_RADIO1;

	CString fileName = MakeFileName(title);
	LPCTSTR filter =
		format == 0? L"mp3 files|*.mp3|All files|*.*||":
		format == 1? L"Video files|*.mp4;*.webm;*.mkv;*.avi;*mov;*.flv|All files|*.*||":
		L"mp4 files|*.mp4|All files|*.*||";
	LPCTSTR defExt =
		format == 0? L"mp3":
		format == 1? nullptr:
		L"mp4";
	if (defExt)
		(fileName += L".") += defExt;

	bool folderDefined = false;
	if(IsDlgButtonChecked(IDC_CHECK1) == BST_CHECKED) // automatically to "downloads"
	{
		CString downloadFolder;
		// Variable to hold the path
		LPWSTR path = nullptr;
		if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Downloads, 0, NULL, &path))) {
			downloadFolder = path;
			CoTaskMemFree(path); // Free the memory allocated for the path
			folderDefined = true;
			//downloadFolder = L"C:\\Users\\lukya\\Downloads"
			fileName = downloadFolder + "\\" + fileName;
		}
	}

	if (!folderDefined) {
		CFileDialog fd(FALSE, defExt, fileName,
			OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST,// | OFN_EXTENSIONDIFFERENT,
			filter, this);

		if (fd.DoModal() != IDOK)
			return;

		fileName = fd.GetPathName();
	}

	AddLog(L"Loading " + fileName + L"...");
	EnableControls(false);
	m_stopEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	m_thread = std::jthread(&CYouTooDlg::DownloadInThread, this, id, fileName, format);
}

void CYouTooDlg::DownloadInThread(CString id, CString fileName, int format) // executed in thread
{
	bool res = DownloadAudio(id, fileName, format, m_stopEvent, [this](const char* txt) { OnPipeRead(txt); });
	PostMessage(WM_THREAD_FINISHED, 0, res);
}

void CYouTooDlg::OnPipeRead(const char* txt)
{
	CStringA str = txt;
	str.Trim();
	if (strncmp(str, "[download]", 10) == 0) {
		m_progress.SetPos(atoi(str.GetString() + 10));
	}
	else
		AddLog(fromUtf8(str.GetString()));
}

LRESULT CYouTooDlg::OnEndThread(WPARAM wParam, LPARAM lParam)
{
	m_thread->join();
	m_thread.reset();
	CloseHandle(m_stopEvent);
	m_stopEvent = 0;
	EnableControls(true);
	AddLog(lParam ? L"Success" : L"Failure");
	if (m_closeOnFinish)
		PostMessage(WM_SYSCOMMAND, SC_CLOSE);
	return 0;
}

void CYouTooDlg::AddLog(const CString& text, bool discardPrevious)
{
	int len = m_status.GetWindowTextLength();
	m_status.SetSel(len, len, TRUE);
	if (discardPrevious)
		m_status.SetWindowText(text + L"\r\n");
	else
		m_status.ReplaceSel(text + L"\r\n");
}

void CYouTooDlg::EnableControls(bool en)
{
	m_dropTarget.Enable(en);
	m_URL.EnableWindow(en);
	for (auto id : { IDC_CHECK1, IDC_RADIO1, IDC_RADIO2, IDC_RADIO3, IDC_BUTTON1 }) { // "to 'download'", "audio", "video", "mp4", "go"
		if (auto btn = GetDlgItem(id))
			btn->EnableWindow(en);
	}
	if (auto btn = GetDlgItem(IDC_BUTTON2)) // "stop"
		btn->ShowWindow(en ? SW_HIDE : SW_SHOW);
	m_progress.ShowWindow(en ? SW_HIDE : SW_SHOW);
	m_progress.SetPos(0);

}

void CYouTooDlg::OnOK()
{
	// just ignore
}


void CYouTooDlg::OnCancel()
{
	if (!m_thread)
		CDialogEx::OnCancel();
	else {
		m_closeOnFinish = true;
		SetEvent(m_stopEvent);
	}
}


void CYouTooDlg::OnBnClickedButton2()
{
	// stop
	SetEvent(m_stopEvent);
}
