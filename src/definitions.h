
#ifndef __MANUS_DEFINITIONS_H
#define __MANUS_DEFINITIONS_H


#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>  // For chdir()
    #include <winsvc.h>
    #include <shlobj.h>
    #include <cmath>

    #ifndef PATH_MAX
    #define PATH_MAX MAX_PATH
    #endif

    #ifndef S_ISDIR
    #define S_ISDIR(x) ((x) & _S_IFDIR)
    #endif

    #define DIRSEP '\\'
    #define snprintf _snprintf
    #define vsnprintf _vsnprintf
    #define sleep(x) Sleep((x))
    #define abs_path(rel, abs, abs_size) _fullpath((abs), (rel), (abs_size))
    #define SIGCHLD 0
    typedef struct _stat file_stat_t;
    #define stat(x, y) _stat((x), (y))
#else
    typedef struct stat file_stat_t;
    #include <sys/wait.h>
    #include <unistd.h>

    #define DIRSEP '/'
    #define __cdecl
    #define abs_path(rel, abs, abs_size) realpath((rel), (abs))
	#define sleep(x) usleep((x) * 1000)
#endif // _WIN32

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#endif
