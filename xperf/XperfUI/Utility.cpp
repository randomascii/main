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
	WIN32_FIND_DATA findData;
	HANDLE hFindFile = FindFirstFileEx(pattern.c_str(), FindExInfoStandard,
				&findData, FindExSearchNameMatch, NULL, 0);

	std::vector<std::string> result;
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
