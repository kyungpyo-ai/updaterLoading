#include "pch.h"
#include "resource.h"
#include "LoadingDlg.h"
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

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
    , m_hAppIcon(nullptr)
{
    GdiplusStartupInput si;
    GdiplusStartup(&m_gdiplusToken, &si, nullptr);
}

CLoadingDlg::~CLoadingDlg()
{
    if (m_hAppIcon) { ::DestroyIcon(m_hAppIcon); m_hAppIcon = nullptr; }
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


    // DWM 드롭 쉐도우
    MARGINS margins = { 1, 1, 1, 1 };
    DwmExtendFrameIntoClientArea(GetSafeHwnd(), &margins);

    // 앱 아이콘 로드 (40×40)
    m_hAppIcon = (HICON)::LoadImage(AfxGetResourceHandle(),
        MAKEINTRESOURCE(IDI_APP_ICON), IMAGE_ICON, 40, 40, LR_DEFAULTCOLOR);

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
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);
    g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);

    DrawBackground(g);
    DrawLogo(g);
    DrawSeparator(g);
    DrawSpinner(g);
    DrawLabel(g);

    dc.BitBlt(0, 0, rc.Width(), rc.Height(), &memDC, 0, 0, SRCCOPY);
    memDC.SelectObject(pOld);
}

BOOL CLoadingDlg::OnEraseBkgnd(CDC*)
{
    return TRUE;
}

void CLoadingDlg::DrawBackground(Gdiplus::Graphics& g)
{
    Gdiplus::SolidBrush bg(Gdiplus::Color(255, 247, 248, 250));
    g.FillRectangle(&bg, 0, 0, DLG_W, DLG_H);

    // 직각 테두리 1px
    Gdiplus::Pen borderPen(Gdiplus::Color(255, 210, 214, 220), 1.f);
    g.DrawRectangle(&borderPen, 0.5f, 0.5f, (float)(DLG_W - 1), (float)(DLG_H - 1));
}

void CLoadingDlg::DrawLogo(Gdiplus::Graphics& g)
{
    Gdiplus::FontFamily ff(L"맑은 고딕");
    Gdiplus::FontFamily fallback(L"Arial");
    const Gdiplus::FontFamily* pFF = (ff.GetLastStatus() == Gdiplus::Ok) ? &ff : &fallback;

    // --- 앱 아이콘 (ICO 리소스) ---
    const int   iconSize = 40;
    const float iconX    = 22.f;
    const float iconY    = 26.f;

    if (m_hAppIcon)
    {
        HDC hdc = g.GetHDC();
        ::DrawIconEx(hdc, (int)iconX, (int)iconY, m_hAppIcon,
                     iconSize, iconSize, 0, nullptr, DI_NORMAL);
        g.ReleaseHDC(hdc);
    }

    // --- 브랜드명 ---
    const float textX = iconX + iconSize + 10.f;

    Gdiplus::Font brandFont(pFF, 15.f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush darkBrush(Gdiplus::Color(255, 25, 31, 40));
    Gdiplus::PointF brandPt(textX, 25.f);
    g.DrawString(L"KFTCOneCAP", -1, &brandFont, brandPt, &darkBrush);

    // --- "Plus" 뱃지 ---
    Gdiplus::RectF boundBox;
    Gdiplus::StringFormat sfLeft;
    sfLeft.SetAlignment(Gdiplus::StringAlignmentNear);
    g.MeasureString(L"KFTCOneCAP", -1, &brandFont, brandPt, &sfLeft, &boundBox);

    const float badgeX = boundBox.GetRight() + 6.f;
    const float badgeY = 28.f;
    const float badgeW = 36.f;
    const float badgeH = 16.f;

    // 양 끝 반원 pill 형태 — 소형 arc 4개보다 훨씬 부드럽게 렌더링
    Gdiplus::SolidBrush badgeBrush(Gdiplus::Color(255, 49, 130, 246));
    Gdiplus::GraphicsPath badgePath;
    badgePath.AddArc(badgeX,                        badgeY, badgeH, badgeH,  90, 180);
    badgePath.AddArc(badgeX + badgeW - badgeH,      badgeY, badgeH, badgeH, 270, 180);
    badgePath.CloseFigure();
    g.FillPath(&badgeBrush, &badgePath);

    Gdiplus::Font badgeFont(pFF, 9.f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush whiteBrush(Gdiplus::Color(255, 255, 255, 255));
    Gdiplus::StringFormat sfCenter;
    sfCenter.SetAlignment(Gdiplus::StringAlignmentCenter);
    sfCenter.SetLineAlignment(Gdiplus::StringAlignmentCenter);
    g.DrawString(L"Plus", -1, &badgeFont,
                 Gdiplus::RectF(badgeX, badgeY, badgeW, badgeH), &sfCenter, &whiteBrush);

    // --- 서브타이틀 ---
    Gdiplus::Font subFont(pFF, 13.f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush grayBrush(Gdiplus::Color(255, 139, 149, 161));
    g.DrawString(L"금융결제원 차세대 결제 솔루션", -1,
                 &subFont, Gdiplus::PointF(textX, 50.f), &grayBrush);
}

void CLoadingDlg::DrawSeparator(Gdiplus::Graphics& g)
{
    Gdiplus::Pen sepPen(Gdiplus::Color(255, 229, 231, 235), 1.f);
    g.DrawLine(&sepPen, 20.f, 88.f, (float)(DLG_W - 20), 88.f);
}

void CLoadingDlg::DrawSpinner(Gdiplus::Graphics& g)
{
    const float cx   = DLG_W / 2.f;
    const float cy   = 143.f;
    const float r    = 24.f;
    const float penW = 4.5f;
    const Gdiplus::RectF bounds(cx - r, cy - r, r * 2.f, r * 2.f);

    Gdiplus::Pen trackPen(Gdiplus::Color(255, 229, 231, 235), penW);
    g.DrawEllipse(&trackPen, bounds);

    Gdiplus::GraphicsPath arcPath;
    arcPath.AddArc(bounds, m_fAngle, 270.f);

    Gdiplus::Pen spinPen(Gdiplus::Color(255, 49, 130, 246), penW);
    spinPen.SetLineCap(Gdiplus::LineCapRound, Gdiplus::LineCapRound, Gdiplus::DashCapFlat);
    g.DrawPath(&spinPen, &arcPath);
}

void CLoadingDlg::DrawLabel(Gdiplus::Graphics& g)
{
    Gdiplus::FontFamily ff(L"맑은 고딕");
    Gdiplus::FontFamily fallback(L"Arial");
    const Gdiplus::FontFamily* pFF = (ff.GetLastStatus() == Gdiplus::Ok) ? &ff : &fallback;

    Gdiplus::Font labelFont(pFF, 13.f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush grayBrush(Gdiplus::Color(255, 80, 92, 107));

    Gdiplus::StringFormat fmt;
    fmt.SetAlignment(Gdiplus::StringAlignmentCenter);

    Gdiplus::RectF labelRect(10.f, 178.f, (float)(DLG_W - 20), 20.f);
    g.DrawString(L"최신 버전으로 업데이트 중...", -1, &labelFont, labelRect, &fmt, &grayBrush);
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
