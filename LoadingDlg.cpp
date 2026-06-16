#include "pch.h"
#include "resource.h"
#include "LoadingDlg.h"
#include <dwmapi.h>

using namespace Gdiplus;

IMPLEMENT_DYNAMIC(CLoadingDlg, CDialogEx)

HWND   CLoadingDlg::s_hWnd       = nullptr;
HANDLE CLoadingDlg::s_hReadyEvent = nullptr;

// =============================================================
// 공개 정적 API
// =============================================================

void CLoadingDlg::ShowLoading()
{
    // 이미 떠 있으면 무시
    if (s_hWnd && ::IsWindow(s_hWnd))
        return;

    // 다이얼로그 스레드가 HWND를 세팅할 때까지 대기하기 위한 이벤트
    s_hReadyEvent = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);

    // 로딩창 전용 스레드 시작
    AfxBeginThread(LoadingThreadProc, nullptr);

    // 다이얼로그가 화면에 나타날 때까지 최대 3초 대기
    ::WaitForSingleObject(s_hReadyEvent, 3000);
    ::CloseHandle(s_hReadyEvent);
    s_hReadyEvent = nullptr;
}

void CLoadingDlg::HideLoading()
{
    // WM_CLOSE를 다이얼로그 스레드로 전달 — OnDestroy에서 스레드 종료
    if (s_hWnd && ::IsWindow(s_hWnd))
        ::PostMessage(s_hWnd, WM_CLOSE, 0, 0);
}

// =============================================================
// 로딩창 전용 스레드 함수
// =============================================================

UINT CLoadingDlg::LoadingThreadProc(LPVOID)
{
    CLoadingDlg* pDlg = new CLoadingDlg();
    pDlg->Create(IDD_LOADING_DIALOG, CWnd::GetDesktopWindow());
    pDlg->ShowWindow(SW_SHOW);
    pDlg->UpdateWindow();

    // HWND 저장 후 메인 스레드 대기 해제
    s_hWnd = pDlg->GetSafeHwnd();
    if (s_hReadyEvent)
        ::SetEvent(s_hReadyEvent);

    // 이 스레드 전용 메시지 루프 — 다이얼로그가 살아있는 동안 계속 실행
    MSG msg;
    while (::GetMessage(&msg, nullptr, 0, 0))
    {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }

    // 메시지 루프 종료 후 정리
    delete pDlg;
    return 0;
}

// =============================================================
// 내부 구현
// =============================================================

CLoadingDlg::CLoadingDlg()
    : CDialogEx(IDD_LOADING_DIALOG, nullptr)
    , m_gdiplusToken(0)
    , m_fAngle(0.f)
{
    GdiplusStartupInput si;
    GdiplusStartup(&m_gdiplusToken, &si, nullptr);
}

CLoadingDlg::~CLoadingDlg()
{
    if (m_gdiplusToken)
        GdiplusShutdown(m_gdiplusToken);
}

void CLoadingDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CLoadingDlg, CDialogEx)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_TIMER()
    ON_WM_DESTROY()
END_MESSAGE_MAP()

BOOL CLoadingDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 타이틀바 / 테두리 완전 제거
    ModifyStyle(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | DS_MODALFRAME, 0);
    ModifyStyleEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE, 0);

    // 작업 영역 기준 화면 정중앙 계산
    RECT workArea = {};
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
    int x = workArea.left + ((workArea.right  - workArea.left) - DLG_W) / 2;
    int y = workArea.top  + ((workArea.bottom - workArea.top)  - DLG_H) / 2;

    // 위치 + 크기 + 최상위(TOPMOST) 한 번에 설정
    SetWindowPos(&wndTopMost, x, y, DLG_W, DLG_H, SWP_FRAMECHANGED | SWP_SHOWWINDOW);

    // 둥근 모서리
    HRGN hRgn = CreateRoundRectRgn(0, 0, DLG_W + 1, DLG_H + 1,
                                   CORNER_R * 2, CORNER_R * 2);
    SetWindowRgn(hRgn, TRUE);

    // DWM 드롭 쉐도우
    MARGINS margins = { 1, 1, 1, 1 };
    DwmExtendFrameIntoClientArea(GetSafeHwnd(), &margins);

    // 스피너 애니메이션 (~60fps)
    SetTimer(TIMER_ANIM, 16, nullptr);

    return TRUE;
}

void CLoadingDlg::OnDestroy()
{
    KillTimer(TIMER_ANIM);
    s_hWnd = nullptr;
    CDialogEx::OnDestroy();

    // 스레드 메시지 루프 종료
    ::PostQuitMessage(0);
}

// ---------------------------------------------------------------------------
// 더블버퍼 페인팅
// ---------------------------------------------------------------------------
void CLoadingDlg::OnPaint()
{
    CPaintDC dc(this);

    CRect rc;
    GetClientRect(&rc);

    CDC memDC;
    memDC.CreateCompatibleDC(&dc);
    CBitmap bmp;
    bmp.CreateCompatibleBitmap(&dc, rc.Width(), rc.Height());
    CBitmap* pOld = memDC.SelectObject(&bmp);

    Graphics g(memDC.m_hDC);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);

    DrawBackground(g);
    DrawSpinner(g);
    DrawLabel(g);

    dc.BitBlt(0, 0, rc.Width(), rc.Height(), &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOld);
}

BOOL CLoadingDlg::OnEraseBkgnd(CDC*)
{
    return TRUE;
}

void CLoadingDlg::DrawBackground(Graphics& g)
{
    SolidBrush bg(Color(255, 255, 255, 255));
    g.FillRectangle(&bg, 0, 0, DLG_W, DLG_H);
}

void CLoadingDlg::DrawSpinner(Graphics& g)
{
    const float cx   = DLG_W / 2.f;
    const float cy   = DLG_H / 2.f - 18.f;
    const float r    = 30.f;
    const float penW = 5.f;
    const RectF bounds(cx - r, cy - r, r * 2.f, r * 2.f);

    Pen trackPen(Color(255, 237, 240, 244), penW);
    g.DrawEllipse(&trackPen, bounds);

    GraphicsPath arcPath;
    arcPath.AddArc(bounds, m_fAngle, 270.f);

    Pen spinPen(Color(255, 49, 130, 246), penW);
    spinPen.SetLineCap(LineCapRound, LineCapRound, DashCapFlat);
    g.DrawPath(&spinPen, &arcPath);
}

void CLoadingDlg::DrawLabel(Graphics& g)
{
    const float cy      = DLG_H / 2.f - 18.f;
    const float spinR   = 30.f;
    const float textTop = cy + spinR + 20.f;

    FontFamily ff(L"\xB9D1\xC740 \xACE0\xB515");
    FontFamily fallback(L"MS Shell Dlg 2");
    const FontFamily* pFF = (ff.GetLastStatus() == Ok) ? &ff : &fallback;

    Font titleFont(pFF, 14.f, FontStyleBold,    UnitPixel);
    Font subFont  (pFF, 11.f, FontStyleRegular, UnitPixel);

    SolidBrush titleBrush(Color(255, 25,  31,  40));
    SolidBrush subBrush  (Color(255, 139, 149, 161));

    StringFormat fmt;
    fmt.SetAlignment(StringAlignmentCenter);

    RectF titleRect(10.f, textTop, DLG_W - 20.f, 22.f);
    g.DrawString(L"금융결제원", -1, &titleFont, titleRect, &fmt, &titleBrush);

    RectF subRect(10.f, textTop + 26.f, DLG_W - 20.f, 18.f);
    g.DrawString(L"결제 프로그램 업데이트중…", -1,
                 &subFont, subRect, &fmt, &subBrush);
}

void CLoadingDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == TIMER_ANIM)
    {
        m_fAngle += 6.f;
        if (m_fAngle >= 360.f) m_fAngle -= 360.f;
        Invalidate(FALSE);
        return;
    }
    CDialogEx::OnTimer(nIDEvent);
}
