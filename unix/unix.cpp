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

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <dirent.h>
#include <SDL2/SDL.h>

#undef USE_THREADS
#define USE_THREADS
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <signal.h>

#include <sys/soundcard.h>
#include <sys/mman.h>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/chrono.hpp>

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
#include "joystick.hpp"
#include "soundSystem.hpp"
#include "inputController.hpp"

SoundSystem *sndSys(NULL);
InputController *inputController(NULL);

// rPi port: added all this joystick stuff
// If for some unfathomable reason your joystick has more than 32 buttons or 8 
// axes, you should change these array definitions to reflect that. 
// int8 joy_buttons[NB_MAX_CONTROLLERS][32];
// uint8 joy_axes[NB_MAX_CONTROLLERS][8];

// joystick_t *joy_buttons_maping = NULL;
// unsigned nb_joy_mapping = 0;
// uint8 *joy_button_numbers[NB_MAX_CONTROLLERS] = {0};


void InitTimer ();

extern void S9xDisplayFrameRate (uint8 *, uint32);
extern void S9xDisplayString (const char *string, uint8 *, uint32);
extern SDL_Texture *screen;
extern SDL_Window *sdlWindow;
extern SDL_Renderer *sdlRenderer;

static uint32 ffc = 0;
bool8_32 nso = FALSE, vga = FALSE;
uint32 xs = 320, ys = 240, cl = 0, cs = 0, mfs = 10;
 

char *rom_filename = NULL;
char *snapshot_filename = NULL;

static void signalHandler(int)
{
	std::cerr<<"Signal received. Exiting"<<std::endl;
	S9xExit();
}


void OutOfMemory ()
{
    fprintf (stderr, "\
Snes9X: Memory allocation failure - not enough RAM/virtual memory available.\n\
        S9xExiting...\n");
    Memory.Deinit ();
    S9xDeinitAPU ();

    if (inputController)
	    delete inputController;

    if (sndSys)
	    delete sndSys;

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

    S9xInitDisplay (argc, argv);
    if (!S9xGraphicsInit ())
	    OutOfMemory ();

    try
    {
	    inputController = new InputController();
    }
    catch (SnesBadConfigFileException e)
    {
	    try
	    {
		    InputConfig tmp(true); //Create new config file
		    tmp.save(std::string(S9xGetSnapshotDirectory()) + "/" + INPUT_CONFIG_DEFAULT_FILE);
		    inputController = new InputController();
	    }
	    catch (...)
	    {
		    std::cerr<<"Unable to create a default configuration file."<<std::endl;
		    std::cerr<<"Check permissions of ";
		    std::cerr<<std::string(S9xGetSnapshotDirectory()) + "/" + INPUT_CONFIG_DEFAULT_FILE<<std::endl;
		    S9xExit();
	    }
    }
    catch(...)
    {
	    S9xExit();
    }

    if (!inputController)
	    OutOfMemory ();

    /* SIGTERM management*/
    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);

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
	    S9xMainLoop ();
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
    if (sndSys)
	    delete sndSys;

    if (inputController)
	    delete inputController;

    S9xDeinitDisplay ();
    Memory.SaveSRAM (S9xGetFilename (".srm"));
//    S9xSaveCheatFile (S9xGetFilename (".cht"));
    Memory.Deinit ();
    S9xDeinitAPU ();
    
    exit (0);
}

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
				register uint32 *dp32 = (uint32 *)(GFX.Screen) + ((i + cl) * xs) + lp;
				register uint32 *sp32 = (uint32 *)(GFX.Screen) + (i << 8) + cs;
				for (register uint32 j = 0; j < 256; j++)
					*dp32++ = *sp32++;
			}
		} else {
			for (register uint32 i = 0; i < Height; i++) {
				register uint16 *dp16 = (uint16 *)(GFX.Screen) + ((i + cl) * xs) * 2 + 64;
				register uint16 *sp16 = (uint16 *)(GFX.Screen) + (i << 9) + cs;
				for (register uint32 j = 0; j < 256; j++, dp16+=2, sp16++) {
					*dp16 = *(dp16+1) = *sp16;
				}
			}
		}
		if (Settings.DisplayFrameRate)
		    S9xDisplayFrameRate ((uint8 *)GFX.Screen + 128, 2560);
		if (GFX.InfoString)
		    S9xDisplayString (GFX.InfoString, (uint8 *)GFX.Screen + 128, 2560);
//  		SDL_UnlockSurface(screen);
		if (ffc < 5) {
			SDL_UpdateTexture(screen, NULL, GFX.Screen, xs * 2);
//			SDL_UpdateRect(screen,0,0,0,0);
			++ffc;
		} else
			SDL_UpdateTexture(screen, NULL, GFX.Screen, xs * 2);
//SDL_UpdateRect(screen,64,0,512,480);
	} else {
		if (Settings.SupportHiRes) {
			if (Width > 256) {
				for (register uint32 i = 0; i < Height; i++) {
					register uint16 *dp16 = (uint16 *)(GFX.Screen) + ((i + cl) * xs) + lp;
					register uint32 *sp32 = (uint32 *)(GFX.Screen) + (i << 8) + cs;
					for (register uint32 j = 0; j < 256; j++) {
						*dp16++ = *sp32++;
					}
				}
			} else {
				for (register uint32 i = 0; i < Height; i++) {
					register uint32 *dp32 = (uint32 *)(GFX.Screen) + ((i + cl) * xs / 2) + lp;
					register uint32 *sp32 = (uint32 *)(GFX.Screen) + (i << 8) + cs;
					for (register uint32 j = 0; j < 128; j++) {
						*dp32++ = *sp32++;
					}
				}
			}
		}
		if (Settings.DisplayFrameRate)
		    S9xDisplayFrameRate ((uint8 *)GFX.Screen + 64, 640);
		if (GFX.InfoString)
		    S9xDisplayString (GFX.InfoString, (uint8 *)GFX.Screen + 64, 640);
//  		SDL_UnlockSurface(screen);
		if (ffc < 5) {
			SDL_UpdateTexture(screen, NULL, GFX.Screen, xs * 2);
//			SDL_UpdateRect(screen,0,0,0,0);
			++ffc;
		} else
			SDL_UpdateTexture(screen, NULL, GFX.Screen, xs * 2);
//			SDL_UpdateRect(screen,32,0,256,Height);
		//	SDL_Flip(screen);
	}
	SDL_RenderClear(sdlRenderer);
	SDL_RenderCopy(sdlRenderer, screen, NULL, NULL);
	SDL_RenderPresent(sdlRenderer);
	return(TRUE);
}

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

    const char *slash = strrchr (path, '/');
    if (!slash)
	slash = strrchr (path, '\\');

    const char *dot = strrchr (path, '.');

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
    
    try
    {
	    sndSys = new SoundSystem (Settings.SoundPlaybackRate, "hw:0,0");
    }
    catch (SnesException e)
    {
	    std::cerr<<"Exception occurs during sound system init "<<e<<std::endl;
	    S9xExit();
    }

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

typedef boost::posix_time::milliseconds ms_t;
typedef boost::chrono::duration<double> duration_t;

typedef boost::chrono::duration<long long, boost::micro> microseconds;
typedef boost::chrono::system_clock::time_point boostTime_t;

void S9xSyncSpeed ()
{
	try
	{
		inputController->process();
		inputController->checkGlobal();
	}
	catch (ExitException e)
	{
		S9xExit();
	}


	if (!Settings.TurboMode && Settings.SkipFrames == AUTO_FRAMERATE)
	{
		boostTime_t now(boost::chrono::system_clock::now());
		static boostTime_t next1(now + microseconds(1));

		if (next1 > now)
		{
			if (IPPU.SkippedFrames == 0)
				boost::this_thread::sleep_until(next1);
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
		next1 += microseconds(Settings.FrameTime);
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
    0, 8192, 11025, 16000, 22050, 29300, 36600, 44100
};

static int BufferSizes [8] =
{
    0, 256, 256, 256, 512, 512, 1024, 1024
};

bool8_32 S9xOpenSoundDevice (int mode, bool8_32 stereo, int buffer_size)
{
    int J, K;


	so.sixteen_bit = TRUE;
    so.stereo = stereo;
    
    so.playback_rate = Rates[mode & 0x07];

    S9xSetPlaybackRate (so.playback_rate);
	so.buffer_size = buffer_size = BufferSizes [mode & 7];
		
    if (buffer_size > MAX_BUFFER_SIZE / 4)
	    buffer_size = MAX_BUFFER_SIZE / 4;
    buffer_size *= 2;
    if (so.stereo)
	buffer_size *= 2;
	    
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
