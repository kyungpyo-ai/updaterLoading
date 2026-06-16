#include "pch.h"
#include "resource.h"
#include "MainDlg.h"

CMainDlg::CMainDlg(CWnd* pParent)
    : CDialogEx(IDD_MAIN_DIALOG, pParent)
{
}

CMainDlg::~CMainDlg()
{
}

void CMainDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMainDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_UPDATE, &CMainDlg::OnBtnUpdate)
END_MESSAGE_MAP()

BOOL CMainDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    return TRUE;
}

void CMainDlg::OnBtnUpdate()
{
    // 로딩창 표시
    CLoadingDlg::ShowLoading();

    // TODO: 실제 업데이트 작업 수행
    //       작업 완료 후 아래 한 줄로 닫기
    // CLoadingDlg::HideLoading();
}
