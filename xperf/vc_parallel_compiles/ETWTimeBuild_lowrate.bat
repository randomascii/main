@if "%~1" == "" goto nofile
@if "%~2" == "" goto nofile

@setlocal
@if not "%VSINSTALLDIR%" == "" goto alreadySet
@echo You must run "%%VS120COMNTOOLS%%..\..\VC\vcvarsall.bat" before running this batch file.
@exit /b
:alreadySet

set wrapper=%~dp0devenvwrapper.exe
set manifest=%~dp0devenvwrapper\devenvwrapperetwprovider.man

@if exist %wrapper% goto wrapperOkay
@echo %wrapper% not found. Build this project first.
@exit /b
@:wrapperOkay

@if exist %manifest% goto manifestOkay
@echo %manifest% not found.
@exit /b
@:manifestOkay

@rem Unregister and then register the ETW provider
@rem The provider executable has to be copied to %temp%, per the manifest.
xcopy /y %wrapper% %temp%
@rem Unregister the provider.
wevtutil um %manifest%
@if %errorlevel% == 5 goto NotElevated
@rem Reregister the provider.
wevtutil im %manifest%
@goto checksPassed

@:NotElevated
@echo This script must be run from an elevated (administrator) command
@echo prompt. Try again from an elevated prompt.
@exit /b

@:checksPassed


@rem Create a file name that includes the solution name and date and time.
@for /F "tokens=2-4 delims=/- " %%A in ('date/T') do @set datevar=%%C_%%A_%%B
@for /F "tokens=1-3 delims=:-. " %%A in ('echo %time%') do @set timevar=%%A.%%B.%%C&set hour=%%A
@rem Make sure that morning hours such as 9:00 are handled as 09 rather than 9
@if %hour% LSS 10 set timevar=0%timevar%
@set tracename=BuildTrace_%~n1_%~n2_%datevar%_%timevar%.etl

@rem Have just enough data to show process lifetimes
@set KernelProviders=PROC_THREAD+LOADER

@rem Enable VS-Hack so we can inject compilation events
@set UserProviders=VS-Hack
@set SessionName=buildsession

@set KBuffers=-buffersize 1024 -minbuffers 400
xperf -on %KernelProviders% %KBuffers% -start %SessionName% -on %UserProviders%
@if not %errorlevel% equ -2147023892 goto NotInvalidFlags
@echo Trying again without the custom providers. Run ETWRegister.bat to register them.
xperf -on %KernelProviders% %KBuffers%
:NotInvalidFlags
@if not %errorlevel% equ 0 goto failure

@set starttime=%time%
@xperf -m "Starting build"

@rem Use the CL environment variable to set /Bt+
set cl=/Bt+

@rem Run the Visual Studio wrapper executable, created from devenvwrapper.sln.
@rem This will add compilation completion events to the ETW stream, as long as
@rem /Bt+ is added to the compiler options of the project being compiled.
%wrapper% devenv "%~1" /rebuild "release" /project "%~2"

@xperf -m "Finishing build"
@echo Build went from %starttime% to %time%
@xperf -stop %SessionName% -stop -d "%tracename%"
@echo Trace data is in %tracename% -- view it with WPA. Use Profiles, Apply... and select %~dp0ProcessLifetimes.wpaProfile
@dir "%tracename%"
@exit /b

:nofile
@echo Usage: %~nx0 solution.sln projectname
@exit /b

:failure
@echo Failed to start tracing.
@echo Make sure you are running from an elevated command prompt.
@echo Make sure you aren't running tracing already.
@echo Force-stopping any previous sessions that may be interfering.
xperf -stop %SessionName% -stop
@exit /b
