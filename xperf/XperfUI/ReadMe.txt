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

To-do before first release:
Renaming of traces and associated files.
Add OS specific checks for what user providers to enable.
Hot key to stop/record traces.

To-do eventually:
Disable compress option for Windows 7 and below.
Allow configuring which symbols should be stripped.
Tool-tips
Add circular-buffer support.
Heap tracing.
Unicode support
Configure a maximum time to trace for to avoid infinitely long traces.
Transparent compression/decompression into .zip files.
CPU frequency monitoring.
Allow configuring the temporary and final trace directories.
Have a button to copy the recommended (or one of several recommended) startup profiles.
Move focus away from buttons before they are disabled, as when starting a trace with Alt+T.
