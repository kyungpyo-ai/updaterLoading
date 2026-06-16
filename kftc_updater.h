#pragma once

#include <afxwin.h>
#include <afxwinappex.h>

class CKftcUpdaterApp : public CWinApp
{
public:
    CKftcUpdaterApp();

    virtual BOOL InitInstance();
    DECLARE_MESSAGE_MAP()
};

extern CKftcUpdaterApp theApp;
