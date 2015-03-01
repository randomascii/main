@rem Customize to specify the location of your provider DLL or EXE

@rem Clean up some obsolete files
@if not exist etwprovider.man goto NoDelete1
@del etwprovider.man
:NoDelete1
@if not exist MultiProvider.exe goto NoDelete2
@del MultiProvider.exe
:NoDelete2

@set DLLFileMain=%~dp0ETWProviders.dll
@set ManifestFileMain=%~dp0etwproviders.man



@echo This will register the custom ETW providers.

@set DLLFile=%DLLFileMain%
@if not exist %DLLFile% goto NoDLL

@set ManifestFile=%ManifestFileMain%
@if not exist %ManifestFile% goto NoManifest

xcopy /y %DLLFile% %temp%
wevtutil um %ManifestFile%
@if %errorlevel% == 5 goto NotElevated
wevtutil im %ManifestFile%
@:Done
@exit /b

@:NotElevated
@echo ETW providers must be registered from an elevated (administrator) command
@echo prompt. Try again from an elevated prompt.
@exit /b

@:NoDLL
@echo Can't find %DLLFileMain%
@exit /b

@:NoManifest
@echo Can't find %ManifestFileMain%
@exit /b
