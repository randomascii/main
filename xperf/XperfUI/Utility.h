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
std::wstring AnsiToUnicode(const std::string& text);

// Return a pointer to the character after the final '\' or final '.'
// in the file part of a path.
// These return a pointer to the appropriate place in the passed in string,
// so it must remain valid. If the last character is '\' then GetFilePart
// will point at the NUL terminator. If there is no '.' after the last '\'
// then GetFileExt will point at the NUL terminator. They won't ever return
// an invalid pointer. 
const wchar_t* GetFilePart(const std::wstring& path);
const wchar_t* GetFileExt(const std::wstring& path);

int DeleteOneFile(HWND hwnd, const std::wstring& path);
int64_t GetFileSize(const std::wstring& path);

void SetClipboardText(const std::wstring& text);

#endif
