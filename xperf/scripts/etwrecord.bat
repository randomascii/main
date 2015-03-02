@setlocal

@rem Add this batch file's directory to the path.
@set batchdir=%~dp0
@set path=%path%;%batchdir%

@rem Set the tracedir and temptracedir environment variables
@if not "%tracedir%" == "" goto TraceDirSet
@set tracedir=%batchdir%..\traces
:TraceDirSet
@if not "%temptracedir%" == "" goto TempTraceDirSet
@set temptracedir=%batchdir%..
:TempTraceDirSet

@rem Make sure %tracedir% and %temptracedir% exist
@if exist %tracedir% goto TraceDirExists
@mkdir %tracedir%
:TraceDirExists
@if exist %temptracedir% goto TempTraceDirExists
@mkdir %temptracedir%
:TempTraceDirExists

@call etwcommonsettings.bat
@call etwregister.bat

@rem Generate a file name based on the current date and time and put it in
@rem the parent directory of the batch file.
@rem Note: this probably fails in some locales. Sorry.
@for /F "tokens=2-4 delims=/- " %%A in ('date/T') do @set datevar=%%C-%%A-%%B
@for /f "tokens=1-2 delims=/:" %%a in ("%TIME%") do @set timevar=%%a-%%b
@set FileName=%tracedir%\%username%_%datevar%_%timevar%

@if "%1" == "" goto NoFileSpecified
@set ext=%~x1
@if "%ext%" == ".etl" goto fullFileSpecified
@rem Must be just a sub-component -- add it to the end
@set Filename=%FileName%_%1.etl
@goto FileSpecified

:fullFileSpecified
@set FileName=%1
@goto FileSpecified

:NoFileSpecified
@set FileName=%FileName%.etl
:FileSpecified
@echo Trace will be saved to %FileName%

@rem Set trace parameters. Latency is a good default group, and Power adds
@rem CPU power management details. Dispatcher allows wait classification and
@rem file IO is useful for seeing what disk reads are requested.
@rem Note that Latency is equal to PROC_THREAD+LOADER+DISK_IO+HARD_FAULTS+DPC+INTERRUPT+CSWITCH+PROFILE
@set KernelProviders=Latency+POWER+DISPATCHER+FILE_IO+FILE_IO_INIT

@set KernelStackWalk=-stackwalk PROFILE
@if "%2" == "nocswitchstacks" goto NoCSwitchStacks
@rem Recording call stacks on context switches is very powerful and useful
@rem but does make context switching more expensive.
@set KernelStackWalk=%KernelStackWalk%+CSWITCH+READYTHREAD
:NoCSwitchStacks

@rem Add recording of VirtAlloc data and stacks
@set KernelProviders=%KernelProviders%+VIRT_ALLOC
@set KernelStackWalk=%KernelStackWalk%+VirtualAlloc

@rem Modified to reduce context switching overhead
@rem @set KernelStackWalk=-stackwalk PROFILE
@rem Disable stack walking if you want to reduce the data rate.
@if "%2" == "nostacks" set KernelStackWalk=
@set SessionName=gamesession

@rem Set the buffer size to 1024 KB (default is 64-KB) and a minimum of 300 buffers.
@rem This helps to avoid losing events. Increase minbuffers if events are still lost,
@rem but be aware that the current setting locks up 300 MB of RAM.
@set KBuffers=-buffersize 1024 -minbuffers 1200

@rem Stop the circular tracing if it is enabled.
@rem @call etwcirc stop

@rem Select locations for the temporary kernel and user trace files.
@rem These locations are chosen to be on the SSD and be in the directory
@rem that is excluded from virus scanning and bit9 hashing.
@set kernelfile=%temptracedir%\kernel.etl
@set userfile=%temptracedir%\user.etl

@rem Start the kernel provider and user-mode provider
xperf -on %KernelProviders% %KernelStackWalk% %KBuffers% -f %kernelfile% -start %SessionName% -on %UserProviders%+%CustomProviders% -f %userfile%
@if not %errorlevel% equ -2147023892 goto NotInvalidFlags
@echo Trying again without the custom providers. Run ETWRegister.bat to register them.
xperf -on %KernelProviders% %KernelStackWalk% %KBuffers% -f %kernelfile%  -start %SessionName% -on %UserProviders% -f %userfile%
:NotInvalidFlags
@if not %errorlevel% equ 0 goto failure

@echo Run the test you want to profile here
@pause

@rem Record the data and stop tracing
xperf -stop %SessionName% -stop
@set FileAndCompressFlags=%FileName% -compress
@if "%NOETWCOMPRESS%" == "" goto compressTrace
@set FileAndCompressFlags=%FileName%
:compressTrace

@rem New method -- allows requesting trace compression. This is a NOP on
@rem Windows 7 but on Windows 8 creates 5-7x smaller traces (that don't load on Windows 7)

@rem Rename c:\Windows\AppCompat\Programs\amcache.hve to avoid serious merge
@rem performance problems (up to six minutes!)
@set HVEDir=c:\Windows\AppCompat\Programs
@rename %HVEDir%\Amcache.hve Amcache_temp.hve 2>nul
@set RenameErrorCode=%errorlevel%

xperf -merge %kernelfile% %userfile% %FileAndCompressFlags%

@rem Rename the file back
@if not "%RenameErrorCode%" equ "0" goto SkipRename
@rename %HVEDir%\Amcache_temp.hve Amcache.hve
:SkipRename

@if not %errorlevel% equ 0 goto FailureToRecord
@rem Delete the temporary ETL files
@del %kernelfile%
@del %userfile%
@echo Trace data is in %FileName% -- load it with wpa or xperfview or gpuview.
@dir %FileName% | find /i ".etl"
@rem Preprocessing symbols to avoid delays with Chrome's huge symbols
@pushd %batchdir%
python StripChromeSymbols.py %FileName%
@popd
start wpa %FileName%
@rem Restart circular tracing.
@rem @call etwcirc StartSilent
@exit /b

:FailureToRecord
@rem Delete the temporary ETL files
@del %kernelfile%
@del %userfile%
@echo Failed to record trace.
@rem Restart circular tracing.
@rem @call etwcirc StartSilent
@exit /b

:failure
@rem Check for Access Denied
@if %errorlevel% == %ACCESSISDENIED% goto NotAdmin
@echo Failed to start tracing. Make sure the custom providers are registered
@echo (using etwregister.bat) or remove the line that adds them to UserProviders.
@echo Make sure you are running from an elevated command prompt.
@echo Forcibly stopping the kernel and user session to correct possible
@echo "file already exists" errors.
xperf -stop %SessionName% -stop
@exit /b

:NotAdmin
@echo You must run this batch file as administrator.
@exit /b
