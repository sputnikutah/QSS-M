/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2005 John Fitzgibbons and others
Copyright (C) 2007-2008 Kristian Duske
Copyright (C) 2010-2014 QuakeSpasm developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "arch_def.h"
#include "quakedef.h"

#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#if defined(PLATFORM_OSX) || defined(PLATFORM_HAIKU)
#include <libgen.h>	/* dirname() and basename() */
#endif
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#ifdef DO_USERDIRS
#include <pwd.h>
#endif

#if defined(SDL_FRAMEWORK) || defined(NO_SDL_CONFIG)
#if defined(USE_SDL2)
#include <SDL2/SDL.h>
#else
#include <SDL/SDL.h>
#endif
#else
#include "SDL.h"
#endif

#include <termios.h> // woods #arrowkeys
#include <unistd.h> // woods #arrowkeys

qboolean		isDedicated;
cvar_t		sys_throttle = {"sys_throttle", "0.02", CVAR_ARCHIVE};

static size_t	sys_handles_max;	/* spike -- removed limit, was 32 (johnfitz -- was 10) */
static FILE		**sys_handles;
static int findhandle (void)
{
	size_t i, n;

	for (i = 1; i < sys_handles_max; i++)
	{
		if (!sys_handles[i])
			return i;
	}
	n = sys_handles_max+10;
	sys_handles = realloc(sys_handles, sizeof(*sys_handles)*n);
	if (!sys_handles)
		Sys_Error ("out of handles");
	while (sys_handles_max < n)
		sys_handles[sys_handles_max++] = NULL;
	return i;
}

qofs_t Sys_filelength (FILE *f)
{
	long		pos, end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}

qofs_t Sys_FileOpenRead (const char *path, int *hndl)
{
	FILE	*f;
	int	i;
	qofs_t retval;

	i = findhandle ();
	f = fopen(path, "rb");

	if (!f)
	{
		*hndl = -1;
		retval = -1;
	}
	else
	{
		sys_handles[i] = f;
		*hndl = i;
		retval = Sys_filelength(f);
	}

	return retval;
}

int Sys_FileOpenWrite (const char *path)
{
	FILE	*f;
	int		i;

	i = findhandle ();
	f = fopen(path, "wb");

	if (!f)
		Sys_Error ("Error opening %s: %s", path, strerror(errno));

	sys_handles[i] = f;
	return i;
}

int Sys_FileOpenStdio (FILE *file)
{
	int		i;
	i = findhandle ();
	sys_handles[i] = file;
	return i;
}

void Sys_FileClose (int handle)
{
	fclose (sys_handles[handle]);
	sys_handles[handle] = NULL;
}

void Sys_FileSeek (int handle, qofs_t position)
{
	fseek (sys_handles[handle], position, SEEK_SET);
}

int Sys_FileRead (int handle, void *dest, int count)
{
	return fread (dest, 1, count, sys_handles[handle]);
}

int Sys_FileWrite (int handle, const void *data, int count)
{
	return fwrite (data, 1, count, sys_handles[handle]);
}

int Sys_FileType (const char *path)
{
	/*
	if (access(path, R_OK) == -1)
		return 0;
	*/
	struct stat	st;

	if (stat(path, &st) != 0)
		return FS_ENT_NONE;
	if (S_ISDIR(st.st_mode))
		return FS_ENT_DIRECTORY;
	if (S_ISREG(st.st_mode))
		return FS_ENT_FILE;

	return FS_ENT_NONE;
}


#if defined(__linux__) || defined(__sun) || defined(sun) || defined(_AIX)
static int Sys_NumCPUs (void)
{
	int numcpus = sysconf(_SC_NPROCESSORS_ONLN);
	return (numcpus < 1) ? 1 : numcpus;
}

#elif defined(PLATFORM_OSX)
#include <sys/sysctl.h>
#if !defined(HW_AVAILCPU)	/* using an ancient SDK? */
#define HW_AVAILCPU		25	/* needs >= 10.2 */
#endif
static int Sys_NumCPUs (void)
{
	int numcpus;
	int mib[2];
	size_t len;

#if defined(_SC_NPROCESSORS_ONLN)	/* needs >= 10.5 */
	numcpus = sysconf(_SC_NPROCESSORS_ONLN);
	if (numcpus != -1)
		return (numcpus < 1) ? 1 : numcpus;
#endif
	len = sizeof(numcpus);
	mib[0] = CTL_HW;
	mib[1] = HW_AVAILCPU;
	sysctl(mib, 2, &numcpus, &len, NULL, 0);
	if (sysctl(mib, 2, &numcpus, &len, NULL, 0) == -1)
	{
		mib[1] = HW_NCPU;
		if (sysctl(mib, 2, &numcpus, &len, NULL, 0) == -1)
			return 1;
	}
	return (numcpus < 1) ? 1 : numcpus;
}

#elif defined(__sgi) || defined(sgi) || defined(__sgi__) /* IRIX */
static int Sys_NumCPUs (void)
{
	int numcpus = sysconf(_SC_NPROC_ONLN);
	if (numcpus < 1)
		numcpus = 1;
	return numcpus;
}

#elif defined(PLATFORM_BSD)
#include <sys/sysctl.h>
static int Sys_NumCPUs (void)
{
	int numcpus;
	int mib[2];
	size_t len;

#if defined(_SC_NPROCESSORS_ONLN)
	numcpus = sysconf(_SC_NPROCESSORS_ONLN);
	if (numcpus != -1)
		return (numcpus < 1) ? 1 : numcpus;
#endif
	len = sizeof(numcpus);
	mib[0] = CTL_HW;
	mib[1] = HW_NCPU;
	if (sysctl(mib, 2, &numcpus, &len, NULL, 0) == -1)
		return 1;
	return (numcpus < 1) ? 1 : numcpus;
}

#elif defined(__hpux) || defined(__hpux__) || defined(_hpux)
#include <sys/mpctl.h>
static int Sys_NumCPUs (void)
{
	int numcpus = mpctl(MPC_GETNUMSPUS, NULL, NULL);
	return numcpus;
}

#else /* unknown OS */
static int Sys_NumCPUs (void)
{
	return -2;
}
#endif

static char	cwd[MAX_OSPATH];
#ifdef DO_USERDIRS
static char	userdir[MAX_OSPATH];
#ifdef PLATFORM_OSX
#define SYS_USERDIR	"Library/Application Support/QuakeSpasm"
#else
#define SYS_USERDIR	".quakespasm"
#endif

static void Sys_GetUserdir (char *dst, size_t dstsize)
{
	size_t		n;
	const char	*home_dir = NULL;
	struct passwd	*pwent;

	pwent = getpwuid( getuid() );
	if (pwent == NULL)
		perror("getpwuid");
	else
		home_dir = pwent->pw_dir;
	if (home_dir == NULL)
		home_dir = getenv("HOME");
	if (home_dir == NULL)
		Sys_Error ("Couldn't determine userspace directory");

/* what would be a maximum path for a file in the user's directory...
 * $HOME/SYS_USERDIR/game_dir/dirname1/dirname2/dirname3/filename.ext
 * still fits in the MAX_OSPATH == 256 definition, but just in case :
 */
	n = strlen(home_dir) + strlen(SYS_USERDIR) + 50;
	if (n >= dstsize)
		Sys_Error ("Insufficient array size for userspace directory");

	q_snprintf (dst, dstsize, "%s/%s", home_dir, SYS_USERDIR);
}
#endif	/* DO_USERDIRS */

#ifdef PLATFORM_OSX
static char *OSX_StripAppBundle (char *dir)
{ /* based on the ioquake3 project at icculus.org. */
	static char	osx_path[MAX_OSPATH];

	q_strlcpy (osx_path, dir, sizeof(osx_path));
	if (strcmp(basename(osx_path), "MacOS"))
		return dir;
	q_strlcpy (osx_path, dirname(osx_path), sizeof(osx_path));
	if (strcmp(basename(osx_path), "Contents"))
		return dir;
	q_strlcpy (osx_path, dirname(osx_path), sizeof(osx_path));
	if (!strstr(basename(osx_path), ".app"))
		return dir;
	q_strlcpy (osx_path, dirname(osx_path), sizeof(osx_path));
	return osx_path;
}

static void Sys_GetBasedir (char *argv0, char *dst, size_t dstsize)
{
	char	*tmp;

	if (realpath(argv0, dst) == NULL)
	{
		perror("realpath");
		if (getcwd(dst, dstsize - 1) == NULL)
	_fail:		Sys_Error ("Couldn't determine current directory");
	}
	else
	{
		/* strip off the binary name */
		if (! (tmp = strdup (dst))) goto _fail;
		q_strlcpy (dst, dirname(tmp), dstsize);
		free (tmp);
	}

	tmp = OSX_StripAppBundle(dst);
	if (tmp != dst)
		q_strlcpy (dst, tmp, dstsize);
}
#else
static void Sys_GetBasedir (char *argv0, char *dst, size_t dstsize)
{
	char	*tmp;

	#ifdef PLATFORM_HAIKU
	if (realpath(argv0, dst) == NULL)
	{
		perror("realpath");
		if (getcwd(dst, dstsize - 1) == NULL)
	_fail:		Sys_Error ("Couldn't determine current directory");
	}
	else
	{
		/* strip off the binary name */
		if (! (tmp = strdup (dst))) goto _fail;
		q_strlcpy (dst, dirname(tmp), dstsize);
		free (tmp);
	}
	#else
	if (getcwd(dst, dstsize - 1) == NULL)
		Sys_Error ("Couldn't determine current directory");

	tmp = dst;
	while (*tmp != 0)
		tmp++;
	while (*tmp == 0 && tmp != dst)
	{
		--tmp;
		if (tmp != dst && *tmp == '/')
			*tmp = 0;
	}
	#endif
}
#endif

void Sys_Init (void)
{
	memset (cwd, 0, sizeof(cwd));
	Sys_GetBasedir(host_parms->argv[0], cwd, sizeof(cwd));
	host_parms->basedir = cwd;
#ifndef DO_USERDIRS
	host_parms->userdir = host_parms->basedir; /* code elsewhere relies on this ! */
#else
	if (COM_CheckParm("-nohome"))
		host_parms->userdir = host_parms->basedir;
	else
	{
		memset (userdir, 0, sizeof(userdir));
		Sys_GetUserdir(userdir, sizeof(userdir));
		Sys_mkdir (userdir);
		host_parms->userdir = userdir;
	}
#endif
	host_parms->numcpus = Sys_NumCPUs ();
	Sys_Printf("Detected %d CPUs.\n", host_parms->numcpus);
}

void Sys_mkdir (const char *path)
{
	int rc = mkdir (path, 0777);
	if (rc != 0 && errno == EEXIST)
	{
		struct stat st;
		if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
			rc = 0;
	}
	if (rc != 0)
	{
		rc = errno;
		Sys_Error("Unable to create directory %s: %s", path, strerror(rc));
	}
}

static const char errortxt1[] = "\nERROR-OUT BEGIN\n\n";
static const char errortxt2[] = "\nQUAKE ERROR: ";

void Sys_Error (const char *error, ...)
{
	va_list		argptr;
	char		text[1024];

	host_parms->errstate++;

	va_start (argptr, error);
	q_vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	fputs (errortxt1, stderr);
	Con_Redirect(NULL);
	PR_SwitchQCVM(NULL);
	Host_Shutdown ();
	fputs (errortxt2, stderr);
	fputs (text, stderr);
	fputs ("\n\n", stderr);
	if (!isDedicated)
		PL_ErrorDialog(text);

	exit (1);
}

void Sys_Printf (const char *fmt, ...)
{
	va_list argptr;
	char text[1024];

	va_start(argptr, fmt);
	q_vsnprintf (text, sizeof (text), fmt, argptr);
	va_end(argptr);

	unsigned char* ch = (unsigned char*)text;
	unsigned char* dst = (unsigned char*)text;

	while (*ch)
	{
		if (*ch == '^' && *(ch + 1) != '\0' && *(ch + 1) == 'm')
		{
			ch += 2; // Skip over '^' and 'm'
			continue;
		}
		*dst = dequake[*ch];
		dst++;
		ch++;
	}
	*dst = '\0';

	printf ("%s", text);
}

void Sys_Quit (void)
{
	Host_Shutdown();

	exit (0);
}

double Sys_DoubleTime (void)
{
	return SDL_GetTicks() / 1000.0;
}

static void safe_write(int fd, const void* buf, size_t count) // woods #arrowkeys
{
	ssize_t result = write(fd, buf, count);
	if (result == -1) {
	}
}

const char *Sys_ConsoleInput (void) // woods #arrowkeys
{
	static char	con_text[256];
	static int	textlen;
    static int cursor_pos;  // Track cursor position separately from text length
	char		c;
	fd_set		set;
	struct timeval	timeout;
    static struct termios orig_termios, raw_termios;
    static qboolean term_setup = false;

    // Set up terminal once
    if (!term_setup)
    {
        if (tcgetattr(0, &orig_termios) != -1)
        {
            raw_termios = orig_termios;
            raw_termios.c_lflag &= ~(ICANON | ECHO);  // Disable canonical mode and echo
            raw_termios.c_cc[VMIN] = 1;
            raw_termios.c_cc[VTIME] = 0;
            tcsetattr(0, TCSANOW, &raw_termios);
            term_setup = true;
            cursor_pos = 0;
        }
    }

    FD_ZERO (&set);
	FD_SET (0, &set);	// stdin
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    while (select (1, &set, NULL, NULL, &timeout))
    {
        ssize_t len = read(0, &c, 1);
        if (len != 1)
            continue;

        // Handle escape sequences for arrow keys
        if (c == 27) // ESC character
        {
            char seq[3] = {0};
            struct timeval seq_timeout;
            seq_timeout.tv_sec = 0;
            seq_timeout.tv_usec = 10000;
            
            FD_ZERO(&set);
            FD_SET(0, &set);
            if (select(1, &set, NULL, NULL, &seq_timeout) > 0)
            {
                len = read(0, seq, 1);
                if (len == 1 && seq[0] == '[')
                {
                    FD_ZERO(&set);
                    FD_SET(0, &set);
                    if (select(1, &set, NULL, NULL, &seq_timeout) > 0)
                    {
                        len = read(0, seq + 1, 1);
                        if (len == 1)
                        {
                            switch (seq[1])
                            {
                                case 'D': // Left arrow
                                    if (cursor_pos > 0)
                                    {
                                        cursor_pos--;
										safe_write(1, "\b", 1);
                                    }
                                    continue;
                                case 'C': // Right arrow
                                    if (cursor_pos < textlen)
                                    {
										safe_write(1, &con_text[cursor_pos], 1);
                                        cursor_pos++;
                                    }
                                    continue;
                                case 'A': // Up arrow
                                case 'B': // Down arrow
                                    continue;
                            }
                        }
                    }
                }
            }
            continue;
        }

        if (c == '\n' || c == '\r')
        {
			safe_write(1, "\n", 1);
            con_text[textlen] = '\0';
            textlen = 0;
            cursor_pos = 0;
            return con_text;
        }
        else if (c == 8 || c == 127)    // backspace or delete
        {
            if (cursor_pos > 0)
            {
                // Move characters after cursor back by one position
                memmove(&con_text[cursor_pos - 1], &con_text[cursor_pos], textlen - cursor_pos);
                cursor_pos--;
                textlen--;
                
                // Rewrite the line from cursor position
				safe_write(1, "\b", 1);
                if (cursor_pos < textlen)
                {
					safe_write(1, &con_text[cursor_pos], textlen - cursor_pos);
					safe_write(1, " ", 1);  // Clear last character
                    // Move cursor back to position
                    for (int i = 0; i < textlen - cursor_pos + 1; i++)
						safe_write(1, "\b", 1);
                }
                else
                {
					safe_write(1, " \b", 2);  // Clear last character
                }
            }
            continue;
        }

        if (textlen < sizeof(con_text)-1 && c >= 32 && c < 127)
        {
            // Insert character at cursor position
            if (cursor_pos < textlen)
            {
                // Make room for new character
                memmove(&con_text[cursor_pos + 1], &con_text[cursor_pos], textlen - cursor_pos);
                con_text[cursor_pos] = c;
                textlen++;
                
                // Write the new character and the rest of the line
				safe_write(1, &con_text[cursor_pos], textlen - cursor_pos);
                
                // Move cursor back to just after inserted character
                cursor_pos++;
                for (int i = 0; i < textlen - cursor_pos; i++)
					safe_write(1, "\b", 1);
            }
            else
            {
                // Append character at end of line
                con_text[textlen] = c;
				safe_write(1, &c, 1);
                textlen++;
                cursor_pos++;
            }
        }
    }

    return NULL;
}

void Sys_Sleep (unsigned long msecs)
{
/*	usleep (msecs * 1000);*/
	SDL_Delay (msecs);
}

void Sys_SendKeyEvents (void)
{
	IN_Commands();		//ericw -- allow joysticks to add keys so they can be used to confirm SCR_ModalMessage
	IN_SendKeyEvents();
}

#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <ImageIO/ImageIO.h>

void Sys_Image_BGRA_To_Clipboard(byte* buffer, int width, int height, int buffersize) // woods #screenshotcopy
{
	CGDataProviderRef provider = CGDataProviderCreateWithData(NULL, buffer, buffersize, NULL);

	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();

	CGImageRef imageRef = CGImageCreate(
		width,
		height,
		8,                      // bits per component
		32,                     // bits per pixel
		width * 4,              // bytes per row
		colorSpace,
		kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst,
		provider,
		NULL,
		false,
		kCGRenderingIntentDefault
	);

	// Create an image destination to hold the image data
	CFMutableDataRef imageData = CFDataCreateMutable(NULL, 0);
	CGImageDestinationRef destination = CGImageDestinationCreateWithData(imageData, kUTTypeTIFF, 1, NULL);
	if (!destination) {
		// Handle error
		CGImageRelease(imageRef);
		CGColorSpaceRelease(colorSpace);
		CGDataProviderRelease(provider);
		CFRelease(imageData);
		return;
	}

	CGImageDestinationAddImage(destination, imageRef, NULL);

	if (!CGImageDestinationFinalize(destination)) {
		// Handle error
		CFRelease(destination);
		CGImageRelease(imageRef);
		CGColorSpaceRelease(colorSpace);
		CGDataProviderRelease(provider);
		CFRelease(imageData);
		return;
	}

	// Get the pasteboard
	PasteboardRef pasteboard;
	OSStatus status = PasteboardCreate(kPasteboardClipboard, &pasteboard);
	if (status != noErr) {
		// Handle error
		CFRelease(destination);
		CGImageRelease(imageRef);
		CGColorSpaceRelease(colorSpace);
		CGDataProviderRelease(provider);
		CFRelease(imageData);
		return;
	}

	// Clear the pasteboard
	status = PasteboardClear(pasteboard);
	if (status != noErr) {
		// Handle error
		CFRelease(pasteboard);
		CFRelease(destination);
		CGImageRelease(imageRef);
		CGColorSpaceRelease(colorSpace);
		CGDataProviderRelease(provider);
		CFRelease(imageData);
		return;
	}

	// Put the image data onto the pasteboard
	PasteboardSynchronize(pasteboard);
	status = PasteboardPutItemFlavor(pasteboard, (PasteboardItemID)1, CFSTR("public.tiff"), imageData, 0);
	if (status != noErr) {
		// Handle error
	}

	// Release resources
	CFRelease(pasteboard);
	CFRelease(destination);
	CGImageRelease(imageRef);
	CGColorSpaceRelease(colorSpace);
	CGDataProviderRelease(provider);
	CFRelease(imageData);
}
#endif