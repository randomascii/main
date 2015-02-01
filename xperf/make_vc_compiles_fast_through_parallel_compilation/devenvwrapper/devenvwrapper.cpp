// This project wraps devenv.exe. If you compile with devenv /build and you have added
// /Bt+ to your compiler options then this program will convert the /Bt+ output to
// ETW events.
// /Bt+ only works reliably on VS 2013.
// For more information see http://randomascii.wordpress.com

#include "stdafx.h"
#include <string>
// Include the event register/write/unregister macros compiled from the manifest file.
// Note that this includes evntprov.h which requires a Vista+ Windows SDK.
#include "DevEnvWrapperETWProviderGenerated.h"

#include <map>

int _tmain(int argc, _TCHAR* argv[])
{
	// Initialize the ETW provider.
	EventRegisterVS_Hack();

	LARGE_INTEGER llFrequency;
	QueryPerformanceFrequency(&llFrequency);
	float frequency = float(llFrequency.QuadPart);

	std::string commandLine = "devenv";
	for (int arg = 2; arg < argc; ++arg)
	{
		commandLine += ' ';
		char buffer[1000];
		// Convert from wchar_t to char
		// Don't put quotes around switches -- devenv hates that!
		if (argv[arg][0] == L'/')
			sprintf_s(buffer, "%S", argv[arg]);
		else
			sprintf_s(buffer, "\"%S\"", argv[arg]);
		commandLine += buffer;
	}
	printf("%s\n", commandLine.c_str());
	FILE* pOutput = _popen(commandLine.c_str(), "r");
	if (!pOutput)
		return 10;

	// Raise our priority so that we can always respond to compiler output quickly. It's fine
	// to run at high priority since we will mostly be idle.
	// It is important to raise our priority after starting devenv because we don't want to
	// accidentally run it at high priority.
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	std::map<std::string, float> firstStageTimes;

	// Buffer for reading output lines from devenv.
	char buffer[2000];
	int timingDetailsCount = 0;
	for (;;)
	{
		if (!fgets(buffer, _countof(buffer), pOutput))
			break;
		// Print all the output lines we see.
		printf("%s", buffer);

		// Need to parse lines that look roughly like this:
		// 1>  time(C:\Program Files (x86)\Microsoft Visual C++ Compiler Nov 2013 CTP\bin\c1xx.dll)=1.38807s < 1379145750955 - 1379148726430 > BB [C:\homedepot\Source\unmanaged\BlogStuff\CompileParallel\Group3_J.cpp]
		// 1>  time(C:\Program Files (x86)\Microsoft Visual C++ Compiler Nov 2013 CTP\bin\c2.dll)=0.00499s < 1379148732623 - 1379148743323 > BB [C:\homedepot\Source\unmanaged\BlogStuff\CompileParallel\Group3_J.cpp]
		// Look for .dll and then walk backwards to find c1xx versus c2. Then look forward for '<' and do a sscanf to get the QueryPerformanceCounter numbers.
		// Then look for [] or perhaps the last '\' to get the file name.
		int stage = 0;
		// Look for the markers for stage 1 and stage 2 compilation.
		if (strstr(buffer, "c1xx.dll"))
			stage = 1;
		if (strstr(buffer, "c2.dll"))
			stage = 2;
		if (stage > 0)
		{
			++timingDetailsCount;
			// Look for the '<' before the timing details.
			char* lessThan = strchr(buffer, '<');
			if (lessThan)
			{
				long long start, end;
				// Parse the timing details -- QPC start and end times
				if (2 == sscanf_s(lessThan, "< %lld - %lld", &start, &end))
				{
					float elapsed = (end - start) / frequency;
					LARGE_INTEGER currentCounter;
					QueryPerformanceCounter(&currentCounter);
					float startOffset = (start - currentCounter.QuadPart) / frequency; // Negative number representing offset from start.
					float endOffset = (end - currentCounter.QuadPart) / frequency; // Negative number representing offset from end.
					// Look for the file part of the source name.
					char* lastSlash = strrchr(lessThan, '\\');
					if (lastSlash)
					{
						const char* filename = lastSlash + 1;
						char* squareBracket = strchr(lastSlash, ']');
						if (squareBracket)
							squareBracket[0] = 0;

						// Write our custom ETW events, as defined in etwprovider.man.
						// Record details of the compile stage we just finished.
						if (stage == 1)
						{
							EventWriteCompileStage1Done(filename, elapsed, startOffset, endOffset);
							firstStageTimes[filename] = elapsed;
						}
						else
						{
							EventWriteCompileStage2Done(filename, elapsed, startOffset, endOffset);
							EventWriteCompileSummary(filename, elapsed + firstStageTimes[filename]);
						}
					}
				}
			}
		}
	}
	fclose(pOutput);

	if (timingDetailsCount)
	{
		printf("%d compilation timing details seen.\n", timingDetailsCount);
	}
	else
	{
		printf("No compilation timing details seen. Did you add /Bt+ to the compiler options?\n");
	}

	return 0;
}
