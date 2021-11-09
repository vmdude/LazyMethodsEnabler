/* ************************
 * Copyright 2013 vmdude.fr
 * ***********************/

/*
 * LazyMethodsEnabler.cpp --
 *
 *      Re-enable method disabled by crappy backup program...
 */


#include <iostream>
#include <string>
#include <cstdio>
#include <windows.h>
#include "vixDiskLib.h"

using std::cout;
using std::string;

#define VIXDISKLIB_VERSION_MAJOR 5
#define VIXDISKLIB_VERSION_MINOR 0

static struct {
    char *host;
    char *userName;
    char *password;
	bool disableMethods;
    int port;
    VixDiskLibConnection connection;
    char *vmxSpec;
    char *libdir;
} appGlobals;

static int ParseArguments(int argc, char* argv[]);

#define CHECK_AND_THROW(vixError)                                    \
   do {                                                              \
      if (VIX_FAILED((vixError))) {                                  \
         throw VixDiskLibErrWrapper((vixError), __FILE__, __LINE__); \
      }                                                              \
   } while (0)


typedef void (VixDiskLibGenericLogFunc)(const char *fmt, va_list args);


/*
 *--------------------------------------------------------------------------
 *
 * VixDiskLibErrWrapper --
 *
 *      Wrapper class for VixDiskLib disk objects.
 *
 *--------------------------------------------------------------------------
 */

class VixDiskLibErrWrapper {
public:
    explicit VixDiskLibErrWrapper(VixError errCode, const char* file, int line)
          :
          _errCode(errCode),
          _file(file),
          _line(line)
    {
        char* msg = VixDiskLib_GetErrorText(errCode, NULL);
        _desc = msg;
        VixDiskLib_FreeErrorText(msg);
    }

    VixDiskLibErrWrapper(const char* description, const char* file, int line)
          :
         _errCode(VIX_E_FAIL),
         _desc(description),
         _file(file),
         _line(line)
    {
    }

    string Description() const { return _desc; }
    VixError ErrorCode() const { return _errCode; }
    string File() const { return _file; }
    int Line() const { return _line; }

private:
    VixError _errCode;
    string _desc;
    string _file;
    int _line;
};


/*
 *--------------------------------------------------------------------------
 *
 * GetConsoleTextAttribute --
 *
 *      Return current console attributes.
 *
 *--------------------------------------------------------------------------
 */

WORD GetConsoleTextAttribute (HANDLE hCon) {
	CONSOLE_SCREEN_BUFFER_INFO con_info;
	GetConsoleScreenBufferInfo(hCon, &con_info);
	return con_info.wAttributes;
}


/*
 *--------------------------------------------------------------------------
 *
 * PrintUsage --
 *
 *      Displays the usage message.
 *
 *--------------------------------------------------------------------------
 */

static int PrintUsage(void) {
	HANDLE  hConsole;
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	const int saved_colors = GetConsoleTextAttribute(hConsole);
    printf("\n\tUsage: lazyMethodsEnabler.exe -host [ESXi or VCENTER IP/FQDN] -user [USERNAME] -password [PASSWORD] -vm [VM IDENTIFIER]\n");
    printf("\tThe VM IDENTIFIER must have the following format \"moid=<moref>\"\n");
	printf("\tExample:\n");
	printf("\t\tlazyMethodsEnabler.exe -host 10.69.69.69 -user adminvcenter -password \"esxi4ever\" -vm \"moid=vm-69\"\n\n");
	SetConsoleTextAttribute(hConsole, 14);
	printf("[DEBUG] You can add the -disableMethods argument for debuguing purpose as it will force methods to be disabled.\n\n");
	SetConsoleTextAttribute(hConsole, saved_colors);
    return 1;
}


/*
 *--------------------------------------------------------------------------
 *
 * main --
 *
 *      Main routine of the program.
 *
 *--------------------------------------------------------------------------
 */

int main(int argc, char* argv[]) {
    int retval;
    bool bVixInit(false);
	
	HANDLE  hConsole;
	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	const int saved_colors = GetConsoleTextAttribute(hConsole);

    memset(&appGlobals, 0, sizeof appGlobals);
    appGlobals.disableMethods = FALSE;

    retval = ParseArguments(argc, argv);
    if (retval) {
        return retval;
    }

    VixDiskLibConnectParams cnxParams = {0};
    VixError vixError;
    try {
		cnxParams.vmxSpec = appGlobals.vmxSpec;
		cnxParams.serverName = appGlobals.host;
		cnxParams.credType = VIXDISKLIB_CRED_UID;
		cnxParams.creds.uid.userName = appGlobals.userName;
		cnxParams.creds.uid.password = appGlobals.password;
		cnxParams.port = appGlobals.port;

		vixError = VixDiskLib_Init(VIXDISKLIB_VERSION_MAJOR,
										VIXDISKLIB_VERSION_MINOR,
										NULL, NULL, NULL, // Log, warn, panic
										appGlobals.libdir);
		CHECK_AND_THROW(vixError);
		bVixInit = true;

		vixError = VixDiskLib_PrepareForAccess(&cnxParams, "lazyMethodsEnabler");
		vixError = VixDiskLib_Connect(&cnxParams,
										&appGlobals.connection);
		CHECK_AND_THROW(vixError);
        retval = 0;
    } catch (const VixDiskLibErrWrapper& e) {
       cout << "Error: [" << e.File() << ":" << e.Line() << "]  " <<
               std::hex << e.ErrorCode() << " " << e.Description() << "\n";
       retval = 1;
    }
	
	if (!appGlobals.disableMethods) {
       vixError = VixDiskLib_EndAccess(&cnxParams, "lazyMethodsEnabler");
	   SetConsoleTextAttribute(hConsole, saved_colors);
	   printf("Methods have been re-enabled, succeed !\n");
    } else {
	   SetConsoleTextAttribute(hConsole, 12);
	   printf("[DEBUG] Methods have been disabled, you should think to re-enable it later !\n");
	   SetConsoleTextAttribute(hConsole, saved_colors);
	}

    if (appGlobals.connection != NULL) {
       VixDiskLib_Disconnect(appGlobals.connection);
    }

    if (bVixInit) {
       VixDiskLib_Exit();
    }
    return retval;
}


/*
 *--------------------------------------------------------------------------
 *
 * ParseArguments --
 *
 *      Parses the arguments passed on the command line.
 *
 *--------------------------------------------------------------------------
 */

static int ParseArguments(int argc, char* argv[]) {
    int i;
    if (argc < 8) { return PrintUsage(); }
    for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-host")) {
            if (i >= argc - 1) { return PrintUsage(); }
            appGlobals.host = argv[++i];
        } else if (!strcmp(argv[i], "-user")) {
            if (i >= argc - 1) { return PrintUsage(); }
            appGlobals.userName = argv[++i];
        } else if (!strcmp(argv[i], "-password")) {
            if (i >= argc - 1) { return PrintUsage(); }
            appGlobals.password = argv[++i];
        } else if (!strcmp(argv[i], "-vm")) {
            if (i >= argc - 1) { return PrintUsage(); }
            appGlobals.vmxSpec = argv[++i];
        } else if (!strcmp(argv[i], "-disableMethods")) {
            appGlobals.disableMethods = TRUE;
		} else { 
           return PrintUsage();
        }
    }

	appGlobals.port = 902;

    if (appGlobals.host == NULL || appGlobals.userName == NULL || appGlobals.password == NULL) {
        return PrintUsage();
    }
    return 0;
}
