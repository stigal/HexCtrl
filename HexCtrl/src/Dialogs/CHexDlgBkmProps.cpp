/****************************************************************************************
* Copyright © 2018-2023 Jovibor https://github.com/jovibor/                             *
* This is a Hex Control for MFC/Win32 applications.                                     *
* Official git repository: https://github.com/jovibor/HexCtrl/                          *
* This software is available under "The HexCtrl License", see the LICENSE file.         *
****************************************************************************************/
#include "stdafx.h"
#include "../HexUtility.h"
#include "CHexDlgBkmProps.h"
#include <cassert>
#include <format>
#include <numeric>

using namespace HEXCTRL;
using namespace HEXCTRL::INTERNAL;

BEGIN_MESSAGE_MAP(CHexDlgBkmProps, CDialogEx)
END_MESSAGE_MAP()

INT_PTR CHexDlgBkmProps::DoModal(HEXBKM& hbs, bool fShowAsHex)
{
	m_pHBS = &hbs;
	m_fShowAsHex = fShowAsHex;

	return CDialogEx::DoModal();
}

void CHexDlgBkmProps::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BOOL CHexDlgBkmProps::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	if (auto pClrBtn = static_cast<CMFCColorButton*>(GetDlgItem(IDC_HEXCTRL_BKMPROPS_COLOR_BK)); pClrBtn != nullptr)
		pClrBtn->SetColor(m_pHBS->clrBk);
	if (auto pClrBtn = static_cast<CMFCColorButton*>(GetDlgItem(IDC_HEXCTRL_BKMPROPS_COLOR_TEXT)); pClrBtn != nullptr)
		pClrBtn->SetColor(m_pHBS->clrText);

	if (!m_pHBS->vecSpan.empty()) {
		m_ullOffset = m_pHBS->vecSpan.front().ullOffset;
		m_ullSize = std::accumulate(m_pHBS->vecSpan.begin(), m_pHBS->vecSpan.end(), ULONGLONG { },
			[](auto ullTotal, const HEXSPAN& ref) { return ullTotal + ref.ullSize; });
	}
	else {
		m_ullOffset = 0;
		m_ullSize = 1;
		m_pHBS->vecSpan.emplace_back(m_ullOffset, m_ullSize);
	}

	auto pEdit = static_cast<CEdit*>(GetDlgItem(IDC_HEXCTRL_BKMPROPS_EDIT_OFFSET));
	pEdit->SetWindowTextW(std::vformat(m_fShowAsHex ? L"0x{:X}" : L"{}", std::make_wformat_args(m_ullOffset)).data());

	pEdit = static_cast<CEdit*>(GetDlgItem(IDC_HEXCTRL_BKMPROPS_EDIT_LENGTH));
	pEdit->SetWindowTextW(std::vformat(m_fShowAsHex ? L"0x{:X}" : L"{}", std::make_wformat_args(m_ullSize)).data());

	pEdit = static_cast<CEdit*>(GetDlgItem(IDC_HEXCTRL_BKMPROPS_EDIT_DESCR));
	pEdit->SetWindowTextW(m_pHBS->wstrDesc.data());

	return TRUE;
}

void CHexDlgBkmProps::OnOK()
{
	wchar_t pwszBuff[512];
	constexpr auto NUMBER_MAX_CHARS = 32; //Text limit 32 chars.

	auto pClrBtn = static_cast<CMFCColorButton*>(GetDlgItem(IDC_HEXCTRL_BKMPROPS_COLOR_BK));
	m_pHBS->clrBk = pClrBtn->GetColor();
	pClrBtn = static_cast<CMFCColorButton*>(GetDlgItem(IDC_HEXCTRL_BKMPROPS_COLOR_TEXT));
	m_pHBS->clrText = pClrBtn->GetColor();

	auto pEdit = static_cast<CEdit*>(GetDlgItem(IDC_HEXCTRL_BKMPROPS_EDIT_OFFSET));
	pEdit->GetWindowTextW(pwszBuff, NUMBER_MAX_CHARS);
	const auto optOffset = StrToULL(pwszBuff);
	if (!optOffset) {
		MessageBoxW(L"Invalid offset format", L"Format Error", MB_ICONERROR);
		return;
	}

	pEdit = static_cast<CEdit*>(GetDlgItem(IDC_HEXCTRL_BKMPROPS_EDIT_LENGTH));
	pEdit->GetWindowTextW(pwszBuff, NUMBER_MAX_CHARS);
	const auto optSize = StrToULL(pwszBuff);
	if (!optSize) {
		MessageBoxW(L"Invalid length format.", L"Format Error", MB_ICONERROR);
		return;
	}
	if (*optSize == 0) {
		MessageBoxW(L"Length can not be zero!", L"Format Error", MB_ICONERROR);
		return;
	}

	if (m_ullOffset != *optOffset || m_ullSize != *optSize) {
		m_pHBS->vecSpan.clear();
		m_pHBS->vecSpan.emplace_back(*optOffset, *optSize);
	}

	pEdit = static_cast<CEdit*>(GetDlgItem(IDC_HEXCTRL_BKMPROPS_EDIT_DESCR));
	pEdit->GetWindowTextW(pwszBuff, static_cast<int>(std::size(pwszBuff)));
	m_pHBS->wstrDesc = pwszBuff;

	CDialogEx::OnOK();
}