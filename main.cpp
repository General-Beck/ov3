// #include <iostream>

using namespace std;
// linking dll in one operation
// gcc -rdynamic -o ovr ovr.cpp -ldl
// gcc ovr.cpp >f  2>&1



// ov3.cpp : Defines the entry point for the console application.
//

///
///  Copyright (c) 2013 Advanced Micro Devices, Inc.

///  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
///  EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
///  WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

/// \file Overdrive_Sample.cpp
/// \brief C/C++ Overdrive Color sample application, based on ADL_Sample project
///
/// A sample test application to test Overdrive get functions
/// Author: Ilia Blank & Denis S. Soldatov aka General-Beck 
/// e-mail: general.beck@gmail.com


#define LINUX

#if defined (LINUX)
#include "include/adl_sdk.h"
#include "include/adl-func.h"
//#include "include/adl_defines.h"
//#include "include/adl_structures.h"
//#include "include/customer/oem_structures.h"
#include <dlfcn.h>	//dyopen, dlsym, dlclose
#include <stdlib.h>
#include <string.h>	//memeset
#include <unistd.h>	//sleep

#else
#include <windows.h>
#include <tchar.h>
#include "include/adl_sdk.h" //Replace with local location of ADL SDK
#include "include/adl-func.h"
#endif

#include <stdio.h>
#include <jsonrpccpp/server/connectors/httpserver.h>
#include <jsonrpccpp/client/connectors/httpclient.h>









int main(int argc, _TCHAR* argv[])
{

#if defined (LINUX)
	void *hDLL;		// Handle to .so library
#else
	HINSTANCE hDLL;		// Handle to DLL
#endif

	ADL_MAIN_CONTROL_CREATE					ADL_Main_Control_Create;
	ADL_MAIN_CONTROL_DESTROY				ADL_Main_Control_Destroy;
	ADL_ADAPTER_NUMBEROFADAPTERS_GET		ADL_Adapter_NumberOfAdapters_Get;
	ADL_ADAPTER_ADAPTERINFO_GET				ADL_Adapter_AdapterInfo_Get;
	ADL_ADAPTER_ACTIVE_GET					ADL_Adapter_Active_Get;
	ADL_OVERDRIVE_CAPS						ADL_Overdrive_Caps;
	LPAdapterInfo						    lpAdapterInfo = NULL;
	int  i;
	int  ADL_Err = ADL_ERR;
	int  iNumberAdapters = 0;
	int  iOverdriveSupported = 0;
	int  iOverdriveEnabled = 0;

	int	 iOverdriveVersion = 0;

#if defined (LINUX)
	hDLL = dlopen("libatiadlxx.so", RTLD_LAZY | RTLD_GLOBAL);
#else

//	LPCWSTR pbuft = ("C:\Documents and Settings\er\ћои документы\Visual Studio 2010\Projects\loaddll4\Debug\dll4.dll");

	hDLL = LoadLibraryA("atiadlxx.dll");
	if (hDLL == NULL)
		// A 32 bit calling application on 64 bit OS will fail to LoadLIbrary.
		// Try to load the 32 bit library (atiadlxy.dll) instead
		hDLL = LoadLibraryA("atiadlxy.dll");
#endif

	if (NULL == hDLL)
	{
		printf("ADL library not found!\n");
		return 0;
	}

	ADL_Main_Control_Create = (ADL_MAIN_CONTROL_CREATE)GetProcAddress(hDLL, "ADL_Main_Control_Create");
	ADL_Main_Control_Destroy = (ADL_MAIN_CONTROL_DESTROY)GetProcAddress(hDLL, "ADL_Main_Control_Destroy");
	ADL_Adapter_NumberOfAdapters_Get = (ADL_ADAPTER_NUMBEROFADAPTERS_GET)GetProcAddress(hDLL, "ADL_Adapter_NumberOfAdapters_Get");
	ADL_Adapter_AdapterInfo_Get = (ADL_ADAPTER_ADAPTERINFO_GET)GetProcAddress(hDLL, "ADL_Adapter_AdapterInfo_Get");
	ADL_Adapter_Active_Get = (ADL_ADAPTER_ACTIVE_GET)GetProcAddress(hDLL, "ADL_Adapter_Active_Get");
	ADL_Overdrive_Caps = (ADL_OVERDRIVE_CAPS)GetProcAddress(hDLL, "ADL_Overdrive_Caps");

	if (NULL == ADL_Main_Control_Create ||
		NULL == ADL_Main_Control_Destroy ||
		NULL == ADL_Adapter_NumberOfAdapters_Get ||
		NULL == ADL_Adapter_AdapterInfo_Get ||
		NULL == ADL_Adapter_Active_Get ||
		NULL == ADL_Overdrive_Caps
		)
	{
		printf("ADL's API is missing!\n");
		return 0;
	}

	// Initialize ADL. The second parameter is 1, which means:
	// retrieve adapter information only for adapters that are physically present and enabled in the system
	if (ADL_OK != ADL_Main_Control_Create(ADL_Main_Memory_Alloc, 1))
	{
		printf("ADL Initialization Error!\n");
		return 0;
	}

	// Obtain the number of adapters for the system
	if (ADL_OK != ADL_Adapter_NumberOfAdapters_Get(&iNumberAdapters))
	{
		printf("Cannot get the number of adapters!\n");
		return 0;
	}

	if (0 < iNumberAdapters)
	{
		lpAdapterInfo = (LPAdapterInfo)malloc(sizeof(AdapterInfo) * iNumberAdapters);
		memset(lpAdapterInfo, '\0', sizeof(AdapterInfo) * iNumberAdapters);

		// Get the AdapterInfo structure for all adapters in the system
		ADL_Adapter_AdapterInfo_Get(lpAdapterInfo, sizeof(AdapterInfo) * iNumberAdapters);
	}

	// Looking for first present and active adapter in the system
	int adapterId = -1;
	 for ( i = 0; i < iNumberAdapters; i++ )
        {
			int adapterActive = 0;
			AdapterInfo adapterInfo = lpAdapterInfo[ i ];
			ADL_Adapter_Active_Get(adapterInfo.iAdapterIndex , &adapterActive);
            if (adapterActive)
            {
				adapterId = adapterInfo.iAdapterIndex;
				//Overdrive 5 APIs should be used if returned version indicates 5. Overdrive 6 APIs are used if 6 is returned.
                //Overdrive 5 is supported on legacy ASICs. Newer ASICs (CIK+) should report Overdrive 6
                if ( ADL_OK != ADL_Overdrive_Caps (adapterId, &iOverdriveSupported, &iOverdriveEnabled, &iOverdriveVersion) )
                    {
                        printf("Can’t get Overdrive capabilities \n");
                        //return 0;
                    }
				if (iOverdriveVersion == 6)
                   Overdrive6get (adapterId, hDLL);
                else
                    Overdrive5get (adapterId, hDLL);
                printf("ID = %x, Present = %x, Name = %s, AdapterName = %s, DisplayName=%s\n", adapterId,adapterInfo.iPresent,adapterInfo.strAdapterName, adapterInfo.strXScreenConfigName,adapterInfo.strDisplayName);
            }

		}

		if (-1 == adapterId)
		{
		   printf("Cannot find active AMD adapter\n");
		   return 0;
		}





}


