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

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h>

#include "quakedef.h"

#include <sys/types.h>
#include <errno.h>
#include <io.h>
#include <direct.h>

#if defined(SDL_FRAMEWORK) || defined(NO_SDL_CONFIG)
#if defined(USE_SDL2)
#include <SDL2/SDL.h>
#else
#include <SDL/SDL.h>
#endif
#else
#include "SDL.h"
#endif


qboolean		isDedicated;
qboolean	Win95, Win95old, WinNT, WinVista;
cvar_t		sys_throttle = {"sys_throttle", "0.02", CVAR_ARCHIVE};

static HANDLE		hinput, houtput;

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

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES	((DWORD)-1)
#endif
int Sys_FileType (const char *path)
{
	DWORD result = GetFileAttributes(path);

	if (result == INVALID_FILE_ATTRIBUTES)
		return FS_ENT_NONE;
	if (result & FILE_ATTRIBUTE_DIRECTORY)
		return FS_ENT_DIRECTORY;

	return FS_ENT_FILE;
}

static char	cwd[1024];

static void Sys_GetBasedir (char *argv0, char *dst, size_t dstsize)
{
	char *tmp;
	size_t rc;

	rc = GetCurrentDirectory(dstsize, dst);
	if (rc == 0 || rc > dstsize)
		Sys_Error ("Couldn't determine current directory");

	tmp = dst;
	while (*tmp != 0)
		tmp++;
	while (*tmp == 0 && tmp != dst)
	{
		--tmp;
		if (tmp != dst && (*tmp == '/' || *tmp == '\\'))
			*tmp = 0;
	}
}

typedef enum { dpi_unaware = 0, dpi_system_aware = 1, dpi_monitor_aware = 2 } dpi_awareness;
typedef BOOL (WINAPI *SetProcessDPIAwareFunc)();
typedef HRESULT (WINAPI *SetProcessDPIAwarenessFunc)(dpi_awareness value);

static void Sys_SetDPIAware (void)
{
	HMODULE hUser32, hShcore;
	SetProcessDPIAwarenessFunc setDPIAwareness;
	SetProcessDPIAwareFunc setDPIAware;

	/* Neither SDL 1.2 nor SDL 2.0.3 can handle the OS scaling our window.
	  (e.g. https://bugzilla.libsdl.org/show_bug.cgi?id=2713)
	  Call SetProcessDpiAwareness/SetProcessDPIAware to opt out of scaling.
	*/

	hShcore = LoadLibraryA ("Shcore.dll");
	hUser32 = LoadLibraryA ("user32.dll");
	setDPIAwareness = (SetProcessDPIAwarenessFunc) (hShcore ? GetProcAddress (hShcore, "SetProcessDpiAwareness") : NULL);
	setDPIAware = (SetProcessDPIAwareFunc) (hUser32 ? GetProcAddress (hUser32, "SetProcessDPIAware") : NULL);

	if (setDPIAwareness) /* Windows 8.1+ */
		setDPIAwareness (dpi_monitor_aware);
	else if (setDPIAware) /* Windows Vista-8.0 */
		setDPIAware ();

	if (hShcore)
		FreeLibrary (hShcore);
	if (hUser32)
		FreeLibrary (hUser32);
}

static void Sys_SetTimerResolution(void)
{
	/* Set OS timer resolution to 1ms.
	   Works around buffer underruns with directsound and SDL2, but also
	   will make Sleep()/SDL_Dleay() accurate to 1ms which should help framerate
	   stability.
	*/
	timeBeginPeriod (1);
}

// woods -- https://github.com/andrei-drexler/ironwail/issues/104 disable CAPSLOCK #disablecaps

#if defined(_WIN32) // woods #disablecaps via ironwail
static HHOOK key_hook = NULL;

#define HOOKED_KEYS			\
	HOOK_KEY (CAPSLOCK)		\
	HOOK_KEY (APPLICATION)	\



#define SC_CAPSLOCK			0x3A
#define SC_APPLICATION		0xE05B // windows key

enum
{
#define HOOK_KEY(k)		HK_##k,
	HOOKED_KEYS
#undef HOOK_KEY

	HK_COUNT,
};

static const SDL_Scancode hk_sdl_scancodes[HK_COUNT] =
{
	#define HOOK_KEY(k)		SDL_SCANCODE_##k,
	HOOKED_KEYS
	#undef HOOK_KEY
};

static int GetFilteredKeyIndex(int scancode)
{
	switch (scancode)
	{
#define HOOK_KEY(k)	case SC_##k: return HK_##k;
		HOOKED_KEYS
#undef HOOK_KEY
	default:
		return -1;
	}
}

LRESULT CALLBACK KeyFilter(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0 && VID_HasMouseOrInputFocus())
	{
		PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;
		int scancode = p->scanCode | (p->flags & 1 ? 0xE000 : 0);
		int key = GetFilteredKeyIndex(scancode);
		if (key != -1)
		{
			// Note: if we intercept a key down message,
			// we also need to intercept the corresponding key up.
			static uint32_t pending_mask = 0;

			qboolean force_intercept = (pending_mask >> key) & 1;
			qboolean down = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
			qboolean intercept =
				force_intercept ||
				(key_dest == key_game || M_KeyBinding())
				;

			if (intercept)
			{
				SDL_Event ev;
				if (down)
					pending_mask |= (1 << key);
				else
					pending_mask &= ~(1 << key);
				memset(&ev, 0, sizeof(ev));
				ev.type = down ? SDL_KEYDOWN : SDL_KEYUP;
				ev.key.state = down ? SDL_PRESSED : SDL_RELEASED;
				ev.key.keysym.scancode = hk_sdl_scancodes[key];
				SDL_PushEvent(&ev);
				return 1;
			}
		}
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}
#endif

void Sys_Init (void)
{
	OSVERSIONINFO	vinfo;

	Sys_SetTimerResolution ();
	Sys_SetDPIAware ();

	memset (cwd, 0, sizeof(cwd));
	Sys_GetBasedir(NULL, cwd, sizeof(cwd));
	host_parms->basedir = cwd;

	/* userdirs not really necessary for windows guys.
	 * can be done if necessary, though... */
	host_parms->userdir = host_parms->basedir; /* code elsewhere relies on this ! */

	vinfo.dwOSVersionInfoSize = sizeof(vinfo);

	if (!GetVersionEx (&vinfo))
		Sys_Error ("Couldn't get OS info");

	if ((vinfo.dwMajorVersion < 4) ||
		(vinfo.dwPlatformId == VER_PLATFORM_WIN32s))
	{
		Sys_Error ("QuakeSpasm requires at least Win95 or NT 4.0");
	}

	if (vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		SYSTEM_INFO info;
		WinNT = true;
		if (vinfo.dwMajorVersion >= 6)
			WinVista = true;
		GetSystemInfo(&info);
		host_parms->numcpus = info.dwNumberOfProcessors;
		if (host_parms->numcpus < 1)
			host_parms->numcpus = 1;
	}
	else
	{
		WinNT = false; /* Win9x or WinME */
		host_parms->numcpus = 1;
		if ((vinfo.dwMajorVersion == 4) && (vinfo.dwMinorVersion == 0))
		{
			Win95 = true;
			/* Win95-gold or Win95A can't switch bpp automatically */
			if (vinfo.szCSDVersion[1] != 'C' && vinfo.szCSDVersion[1] != 'B')
				Win95old = true;
		}
	}
	Sys_Printf("Detected %d CPUs.\n", host_parms->numcpus);

	if (isDedicated)
	{
		if (!AllocConsole ())
		{
			isDedicated = false;	/* so that we have a graphical error dialog */
			Sys_Error ("Couldn't create dedicated server console");
		}

		hinput = GetStdHandle (STD_INPUT_HANDLE);
		houtput = GetStdHandle (STD_OUTPUT_HANDLE);
	}

	else
	{
		key_hook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyFilter, GetModuleHandleW(NULL), 0);
		if (!key_hook)
			Sys_Printf("Warning: SetWindowsHookExW failed (%ld)\n", GetLastError());
	}

}

void Sys_mkdir (const char *path)
{
	if (CreateDirectory(path, NULL) != 0)
		return;
	if (GetLastError() != ERROR_ALREADY_EXISTS)
		Sys_Error("Unable to create directory %s", path);
}

static const char errortxt1[] = "\nERROR-OUT BEGIN\n\n";
static const char errortxt2[] = "\nQUAKE ERROR: ";

void Sys_Error (const char *error, ...)
{
	va_list		argptr;
	char		text[1024];
	DWORD		dummy;

	host_parms->errstate++;

	va_start (argptr, error);
	q_vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	PR_SwitchQCVM(NULL);

	Con_Redirect(NULL);

	if (isDedicated)
		WriteFile (houtput, errortxt1, strlen(errortxt1), &dummy, NULL);
	/* SDL will put these into its own stderr log,
	   so print to stderr even in graphical mode. */
	fputs (errortxt1, stderr);
	Host_Shutdown ();
	fputs (errortxt2, stderr);
	fputs (text, stderr);
	fputs ("\n\n", stderr);
	if (!isDedicated)
		PL_ErrorDialog(text);
	else
	{
		WriteFile (houtput, errortxt2, strlen(errortxt2), &dummy, NULL);
		WriteFile (houtput, text,      strlen(text),      &dummy, NULL);
		WriteFile (houtput, "\r\n",    2,		  &dummy, NULL);
		SDL_Delay (3000);	/* show the console 3 more seconds */
	}

	exit (1);
}

void Sys_Printf (const char *fmt, ...)
{
	va_list		argptr;
	char		text[1024];
	DWORD		dummy;

	va_start (argptr,fmt);
	q_vsnprintf (text, sizeof(text), fmt, argptr);
	va_end (argptr);

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

	if (isDedicated)
	{
		if (*text == 1 || *text == 2)
		{	//mostly for Con_[D]Warning
			SetConsoleTextAttribute(houtput, FOREGROUND_RED);
			WriteFile(houtput, text+1, strlen(text+1), &dummy, NULL);
			SetConsoleTextAttribute(houtput, FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_RED);
		}
		else
			WriteFile(houtput, text, strlen(text), &dummy, NULL);
	}
	else
	{
	/* SDL will put these into its own stdout log,
	   so print to stdout even in graphical mode. */
		fputs (text, stdout);
	}
}

void Sys_Quit (void)
{
	Host_Shutdown();

	if (isDedicated)
		FreeConsole ();

	exit (0);
}

double Sys_DoubleTime (void)
{
#if 1
	return SDL_GetPerformanceCounter() / (long double)SDL_GetPerformanceFrequency();
#else
	return SDL_GetTicks() / 1000.0;
#endif
}

#if defined(_WIN32)
void Sys_Image_BGRA_To_Clipboard(byte* bmbits, int width, int height, int size) // woods #screenshotcopy
{

	HBITMAP hBitmap = CreateBitmap(width, height, 1, 32 /* bits per pixel is 32 */, bmbits);

	OpenClipboard(NULL);

	if (!EmptyClipboard())
	{
		CloseClipboard();
		return;
	}

	if ((SetClipboardData(CF_BITMAP, hBitmap)) == NULL)
		Sys_Error("SetClipboardData failed");

	CloseClipboard();
}
#endif

const char *Sys_ConsoleInput (void) // woods #arrowkeys
{
	static char	con_text[256];
	static int	textlen;
	static int  cursor_pos;
	INPUT_RECORD	recs[1024];
	int		ch;
	DWORD		dummy, numread, numevents;

	for ( ;; )
	{
		if (GetNumberOfConsoleInputEvents(hinput, &numevents) == 0)
			Sys_Error ("Error getting # of console events");

		if (! numevents)
			break;

		if (ReadConsoleInput(hinput, recs, 1, &numread) == 0)
			Sys_Error ("Error reading console input");

		if (numread != 1)
			Sys_Error ("Couldn't read console input");

		if (recs[0].EventType == KEY_EVENT)
		{
			if (recs[0].Event.KeyEvent.bKeyDown == FALSE)
			{
				if (recs[0].Event.KeyEvent.wVirtualKeyCode == VK_LEFT)
				{
					if (cursor_pos > 0)
					{
						cursor_pos--;
						WriteFile(houtput, "\b", 1, &dummy, NULL);
					}
					continue;
				}
				else if (recs[0].Event.KeyEvent.wVirtualKeyCode == VK_RIGHT)
				{
					if (cursor_pos < textlen)
					{
						WriteFile(houtput, &con_text[cursor_pos], 1, &dummy, NULL);
						cursor_pos++;
					}
					continue;
				}

				ch = recs[0].Event.KeyEvent.uChar.AsciiChar;

				switch (ch)
				{
				case '\r':
					WriteFile(houtput, "\r\n", 2, &dummy, NULL);

					if (textlen != 0)
					{
						con_text[textlen] = 0;
						textlen = 0;
						cursor_pos = 0; // woods #arrowkeys
						return con_text;
					}

					break;

				case '\b':
					if (cursor_pos > 0)
					{
						// Move characters after cursor back by one position
						memmove(&con_text[cursor_pos - 1], &con_text[cursor_pos], textlen - cursor_pos);
						cursor_pos--;
						textlen--;
						
						// Rewrite the line from cursor position
						WriteFile(houtput, "\b", 1, &dummy, NULL);
						if (cursor_pos < textlen)
						{
							WriteFile(houtput, &con_text[cursor_pos], textlen - cursor_pos, &dummy, NULL);
							WriteFile(houtput, " ", 1, &dummy, NULL);  // Clear last character
							// Move cursor back to position
							for (int i = 0; i < textlen - cursor_pos + 1; i++)
								WriteFile(houtput, "\b", 1, &dummy, NULL);
						}
						else
						{
							WriteFile(houtput, " \b", 2, &dummy, NULL);  // Clear last character
						}
					}
					break;

				default:
					if (ch >= ' ')
					{
						// Insert character at cursor position
						if (cursor_pos < textlen)
						{
							// Make room for new character
							memmove(&con_text[cursor_pos + 1], &con_text[cursor_pos], textlen - cursor_pos);
							con_text[cursor_pos] = ch;
							textlen++;
							
							// Write the new character and the rest of the line
							WriteFile(houtput, &con_text[cursor_pos], textlen - cursor_pos, &dummy, NULL);
							
							// Move cursor back to just after inserted character
							cursor_pos++;
							for (int i = 0; i < textlen - cursor_pos; i++)
								WriteFile(houtput, "\b", 1, &dummy, NULL);
						}
						else
						{
							// Append character at end of line
							con_text[textlen] = ch;
							WriteFile(houtput, &ch, 1, &dummy, NULL);
							textlen++;
							cursor_pos++;
						}
					}

					break;
				}
			}
		}
	}

	return NULL;
}

void Sys_Sleep (unsigned long msecs)
{
/*	Sleep (msecs);*/
	SDL_Delay (msecs);
}

void Sys_SendKeyEvents (void)
{
	IN_Commands();		//ericw -- allow joysticks to add keys so they can be used to confirm SCR_ModalMessage
	IN_SendKeyEvents();
}

#if defined(_WIN32) // woods #disablecaps via ironwail
void Sys_ActivateKeyFilter (qboolean active)
{
	if (isDedicated || !!active == (key_hook != NULL))
		return;

	if (key_hook)
	{
		UnhookWindowsHookEx(key_hook);
		key_hook = NULL;
	}
	else
	{
		key_hook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyFilter, GetModuleHandleW(NULL), 0);
		if (!key_hook)
			Sys_Printf("Warning: SetWindowsHookExW failed (%lu)\n", GetLastError());
	}
}
#endif