
// XperfUIDlg.h : header file
//

#pragma once

#include <string>

// CXperfUIDlg dialog
class CXperfUIDlg : public CDialogEx
{
// Construction
public:
	CXperfUIDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_XPERFUI_DIALOG };

	void vprintf(const char* pFormat, va_list marker);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// Implementation
protected:
	HICON m_hIcon;

	bool bCompress_ = true;
	bool bSampledStacks_ = true;
	bool bCswitchStacks_ = true;
	bool bIsTracing_ = false;
	bool bShowCommands_ = false;

	CButton btStartTracing_;
	CButton btSaveTraceBuffers_;
	CButton btStopTracing_;
	CButton btCompress_;
	CButton btSampledStacks_;
	CButton btCswitchStacks_;
	CButton btShowCommands_;

	std::string traceDir_;
	std::string tempTraceDir_;

	std::string output_;

	std::string GetWPTDir();
	std::string GetXperfPath();
	std::string GetTraceDir();
	std::string GetResultFile();
	std::string GetTempTraceDir();
	std::string GetKernelFile();
	std::string GetUserFile();

	// Update the enabled/disabled states of buttons.
	void UpdateEnabling();
	void LaunchTraceViewer(const std::string traceFilename);

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedStarttracing();
	afx_msg void OnBnClickedStoptracing();
	afx_msg void OnBnClickedCompresstrace();
	afx_msg void OnBnClickedCpusamplingcallstacks();
	afx_msg void OnBnClickedContextswitchcallstacks();
	afx_msg void OnBnClickedShowcommands();
};
