#include "stdafx.h"
#include "XperfUI.h"
#include "XperfUIDlg.h"

/*
Helper file to put random big messy functions in, to avoid clutter
in XperfUIDlg.cpp. In other words, a second implementation file
for CXperfUIDlg.
*/

static const wchar_t* pSettings = L"Settings";

struct NameToBoolSetting
{
	const wchar_t* pName;
	bool* pSetting;
};

struct NameToRangedInt
{
	const wchar_t* pName;
	int* pSetting;
	int min, max;
};

struct NameToString
{
	const wchar_t* pName;
	std::wstring* pSetting;
};

void CXperfUIDlg::TransferSettings(bool saving)
{
	CWinApp* pApp = AfxGetApp();

	NameToBoolSetting bools[] =
	{
		{ L"CompressTraces", &bCompress_ },
		{ L"CswitchStacks", &bCswitchStacks_ },
		{ L"SampledStacks", &bSampledStacks_ },
		{ L"FastSampling", &bFastSampling_ },
		{ L"DirectXTracing", &bDirectXTracing_ },
		{ L"ShowCommands", &bShowCommands_ },
	};

	for (auto& m : bools)
	{
		if (saving)
			pApp->WriteProfileInt(pSettings, m.pName, *m.pSetting);
		else
			*m.pSetting = pApp->GetProfileIntW(pSettings, m.pName, *m.pSetting) != false;
	}

	NameToRangedInt ints[] =
	{
		// Note that a setting of kKeyLoggerFull cannot be restored from
		// settings, to avoid privacy problems.
		{ L"InputTracing", (int*)&InputTracing_, kKeyLoggerOff, kKeyLoggerAnonymized },
		{ L"TracingMode", (int*)&tracingMode_, kTracingToFile, kHeapTracingToFile },
	};

	for (auto& m : ints)
	{
		if (saving)
			pApp->WriteProfileInt(pSettings, m.pName, *m.pSetting);
		else
		{
			int temp = pApp->GetProfileIntW(pSettings, m.pName, *m.pSetting);
			if (temp < m.min)
				temp = m.min;
			if (temp > m.max)
				temp = m.max;
			*m.pSetting = temp;
		}
	}

	NameToString strings[] =
	{
		{ L"HeapProfiledProcess", &heapTracingExe_ },
	};

	for (auto& m : strings)
	{
		if (saving)
			pApp->WriteProfileStringW(pSettings, m.pName, m.pSetting->c_str());
		else
		{
			CString result = pApp->GetProfileStringW(pSettings, m.pName, m.pSetting->c_str());
			*m.pSetting = result;
		}
	}
}
