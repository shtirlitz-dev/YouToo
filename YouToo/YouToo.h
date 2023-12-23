
// YouToo.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CYouTooApp:
// See YouToo.cpp for the implementation of this class
//

class CYouTooApp : public CWinApp
{
public:
	CYouTooApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
	virtual int ExitInstance();
};

extern CYouTooApp theApp;
