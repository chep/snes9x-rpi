/*
 * Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
 *
 * (c) Copyright 1996 - 2001 Gary Henderson (gary.henderson@ntlworld.com) and
 *                           Jerremy Koot (jkoot@snes9x.com)
 *
 * Super FX C emulator code 
 * (c) Copyright 1997 - 1999 Ivar (ivar@snes9x.com) and
 *                           Gary Henderson.
 * Super FX assembler emulator code (c) Copyright 1998 zsKnight and _Demo_.
 *
 * DSP1 emulator code (c) Copyright 1998 Ivar, _Demo_ and Gary Henderson.
 * C4 asm and some C emulation code (c) Copyright 2000 zsKnight and _Demo_.
 * C4 C code (c) Copyright 2001 Gary Henderson (gary.henderson@ntlworld.com).
 *
 * DOS port code contains the works of other authors. See headers in
 * individual files.
 *
 * Snes9x homepage: http://www.snes9x.com
 *
 * Permission to use, copy, modify and distribute Snes9x in both binary and
 * source form, for non-commercial purposes, is hereby granted without fee,
 * providing that this license information and copyright notice appear with
 * all copies and any derived work.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event shall the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Snes9x is freeware for PERSONAL USE only. Commercial users should
 * seek permission of the copyright holders first. Commercial use includes
 * charging money for Snes9x or software derived from Snes9x.
 *
 * The copyright holders request that bug fixes and improvements to the code
 * should be forwarded to them so everyone can benefit from the modifications
 * in future versions.
 *
 * Super NES and Super Nintendo Entertainment System are trademarks of
 * Nintendo Co., Limited and its subsidiary companies.
 */
#include <signal.h>
#include <errno.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <ctype.h>
#include <dirent.h>
#include <SDL/SDL.h>
#include "keydef.h"

#include "joydef.h"
#include "keys.h" // These two were added in the rPi port. See docs.

#undef USE_THREADS
#define USE_THREADS
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#ifdef USE_THREADS
#include <pthread.h>
#include <sched.h>

pthread_t thread;
pthread_mutex_t mutex;
#endif

#include <sys/soundcard.h>
#include <sys/mman.h>

#include "snes9x.h"
#include "memmap.h"
#include "debug.h"
#include "cpuexec.h"
#include "ppu.h"
#include "snapshot.h"
#include "apu.h"
#include "display.h"
#include "gfx.h"
#include "soundux.h"
#include "spc700.h"

uint8 *keyssnes;
int OldSkipFrame;

// rPi port: added all this joystick stuff
// If for some unfathomable reason your joystick has more than 32 buttons or 8 
// axes, you should change these array definitions to reflect that. 
uint8 joy_buttons[32];
uint8 joy_axes[8];

void InitTimer ();
void *S9xProcessSound (void *);

extern void S9xDisplayFrameRate (uint8 *, uint32);
extern void S9xDisplayString (const char *string, uint8 *, uint32);
extern SDL_Surface *screen;

static uint32 ffc = 0;
bool8_32 nso = FALSE, vga = FALSE;
uint32 xs = 320, ys = 240, cl = 0, cs = 0, mfs = 10;
 

char *rom_filename = NULL;
char *snapshot_filename = NULL;

#ifndef _ZAURUS
#if defined(__linux) || defined(__sun)
static void sigbrkhandler(int)
{
#ifdef DEBUGGER
    CPU.Flags |= DEBUG_MODE_FLAG;
    signal(SIGINT, (SIG_PF) sigbrkhandler);
#endif
}
#endif
#endif

void OutOfMemory ()
{
    fprintf (stderr, "\
Snes9X: Memory allocation failure - not enough RAM/virtual memory available.\n\
        S9xExiting...\n");
    Memory.Deinit ();
    S9xDeinitAPU ();
    
    exit (1);
}

void S9xParseArg (char **argv, int &i, int argc)
{

    if (strcasecmp (argv [i], "-b") == 0 ||
	     strcasecmp (argv [i], "-bs") == 0 ||
	     strcasecmp (argv [i], "-buffersize") == 0)
    {
	if (i + 1 < argc)
	    Settings.SoundBufferSize = atoi (argv [++i]);
	else
	    S9xUsage ();
    }
    else if (strcmp (argv [i], "-l") == 0 ||
	     strcasecmp (argv [i], "-loadsnapshot") == 0)
    {
	if (i + 1 < argc)
	    snapshot_filename = argv [++i];
	else
	    S9xUsage ();
    }
    else if (strcmp (argv [i], "-nso") == 0)
		nso = TRUE;
    else if (strcmp (argv [i], "-x2") == 0)
		vga = TRUE;
    else if (strcmp (argv [i], "-xs") == 0)
    {
	if (i + 1 < argc)
	    xs = atoi(argv [++i]);
	else
	    S9xUsage ();
	}
    else if (strcmp (argv [i], "-ys") == 0)
    {
	if (i + 1 < argc)
	    ys = atoi(argv [++i]);
	else
	    S9xUsage ();
	}
    else if (strcmp (argv [i], "-cl") == 0)
    {
	if (i + 1 < argc)
	    cl = atoi(argv [++i]);
	else
	    S9xUsage ();
	}
    else if (strcmp (argv [i], "-cs") == 0)
    {
	if (i + 1 < argc)
	    cs = atoi(argv [++i]);
	else
	    S9xUsage ();
	}
    else if (strcmp (argv [i], "-mfs") == 0)
    {
	if (i + 1 < argc)
	    mfs = atoi(argv [++i]);
	else
	    S9xUsage ();
	}
    else
	    S9xUsage ();
//	S9xParseDisplayArg (argv, i, argc);
}

/*#include "cheats.h"*/
extern "C"
int main (int argc, char **argv)
{
    if (argc < 2)
	S9xUsage ();
    ZeroMemory (&Settings, sizeof (Settings));

    Settings.JoystickEnabled = TRUE; // rPi changed default
    Settings.SoundPlaybackRate = 7;
    Settings.Stereo = TRUE;
    Settings.SoundBufferSize = 256;
    Settings.CyclesPercentage = 100;
    Settings.DisableSoundEcho = FALSE;
    Settings.APUEnabled = Settings.NextAPUEnabled = TRUE;
    Settings.H_Max = SNES_CYCLES_PER_SCANLINE;
    Settings.SkipFrames = AUTO_FRAMERATE;
    Settings.ShutdownMaster = TRUE;
    Settings.FrameTimePAL = 20000;
    Settings.FrameTimeNTSC = 16667;
    Settings.FrameTime = Settings.FrameTimeNTSC;
    Settings.DisableSampleCaching = FALSE;
    Settings.DisableMasterVolume = FALSE;
    Settings.Mouse = FALSE;
    Settings.SuperScope = FALSE;
    Settings.MultiPlayer5 = FALSE;
//    Settings.ControllerOption = SNES_MULTIPLAYER5;
    Settings.ControllerOption = 0;
    Settings.Transparency = TRUE;
    Settings.SixteenBit = TRUE;
    Settings.SupportHiRes = FALSE;
    Settings.NetPlay = FALSE;
    Settings.ServerName [0] = 0;
    Settings.ThreadSound = TRUE;
    Settings.AutoSaveDelay = 30;
    Settings.ApplyCheats = TRUE;
    Settings.TurboMode = FALSE;
    Settings.TurboSkipFrames = 15;
    rom_filename = S9xParseArgs (argv, argc);

//    Settings.Transparency = Settings.ForceTransparency;
    if (Settings.ForceNoTransparency)
	Settings.Transparency = FALSE;

    if (Settings.Transparency)
	Settings.SixteenBit = TRUE;

    Settings.HBlankStart = (256 * Settings.H_Max) / SNES_HCOUNTER_MAX;

    if (!Memory.Init () || !S9xInitAPU())
	OutOfMemory ();

   (void) S9xInitSound (Settings.SoundPlaybackRate, Settings.Stereo,
			 Settings.SoundBufferSize);

    if (!Settings.APUEnabled)
	S9xSetSoundMute (TRUE);

    uint32 saved_flags = CPU.Flags;

#ifdef GFX_MULTI_FORMAT
    S9xSetRenderPixelFormat (RGB565);
#endif

    S9xInitInputDevices ();

    S9xInitDisplay (argc, argv);
    if (!S9xGraphicsInit ())
	OutOfMemory ();
#ifndef _ZAURUS
    S9xTextMode ();
#endif
    if (rom_filename)
    {
	if (!Memory.LoadROM (rom_filename))
	{
	    char dir [_MAX_DIR + 1];
	    char drive [_MAX_DRIVE + 1];
	    char name [_MAX_FNAME + 1];
	    char ext [_MAX_EXT + 1];
	    char fname [_MAX_PATH + 1];

	    _splitpath (rom_filename, drive, dir, name, ext);
	    _makepath (fname, drive, dir, name, ext);

	    strcpy (fname, S9xGetROMDirectory ());
	    strcat (fname, SLASH_STR);
	    strcat (fname, name);
	    if (ext [0])
	    {
		strcat (fname, ".");
		strcat (fname, ext);
	    }
	    _splitpath (fname, drive, dir, name, ext);
	    _makepath (fname, drive, dir, name, ext);
	    if (!Memory.LoadROM (fname))
	    {
		printf ("Error opening: %s\n", rom_filename);
		exit (1);
	    }
	}
	Memory.LoadSRAM (S9xGetFilename (".srm"));
//	S9xLoadCheatFile (S9xGetFilename (".cht"));
    }
    else
    {
	S9xReset ();
	Settings.Paused |= 2;
    }
    CPU.Flags = saved_flags;

    struct sigaction sa;
//    sa.sa_handler = sigbrkhandler;

#ifdef SA_RESTART
    sa.sa_flags = SA_RESTART;
#else
    sa.sa_flags = 0;
#endif

    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);

    if (snapshot_filename)
    {
	int Flags = CPU.Flags & (DEBUG_MODE_FLAG | TRACE_FLAG);
	if (!S9xLoadSnapshot (snapshot_filename))
	    exit (1);
	CPU.Flags |= Flags;
    }
#ifndef _ZAURUS
    S9xGraphicsMode ();
    sprintf (String, "\"%s\" %s: %s", Memory.ROMName, TITLE, VERSION);
    S9xSetTitle (String);
#endif

 	if (nso) {
		Settings.SoundBufferSize = 8192;
    	Settings.SoundPlaybackRate = 1;
    	Settings.DisableSoundEcho = TRUE;
    	Settings.DisableMasterVolume = TRUE;
		Settings.Stereo = FALSE;
    	S9xSetSoundMute (TRUE);
	} else {
	    if (!Settings.APUEnabled)
			S9xSetSoundMute (FALSE);
		else
	    	InitTimer ();
	}

    while (1)
    {
#ifndef _ZAURUS
	if (!Settings.Paused
#ifdef DEBUGGER
	    || (CPU.Flags & (DEBUG_MODE_FLAG | SINGLE_STEP_FLAG))
#endif
           )
#endif
	    S9xMainLoop ();
#ifndef _ZAURUS
	if (Settings.Paused
#ifdef DEBUGGER
	    || (CPU.Flags & DEBUG_MODE_FLAG)
#endif
           )
	{
	    S9xSetSoundMute (TRUE);
	}

#ifdef DEBUGGER
	if (CPU.Flags & DEBUG_MODE_FLAG)
	{
	    S9xDoDebug ();
	}
	else
#endif
	if (Settings.Paused)
	    S9xProcessEvents (TRUE);

//	S9xProcessEvents (FALSE);

	if (!Settings.Paused
#ifdef DEBUGGER
	    && !(CPU.Flags & DEBUG_MODE_FLAG)
#endif	    
           )
	{
	    S9xSetSoundMute (FALSE);
	}
#endif
    }
    return (0);
}

void S9xAutoSaveSRAM ()
{
    Memory.SaveSRAM (S9xGetFilename (".srm"));
}

void S9xExit ()
{
    S9xSetSoundMute (TRUE);
    S9xDeinitDisplay ();
    Memory.SaveSRAM (S9xGetFilename (".srm"));
//    S9xSaveCheatFile (S9xGetFilename (".cht"));
    Memory.Deinit ();
    S9xDeinitAPU ();

    exit (0);
}

Uint16 sfc_key[256];
void S9xInitInputDevices ()
{
	memset(sfc_key, 0, 256);
	sfc_key[QUIT] = SDLK_a;
	sfc_key[A_1] = RPI_KEY_A;
	sfc_key[B_1] = RPI_KEY_B;
	sfc_key[X_1] = RPI_KEY_X;
	sfc_key[Y_1] = RPI_KEY_Y;
	sfc_key[L_1] = RPI_KEY_L;
	sfc_key[R_1] = RPI_KEY_R;
	sfc_key[START_1] = RPI_KEY_START;
	sfc_key[SELECT_1] = RPI_KEY_SELECT;
	sfc_key[LEFT_1] = RPI_KEY_LEFT;
	sfc_key[RIGHT_1] = RPI_KEY_RIGHT;
	sfc_key[UP_1] = RPI_KEY_UP;
	sfc_key[DOWN_1] = RPI_KEY_DOWN;

	sfc_key[QUIT] = RPI_KEY_QUIT;
	sfc_key[ACCEL] = RPI_KEY_ACCEL;

/*	sfc_key[LEFT_2] = SDLK_4;
	sfc_key[RIGHT_2] = SDLK_6;
	sfc_key[UP_2] = SDLK_8;
	sfc_key[DOWN_2] = SDLK_2;
	sfc_key[LU_2] = SDLK_7;
	sfc_key[LD_2] = SDLK_1;
	sfc_key[RU_2] = SDLK_9;
	sfc_key[RD_2] = SDLK_3; */


	int i = 0;
	char *envp, *j;
	envp = j = getenv ("S9XKEYS");
	if (envp) {
		do {
			if (j = strchr(envp, ','))
				*j = 0;
			if (i == 0) sfc_key[QUIT] = atoi(envp);
			else if (i == 1)  sfc_key[A_1] = atoi(envp);
			else if (i == 2)  sfc_key[B_1] = atoi(envp);
			else if (i == 3)  sfc_key[X_1] = atoi(envp);
			else if (i == 4)  sfc_key[Y_1] = atoi(envp);
			else if (i == 5)  sfc_key[L_1] = atoi(envp);
			else if (i == 6)  sfc_key[R_1] = atoi(envp);
			else if (i == 7)  sfc_key[START_1] = atoi(envp);
			else if (i == 8)  sfc_key[SELECT_1] = atoi(envp);
/*			else if (i == 9)  sfc_key[LEFT_2] = atoi(envp);
			else if (i == 10) sfc_key[RIGHT_2] = atoi(envp);
			else if (i == 11) sfc_key[UP_2] = atoi(envp);
			else if (i == 12) sfc_key[DOWN_2] = atoi(envp);
			else if (i == 13) sfc_key[LU_2] = atoi(envp);
			else if (i == 14) sfc_key[LD_2] = atoi(envp);
			else if (i == 15) sfc_key[RU_2] = atoi(envp);
			else if (i == 16) sfc_key[RD_2] = atoi(envp); */
			envp = j + 1;
			++i;
		} while(j);
	}}

const char *GetHomeDirectory ()
{
    return (getenv ("HOME"));
}

const char *S9xGetSnapshotDirectory ()
{
    static char filename [PATH_MAX];
    const char *snapshot;
    
    if (!(snapshot = getenv ("SNES9X_SNAPSHOT_DIR")) &&
	!(snapshot = getenv ("SNES96_SNAPSHOT_DIR")))
    {
	const char *home = GetHomeDirectory ();
	strcpy (filename, home);
	strcat (filename, SLASH_STR);
	strcat (filename, ".snes96_snapshots");
	mkdir (filename, 0777);
	chown (filename, getuid (), getgid ());
    }
    else
	return (snapshot);

    return (filename);
}

const char *S9xGetFilename (const char *ex)
{
    static char filename [PATH_MAX + 1];
    char drive [_MAX_DRIVE + 1];
    char dir [_MAX_DIR + 1];
    char fname [_MAX_FNAME + 1];
    char ext [_MAX_EXT + 1];

    _splitpath (Memory.ROMFilename, drive, dir, fname, ext);
    strcpy (filename, S9xGetSnapshotDirectory ());
    strcat (filename, SLASH_STR);
    strcat (filename, fname);
    strcat (filename, ex);

    return (filename);
}

const char *S9xGetROMDirectory ()
{
    const char *roms;
    
    if (!(roms = getenv ("SNES9X_ROM_DIR")) &&
	!(roms = getenv ("SNES96_ROM_DIR")))
	return ("." SLASH_STR "roms");
    else
	return (roms);
}

const char *S9xBasename (const char *f)
{
    const char *p;
    if ((p = strrchr (f, '/')) != NULL || (p = strrchr (f, '\\')) != NULL)
	return (p + 1);

    return (f);
}

#ifndef _ZAURUS
const char *S9xChooseFilename (bool8 read_only)
{
    char def [PATH_MAX + 1];
    char title [PATH_MAX + 1];
    char drive [_MAX_DRIVE + 1];
    char dir [_MAX_DIR + 1];
    char ext [_MAX_EXT + 1];

    _splitpath (Memory.ROMFilename, drive, dir, def, ext);
    strcat (def, ".s96");
    sprintf (title, "%s snapshot filename",
	    read_only ? "Select load" : "Choose save");
    const char *filename;

    S9xSetSoundMute (TRUE);
    filename = S9xSelectFilename (def, S9xGetSnapshotDirectory (), "s96", title);
    S9xSetSoundMute (FALSE);
    return (filename);
}
#endif

bool8 S9xOpenSnapshotFile (const char *fname, bool8 read_only, STREAM *file)
{
    char filename [PATH_MAX + 1];
    char drive [_MAX_DRIVE + 1];
    char dir [_MAX_DIR + 1];
    char ext [_MAX_EXT + 1];

    _splitpath (fname, drive, dir, filename, ext);

    if (*drive || *dir == '/' ||
	(*dir == '.' && (*(dir + 1) == '/'
        )))
    {
	strcpy (filename, fname);
	if (!*ext)
	    strcat (filename, ".s96");
    }
    else
    {
	strcpy (filename, S9xGetSnapshotDirectory ());
	strcat (filename, SLASH_STR);
	strcat (filename, fname);
	if (!*ext)
	    strcat (filename, ".s96");
    }
    
#ifdef ZLIB
    if (read_only)
    {
	if ((*file = OPEN_STREAM (filename, "rb")))
	    return (TRUE);
    }
    else
    {
	if ((*file = OPEN_STREAM (filename, "wb")))
	{
	    chown (filename, getuid (), getgid ());
	    return (TRUE);
	}
    }
#else
    char command [PATH_MAX];
    
    if (read_only)
    {
	sprintf (command, "gzip -d <\"%s\"", filename);
	if (*file = popen (command, "r"))
	    return (TRUE);
    }
    else
    {
	sprintf (command, "gzip --best >\"%s\"", filename);
	if (*file = popen (command, "wb"))
	    return (TRUE);
    }
#endif
    return (FALSE);
}

void S9xCloseSnapshotFile (STREAM file)
{
#ifdef ZLIB
    CLOSE_STREAM (file);
#else
    pclose (file);
#endif
}

#ifndef _ZAURUS
bool8_32 S9xInitUpdate ()
{
    return (TRUE);
}
#endif

bool8_32 S9xDeinitUpdate (int Width, int Height)
{
	register uint32 lp = (xs > 256) ? 16 : 0;

	if (Width > 256 || vga)
		lp *= 2;

// 	SDL_LockSurface(screen);
	if (vga) {
		if (Width > 256) {
			for (register uint32 i = 0; i < Height; i++) {
				register uint32 *dp32 = (uint32 *)(screen->pixels) + ((i + cl) * xs) + lp;
				register uint32 *sp32 = (uint32 *)(GFX.Screen) + (i << 8) + cs;
				for (register uint32 j = 0; j < 256; j++)
					*dp32++ = *sp32++;
			}
		} else {
			for (register uint32 i = 0; i < Height; i++) {
				register uint16 *dp16 = (uint16 *)(screen->pixels) + ((i + cl) * xs) * 2 + 64;
				register uint16 *sp16 = (uint16 *)(GFX.Screen) + (i << 9) + cs;
				for (register uint32 j = 0; j < 256; j++, dp16+=2, sp16++) {
					*dp16 = *(dp16+1) = *sp16;
				}
			}
		}
		if (Settings.DisplayFrameRate)
		    S9xDisplayFrameRate ((uint8 *)screen->pixels + 128, 2560);
		if (GFX.InfoString)
		    S9xDisplayString (GFX.InfoString, (uint8 *)screen->pixels + 128, 2560);
//  		SDL_UnlockSurface(screen);
		if (ffc < 5) {
			SDL_UpdateRect(screen,0,0,0,0);
			++ffc;
		} else
			SDL_UpdateRect(screen,64,0,512,480);
	} else {
		if (Settings.SupportHiRes) {
			if (Width > 256) {
				for (register uint32 i = 0; i < Height; i++) {
					register uint16 *dp16 = (uint16 *)(screen->pixels) + ((i + cl) * xs) + lp;
					register uint32 *sp32 = (uint32 *)(GFX.Screen) + (i << 8) + cs;
					for (register uint32 j = 0; j < 256; j++) {
						*dp16++ = *sp32++;
					}
				}
			} else {
				for (register uint32 i = 0; i < Height; i++) {
					register uint32 *dp32 = (uint32 *)(screen->pixels) + ((i + cl) * xs / 2) + lp;
					register uint32 *sp32 = (uint32 *)(GFX.Screen) + (i << 8) + cs;
					for (register uint32 j = 0; j < 128; j++) {
						*dp32++ = *sp32++;
					}
				}
			}
		}
		if (Settings.DisplayFrameRate)
		    S9xDisplayFrameRate ((uint8 *)screen->pixels + 64, 640);
		if (GFX.InfoString)
		    S9xDisplayString (GFX.InfoString, (uint8 *)screen->pixels + 64, 640);
//  		SDL_UnlockSurface(screen);
		if (ffc < 5) {
			SDL_UpdateRect(screen,0,0,0,0);
			++ffc;
		} else
			SDL_UpdateRect(screen,32,0,256,Height);
		//	SDL_Flip(screen);
	}
	return(TRUE);
}

#ifndef _ZAURUS
static unsigned long now ()
{
    static unsigned long seconds_base = 0;
    struct timeval tp;
    gettimeofday (&tp, NULL);
    if (!seconds_base)
	seconds_base = tp.tv_sec;

    return ((tp.tv_sec - seconds_base) * 1000 + tp.tv_usec / 1000);
}
#endif

void _makepath (char *path, const char *, const char *dir,
		const char *fname, const char *ext)
{
    if (dir && *dir)
    {
	strcpy (path, dir);
	strcat (path, "/");
    }
    else
	*path = 0;
    strcat (path, fname);
    if (ext && *ext)
    {
        strcat (path, ".");
        strcat (path, ext);
    }
}

void _splitpath (const char *path, char *drive, char *dir, char *fname,
		 char *ext)
{
    *drive = 0;

    char *slash = strrchr (path, '/');
    if (!slash)
	slash = strrchr (path, '\\');

    char *dot = strrchr (path, '.');

    if (dot && slash && dot < slash)
	dot = NULL;

    if (!slash)
    {
	strcpy (dir, "");
	strcpy (fname, path);
        if (dot)
        {
	    *(fname + (dot - path)) = 0;
	    strcpy (ext, dot + 1);
        }
	else
	    strcpy (ext, "");
    }
    else
    {
	strcpy (dir, path);
	*(dir + (slash - path)) = 0;
	strcpy (fname, slash + 1);
        if (dot)
	{
	    *(fname + (dot - slash) - 1) = 0;
    	    strcpy (ext, dot + 1);
	}
	else
	    strcpy (ext, "");
    }
}

#ifndef _ZAURUS
void S9xToggleSoundChannel (int c)
{
    if (c == 8)
	so.sound_switch = 255;
    else
	so.sound_switch ^= 1 << c;
    S9xSetSoundControl (so.sound_switch);
}
#endif

static void SoundTrigger ()
{
    if (Settings.APUEnabled && !so.mute_sound)
	S9xProcessSound (NULL);
}

void StopTimer ()
{
    struct itimerval timeout;

    timeout.it_interval.tv_sec = 0;
    timeout.it_interval.tv_usec = 0;
    timeout.it_value.tv_sec = 0;
    timeout.it_value.tv_usec = 0;
    if (setitimer (ITIMER_REAL, &timeout, NULL) < 0)
	perror ("setitimer");
}

void InitTimer ()
{
    struct itimerval timeout;
    struct sigaction sa;
    
#ifdef USE_THREADS
    if (Settings.ThreadSound)
    {
	pthread_mutex_init (&mutex, NULL);
	pthread_create (&thread, NULL, S9xProcessSound, NULL);
	return;
    }
#endif

    sa.sa_handler = (SIG_PF) SoundTrigger;

#if defined (SA_RESTART)
    sa.sa_flags = SA_RESTART;
#else
    sa.sa_flags = 0;
#endif

    sigemptyset (&sa.sa_mask);
    sigaction (SIGALRM, &sa, NULL);
    
    timeout.it_interval.tv_sec = 0;
    timeout.it_interval.tv_usec = 10000;
    timeout.it_value.tv_sec = 0;
    timeout.it_value.tv_usec = 10000;
    if (setitimer (ITIMER_REAL, &timeout, NULL) < 0)
	perror ("setitimer");
}

void S9xSyncSpeed ()
{
	S9xProcessEvents (FALSE);
    if (!Settings.TurboMode && Settings.SkipFrames == AUTO_FRAMERATE)
    {
	static struct timeval next1 = {0, 0};
	struct timeval now;

	while (gettimeofday (&now, NULL) < 0) ;
	if (next1.tv_sec == 0)
	{
	    next1 = now;
	    next1.tv_usec++;
	}

	if (timercmp(&next1, &now, >))
	{
	    if (IPPU.SkippedFrames == 0)
	    {
		do
		{
		    CHECK_SOUND ();
//		    S9xProcessEvents (FALSE);
		    while (gettimeofday (&now, NULL) < 0) ;
		} while (timercmp(&next1, &now, >));
	    }
	    IPPU.RenderThisFrame = TRUE;
	    IPPU.SkippedFrames = 0;
	}
	else
	{
	    if (IPPU.SkippedFrames < mfs)
	    {
		IPPU.SkippedFrames++;
		IPPU.RenderThisFrame = FALSE;
	    }
	    else
	    {
		IPPU.RenderThisFrame = TRUE;
		IPPU.SkippedFrames = 0;
		next1 = now;
	    }
	}
	next1.tv_usec += Settings.FrameTime;
	if (next1.tv_usec >= 1000000)
	{
	    next1.tv_sec += next1.tv_usec / 1000000;
	    next1.tv_usec %= 1000000;
	}
    }
    else
    {
	if (++IPPU.FrameSkip >= (Settings.TurboMode ? Settings.TurboSkipFrames
						    : Settings.SkipFrames))
	{
	    IPPU.FrameSkip = 0;
	    IPPU.SkippedFrames = 0;
	    IPPU.RenderThisFrame = TRUE;
	}
	else
	{
	    IPPU.SkippedFrames++;
	    IPPU.RenderThisFrame = FALSE;
	}
    }
}

void S9xProcessEvents (bool8_32 block)
{
	uint8 jbtn = 0;
	uint32 num = 0;
	static bool8_32 TURBO = FALSE;

	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		switch(event.type) {
		case SDL_JOYBUTTONDOWN:
			joy_buttons[event.jbutton.button] = 1;
			break;
		case SDL_JOYBUTTONUP:
			joy_buttons[event.jbutton.button] = 0;
			break;
		case SDL_JOYAXISMOTION:
			switch(event.jaxis.axis) {
				case JA_LR:
					if(event.jaxis.value == 0)
						joy_axes[JA_LR] = CENTER;
					else if(event.jaxis.value > 0)
						joy_axes[JA_LR] = RIGHT;
					else
						joy_axes[JA_LR] = LEFT;
				break;
				case JA_UD:
					if(event.jaxis.value == 0)
						joy_axes[JA_UD] = CENTER;
					else if(event.jaxis.value > 0)
						joy_axes[JA_UD] = DOWN;
					else
						joy_axes[JA_UD] = UP;
				break;
			}
		case SDL_KEYDOWN:
			keyssnes = SDL_GetKeyState(NULL);

			if (event.key.keysym.sym == sfc_key[QUIT])
				S9xExit();

      		if (event.key.keysym.sym == SDLK_0)
				Settings.DisplayFrameRate = !Settings.DisplayFrameRate;
		    else if (event.key.keysym.sym == SDLK_1)	PPU.BG_Forced ^= 1;
		    else if (event.key.keysym.sym == SDLK_2)	PPU.BG_Forced ^= 2;
		    else if (event.key.keysym.sym == SDLK_3)	PPU.BG_Forced ^= 4;
		    else if (event.key.keysym.sym == SDLK_4)	PPU.BG_Forced ^= 8;
		    else if (event.key.keysym.sym == SDLK_5)	PPU.BG_Forced ^= 16;
			else if (event.key.keysym.sym == SDLK_6)	num = 1;
			else if (event.key.keysym.sym == SDLK_7)	num = 2;
			else if (event.key.keysym.sym == SDLK_8)	num = 3;
			else if (event.key.keysym.sym == SDLK_9)	num = 4;
			else if (event.key.keysym.sym == SDLK_r) {
			    if (event.key.keysym.mod & KMOD_SHIFT)
					S9xReset();
			}
			if (num) {
				char fname[256], ext[8];
				sprintf(ext, ".00%d", num - 1);
				strcpy(fname, S9xGetFilename (ext));
			    if (event.key.keysym.mod & KMOD_SHIFT)
				    S9xFreezeGame (fname);
				else
					S9xLoadSnapshot (fname);
			}
			break;
		case SDL_KEYUP:
			keyssnes = SDL_GetKeyState(NULL);
		}
		if (keyssnes[sfc_key[ACCEL]] == SDL_PRESSED) {
			if (!TURBO) {
				TURBO = TRUE;
				OldSkipFrame = Settings.SkipFrames;
				Settings.SkipFrames = 10;
			}
		} else {
			if (TURBO) {
				TURBO = FALSE;
				Settings.SkipFrames = OldSkipFrame;
			}
		}
	}
}

static long log2 (long num)
{
    long n = 0;

    while (num >>= 1)
	n++;

    return (n);
}

#ifndef _ZAURUS
static long power (int num, int pow)
{
    long val = num;
    int i;
    
    if (pow == 0)
	return (1);

    for (i = 1; i < pow; i++)
	val *= num;

    return (val);
}
#endif

static int Rates[8] =
{
    0, 8192, 11025, 16000, 22050, 29300, 36600, 44000
};

static int BufferSizes [8] =
{
    0, 256, 256, 256, 512, 512, 1024, 1024
};

bool8_32 S9xOpenSoundDevice (int mode, bool8_32 stereo, int buffer_size)
{
    int J, K;

    if ((so.sound_fd = open ("/dev/dsp", O_WRONLY)) < 0)
    {
	perror ("/dev/dsp");
	return (FALSE);
    }

    J = AFMT_S16_LE;
//    J = AFMT_U8;
    if (ioctl (so.sound_fd, SNDCTL_DSP_SETFMT, &J) < 0)
    {
	perror ("ioctl SNDCTL_DSP_SETFMT");
	return (FALSE);
    }
	so.sixteen_bit = TRUE;
    so.stereo = stereo;
    if (ioctl (so.sound_fd, SNDCTL_DSP_STEREO, &so.stereo) < 0)
    {
	perror ("ioctl SNDCTL_DSP_STEREO");
	return (FALSE);
    }
    
    so.playback_rate = Rates[mode & 0x07];
 //   so.playback_rate = 16000;
    if (ioctl (so.sound_fd, SNDCTL_DSP_SPEED, &so.playback_rate) < 0)
    {
	perror ("ioctl SNDCTL_DSP_SPEED");
	return (FALSE);
    }

    S9xSetPlaybackRate (so.playback_rate);

//    if (buffer_size == 0)
	so.buffer_size = buffer_size = BufferSizes [mode & 7];
//	buffer_size = so.buffer_size = 256;
		
    if (buffer_size > MAX_BUFFER_SIZE / 4)
	buffer_size = MAX_BUFFER_SIZE / 4;
//    if (so.sixteen_bit)
	buffer_size *= 2;
    if (so.stereo)
	buffer_size *= 2;
	
    int power2 = log2 (buffer_size);
    J = K = power2 | (3 << 16);
    if (ioctl (so.sound_fd, SNDCTL_DSP_SETFRAGMENT, &J) < 0)
    {
	perror ("ioctl SNDCTL_DSP_SETFRAGMENT");
	return (FALSE);
    }
    
    printf ("Rate: %d, Buffer size: %d, 16-bit: %s, Stereo: %s, Encoded: %s\n",
	    so.playback_rate, so.buffer_size, so.sixteen_bit ? "yes" : "no",
	    so.stereo ? "yes" : "no", so.encoded ? "yes" : "no");

    return (TRUE);
}

void S9xUnixProcessSound (void)
{
}

static uint8 Buf[MAX_BUFFER_SIZE] __attribute__((aligned(4)));

#define FIXED_POINT 0x10000
#define FIXED_POINT_SHIFT 16
#define FIXED_POINT_REMAINDER 0xffff

static volatile bool8 block_signal = FALSE;
static volatile bool8 block_generate_sound = FALSE;
static volatile bool8 pending_signal = FALSE;

void S9xGenerateSound ()
{
    int bytes_so_far = (so.samples_mixed_so_far << 1);
//    int bytes_so_far = so.sixteen_bit ? (so.samples_mixed_so_far << 1) :
//				        so.samples_mixed_so_far;
#ifndef _ZAURUS
    if (Settings.SoundSync == 2)
    {
	// Assumes sound is signal driven
	while (so.samples_mixed_so_far >= so.buffer_size && !so.mute_sound)
	    pause ();
    }
    else
#endif
    if (bytes_so_far >= so.buffer_size)
	return;

#ifdef USE_THREADS
    if (Settings.ThreadSound)
    {
	if (block_generate_sound || pthread_mutex_trylock (&mutex))
	    return;
    }
#endif

    block_signal = TRUE;

    so.err_counter += so.err_rate;
    if (so.err_counter >= FIXED_POINT)
    {
        int sample_count = so.err_counter >> FIXED_POINT_SHIFT;
	int byte_offset;
	int byte_count;

        so.err_counter &= FIXED_POINT_REMAINDER;
	if (so.stereo)
	    sample_count <<= 1;
	byte_offset = bytes_so_far + so.play_position;
	    
	do
	{
	    int sc = sample_count;
	    byte_count = sample_count;
//	    if (so.sixteen_bit)
		byte_count <<= 1;
	    
	    if ((byte_offset & SOUND_BUFFER_SIZE_MASK) + byte_count > SOUND_BUFFER_SIZE)
	    {
		sc = SOUND_BUFFER_SIZE - (byte_offset & SOUND_BUFFER_SIZE_MASK);
		byte_count = sc;
//		if (so.sixteen_bit)
		    sc >>= 1;
	    }
	    if (bytes_so_far + byte_count > so.buffer_size)
	    {
		byte_count = so.buffer_size - bytes_so_far;
		if (byte_count == 0)
		    break;
		sc = byte_count;
//		if (so.sixteen_bit)
		    sc >>= 1;
	    }
	    S9xMixSamplesO (Buf, sc,
			    byte_offset & SOUND_BUFFER_SIZE_MASK);
	    so.samples_mixed_so_far += sc;
	    sample_count -= sc;
	    bytes_so_far = (so.samples_mixed_so_far << 1);
//	    bytes_so_far = so.sixteen_bit ? (so.samples_mixed_so_far << 1) :
//	 	           so.samples_mixed_so_far;
	    byte_offset += byte_count;
	} while (sample_count > 0);
    }
    block_signal = FALSE;

#ifdef USE_THREADS
    if (Settings.ThreadSound)
	pthread_mutex_unlock (&mutex);
    else
#endif    
    if (pending_signal)
    {
	S9xProcessSound (NULL);
	pending_signal = FALSE;
    }
}

void *S9xProcessSound (void *)
{
/*    audio_buf_info info;

    if (!Settings.ThreadSound &&
	(ioctl (so.sound_fd, SNDCTL_DSP_GETOSPACE, &info) == -1 ||
	 info.bytes < so.buffer_size))
    {
	return (NULL);
    }
*/
#ifdef USE_THREADS
    do
    {
#endif

    int sample_count = so.buffer_size;
    int byte_offset;

//    if (so.sixteen_bit)
	sample_count >>= 1;
 
#ifdef USE_THREADS
//    if (Settings.ThreadSound)
	pthread_mutex_lock (&mutex);
//    else
#endif
//    if (block_signal)
//    {
//	pending_signal = TRUE;
//	return (NULL);
//    }

    block_generate_sound = TRUE;

    if (so.samples_mixed_so_far < sample_count)
    {
//	byte_offset = so.play_position + 
//		      (so.sixteen_bit ? (so.samples_mixed_so_far << 1)
//				      : so.samples_mixed_so_far);
	byte_offset = so.play_position + (so.samples_mixed_so_far << 1);

//printf ("%d:", sample_count - so.samples_mixed_so_far); fflush (stdout);
	if (Settings.SoundSync == 2)
	{
	    memset (Buf + (byte_offset & SOUND_BUFFER_SIZE_MASK), 0,
		    sample_count - so.samples_mixed_so_far);
	}
	else
	    S9xMixSamplesO (Buf, sample_count - so.samples_mixed_so_far,
			    byte_offset & SOUND_BUFFER_SIZE_MASK);
	so.samples_mixed_so_far = 0;
    }
    else
	so.samples_mixed_so_far -= sample_count;
    
//    if (!so.mute_sound)
    {
	int I;
	int J = so.buffer_size;

	byte_offset = so.play_position;
	so.play_position = (so.play_position + so.buffer_size) & SOUND_BUFFER_SIZE_MASK;

#ifdef USE_THREADS
//	if (Settings.ThreadSound)
	    pthread_mutex_unlock (&mutex);
#endif
	block_generate_sound = FALSE;
	do
	{
	    if (byte_offset + J > SOUND_BUFFER_SIZE)
	    {
		I = write (so.sound_fd, (char *) Buf + byte_offset,
			   SOUND_BUFFER_SIZE - byte_offset);
		if (I > 0)
		{
		    J -= I;
		    byte_offset = (byte_offset + I) & SOUND_BUFFER_SIZE_MASK;
		}
	    }
	    else
	    {
		I = write (so.sound_fd, (char *) Buf + byte_offset, J);
		if (I > 0)
		{
		    J -= I;
		    byte_offset = (byte_offset + I) & SOUND_BUFFER_SIZE_MASK;
		}
	    }
	} while ((I < 0 && errno == EINTR) || J > 0);
    }

#ifdef USE_THREADS
//    } while (Settings.ThreadSound);
    } while (1);
#endif

    return (NULL);
}

uint32 S9xReadJoypad (int which1)
{
	uint32 val=0x80000000;

	if (keyssnes[sfc_key[L_1]] == SDL_PRESSED || joy_buttons[JB_L])		val |= SNES_TL_MASK;
	if (keyssnes[sfc_key[R_1]] == SDL_PRESSED || joy_buttons[JB_R])		val |= SNES_TR_MASK;
	if (keyssnes[sfc_key[X_1]] == SDL_PRESSED || joy_buttons[JB_X])		val |= SNES_X_MASK;
	if (keyssnes[sfc_key[Y_1]] == SDL_PRESSED || joy_buttons[JB_Y])		val |= SNES_Y_MASK;
	if (keyssnes[sfc_key[B_1]] == SDL_PRESSED || joy_buttons[JB_B])		val |= SNES_B_MASK;
	if (keyssnes[sfc_key[A_1]] == SDL_PRESSED || joy_buttons[JB_A])		val |= SNES_A_MASK;
	if (keyssnes[sfc_key[START_1]] == SDL_PRESSED || joy_buttons[JB_START])	val |= SNES_START_MASK;
	if (keyssnes[sfc_key[SELECT_1]] == SDL_PRESSED || joy_buttons[JB_SELECT])	val |= SNES_SELECT_MASK;
	if (keyssnes[sfc_key[UP_1]] == SDL_PRESSED || joy_axes[JA_UD] == UP)		val |= SNES_UP_MASK;
	if (keyssnes[sfc_key[DOWN_1]] == SDL_PRESSED || joy_axes[JA_UD] == DOWN)	val |= SNES_DOWN_MASK;
	if (keyssnes[sfc_key[LEFT_1]] == SDL_PRESSED || joy_axes[JA_LR] == LEFT)	val |= SNES_LEFT_MASK;
	if (keyssnes[sfc_key[RIGHT_1]] == SDL_PRESSED || joy_axes[JA_LR] == RIGHT)	val |= SNES_RIGHT_MASK;
/*	if (keyssnes[sfc_key[UP_2]] == SDL_PRESSED)	val |= SNES_UP_MASK;
	if (keyssnes[sfc_key[DOWN_2]] == SDL_PRESSED)	val |= SNES_DOWN_MASK;
	if (keyssnes[sfc_key[LEFT_2]] == SDL_PRESSED)	val |= SNES_LEFT_MASK;
	if (keyssnes[sfc_key[RIGHT_2]] == SDL_PRESSED)	val |= SNES_RIGHT_MASK;
	if (keyssnes[sfc_key[LU_2]] == SDL_PRESSED)	val |= SNES_LEFT_MASK | SNES_UP_MASK;
	if (keyssnes[sfc_key[LD_2]] == SDL_PRESSED)	val |= SNES_LEFT_MASK | SNES_DOWN_MASK;
	if (keyssnes[sfc_key[RU_2]] == SDL_PRESSED)	val |= SNES_RIGHT_MASK | SNES_UP_MASK;
	if (keyssnes[sfc_key[RD_2]] == SDL_PRESSED)	val |= SNES_RIGHT_MASK | SNES_DOWN_MASK; */
	return(val);
}

#if 0
void S9xParseConfigFile ()
{
    int i, t = 0;
    char *b, buf[10];
    struct ffblk f;

    set_config_file("SNES9X.CFG");

    if (findfirst("SNES9X.CFG", &f, 0) != 0)
    {
        set_config_int("Graphics", "VideoMode", -1);
        set_config_int("Graphics", "AutoFrameskip", 1);
        set_config_int("Graphics", "Frameskip", 0);
        set_config_int("Graphics", "Shutdown", 1);
        set_config_int("Graphics", "FrameTimePAL", 20000);
        set_config_int("Graphics", "FrameTimeNTSC", 16667);
        set_config_int("Graphics", "Transparency", 0);
        set_config_int("Graphics", "HiColor", 0);
        set_config_int("Graphics", "Hi-ResSupport", 0);
        set_config_int("Graphics", "CPUCycles", 100);
        set_config_int("Graphics", "Scale", 0);
        set_config_int("Graphics", "VSync", 0);
        set_config_int("Sound", "APUEnabled", 1);
        set_config_int("Sound", "SoundPlaybackRate", 7);
        set_config_int("Sound", "Stereo", 1);
        set_config_int("Sound", "SoundBufferSize", 256);
        set_config_int("Sound", "SPCToCPURatio", 2);
        set_config_int("Sound", "Echo", 1);
        set_config_int("Sound", "SampleCaching", 1);
        set_config_int("Sound", "MasterVolume", 1);
        set_config_int("Peripherals", "Mouse", 1);
        set_config_int("Peripherals", "SuperScope", 1);
        set_config_int("Peripherals", "MultiPlayer5", 1);
        set_config_int("Peripherals", "Controller", 0);
        set_config_int("Controllers", "Type", JOY_TYPE_AUTODETECT);
        set_config_string("Controllers", "Button1", "A");
        set_config_string("Controllers", "Button2", "B");
        set_config_string("Controllers", "Button3", "X");
        set_config_string("Controllers", "Button4", "Y");
        set_config_string("Controllers", "Button5", "TL");
        set_config_string("Controllers", "Button6", "TR");
        set_config_string("Controllers", "Button7", "START");
        set_config_string("Controllers", "Button8", "SELECT");
        set_config_string("Controllers", "Button9", "NONE");
        set_config_string("Controllers", "Button10", "NONE");
    }

    mode = get_config_int("Graphics", "VideoMode", -1);
    Settings.SkipFrames = get_config_int("Graphics", "AutoFrameskip", 1);
    if (!Settings.SkipFrames)
       Settings.SkipFrames = get_config_int("Graphics", "Frameskip", AUTO_FRAMERATE);
    else
       Settings.SkipFrames = AUTO_FRAMERATE;
    Settings.ShutdownMaster = get_config_int("Graphics", "Shutdown", TRUE);
    Settings.FrameTimePAL = get_config_int("Graphics", "FrameTimePAL", 20000);
    Settings.FrameTimeNTSC = get_config_int("Graphics", "FrameTimeNTSC", 16667);
    Settings.FrameTime = Settings.FrameTimeNTSC;
    Settings.Transparency = get_config_int("Graphics", "Transparency", FALSE);
    Settings.SixteenBit = get_config_int("Graphics", "HiColor", FALSE);
    Settings.SupportHiRes = get_config_int("Graphics", "Hi-ResSupport", FALSE);
    i = get_config_int("Graphics", "CPUCycles", 100);
    Settings.H_Max = (i * SNES_CYCLES_PER_SCANLINE) / i;
    stretch = get_config_int("Graphics", "Scale", 0);
    _vsync = get_config_int("Graphics", "VSync", 0);

    Settings.APUEnabled = get_config_int("Sound", "APUEnabled", TRUE);
    Settings.SoundPlaybackRate = get_config_int("Sound", "SoundPlaybackRate", 7);
    Settings.Stereo = get_config_int("Sound", "Stereo", TRUE);
    Settings.SoundBufferSize = get_config_int("Sound", "SoundBufferSize", 256);
    Settings.SPCTo65c816Ratio = get_config_int("Sound", "SPCToCPURatio", 2);
    Settings.DisableSoundEcho = get_config_int("Sound", "Echo", TRUE) ? FALSE : TRUE;
    Settings.DisableSampleCaching = get_config_int("Sound", "SampleCaching", TRUE) ? FALSE : TRUE;
    Settings.DisableMasterVolume = get_config_int("Sound", "MasterVolume", TRUE) ? FALSE : TRUE;

    Settings.Mouse = get_config_int("Peripherals", "Mouse", TRUE);
    Settings.SuperScope = get_config_int("Peripherals", "SuperScope", TRUE);
    Settings.MultiPlayer5 = get_config_int("Peripherals", "MultiPlayer5", TRUE);
    Settings.ControllerOption = (uint32)get_config_int("Peripherals", "Controller", SNES_MULTIPLAYER5);

    joy_type = get_config_int("Controllers", "Type", JOY_TYPE_AUTODETECT);
    for (i = 0; i < 10; i++)
    {
        sprintf(buf, "Button%d", i+1);
        b = get_config_string("Controllers", buf, "NONE");
        if (!strcasecmp(b, "A"))
        {JOY_BUTTON_INDEX[t] = i; SNES_BUTTON_MASKS[t++] = SNES_A_MASK;}
        else if (!strcasecmp(b, "B"))
        {JOY_BUTTON_INDEX[t] = i; SNES_BUTTON_MASKS[t++] = SNES_B_MASK;}
        else if (!strcasecmp(b, "X"))
        {JOY_BUTTON_INDEX[t] = i; SNES_BUTTON_MASKS[t++] = SNES_X_MASK;}
        else if (!strcasecmp(b, "Y"))
        {JOY_BUTTON_INDEX[t] = i; SNES_BUTTON_MASKS[t++] = SNES_Y_MASK;}
        else if (!strcasecmp(b, "TL"))
        {JOY_BUTTON_INDEX[t] = i; SNES_BUTTON_MASKS[t++] = SNES_TL_MASK;}
        else if (!strcasecmp(b, "TR"))
        {JOY_BUTTON_INDEX[t] = i; SNES_BUTTON_MASKS[t++] = SNES_TR_MASK;}
        else if (!strcasecmp(b, "START"))
        {JOY_BUTTON_INDEX[t] = i; SNES_BUTTON_MASKS[t++] = SNES_START_MASK;}
        else if (!strcasecmp(b, "SELECT"))
        {JOY_BUTTON_INDEX[t] = i; SNES_BUTTON_MASKS[t++] = SNES_SELECT_MASK;}
    }
}
#endif
#ifndef _ZAURUS
static int S9xCompareSDD1IndexEntries (const void *p1, const void *p2)
{
    return (*(uint32 *) p1 - *(uint32 *) p2);
}

void S9xLoadSDD1Data ()
{
    char filename [_MAX_PATH + 1];
    char index [_MAX_PATH + 1];
    char data [_MAX_PATH + 1];
    char patch [_MAX_PATH + 1];

    Memory.FreeSDD1Data ();

    strcpy (filename, S9xGetSnapshotDirectory ());

    if (strncmp (Memory.ROMName, "Star Ocean", 10) == 0)
	strcat (filename, "/socnsdd1");
    else
	strcat (filename, "/sfa2sdd1");

    DIR *dir = opendir (filename);

    index [0] = 0;
    data [0] = 0;
    patch [0] = 0;

    if (dir)
    {
	struct dirent *d;
	
	while ((d = readdir (dir)))
	{
	    if (strcasecmp (d->d_name, "SDD1GFX.IDX") == 0)
	    {
		strcpy (index, filename);
		strcat (index, "/");
		strcat (index, d->d_name);
	    }
	    else
	    if (strcasecmp (d->d_name, "SDD1GFX.DAT") == 0)
	    {
		strcpy (data, filename);
		strcat (data, "/");
		strcat (data, d->d_name);
	    }
	    if (strcasecmp (d->d_name, "SDD1GFX.PAT") == 0)
	    {
		strcpy (patch, filename);
		strcat (patch, "/");
		strcat (patch, d->d_name);
	    }
	}
	closedir (dir);

	if (strlen (index) && strlen (data))
	{
	    FILE *fs = fopen (index, "rb");
	    int len = 0;

	    if (fs)
	    {
		// Index is stored as a sequence of entries, each entry being
		// 12 bytes consisting of:
		// 4 byte key: (24bit address & 0xfffff * 16) | translated block
		// 4 byte ROM offset
		// 4 byte length
		fseek (fs, 0, SEEK_END);
		len = ftell (fs);
		rewind (fs);
		Memory.SDD1Index = (uint8 *) malloc (len);
		fread (Memory.SDD1Index, 1, len, fs);
		fclose (fs);
		Memory.SDD1Entries = len / 12;

		if (!(fs = fopen (data, "rb")))
		{
		    free ((char *) Memory.SDD1Index);
		    Memory.SDD1Index = NULL;
		    Memory.SDD1Entries = 0;
		}
		else
		{
		    fseek (fs, 0, SEEK_END);
		    len = ftell (fs);
		    rewind (fs);
		    Memory.SDD1Data = (uint8 *) malloc (len);
		    fread (Memory.SDD1Data, 1, len, fs);
		    fclose (fs);

		    if (strlen (patch) > 0 &&
			(fs = fopen (patch, "rb")))
		    {
			fclose (fs);
		    }
#ifdef MSB_FIRST
		    // Swap the byte order of the 32-bit value triplets on
		    // MSBFirst machines.
		    uint8 *ptr = Memory.SDD1Index;
		    for (int i = 0; i < Memory.SDD1Entries; i++, ptr += 12)
		    {
			SWAP_DWORD ((*(uint32 *) (ptr + 0)));
			SWAP_DWORD ((*(uint32 *) (ptr + 4)));
			SWAP_DWORD ((*(uint32 *) (ptr + 8)));
		    }
#endif
		    qsort (Memory.SDD1Index, Memory.SDD1Entries, 12,
			   S9xCompareSDD1IndexEntries);
		}
	    }
	}
	else
	{
	    printf ("Decompressed data pack not found in '%s'.\n", filename);
	}
    }
}
#endif
