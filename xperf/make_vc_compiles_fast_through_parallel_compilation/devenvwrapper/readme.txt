This is an extremely simple example of an ETW provider, and it does something useful.

etwprovider.man has a custom build step which generates a header file, a .rc file, and some other stuff.

The resource file must be added to the project.

The header file should be included so that its functions can be called -- specifically
EventRegisterVS_Hack() and EventWriteCompileStageDone().

Before using the ETW provider you have to register it with the system. Roughly speaking this involves
doing this from an administrator command prompt:

	xcopy /y devenvwrapper.exe %temp%
	wevtutil um devenvwrapper\etwprovider.man
	wevtutil im devenvwrapper\etwprovider.man

For more details see https://randomascii.wordpress.com/2014/03/22/make-vc-compiles-fast-through-parallel-compilation/

This has only been tested with VC++ 2013. It requires Windows Vista or higher.
