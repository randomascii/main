#ifndef XPERFUI_UTILITY_H
#define XPERFUI_UTILITY_H

#include <vector>
#include <string>

std::vector<std::string> split(const std::string& s, char c);
std::vector<std::string> GetFileList(const std::string& pattern);
std::string LoadFileAsText(const std::string& fileName);
void WriteTextAsFile(const std::string& fileName, const std::string& text);

void SetRegistryDWORD(HKEY root, const std::string& subkey, const std::string& valueName, DWORD value);
void CreateRegistryKey(HKEY root, const std::string& subkey, const std::string& newKey);

// Various functions to hack a UNICODE build into working.
std::string GetEditControlText(HWND hwnd, int id);
std::string GetListControlText(HWND hwnd, int id, int index);
#ifdef _UNICODE
std::wstring AnsiToTChar(const std::string& text);
#else
std::string AnsiToTChar(const std::string& text);
#endif

#endif
