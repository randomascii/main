// Settings.cpp : implementation file
//

#include "stdafx.h"
#include "XperfUI.h"
#include "Settings.h"
#include "Utility.h"
#include "afxdialogex.h"


// CSettings dialog

IMPLEMENT_DYNAMIC(CSettings, CDialogEx)

CSettings::CSettings(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSettings::IDD, pParent)
{

}

CSettings::~CSettings()
{
}

void CSettings::DoDataExchange(CDataExchange* pDX)
{
	DDX_Control(pDX, IDC_HEAPEXE, btHeapTracingExe_);
	DDX_Control(pDX, IDC_TRACEDIR, btTraceDir_);
	DDX_Control(pDX, IDC_TEMPTRACEDIR, btTempTraceDir_);
	DDX_Control(pDX, IDC_BUFFERSIZES, btBufferSizes_);

	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSettings, CDialogEx)
END_MESSAGE_MAP()

BOOL CSettings::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetDlgItemText(IDC_HEAPEXE, heapTracingExe_.c_str());
	btTraceDir_.EnableWindow(FALSE);
	btTempTraceDir_.EnableWindow(FALSE);
	btBufferSizes_.EnableWindow(FALSE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CSettings::OnOK()
{
	heapTracingExe_ = GetEditControlText(*this, IDC_HEAPEXE);
	if (heapTracingExe_.size() <= 4 || heapTracingExe_.substr(heapTracingExe_.size() - 4, heapTracingExe_.size()) != L".exe")
	{
		AfxMessageBox(_T("The heap-profiled process name must end in .exe"));
		return;
	}
	CDialog::OnOK();
}
