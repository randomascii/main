#pragma once

#include <string>

// CSettings dialog

class CSettings : public CDialogEx
{
	DECLARE_DYNAMIC(CSettings)

public:
	CSettings(CWnd* pParent, const std::wstring& exeDir, const std::wstring& wptDir);   // standard constructor
	virtual ~CSettings();

// Dialog Data
	enum { IDD = IDD_SETTINGS };

	std::wstring heapTracingExe_;

protected:
	CEdit btHeapTracingExe_;
	CEdit btExtraProviders_;
	CEdit btExtraStackwalks_;
	CComboBox btBufferSizes_;

	CButton btCopyStartupProfile_;
	CButton btCopySymbolDLLs_;

	CToolTipCtrl toolTip_;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	const std::wstring exeDir_;
	const std::wstring wptDir_;

	DECLARE_MESSAGE_MAP()
	afx_msg void OnOK();
public:
	afx_msg void OnBnClickedCopystartupprofile();
	afx_msg void OnBnClickedCopysymboldlls();
	afx_msg BOOL PreTranslateMessage(MSG* pMsg);
};
