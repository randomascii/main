/*
Copyright 2013 Cygnus Software

This code exists to correct a Windows 7 problem with alt+tab that has been
documented at:
http://randomascii.wordpress.com/2011/10/16/alttab-depth-inversion-bug/

This program is distributed for free, with no warranty.
*/

#include "stdafx.h"

void RunFixer();

int _tmain(int argc, _TCHAR* argv[])
{
	printf("Fixing alt+tab depth problems by clearing the visible flag on all\n"
		   "zero by zero always-on-top visible windows.\n\n");

	RunFixer();

	printf("\nAlt+tab should now work correctly. To have this fix run every time\n"
		   "you type Tab just copy AltTabFixContinuous.exe to your startup folder.\n"
		   "This program is provided free of charge to fix the problems discussed here:\n"
		   "http://randomascii.wordpress.com/2011/10/16/alttab-depth-inversion-bug/.\n"
		   "Copyright 2013 Cygnus Software.\n"
		   "Press enter to exit.\n");

	getc(stdin);
	return 0;
}
