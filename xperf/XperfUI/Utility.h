#ifndef XPERFUI_UTILITY_H
#define XPERFUI_UTILITY_H

#include <vector>
#include <string>

std::vector<std::wstring> split(const std::wstring& s, char c);
std::vector<std::wstring> GetFileList(const std::wstring& pattern);

// Load an ANSI or UTF-16 file into a wstring
std::wstring LoadFileAsText(const std::wstring& fileName);
// Write a wstring as UTF-16.
void WriteTextAsFile(const std::wstring& fileName, const std::wstring& text);

void SetRegistryDWORD(HKEY root, const std::wstring& subkey, const std::wstring& valueName, DWORD value);
void CreateRegistryKey(HKEY root, const std::wstring& subkey, const std::wstring& newKey);

// Various functions to hack a UNICODE build into working.
std::wstring GetEditControlText(HWND hwnd, int id);
std::wstring GetListControlText(HWND hwnd, int id, int index);
std::wstring AnsiToUnicode(const std::string& text);

#endif
