/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#pragma once
#include <afxwin.h>

namespace HEXCTRL::INTERNAL::SCROLLEX
{
	class CScrollEx : public CWnd
	{
		enum class EState : std::uint8_t; //Forward declaration.
	public:
		void AddSibling(CScrollEx* pSibling);
		bool Create(CWnd* pParent, bool fVert, int iIDRESArrow, ULONGLONG ullScrolline, ULONGLONG ullScrollPage, ULONGLONG ullScrollSizeMax);
		[[nodiscard]] CWnd* GetParent()const;
		[[nodiscard]] ULONGLONG GetScrollPos()const;
		[[nodiscard]] LONGLONG GetScrollPosDelta()const;
		[[nodiscard]] ULONGLONG GetScrollLineSize()const;
		[[nodiscard]] ULONGLONG GetScrollPageSize()const;
		[[nodiscard]] bool IsThumbReleased()const;
		[[nodiscard]] bool IsVisible()const;

		/************************************************************************************
		* CALLBACK METHODS:                                                                 *
		* These methods must be called from the corresponding methods of the parent window. *
		************************************************************************************/
		void OnNcActivate(BOOL bActive)const;
		void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
		void OnNcPaint()const;
		void OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
		void OnMouseMove(UINT nFlags, CPoint point);
		void OnLButtonUp(UINT nFlags, CPoint point);
		/************************************************************************************
		* END OF THE CALLBACK METHODS.                                                      *
		************************************************************************************/

		void SetScrollSizes(ULONGLONG ullLine, ULONGLONG ullPage, ULONGLONG ullSizeMax);
		ULONGLONG SetScrollPos(ULONGLONG ullNewPos);
		void ScrollLineUp();
		void ScrollLineDown();
		void ScrollLineLeft();
		void ScrollLineRight();
		void ScrollPageUp();
		void ScrollPageDown();
		void ScrollPageLeft();
		void ScrollPageRight();
		void ScrollHome();
		void ScrollEnd();
		void SetScrollPageSize(ULONGLONG ullSize);
	private:
		DECLARE_MESSAGE_MAP()
			[[nodiscard]] bool CreateArrows(int iIDRESArrow, bool fVert);
		void DrawScrollBar()const;      //Draws the whole Scrollbar.
		void DrawArrows(CDC* pDC)const; //Draws arrows.
		void DrawThumb(CDC* pDC)const;  //Draws the Scroll thumb.
		[[nodiscard]] CRect GetScrollRect(bool fWithNCArea = false)const;          //Scroll's whole rect.
		[[nodiscard]] CRect GetScrollWorkAreaRect(bool fClientCoord = false)const; //Rect without arrows.
		[[nodiscard]] UINT GetScrollSizeWH()const;                                 //Scroll size in pixels, width or height.
		[[nodiscard]] UINT GetScrollWorkAreaSizeWH()const;                         //Scroll size (WH) without arrows.
		[[nodiscard]] CRect GetThumbRect(bool fClientCoord = false)const;
		[[nodiscard]] UINT GetThumbSizeWH()const;
		[[nodiscard]] int GetThumbPos()const;                                      //Current Thumb pos.
		void SetThumbPos(int iPos);
		[[nodiscard]] long double GetThumbScrollingSize()const;
		[[nodiscard]] CRect GetFirstArrowRect(bool fClientCoord = false)const;
		[[nodiscard]] CRect GetLastArrowRect(bool fClientCoord = false)const;
		[[nodiscard]] CRect GetFirstChannelRect(bool fClientCoord = false)const;
		[[nodiscard]] CRect GetLastChannelRect(bool fClientCoord = false)const;
		[[nodiscard]] CRect GetParentRect(bool fClient = true)const;
		[[nodiscard]] int GetTopDelta()const;       //Difference between parent window's Window and Client area. Very important in hit testing.
		[[nodiscard]] int GetLeftDelta()const;
		[[nodiscard]] bool IsVert()const;           //Is vertical or horizontal scrollbar.
		[[nodiscard]] bool IsThumbDragging()const;  //Is the thumb currently dragged by mouse.
		[[nodiscard]] bool IsSiblingVisible()const; //Is sibling scrollbar currently visible or not.
		void RedrawNC()const;
		void SendParentScrollMsg()const;            //Sends the WM_(V/H)SCROLL to the parent window.
		afx_msg void OnDestroy();
		afx_msg void OnTimer(UINT_PTR nIDEvent);
	private:
		CWnd* m_pParent { };                        //Parent window.
		CScrollEx* m_pSibling { };                  //Sibling scrollbar, added with AddSibling.
		CBitmap m_bmpArrowFirst;                    //Up or Left arrow bitmap.
		CBitmap m_bmpArrowLast;                     //Down or Right arrow bitmap.
		EState m_enState { };                       //Current state.
		CPoint m_ptCursorCur { };                   //Cursor's current position.
		UINT m_uiScrollBarSizeWH { };               //Scrollbar size (width if vertical, height if horz).
		ULONGLONG m_ullScrollPosCur { 0 };          //Current scroll position.
		ULONGLONG m_ullScrollPosPrev { };           //Previous scroll position.
		ULONGLONG m_ullScrollLine { };              //Size of one line scroll, when clicking arrow.
		ULONGLONG m_ullScrollPage { };              //Size of page scroll, when clicking channel.
		ULONGLONG m_ullScrollSizeMax { };           //Maximum scroll size (limit).
		const COLORREF m_clrBkNC { GetSysColor(COLOR_3DFACE) }; //Bk color of the non client area. 
		bool m_fCreated { false };                  //Main creation flag.
		bool m_fVisible { false };                  //Is visible at the moment or not.
		bool m_fScrollVert { };                     //Scrollbar type, horizontal or vertical.
	};
}