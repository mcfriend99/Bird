#ifndef birdy_compart_unistd_h
#define birdy_compart_unistd_h

#include "common.h"

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__)) || defined(__MINGW32_MAJOR_VERSION) || defined(__CYGWIN__)

#include <unistd.h>

#else

#ifndef _UNISTD_H
#define _UNISTD_H 1

/* This is intended as a drop-in replacement for unistd.h on Windows.
 * Please add functionality as neeeded.
 * https://stackoverflow.com/a/826027/1202830
 */

#include <direct.h> /* for _getcwd() and _chdir() */
#include "compat/getopt.h" /* getopt at: https://gist.github.com/ashelly/7776712 */
#include <io.h>
#include <process.h> /* for getpid() and the exec..() family */
#include <stdlib.h>

#define srandom srand
#define random rand

/* Values for the second argument to access.
   These may be OR'd together.  */
#define R_OK 4 /* Test for read permission.  */
#define W_OK 2 /* Test for write permission.  */
//#define   X_OK    1       /* execute permission - unsupported in windows*/
#define F_OK 0 /* Test for existence.  */

#define access _access
#define dup2 _dup2
#define execve _execve
#define ftruncate _chsize
#define unlink _unlink
#define fileno _fileno
#define getcwd _getcwd
#define chdir _chdir
#define isatty _isatty
#define lseek _lseek
#define rmdir _rmdir
//#define truncate _truncate
/* read, write, and close are NOT being #defined here, because while there are
 * file handle specific versions for Windows, they probably don't work for
 * sockets. You need to look at your app and consider whether to call e.g.
 * closesocket(). */

#ifdef _WIN32
#define ssize_t __int64
#else
#define ssize_t long
#endif

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
/* should be in some equivalent to <sys/types.h> */
typedef __int8 int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef __int64 int64_t;
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;

#endif /* unistd.h  */

#ifdef IS_WINDOWS
#define ftruncate _chsize
#define fileno _fileno
#endif

#endif

#endif