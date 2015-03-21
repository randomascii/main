#include "stdafx.h"
#include "Utility.h"
#include <fstream>

std::vector<std::wstring> split(const std::wstring& s, char c)
{
	std::wstring::size_type i = 0;
	std::wstring::size_type j = s.find(c);

	std::vector<std::wstring> result;
	while (j != std::wstring::npos)
	{
		result.push_back(s.substr(i, j - i));
		i = ++j;
		j = s.find(c, j);
	}

	if (!s.empty())
		result.push_back(s.substr(i, s.length()));

	return result;
}

std::vector<std::wstring> GetFileList(const std::wstring& pattern)
{
	WIN32_FIND_DATA findData;
	HANDLE hFindFile = FindFirstFileEx(pattern.c_str(), FindExInfoStandard,
				&findData, FindExSearchNameMatch, NULL, 0);

	std::vector<std::wstring> result;
	if (hFindFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			result.push_back(findData.cFileName);
		} while (FindNextFile(hFindFile, &findData));

		FindClose(hFindFile);
	}

	return result;
}

// Load a file and convert to a string. If the file contains
// an embedded NUL then the resulting string will be truncated.
std::wstring LoadFileAsText(const std::wstring& fileName)
{
	std::ifstream f;
	f.open(fileName, std::ios_base::binary);
	if (!f)
		return L"";

	// Find the file length.
	f.seekg(0, std::ios_base::end);
	size_t length = (size_t)f.tellg();
	f.seekg(0, std::ios_base::beg);

	// Allocate a buffer and read the file.
	std::vector<char> data(length + 2);
	f.read(&data[0], length);
	if (!f)
		return L"";

	// Add a multi-byte null terminator.
	data[length] = 0;
	data[length+1] = 0;

	const wchar_t bom = 0xFEFF; // Always write a byte order mark
	if (memcmp(&bom, &data[0], sizeof(bom)) == 0)
	{
		// Assume UTF-16, strip bom, and return.
		return reinterpret_cast<const wchar_t*>(&data[sizeof(bom)]);
	}

	// If not-UTF-16 then convert from ANSI to wstring and return
	return AnsiToUnicode(&data[0]);
}


void WriteTextAsFile(const std::wstring& fileName, const std::wstring& text)
{
	std::ofstream outFile;
	outFile.open(fileName, std::ios_base::binary);
	if (!outFile)
		return;

	const wchar_t bom = 0xFEFF; // Always write a byte order mark
	outFile.write(reinterpret_cast<const char*>(&bom), sizeof(bom));
	outFile.write(reinterpret_cast<const char*>(text.c_str()), text.size() * sizeof(text[0]));
}

void SetRegistryDWORD(HKEY root, const std::wstring& subkey, const std::wstring& valueName, DWORD value)
{
	HKEY key;
	LONG result = RegOpenKeyEx(root, subkey.c_str(), 0, KEY_ALL_ACCESS, &key);
	if (result == ERROR_SUCCESS)
	{
		LONG setResult = RegSetValueEx(key, valueName.c_str(), 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value));
		RegCloseKey(key);
	}
}

void CreateRegistryKey(HKEY root, const std::wstring& subkey, const std::wstring& newKey)
{
	HKEY key;
	LONG result = RegOpenKeyEx(root, subkey.c_str(), 0, KEY_ALL_ACCESS, &key);
	if (result == ERROR_SUCCESS)
	{
		HKEY resultKey;
		result = RegCreateKey(key, newKey.c_str(), &resultKey);
		if (result == ERROR_SUCCESS)
		{
			RegCloseKey(resultKey);
		}
		RegCloseKey(key);
	}
}

std::wstring GetEditControlText(HWND hwnd, int id)
{
	std::wstring result;
	HWND hEdit = GetDlgItem(hwnd, id);
	if (!hEdit)
		return result;
	int length = GetWindowTextLength(hEdit);
	std::vector<wchar_t> buffer(length + 1);
	GetDlgItemText(hwnd, id, &buffer[0], buffer.size());
	// Double-verify that the buffer is null-terminated.
	buffer[buffer.size() - 1] = 0;
	return &buffer[0];
}

std::wstring AnsiToUnicode(const std::string& text)
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

const wchar_t* GetFilePart(const std::wstring& path)
{
	const wchar_t* pLastSlash = wcsrchr(path.c_str(), '\\');
	if (pLastSlash)
		return pLastSlash + 1;
	return path.c_str() + path.size();
}

const wchar_t* GetFileExt(const std::wstring& path)
{
	const wchar_t* pFilePart = GetFilePart(path);
	const wchar_t* pLastPeriod = wcsrchr(pFilePart, '.');
	if (pLastPeriod)
		return pLastPeriod + 1;
	return pFilePart + wcslen(pFilePart);
}

int DeleteOneFile(HWND hwnd, const std::wstring& path)
{
	std::vector<wchar_t> fileNames;
	// Push the file name and its NULL terminator onto the vector.
	fileNames.insert(fileNames.end(), path.c_str(), path.c_str() + path.size());
	fileNames.push_back(0);

	// Double null-terminate.
	fileNames.push_back(0);

	SHFILEOPSTRUCT fileOp =
	{
		hwnd,
		FO_DELETE,
		&fileNames[0],
		NULL,
		FOF_ALLOWUNDO | FOF_FILESONLY | FOF_NOCONFIRMATION,
	};
	// Delete using the recycle bin.
	int result = SHFileOperation(&fileOp);

	return result;
}

void SetClipboardText(const std::wstring& text)
{
	BOOL cb = OpenClipboard(GetDesktopWindow());
	if (!cb)
		return;

	EmptyClipboard();

	size_t length = (text.size() + 1) * sizeof(wchar_t);
	HANDLE hmem = GlobalAlloc(GMEM_MOVEABLE, length);
	if (hmem)
	{
		void *ptr = GlobalLock(hmem);
		if (ptr != NULL)
		{
			memcpy(ptr, text.c_str(), length);
			GlobalUnlock(hmem);

			SetClipboardData(CF_UNICODETEXT, hmem);
		}
	}

	CloseClipboard();
}

int64_t GetFileSize(const std::wstring& path)
{
	LARGE_INTEGER result;
	HANDLE hFile = CreateFile(path.c_str(), GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return 0;

	if (GetFileSizeEx(hFile, &result))
	{
		CloseHandle(hFile);
		return result.QuadPart;
	}
	CloseHandle(hFile);
	return 0;
}
