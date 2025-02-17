/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "CScrollEx.h"
#include <cassert>
#include <cmath>
#include <memory>

using namespace HEXCTRL::INTERNAL::SCROLLEX;

namespace HEXCTRL::INTERNAL::SCROLLEX
{
	enum class CScrollEx::EState : std::uint8_t
	{
		STATE_DEFAULT,
		FIRSTARROW_HOVER, FIRSTARROW_CLICK,
		FIRSTCHANNEL_CLICK,
		THUMB_HOVER, THUMB_CLICK,
		LASTCHANNEL_CLICK,
		LASTARROW_CLICK, LASTARROW_HOVER
	};

	enum class ETimer : std::uint16_t
	{
		IDT_FIRSTCLICK = 0x7ff0,
		IDT_CLICKREPEAT = 0x7ff1
	};

	constexpr auto SIZE_PX_BMP_ARROW { 34 }; //Arrow rect size in pixels, in the loaded arrow bitmap.
	constexpr auto CLR_SCROLL_BK { RGB(241, 241, 241) };    //Scrollbar Bk color.
	constexpr auto CLR_SCROLL_THUMB { RGB(192, 192, 192) }; //Scrollbar thumb color.
	constexpr auto THUMB_POS_MAX { 0x7FFFFFFF };
}

BEGIN_MESSAGE_MAP(CScrollEx, CWnd)
	ON_WM_DESTROY()
	ON_WM_TIMER()
END_MESSAGE_MAP()

void CScrollEx::AddSibling(CScrollEx* pSibling)
{
	assert(m_fCreated);
	if (!m_fCreated)
		return;

	m_pSibling = pSibling;
}

bool CScrollEx::Create(CWnd* pParent, bool fVert, int iIDRESArrow,
	ULONGLONG ullScrolline, ULONGLONG ullScrollPage, ULONGLONG ullScrollSizeMax)
{
	assert(!m_fCreated); //Already created.
	assert(pParent != nullptr);
	if (m_fCreated || pParent == nullptr)
		return false;

	if (!CWnd::CreateEx(0, AfxRegisterWndClass(0), nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr))
		return false;

	m_fScrollVert = fVert;
	m_pParent = pParent;
	m_uiScrollBarSizeWH = GetSystemMetrics(fVert ? SM_CXVSCROLL : SM_CXHSCROLL);

	if (!CreateArrows(iIDRESArrow, fVert))
		return false;

	m_fCreated = true;
	SetScrollSizes(ullScrolline, ullScrollPage, ullScrollSizeMax);

	return true;
}

CWnd* CScrollEx::GetParent()const
{
	assert(m_fCreated);
	if (!m_fCreated)
		return nullptr;

	return m_pParent;
}

ULONGLONG CScrollEx::GetScrollPos()const
{
	assert(m_fCreated);
	if (!m_fCreated)
		return { };

	return m_ullScrollPosCur;
}

LONGLONG CScrollEx::GetScrollPosDelta()const
{
	assert(m_fCreated);
	if (!m_fCreated)
		return { };

	return static_cast<LONGLONG>(m_ullScrollPosCur - m_ullScrollPosPrev);
}

ULONGLONG CScrollEx::GetScrollLineSize()const
{
	assert(m_fCreated);
	if (!m_fCreated)
		return { };

	return m_ullScrollLine;
}

ULONGLONG CScrollEx::GetScrollPageSize()const
{
	assert(m_fCreated);
	if (!m_fCreated)
		return { };

	return m_ullScrollPage;
}

bool CScrollEx::IsThumbReleased()const
{
	assert(m_fCreated);
	if (!m_fCreated)
		return false;

	return m_enState != EState::THUMB_CLICK;
}

bool CScrollEx::IsVisible()const
{
	if (!m_fCreated)
		return false;

	return m_fVisible;
}

void CScrollEx::OnLButtonUp(UINT /*nFlags*/, CPoint /*point*/)
{
	assert(m_fCreated);
	if (!m_fCreated || m_enState == EState::STATE_DEFAULT)
		return;

	m_enState = EState::STATE_DEFAULT;
	SendParentScrollMsg(); //For parent to check IsThumbReleased.
	KillTimer(static_cast<UINT_PTR>(ETimer::IDT_FIRSTCLICK));
	KillTimer(static_cast<UINT_PTR>(ETimer::IDT_CLICKREPEAT));
	ReleaseCapture();
	DrawScrollBar();
}

void CScrollEx::OnMouseMove(UINT /*nFlags*/, CPoint point)
{
	assert(m_fCreated);
	if (!m_fCreated || !IsThumbDragging())
		return;

	const auto rc = GetScrollWorkAreaRect(true);
	const auto iCurrPos = GetThumbPos();
	int iNewPos;

	if (IsVert()) {
		if (point.y < rc.top) {
			iNewPos = 0;
			m_ptCursorCur.y = rc.top;
		}
		else if (point.y > rc.bottom) {
			iNewPos = THUMB_POS_MAX;
			m_ptCursorCur.y = rc.bottom;
		}
		else {
			iNewPos = iCurrPos + (point.y - m_ptCursorCur.y);
			m_ptCursorCur.y = point.y;
		}
	}
	else {
		if (point.x < rc.left) {
			iNewPos = 0;
			m_ptCursorCur.x = rc.left;
		}
		else if (point.x > rc.right) {
			iNewPos = THUMB_POS_MAX;
			m_ptCursorCur.x = rc.right;
		}
		else {
			iNewPos = iCurrPos + (point.x - m_ptCursorCur.x);
			m_ptCursorCur.x = point.x;
		}
	}

	if (iNewPos != iCurrPos)  //Set new thumb pos only if it has been changed.
		SetThumbPos(iNewPos);
}

void CScrollEx::OnNcActivate(BOOL /*bActive*/)const
{
	if (!m_fCreated)
		return;

	RedrawNC(); //To repaint NC area.
}

void CScrollEx::OnNcCalcSize(BOOL /*bCalcValidRects*/, NCCALCSIZE_PARAMS* lpncsp)
{
	if (!m_fCreated)
		return;

	const CRect rc = lpncsp->rgrc[0];
	const auto ullCurPos = GetScrollPos();
	if (IsVert()) {
		const UINT uiHeight { IsSiblingVisible() ? rc.Height() - m_uiScrollBarSizeWH : rc.Height() };
		if (uiHeight < m_ullScrollSizeMax) {
			m_fVisible = true;
			if (ullCurPos + uiHeight > m_ullScrollSizeMax)
				SetScrollPos(m_ullScrollSizeMax - uiHeight);
			else
				DrawScrollBar();
			lpncsp->rgrc[0].right -= m_uiScrollBarSizeWH;
		}
		else {
			SetScrollPos(0);
			m_fVisible = false;
		}
	}
	else {
		const UINT uiWidth { IsSiblingVisible() ? rc.Width() - m_uiScrollBarSizeWH : rc.Width() };
		if (uiWidth < m_ullScrollSizeMax) {
			m_fVisible = true;
			if (ullCurPos + uiWidth > m_ullScrollSizeMax)
				SetScrollPos(m_ullScrollSizeMax - uiWidth);
			else
				DrawScrollBar();
			lpncsp->rgrc[0].bottom -= m_uiScrollBarSizeWH;
		}
		else {
			SetScrollPos(0);
			m_fVisible = false;
		}
	}
}

void CScrollEx::OnNcPaint()const
{
	if (!m_fCreated)
		return;

	DrawScrollBar();
}

void CScrollEx::OnSetCursor(CWnd* /*pWnd*/, UINT nHitTest, UINT message)
{
	assert(m_fCreated);
	if (!m_fCreated)
		return;

	if (nHitTest == HTTOPLEFT || nHitTest == HTLEFT || nHitTest == HTBOTTOMLEFT
		|| nHitTest == HTTOPRIGHT || nHitTest == HTRIGHT || nHitTest == HTBOTTOMRIGHT
		|| nHitTest == HTBOTTOM || nHitTest == HTSIZE || !IsVisible())
		return;

	switch (message) {
	case WM_LBUTTONDOWN:
	{
		const auto pParent = GetParent();
		if (pParent == nullptr)
			return;

		POINT pt;
		GetCursorPos(&pt);
		pParent->ScreenToClient(&pt);
		pParent->SetFocus();
		constexpr auto uTimerFirstClick { 200U }; //Milliseconds for WM_TIMER for first channel click.

		if (GetThumbRect(true).PtInRect(pt)) {
			m_ptCursorCur = pt;
			m_enState = EState::THUMB_CLICK;
			pParent->SetCapture();
		}
		else if (GetFirstArrowRect(true).PtInRect(pt)) {
			ScrollLineUp();
			m_enState = EState::FIRSTARROW_CLICK;
			pParent->SetCapture();
			SetTimer(static_cast<UINT_PTR>(ETimer::IDT_FIRSTCLICK), uTimerFirstClick, nullptr);
		}
		else if (GetLastArrowRect(true).PtInRect(pt)) {
			ScrollLineDown();
			m_enState = EState::LASTARROW_CLICK;
			pParent->SetCapture();
			SetTimer(static_cast<UINT_PTR>(ETimer::IDT_FIRSTCLICK), uTimerFirstClick, nullptr);
		}
		else if (GetFirstChannelRect(true).PtInRect(pt)) {
			ScrollPageUp();
			m_enState = EState::FIRSTCHANNEL_CLICK;
			pParent->SetCapture();
			SetTimer(static_cast<UINT_PTR>(ETimer::IDT_FIRSTCLICK), uTimerFirstClick, nullptr);
		}
		else if (GetLastChannelRect(true).PtInRect(pt)) {
			ScrollPageDown();
			m_enState = EState::LASTCHANNEL_CLICK;
			pParent->SetCapture();
			SetTimer(static_cast<UINT_PTR>(ETimer::IDT_FIRSTCLICK), uTimerFirstClick, nullptr);
		}
	}
	break;
	default:
		break;
	}
}

void CScrollEx::SetScrollSizes(ULONGLONG ullLine, ULONGLONG ullPage, ULONGLONG ullSizeMax)
{
	assert(m_fCreated);
	if (!m_fCreated)
		return;

	m_ullScrollLine = ullLine;
	m_ullScrollPage = ullPage;
	m_ullScrollSizeMax = ullSizeMax;

	RedrawNC(); //To repaint NC area.
}

ULONGLONG CScrollEx::SetScrollPos(ULONGLONG ullNewPos)
{
	assert(m_fCreated);
	if (!m_fCreated)
		return { };

	m_ullScrollPosPrev = m_ullScrollPosCur;
	if (m_ullScrollPosCur == ullNewPos)
		return m_ullScrollPosPrev;

	m_ullScrollPosCur = ullNewPos;

	const auto rc = GetParentRect();
	const auto iScreenSize { IsVert() ? rc.Height() : rc.Width() };
	const ULONGLONG ullMax { iScreenSize > m_ullScrollSizeMax ? 0 : m_ullScrollSizeMax - iScreenSize };

	if (m_ullScrollPosCur > ullMax)
		m_ullScrollPosCur = ullMax;

	SendParentScrollMsg();
	DrawScrollBar();

	return m_ullScrollPosPrev;
}

void CScrollEx::ScrollLineDown()
{
	assert(m_fCreated);
	if (!m_fCreated)
		return;

	const auto ullCur = GetScrollPos();
	const ULONGLONG ullNew { ULONGLONG_MAX - ullCur < m_ullScrollLine ? ULONGLONG_MAX : ullCur + m_ullScrollLine };
	SetScrollPos(ullNew);
}

void CScrollEx::ScrollLineRight()
{
	assert(m_fCreated);
	if (!m_fCreated)
		return;

	ScrollLineDown();
}

void CScrollEx::ScrollLineUp()
{
	assert(m_fCreated);
	if (!m_fCreated)
		return;

	const auto ullCur = GetScrollPos();
	SetScrollPos(m_ullScrollLine > ullCur ? 0 : ullCur - m_ullScrollLine);
}

void CScrollEx::ScrollLineLeft()
{
	assert(m_fCreated);
	if (!m_fCreated)
		return;

	ScrollLineUp();
}

void CScrollEx::ScrollPageDown()
{
	assert(m_fCreated);
	if (!m_fCreated)
		return;

	const auto ullCur = GetScrollPos();
	SetScrollPos(ULONGLONG_MAX - ullCur < m_ullScrollPage ? ULONGLONG_MAX : ullCur + m_ullScrollPage);
}

void CScrollEx::ScrollPageLeft()
{
	assert(m_fCreated);
	if (!m_fCreated)
		return;

	ScrollPageUp();
}

void CScrollEx::ScrollPageRight()
{
	assert(m_fCreated);
	if (!m_fCreated)
		return;

	ScrollPageDown();
}

void CScrollEx::ScrollPageUp()
{
	assert(m_fCreated);
	if (!m_fCreated)
		return;

	const auto ullCur = GetScrollPos();
	SetScrollPos(m_ullScrollPage > ullCur ? 0 : ullCur - m_ullScrollPage);
}

void CScrollEx::ScrollHome()
{
	assert(m_fCreated);
	if (!m_fCreated)
		return;

	SetScrollPos(0);
}

void CScrollEx::ScrollEnd()
{
	assert(m_fCreated);
	if (!m_fCreated)
		return;

	SetScrollPos(m_ullScrollSizeMax);
}

void CScrollEx::SetScrollPageSize(ULONGLONG ullSize)
{
	assert(m_fCreated);
	if (!m_fCreated)
		return;

	m_ullScrollPage = ullSize;
}


/********************************************************************
* Private methods.
********************************************************************/

bool CScrollEx::CreateArrows(int iIDRESArrow, bool fVert)
{
	CBitmap bmpArrow; //Bitmap of the arrow.
	if (!bmpArrow.LoadBitmapW(iIDRESArrow))
		return false;

	BITMAP hBitmap;
	bmpArrow.GetBitmap(&hBitmap); //hBitmap.bmBits is nullptr here.
	const auto nWidth = static_cast<std::size_t>(hBitmap.bmWidth);
	const auto nHeight = static_cast<std::size_t>(hBitmap.bmHeight);
	const auto dwBytesBmp = hBitmap.bmWidthBytes * hBitmap.bmHeight;
	const auto dwPixels = nWidth * nHeight;
	const auto pOrigCOLOR = std::make_unique<COLORREF[]>(dwPixels);
	bmpArrow.GetBitmapBits(dwBytesBmp, pOrigCOLOR.get());

	const auto lmbTranspose = [](COLORREF* pInOut, std::size_t nWidth, std::size_t nHeight) {
		for (std::size_t itHeight = 0; itHeight < nHeight; ++itHeight) //Transpose matrix.
			for (std::size_t j = itHeight; j < nWidth; ++j)
				std::swap(pInOut[itHeight * nHeight + j], pInOut[j * nHeight + itHeight]);
	};
	const auto lmbFlipVert = [](COLORREF* pInOut, std::size_t nWidth, std::size_t nHeight) {
		for (std::size_t itWidth = 0; itWidth < nWidth; ++itWidth) //Flip matrix' columns.
			for (std::size_t itHeight = 0, itHeightBack = nHeight - 1; itHeight < itHeightBack; ++itHeight, --itHeightBack)
				std::swap(pInOut[itHeight * nHeight + itWidth], pInOut[itHeightBack * nWidth + itWidth]);
	};
	const auto lmbFlipHorz = [](COLORREF* pInOut, std::size_t nWidth, std::size_t nHeight) {
		for (std::size_t itHeight = 0; itHeight < nHeight; ++itHeight) //Flip matrix' rows.
			for (std::size_t itWidth = 0, itWidthBack = nWidth - 1; itWidth < itWidthBack; ++itWidth, --itWidthBack)
				std::swap(pInOut[itHeight * nWidth + itWidth], pInOut[itHeight * nWidth + itWidthBack]);
	};
	const auto lmb90CW = [&](COLORREF* pInOut, std::size_t nWidth, std::size_t nHeight) {
		lmbFlipVert(pInOut, nWidth, nHeight);
		lmbTranspose(pInOut, nWidth, nHeight);
	};
	const auto lmb90CCW = [&](COLORREF* pInOut, std::size_t nWidth, std::size_t nHeight) {
		lmbTranspose(pInOut, nWidth, nHeight);
		lmbFlipVert(pInOut, nWidth, nHeight);
	};

	m_bmpArrowFirst.CreateBitmapIndirect(&hBitmap);
	m_bmpArrowLast.CreateBitmapIndirect(&hBitmap);
	if (fVert) {
		m_bmpArrowFirst.SetBitmapBits(dwBytesBmp, pOrigCOLOR.get()); //Up arrow.
		lmbFlipVert(pOrigCOLOR.get(), nWidth, nHeight);              //Down arrow.
	}
	else {
		lmb90CCW(pOrigCOLOR.get(), nWidth, nHeight);
		m_bmpArrowFirst.SetBitmapBits(dwBytesBmp, pOrigCOLOR.get()); //Left arrow.
		lmbFlipHorz(pOrigCOLOR.get(), nWidth, nHeight);              //Right arrow.
	}
	m_bmpArrowLast.SetBitmapBits(dwBytesBmp, pOrigCOLOR.get());

	return true;
}

void CScrollEx::DrawScrollBar()const
{
	if (!IsVisible())
		return;

	const auto pParent = GetParent();
	CWindowDC dcParent(pParent);
	CDC dcMem;
	dcMem.CreateCompatibleDC(&dcParent);
	CBitmap bitmap;
	const auto rcWnd = GetParentRect(false);
	bitmap.CreateCompatibleBitmap(&dcParent, rcWnd.Width(), rcWnd.Height());
	dcMem.SelectObject(&bitmap);
	const auto pDC = &dcMem;

	const auto rcSNC = GetScrollRect(true);	//Scroll bar with any additional non client area, to fill it below.
	pDC->FillSolidRect(&rcSNC, m_clrBkNC);	//Scroll bar with NC Bk.
	const auto rcS = GetScrollRect();
	pDC->FillSolidRect(&rcS, CLR_SCROLL_BK); //Scroll bar Bk.
	DrawArrows(pDC);
	DrawThumb(pDC);

	//Copy drawn Scrollbar from dcMem to parent window.
	dcParent.BitBlt(rcSNC.left, rcSNC.top, rcSNC.Width(), rcSNC.Height(), &dcMem, rcSNC.left, rcSNC.top, SRCCOPY);
}

void CScrollEx::DrawArrows(CDC* pDC)const
{
	const auto rcScroll = GetScrollRect();
	int iFirstBtnWH;
	int iLastBtnWH;
	int iLastBtnOffsetDrawX;
	int iLastBtnOffsetDrawY;
	const auto iFirstBtnOffsetDrawX = rcScroll.left;
	const auto iFirstBtnOffsetDrawY = rcScroll.top;

	if (IsVert()) {
		iFirstBtnWH = iLastBtnWH = rcScroll.Width();
		iLastBtnOffsetDrawX = rcScroll.left;
		iLastBtnOffsetDrawY = rcScroll.bottom - rcScroll.Width();
	}
	else {
		iFirstBtnWH = iLastBtnWH = rcScroll.Height();
		iLastBtnOffsetDrawX = rcScroll.right - rcScroll.Height();
		iLastBtnOffsetDrawY = rcScroll.top;
	}

	pDC->SetStretchBltMode(HALFTONE); //To stretch bmp at max quality, without artifacts.

	//https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-setstretchbltmode
	//After setting the HALFTONE stretching mode, an application must call the Win32 function
	//SetBrushOrgEx to set the brush origin. If it fails to do so, brush misalignment occurs.
	SetBrushOrgEx(pDC->m_hDC, 0, 0, nullptr);

	CDC dcSource;
	dcSource.CreateCompatibleDC(pDC);

	dcSource.SelectObject(m_bmpArrowFirst);	//First arrow button.
	pDC->StretchBlt(iFirstBtnOffsetDrawX, iFirstBtnOffsetDrawY, iFirstBtnWH, iFirstBtnWH,
		&dcSource, 0, 0, SIZE_PX_BMP_ARROW, SIZE_PX_BMP_ARROW, SRCCOPY);

	dcSource.SelectObject(m_bmpArrowLast); //Last arrow button.
	pDC->StretchBlt(iLastBtnOffsetDrawX, iLastBtnOffsetDrawY, iLastBtnWH, iLastBtnWH,
		&dcSource, 0, 0, SIZE_PX_BMP_ARROW, SIZE_PX_BMP_ARROW, SRCCOPY);
}

void CScrollEx::DrawThumb(CDC* pDC)const
{
	auto rcThumb = GetThumbRect();
	if (!rcThumb.IsRectNull())
		pDC->FillSolidRect(rcThumb, CLR_SCROLL_THUMB);
}

CRect CScrollEx::GetScrollRect(bool fWithNCArea)const
{
	if (!m_fCreated)
		return { };

	const auto* const pParent = GetParent();
	auto rcClient = GetParentRect();
	pParent->MapWindowPoints(nullptr, &rcClient);
	const auto rcWnd = GetParentRect(false);
	const auto iTopDelta = GetTopDelta();
	const auto iLeftDelta = GetLeftDelta();

	CRect rcScroll;
	if (IsVert()) {
		rcScroll.left = rcClient.right + iLeftDelta;
		rcScroll.top = rcClient.top + iTopDelta;
		rcScroll.right = rcScroll.left + m_uiScrollBarSizeWH;
		if (fWithNCArea) //Adding difference here to gain equality in coords when call to pParent->ScreenToClient below.
			rcScroll.bottom = rcWnd.bottom + iTopDelta;
		else
			rcScroll.bottom = rcScroll.top + rcClient.Height();
	}
	else {
		rcScroll.left = rcClient.left + iLeftDelta;
		rcScroll.top = rcClient.bottom + iTopDelta;
		rcScroll.bottom = rcScroll.top + m_uiScrollBarSizeWH;
		if (fWithNCArea)
			rcScroll.right = rcWnd.right + iLeftDelta;
		else
			rcScroll.right = rcScroll.left + rcClient.Width();
	}
	pParent->ScreenToClient(&rcScroll);

	return rcScroll;
}

CRect CScrollEx::GetScrollWorkAreaRect(bool fClientCoord)const
{
	auto rc = GetScrollRect();
	if (IsVert())
		rc.DeflateRect(0, m_uiScrollBarSizeWH, 0, m_uiScrollBarSizeWH);
	else
		rc.DeflateRect(m_uiScrollBarSizeWH, 0, m_uiScrollBarSizeWH, 0);

	if (fClientCoord)
		rc.OffsetRect(-GetLeftDelta(), -GetTopDelta());

	return rc;
}

UINT CScrollEx::GetScrollSizeWH()const
{
	return IsVert() ? GetScrollRect().Height() : GetScrollRect().Width();
}

UINT CScrollEx::GetScrollWorkAreaSizeWH()const
{
	const auto uiScrollSize = GetScrollSizeWH();

	return uiScrollSize <= m_uiScrollBarSizeWH * 2 ? 0 : uiScrollSize - m_uiScrollBarSizeWH * 2; //Minus two arrow's size.
}

CRect CScrollEx::GetThumbRect(bool fClientCoord)const
{
	CRect rc { };
	const auto uiThumbSize = GetThumbSizeWH();
	if (!uiThumbSize)
		return rc;

	const auto rcScrollWA = GetScrollWorkAreaRect();
	if (IsVert()) {
		rc.left = rcScrollWA.left;
		rc.top = rcScrollWA.top + GetThumbPos();
		rc.right = rc.left + m_uiScrollBarSizeWH;
		rc.bottom = rc.top + uiThumbSize;
		if (rc.bottom > rcScrollWA.bottom)
			rc.bottom = rcScrollWA.bottom;
	}
	else {
		rc.left = rcScrollWA.left + GetThumbPos();
		rc.top = rcScrollWA.top;
		rc.right = rc.left + uiThumbSize;
		rc.bottom = rc.top + m_uiScrollBarSizeWH;
	}
	if (fClientCoord)
		rc.OffsetRect(-GetLeftDelta(), -GetTopDelta());

	return rc;
}

UINT CScrollEx::GetThumbSizeWH()const
{
	constexpr auto uThumbSizeMin = 15U; //Minimum allowed thumb size.

	const auto uiScrollWorkAreaSizeWH = GetScrollWorkAreaSizeWH();
	const auto rcParent = GetParentRect();
	const long double dDelta { IsVert() ?
		static_cast<long double>(rcParent.Height()) / m_ullScrollSizeMax :
		static_cast<long double>(rcParent.Width()) / m_ullScrollSizeMax };

	const UINT uiThumbSize { static_cast<UINT>(std::lroundl(uiScrollWorkAreaSizeWH * dDelta)) };

	return uiThumbSize < uThumbSizeMin ? uThumbSizeMin : uiThumbSize;
}

int CScrollEx::GetThumbPos()const
{
	const auto ullScrollPos = GetScrollPos();
	const auto dThumbScrollingSize = GetThumbScrollingSize();

	return ullScrollPos < dThumbScrollingSize ? 0 : std::lroundl(ullScrollPos / dThumbScrollingSize);
}

void CScrollEx::SetThumbPos(int iPos)
{
	if (iPos == GetThumbPos())
		return;

	const auto rcWorkArea = GetScrollWorkAreaRect();
	const auto uiThumbSize = GetThumbSizeWH();
	ULONGLONG ullNewScrollPos;

	if (iPos < 0)
		ullNewScrollPos = 0;
	else if (iPos == THUMB_POS_MAX)
		ullNewScrollPos = m_ullScrollSizeMax;
	else {
		if (IsVert()) {
			if (iPos + static_cast<int>(uiThumbSize) > rcWorkArea.Height())
				iPos = rcWorkArea.Height() - uiThumbSize;
		}
		else {
			if (iPos + static_cast<int>(uiThumbSize) > rcWorkArea.Width())
				iPos = rcWorkArea.Width() - uiThumbSize;
		}
		ullNewScrollPos = static_cast<ULONGLONG>(std::llroundl(iPos * GetThumbScrollingSize()));
	}

	SetScrollPos(ullNewScrollPos);
}

long double CScrollEx::GetThumbScrollingSize()const
{
	if (!m_fCreated)
		return 0;

	const auto uiWAWOThumb = GetScrollWorkAreaSizeWH() - GetThumbSizeWH(); //Work area without thumb.
	const auto iPage { IsVert() ? GetParentRect().Height() : GetParentRect().Width() };

	return (m_ullScrollSizeMax - iPage) / static_cast<long double>(uiWAWOThumb);
}

CRect CScrollEx::GetFirstArrowRect(bool fClientCoord)const
{
	auto rc = GetScrollRect();
	if (IsVert())
		rc.bottom = rc.top + m_uiScrollBarSizeWH;
	else
		rc.right = rc.left + m_uiScrollBarSizeWH;

	if (fClientCoord)
		rc.OffsetRect(-GetLeftDelta(), -GetTopDelta());

	return rc;
}

CRect CScrollEx::GetLastArrowRect(bool fClientCoord)const
{
	CRect rc = GetScrollRect();
	if (IsVert())
		rc.top = rc.bottom - m_uiScrollBarSizeWH;
	else
		rc.left = rc.right - m_uiScrollBarSizeWH;

	if (fClientCoord)
		rc.OffsetRect(-GetLeftDelta(), -GetTopDelta());

	return rc;
}

CRect CScrollEx::GetFirstChannelRect(bool fClientCoord)const
{
	const auto rcThumb = GetThumbRect();
	const auto rcArrow = GetFirstArrowRect();
	CRect rc;
	if (IsVert())
		rc.SetRect(rcArrow.left, rcArrow.bottom, rcArrow.right, rcThumb.top);
	else
		rc.SetRect(rcArrow.right, rcArrow.top, rcThumb.left, rcArrow.bottom);

	if (fClientCoord)
		rc.OffsetRect(-GetLeftDelta(), -GetTopDelta());

	return rc;
}

CRect CScrollEx::GetLastChannelRect(bool fClientCoord)const
{
	const auto rcThumb = GetThumbRect();
	const auto rcArrow = GetLastArrowRect();
	CRect rc;
	if (IsVert())
		rc.SetRect(rcArrow.left, rcThumb.bottom, rcArrow.right, rcArrow.top);
	else
		rc.SetRect(rcThumb.left, rcArrow.top, rcArrow.left, rcArrow.bottom);

	if (fClientCoord)
		rc.OffsetRect(-GetLeftDelta(), -GetTopDelta());

	return rc;
}

CRect CScrollEx::GetParentRect(bool fClient)const
{
	CRect rc;
	if (fClient)
		GetParent()->GetClientRect(&rc);
	else
		GetParent()->GetWindowRect(&rc);

	return rc;
}

int CScrollEx::GetTopDelta()const
{
	CRect rcClient = GetParentRect();
	GetParent()->MapWindowPoints(nullptr, &rcClient);

	return rcClient.top - GetParentRect(false).top;
}

int CScrollEx::GetLeftDelta()const
{
	CRect rcClient = GetParentRect();
	GetParent()->MapWindowPoints(nullptr, &rcClient);

	return rcClient.left - GetParentRect(false).left;
}

bool CScrollEx::IsVert()const
{
	return m_fScrollVert;
}

bool CScrollEx::IsThumbDragging()const
{
	return m_enState == EState::THUMB_CLICK;
}

bool CScrollEx::IsSiblingVisible()const
{
	return m_pSibling ? m_pSibling->IsVisible() : false;
}

void CScrollEx::RedrawNC()const
{
	//To repaint NC area.
	if (auto* pWnd = GetParent(); pWnd != nullptr) {
		pWnd->SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	}
}

void CScrollEx::SendParentScrollMsg()const
{
	if (!m_fCreated)
		return;

	GetParent()->SendMessageW(IsVert() ? WM_VSCROLL : WM_HSCROLL);
}

void CScrollEx::OnTimer(UINT_PTR nIDEvent)
{
	constexpr auto uTimerRepeat { 50U }; //Milliseconds for repeat when click and hold on channel.

	switch (nIDEvent) {
	case (static_cast<UINT_PTR>(ETimer::IDT_FIRSTCLICK)):
		KillTimer(static_cast<UINT_PTR>(ETimer::IDT_FIRSTCLICK));
		SetTimer(static_cast<UINT_PTR>(ETimer::IDT_CLICKREPEAT), uTimerRepeat, nullptr);
		break;
	case (static_cast<UINT_PTR>(ETimer::IDT_CLICKREPEAT)):
		switch (m_enState) {
		case EState::FIRSTARROW_CLICK:
			ScrollLineUp();
			break;
		case EState::LASTARROW_CLICK:
			ScrollLineDown();
			break;
		case EState::FIRSTCHANNEL_CLICK:
		{
			CPoint pt;
			GetCursorPos(&pt);
			CRect rc = GetThumbRect(true);
			GetParent()->ClientToScreen(rc);
			if (IsVert()) {
				if (pt.y < rc.top)
					ScrollPageUp();
			}
			else {
				if (pt.x < rc.left)
					ScrollPageUp();
			}
		}
		break;
		case EState::LASTCHANNEL_CLICK:
		{
			CPoint pt;
			GetCursorPos(&pt);
			CRect rc = GetThumbRect(true);
			GetParent()->ClientToScreen(rc);
			if (IsVert()) {
				if (pt.y > rc.bottom)
					ScrollPageDown();
			}
			else {
				if (pt.x > rc.right)
					ScrollPageDown();
			}
		}
		break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	CWnd::OnTimer(nIDEvent);
}

void CScrollEx::OnDestroy()
{
	m_bmpArrowFirst.DeleteObject();
	m_bmpArrowLast.DeleteObject();
	m_pParent = nullptr;
	m_fCreated = false;

	CWnd::OnDestroy();
}