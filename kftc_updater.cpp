#include "pch.h"
#include "kftc_updater.h"
#include "MainDlg.h"

BEGIN_MESSAGE_MAP(CKftcUpdaterApp, CWinApp)
END_MESSAGE_MAP()

CKftcUpdaterApp theApp;

CKftcUpdaterApp::CKftcUpdaterApp()
{
}

BOOL CKftcUpdaterApp::InitInstance()
{
    CWinApp::InitInstance();

    CMainDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();

    return FALSE;
}
