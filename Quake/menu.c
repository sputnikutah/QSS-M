/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
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

#include "quakedef.h"
#include "bgmusic.h"
#include "q_ctype.h" // woods #modsmenu (iw)

void (*vid_menucmdfn)(void); //johnfitz
void (*vid_menudrawfn)(void);
void (*vid_menukeyfn)(int key);

enum m_state_e m_state;
extern qboolean	keydown[256]; // woods #modsmenu (iw)

void M_Menu_Main_f (void);
	void M_Menu_SinglePlayer_f (void);
		void M_Menu_Load_f (void);
		void M_Menu_Save_f (void);
	void M_Menu_MultiPlayer_f (void);
		void M_Menu_Setup_f (void);
	void M_Menu_NameMaker_f(void); // woods #namemaker
		void M_Menu_Net_f (void);
		void M_Menu_LanConfig_f (void);
		void M_Menu_GameOptions_f (void);
		void M_Menu_Search_f (enum slistScope_e scope);
		void M_Menu_ServerList_f (void);
	void M_Menu_Options_f (void);
		void M_Menu_Keys_f (void);
		void M_Menu_Extras_f (void);
		void M_Menu_Video_f (void);
	void M_Menu_Mods_f(void); // woods #modsmenu (iw)
	void M_Menu_Help_f (void);
	void M_Menu_Quit_f (void);

void M_Main_Draw (void);
	void M_SinglePlayer_Draw (void);
		void M_Load_Draw (void);
		void M_Save_Draw (void);
	void M_MultiPlayer_Draw (void);
		void M_Setup_Draw (void);
void M_NameMaker_Draw(void); // woods #namemaker
		void M_Net_Draw (void);
		void M_LanConfig_Draw (void);
		void M_GameOptions_Draw (void);
		void M_Search_Draw (void);
		void M_ServerList_Draw (void);
	void M_Options_Draw (void);
		void M_Keys_Draw (void);
		void M_Video_Draw (void);
	void M_Mods_Draw(void); // woods #modsmenu (iw)
	void M_Help_Draw (void);
	void M_Quit_Draw (void);

void M_Main_Key (int key);
	void M_SinglePlayer_Key (int key);
		void M_Load_Key (int key);
		void M_Save_Key (int key);
	void M_MultiPlayer_Key (int key);
		void M_Setup_Key (int key);
		void M_Net_Key (int key);
		void M_LanConfig_Key (int key);
		void M_GameOptions_Key (int key);
		void M_Search_Key (int key);
		void M_ServerList_Key (int key);
	void M_Options_Key (int key);
		void M_Keys_Key (int key);
		void M_Video_Key (int key);
	void M_Help_Key (int key);
	void M_Quit_Key (int key);
	void M_NameMaker_Key(int key); // woods #namemaker

qboolean	m_entersound;		// play after drawing a frame, so caching
								// won't disrupt the sound
qboolean	m_recursiveDraw;

enum m_state_e	m_return_state;
qboolean	m_return_onerror;
char		m_return_reason [32];

#define StartingGame	(m_multiplayer_cursor == 1)
#define JoiningGame		(m_multiplayer_cursor == 0)
//#define	IPXConfig		(m_net_cursor == 1) // woods #skipipx
#define	TCPIPConfig		(m_net_cursor == 0)

void M_ConfigureNetSubsystem(void);

/*
================
M_DrawCharacter

Draws one solid graphics character
================
*/
void M_DrawCharacter (int cx, int line, int num)
{
	Draw_Character (cx, line, num);
}

void M_Print (int cx, int cy, const char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, (*str)+128);
		str++;
		cx += 8;
	}
}

void M_DrawCharacterRGBA (int cx, int line, int num, plcolour_t c, float alpha) // woods
{
	Draw_CharacterRGBA (cx, line, num, c, alpha);
}

void M_PrintRGBA (int cx, int cy, const char* str, plcolour_t c, float alpha) // woods
{
	while (*str)
	{
		M_DrawCharacterRGBA (cx, cy, (*str), c, alpha);
		str++;
		cx += 8;
	}
}

void M_Print2 (int cx, int cy, const char* str) // woods #speed yellow/gold numbers
{
	while (*str)
	{
		M_DrawCharacter(cx, cy, (*str) -30);
		str++;
		cx += 8;
	}
}

void M_PrintWhite (int cx, int cy, const char *str)
{
	while (*str)
	{
		M_DrawCharacter (cx, cy, *str);
		str++;
		cx += 8;
	}
}

void M_DrawTransPic (int x, int y, qpic_t *pic)
{
	Draw_Pic (x, y, pic); //johnfitz -- simplified becuase centering is handled elsewhere
}

void M_DrawPic (int x, int y, qpic_t *pic)
{
	Draw_Pic (x, y, pic); //johnfitz -- simplified becuase centering is handled elsewhere
}

void M_DrawSubpic (int x, int y, qpic_t* pic, int left, int top, int width, int height) // woods #modsmenu (iw)
{
	float s1 = left / (float)pic->width;
	float t1 = top / (float)pic->height;
	float s2 = width / (float)pic->width;
	float t2 = height / (float)pic->height;
	Draw_SubPic (x, y, width, height, pic, s1, t1, s2, t2);
}

void M_DrawTransPicTranslate (int x, int y, qpic_t *pic, plcolour_t top, plcolour_t bottom) //johnfitz -- more parameters
{
	Draw_TransPicTranslate (x, y, pic, top, bottom); //johnfitz -- simplified becuase centering is handled elsewhere
}

void M_DrawTextBox (int x, int y, int width, int lines)
{
	qpic_t	*p;
	int		cx, cy;
	int		n;

	// draw left side
	cx = x;
	cy = y;
	p = Draw_CachePic ("gfx/box_tl.lmp");
	M_DrawTransPic (cx, cy, p);
	p = Draw_CachePic ("gfx/box_ml.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_bl.lmp");
	M_DrawTransPic (cx, cy+8, p);

	// draw middle
	cx += 8;
	while (width > 0)
	{
		cy = y;
		p = Draw_CachePic ("gfx/box_tm.lmp");
		M_DrawTransPic (cx, cy, p);
		p = Draw_CachePic ("gfx/box_mm.lmp");
		for (n = 0; n < lines; n++)
		{
			cy += 8;
			if (n == 1)
				p = Draw_CachePic ("gfx/box_mm2.lmp");
			M_DrawTransPic (cx, cy, p);
		}
		p = Draw_CachePic ("gfx/box_bm.lmp");
		M_DrawTransPic (cx, cy+8, p);
		width -= 2;
		cx += 16;
	}

	// draw right side
	cy = y;
	p = Draw_CachePic ("gfx/box_tr.lmp");
	M_DrawTransPic (cx, cy, p);
	p = Draw_CachePic ("gfx/box_mr.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		M_DrawTransPic (cx, cy, p);
	}
	p = Draw_CachePic ("gfx/box_br.lmp");
	M_DrawTransPic (cx, cy+8, p);
}


void M_DrawQuakeBar(int x, int y, int cols) // woods #modsmenu (iw)
{
	M_DrawCharacter(x, y, '\35');
	x += 8;
	cols -= 2;
	while (cols-- > 0)
	{
		M_DrawCharacter(x, y, '\36');
		x += 8;
	}
	M_DrawCharacter(x, y, '\37');
}

void M_DrawEllipsisBar(int x, int y, int cols) // woods #modsmenu (iw)
{
	while (cols > 0)
	{
		M_DrawCharacter(x, y, '.' | 128);
		cols -= 2;
		x += 16;
	}
}

//=============================================================================

int m_save_demonum;


//=============================================================================
/* MAIN MENU */

int	m_main_cursor;
int m_main_mods; // woods #modsmenu (iw)

enum // woods #modsmenu (iw)
{
	MAIN_SINGLEPLAYER,
	MAIN_MULTIPLAYER,
	MAIN_OPTIONS,
	MAIN_MODS,
	MAIN_HELP,
	MAIN_QUIT,

	MAIN_ITEMS,
};


void M_Menu_Main_f (void)
{
	if (key_dest != key_menu)
	{
		m_save_demonum = cls.demonum;
		cls.demonum = -1;
	}
	key_dest = key_menu;
	m_state = m_main;
	m_entersound = true;

	// woods #modsmenu (iw)

	// When switching to a mod with a custom UI the 'Mods' option
// is no longer available in the main menu, so we move the cursor
// to 'Options' to nudge the player toward the secondary location.
// TODO (maybe): inform the user about the missing option
// and its alternative location?
	if (!m_main_mods && m_main_cursor == MAIN_MODS)
	{
		extern int options_cursor;
		m_main_cursor = MAIN_OPTIONS;
		options_cursor = 3; // OPT_MODS
	}

	IN_UpdateGrabs();
}


void M_Main_Draw (void) // woods #modsmenu (iw)
{
	int		cursor, f;
	qpic_t	*p;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/ttl_main.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	p = Draw_CachePic("gfx/mainmenu.lmp");
	if (m_main_mods)
	{
		int split = 60;
		M_DrawSubpic(72, 32, p, 0, 0, p->width, split);
		if (m_main_mods > 0)
			M_DrawTransPic(72, 32 + split, Draw_CachePic("gfx/menumods.lmp"));
		else
			M_Print(74, 32 + split + 1, "MODS");
		M_DrawSubpic(72, 32 + split + 20, p, 0, split, p->width, p->height - split);
	}
	else
		M_DrawTransPic(72, 32, Draw_CachePic("gfx/mainmenu.lmp"));




	f = (int)(realtime * 10) % 6;
	cursor = m_main_cursor;
	if (!m_main_mods && cursor > MAIN_MODS)
		--cursor;
	M_DrawTransPic(54, 32 + cursor * 20, Draw_CachePic(va("gfx/menudot%i.lmp", f + 1)));
}


void M_Main_Key (int key) // woods #modsmenu (iw)
{
	switch (key)
	{
	case K_ESCAPE:
	case K_BBUTTON:
		key_dest = key_game;
		m_state = m_none;
		cls.demonum = m_save_demonum;
		IN_UpdateGrabs();
		if (!cl_demoreel.value)	/* QuakeSpasm customization: */
			break;
		if (cl_demoreel.value >= 2 && cls.demonum == -1)
			cls.demonum = 0;
		if (cls.demonum != -1 && !cls.demoplayback && cls.state != ca_connected)
			CL_NextDemo ();
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		if (++m_main_cursor >= MAIN_ITEMS)
			m_main_cursor = 0;
		else if (!m_main_mods && m_main_cursor == MAIN_MODS)
			++m_main_cursor;
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		if (--m_main_cursor < 0)
			m_main_cursor = MAIN_ITEMS - 1;
		else if (!m_main_mods && m_main_cursor == MAIN_MODS)
			--m_main_cursor;
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
		m_entersound = true;

		switch (m_main_cursor)
		{
		case MAIN_SINGLEPLAYER:
			M_Menu_SinglePlayer_f ();
			break;

		case MAIN_MULTIPLAYER:
			M_Menu_MultiPlayer_f ();
			break;

		case MAIN_OPTIONS:
			M_Menu_Options_f ();
			break;

		case MAIN_HELP:
			M_Menu_Help_f ();
			break;

		case MAIN_MODS:
			M_Menu_Mods_f();
			break;

		case MAIN_QUIT:
			M_Menu_Quit_f ();
			break;
		}
	}
}

//=============================================================================

void M_CheckMods (void) // woods #modsmenu (iw)
{
	unsigned int id_mods, id_main;
	int h, length;

	m_main_mods = 0;
	if (!COM_FileExists("gfx/menumods.lmp", &id_mods))
		return;

	length = COM_OpenFile("gfx/mainmenu.lmp", &h, &id_main);
	if (length == 26888)
	{
		int mark = Hunk_LowMark();
		byte* data = (byte*)Hunk_Alloc(length);
		if (length == Sys_FileRead(h, data, length))
		{
			unsigned int hash = COM_HashBlock(data, length);
			if (hash == 0x136bc7fd || hash == 0x90555cb4)
				m_main_mods = 1;
		}
		Hunk_FreeToLowMark(mark);
	}

	if (id_mods >= id_main)
		m_main_mods = 1;

	COM_CloseFile(h);
}

//=============================================================================
/* SINGLE PLAYER MENU */

int	m_singleplayer_cursor;
#define	SINGLEPLAYER_ITEMS	3


void M_Menu_SinglePlayer_f (void)
{
	key_dest = key_menu;
	m_state = m_singleplayer;
	m_entersound = true;

	IN_UpdateGrabs();
}


void M_SinglePlayer_Draw (void)
{
	int		f;
	qpic_t	*p;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/ttl_sgl.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);
	M_DrawTransPic (72, 32, Draw_CachePic ("gfx/sp_menu.lmp") );

	f = (int)(realtime * 10)%6;

	M_DrawTransPic (54, 32 + m_singleplayer_cursor * 20,Draw_CachePic( va("gfx/menudot%i.lmp", f+1 ) ) );
}


void M_SinglePlayer_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case K_BBUTTON:
		M_Menu_Main_f ();
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		if (++m_singleplayer_cursor >= SINGLEPLAYER_ITEMS)
			m_singleplayer_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		if (--m_singleplayer_cursor < 0)
			m_singleplayer_cursor = SINGLEPLAYER_ITEMS - 1;
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
		m_entersound = true;

		switch (m_singleplayer_cursor)
		{
		case 0:
			if (sv.active)
				if (!SCR_ModalMessage("Are you sure you want to\nstart a new game?\n", 0.0f))
					break;
			key_dest = key_game;
			IN_UpdateGrabs();
			if (sv.active)
				Cbuf_AddText ("disconnect\n");
			Cbuf_AddText ("maxplayers 1\n");
			Cbuf_AddText ("samelevel 0\n"); //spike -- you'd be amazed how many qw players have this setting breaking their singleplayer experience...
			Cbuf_AddText ("deathmatch 0\n"); //johnfitz
			Cbuf_AddText ("coop 0\n"); //johnfitz
			Cbuf_AddText ("startmap_sp\n");
			break;

		case 1:
			M_Menu_Load_f ();
			break;

		case 2:
			M_Menu_Save_f ();
			break;
		}
	}
}

//=============================================================================
/* LOAD/SAVE MENU */

int		load_cursor;		// 0 < load_cursor < MAX_SAVEGAMES

#define	MAX_SAVEGAMES		20	/* johnfitz -- increased from 12 */
char	m_filenames[MAX_SAVEGAMES][SAVEGAME_COMMENT_LENGTH+1];
int		loadable[MAX_SAVEGAMES];

void M_ScanSaves (void)
{
	int	i, j;
	char	name[MAX_OSPATH];
	FILE	*f;
	int	version;

	for (i = 0; i < MAX_SAVEGAMES; i++)
	{
		strcpy (m_filenames[i], "--- UNUSED SLOT ---");
		loadable[i] = false;
		q_snprintf (name, sizeof(name), "%s/s%i.sav", com_gamedir, i);
		f = fopen (name, "r");
		if (!f)
			continue;
		fscanf (f, "%i\n", &version);
		fscanf (f, "%79s\n", name);
		q_strlcpy (m_filenames[i], name, SAVEGAME_COMMENT_LENGTH+1);

	// change _ back to space
		for (j = 0; j < SAVEGAME_COMMENT_LENGTH; j++)
		{
			if (m_filenames[i][j] == '_')
				m_filenames[i][j] = ' ';
		}
		loadable[i] = true;
		fclose (f);
	}
}

void M_Menu_Load_f (void)
{
	m_entersound = true;
	m_state = m_load;

	key_dest = key_menu;
	M_ScanSaves ();

	IN_UpdateGrabs();
}


void M_Menu_Save_f (void)
{
	if (!sv.active)
		return;
	if (cl.intermission)
		return;
	if (svs.maxclients != 1)
		return;
	m_entersound = true;
	m_state = m_save;

	key_dest = key_menu;
	IN_UpdateGrabs();
	M_ScanSaves ();
}


void M_Load_Draw (void)
{
	int		i;
	qpic_t	*p;

	p = Draw_CachePic ("gfx/p_load.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	for (i = 0; i < MAX_SAVEGAMES; i++)
		M_Print (16, 32 + 8*i, m_filenames[i]);

// line cursor
	M_DrawCharacter (8, 32 + load_cursor*8, 12+((int)(realtime*4)&1));
}


void M_Save_Draw (void)
{
	int		i;
	qpic_t	*p;

	p = Draw_CachePic ("gfx/p_save.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	for (i = 0; i < MAX_SAVEGAMES; i++)
		M_Print (16, 32 + 8*i, m_filenames[i]);

// line cursor
	M_DrawCharacter (8, 32 + load_cursor*8, 12+((int)(realtime*4)&1));
}


void M_Load_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
	case K_BBUTTON:
		M_Menu_SinglePlayer_f ();
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
		S_LocalSound ("misc/menu2.wav");
		if (!loadable[load_cursor])
			return;
		m_state = m_none;
		key_dest = key_game;
		IN_UpdateGrabs();

	// Host_Loadgame_f can't bring up the loading plaque because too much
	// stack space has been used, so do it now
		SCR_BeginLoadingPlaque ();

	// issue the load command
		Cbuf_AddText (va ("load s%i\n", load_cursor) );
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("misc/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("misc/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;
	}
}


void M_Save_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
	case K_BBUTTON:
		M_Menu_SinglePlayer_f ();
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
		m_state = m_none;
		key_dest = key_game;
		IN_UpdateGrabs();
		Cbuf_AddText (va("save s%i\n", load_cursor));
		return;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("misc/menu1.wav");
		load_cursor--;
		if (load_cursor < 0)
			load_cursor = MAX_SAVEGAMES-1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("misc/menu1.wav");
		load_cursor++;
		if (load_cursor >= MAX_SAVEGAMES)
			load_cursor = 0;
		break;
	}
}

//=============================================================================
/* MULTIPLAYER MENU */

int	m_multiplayer_cursor;
#define	MULTIPLAYER_ITEMS	3


void M_Menu_MultiPlayer_f (void)
{
	key_dest = key_menu;
	m_state = m_multiplayer;
	m_entersound = true;
	IN_UpdateGrabs();
}

extern char	lastmphost[NET_NAMELEN]; // woods - connected server address

void M_MultiPlayer_Draw (void)
{
	int		f, i; // woods
	qpic_t	*p;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);
	M_DrawTransPic (72, 32, Draw_CachePic ("gfx/mp_menu.lmp") );

	f = (int)(realtime * 10)%6;
	i = 24;
	if (strlen(lastmphost) > i)
		i = (strlen(lastmphost));

	M_DrawTransPic (54, 32 + m_multiplayer_cursor * 20,Draw_CachePic( va("gfx/menudot%i.lmp", f+1 ) ) );

	
	if (cl.maxclients > 1 && cls.state == ca_connected) // woods, give some extra info in mp menu
	{
		f = (320 - 26 * 8) / 2;
		M_DrawTextBox(f, 96, i, 2);
		f += 8;
		M_Print(f, 104, "currently connected to:");
		M_PrintWhite(f, 112, lastmphost);
	}

	if (ipxAvailable || ipv4Available || ipv6Available)
		return;
	M_PrintWhite ((320/2) - ((27*8)/2), 148, "No Communications Available");
}


void M_MultiPlayer_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case K_BBUTTON:
		M_Menu_Main_f ();
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		if (++m_multiplayer_cursor >= MULTIPLAYER_ITEMS)
			m_multiplayer_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		if (--m_multiplayer_cursor < 0)
			m_multiplayer_cursor = MULTIPLAYER_ITEMS - 1;
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
		m_entersound = true;
		switch (m_multiplayer_cursor)
		{
		case 0:
			if (ipxAvailable || ipv4Available || ipv6Available)
				M_Menu_LanConfig_f (); // woods #skipipx
			break;

		case 1:
			if (ipxAvailable || ipv4Available || ipv6Available)
				M_Menu_LanConfig_f (); // woods #skipipx
			break;

		case 2:
			M_Menu_Setup_f ();
			break;
		}
	}
}

//=============================================================================
/* SETUP MENU */

static int		setup_cursor = 5; // woods 4 to 5 #namemaker
static int		setup_cursor_table[] = {40, 56, 72, 88, 112, 150}; // woods add value, change position #namemaker #colorbar

char	namemaker_name[16]; // woods #namemaker
qboolean namemaker_shortcut = false; // woods #namemaker
qboolean from_namemaker = false; // woods #namemaker

static char	setup_hostname[16];
static char	setup_myname[16];
static plcolour_t	setup_oldtop;
static plcolour_t	setup_oldbottom;
static plcolour_t	setup_top;
static plcolour_t	setup_bottom;
extern qboolean	keydown[];


//http://axonflux.com/handy-rgb-to-hsl-and-rgb-to-hsv-color-model-c
static void rgbtohsv(byte *rgb, vec3_t result)
{	//helper for the setup menu
	int r = rgb[0], g = rgb[1], b = rgb[2];
	float maxc = q_max(r, q_max(g, b)), minc = q_min(r, q_min(g, b));
    float h, s, l = (maxc + minc) / 2;

	float d = maxc - minc;
	if (maxc)
		s = d / maxc;
	else
		s = 0;

	if(maxc == minc)
	{
		h = 0; // achromatic
	}
	else
	{
		if (maxc == r)
			h = (g - b) / d + ((g < b) ? 6 : 0);
		else if (maxc == g)
			h = (b - r) / d + 2;
		else
			h = (r - g) / d + 4;
		h /= 6;
    }

	result[0] = h;
	result[1] = s;
	result[2] = l;
};
//http://axonflux.com/handy-rgb-to-hsl-and-rgb-to-hsv-color-model-c
static void hsvtorgb(float inh, float s, float v, byte *out)
{	//helper for the setup menu
	int r, g, b;
	float h = inh - (int)floor(inh);
	int i = h * 6;
	float f = h * 6 - i;
	float p = v * (1 - s);
	float q = v * (1 - f * s);
	float t = v * (1 - (1 - f) * s);
	switch(i)
	{
	default:
	case 0: r = v*0xff, g = t*0xff, b = p*0xff; break;
	case 1: r = q*0xff, g = v*0xff, b = p*0xff; break;
	case 2: r = p*0xff, g = v*0xff, b = t*0xff; break;
	case 3: r = p*0xff, g = q*0xff, b = v*0xff; break;
	case 4: r = t*0xff, g = p*0xff, b = v*0xff; break;
	case 5: r = v*0xff, g = p*0xff, b = q*0xff; break;
	}

	out[0] = r;
	out[1] = g;
	out[2] = b;
};

qboolean rgbactive; // woods
qboolean colordelta; // woods

void M_AdjustColour(plcolour_t *tr, int dir)
{
	if (keydown[K_SHIFT])
	{
		rgbactive = true; // woods
		vec3_t hsv;
		rgbtohsv(CL_PLColours_ToRGB(tr), hsv);

		hsv[0] += dir/128.0;
		hsv[1] = 1;
		hsv[2] = 1;	//make these consistent and not inherited from any legacy colours. we're persisting in rgb with small hue changes so we can't actually handle greys, so whack the saturation and brightness right up.
		tr->type = 2;	//rgb...
		tr->basic = 0;	//no longer relevant.
		hsvtorgb(hsv[0], hsv[1], hsv[2], tr->rgb);
	}
	else
	{
		tr->type = 1;
		if (tr->basic+dir < 0)
			tr->basic = 13;
		else if (tr->basic+dir > 13)
			tr->basic = 0;
		else
			tr->basic += dir;
	}
}

#define	NUM_SETUP_CMDS	6 // woods 5 to 6 #namemaker
void M_Menu_Setup_f (void)
{
	key_dest = key_menu;
	m_state = m_setup;
	m_entersound = true;
	if (from_namemaker) // woods #namemaker
		from_namemaker = !from_namemaker;
	else
		Q_strcpy(setup_myname, cl_name.string);
	Q_strcpy(setup_hostname, hostname.string);
	setup_top = setup_oldtop = CL_PLColours_Parse(cl_topcolor.string);
	setup_bottom = setup_oldbottom = CL_PLColours_Parse(cl_bottomcolor.string);

	IN_UpdateGrabs();
}

qboolean chasewasnotactive; // woods #3rdperson
qboolean flyme; // woods #3rdperson

void M_DrawColorBar_Top (int x, int y, int highlight) // woods #colorbar -- mh
{
	int i;
	int intense = highlight * 16 + (highlight < 8 ? 11 : 4);

	if (setup_top.type == 2)
	{
		Draw_FillPlayer (x, y + 4, 8, 8, setup_top, 1);
	}
	else
	{
		// position correctly
		x = 64;

		for (i = 0; i < 14; i++)
		{
			// take the approximate midpoint colour (handle backward ranges)
			int c = i * 16 + (i < 8 ? 8 : 7);

			// braw baseline colour (offset downwards a little so that it fits correctly
			Draw_Fill(x + i * 8, y + 4, 8, 8, c, 1);
		}

		// draw the highlight rectangle
		Draw_Fill(x - 1 + highlight * 8, y + 3, 10, 10, 15, 1);

		// redraw the highlighted color at brighter intensity
		Draw_Fill(x + highlight * 8, y + 4, 8, 8, intense, 1);
	}
}

void M_DrawColorBar_Bot (int x, int y, int highlight) // woods #colorbar -- mh
{
	int i;
	int intense = highlight * 16 + (highlight < 8 ? 11 : 4);

	if (setup_bottom.type == 2)
	{
		Draw_FillPlayer (x, y + 4, 8, 8, setup_bottom, 1);
	}
	else
	{
		// position correctly
		x = 64;

		for (i = 0; i < 14; i++)
		{
			// take the approximate midpoint colour (handle backward ranges)
			int c = i * 16 + (i < 8 ? 8 : 7);

			// braw baseline colour (offset downwards a little so that it fits correctly
			Draw_Fill(x + i * 8, y + 4, 8, 8, c, 1);
		}

		// draw the highlight rectangle
		Draw_Fill(x - 1 + highlight * 8, y + 3, 10, 10, 15, 1);

		// redraw the highlighted color at brighter intensity
		Draw_Fill(x + highlight * 8, y + 4, 8, 8, intense, 1);
	}
}

void M_Setup_Draw (void)
{
	qpic_t	*p;

	if (cls.state == ca_connected)
	{
		char buf[15];
		const char* obs;
		obs = Info_GetKey(cl.scores[cl.realviewentity - 1].userinfo, "observer", buf, sizeof(buf));

		if (!strcmp(obs, "fly")) // woods #3rdperson
			flyme = true;
		else
			flyme = false;
	}

	if (!chase_active.value && !cls.demoplayback && host_initialized && !flyme && cls.state == ca_connected) // woods #3rdperson
	{
		chasewasnotactive = true;
		Cbuf_AddText("chase_active 1\n");
	}

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	M_Print (64, 40, "Hostname");
	M_DrawTextBox (160, 32, 16, 1);
	M_Print (168, 40, setup_hostname);

	M_Print (64, 56, "Your name");
	M_DrawTextBox (160, 48, 16, 1);
	M_PrintWhite (168, 56, setup_myname); // woods change to white #namemaker

	M_Print(64, 72, "Name Maker"); // woods #namemaker

	M_Print (64, 88, "Shirt -"); // woods 80 to 104 #namemaker #showcolornum
	M_PrintWhite (126, 88, CL_PLColours_ToString (setup_top)); // woods #showcolornum
	M_DrawColorBar_Top (64, 94, atoi(CL_PLColours_ToString (setup_top))); // woods #colorbar
	M_Print (64, 112, "Pants -"); // woods 104 to 128 #namemaker #showcolornum
	M_PrintWhite (126, 112, CL_PLColours_ToString (setup_bottom)); // woods #showcolornum
	M_DrawColorBar_Bot (64, 118, atoi(CL_PLColours_ToString (setup_bottom))); // woods #colorbar

	if (!rgbactive) // woods
		M_PrintRGBA (64, 170, "Shift For RGB Colors", CL_PLColours_Parse ("0xffffff"), 0.6f); // woods

	M_DrawTextBox (64, 142, 14, 1);  // woods 140 to 152 #namemaker
	M_Print (72, 150, "Accept Changes"); // woods #colorbar

	p = Draw_CachePic ("gfx/bigbox.lmp");
	M_DrawTransPic (196, 77, p); // woods #colorbar
	p = Draw_CachePic ("gfx/menuplyr.lmp");

	setup_top = CL_PLColours_Parse(CL_PLColours_ToString(setup_top)); // woods menu color fix
	setup_bottom = CL_PLColours_Parse(CL_PLColours_ToString(setup_bottom)); // woods menu color fix

	M_DrawTransPicTranslate (208, 85, p, setup_top, setup_bottom); // woods #colorbar

	M_DrawCharacter (56, setup_cursor_table [setup_cursor], 12+((int)(realtime*4)&1));

	if (setup_cursor == 0)
		M_DrawCharacter (168 + 8*strlen(setup_hostname), setup_cursor_table [setup_cursor], 10+((int)(realtime*4)&1));

	if (setup_cursor == 1)
		M_DrawCharacter (168 + 8*strlen(setup_myname), setup_cursor_table [setup_cursor], 10+((int)(realtime*4)&1));
}

char lastColorSelected[10]; // woods

void M_Setup_Key (int k)
{
	int	l; // woods #namemaker

	switch (k)
	{
	case K_ESCAPE:
		if (chasewasnotactive && !cls.demoplayback && host_initialized && !flyme) // woods #3rdperson
		{
			chasewasnotactive = false;
			Cbuf_AddText("chase_active 0\n");
		}
		if (colordelta)
		{
			colordelta = false;
			Cbuf_AddText(va("color %s %s\n", CL_PLColours_ToString(setup_oldtop), CL_PLColours_ToString(setup_oldbottom)));
		}
	case K_BBUTTON:
		M_Menu_MultiPlayer_f ();
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		setup_cursor--;
		if (setup_cursor < 0)
			setup_cursor = NUM_SETUP_CMDS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		setup_cursor++;
		if (setup_cursor >= NUM_SETUP_CMDS)
			setup_cursor = 0;
		break;

	case K_MWHEELDOWN:
	case K_LEFTARROW:
		if (setup_cursor < 2)
			return;
		S_LocalSound ("misc/menu3.wav");
		if (setup_cursor == 3) // 2 to 3 woods #namemaker
		{
			M_AdjustColour(&setup_top, -1);
			strncpy(lastColorSelected, CL_PLColours_ToString(setup_top), sizeof((CL_PLColours_ToString(setup_top))));
			if (chase_active.value && !cls.demoplayback && host_initialized && !flyme) // woods #3rdperson
				if (!CL_PLColours_Equals(setup_top, setup_oldtop) || !CL_PLColours_Equals(setup_bottom, setup_oldbottom))
				{
					Cbuf_AddText(va("color %s %s\n", CL_PLColours_ToString(setup_top), CL_PLColours_ToString(setup_bottom)));
					colordelta = true;
				}
		}
		if (setup_cursor == 4) // 3 to 4 woods #namemaker
		{
			M_AdjustColour(&setup_bottom, -1);
			strncpy(lastColorSelected, CL_PLColours_ToString(setup_bottom), sizeof((CL_PLColours_ToString(setup_bottom))));
			if (chase_active.value && !cls.demoplayback && host_initialized && !flyme) // woods #3rdperson
				if (!CL_PLColours_Equals(setup_top, setup_oldtop) || !CL_PLColours_Equals(setup_bottom, setup_oldbottom))
				{
					Cbuf_AddText(va("color %s %s\n", CL_PLColours_ToString(setup_top), CL_PLColours_ToString(setup_bottom)));
					colordelta = true;
				}
		}
		break;
	case K_MWHEELUP:
	case K_RIGHTARROW:
		if (setup_cursor < 2)
			return;
	forward:
		S_LocalSound ("misc/menu3.wav");
		if (setup_cursor == 3) // 2 to 3 woods #namemaker
		{
			M_AdjustColour(&setup_top, +1);
			strncpy (lastColorSelected, CL_PLColours_ToString (setup_top), sizeof ((CL_PLColours_ToString (setup_top))));
			if (chase_active.value && !cls.demoplayback && host_initialized && !flyme) // woods #3rdperson
				if (!CL_PLColours_Equals(setup_top, setup_oldtop) || !CL_PLColours_Equals(setup_bottom, setup_oldbottom))
				{
					Cbuf_AddText(va("color %s %s\n", CL_PLColours_ToString(setup_top), CL_PLColours_ToString(setup_bottom)));
					colordelta = true;
				}
		}
		if (setup_cursor == 4) // 3 to 4 woods #namemaker
		{
			M_AdjustColour(&setup_bottom, +1);
			strncpy (lastColorSelected, CL_PLColours_ToString (setup_bottom), sizeof ((CL_PLColours_ToString (setup_bottom))));
			if (chase_active.value && !cls.demoplayback && host_initialized && !flyme) // woods #3rdperson
				if (!CL_PLColours_Equals(setup_top, setup_oldtop) || !CL_PLColours_Equals(setup_bottom, setup_oldbottom))
				{
					Cbuf_AddText(va("color %s %s\n", CL_PLColours_ToString(setup_top), CL_PLColours_ToString(setup_bottom)));
					colordelta = true;
				}
		}
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
		if (setup_cursor == 0 || setup_cursor == 1)
			return;

		if (setup_cursor == 3 || setup_cursor == 4) // inc 1 both woods #namemaker
			goto forward;

		if (setup_cursor == 2) // woods #namemaker
		{
			m_entersound = true;
			M_Menu_NameMaker_f();
			break;
		}

		// setup_cursor == 4 (OK)
		if (Q_strcmp(cl_name.string, setup_myname) != 0)
			Cbuf_AddText ( va ("name \"%s\"\n", setup_myname) );
		if (Q_strcmp(hostname.string, setup_hostname) != 0)
			Cvar_Set("hostname", setup_hostname);
		if (!CL_PLColours_Equals(setup_top, setup_oldtop) || !CL_PLColours_Equals(setup_bottom, setup_oldbottom))
			Cbuf_AddText( va ("color %s %s\n", CL_PLColours_ToString(setup_top), CL_PLColours_ToString(setup_bottom)) );
		m_entersound = true;

		if (chasewasnotactive && !cls.demoplayback && host_initialized && !flyme) // woods #3rdperson
		{
			chasewasnotactive = false;
			Cbuf_AddText("chase_active 0\n");
		}

		M_Menu_MultiPlayer_f ();
		break;

	case K_BACKSPACE:
		if (setup_cursor == 0)
		{
			if (strlen(setup_hostname))
				setup_hostname[strlen(setup_hostname)-1] = 0;
		}

		if (setup_cursor == 1)
		{
			if (strlen(setup_myname))
				setup_myname[strlen(setup_myname)-1] = 0;
		}
		break;

	case 'c': // woods, copy color
	case 'C':
		if (keydown[K_CTRL])
		{
			if (lastColorSelected[0] != '\0')
				SDL_SetClipboardText (lastColorSelected);
			else
				SDL_SetClipboardText (CL_PLColours_ToString (setup_bottom));
			const char* soundFile = COM_FileExists("sound/qssm/copy.wav", NULL) ? "qssm/copy.wav" : "player/tornoff2.wav";
			S_LocalSound(soundFile); // woods add sound to screenshot
		}
		break;

	default: // woods #namemaker
		if (k < 32 || k > 127)
			break;

		Key_Extra (&k);

		if (setup_cursor == 0)
		{
			l = strlen(setup_hostname);
			if (l < 15)
			{
				setup_hostname[l] = k;
				setup_hostname[l + 1] = 0;
			}
		}
		else if (setup_cursor == 2)
		{
			l = strlen(setup_myname);
			if (l < 15)
			{
				setup_myname[l] = k;
				setup_myname[l + 1] = 0;
			}
		}
		break;
	}
}


void M_Setup_Char (int k)
{
	int l;

	switch (setup_cursor)
	{
	case 0:
		l = strlen(setup_hostname);
		if (l < 15)
		{
			setup_hostname[l+1] = 0;
			setup_hostname[l] = k;
		}
		break;
	case 1:
		l = strlen(setup_myname);
		if (l < 15)
		{
			setup_myname[l+1] = 0;
			setup_myname[l] = k;
		}
		break;
	}
}


qboolean M_Setup_TextEntry (void)
{
	return (setup_cursor == 0 || setup_cursor == 1);
}


//=============================================================================
/* NAME MAKER MENU */ // woods #namemaker from joequake, qrack
//=============================================================================
int	namemaker_cursor_x, namemaker_cursor_y;
#define	NAMEMAKER_TABLE_SIZE	16
//extern int key_special_dest;

void M_Menu_NameMaker_f (void)
{
	key_dest = key_menu;
	//key_special_dest = 1;
	m_state = m_namemaker;
	m_entersound = true;
	q_strlcpy(namemaker_name, setup_myname, sizeof(namemaker_name));
}

void M_Shortcut_NameMaker_f (void)
{
	// Baker: our little shortcut into the name maker
	namemaker_shortcut = true;
	q_strlcpy(setup_myname, cl_name.string, sizeof(setup_myname));//R00k
	namemaker_cursor_x = 0;
	namemaker_cursor_y = 0;
	M_Menu_NameMaker_f();
}

void M_NameMaker_Draw (void)
{
	int	x, y;

	M_Print(48, 16, "Your name");
	M_DrawTextBox(120, 8, 16, 1);
	M_PrintWhite(128, 16, namemaker_name);

	for (y = 0; y < NAMEMAKER_TABLE_SIZE; y++)
		for (x = 0; x < NAMEMAKER_TABLE_SIZE; x++)
			M_DrawCharacter(32 + (16 * x), 40 + (8 * y), NAMEMAKER_TABLE_SIZE * y + x);

	if (namemaker_cursor_y == NAMEMAKER_TABLE_SIZE)
		M_DrawCharacter(128, 184, 12 + ((int)(realtime * 4) & 1));
	else
		M_DrawCharacter(24 + 16 * namemaker_cursor_x, 40 + 8 * namemaker_cursor_y, 12 + ((int)(realtime * 4) & 1));

	//	M_DrawTextBox (136, 176, 2, 1);
	M_Print(56, 184, "press");
	M_PrintWhite(103, 184, "ESC");
	M_Print(133, 184, "to save changes");
}

void M_NameMaker_Key (int k)
{
	int	l;

	switch (k)
	{
	case K_ESCAPE:
	case K_BBUTTON:
		//key_special_dest = false;

		if (namemaker_shortcut)
		{// Allow quick exit for namemaker command
			key_dest = key_game;
			m_state = m_none;

			//Save the name
			if (strcmp(namemaker_name, cl_name.string))
			{
				Cbuf_AddText(va("name \"%s\"\n", namemaker_name));
				Con_Printf("name changed to %s\n", namemaker_name);
			}
			namemaker_shortcut = false;
			from_namemaker = false;
		}
		else
		{
			from_namemaker = true;
			q_strlcpy(setup_myname, namemaker_name, sizeof(setup_myname));//R00k
			M_Menu_Setup_f();
		}

		break;

	case K_UPARROW:
		S_LocalSound("misc/menu1.wav");
		namemaker_cursor_y--;
		if (namemaker_cursor_y < 0)
			namemaker_cursor_y = NAMEMAKER_TABLE_SIZE - 1;
		break;

	case K_DOWNARROW:
		S_LocalSound("misc/menu1.wav");
		namemaker_cursor_y++;
		if (namemaker_cursor_y > NAMEMAKER_TABLE_SIZE - 1)
			namemaker_cursor_y = 0;
		break;

	case K_PGUP:
		S_LocalSound("misc/menu1.wav");
		namemaker_cursor_y = 0;
		break;

	case K_PGDN:
		S_LocalSound("misc/menu1.wav");
		namemaker_cursor_y = NAMEMAKER_TABLE_SIZE - 1;
		break;

	case K_LEFTARROW:
		S_LocalSound("misc/menu1.wav");
		namemaker_cursor_x--;
		if (namemaker_cursor_x < 0)
			namemaker_cursor_x = NAMEMAKER_TABLE_SIZE - 1;
		break;

	case K_RIGHTARROW:
		S_LocalSound("misc/menu1.wav");
		namemaker_cursor_x++;
		if (namemaker_cursor_x >= NAMEMAKER_TABLE_SIZE - 1)
			namemaker_cursor_x = 0;
		break;

	case K_HOME:
		S_LocalSound("misc/menu1.wav");
		namemaker_cursor_x = 0;
		break;

	case K_END:
		S_LocalSound("misc/menu1.wav");
		namemaker_cursor_x = NAMEMAKER_TABLE_SIZE - 1;
		break;

	case K_BACKSPACE:
		if ((l = strlen(namemaker_name)))
			namemaker_name[l - 1] = 0;
		break;

	// If we reached this point, we are simulating ENTER

	case K_SPACE:
	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
		if (namemaker_cursor_y == NAMEMAKER_TABLE_SIZE)
		{
			q_strlcpy(setup_myname, namemaker_name, sizeof(setup_myname));
			M_Menu_Setup_f();
		}
		else
		{
			l = strlen(namemaker_name);
			if (l < 15)
			{
				namemaker_name[l] = NAMEMAKER_TABLE_SIZE * namemaker_cursor_y + namemaker_cursor_x;
				namemaker_name[l + 1] = 0;
			}
		}
		break;

	default:
		if (k < 32 || k > 127)
			break;

		Key_Extra (&k);

		l = strlen(namemaker_name);
		if (l < 15)
		{
			namemaker_name[l] = k;
			namemaker_name[l + 1] = 0;
		}
		break;
	}
}

//=============================================================================
/* NET MENU */

int	m_net_cursor;
int m_net_items;

const char *net_helpMessage [] =
{
/* .........1.........2.... */
  " Novell network LANs    ",
  " or Windows 95 DOS-box. ",
  "                        ",
  "(LAN=Local Area Network)",

  " Commonly used to play  ",
  " over the Internet, but ",
  " also used on a Local   ",
  " Area Network.          "
};

void M_Menu_Net_f (void)
{
	key_dest = key_menu;
	m_state = m_net;
	m_entersound = true;
	m_net_items = 2;

	IN_UpdateGrabs();

	if (m_net_cursor >= m_net_items)
		m_net_cursor = 0;
	m_net_cursor--;
	M_Net_Key (K_DOWNARROW);
}


void M_Net_Draw (void)
{
	int		f;
	qpic_t	*p;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	f = 32;

	/*if (ipxAvailable)   // woods this is not needed
		p = Draw_CachePic ("gfx/netmen3.lmp");
	else
		p = Draw_CachePic ("gfx/dim_ipx.lmp");
	M_DrawTransPic (72, f, p);*/

	f += 19;
	if (ipv4Available || ipv6Available)
		p = Draw_CachePic ("gfx/netmen4.lmp");
	else
		p = Draw_CachePic ("gfx/dim_tcp.lmp");
	M_DrawTransPic (72, f, p);

	f = (320-26*8)/2;
	M_DrawTextBox (f, 96, 24, 4);
	f += 8;
	M_Print (f, 104, net_helpMessage[m_net_cursor*4+0]);
	M_Print (f, 112, net_helpMessage[m_net_cursor*4+1]);
	M_Print (f, 120, net_helpMessage[m_net_cursor*4+2]);
	M_Print (f, 128, net_helpMessage[m_net_cursor*4+3]);

	f = (int)(realtime * 10)%6;
	M_DrawTransPic (54, 32 + m_net_cursor * 20,Draw_CachePic( va("gfx/menudot%i.lmp", f+1 ) ) );
}


void M_Net_Key (int k)
{
again:
	switch (k)
	{
	case K_ESCAPE:
	case K_BBUTTON:
		M_Menu_MultiPlayer_f ();
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		if (++m_net_cursor >= m_net_items)
			m_net_cursor = 0;
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		if (--m_net_cursor < 0)
			m_net_cursor = m_net_items - 1;
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
		m_entersound = true;
		M_Menu_LanConfig_f ();
		break;
	}

	if (m_net_cursor == 0 && !ipxAvailable)
		goto again;
	if (m_net_cursor == 1 && !(ipv4Available || ipv6Available))
		goto again;
}

//=============================================================================
/* OPTIONS MENU */

enum
{
	OPT_CUSTOMIZE = 0,
	OPT_CONSOLE,	// 1
	OPT_DEFAULTS,	// 2
	OPT_SCALE,
	OPT_SCRSIZE,
	OPT_GAMMA,
	OPT_CONTRAST,
	OPT_MOUSESPEED,
	OPT_SBALPHA,
	OPT_SNDVOL,
	OPT_MUSICVOL,
	OPT_MUSICEXT,
	OPT_ALWAYRUN,
	OPT_INVMOUSE,
	OPT_ALWAYSMLOOK,
	OPT_LOOKSPRING,
	OPT_LOOKSTRAFE,
//#ifdef _WIN32
//	OPT_USEMOUSE,
//#endif
	OPT_EXTRAS,
	OPT_VIDEO,	// This is the last before OPTIONS_ITEMS
	OPTIONS_ITEMS
};

enum
{
	ALWAYSRUN_OFF = 0,
	ALWAYSRUN_VANILLA,
	ALWAYSRUN_QUAKESPASM,
	ALWAYSRUN_ITEMS
};

#define	SLIDER_RANGE	10

int		options_cursor;

void M_Menu_Options_f (void)
{
	key_dest = key_menu;
	m_state = m_options;
	m_entersound = true;

	IN_UpdateGrabs();
}


void M_AdjustSliders (int dir)
{
	int	curr_alwaysrun, target_alwaysrun;
	float	f, l;

	S_LocalSound ("misc/menu3.wav");

	switch (options_cursor)
	{
	case OPT_SCALE:	// console and menu scale
		l = ((vid.width + 31) / 32) / 10.0;
		f = scr_conscale.value + dir * .1;
		if (f < 1)	f = 1;
		else if(f > l)	f = l;
		Cvar_SetValue ("scr_conscale", f);
		Cvar_SetValue ("scr_menuscale", f);
		Cvar_SetValue ("scr_sbarscale", f);
		break;
	case OPT_SCRSIZE:	// screen size
		f = scr_viewsize.value + dir * 10;
		if (f > 130)	f = 130;
		else if(f < 30)	f = 30;
		Cvar_SetValue ("viewsize", f);
		break;
	case OPT_GAMMA:	// gamma
		f = vid_gamma.value - dir * 0.05;
		if (f < 0.5)	f = 0.5;
		else if (f > 1)	f = 1;
		Cvar_SetValue ("gamma", f);
		break;
	case OPT_CONTRAST:	// contrast
		f = vid_contrast.value + dir * 0.1;
		if (f < 1)	f = 1;
		else if (f > 2)	f = 2;
		Cvar_SetValue ("contrast", f);
		break;
	case OPT_MOUSESPEED:	// mouse speed
		f = sensitivity.value + dir * 0.5;
		if (f > 11)	f = 11;
		else if (f < 1)	f = 1;
		Cvar_SetValue ("sensitivity", f);
		break;
	case OPT_SBALPHA:	// statusbar alpha
		f = scr_sbaralpha.value - dir * 0.05;
		if (f < 0)	f = 0;
		else if (f > 1)	f = 1;
		Cvar_SetValue ("scr_sbaralpha", f);
		break;
	case OPT_MUSICVOL:	// music volume
		f = bgmvolume.value + dir * 0.1;
		if (f < 0)	f = 0;
		else if (f > 1)	f = 1;
		Cvar_SetValue ("bgmvolume", f);
		break;
	case OPT_MUSICEXT:	// enable external music vs cdaudio
		Cvar_Set ("bgm_extmusic", bgm_extmusic.value ? "0" : "1");
		break;
	case OPT_SNDVOL:	// sfx volume
		f = sfxvolume.value + dir * 0.1;
		if (f < 0)	f = 0;
		else if (f > 1)	f = 1;
		Cvar_SetValue ("volume", f);
		break;

	case OPT_ALWAYRUN:	// always run
		if (cl_alwaysrun.value)
			curr_alwaysrun = ALWAYSRUN_QUAKESPASM;
		else if (cl_forwardspeed.value > 200)
			curr_alwaysrun = ALWAYSRUN_VANILLA;
		else
			curr_alwaysrun = ALWAYSRUN_OFF;
			
		target_alwaysrun = (ALWAYSRUN_ITEMS + curr_alwaysrun + dir) % ALWAYSRUN_ITEMS;
			
		if (target_alwaysrun == ALWAYSRUN_VANILLA)
		{
			Cvar_SetValue ("cl_alwaysrun", 0);
			Cvar_SetValue ("cl_forwardspeed", 400);
			Cvar_SetValue ("cl_backspeed", 400);
		}
		else if (target_alwaysrun == ALWAYSRUN_QUAKESPASM)
		{
			Cvar_SetValue ("cl_alwaysrun", 1);
			Cvar_SetValue ("cl_forwardspeed", 200);
			Cvar_SetValue ("cl_backspeed", 200);
		}
		else // ALWAYSRUN_OFF
		{
			Cvar_SetValue ("cl_alwaysrun", 0);
			Cvar_SetValue ("cl_forwardspeed", 200);
			Cvar_SetValue ("cl_backspeed", 200);
		}
		break;

	case OPT_INVMOUSE:	// invert mouse
		Cvar_SetValue ("m_pitch", -m_pitch.value);
		break;

	case OPT_ALWAYSMLOOK:
		if (in_mlook.state & 1)
			Cbuf_AddText("-mlook");
		else
			Cbuf_AddText("+mlook");
		break;

	case OPT_LOOKSPRING:	// lookspring
		Cvar_Set ("lookspring", lookspring.value ? "0" : "1");
		break;

	case OPT_LOOKSTRAFE:	// lookstrafe
		Cvar_Set ("lookstrafe", lookstrafe.value ? "0" : "1");
		break;
	}
}


void M_DrawSlider (int x, int y, float range)
{
	int	i;

	if (range < 0)
		range = 0;
	if (range > 1)
		range = 1;
	M_DrawCharacter (x-8, y, 128);
	for (i = 0; i < SLIDER_RANGE; i++)
		M_DrawCharacter (x + i*8, y, 129);
	M_DrawCharacter (x+i*8, y, 130);
	M_DrawCharacter (x + (SLIDER_RANGE-1)*8 * range, y, 131);
}

void M_DrawCheckbox (int x, int y, int on)
{
#if 0
	if (on)
		M_DrawCharacter (x, y, 131);
	else
		M_DrawCharacter (x, y, 129);
#endif
	if (on)
		M_Print (x, y, "on");
	else
		M_Print (x, y, "off");
}

void M_Options_Draw (void)
{
	float		r, l;
	qpic_t	*p;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_option.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	// Draw the items in the order of the enum defined above:
	// OPT_CUSTOMIZE:
	M_Print (16, 32,			"              Controls");
	// OPT_CONSOLE:
	M_Print (16, 32 + 8*OPT_CONSOLE,	"          Goto console");
	// OPT_DEFAULTS:
	M_Print (16, 32 + 8*OPT_DEFAULTS,	"          Reset config");

	// OPT_SCALE:
	M_Print (16, 32 + 8*OPT_SCALE,		"                 Scale");
	l = (vid.width / 320.0) - 1;
	r = l > 0 ? (scr_conscale.value - 1) / l : 0;
	M_DrawSlider (220, 32 + 8*OPT_SCALE, r);

	// OPT_SCRSIZE:
	M_Print (16, 32 + 8*OPT_SCRSIZE,	"           Screen size");
	r = (scr_viewsize.value - 30) / (130 - 30);
	M_DrawSlider (220, 32 + 8*OPT_SCRSIZE, r);

	// OPT_GAMMA:
	M_Print (16, 32 + 8*OPT_GAMMA,		"            Brightness");
	r = (1.0 - vid_gamma.value) / 0.5;
	M_DrawSlider (220, 32 + 8*OPT_GAMMA, r);

	// OPT_CONTRAST:
	M_Print (16, 32 + 8*OPT_CONTRAST,	"              Contrast");
	r = vid_contrast.value - 1.0;
	M_DrawSlider (220, 32 + 8*OPT_CONTRAST, r);
	
	// OPT_MOUSESPEED:
	M_Print (16, 32 + 8*OPT_MOUSESPEED,	"           Mouse Speed");
	r = (sensitivity.value - 1)/10;
	M_DrawSlider (220, 32 + 8*OPT_MOUSESPEED, r);

	// OPT_SBALPHA:
	M_Print (16, 32 + 8*OPT_SBALPHA,	"       Statusbar alpha");
	r = (1.0 - scr_sbaralpha.value) ; // scr_sbaralpha range is 1.0 to 0.0
	M_DrawSlider (220, 32 + 8*OPT_SBALPHA, r);

	// OPT_SNDVOL:
	M_Print (16, 32 + 8*OPT_SNDVOL,		"          Sound Volume");
	r = sfxvolume.value;
	M_DrawSlider (220, 32 + 8*OPT_SNDVOL, r);

	// OPT_MUSICVOL:
	M_Print (16, 32 + 8*OPT_MUSICVOL,	"          Music Volume");
	r = bgmvolume.value;
	M_DrawSlider (220, 32 + 8*OPT_MUSICVOL, r);

	// OPT_MUSICEXT:
	M_Print (16, 32 + 8*OPT_MUSICEXT,	"        External Music");
	M_DrawCheckbox (220, 32 + 8*OPT_MUSICEXT, bgm_extmusic.value);

	// OPT_ALWAYRUN:
	M_Print (16, 32 + 8*OPT_ALWAYRUN,	"            Always Run");
	if (cl_alwaysrun.value)
		M_Print (220, 32 + 8*OPT_ALWAYRUN, "qs/power hop"); // woods
	else if (cl_forwardspeed.value > 200.0)
		M_Print (220, 32 + 8*OPT_ALWAYRUN, "vanilla");
	else
		M_Print (220, 32 + 8*OPT_ALWAYRUN, "off");

	// OPT_INVMOUSE:
	M_Print (16, 32 + 8*OPT_INVMOUSE,	"          Invert Mouse");
	M_DrawCheckbox (220, 32 + 8*OPT_INVMOUSE, m_pitch.value < 0);

	// OPT_ALWAYSMLOOK:
	M_Print (16, 32 + 8*OPT_ALWAYSMLOOK,	"            Mouse Look");
	M_DrawCheckbox (220, 32 + 8*OPT_ALWAYSMLOOK, in_mlook.state & 1);

	// OPT_LOOKSPRING:
	M_Print (16, 32 + 8*OPT_LOOKSPRING,	"            Lookspring");
	M_DrawCheckbox (220, 32 + 8*OPT_LOOKSPRING, lookspring.value);

	// OPT_LOOKSTRAFE:
	M_Print (16, 32 + 8*OPT_LOOKSTRAFE,	"            Lookstrafe");
	M_DrawCheckbox (220, 32 + 8*OPT_LOOKSTRAFE, lookstrafe.value);

	// OPT_EXTRAS:
	M_Print (16, 32 + 8*OPT_EXTRAS,	    "         Extra Options");

	// OPT_VIDEO:
	if (vid_menudrawfn)
		M_Print (16, 32 + 8*OPT_VIDEO,	"         Video Options");

// cursor
	M_DrawCharacter (200, 32 + options_cursor*8, 12+((int)(realtime*4)&1));
}


void M_Options_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
	case K_BBUTTON:
		M_Menu_Main_f ();
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
		m_entersound = true;
		switch (options_cursor)
		{
		case OPT_CUSTOMIZE:
			M_Menu_Keys_f ();
			break;
		case OPT_CONSOLE:
			m_state = m_none;
			Con_ToggleConsole_f ();
			break;
		case OPT_DEFAULTS:
			if (SCR_ModalMessage("This will reset all controls\n"
					"and stored cvars. Continue? (y/n)\n", 15.0f))
			{
				Cbuf_AddText ("resetcfg\n");
				Cbuf_AddText ("exec default.cfg\n");
			}
			break;
		case OPT_EXTRAS:
			M_Menu_Extras_f ();
			break;
		case OPT_VIDEO:
			M_Menu_Video_f ();
			break;
		default:
			M_AdjustSliders (1);
			break;
		}
		return;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		options_cursor--;
		if (options_cursor < 0)
			options_cursor = OPTIONS_ITEMS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		options_cursor++;
		if (options_cursor >= OPTIONS_ITEMS)
			options_cursor = 0;
		break;

	case K_LEFTARROW:
		M_AdjustSliders (-1);
		break;

	case K_RIGHTARROW:
		M_AdjustSliders (1);
		break;
	}

	if (options_cursor == OPTIONS_ITEMS - 1 && vid_menudrawfn == NULL)
	{
		if (k == K_UPARROW)
			options_cursor = OPTIONS_ITEMS - 2;
		else
			options_cursor = 0;
	}
}

//=============================================================================
/* KEYS MENU */

const char *quakebindnames[][2] =
{
	{"+attack",		"attack"},
	{"impulse 10",		"next weapon"},
	{"impulse 12",		"prev weapon"},
	{"+jump",		"jump / swim up"},
	{"+forward",		"walk forward"},
	{"+back",		"backpedal"},
	{"+left",		"turn left"},
	{"+right",		"turn right"},
	{"+speed",		"run"},
	{"+moveleft",		"step left"},
	{"+moveright",		"step right"},
	{"+strafe",		"sidestep"},
	{"+lookup",		"look up"},
	{"+lookdown",		"look down"},
	{"centerview",		"center view"},
	{"+mlook",		"mouse look"},
	{"+klook",		"keyboard look"},
	{"+moveup",		"swim up"},
	{"+movedown",		"swim down"},
	{"+voip",		"Voice Chat"}
};
#define	NUMQUAKECOMMANDS	(sizeof(quakebindnames)/sizeof(quakebindnames[0]))

static struct
{
	char *cmd;
	char *desc;
} *bindnames;
static size_t numbindnames;

static size_t	keys_first;
static size_t	keys_cursor;
qboolean	bind_grab;

void M_Keys_Close (void)
{
	while (numbindnames>0)
	{
		numbindnames--;
		Z_Free(bindnames[numbindnames].cmd);
		Z_Free(bindnames[numbindnames].desc);
	}
	Z_Free(bindnames);
	bindnames = NULL;
	numbindnames = 0;
}
void M_Keys_Populate(void)
{
	FILE *file;
	char line[1024];
	if (numbindnames)
		return;

	if (COM_FOpenFile("bindlist.lst", &file, NULL) >= 0)
	{
		while (fgets(line, sizeof(line), file))
		{
			const char *cmd, *desc/*, tip*/;
			Cmd_TokenizeString(line);
			cmd = Cmd_Argv(0);
			desc = Cmd_Argv(1);
			/*tip = Cmd_Argv(2); unused in quakespasm*/

			if (*cmd)
			{
				bindnames = Z_Realloc(bindnames, sizeof(*bindnames)*(numbindnames+1));
				bindnames[numbindnames].cmd = strcpy(Z_Malloc(strlen(cmd)+1), cmd);
				bindnames[numbindnames].desc = strcpy(Z_Malloc(strlen(desc)+1), desc);
				numbindnames++;
			}
		}
		fclose(file);
	}

	if (!numbindnames)
	{
		bindnames = Z_Realloc(bindnames, sizeof(*bindnames)*NUMQUAKECOMMANDS);
		while(numbindnames < NUMQUAKECOMMANDS)
		{
			bindnames[numbindnames].cmd = strcpy(Z_Malloc(strlen(quakebindnames[numbindnames][0])+1), quakebindnames[numbindnames][0]);
			bindnames[numbindnames].desc = strcpy(Z_Malloc(strlen(quakebindnames[numbindnames][1])+1), quakebindnames[numbindnames][1]);
			numbindnames++;
		}
	}

	//don't start with it on text
	keys_first = keys_cursor = 0;
	while (keys_cursor < numbindnames-1 && !strcmp(bindnames[keys_cursor].cmd, "-"))
		keys_cursor++;
}

void M_Menu_Keys_f (void)
{
	key_dest = key_menu;
	m_state = m_keys;
	m_entersound = true;
	IN_UpdateGrabs();

	M_Keys_Populate();
}


void M_FindKeysForCommand (const char *command, int *threekeys)
{
	int		count;
	int		j;
	int		l;
	char	*b;
	int		bindmap = 0;

	threekeys[0] = threekeys[1] = threekeys[2] = -1;
	l = strlen(command);
	count = 0;

	for (j = 0; j < MAX_KEYS; j++)
	{
		b = keybindings[bindmap][j];
		if (!b)
			continue;
		if (!strncmp (b, command, l) )
		{
			threekeys[count] = j;
			count++;
			if (count == 3)
				break;
		}
	}
}

void M_UnbindCommand (const char *command)
{
	int		j;
	int		l;
	char	*b;
	int		bindmap = 0;

	l = strlen(command);

	for (j = 0; j < MAX_KEYS; j++)
	{
		b = keybindings[bindmap][j];
		if (!b)
			continue;
		if (!strncmp (b, command, l) )
			Key_SetBinding (j, NULL, bindmap);
	}
}

extern qpic_t	*pic_up, *pic_down;

void M_Keys_Draw (void)
{
	size_t		i;
	int x, y;
	int		keys[3];
	const char	*name;
	qpic_t	*p;
	size_t keys_shown;

	p = Draw_CachePic ("gfx/ttl_cstm.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	if (bind_grab)
		M_Print (12, 32, "Press a key or button for this action");
	else
		M_Print (18, 32, "Enter to change, backspace to clear");

	keys_shown = numbindnames;
	if (keys_shown > (200-48)/8)
		keys_shown = (200-48)/8;
	if (keys_first+keys_shown-1 < keys_cursor)
		keys_first = keys_cursor-(keys_shown-1);
	if (keys_first > keys_cursor)
		keys_first = keys_cursor;

// search for known bindings
	for (i = keys_first; i < keys_first+keys_shown; i++)
	{
		qboolean active = (i == keys_cursor && bind_grab); // woods #invert color of selected (ironwail)
		void (*print_fn) (int cx, int cy, const char* text) = // woods #invert color of selected (ironwail)
			active ? M_PrintWhite : M_Print;
		
		y = 48 + 8*(i-keys_first);

		if (!strcmp(bindnames[i].cmd, "-"))
		{
			print_fn ((320-strlen(bindnames[i].desc)*8)/2, y, bindnames[i].desc); // woods #invert color of selected (ironwail)
			continue;
		}

		print_fn (16, y, bindnames[i].desc); // woods #invert (ironwail)

		M_FindKeysForCommand (bindnames[i].cmd, keys);

		if (i == bind_grab && keys[2] != -1) // woods #invert (ironwail)
			keys[0] = -1;

		if (keys[0] == -1)
		{
			print_fn (140, y, "???"); // woods #invert (ironwail)
		}
		else
		{
			name = Key_KeynumToString (keys[0]);
			print_fn (140, y, name); // woods #invert color of selected (ironwail)
			x = strlen(name) * 8;
			if (keys[1] != -1)
			{
				name = Key_KeynumToString (keys[1]);
				print_fn (140 + x + 8, y, "or"); // woods #invert (ironwail)
				print_fn (140 + x + 32, y, name); // woods #invert (ironwail)
				x = x + 32 + strlen(name) * 8;
				if (keys[2] != -1)
				{
					print_fn (140 + x + 8, y, "or"); // woods #invert (ironwail)
					print_fn (140 + x + 32, y, Key_KeynumToString (keys[2])); // woods #invert (ironwail)
				}
			}
		}
	}

	if (bind_grab)
		M_DrawCharacter (130, 48 + (keys_cursor-keys_first)*8, '=');
	else
		M_DrawCharacter (130, 48 + (keys_cursor-keys_first)*8, 12+((int)(realtime*4)&1));
}


void M_Keys_Key (int k)
{
	char	cmd[80];
	int		keys[3];

	if (bind_grab)
	{	// defining a key
		S_LocalSound ("misc/menu1.wav");
		if ((k != K_ESCAPE) && (k != '`'))
		{
			sprintf (cmd, "bind \"%s\" \"%s\"\n", Key_KeynumToString (k), bindnames[keys_cursor].cmd);
			Cbuf_InsertText (cmd);
		}

		bind_grab = false;
		IN_UpdateGrabs();
		return;
	}

	switch (k)
	{
	case K_ESCAPE:
	case K_BBUTTON:
		M_Menu_Options_f ();
		break;

	case K_LEFTARROW:
	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		do
		{
			keys_cursor--;
			if (keys_cursor >= numbindnames)
			{
				if (keys_first && strcmp(bindnames[keys_first].cmd, "-"))
				{	//some weirdness, so the user can re-view any leading titles
					keys_cursor = keys_first;
					keys_first = 0;
				}
				else
					keys_cursor = numbindnames-1;
				break;
			}
		} while (!strcmp(bindnames[keys_cursor].cmd, "-"));
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("misc/menu1.wav");
		do
		{
			keys_cursor++;
			if (keys_cursor >= numbindnames)
				keys_cursor = keys_first = 0;
			else if (keys_cursor == numbindnames-1)
				break;
		} while (!strcmp(bindnames[keys_cursor].cmd, "-"));
		break;

	case K_ENTER:		// go into bind mode
	case K_KP_ENTER:
	case K_ABUTTON:
		M_FindKeysForCommand (bindnames[keys_cursor].cmd, keys);
		S_LocalSound ("misc/menu2.wav");
		if (keys[2] != -1)
			M_UnbindCommand (bindnames[keys_cursor].cmd);
		bind_grab = true;
		IN_UpdateGrabs(); // activate to allow mouse key binding
		break;

	case K_BACKSPACE:	// delete bindings
	case K_DEL:
		S_LocalSound ("misc/menu2.wav");
		M_UnbindCommand (bindnames[keys_cursor].cmd);
		break;
	}
}

//=============================================================================
/* QSS's EXTRAS MENU */

static enum extras_e
{
	EXTRAS_FILTERING,
	EXTRAS_EXTERNALTEX,
	EXTRAS_REPLACEMENTMODELS,
	EXTRAS_MODELLERP,
	EXTRAS_FPSCAP,
	EXTRAS_YIELD,
	EXTRAS_DEMOREEL,
	EXTRAS_RENDERSCALE,
	EXTRAS_NETEXTENSIONS,
	EXTRAS_QCEXTENSIONS,
	EXTRAS_CLASSICPARTICLES,
	EXTRAS_AUDIORATE,
	EXTRAS_PREDICTION,
	EXTRAS_ITEMS
} extras_cursor;

void M_Menu_Extras_f (void)
{
	key_dest = key_menu;
	m_state = m_extras;
	m_entersound = true;

	IN_UpdateGrabs();
}


static void M_Extras_AdjustSliders (int dir)
{
	extern cvar_t pr_checkextension, r_replacemodels, gl_load24bit, cl_nopext, r_lerpmodels, r_lerpmove, host_maxfps, sys_throttle, r_particles, sv_nqplayerphysics, cl_nopred;
	int m;
	S_LocalSound ("misc/menu3.wav");

	switch (extras_cursor)
	{
	case EXTRAS_FILTERING:
		m = TexMgr_GetTextureMode() + dir;
		while (m == 3 || (m>4&&m<8) || (m>8&&m<16))
			m += dir;
		if (m < 0)
			m = 16;
		else if (m > 16)
			m = 0;
		if (m == 0)
		{
			Cvar_Set ("gl_texturemode", "nll");	//use linear minification filter to reduce distant noise without uglifying the visuals.
			Cvar_Set ("gl_texture_anisotropy", "1");
		}
		else
		{
			Cvar_Set ("gl_texturemode", "GL_LINEAR_MIPMAP_LINEAR");
			Cvar_SetValue ("gl_texture_anisotropy", m);
		}
		break;
	case EXTRAS_EXTERNALTEX:
		Cvar_SetValueQuick (&gl_load24bit, !gl_load24bit.value);
		Cbuf_AddText("flush\n");	//needs to be a vid_reload, but qs doesn't exactly do that nicely...
		break;
	case EXTRAS_REPLACEMENTMODELS:
		Cvar_SetQuick (&r_replacemodels, *r_replacemodels.string?"":"iqm md5mesh md3");
		Cbuf_AddText("flush\n");
		break;
	case EXTRAS_MODELLERP:
		if (r_lerpmodels.value || r_lerpmove.value)
		{
			Cvar_SetValueQuick(&r_lerpmodels, 0);
			Cvar_SetValueQuick(&r_lerpmove, 0);
		}
		else
		{
			Cvar_SetValueQuick(&r_lerpmodels, 1);
			Cvar_SetValueQuick(&r_lerpmove,  1);
		}
		break;
	case EXTRAS_FPSCAP:
		{
			static int caps[] = {30, 60, 72, 120, 144, 500, 0};
			int best = 0, bestdiff = INT_MAX, diff, i;
			for (i = 0; i < countof(caps); i++)
			{
				diff = abs((int)host_maxfps.value - caps[i]);
				if (diff < bestdiff)
				{
					bestdiff = diff;
					best = i;
				}
			}
			best += dir;
			best = CLAMP(0, best, countof(caps)-1);
			Cvar_SetValueQuick (&host_maxfps, caps[best]);
		}
		break;
	case EXTRAS_YIELD:
		Cvar_SetQuick (&sys_throttle, sys_throttle.value?"0":sys_throttle.default_string);
		break;
	case EXTRAS_DEMOREEL:
		m = cl_demoreel.value+dir;
		if (m < 0)
			m = 2;
		else if (m > 2)
			m = 0;
		Cvar_SetValueQuick (&cl_demoreel, m);
		break;
	case EXTRAS_RENDERSCALE:
		m = r_scale.value-dir;
		m = CLAMP(1, m, 4);
		Cvar_SetValueQuick(&r_scale, m);
		break;
	case EXTRAS_NETEXTENSIONS:
		Cvar_SetValueQuick (&cl_nopext, !cl_nopext.value);
		break;
	case EXTRAS_QCEXTENSIONS:
		Cvar_SetValueQuick (&pr_checkextension, !pr_checkextension.value);
		break;
	case EXTRAS_CLASSICPARTICLES:
		Cvar_SetValueQuick (&r_particles, (r_particles.value==1)?2:1);
		break;
	case EXTRAS_AUDIORATE:
		Cvar_SetValueQuick (&snd_mixspeed, (snd_mixspeed.value==48000)?44100:48000);
	//	Cbuf_AddText("\nsnd_restart\n");
		break;
	case EXTRAS_PREDICTION:
		m = ((!!cl_nopred.value)<<1)|(!!sv_nqplayerphysics.value);
		m += dir;
		if ((m&3)==2)
			m += dir; //boo! don't like that combo. skip it
		m &= 3;
		Cvar_SetValueQuick (&cl_nopred, (m>>1)&1);
		Cvar_SetValueQuick (&sv_nqplayerphysics, (m>>0)&1);
		break;
	case EXTRAS_ITEMS:	//not a real option
		break;
	}
}

void M_Extras_Draw (void)
{
	extern cvar_t pr_checkextension, r_replacemodels, gl_load24bit, cl_nopext, r_lerpmodels, r_lerpmove, host_maxfps, sys_throttle, r_particles, sv_nqplayerphysics, cl_nopred;
	int m;
	qpic_t	*p;
	enum extras_e i;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_option.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	for (i = 0; i < EXTRAS_ITEMS; i++)
	{
		int y = 32 + 8*i;
		switch(i)
		{
		case EXTRAS_FILTERING:
			M_Print (16, y,	"     Texture Filtering");
			m = TexMgr_GetTextureMode();
			switch(m)
			{
			case 0:
				M_Print (220, 32 + 8*i, "nearest");
				break;
			case 1:
				M_Print (220, 32 + 8*i, "linear");
				break;
			default:
				M_Print (220, 32 + 8*i, va("anisotropic %i", m));
				break;
			}
			break;
		case EXTRAS_EXTERNALTEX:
			M_Print (16, y,	"  Replacement Textures");
			M_DrawCheckbox (220, y, !!gl_load24bit.value);
			break;
		case EXTRAS_REPLACEMENTMODELS:
			M_Print (16, y,	"    Replacement Models");
			M_DrawCheckbox (220, y, !!*r_replacemodels.string);
			break;
		case EXTRAS_MODELLERP:
			M_Print (16, y,	"            Model Lerp");
			M_DrawCheckbox(220, y, !!r_lerpmodels.value && !!r_lerpmove.value);
			break;
		case EXTRAS_FPSCAP:
			if (host_maxfps.value < 0)
				M_Print (16, y,	"           Maximum PPS");
			else
				M_Print (16, y,	"           Maximum FPS");
			if (host_maxfps.value)
				M_Print (220, y, va("%g", fabs(host_maxfps.value)));
			else
				M_Print (220, y, "uncapped");
			break;
		case EXTRAS_YIELD:
			M_Print (16, y,	"  Sleep Between Frames");
			if (sys_throttle.value)
				M_Print (220, y, "on");
			else
				M_Print (220, y, "off");
			break;
		case EXTRAS_DEMOREEL:
			M_Print (16, y,	"          Attract Mode");
			if (cl_demoreel.value>1)
				M_Print (220, y, "on");
			else if (cl_demoreel.value)
				M_Print (220, y, "startup only");
			else
				M_Print (220, y, "off");
			break;
		case EXTRAS_RENDERSCALE:
			M_Print (16, y,	"          Render Scale");
			if (r_scale.value==1)
				M_Print (220, y, "native");
			else
				M_Print (220, y, va("1/%g", r_scale.value));
			break;
		case EXTRAS_NETEXTENSIONS:
			M_Print (16, y,	"   Protocol Extensions");
			M_Print (220, y, cl_nopext.value?"blocked":"enabled");
			break;
		case EXTRAS_QCEXTENSIONS:
			M_Print (16, y,	"         QC Extensions");
			M_Print (220, y, pr_checkextension.value?"enabled":"blocked");
			break;

		case EXTRAS_CLASSICPARTICLES:
			M_Print (16, y,	"     Classic Particles");
			if (r_particles.value == 1)
				M_Print (220, y, "disabled");
			else if (r_particles.value == 1)
				M_Print (220, y, "round");
			else if (r_particles.value == 2)
				M_Print (220, y, "square");
			else
				M_Print (220, y, "?!?");
			break;

		case EXTRAS_AUDIORATE:
			M_Print (16, y,	"            Audio Rate");
			if (snd_mixspeed.value == 48000)
				M_Print (220, y, "48000 hz (DVD)");
			else if (r_particles.value == 1)
				M_Print (220, y, "44100 hz (CD)");
			else
				M_Print (220, y, va("%i hz", (int)snd_mixspeed.value));
			break;

		case EXTRAS_PREDICTION:
			M_Print (16, y,	"    Prediction/Physics");
			if (!cl_nopred.value && !sv_nqplayerphysics.value)
				M_Print (220, y, "on (override ssqc)");	//deathmatch! will break quirky mods like quakerally.
			else if (!cl_nopred.value && sv_nqplayerphysics.value)
				M_Print (220, y, "on (compatible physics)");	//conservative / default setting.
			else if (cl_nopred.value && !sv_nqplayerphysics.value)
				M_Print (220, y, "off (but override ssqc)"); //silly setting (skipped when changing in menu)
			else
				M_Print (220, y, "off");	//honest/oldskool setting.
			break;

		case EXTRAS_ITEMS:	//unreachable.
			break;
		}
	}

// cursor
	M_DrawCharacter (200, 32 + extras_cursor*8, 12+((int)(realtime*4)&1));
}


void M_Extras_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
	case K_BBUTTON:
		M_Menu_Main_f ();
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
		m_entersound = true;
		switch (extras_cursor)
		{
		default:
			M_Extras_AdjustSliders (1);
			break;
		}
		return;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		if (extras_cursor <= 0)
			extras_cursor = EXTRAS_ITEMS-1;
		else
			extras_cursor--;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		extras_cursor++;
		if (extras_cursor >= EXTRAS_ITEMS)
			extras_cursor = 0;
		break;

	case K_LEFTARROW:
		M_Extras_AdjustSliders (-1);
		break;

	case K_RIGHTARROW:
		M_Extras_AdjustSliders (1);
		break;
	}
}

//=============================================================================
/* VIDEO MENU */

void M_Menu_Video_f (void)
{
	(*vid_menucmdfn) (); //johnfitz
}


void M_Video_Draw (void)
{
	(*vid_menudrawfn) ();
}


void M_Video_Key (int key)
{
	(*vid_menukeyfn) (key);
}

//=============================================================================
/* HELP MENU */

int		help_page;
#define	NUM_HELP_PAGES	6


void M_Menu_Help_f (void)
{
	key_dest = key_menu;
	m_state = m_help;
	m_entersound = true;
	help_page = 0;
	IN_UpdateGrabs();
	SDL_OpenURL("https://qssm.quakeone.com");
}



void M_Help_Draw (void)
{
	M_DrawPic (0, 0, Draw_CachePic ( va("gfx/help%i.lmp", help_page)) );
}


void M_Help_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case K_BBUTTON:
		M_Menu_Main_f ();
		break;

	case K_UPARROW:
	case K_RIGHTARROW:
		m_entersound = true;
		if (++help_page >= NUM_HELP_PAGES)
			help_page = 0;
		break;

	case K_DOWNARROW:
	case K_LEFTARROW:
		m_entersound = true;
		if (--help_page < 0)
			help_page = NUM_HELP_PAGES-1;
		break;
	}

}

//=============================================================================
/* QUIT MENU */

int		msgNumber;
enum m_state_e	m_quit_prevstate;
qboolean	wasInMenus;

void M_Menu_Quit_f (void)
{
	if (m_state == m_quit)
		return;
	wasInMenus = (key_dest == key_menu);
	key_dest = key_menu;
	m_quit_prevstate = m_state;
	m_state = m_quit;
	m_entersound = true;
	msgNumber = rand()&7;

	IN_UpdateGrabs();
}


void M_Quit_Key (int key)
{
	if (key == K_ESCAPE)
	{
		if (wasInMenus)
		{
			m_state = m_quit_prevstate;
			m_entersound = true;
		}
		else
		{
			key_dest = key_game;
			m_state = m_none;
			IN_UpdateGrabs();
		}
	}
}


void M_Quit_Char (int key)
{
	switch (key)
	{
	case 'n':
	case 'N':
		if (wasInMenus)
		{
			m_state = m_quit_prevstate;
			m_entersound = true;
		}
		else
		{
			key_dest = key_game;
			m_state = m_none;
			IN_UpdateGrabs();
		}
		break;

	case 'y':
	case 'Y':
		key_dest = key_console;
		Host_Quit_f ();
		IN_UpdateGrabs();
		break;

	default:
		break;
	}

}


qboolean M_Quit_TextEntry (void)
{
	return true;
}


void M_Quit_Draw (void) //johnfitz -- modified for new quit message -- woods modified for match quit warning #matchquit
{
	char	msg1[] = "you are currently a match participant";
	char	msg2[] = "quiting will disrupt the match"; /* msg2/msg3 are [38] at most */
	char	msg3[] = "press y to quit";
	int		boxlen;

	if (wasInMenus)
	{
		m_state = m_quit_prevstate;
		m_recursiveDraw = true;
		M_Draw ();
		m_state = m_quit;
	}

	//okay, this is kind of fucked up.  M_DrawTextBox will always act as if
	//width is even. Also, the width and lines values are for the interior of the box,
	//but the x and y values include the border.
	boxlen = (q_max(sizeof(msg1), q_max(sizeof(msg2),sizeof(msg3))) + 1) & ~1;
	M_DrawTextBox	(160-4*(boxlen+2), 76, boxlen, 4);

	//now do the text
	M_Print			(160-4*(sizeof(msg1)-1), 88, msg1);
	M_Print			(160-4*(sizeof(msg2)-1), 96, msg2);
	M_PrintWhite		(160-4*(sizeof(msg3)-1), 104, msg3);
}

//=============================================================================
/* LAN CONFIG MENU */

int		lanConfig_cursor = -1;
#define NUM_LANCONFIG_CMDS	4

int 	lanConfig_port;
char	lanConfig_portname[6];
char	lanConfig_joinname[22];

void M_Menu_LanConfig_f (void)
{
	key_dest = key_menu;
	m_state = m_lanconfig;
	m_entersound = true;
	if (lanConfig_cursor == -1)
	{
		if (JoiningGame && TCPIPConfig)
			lanConfig_cursor = 2;
		else
			lanConfig_cursor = 1;
	}
	if (StartingGame && lanConfig_cursor >= 2)
		lanConfig_cursor = 1;
	lanConfig_port = DEFAULTnet_hostport;
	sprintf(lanConfig_portname, "%u", lanConfig_port);

	m_return_onerror = false;
	m_return_reason[0] = 0;
	IN_UpdateGrabs();
}


void M_LanConfig_Draw (void)
{
	qpic_t	*p;
	int		basex;
	int		y;
	int		numaddresses, i;
	qhostaddr_t addresses[16];
	const char	*startJoin;
	const char	*protocol;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	basex = (320-p->width)/2;
	M_DrawPic (basex, 4, p);

	basex = 72; /* Arcane Dimensions has an oversized gfx/p_multi.lmp */

	if (StartingGame)
		startJoin = "New Game";
	else
		startJoin = "Join Game";
	/*if (IPXConfig) // woods #skipipx
		protocol = "IPX";
	else*/
	protocol = "TCP/IP";
	M_Print (basex, 32, va ("%s - %s", startJoin, protocol));
	basex += 8;

	y = 52;
	M_Print (basex, y, "Address:");
#if 1
	numaddresses = NET_ListAddresses(addresses, sizeof(addresses)/sizeof(addresses[0]));
	if (!numaddresses)
	{
		M_Print (basex+9*8, y, "NONE KNOWN");
		y += 8;
	}
	else for (i = 0; i < numaddresses; i++)
	{
		M_Print (basex+9*8, y, addresses[i]);
		y += 8;
	}
#else
	if (IPXConfig)
	{
		M_Print (basex+9*8, y, my_ipx_address);
		y+=8;
	}
	else
	{
		if (ipv4Available && ipv6Available)
		{
			M_Print (basex+9*8, y, my_ipv4_address);
			y+=8;
			M_Print (basex+9*8, y, my_ipv6_address);
			y+=8;
		}
		else
		{
			if (ipv4Available)
				M_Print (basex+9*8, y, my_ipv4_address);
			if (ipv6Available)
				M_Print (basex+9*8, y, my_ipv6_address);
			y+=8;
		}
	}
#endif

	y+=8;	//for the port's box
	M_Print (basex, y, "Port");
	M_DrawTextBox (basex+8*8, y-8, 6, 1);
	M_Print (basex+9*8, y, lanConfig_portname);
	if (lanConfig_cursor == 0)
	{
		M_DrawCharacter (basex+9*8 + 8*strlen(lanConfig_portname), y, 10+((int)(realtime*4)&1));
		M_DrawCharacter (basex-8, y, 12+((int)(realtime*4)&1));
	}
	y += 20;

	if (JoiningGame)
	{
		M_Print (basex, y, "Search for local games...");
		if (lanConfig_cursor == 1)
			M_DrawCharacter (basex-8, y, 12+((int)(realtime*4)&1));
		y+=8;

		M_Print (basex, y, "Search for public games...");
		if (lanConfig_cursor == 2)
			M_DrawCharacter (basex-8, y, 12+((int)(realtime*4)&1));
		y+=8;

		M_Print (basex, y, "Join game at:");
		y+=24;
		M_DrawTextBox (basex+8, y-8, 22, 1);
		M_Print (basex+16, y, lanConfig_joinname);
		if (lanConfig_cursor == 3)
		{
			M_DrawCharacter (basex+16 + 8*strlen(lanConfig_joinname), y, 10+((int)(realtime*4)&1));
			M_DrawCharacter (basex-8, y, 12+((int)(realtime*4)&1));
		}
		y += 16;
	}
	else
	{
		M_DrawTextBox (basex, y-8, 2, 1);
		M_Print (basex+8, y, "OK");
		if (lanConfig_cursor == 1)
			M_DrawCharacter (basex-8, y, 12+((int)(realtime*4)&1));
		y += 16;
	}

	if (*m_return_reason)
		M_PrintWhite (basex, 148, m_return_reason);
}


void M_LanConfig_Key (int key)
{
	int		l;

	switch (key)
	{
	case K_ESCAPE:
	case K_BBUTTON:
		M_Menu_MultiPlayer_f (); // woods #skipipx
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		lanConfig_cursor--;
		if (lanConfig_cursor < 0)
			lanConfig_cursor = NUM_LANCONFIG_CMDS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		lanConfig_cursor++;
		if (lanConfig_cursor >= NUM_LANCONFIG_CMDS)
			lanConfig_cursor = 0;
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
		if (lanConfig_cursor == 0)
			break;

		m_entersound = true;

		M_ConfigureNetSubsystem ();

		if (StartingGame)
		{
			if (lanConfig_cursor == 1)
				M_Menu_GameOptions_f ();
		}
		else
		{
			if (lanConfig_cursor == 1)
				M_Menu_Search_f(SLIST_LAN);
			else if (lanConfig_cursor == 2)
				M_Menu_Search_f(SLIST_INTERNET);
			else if (lanConfig_cursor == 3)
			{
				m_return_state = m_state;
				m_return_onerror = true;
				key_dest = key_game;
				m_state = m_none;
				IN_UpdateGrabs();
				Cbuf_AddText ( va ("connect \"%s\"\n", lanConfig_joinname) );
			}
		}

		break;

	case K_BACKSPACE:
		if (lanConfig_cursor == 0)
		{
			if (strlen(lanConfig_portname))
				lanConfig_portname[strlen(lanConfig_portname)-1] = 0;
		}

		if (lanConfig_cursor == 3)
		{
			if (strlen(lanConfig_joinname))
				lanConfig_joinname[strlen(lanConfig_joinname)-1] = 0;
		}
		break;
	}

	if (StartingGame && lanConfig_cursor >= 2)
	{
		if (key == K_UPARROW)
			lanConfig_cursor = 1;
		else
			lanConfig_cursor = 0;
	}

	l =  Q_atoi(lanConfig_portname);
	if (l > 65535)
		l = lanConfig_port;
	else
		lanConfig_port = l;
	sprintf(lanConfig_portname, "%u", lanConfig_port);
}


void M_LanConfig_Char (int key)
{
	int l;

	switch (lanConfig_cursor)
	{
	case 0:
		if (key < '0' || key > '9')
			return;
		l = strlen(lanConfig_portname);
		if (l < 5)
		{
			lanConfig_portname[l+1] = 0;
			lanConfig_portname[l] = key;
		}
		break;
	case 3:
		l = strlen(lanConfig_joinname);
		if (l < 21)
		{
			lanConfig_joinname[l+1] = 0;
			lanConfig_joinname[l] = key;
		}
		break;
	}
}


qboolean M_LanConfig_TextEntry (void)
{
	return (lanConfig_cursor == 0 || lanConfig_cursor == 3);
}

//=============================================================================
/* GAME OPTIONS MENU */

typedef struct
{
	const char	*name;
	const char	*description;
} level_t;

level_t		levels[] =
{
	{"start", "Entrance"},	// 0

	{"e1m1", "Slipgate Complex"},				// 1
	{"e1m2", "Castle of the Damned"},
	{"e1m3", "The Necropolis"},
	{"e1m4", "The Grisly Grotto"},
	{"e1m5", "Gloom Keep"},
	{"e1m6", "The Door To Chthon"},
	{"e1m7", "The House of Chthon"},
	{"e1m8", "Ziggurat Vertigo"},

	{"e2m1", "The Installation"},				// 9
	{"e2m2", "Ogre Citadel"},
	{"e2m3", "Crypt of Decay"},
	{"e2m4", "The Ebon Fortress"},
	{"e2m5", "The Wizard's Manse"},
	{"e2m6", "The Dismal Oubliette"},
	{"e2m7", "Underearth"},

	{"e3m1", "Termination Central"},			// 16
	{"e3m2", "The Vaults of Zin"},
	{"e3m3", "The Tomb of Terror"},
	{"e3m4", "Satan's Dark Delight"},
	{"e3m5", "Wind Tunnels"},
	{"e3m6", "Chambers of Torment"},
	{"e3m7", "The Haunted Halls"},

	{"e4m1", "The Sewage System"},				// 23
	{"e4m2", "The Tower of Despair"},
	{"e4m3", "The Elder God Shrine"},
	{"e4m4", "The Palace of Hate"},
	{"e4m5", "Hell's Atrium"},
	{"e4m6", "The Pain Maze"},
	{"e4m7", "Azure Agony"},
	{"e4m8", "The Nameless City"},

	{"end", "Shub-Niggurath's Pit"},			// 31

	{"dm1", "Place of Two Deaths"},				// 32
	{"dm2", "Claustrophobopolis"},
	{"dm3", "The Abandoned Base"},
	{"dm4", "The Bad Place"},
	{"dm5", "The Cistern"},
	{"dm6", "The Dark Zone"}
};

//MED 01/06/97 added hipnotic levels
level_t     hipnoticlevels[] =
{
	{"start", "Command HQ"},	// 0

	{"hip1m1", "The Pumping Station"},			// 1
	{"hip1m2", "Storage Facility"},
	{"hip1m3", "The Lost Mine"},
	{"hip1m4", "Research Facility"},
	{"hip1m5", "Military Complex"},

	{"hip2m1", "Ancient Realms"},				// 6
	{"hip2m2", "The Black Cathedral"},
	{"hip2m3", "The Catacombs"},
	{"hip2m4", "The Crypt"},
	{"hip2m5", "Mortum's Keep"},
	{"hip2m6", "The Gremlin's Domain"},

	{"hip3m1", "Tur Torment"},				// 12
	{"hip3m2", "Pandemonium"},
	{"hip3m3", "Limbo"},
	{"hip3m4", "The Gauntlet"},

	{"hipend", "Armagon's Lair"},				// 16

	{"hipdm1", "The Edge of Oblivion"}			// 17
};

//PGM 01/07/97 added rogue levels
//PGM 03/02/97 added dmatch level
level_t		roguelevels[] =
{
	{"start",	"Split Decision"},
	{"r1m1",	"Deviant's Domain"},
	{"r1m2",	"Dread Portal"},
	{"r1m3",	"Judgement Call"},
	{"r1m4",	"Cave of Death"},
	{"r1m5",	"Towers of Wrath"},
	{"r1m6",	"Temple of Pain"},
	{"r1m7",	"Tomb of the Overlord"},
	{"r2m1",	"Tempus Fugit"},
	{"r2m2",	"Elemental Fury I"},
	{"r2m3",	"Elemental Fury II"},
	{"r2m4",	"Curse of Osiris"},
	{"r2m5",	"Wizard's Keep"},
	{"r2m6",	"Blood Sacrifice"},
	{"r2m7",	"Last Bastion"},
	{"r2m8",	"Source of Evil"},
	{"ctf1",    "Division of Change"}
};

typedef struct
{
	const char	*description;
	int		firstLevel;
	int		levels;
} episode_t;

episode_t	episodes[] =
{
	{"Welcome to Quake", 0, 1},
	{"Doomed Dimension", 1, 8},
	{"Realm of Black Magic", 9, 7},
	{"Netherworld", 16, 7},
	{"The Elder World", 23, 8},
	{"Final Level", 31, 1},
	{"Deathmatch Arena", 32, 6}
};

//MED 01/06/97  added hipnotic episodes
episode_t   hipnoticepisodes[] =
{
	{"Scourge of Armagon", 0, 1},
	{"Fortress of the Dead", 1, 5},
	{"Dominion of Darkness", 6, 6},
	{"The Rift", 12, 4},
	{"Final Level", 16, 1},
	{"Deathmatch Arena", 17, 1}
};

//PGM 01/07/97 added rogue episodes
//PGM 03/02/97 added dmatch episode
episode_t	rogueepisodes[] =
{
	{"Introduction", 0, 1},
	{"Hell's Fortress", 1, 7},
	{"Corridors of Time", 8, 8},
	{"Deathmatch Arena", 16, 1}
};

extern cvar_t sv_public;

int	startepisode;
int	startlevel;
int maxplayers;

void M_Menu_GameOptions_f (void)
{
	key_dest = key_menu;
	m_state = m_gameoptions;
	IN_UpdateGrabs();
	m_entersound = true;
	if (maxplayers == 0)
		maxplayers = svs.maxclients;
	if (maxplayers < 2)
		maxplayers = 16;
}


int gameoptions_cursor_table[] = {40, 56, 64, 72, 80, 88, 96, 104, 120, 128};
#define	NUM_GAMEOPTIONS	10
int		gameoptions_cursor;

void M_GameOptions_Draw (void)
{
	qpic_t	*p;
	int y = 40;

	M_DrawTransPic (16, 4, Draw_CachePic ("gfx/qplaque.lmp") );
	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);

	M_DrawTextBox (152, y-8, 10, 1);
	M_Print (160, y, "begin game");
	y+=16;

	M_Print (0, y, "      Max players");
	M_Print (160, y, va("%i", maxplayers) );
	y+=8;

	M_Print (0, y, "           Public");
	if (sv_public.value)
		M_Print (160, y, "Yes");
	else
		M_Print (160, y, "No");
	y+=8;

	M_Print (0, y, "        Game Type");
	if (coop.value)
		M_Print (160, y, "Cooperative");
	else
		M_Print (160, y, "Deathmatch");
	y+=8;

	M_Print (0, y, "        Teamplay");
	if (rogue)
	{
		const char *msg;

		switch((int)teamplay.value)
		{
			case 1: msg = "No Friendly Fire"; break;
			case 2: msg = "Friendly Fire"; break;
			case 3: msg = "Tag"; break;
			case 4: msg = "Capture the Flag"; break;
			case 5: msg = "One Flag CTF"; break;
			case 6: msg = "Three Team CTF"; break;
			default: msg = "Off"; break;
		}
		M_Print (160, y, msg);
	}
	else
	{
		const char *msg;

		switch((int)teamplay.value)
		{
			case 1: msg = "No Friendly Fire"; break;
			case 2: msg = "Friendly Fire"; break;
			default: msg = "Off"; break;
		}
		M_Print (160, y, msg);
	}
	y+=8;

	M_Print (0, y, "            Skill");
	if (skill.value == 0)
		M_Print (160, y, "Easy difficulty");
	else if (skill.value == 1)
		M_Print (160, y, "Normal difficulty");
	else if (skill.value == 2)
		M_Print (160, y, "Hard difficulty");
	else
		M_Print (160, y, "Nightmare difficulty");
	y+=8;

	M_Print (0, y, "       Frag Limit");
	if (fraglimit.value == 0)
		M_Print (160, y, "none");
	else
		M_Print (160, y, va("%i frags", (int)fraglimit.value));
	y+=8;

	M_Print (0, y, "       Time Limit");
	if (timelimit.value == 0)
		M_Print (160, y, "none");
	else
		M_Print (160, y, va("%i minutes", (int)timelimit.value));
	y+=8;

	y+=8;

	M_Print (0, y, "         Episode");
	// MED 01/06/97 added hipnotic episodes
	if (hipnotic)
		M_Print (160, y, hipnoticepisodes[startepisode].description);
	// PGM 01/07/97 added rogue episodes
	else if (rogue)
		M_Print (160, y, rogueepisodes[startepisode].description);
	else
		M_Print (160, y, episodes[startepisode].description);
	y+=8;

	M_Print (0, y, "           Level");
	// MED 01/06/97 added hipnotic episodes
	if (hipnotic)
	{
		M_Print (160, y, hipnoticlevels[hipnoticepisodes[startepisode].firstLevel + startlevel].description);
		M_Print (160, y+8, hipnoticlevels[hipnoticepisodes[startepisode].firstLevel + startlevel].name);
	}
	// PGM 01/07/97 added rogue episodes
	else if (rogue)
	{
		M_Print (160, y, roguelevels[rogueepisodes[startepisode].firstLevel + startlevel].description);
		M_Print (160, y+8, roguelevels[rogueepisodes[startepisode].firstLevel + startlevel].name);
	}
	else
	{
		M_Print (160, y, levels[episodes[startepisode].firstLevel + startlevel].description);
		M_Print (160, y+8, levels[episodes[startepisode].firstLevel + startlevel].name);
	}
	y+=8;

// line cursor
	M_DrawCharacter (144, gameoptions_cursor_table[gameoptions_cursor], 12+((int)(realtime*4)&1));
}


void M_NetStart_Change (int dir)
{
	int count;
	float	f;

	switch (gameoptions_cursor)
	{
	case 1:
		maxplayers += dir;
		if (maxplayers > svs.maxclientslimit)
			maxplayers = svs.maxclientslimit;
		if (maxplayers < 2)
			maxplayers = 2;
		break;

	case 2:
		Cvar_SetQuick (&sv_public, sv_public.value ? "0" : "1");
		break;

	case 3:
		Cvar_Set ("coop", coop.value ? "0" : "1");
		break;

	case 4:
		count = (rogue) ? 6 : 2;
		f = teamplay.value + dir;
		if (f > count)	f = 0;
		else if (f < 0)	f = count;
		Cvar_SetValue ("teamplay", f);
		break;

	case 5:
		f = skill.value + dir;
		if (f > 3)	f = 0;
		else if (f < 0)	f = 3;
		Cvar_SetValue ("skill", f);
		break;

	case 6:
		f = fraglimit.value + dir * 10;
		if (f > 100)	f = 0;
		else if (f < 0)	f = 100;
		Cvar_SetValue ("fraglimit", f);
		break;

	case 7:
		f = timelimit.value + dir * 5;
		if (f > 60)	f = 0;
		else if (f < 0)	f = 60;
		Cvar_SetValue ("timelimit", f);
		break;

	case 8:
		startepisode += dir;
	//MED 01/06/97 added hipnotic count
		if (hipnotic)
			count = 6;
	//PGM 01/07/97 added rogue count
	//PGM 03/02/97 added 1 for dmatch episode
		else if (rogue)
			count = 4;
		else if (registered.value)
			count = 7;
		else
			count = 2;

		if (startepisode < 0)
			startepisode = count - 1;

		if (startepisode >= count)
			startepisode = 0;

		startlevel = 0;
		break;

	case 9:
		startlevel += dir;
	//MED 01/06/97 added hipnotic episodes
		if (hipnotic)
			count = hipnoticepisodes[startepisode].levels;
	//PGM 01/06/97 added hipnotic episodes
		else if (rogue)
			count = rogueepisodes[startepisode].levels;
		else
			count = episodes[startepisode].levels;

		if (startlevel < 0)
			startlevel = count - 1;

		if (startlevel >= count)
			startlevel = 0;
		break;
	}
}

void M_GameOptions_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
	case K_BBUTTON:
		M_Menu_MultiPlayer_f (); // woods #skipipx
		break;

	case K_UPARROW:
		S_LocalSound ("misc/menu1.wav");
		gameoptions_cursor--;
		if (gameoptions_cursor < 0)
			gameoptions_cursor = NUM_GAMEOPTIONS-1;
		break;

	case K_DOWNARROW:
		S_LocalSound ("misc/menu1.wav");
		gameoptions_cursor++;
		if (gameoptions_cursor >= NUM_GAMEOPTIONS)
			gameoptions_cursor = 0;
		break;

	case K_LEFTARROW:
	case K_MWHEELDOWN:
	//case K_MOUSE2:
		if (gameoptions_cursor == 0)
			break;
		S_LocalSound ("misc/menu3.wav");
		M_NetStart_Change (-1);
		break;

	case K_RIGHTARROW:
	case K_MWHEELUP:
		if (gameoptions_cursor == 0)
			break;
		S_LocalSound ("misc/menu3.wav");
		M_NetStart_Change (1);
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
		S_LocalSound ("misc/menu2.wav");
		if (gameoptions_cursor == 0)
		{
			if (sv.active)
				Cbuf_AddText ("disconnect\n");
			Cbuf_AddText ("listen 0\n");	// so host_netport will be re-examined
			Cbuf_AddText ( va ("maxplayers %u\n", maxplayers) );
			SCR_BeginLoadingPlaque ();

			if (hipnotic)
				Cbuf_AddText ( va ("map %s\n", hipnoticlevels[hipnoticepisodes[startepisode].firstLevel + startlevel].name) );
			else if (rogue)
				Cbuf_AddText ( va ("map %s\n", roguelevels[rogueepisodes[startepisode].firstLevel + startlevel].name) );
			else
				Cbuf_AddText ( va ("map %s\n", levels[episodes[startepisode].firstLevel + startlevel].name) );

			return;
		}

		M_NetStart_Change (1);
		break;
	}
}

//=============================================================================
/* SEARCH MENU */

qboolean	searchComplete = false;
double		searchCompleteTime;
enum slistScope_e searchLastScope = SLIST_LAN;

void M_Menu_Search_f (enum slistScope_e scope)
{
	key_dest = key_menu;
	m_state = m_search;
	IN_UpdateGrabs();
	m_entersound = false;
	slistSilent = true;
	slistScope = searchLastScope = scope;
	searchComplete = false;
	NET_Slist_f();

}


void M_Search_Draw (void)
{
	qpic_t	*p;
	int x;

	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);
	x = (320/2) - ((12*8)/2) + 4;
	M_DrawTextBox (x-8, 32, 12, 1);
	M_Print (x, 40, "Searching...");

	if(slistInProgress)
	{
		NET_Poll();
		return;
	}

	if (! searchComplete)
	{
		searchComplete = true;
		searchCompleteTime = realtime;
	}

	if (hostCacheCount)
	{
		M_Menu_ServerList_f ();
		return;
	}

	M_PrintWhite ((320/2) - ((22*8)/2), 64, "No Quake servers found");
	if ((realtime - searchCompleteTime) < 3.0)
		return;

	M_Menu_LanConfig_f ();
}


void M_Search_Key (int key)
{
}

//=============================================================================
/* SLIST MENU */

size_t		slist_cursor;
size_t		slist_first;
qboolean slist_sorted;

void M_Menu_ServerList_f (void)
{
	key_dest = key_menu;
	m_state = m_slist;
	IN_UpdateGrabs();
	m_entersound = true;
	slist_cursor = 0;
	slist_first = 0;
	m_return_onerror = false;
	m_return_reason[0] = 0;
	slist_sorted = false;
}


void M_ServerList_Draw (void)
{
	size_t	n, slist_shown;
	qpic_t	*p;

	if (!slist_sorted)
	{
		slist_sorted = true;
		NET_SlistSort ();
	}

	slist_shown = hostCacheCount;
	if (slist_shown > (200-32)/8)
		slist_shown = (200-32)/8;
	if (slist_first+slist_shown-1 < slist_cursor)
		slist_first = slist_cursor-(slist_shown-1);
	if (slist_first > slist_cursor)
		slist_first = slist_cursor;

	p = Draw_CachePic ("gfx/p_multi.lmp");
	M_DrawPic ( (320-p->width)/2, 4, p);
	for (n = 0; n < slist_shown; n++)
		M_Print (16, 32 + 8*n, NET_SlistPrintServer (slist_first+n));
	M_DrawCharacter (0, 32 + (slist_cursor-slist_first)*8, 12+((int)(realtime*4)&1));

	if (*m_return_reason)
		M_PrintWhite (16, 148, m_return_reason);
}


void M_ServerList_Key (int k)
{
	switch (k)
	{
	case K_ESCAPE:
	case K_BBUTTON:
		M_Menu_LanConfig_f ();
		break;

	case K_SPACE:
		M_Menu_Search_f (searchLastScope);
		break;

	case K_UPARROW:
	case K_LEFTARROW:
		S_LocalSound ("misc/menu1.wav");
		slist_cursor--;
		if (slist_cursor >= hostCacheCount)
			slist_cursor = hostCacheCount - 1;
		break;

	case K_DOWNARROW:
	case K_RIGHTARROW:
		S_LocalSound ("misc/menu1.wav");
		slist_cursor++;
		if (slist_cursor >= hostCacheCount)
			slist_cursor = 0;
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
		S_LocalSound ("misc/menu2.wav");
		m_return_state = m_state;
		m_return_onerror = true;
		slist_sorted = false;
		key_dest = key_game;
		m_state = m_none;
		IN_UpdateGrabs();
		Cbuf_AddText ( va ("connect \"%s\"\n", NET_SlistPrintServerName(slist_cursor)) );
		break;

	default:
		break;
	}

}


//=============================================================================

// woods #modsmenu (iw)

/* Listbox */

typedef struct
{
	int			cursor;
	int			numitems;
	int			viewsize;
	int			scroll;
} menulist_t;

void M_List_CheckIntegrity(const menulist_t* list)
{
	SDL_assert(list->numitems >= 0);
	SDL_assert(list->cursor >= 0);
	SDL_assert(list->cursor < list->numitems);
	SDL_assert(list->scroll >= 0);
	SDL_assert(list->scroll < list->numitems);
	SDL_assert(list->viewsize > 0);
}

void M_List_AutoScroll(menulist_t* list)
{
	if (list->numitems <= list->viewsize)
		return;
	if (list->cursor < list->scroll + 1)
		list->scroll = list->cursor;
	else if (list->cursor >= list->scroll + list->viewsize)
		list->scroll = list->cursor - list->viewsize + 1;
}

void M_List_CenterCursor(menulist_t* list)
{
	if (list->cursor >= list->viewsize)
	{
		if (list->cursor + list->viewsize >= list->numitems)
			list->scroll = list->numitems - list->viewsize; // last page, scroll to the end
		else
			list->scroll = list->cursor - list->viewsize / 2; // keep centered
		list->scroll = CLAMP(0, list->scroll, list->numitems - list->viewsize);
	}
	else
		list->scroll = 0;
}

int M_List_GetOverflow(const menulist_t* list)
{
	return list->numitems - list->viewsize;
}

void M_List_GetScrollbar(const menulist_t* list, int* y, int* size)
{
	if (list->numitems <= list->viewsize)
	{
		*y = 0;
		*size = 0;
		return;
	}

	*size = (int)(list->viewsize * list->viewsize / (float)list->numitems + 0.5f);
	*size = q_max(*size, 2);
	*y = (int)(list->scroll / (float)(list->numitems - list->viewsize) * (list->viewsize - *size) + 0.5f);
}

void M_List_GetVisibleRange(const menulist_t* list, int* first, int* count)
{
	*first = list->scroll;
	*count = q_min(list->scroll + list->viewsize, list->numitems) - list->scroll;
}

qboolean M_List_IsItemVisible(const menulist_t* list, int i)
{
	int first, count;
	M_List_GetVisibleRange(list, &first, &count);
	return (unsigned)(i - first) < (unsigned)count;
}

qboolean M_List_Key(menulist_t* list, int key)
{
	switch (key)
	{
	case K_HOME:
	case K_KP_HOME:
		S_LocalSound("misc/menu1.wav");
		list->cursor = 0;
		M_List_AutoScroll(list);
		return true;

	case K_END:
	case K_KP_END:
		S_LocalSound("misc/menu1.wav");
		list->cursor = list->numitems - 1;
		M_List_AutoScroll(list);
		return true;

	case K_PGDN:
	case K_KP_PGDN:
		S_LocalSound("misc/menu1.wav");
		if (list->cursor - list->scroll < list->viewsize - 1)
			list->cursor = list->scroll + list->viewsize - 1;
		else
			list->cursor += list->viewsize - 1;
		list->cursor = q_min(list->cursor, list->numitems - 1);
		M_List_AutoScroll(list);
		return true;

	case K_PGUP:
	case K_KP_PGUP:
		S_LocalSound("misc/menu1.wav");
		if (list->cursor > list->scroll)
			list->cursor = list->scroll;
		else
			list->cursor -= list->viewsize - 1;
		list->cursor = q_max(list->cursor, 0);
		M_List_AutoScroll(list);
		return true;

	case K_UPARROW:
	case K_KP_UPARROW:
		S_LocalSound("misc/menu1.wav");
		if (--list->cursor < 0)
			list->cursor = list->numitems - 1;
		M_List_AutoScroll(list);
		return true;

	case K_DOWNARROW:
	case K_KP_DOWNARROW:
		S_LocalSound("misc/menu1.wav");
		if (++list->cursor >= list->numitems)
			list->cursor = 0;
		M_List_AutoScroll(list);
		return true;

	default:
		return false;
	}
}

qboolean M_List_CycleMatch(menulist_t* list, int key, qboolean(*match_fn) (int idx, char c))
{
	int i, j, dir;

	if (!(key >= 'a' && key <= 'z') &&
		!(key >= 'A' && key <= 'Z') &&
		!(key >= '0' && key <= '9'))
		return false;

	if (list->numitems <= 0)
		return false;

	S_LocalSound("misc/menu1.wav");

	key = q_tolower(key);
	dir = keydown[K_SHIFT] ? -1 : 1;

	for (i = 1, j = list->cursor + dir; i < list->numitems; i++, j += dir)
	{
		j = (j + list->numitems) % list->numitems; // avoid negative mod
		if (match_fn(j, (char)key))
		{
			list->cursor = j;
			M_List_AutoScroll(list);
			break;
		}
	}

	return true;
}

//=============================================================================

// woods #modsmenu (iw)

/* Mods menu */

#define MAX_MODS		4096
#define MAX_VIS_MODS	18

typedef struct
{
	const char* name;
	qboolean	active;
} moditem_t;

static struct
{
	menulist_t			list;
	enum m_state_e		prev;
	moditem_t			items[MAX_MODS];
} modsmenu;

static qboolean M_Mods_IsActive(const char* game)
{
	extern char com_gamenames[];
	const char* list, * end, * p;

	if (!q_strcasecmp(game, GAMENAME))
		return !*com_gamenames;

	list = com_gamenames;
	while (*list)
	{
		end = list;
		while (*end && *end != ';')
			end++;

		p = game;
		while (*p && list != end)
			if (q_tolower(*p) == q_tolower(*list))
				p++, list++;
			else
				break;

		if (!*p && list == end)
			return true;

		list = end;
		if (*list)
			list++;
	}

	return false;
}

static void M_Mods_Add(const char* name)
{
	moditem_t* mod = &modsmenu.items[modsmenu.list.numitems];
	mod->name = name;
	mod->active = M_Mods_IsActive(name);
	if (mod->active && modsmenu.list.cursor == -1)
		modsmenu.list.cursor = modsmenu.list.numitems;
	modsmenu.list.numitems++;
}

static void M_Mods_Init(void)
{
	filelist_item_t* item;

	modsmenu.list.viewsize = MAX_VIS_MODS;
	modsmenu.list.cursor = -1;
	modsmenu.list.scroll = 0;
	modsmenu.list.numitems = 0;

	for (item = modlist; item && modsmenu.list.numitems < MAX_MODS; item = item->next)
		M_Mods_Add(item->name);

	if (modsmenu.list.cursor == -1)
		modsmenu.list.cursor = 0;

	M_List_CenterCursor(&modsmenu.list);
}

void M_Menu_Mods_f(void)
{
	key_dest = key_menu;
	modsmenu.prev = m_state;
	m_state = m_mods;
	m_entersound = true;
	M_Mods_Init();
}

void M_Mods_Draw(void)
{
	const char* str;
	int x, y, i, j, cols;
	int firstvis, numvis;

	x = 64;
	y = 32;
	cols = 28;

	M_DrawTransPic(16, 4, Draw_CachePic("gfx/qplaque.lmp"));
	Draw_String(x, y - 28, "Mods");
	M_DrawQuakeBar(x - 8, y - 16, cols + 2);

	M_List_GetVisibleRange(&modsmenu.list, &firstvis, &numvis);
	for (i = 0; i < numvis; i++)
	{
		int idx = i + firstvis;
		int mask = modsmenu.items[idx].active ? 0 : 128;
		for (j = 0; j < cols - 1 && modsmenu.items[idx].name[j]; j++)
			M_DrawCharacter(x + j * 8, y + i * 8, modsmenu.items[idx].name[j] | mask);

		if (idx == modsmenu.list.cursor)
			M_DrawCharacter(x - 8, y + i * 8, 12 + ((int)(realtime * 4) & 1));
	}

	if (M_List_GetOverflow(&modsmenu.list) > 0)
	{
		int scrollbary, scrollbarh;
		M_List_GetScrollbar(&modsmenu.list, &scrollbary, &scrollbarh);
		M_DrawTextBox(x + cols * 8 - 12, y + scrollbary * 8 - 4, 0, scrollbarh - 1);

		str = va("%d-%d of %d", firstvis + 1, firstvis + numvis, modsmenu.list.numitems);
		M_Print(x + (cols - strlen(str)) * 8, y - 24, str);

		if (modsmenu.list.scroll > 0)
			M_DrawEllipsisBar(x, y - 8, cols);
		if (modsmenu.list.scroll + modsmenu.list.viewsize < modsmenu.list.numitems)
			M_DrawEllipsisBar(x, y + modsmenu.list.viewsize * 8, cols);
	}
}

qboolean M_Mods_Match(int index, char initial)
{
	return q_tolower(modsmenu.items[index].name[0]) == initial;
}

void M_Mods_Key(int key)
{
	if (M_List_Key(&modsmenu.list, key))
		return;

	if (M_List_CycleMatch(&modsmenu.list, key, M_Mods_Match))
		return;

	switch (key)
	{
	case K_ESCAPE:
	case K_BBUTTON:
		if (modsmenu.prev == m_options)
			M_Menu_Options_f();
		else
			M_Menu_Main_f();
		break;

	case K_ENTER:
	case K_KP_ENTER:
	case K_ABUTTON:
		Cbuf_AddText(va("game %s\n", modsmenu.items[modsmenu.list.cursor].name));
		M_Menu_Main_f();
		break;

	default:
		break;
	}
}

//=============================================================================
/* Credits menu -- used by the 2021 re-release */

void M_Menu_Credits_f (void)
{
}

static struct
{
	const char *name;
	xcommand_t function;
	cmd_function_t *cmd;
} menucommands[] =
{
	{"menu_main", M_Menu_Main_f},
	{"menu_singleplayer", M_Menu_SinglePlayer_f},
	{"menu_load", M_Menu_Load_f},
	{"menu_save", M_Menu_Save_f},
	{"menu_multiplayer", M_Menu_MultiPlayer_f},
	{"menu_setup", M_Menu_Setup_f},
	{"menu_options", M_Menu_Options_f},
	{"menu_keys", M_Menu_Keys_f},
	{"menu_extras", M_Menu_Extras_f},
	{"menu_video", M_Menu_Video_f},
	{"help", M_Menu_Help_f},
	{"menu_quit", M_Menu_Quit_f},
	{"menu_credits", M_Menu_Credits_f}, // needed by the 2021 re-release
	{"namemaker", M_Shortcut_NameMaker_f}, // woods
	{"menu_mods", M_Menu_Mods_f}, // woods


};

//=============================================================================
/* MenuQC Subsystem */
#define MENUQC_PROGHEADER_CRC 10020
void MQC_End(void)
{
	PR_SwitchQCVM(NULL);
}
void MQC_Begin(void)
{
	PR_SwitchQCVM(&cls.menu_qcvm);
	pr_global_struct = NULL;
}
static qboolean MQC_Init(void)
{
	size_t i;
	qboolean success;
	PR_SwitchQCVM(&cls.menu_qcvm);
	if (COM_CheckParm("-qmenu") || fitzmode || !pr_checkextension.value)
		success = false;
	else
		success = PR_LoadProgs("menu.dat", false, MENUQC_PROGHEADER_CRC, pr_menubuiltins, pr_menunumbuiltins);
	if (success && qcvm->extfuncs.m_draw)
	{
		for (i = 0; i < sizeof(menucommands)/sizeof(menucommands[0]); i++)
			if (menucommands[i].cmd)
			{
				Cmd_RemoveCommand (menucommands[i].cmd);
				menucommands[i].cmd = NULL;
			}


		qcvm->max_edicts = CLAMP (MIN_EDICTS,(int)max_edicts.value,MAX_EDICTS);
		qcvm->edicts = (edict_t *) malloc (qcvm->max_edicts*qcvm->edict_size);
		qcvm->num_edicts = qcvm->reserved_edicts = 1;
		memset(qcvm->edicts, 0, qcvm->num_edicts*qcvm->edict_size);

		if (qcvm->extfuncs.m_init)
			PR_ExecuteProgram(qcvm->extfuncs.m_init);
	}
	PR_SwitchQCVM(NULL);
	return success;
}

void MQC_Shutdown(void)
{
	size_t i;
	if (key_dest == key_menu)
		key_dest = key_console;
	PR_ClearProgs(&cls.menu_qcvm);					//nuke it from orbit

	for (i = 0; i < sizeof(menucommands)/sizeof(menucommands[0]); i++)
		if (!menucommands[i].cmd)
			menucommands[i].cmd = Cmd_AddCommand (menucommands[i].name, menucommands[i].function);
}

static void MQC_Command_f(void)
{
	if (cls.menu_qcvm.extfuncs.GameCommand)
	{
		MQC_Begin();
		G_INT(OFS_PARM0) = PR_MakeTempString(Cmd_Args());
		PR_ExecuteProgram(qcvm->extfuncs.GameCommand);
		MQC_End();
	}
	else
		Con_Printf("menu_cmd: no menuqc GameCommand function available\n");
}

//=============================================================================
/* Menu Subsystem */

/*
================
M_ToggleMenu_f
================
*/
void M_ToggleMenu (int mode)
{
	if (cls.menu_qcvm.extfuncs.m_toggle)
	{
		MQC_Begin();
		G_FLOAT(OFS_PARM0) = mode;
		PR_ExecuteProgram(qcvm->extfuncs.m_toggle);
		MQC_End();
		return;
	}

	m_entersound = true;

	if (key_dest == key_menu)
	{
		if (mode != 0 && m_state != m_main)
		{
			M_Menu_Main_f ();
			return;
		}

		key_dest = key_game;
		m_state = m_none;

		IN_UpdateGrabs();
		return;
	}
	else if (mode == 0)
		return;
	if (mode == -1 && key_dest == key_console)
	{
		Con_ToggleConsole_f ();
	}
	else
	{
		M_Menu_Main_f ();
	}
}
static void M_ToggleMenu_f (void)
{
	M_ToggleMenu((Cmd_Argc() < 2) ? -1 : atoi(Cmd_Argv(1)));
}

static void M_MenuRestart_f (void)
{
	qboolean off = !strcmp(Cmd_Argv(1), "off");
	if (off || !MQC_Init())
		MQC_Shutdown();
}

void M_Init (void)
{
	Cmd_AddCommand ("togglemenu", M_ToggleMenu_f);
	Cmd_AddCommand ("menu_cmd", MQC_Command_f);
	Cmd_AddCommand ("menu_restart", M_MenuRestart_f);	//qss still loads progs on hunk, so we can't do this safely.

	if (!MQC_Init())
		MQC_Shutdown();
}


void M_Draw (void)
{
	if (cls.menu_qcvm.extfuncs.m_draw)
	{	//Spike -- menuqc
		float s = q_min((float)glwidth / 320.0, (float)glheight / 200.0);
		s = CLAMP (1.0, scr_menuscale.value, s);
		if (!host_initialized)
			return;
		MQC_Begin();

		if (scr_con_current && key_dest == key_menu)
		{	//make sure we don't have the console getting drawn in the background making the menu unreadable.
			//FIXME: rework console to show over the top of menuqc.
			Draw_ConsoleBackground ();
			S_ExtraUpdate ();
		}

		GL_SetCanvas (CANVAS_MENUQC);
		glEnable (GL_BLEND);	//in the finest tradition of glquake, we litter gl state calls all over the place. yay state trackers.
		glDisable (GL_ALPHA_TEST);	//in the finest tradition of glquake, we litter gl state calls all over the place. yay state trackers.
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

		if (qcvm->extglobals.time)
			*qcvm->extglobals.time = realtime;
		if (qcvm->extglobals.frametime)
			*qcvm->extglobals.frametime = host_frametime;
		G_FLOAT(OFS_PARM0+0) = vid.width/s;
		G_FLOAT(OFS_PARM0+1) = vid.height/s;
		G_FLOAT(OFS_PARM0+2) = 0;
		PR_ExecuteProgram(qcvm->extfuncs.m_draw);

		glDisable (GL_BLEND);
		glEnable (GL_ALPHA_TEST);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);	//back to ignoring vertex colours.
		glDisable(GL_SCISSOR_TEST);
		glColor3f (1,1,1);

		MQC_End();
		return;
	}

	if (m_state == m_none || key_dest != key_menu)
		return;

	if (!m_recursiveDraw)
	{
		if (scr_con_current)
		{
			Draw_ConsoleBackground ();
			S_ExtraUpdate ();
		}

		Draw_FadeScreen (); //johnfitz -- fade even if console fills screen
	}
	else
	{
		m_recursiveDraw = false;
	}

	GL_SetCanvas (CANVAS_MENU); //johnfitz

	switch (m_state)
	{
	case m_none:
		break;

	case m_main:
		M_Main_Draw ();
		break;

	case m_singleplayer:
		M_SinglePlayer_Draw ();
		break;

	case m_load:
		M_Load_Draw ();
		break;

	case m_save:
		M_Save_Draw ();
		break;

	case m_multiplayer:
		M_MultiPlayer_Draw ();
		break;

	case m_setup:
		M_Setup_Draw ();
		break;

	case m_namemaker: // woods #namemaker
		M_NameMaker_Draw();
		break;

	case m_net:
		M_Net_Draw ();
		break;

	case m_options:
		M_Options_Draw ();
		break;

	case m_keys:
		M_Keys_Draw ();
		break;

	case m_extras:
		M_Extras_Draw ();
		break;

	case m_video:
		M_Video_Draw ();
		break;

	case m_mods: // woods #modsmenu (iw)
		M_Mods_Draw();
		break;

	case m_help:
		M_Help_Draw ();
		break;

	case m_quit:
		if (/*!fitzmode || */(cl.matchinp != 1 || cl.notobserver != 1 || cl.teamcolor[0] == 0) || cls.demoplayback) // woods #matchquit
		{ /* QuakeSpasm customization: */
			/* Quit now! S.A. */
			key_dest = key_console;
			Host_Quit_f ();
		}
		M_Quit_Draw ();
		break;

	case m_lanconfig:
		M_LanConfig_Draw ();
		break;

	case m_gameoptions:
		M_GameOptions_Draw ();
		break;

	case m_search:
		M_Search_Draw ();
		break;

	case m_slist:
		M_ServerList_Draw ();
		break;
	}

	if (m_entersound)
	{
		S_LocalSound ("misc/menu2.wav");
		m_entersound = false;
	}

	S_ExtraUpdate ();
}


void M_Keydown (int key)
{
	if (cls.menu_qcvm.extfuncs.m_draw)	//don't get confused.
		return;

	switch (m_state)
	{
	case m_none:
		return;

	case m_main:
		M_Main_Key (key);
		return;

	case m_singleplayer:
		M_SinglePlayer_Key (key);
		return;

	case m_load:
		M_Load_Key (key);
		return;

	case m_save:
		M_Save_Key (key);
		return;

	case m_multiplayer:
		M_MultiPlayer_Key (key);
		return;

	case m_setup:
		M_Setup_Key (key);
		return;

	case m_namemaker: // woods #namemaker
		M_NameMaker_Key(key);
		return;

	case m_net:
		M_Net_Key (key);
		return;

	case m_options:
		M_Options_Key (key);
		return;

	case m_keys:
		M_Keys_Key (key);
		return;

	case m_extras:
		M_Extras_Key (key);
		return;

	case m_video:
		M_Video_Key (key);
		return;

	case m_mods: // woods #modsmenu (iw)
		M_Mods_Key(key);
		return;

	case m_help:
		M_Help_Key (key);
		return;

	case m_quit:
		M_Quit_Key (key);
		return;

	case m_lanconfig:
		M_LanConfig_Key (key);
		return;

	case m_gameoptions:
		M_GameOptions_Key (key);
		return;

	case m_search:
		M_Search_Key (key);
		break;

	case m_slist:
		M_ServerList_Key (key);
		return;
	}
}


void M_Charinput (int key)
{
	if (cls.menu_qcvm.extfuncs.m_draw)	//don't get confused.
		return;

	switch (m_state)
	{
	case m_setup:
		M_Setup_Char (key);
		return;
	case m_quit:
		M_Quit_Char (key);
		return;
	case m_lanconfig:
		M_LanConfig_Char (key);
		return;
	default:
		return;
	}
}


qboolean M_TextEntry (void)
{
	switch (m_state)
	{
	case m_setup:
		return M_Setup_TextEntry ();
	case m_quit:
		return M_Quit_TextEntry ();
	case m_lanconfig:
		return M_LanConfig_TextEntry ();
	default:
		return false;
	}
}

#if defined(_WIN32) // woods #disablecaps via ironwail
qboolean M_KeyBinding(void)
{
	return key_dest == key_menu && m_state == m_keys && bind_grab;
}
#endif

void M_ConfigureNetSubsystem(void)
{
// enable/disable net systems to match desired config
	Cbuf_AddText ("stopdemo\n");

	if (/*IPXConfig || */TCPIPConfig) // woods #skipipx
		net_hostport = lanConfig_port;
}

