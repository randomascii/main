// Copyright 2013 Cygnus Software
// This project displays information about PDB symbol files. It displays
// the PDB's timestamp, GUID, and age. Note that the GUID, age and name
// are all that matters for matching a PDB file to an executable, crash
// dump, etc.
//

#include <stdio.h>
#include <tchar.h>
#include <atlbase.h>
#include "dia2.h"

#pragma comment(lib, "diaguids.lib")

bool DumpAgeAndSignature(_TCHAR* sFile);

int _tmain(int argc, _TCHAR* argv[])
{
    if (argc != 2)
    {
        printf("Displays pdb file age and guid.\n\n");
        printf("usage: %S <pdb>\n", argv[0]);
        return 1;
    }

    if (FAILED(CoInitialize(NULL)))
    {
        printf("CoInitialize failed.\n");
        return 1;
    }

    int iResult = (DumpAgeAndSignature(argv[1]) == true) ? 0 : 1;

    CoUninitialize();

    return iResult;
}

bool DumpAgeAndSignature(_TCHAR* sFile)
{
    CComPtr<IDiaDataSource> pSource;
    if (FAILED(CoCreateInstance(CLSID_DiaSource, NULL, CLSCTX_INPROC_SERVER, __uuidof( IDiaDataSource ), (void **) &pSource)))
    {   
        printf("Could not CoCreate CLSID_DiaSource. Please register msdia80.dll.\n");
        return false;
    }

    CComBSTR sPdbFile(sFile);

    if (FAILED(pSource->loadDataFromPdb(sPdbFile)))
    {
        printf("loadDataFromPdb failed.\n");
        return false;
    }

    CComPtr<IDiaSession> pSession;
    if (FAILED(pSource->openSession(&pSession))) 
    {
        printf("openSession failed.\n");
        return false;
    }

    CComPtr<IDiaSymbol> pGlobal;
    if (FAILED(pSession->get_globalScope(&pGlobal)))
    {
        printf("get_globalScope failed.\n");
        return false;
    }

    // Initialize to zero so that if get_age returns S_FALSE (indicating
    // no value available) we're guaranteed to have a valid value.
    DWORD dwAge = 0;
    if (FAILED(pGlobal->get_age(&dwAge)))
    {
        printf("get_age failed.\n");
        return false;
    }

    GUID guid = {0};
    if (FAILED(pGlobal->get_guid(&guid)))
    {
        printf("get_guid failed.\n");
        return false;
    }

    DWORD timeStamp = 0;
    // Yes, you use get_signature to get the timestamp, not get_timestamp
    if (FAILED(pGlobal->get_signature(&timeStamp)))
    {
        printf("get_timeStamp failed.\n");
        return false;
    }

    wchar_t wszGuid[64];
    StringFromGUID2(guid, wszGuid, 64);
    printf("Timestamp            GUID                        age\n");
    printf("%08X, %ws, %d\n", timeStamp, wszGuid, dwAge);

    return true;
}
