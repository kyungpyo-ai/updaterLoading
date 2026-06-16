#pragma once

#include <afxdialogex.h>
#include "resource.h"
#include "LoadingDlg.h"

class CMainDlg : public CDialogEx
{
public:
    CMainDlg(CWnd* pParent = nullptr);
    virtual ~CMainDlg();

    enum { IDD = IDD_MAIN_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnBtnUpdate();
    DECLARE_MESSAGE_MAP()
};
