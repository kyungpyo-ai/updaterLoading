#pragma once

#include <afxdialogex.h>
#include "resource.h"
#include <gdiplus.h>

class CLoadingDlg : public CDialogEx
{
    DECLARE_DYNAMIC(CLoadingDlg)

public:
    // -------------------------------------------------------
    // 외부에서 이 두 함수만 호출하면 됩니다.
    // 로딩창은 자체 스레드에서 동작하므로
    // 메인 스레드가 무거운 작업을 해도 스피너가 멈추지 않습니다.
    // -------------------------------------------------------
    static void ShowLoading();
    static void HideLoading();

    enum { IDD = IDD_LOADING_DIALOG };

protected:
    CLoadingDlg();
    virtual ~CLoadingDlg();

    virtual BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange* pDX);
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnDestroy();
    DECLARE_MESSAGE_MAP()

private:
    static const UINT TIMER_ANIM = 1;
    static const int  DLG_W     = 360;
    static const int  DLG_H     = 230;
    static const int  CORNER_R  = 16;

    // 다이얼로그 스레드 간 공유 — HWND만 저장 (스레드 안전)
    static HWND   s_hWnd;
    static HANDLE s_hReadyEvent; // ShowLoading이 대기할 이벤트

    static UINT LoadingThreadProc(LPVOID); // 다이얼로그 전용 스레드

    ULONG_PTR m_gdiplusToken;
    float     m_fAngle;
    HICON     m_hAppIcon;

    void DrawBackground(Gdiplus::Graphics& g);
    void DrawLogo(Gdiplus::Graphics& g);
    void DrawSeparator(Gdiplus::Graphics& g);
    void DrawSpinner(Gdiplus::Graphics& g);
    void DrawLabel(Gdiplus::Graphics& g);
};
