/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/*
	Random portability stuff

	See comments in porting.h
*/

#include "porting.h"

#if defined(__FreeBSD__)  || defined(__NetBSD__) || defined(__DragonFly__)
	#include <sys/types.h>
	#include <sys/sysctl.h>
#elif defined(_WIN32)
	#include <windows.h>
	#include <wincrypt.h>
	#include <algorithm>
	#include <shlwapi.h>
#endif
#if !defined(_WIN32)
	#include <unistd.h>
	#include <sys/utsname.h>
#endif
#if defined(__hpux)
	#define _PSTAT64
	#include <sys/pstat.h>
#endif

#include "config.h"
#include "debug.h"
#include "filesys.h"
#include "log.h"
#include "util/string.h"
#include "settings.h"
#include <list>

namespace porting
{

/*
	Signal handler (grabs Ctrl-C on POSIX systems)
*/

bool g_killed = false;

bool *signal_handler_killstatus()
{
	return &g_killed;
}

#if !defined(_WIN32) // POSIX
	#include <signal.h>

void signal_handler(int sig)
{
	if (!g_killed) {
		if (sig == SIGINT) {
			dstream << "INFO: signal_handler(): "
				<< "Ctrl-C pressed, shutting down." << std::endl;
		} else if (sig == SIGTERM) {
			dstream << "INFO: signal_handler(): "
				<< "got SIGTERM, shutting down." << std::endl;
		}

		// Comment out for less clutter when testing scripts
		/*dstream << "INFO: sigint_handler(): "
				<< "Printing debug stacks" << std::endl;
		debug_stacks_print();*/

		g_killed = true;
	} else {
		(void)signal(sig, SIG_DFL);
	}
}

void signal_handler_init(void)
{
	(void)signal(SIGINT, signal_handler);
	(void)signal(SIGTERM, signal_handler);
}

#else // _WIN32
	#include <signal.h>

BOOL WINAPI event_handler(DWORD sig)
{
	switch (sig) {
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		if (!g_killed) {
			dstream << "INFO: event_handler(): "
				<< "Ctrl+C, Close Event, Logoff Event or Shutdown Event,"
				" shutting down." << std::endl;
			g_killed = true;
		} else {
			(void)signal(SIGINT, SIG_DFL);
		}
		break;
	case CTRL_BREAK_EVENT:
		break;
	}

	return TRUE;
}

void signal_handler_init(void)
{
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)event_handler, TRUE);
}

#endif


/*
	Path mangler
*/

// Default to RUN_IN_PLACE style relative paths
std::string path_share = "..";
std::string path_user = "..";
std::string path_locale = path_share + DIR_DELIM + "locale";
std::string path_cache = path_user + DIR_DELIM + "cache";


std::string getDataPath(const char *subpath)
{
	return path_share + DIR_DELIM + subpath;
}

void pathRemoveFile(char *path, char delim)
{
	// Remove filename and path delimiter
	int i;
	for(i = strlen(path)-1; i>=0; i--)
	{
		if(path[i] == delim)
			break;
	}
	path[i] = 0;
}

bool detectMSVCBuildDir(const std::string &path)
{
	const char *ends[] = {
		"bin\\Release",
		"bin\\MinSizeRel",
		"bin\\RelWithDebInfo",
		"bin\\Debug",
		"bin\\Build",
		NULL
	};
	return (!removeStringEnd(path, ends).empty());
}

std::string get_sysinfo()
{
#ifdef _WIN32

	std::ostringstream oss;
	LPSTR filePath = new char[MAX_PATH];
	UINT blockSize;
	VS_FIXEDFILEINFO *fixedFileInfo;

	GetSystemDirectoryA(filePath, MAX_PATH);
	PathAppendA(filePath, "kernel32.dll");

	DWORD dwVersionSize = GetFileVersionInfoSizeA(filePath, NULL);
	LPBYTE lpVersionInfo = new BYTE[dwVersionSize];

	GetFileVersionInfoA(filePath, 0, dwVersionSize, lpVersionInfo);
	VerQueryValueA(lpVersionInfo, "\\", (LPVOID *)&fixedFileInfo, &blockSize);

	oss << "Windows/"
		<< HIWORD(fixedFileInfo->dwProductVersionMS) << '.' // Major
		<< LOWORD(fixedFileInfo->dwProductVersionMS) << '.' // Minor
		<< HIWORD(fixedFileInfo->dwProductVersionLS) << ' '; // Build

	#ifdef _WIN64
	oss << "x86_64";
	#else
	BOOL is64 = FALSE;
	if (IsWow64Process(GetCurrentProcess(), &is64) && is64)
		oss << "x86_64"; // 32-bit app on 64-bit OS
	else
		oss << "x86";
	#endif

	delete[] lpVersionInfo;
	delete[] filePath;

	return oss.str();
#else
	struct utsname osinfo;
	uname(&osinfo);
	return std::string(osinfo.sysname) + "/"
		+ osinfo.release + " " + osinfo.machine;
#endif
}


bool getCurrentWorkingDir(char *buf, size_t len)
{
#ifdef _WIN32
	DWORD ret = GetCurrentDirectory(len, buf);
	return (ret != 0) && (ret <= len);
#else
	return getcwd(buf, len);
#endif
}


bool getExecPathFromProcfs(char *buf, size_t buflen)
{
#ifndef _WIN32
	buflen--;

	ssize_t len;
	if ((len = readlink("/proc/self/exe",     buf, buflen)) == -1 &&
		(len = readlink("/proc/curproc/file", buf, buflen)) == -1 &&
		(len = readlink("/proc/curproc/exe",  buf, buflen)) == -1)
		return false;

	buf[len] = '\0';
	return true;
#else
	return false;
#endif
}

//// Windows
#if defined(_WIN32)

bool getCurrentExecPath(char *buf, size_t len)
{
	DWORD written = GetModuleFileNameA(NULL, buf, len);
	if (written == 0 || written == len)
		return false;

	return true;
}


//// Linux
#elif defined(__linux__)

bool getCurrentExecPath(char *buf, size_t len)
{
	if (!getExecPathFromProcfs(buf, len))
		return false;

	return true;
}


//// Mac OS X, Darwin
#elif defined(__APPLE__)

bool getCurrentExecPath(char *buf, size_t len)
{
	uint32_t lenb = (uint32_t)len;
	if (_NSGetExecutablePath(buf, &lenb) == -1)
		return false;

	return true;
}


//// FreeBSD, NetBSD, DragonFlyBSD
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__DragonFly__)

bool getCurrentExecPath(char *buf, size_t len)
{
	// Try getting path from procfs first, since valgrind
	// doesn't work with the latter
	if (getExecPathFromProcfs(buf, len))
		return true;

	int mib[4];

	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_PATHNAME;
	mib[3] = -1;

	if (sysctl(mib, 4, buf, &len, NULL, 0) == -1)
		return false;

	return true;
}


//// Solaris
#elif defined(__sun) || defined(sun)

bool getCurrentExecPath(char *buf, size_t len)
{
	const char *exec = getexecname();
	if (exec == NULL)
		return false;

	if (strlcpy(buf, exec, len) >= len)
		return false;

	return true;
}


// HP-UX
#elif defined(__hpux)

bool getCurrentExecPath(char *buf, size_t len)
{
	struct pst_status psts;

	if (pstat_getproc(&psts, sizeof(psts), 0, getpid()) == -1)
		return false;

	if (pstat_getpathname(buf, len, &psts.pst_fid_text) == -1)
		return false;

	return true;
}


#else

bool getCurrentExecPath(char *buf, size_t len)
{
	return false;
}

#endif


//// Windows
#if defined(_WIN32)

bool setSystemPaths()
{
	char buf[BUFSIZ];

	// Find path of executable and set path_share relative to it
	FATAL_ERROR_IF(!getCurrentExecPath(buf, sizeof(buf)),
		"Failed to get current executable path");
	pathRemoveFile(buf, '\\');

	// Use ".\bin\.."
	path_share = std::string(buf) + "\\..";

	// Use "C:\Documents and Settings\user\Application Data\<PROJECT_NAME>"
	DWORD len = GetEnvironmentVariable("APPDATA", buf, sizeof(buf));
	FATAL_ERROR_IF(len == 0 || len > sizeof(buf), "Failed to get APPDATA");

	path_user = std::string(buf) + DIR_DELIM + PROJECT_NAME;
	return true;
}


//// Linux
#elif defined(__linux__)

bool setSystemPaths()
{
	char buf[BUFSIZ];

	if (!getCurrentExecPath(buf, sizeof(buf))) {
#ifdef __ANDROID__
		errorstream << "Unable to read bindir "<< std::endl;
#else
		FATAL_ERROR("Unable to read bindir");
#endif
		return false;
	}

	pathRemoveFile(buf, '/');
	std::string bindir(buf);

	// Find share directory from these.
	// It is identified by containing the subdirectory "builtin".
	std::list<std::string> trylist;
	std::string static_sharedir = STATIC_SHAREDIR;
	if (!static_sharedir.empty() && static_sharedir != ".")
		trylist.push_back(static_sharedir);

	trylist.push_back(bindir + DIR_DELIM ".." DIR_DELIM "share"
		DIR_DELIM + PROJECT_NAME);
	trylist.push_back(bindir + DIR_DELIM "..");

#ifdef __ANDROID__
	trylist.push_back(path_user);
#endif

	for (std::list<std::string>::const_iterator
			i = trylist.begin(); i != trylist.end(); ++i) {
		const std::string &trypath = *i;
		if (!fs::PathExists(trypath) ||
			!fs::PathExists(trypath + DIR_DELIM + "builtin")) {
			warningstream << "system-wide share not found at \""
					<< trypath << "\""<< std::endl;
			continue;
		}

		// Warn if was not the first alternative
		if (i != trylist.begin()) {
			warningstream << "system-wide share found at \""
					<< trypath << "\"" << std::endl;
		}

		path_share = trypath;
		break;
	}

#ifndef __ANDROID__
	path_user = std::string(getenv("HOME")) + DIR_DELIM "."
		+ PROJECT_NAME;
#endif

	return true;
}


//// Mac OS X
#elif defined(__APPLE__)

bool setSystemPaths()
{
	CFBundleRef main_bundle = CFBundleGetMainBundle();
	CFURLRef resources_url = CFBundleCopyResourcesDirectoryURL(main_bundle);
	char path[PATH_MAX];
	if (CFURLGetFileSystemRepresentation(resources_url,
			TRUE, (UInt8 *)path, PATH_MAX)) {
		path_share = std::string(path);
	} else {
		warningstream << "Could not determine bundle resource path" << std::endl;
	}
	CFRelease(resources_url);

	path_user = std::string(getenv("HOME"))
		+ "/Library/Application Support/"
		+ PROJECT_NAME;
	return true;
}


#else

bool setSystemPaths()
{
	path_share = STATIC_SHAREDIR;
	path_user  = std::string(getenv("HOME")) + DIR_DELIM "."
		+ lowercase(PROJECT_NAME);
	return true;
}


#endif

void migrateCachePath()
{
	const std::string local_cache_path = path_user + DIR_DELIM + "cache";

	// Delete tmp folder if it exists (it only ever contained
	// a temporary ogg file, which is no longer used).
	if (fs::PathExists(local_cache_path + DIR_DELIM + "tmp"))
		fs::RecursiveDelete(local_cache_path + DIR_DELIM + "tmp");

	// Bail if migration impossible
	if (path_cache == local_cache_path || !fs::PathExists(local_cache_path)
			|| fs::PathExists(path_cache)) {
		return;
	}
	if (!fs::Rename(local_cache_path, path_cache)) {
		errorstream << "Failed to migrate local cache path "
			"to system path!" << std::endl;
	}
}

void initializePaths()
{
#if RUN_IN_PLACE
	char buf[BUFSIZ];

	infostream << "Using relative paths (RUN_IN_PLACE)" << std::endl;

	bool success =
		getCurrentExecPath(buf, sizeof(buf)) ||
		getExecPathFromProcfs(buf, sizeof(buf));

	if (success) {
		pathRemoveFile(buf, DIR_DELIM_CHAR);
		std::string execpath(buf);

		path_share = execpath + DIR_DELIM "..";
		path_user  = execpath + DIR_DELIM "..";

		if (detectMSVCBuildDir(execpath)) {
			path_share += DIR_DELIM "..";
			path_user  += DIR_DELIM "..";
		}
	} else {
		errorstream << "Failed to get paths by executable location, "
			"trying cwd" << std::endl;

		if (!getCurrentWorkingDir(buf, sizeof(buf)))
			FATAL_ERROR("Ran out of methods to get paths");

		size_t cwdlen = strlen(buf);
		if (cwdlen >= 1 && buf[cwdlen - 1] == DIR_DELIM_CHAR) {
			cwdlen--;
			buf[cwdlen] = '\0';
		}

		if (cwdlen >= 4 && !strcmp(buf + cwdlen - 4, DIR_DELIM "bin"))
			pathRemoveFile(buf, DIR_DELIM_CHAR);

		std::string execpath(buf);

		path_share = execpath;
		path_user  = execpath;
	}
	path_cache = path_user + DIR_DELIM + "cache";
#else
	infostream << "Using system-wide paths (NOT RUN_IN_PLACE)" << std::endl;

	if (!setSystemPaths())
		errorstream << "Failed to get one or more system-wide path" << std::endl;

	// Initialize path_cache
	// First try $XDG_CACHE_HOME/PROJECT_NAME
	const char *cache_dir = getenv("XDG_CACHE_HOME");
	const char *home_dir = getenv("HOME");
	if (cache_dir) {
		path_cache = std::string(cache_dir) + DIR_DELIM + PROJECT_NAME;
	} else if (home_dir) {
		// Then try $HOME/.cache/PROJECT_NAME
		path_cache = std::string(home_dir) + DIR_DELIM + ".cache"
			+ DIR_DELIM + PROJECT_NAME;
	} else {
		// If neither works, use $PATH_USER/cache
		path_cache = path_user + DIR_DELIM + "cache";
	}
	// Migrate cache folder to new location if possible
	migrateCachePath();
#endif

	infostream << "Detected share path: " << path_share << std::endl;
	infostream << "Detected user path: " << path_user << std::endl;
	infostream << "Detected cache path: " << path_cache << std::endl;

#if USE_GETTEXT
	bool found_localedir = false;
#  ifdef STATIC_LOCALEDIR
	if (STATIC_LOCALEDIR[0] && fs::PathExists(STATIC_LOCALEDIR)) {
		found_localedir = true;
		path_locale = STATIC_LOCALEDIR;
		infostream << "Using locale directory " << STATIC_LOCALEDIR << std::endl;
	} else {
		path_locale = getDataPath("locale");
		if (fs::PathExists(path_locale)) {
			found_localedir = true;
			infostream << "Using in-place locale directory " << path_locale
				<< " even though a static one was provided "
				<< "(RUN_IN_PLACE or CUSTOM_LOCALEDIR)." << std::endl;
		}
	}
#  else
	path_locale = getDataPath("locale");
	if (fs::PathExists(path_locale)) {
		found_localedir = true;
	}
#  endif
	if (!found_localedir) {
		warningstream << "Couldn't find a locale directory!" << std::endl;
	}
#endif  // USE_GETTEXT
}

////
//// OS-specific Secure Random
////

#ifdef WIN32

bool secure_rand_fill_buf(void *buf, size_t len)
{
	HCRYPTPROV wctx;

	if (!CryptAcquireContext(&wctx, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
		return false;

	CryptGenRandom(wctx, len, (BYTE *)buf);
	CryptReleaseContext(wctx, 0);
	return true;
}

#else

bool secure_rand_fill_buf(void *buf, size_t len)
{
	// N.B.  This function checks *only* for /dev/urandom, because on most
	// common OSes it is non-blocking, whereas /dev/random is blocking, and it
	// is exceptionally uncommon for there to be a situation where /dev/random
	// exists but /dev/urandom does not.  This guesswork is necessary since
	// random devices are not covered by any POSIX standard...
	FILE *fp = fopen("/dev/urandom", "rb");
	if (!fp)
		return false;

	bool success = fread(buf, len, 1, fp) == 1;

	fclose(fp);
	return success;
}

#endif

void attachOrCreateConsole()
{
#ifdef _WIN32
	static bool consoleAllocated = false;
	const bool redirected = (_fileno(stdout) == -2 || _fileno(stdout) == -1); // If output is redirected to e.g a file
	if (!consoleAllocated && redirected && (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole())) {
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
		consoleAllocated = true;
	}
#endif
}

// Load performance counter frequency only once at startup
#ifdef _WIN32

inline double get_perf_freq()
{
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	return freq.QuadPart;
}

double perf_freq = get_perf_freq();

#endif

} //namespace porting