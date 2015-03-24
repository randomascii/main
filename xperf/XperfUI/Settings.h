#pragma once

#include <string>

// CSettings dialog

class CSettings : public CDialogEx
{
	DECLARE_DYNAMIC(CSettings)

public:
	CSettings(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSettings();

// Dialog Data
	enum { IDD = IDD_SETTINGS };

	std::wstring heapTracingExe_;

protected:
	CEdit btHeapTracingExe_;
	CEdit btExtraProviders_;
	CEdit btExtraStackwalks_;
	CComboBox btBufferSizes_;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnOK();
};
