#include "quakedef.h"
#include "winquake.h"
#include "neh.h"
//FMOD removed from the Nehahra | AlexQuaker
//#include "fmod/fmod.h"
//#include "fmod/fmod_errors.hg"
#include "SDL2/include/SDL.h"
#include "libxmp/xmp.h"

#if ((XMP_VERCODE+0) < 0x040200)
#error libxmp version 4.2 or newer is required
#endif
//end below defines from snd_xmp.c from Quakespasm
int     smoketexture, DoFullbright;
int     NehGameType = TYPE_DEMO;
int     modplaying = 0, mp3playing = 0;

cvar_t  r_dynamicbothsides = {"r_dynamicbothsides", "0"}; // LordHavoc: dynamic lights no longer light backfaces
func_t RestoreGame;


// Ender
// ----------------------------------------------------------------------
// [FOG]  Gl Cvar's
        cvar_t       gl_fogenable = {"gl_fogenable", "0"};
        cvar_t       gl_fogdisable = {"gl_fogdisable", "0"}; // Overrides
        cvar_t       gl_fogdensity = {"gl_fogdensity", "0.8"};
        cvar_t       gl_fogred = {"gl_fogred","0.3"};
        cvar_t       gl_fogblue = {"gl_fogblue","0.3"};
        cvar_t       gl_foggreen = {"gl_foggreen","0.3"};
        cvar_t       gl_skyclip = {"gl_skyclip", "4608"}; // Farclip for sky in fog
        cvar_t       gl_skyfix = {"gl_skyfix", "1"};	  // Fix to draw skybox over world (multitexture only)
	qboolean     gl_do_skyfix; // Combines all conditions for performing skyfix


// [INT]  Interpolation
        cvar_t  r_interpolate_model_animation = {"r_interpolate_model_animation", "1", true};
        cvar_t  r_interpolate_model_transform = {"r_interpolate_model_transform", "1", true};
        cvar_t  r_interpolate_model_weapon = {"r_interpolate_model_weapon", "1", true};

// [APH]  Model_Alpha
        cvar_t       gl_notrans = {"gl_notrans", "0"};
        float        model_alpha;

// [DEM]  Cutscene demo usage
        cvar_t  nehx00 = {"nehx00", "0"};cvar_t nehx01 = {"nehx01", "0"};
        cvar_t  nehx02 = {"nehx02", "0"};cvar_t nehx03 = {"nehx03", "0"};
        cvar_t  nehx04 = {"nehx04", "0"};cvar_t nehx05 = {"nehx05", "0"};
        cvar_t  nehx06 = {"nehx06", "0"};cvar_t nehx07 = {"nehx07", "0"};
        cvar_t  nehx08 = {"nehx08", "0"};cvar_t nehx09 = {"nehx09", "0"};
        cvar_t  nehx10 = {"nehx10", "0"};cvar_t nehx11 = {"nehx11", "0"};
        cvar_t  nehx12 = {"nehx12", "0"};cvar_t nehx13 = {"nehx13", "0"};
        cvar_t  nehx14 = {"nehx14", "0"};cvar_t nehx15 = {"nehx15", "0"};
        cvar_t  nehx16 = {"nehx16", "0"};cvar_t nehx17 = {"nehx17", "0"};
        cvar_t  nehx18 = {"nehx18", "0"};cvar_t nehx19 = {"nehx19", "0"};
        cvar_t  cutscene = {"cutscene", "1", true};

// [EVL]  Evaluation Shortcuts
        int eval_gravity, eval_items2, eval_alpha, eval_fullbright;
        int eval_idealpitch, eval_pitch_speed;

// [MOD]  FMOD stuff, removed.
		//libxmp stuff added
        //FMUSIC_MODULE *mod = NULL;
		xmp_context c = NULL;
        cvar_t        modvolume = {"modvolume", "0.5"};

// [MSC]  Misc
        cvar_t       gl_rsmoke = {"gl_rsmoke", "1"};
        cvar_t       nospr32   = {"nospr32", "0"};
        cvar_t       slowmo    = {"slowmo", "1.0"};
        cvar_t       r_waterripple = {"r_waterripple", "3"};
        cvar_t       r_nowaterripple = {"r_nowaterripple", "0"}; // Overrides
        cvar_t       tweak = {"tweak", "1"};

// Light Flare = 3,2,1
// Lighting Gun = 1,1,50

        cvar_t       gl_glows = {"gl_glows", "1", true};

        int  eval_gravity, num_sfxorig;
        void NEH_sky(void);
        void CheckMode(void);

void MOD_stop (void)//modified by AlexQuaker
{
	if (modplaying)
	{
	//	FMUSIC_FreeSong (mod);
	SDL_PauseAudio(1);
	xmp_stop_module(c);
	xmp_release_module(c);        /* unload module */
	xmp_end_player(c);
	Con_Print("Stopped playing mod\n");

	modplaying = 0;
	}
}

void MOD_SetVolume_f (void)//modified by AlexQuaker
{

	if (!modplaying)
		return;

//	FMUSIC_SetMasterVolume (mod, modvolume.value * 128);
	xmp_set_player(c,XMP_PLAYER_VOLUME,modvolume.value * 100);//XMP_PLAYER_VOLUME in xmp is the same as FMUSIC_SetMasterVolume in fmod
}

static void fill_music(void *udata, unsigned __int8	 *stream, int len)//Added by AlexQuaker
{
    xmp_play_buffer(udata, stream, len, 0);
}
int music_init(xmp_context c, int sampling_rate, int channels)//Added by AlexQuaker from LibXMP samples
{
    SDL_AudioSpec a;

    a.freq = sampling_rate;
    a.format = (AUDIO_S16);
    a.channels = channels;
    a.samples = 2048;
    a.callback = fill_music;
    a.userdata = c;

    if (SDL_OpenAudio(&a, NULL) < 0) {
            Con_Printf("%s\n", SDL_GetError());
            return -1;
    }
	Con_Print("SDL init successful\n");
	return 0;
	
}
void MOD_play (void)//modified by AlexQuaker, significantly
{
	static char modname[256];
	char	    modfile[256];
	char	    *buffer;
	int	    mark;
	//Taken comments from Quakespasm | AlexQuaker
/* need to load the whole file into memory and pass it to libxmp
 * using xmp_load_module_from_memory() which requires libxmp >= 4.2.
 * libxmp-4.0/4.1 only have xmp_load_module() which accepts a file
 * name which isn't good with files in containers like paks, etc. */
	byte *moddata;
	short mod_load_errcode;
	short xmp_player_errcode;
	short result;
	
	if (modplaying)
	{
		if (!strcmp(modname, Cmd_Argv(1)))
			return; // Ignore repeated play commands of the same song

		MOD_stop ();
	}
	
	strcpy (modname, Cmd_Argv(1));

	if (strlen(modname) < 3)
	{
		Con_Print ("Format: PlayMod <filename.ext>\n");
		return;
	}
	mark = Hunk_LowMark ();
	buffer = (char *)COM_LoadHunkFile (modname);

	if (!buffer)
	{
		Hunk_FreeToLowMark (mark);
		Con_Printf ("Couldn't find: %s\n", Cmd_Argv(1));
		return;
	}

	moddata = (byte *) COM_LoadHunkFile(modname);
//	xmp_set_player(c, XMP_PLAYER_INTERP, XMP_INTERP_SPLINE);
//	xmp_set_player(c, XMP_PLAYER_SMPCTL, XMP_SMPCTL_SKIP);
//	xmp_set_player(c, XMP_PLAYER_DEFPAN, 100);
	
	mod_load_errcode = xmp_load_module_from_memory(c, moddata, com_filesize);
	//mod = xmp_load_module(c, modname);
	if (mod_load_errcode != 0) 
	{
		Con_Printf ("Could not load module %s\n", Cmd_Argv(1));
		Con_Printf ("Error code is %s\n", mod_load_errcode);
		return;
	}
	//mod = FMUSIC_LoadSongMemory (buffer, com_filesize);
	Hunk_FreeToLowMark (mark);

	/*if (!mod)
	{
		Con_Printf ("%s\n", FMOD_ErrorString(FSOUND_GetError()));
		return;
	}*/

	modplaying = 1;
	//MOD_SetVolume_f ();//moved down | AlexQuaker
	//FMUSIC_PlaySong (mod);
	xmp_player_errcode = xmp_start_player(c,44100,0);//changed freq to modern 44100 Hz | AlexQuaker
	if (xmp_player_errcode != 0)
	{
	Con_Printf ("Could not start libXMP player, error code is %s\n",xmp_player_errcode);
	return;
	}
	
	/* interpolation type, default is XMP_INTERP_LINEAR */
	xmp_set_player(c, XMP_PLAYER_INTERP, XMP_INTERP_SPLINE);


	SDL_PauseAudio(0);//This, in fact, is the "Play" command in SDL.
	MOD_SetVolume_f ();//it should be here because the volume cannot be set if not playing the mod... Default volume is 100% in xmp
	Con_Printf("Playing mod: %s\n", Cmd_Argv(1)); 
	
}

void MOD_init(void)
{
	if (COM_CheckParm("-nosound"))
		return;
	
	c = xmp_create_context();
	if (c == NULL)
	{
		return;
	}
	else
	{
	Con_Printf ("LibXMP init successful\n");
	}
	music_init(c, 44100, 2);//SDL initialization for xmp
	//below code doesn't seem to be needed anymore.| AlexQuaker..
	//FSOUND_SetBufferSize(300);

	//if (!FSOUND_Init(11025, 32, 0)) // Ender: Changed init frequency
	//{
	//	Con_Printf("%s\n", FMOD_ErrorString(FSOUND_GetError()));
	//	return;
	//}

//	FSOUND_SetMixer(FSOUND_MIXER_AUTODETECT);

	/*switch (FSOUND_GetMixer())
	{
		case FSOUND_MIXER_BLENDMODE:     Con_Printf("FSOUND_MIXER_BLENDMODE\n");     break;
		case FSOUND_MIXER_MMXP5:         Con_Printf("FSOUND_MIXER_MMXP5\n");         break;
		case FSOUND_MIXER_MMXP6:         Con_Printf("FSOUND_MIXER_MMXP6\n");         break;
		case FSOUND_MIXER_QUALITY_FPU:   Con_Printf("FSOUND_MIXER_QUALITY_FPU\n");   break;
		case FSOUND_MIXER_QUALITY_MMXP5: Con_Printf("FSOUND_MIXER_QUALITY_MMXP5\n"); break;
		case FSOUND_MIXER_QUALITY_MMXP6: Con_Printf("FSOUND_MIXER_QUALITY_MMXP6\n"); break;
	}*/

	// Con_Printf("%s\n", FSOUND_GetDriverName(FSOUND_GetDriver()));

	Cvar_RegisterVariable (&modvolume);
	Cmd_AddCommand ("stopmod",MOD_stop);
	Cmd_AddCommand ("playmod",MOD_play);
}

void MOD_done (void)
{
	xmp_free_context(c);
	SDL_CloseAudio();
	Con_Printf ("LibXMP shutdown \n");
	Con_Printf ("SDL shutdown \n");
//	FSOUND_Close ();
}

void pausedemo (void)
{
	if (cls.demopaused)
		Con_Printf ("Unpaused demo\n");
	else
		Con_Printf ("Paused demo\n");
		
	cls.demopaused = !cls.demopaused;
}

void fog (void)
{
	if (Cmd_Argc() < 2 || Cmd_Argc() > 5)
	{
		Con_Printf ("usage:\n");
		Con_Printf ("   fog <density>\n");
		Con_Printf ("   fog <density> <rgb>\n");
		Con_Printf ("   fog <red> <green> <blue>\n");
		Con_Printf ("   fog <density> <red> <green> <blue>\n");
		Con_Printf("current values:\n");
		Con_Printf("   fog is %sabled%s\n", gl_fogenable.value ? "en" : "dis", gl_fogenable.value && gl_fogdisable.value ? " but blocked" : "");
		Con_Printf("   density is %g\n", gl_fogdensity.value);
		Con_Printf("   red   is %g\n", gl_fogred.value);
		Con_Printf("   green is %g\n", gl_foggreen.value);
		Con_Printf("   blue  is %g\n", gl_fogblue.value);
		return;
	}

	if (Cmd_Argc() == 2 && atof(Cmd_Argv(1)) == 0)
	{
		// Disable
		Cvar_SetValue ("gl_fogenable", 0);
		Cvar_SetValue ("gl_fogdensity", 0);
		return;
	}

	Cvar_SetValue ("gl_fogenable", 1);
	
	if (Cmd_Argc() != 4)
		Cvar_Set ("gl_fogdensity", Cmd_Argv(1));
	
	if (Cmd_Argc() == 3)
	{
		// RGB
		Cvar_Set ("gl_fogred", Cmd_Argv(2));
		Cvar_Set ("gl_foggreen", Cmd_Argv(2));
		Cvar_Set ("gl_fogblue", Cmd_Argv(2));
	}
	else if (Cmd_Argc() > 3)
	{
		// Separate colours
		Cvar_Set ("gl_fogred", Cmd_Argv(Cmd_Argc() - 3));
		Cvar_Set ("gl_foggreen", Cmd_Argv(Cmd_Argc() - 2));
		Cvar_Set ("gl_fogblue", Cmd_Argv(Cmd_Argc() - 1));
	}
}

int FindFieldOffset(char *field)
{
	ddef_t *d;
	d = ED_FindField(field);
	if (!d)
		return 0;
	return d->ofs*4;
}

void FindEdictFieldOffsets() {
        dfunction_t *f;

	eval_gravity = FindFieldOffset("gravity");
	eval_alpha = FindFieldOffset("alpha");
	eval_fullbright = FindFieldOffset("fullbright");
	eval_idealpitch = FindFieldOffset("idealpitch");
	eval_pitch_speed = FindFieldOffset("pitch_speed");

        RestoreGame = 0;
        if ((f = ED_FindFunction ("RestoreGame")) != NULL)
           RestoreGame = (func_t)(f - pr_functions);
}

void DoBindings(void)
{
	if (NehGameType != TYPE_DEMO)
		return;

	// Never worked properly, doubtful if desired

/*	Cbuf_InsertText ("bind F1 \"slowmo 0.4\"\n");
	Cbuf_InsertText ("bind F2 \"slowmo 0.6\"\n");
	Cbuf_InsertText ("bind F3 \"slowmo 0.8\"\n");

	Cbuf_InsertText ("bind F4 \"slowmo 1.0\"\n");
	Cbuf_InsertText ("bind F5 \"slowmo 1.4\"\n");
	Cbuf_InsertText ("bind F6 \"slowmo 1.6\"\n");
	Cbuf_InsertText ("bind F7 \"slowmo 1.8\"\n");

	Cbuf_InsertText ("bind F8 \"slowmo 2.0\"\n");
	Cbuf_InsertText ("bind F9 \"slowmo 2.4\"\n");
	Cbuf_InsertText ("bind F10 \"slowmo 2.6\"\n");

	Cbuf_InsertText ("bind PAUSE pausedemo\n");*/
}

void Neh_Init(void)
{
	// Fog
	Cvar_RegisterVariable (&gl_fogenable);
	Cvar_RegisterVariable (&gl_fogdisable);
	Cvar_RegisterVariable (&gl_fogdensity);
	Cvar_RegisterVariable (&gl_fogred);
	Cvar_RegisterVariable (&gl_foggreen);
	Cvar_RegisterVariable (&gl_fogblue);
	Cvar_RegisterVariable (&gl_skyclip);
	Cvar_RegisterVariable (&gl_skyfix);


	Cvar_RegisterVariable (&gl_glows);
	Cvar_RegisterVariable (&r_interpolate_model_animation);
	Cvar_RegisterVariable (&r_interpolate_model_transform);
	Cvar_RegisterVariable (&r_interpolate_model_weapon);

	Cvar_RegisterVariable (&gl_rsmoke);
	Cvar_RegisterVariable (&r_waterripple);
	Cvar_RegisterVariable (&r_nowaterripple);
	Cvar_RegisterVariable (&gl_notrans);
	Cvar_RegisterVariable (&nospr32);

	// Nehahra uses these to pass data around cutscene demos
	Cvar_RegisterVariable (&nehx00);Cvar_RegisterVariable (&nehx01);
	Cvar_RegisterVariable (&nehx02);Cvar_RegisterVariable (&nehx03);
	Cvar_RegisterVariable (&nehx04);Cvar_RegisterVariable (&nehx05);
	Cvar_RegisterVariable (&nehx06);Cvar_RegisterVariable (&nehx07);
	Cvar_RegisterVariable (&nehx08);Cvar_RegisterVariable (&nehx09);
	Cvar_RegisterVariable (&nehx10);Cvar_RegisterVariable (&nehx11);
	Cvar_RegisterVariable (&nehx12);Cvar_RegisterVariable (&nehx13);
	Cvar_RegisterVariable (&nehx14);Cvar_RegisterVariable (&nehx15);
	Cvar_RegisterVariable (&nehx16);Cvar_RegisterVariable (&nehx17);
	Cvar_RegisterVariable (&nehx18);Cvar_RegisterVariable (&nehx19);

	Cvar_RegisterVariable (&cutscene);
	Cvar_RegisterVariable (&tweak);
	Cvar_RegisterVariable (&slowmo);
	Cmd_AddCommand ("pausedemo", pausedemo);
	Cmd_AddCommand ("fog", fog);

	CheckMode ();
	DoBindings ();

	if (COM_CheckParm ("-matrox"))
		Cvar_SetValue ("nospr32", 1);

	MOD_init ();
	DzipInit ();
}

void Neh_BeginFrame(void) // Called from gl_rmain.c (R_RenderView)
{
	if (gl_fogenable.value && !gl_fogdisable.value)
	{
		GLfloat colors[4] = {(GLfloat) gl_fogred.value, (GLfloat) gl_foggreen.value, (GLfloat) gl_fogblue.value, (GLfloat) 1};
		glFogi (GL_FOG_MODE, GL_EXP2);
		glFogf (GL_FOG_DENSITY, (GLfloat) gl_fogdensity.value / 100);
		glFogfv (GL_FOG_COLOR, colors);
		glEnable (GL_FOG);
	}
}

void Neh_EndFrame (void)
{
	if (gl_fogenable.value && !gl_fogdisable.value)
		glDisable (GL_FOG);
}

#define SHOWLMP_MAXLABELS 256
typedef struct showlmp_s
{
	qboolean	isactive;
	float		x;
	float		y;
	char		label[32];
	char		pic[128];
} showlmp_t;

showlmp_t showlmp[SHOWLMP_MAXLABELS];

void SHOWLMP_decodehide()
{
	int i;
	byte *lmplabel;
	lmplabel = MSG_ReadString();
	for (i = 0;i < SHOWLMP_MAXLABELS;i++)
		if (showlmp[i].isactive && strcmp(showlmp[i].label, lmplabel) == 0)
		{
			showlmp[i].isactive = false;
			return;
		}
}

void SHOWLMP_decodeshow()
{
	int i, k;
	byte lmplabel[256], picname[256];
	float x, y;
	strcpy(lmplabel,MSG_ReadString());
	strcpy(picname, MSG_ReadString());
	x = MSG_ReadByte();
	y = MSG_ReadByte();
	k = -1;
	for (i = 0;i < SHOWLMP_MAXLABELS;i++)
		if (showlmp[i].isactive)
		{
			if (strcmp(showlmp[i].label, lmplabel) == 0)
			{
				k = i;
				break; // drop out to replace it
			}
		}
		else if (k < 0) // find first empty one to replace
			k = i;
	if (k < 0)
		return; // none found to replace
	// change existing one
	showlmp[k].isactive = true;
	strcpy(showlmp[k].label, lmplabel);
	strcpy(showlmp[k].pic, picname);
	showlmp[k].x = x;
	showlmp[k].y = y;
}

void SHOWLMP_drawall()
{
	int i;
	for (i = 0;i < SHOWLMP_MAXLABELS;i++)
		if (showlmp[i].isactive)
			Draw_TransPic(showlmp[i].x, showlmp[i].y, Draw_CachePic(showlmp[i].pic));
}

void SHOWLMP_clear()
{
	int i;
	for (i = 0;i < SHOWLMP_MAXLABELS;i++)
		showlmp[i].isactive = false;
}


void CheckMode (void)
{
	int h = 4, h2 = 5;
	int movieinstalled = 0, gameinstalled = 0;

	//  Con_SafePrintf("Beginning check..\n");
	// Check for movies

	if (COM_FindOnly ("hearing.dem") == 1)
		movieinstalled = 1;
	else if (COM_FindOnly ("hearing.dz") == 1)
		movieinstalled = 1;

	//  Con_SafePrintf("Done Movie..\n");

	// Check for game

	if (COM_FindOnly ("maps/neh1m4.bsp") == 1)
		gameinstalled = 1;

	//  Con_SafePrintf("Done Game..\n");

	Con_SafePrintf ("\x02Nehahra type: ");

	if (gameinstalled && movieinstalled)
	{
		NehGameType = TYPE_BOTH;             // mainmenu.lmp
		Con_SafePrintf ("Both\n");
		return;
	}

	if (gameinstalled)
	{
		NehGameType = TYPE_GAME;             // gamemenu.lmp
		Con_SafePrintf ("Game\n");
		return;
	}

	if (movieinstalled)
	{
		NehGameType = TYPE_DEMO;             // demomenu.lmp
		Con_SafePrintf ("Demo\n");
		return;
	}

	Sys_Error ("You must install the Nehahra game first!");
}

void Neh_GameStart (void) // From host_cmd.c
{
	if (RestoreGame)
	{
		 Con_SafePrintf ("Loading enhanced game - RestoreGame()\n");
		 pr_global_struct->time = sv.time;
		 pr_global_struct->self = EDICT_TO_PROG(sv_player);
		 PR_ExecuteProgram (RestoreGame, "RestoreGame");
	}
	else
		Con_SafePrintf ("Loading standard game - !RestoreGame()\n");
}
