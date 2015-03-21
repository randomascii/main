This is the to-do list for XperfUI, a UI to wrap recording ETW traces with xperf.exe.

Done:
Get appropriate default for tracedir.
Create tracedir as needed.
Launch wpa after a trace finishes recording.
Disable start/stop/snap buttons as appropriate.
Show commands and command output.
Add verbose option (default is off) to control whether commands are displayed.
Print readable, well-spaced information about the state of tracing.
Respect tracedir and temptracedir.
Generate good trace names, with built-in keywords.
Input recording.
Move more initialization like _NT_SYMBOL_PATH to startup.
Process Chrome symbols.
Finish input recording options and give warning about full input recording.
List all traces in tracedir and allow viewing them.
Register providers.
set DisablePagingExecutive to 1
Vertically resizable window to show more traces.
Editing and auto-saving of trace notes.
Stop tracing on shutdown!!!
Hot key to stop/record traces.
Make sure trace notes are saved when closing.
Tool-tips
Add circular-buffer support.
Heap tracing.
Show ProcessChromeSymbols.py status as it runs so it doesn't look like it's hung forever.
Rename output executables and check in a renamed copy so people don't have to build it.
StripChromeSymbols.py needs to pass the appropriate command-line to xperf to ignored dropped events and time inversions.
Unicode support
Trace list should let the user:
- Delete traces
- Compress traces
- Explore to the trace directory
- Copy the traces name/path to the clipboard
- Run StripChromeSymbols.py
Remove references to _T macro.
Remove .etl extension from list of traces -- it adds no value and wastes space.



Most important tasks:
Copy over startup profile on first-run, and subsequent runs?
Optionally copy over 64-bit dbghelp.dll and symsrv.dll?

Renaming of traces and associated files.
Add OS specific checks for what user providers to enable.

To-do eventually:
Error checking -- checking for failures to start or stop tracing.
Make sure trace notes are disabled when no trace is selected, including when
traces are added or deleted.
Disable compress options (checkbox and menu) for Windows 7 and below.
Allow configuring which symbols should be stripped.
Configure a maximum time to trace for to avoid infinitely long traces.
Transparent compression/decompression into .zip files.
CPU frequency monitoring.
Have a button to copy the recommended (or one of several recommended) startup profiles.
Move focus away from buttons before they are disabled, as when starting a trace with Alt+T.
Disable ESC as a way to close XperfUI.
Remember settings.
Implement settings dialog - configure trace directories, buffer sizes, heap-tracing executable,
option for stacks on user events, option for DX tracing.
Trace list should let the user:
- Rename traces (and associated text file)
- Run arbitrary scripts on all traces
Code cleanup:
- getenv wrapper
- ordering code sanely
- moving more code to separate functions/files
Have xperftemptracedir default to %temp%?
Add compatibility manifest up to Windows 8.1
Create an installer that will install the MFC DLLs: https://msdn.microsoft.com/en-us/library/dd293568.aspx
