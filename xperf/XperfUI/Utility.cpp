#include "stdafx.h"
#include "Utility.h"
#include <fstream>

std::vector<std::string> split(const std::string& s, char c)
{
	std::string::size_type i = 0;
	std::string::size_type j = s.find(c);

	std::vector<std::string> result;
	while (j != std::string::npos)
	{
		result.push_back(s.substr(i, j - i));
		i = ++j;
		j = s.find(c, j);
	}

	if (!s.empty())
		result.push_back(s.substr(i, s.length()));

	return result;
}

std::vector<std::string> GetFileList(const std::string& pattern)
{
	WIN32_FIND_DATAA findData;
	HANDLE hFindFile = FindFirstFileExA(pattern.c_str(), FindExInfoStandard,
				&findData, FindExSearchNameMatch, NULL, 0);

	std::vector<std::string> result;
	if (hFindFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			result.push_back(findData.cFileName);
		} while (FindNextFileA(hFindFile, &findData));

		FindClose(hFindFile);
	}

	return result;
}

// Load a file and convert to a string. If the file contains
// an embedded NUL then the resulting string will be truncated.
std::string LoadFileAsText(const std::string& fileName)
{
	std::ifstream f;
	f.open(fileName, std::ios_base::binary);
	if (!f)
		return "";

	// Find the file length.
	f.seekg(0, std::ios_base::end);
	size_t length = (size_t)f.tellg();
	f.seekg(0, std::ios_base::beg);

	// Allocate a buffer and read the file.
	std::vector<char> data(length + 1);
	f.read(&data[0], length);
	if (!f)
		return "";

	// Add a null terminator.
	data[length] = 0;

	// Convert to string and return
	return &data[0];
}


void WriteTextAsFile(const std::string& fileName, const std::string& text)
{
	std::ofstream outFile;
	outFile.open(fileName, std::ios_base::binary);
	if (!outFile)
		return;

	outFile.write(text.c_str(), text.size());
}

void SetRegistryDWORD(HKEY root, const std::string& subkey, const std::string& valueName, DWORD value)
{
	HKEY key;
	LONG result = RegOpenKeyExA(root, subkey.c_str(), 0, KEY_ALL_ACCESS, &key);
	if (result == ERROR_SUCCESS)
	{
		LONG setResult = RegSetValueExA(key, valueName.c_str(), 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value));
		RegCloseKey(key);
	}
}

void CreateRegistryKey(HKEY root, const std::string& subkey, const std::string& newKey)
{
	HKEY key;
	LONG result = RegOpenKeyExA(root, subkey.c_str(), 0, KEY_ALL_ACCESS, &key);
	if (result == ERROR_SUCCESS)
	{
		HKEY resultKey;
		result = RegCreateKeyA(key, newKey.c_str(), &resultKey);
		if (result == ERROR_SUCCESS)
		{
			RegCloseKey(resultKey);
		}
		RegCloseKey(key);
	}
}

std::string GetEditControlText(HWND hwnd, int id)
{
	std::string result;
	HWND hEdit = GetDlgItem(hwnd, id);
	if (!hEdit)
		return result;
	int length = GetWindowTextLengthA(hEdit);
	std::vector<char> buffer(length + 1);
	GetDlgItemTextA(hwnd, id, &buffer[0], buffer.size());
	// Double-verify that the buffer is null-terminated.
	buffer[buffer.size() - 1] = 0;
	return &buffer[0];
}

std::string GetListControlText(HWND hwnd, int id, int index)
{
	std::string result;
	int length = SendDlgItemMessageA(hwnd, id, LB_GETTEXTLEN, index, 0);
	if (length == LB_ERR)
		return result;
	std::vector<char> buffer(length + 1);
	SendDlgItemMessageA(hwnd, id, LB_GETTEXT, index, (LPARAM)&buffer[0]);
	// Double-verify that the buffer is null-terminated.
	buffer[buffer.size() - 1] = 0;
	return &buffer[0];
}

#ifdef _UNICODE
std::wstring AnsiToTChar(const std::string& text)
{
	// Determine number of wide characters to be allocated for the
	// Unicode string.
	size_t cCharacters = text.size() + 1;

	std::vector<wchar_t> buffer(cCharacters);

	// Convert to Unicode.
	std::wstring result;
	if (MultiByteToWideChar(CP_ACP, 0, text.c_str(), cCharacters, &buffer[0], cCharacters))
	{
		// Double-verify that the buffer is null-terminated.
		buffer[buffer.size() - 1] = 0;
		result = &buffer[0];
		return result;
	}

	return result;
}
#else
std::string AnsiToTChar(const std::string& text)
{
	return text;
}
#endif
