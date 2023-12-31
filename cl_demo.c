/*
Copyright (C) 1996-1997 Id Software, Inc.

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
#include "neh.h"

//#define DZ_VALIDATE 1

static long demofile_len, demofile_start;
static char dz_tempfile[] = "dz_temp.dem";

void CL_FinishTimeDemo (void);

/*
==============================================================================

DEMO CODE

When a demo is playing back, all NET_SendMessages are skipped, and
NET_GetMessages are read from the demo file.

Whenever cl.time gets past the last received message, another message is
read from the demo file.
==============================================================================
*/

/*
====================
CL_CloseDemoFile
====================
*/
void CL_CloseDemoFile (void)
{
	if (!cls.demofile)
		return;

	fclose (cls.demofile);
	cls.demofile = NULL;
}

/*
==============
CL_StopPlayback

Called when a demo file runs out, or the user starts a game
==============
*/
void CL_StopPlayback (void)
{
        char name[MAX_OSPATH];

	if (!cls.demoplayback)
		return;

	cls.demoplayback = false;
	CL_CloseDemoFile ();
		
#ifndef DZ_VALIDATE
	sprintf (name, "%s\\%s", com_gamedir, dz_tempfile);
	remove (name); // Delete temp file
#endif
	
	cls.state = ca_disconnected;
        cls.demopaused = false;

	// Make sure screen is updated shortly after this
	SCR_SetTimeout (0);

	if (cls.timedemo)
		CL_FinishTimeDemo ();
}

/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length and view angles
====================
*/
qboolean CL_WriteDemoMessage (void)
{
	int	    len;
	int	    i;
	float	    f;
	qboolean    Success;

        if (cls.demopaused) return true;
	len = LittleLong (net_message.cursize);
	Success = fwrite (&len, 4, 1, cls.demofile) == 1;
	for (i=0 ; i<3 && Success ; i++)
	{
		f = LittleFloat (cl.viewangles[i]);
		Success = fwrite (&f, 4, 1, cls.demofile) == 1;
	}
	
	if (Success)
		Success = fwrite (net_message.data, net_message.cursize, 1, cls.demofile) == 1;

	if (Success)
		fflush (cls.demofile);
	else
	{
		CL_CloseDemoFile ();
		Con_Printf ("Error writing demofile\n");
	}

	return Success;
}

/*
====================
CL_GetMessage

Handles recording and playback of demos, on top of NET_ code
====================
*/
int CL_GetMessage (void)
{
	int	    r, i;
	float	    f;
	qboolean    Success;

	if	(cls.demoplayback)
	{
		if (cls.demopaused) return 0;

	// decide if it is time to grab the next message		
		if (cls.signon == SIGNONS)	// allways grab until fully connected
		{
			if (cls.timedemo)
			{
				if (host_framecount == cls.td_lastframe)
					return 0;		// allready read this frame's message
				cls.td_lastframe = host_framecount;
			// if this is the second frame, grab the real td_starttime
			// so the bogus time on the first frame doesn't count
				if (host_framecount == cls.td_startframe + 1)
					cls.td_starttime = realtime;
			}
			else if ( /* cl.time > 0 && */ cl.time <= cl.mtime[0])
			{
					return 0;		// don't need another message yet
			}
		}
		
		// Detect EOF, especially for demos in pak files
		if (ftell(cls.demofile) - demofile_start >= demofile_len)
			Host_EndGame ("Missing disconnect in demofile\n");
	
	// get the next message
		Success = fread (&net_message.cursize, 4, 1, cls.demofile) == 1;
		
		VectorCopy (cl.mviewangles[0], cl.mviewangles[1]);
		for (i=0 ; i<3 && Success ; i++)
		{
			Success = fread (&f, 4, 1, cls.demofile) == 1;
			cl.mviewangles[0][i] = LittleFloat (f);
		}
		
		if (Success)
		{
			net_message.cursize = LittleLong (net_message.cursize);
			if (net_message.cursize > MAX_MSGLEN)
				Host_Error ("Demo message %d > MAX_MSGLEN (%d)", net_message.cursize, MAX_MSGLEN);
			Success = fread (net_message.data, net_message.cursize, 1, cls.demofile) == 1;
		}
		
		if (!Success)
		{
			Con_Printf ("Error reading demofile\n");
			CL_Disconnect ();
			return 0;
		}
	
		return 1;
	}

	while (1)
	{
		r = NET_GetMessage (cls.netcon);
		
		if (r != 1 && r != 2)
			return r;
	
	// discard nop keepalive message
		if (net_message.cursize == 1 && net_message.data[0] == svc_nop)
			Con_Printf ("<-- server to client keepalive\n");
		else
			break;
	}

	if (cls.demorecording)
	{
		if (!CL_WriteDemoMessage ())
			return -1; // File write failure
	}
	
	return r;
}

/*
====================
CL_Stop_f

stop recording a demo
====================
*/
void CL_Stop_f (void)
{
	if (cmd_source != src_command)
		return;

	if (!cls.demorecording)
	{
		Con_Printf ("Not recording a demo.\n");
		return;
	}

	if (cls.demofile)
	{
		// write a disconnect message to the demo file
		SZ_Clear (&net_message);
		MSG_WriteByte (&net_message, svc_disconnect);
		CL_WriteDemoMessage ();

		// finish up
		CL_CloseDemoFile ();
	}

	cls.demorecording = false;
        cls.demopaused = false;
	Con_Printf ("Completed demo\n");
}

/*
====================
CL_Record_f

record <demoname> <map> [cd track]
====================
*/
void CL_Record_f (void)
{
	int		c;
	char	name[MAX_OSPATH];
	int		track;

	if (cmd_source != src_command)
		return;

	c = Cmd_Argc();
	if (c != 2 && c != 3 && c != 4)
	{
		Con_Printf ("record <demoname> [<map> [cd track]]\n");
		return;
	}

	if (strstr(Cmd_Argv(1), ".."))
	{
		Con_Printf ("Relative pathnames are not allowed.\n");
		return;
	}

	if (c == 2 && cls.state == ca_connected)
	{
		Con_Printf("Can not record - already connected to server\nClient demo recording must be started before connecting\n");
		return;
	}

// write the forced cd track number, or -1
	if (c == 4)
	{
		track = atoi(Cmd_Argv(3));
		Con_Printf ("Forcing CD track to %i\n", cls.forcetrack);
	}
	else
		track = -1;	

	sprintf (name, "%s/%s", com_gamedir, Cmd_Argv(1));
	
//
// start the map up
//
	if (c > 2)
		Cmd_ExecuteString ( va("map %s", Cmd_Argv(2)), src_command);
	
//
// open the demo file
//
	COM_DefaultExtension (name, ".dem");

	Con_Printf ("recording to %s.\n", name);
	cls.demofile = fopen (name, "wb");
	if (!cls.demofile)
	{
		Con_Printf ("ERROR: couldn't open.\n");
		return;
	}

	cls.forcetrack = track;
	fprintf (cls.demofile, "%i\n", cls.forcetrack);
	
	cls.demorecording = true;
}


/*
====================
CL_PlayDemo_f

playdemo [demoname]
====================
*/
void CL_PlayDemo_f (void)
{
        char     name[MAX_OSPATH], name2[MAX_OSPATH], name3[MAX_OSPATH], name4[MAX_OSPATH]; // HACK :P
        int	 c, compressed = 0;
	qboolean neg = false;

	if (cmd_source != src_command)
		return;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("playdemo <demoname> : plays a demo\n");
		return;
	}

//
// disconnect from server
//
	CL_Disconnect ();


//
// open the demo file
//


        COM_StripExtension(strlwr(Cmd_Argv(1)), name);

        strcpy (name2, name);
	COM_DefaultExtension (name, ".dem");
        COM_DefaultExtension (name2, ".dz");
        strcpy (name3, name2); strcpy (name4, name2);

        Con_Printf ("Playing demo from %s.\n", name);

        if (COM_FindOnly(name) == 1)
		Con_Printf ("Loading...\n");
	else if (COM_FindOnly(name2) == 1)
	{
		FILE *handle;

		COM_FOpenFile (name3, &handle);
		if (!handle)
			Sys_Error ("Unable to locate located file..");

#ifdef DZ_VALIDATE
		strcpy (dz_tempfile, name);
#endif
		sprintf (name, "%s\\%s", com_gamedir, dz_tempfile);

		Con_Printf ("Decompressing...\n");
		uncompress_file (handle, name4, name); // Also closes handle
		strcpy (name, dz_tempfile);
        }

#ifdef DZ_VALIDATE
        cls.demofile = NULL;
#else
        demofile_len = COM_FOpenFile (name, &cls.demofile);
#endif

        if (!cls.demofile)
	{
		Con_Printf ("ERROR: couldn't open %s\n", name);
		cls.demonum = -1;               // stop demo loop
		return;
        }

        demofile_start = ftell (cls.demofile);

	cls.demoplayback = true;
	cls.state = ca_connected;
	cls.forcetrack = 0;

	while ((c = getc(cls.demofile)) != '\n')
		if (c == '-')
			neg = true;
		else
			cls.forcetrack = cls.forcetrack * 10 + (c - '0');

	if (neg)
		cls.forcetrack = -cls.forcetrack;
// ZOID, fscanf is evil
//	fscanf (cls.demofile, "%i\n", &cls.forcetrack);
}

/*
====================
CL_FinishTimeDemo

====================
*/
void CL_FinishTimeDemo (void)
{
	int		frames;
	float	time;
	
	cls.timedemo = false;
	
// the first frame didn't count
	frames = (host_framecount - cls.td_startframe) - 1;
	time = realtime - cls.td_starttime;
	if (!time)
		time = 1;
	Con_Printf ("%i frames %5.1f seconds %5.1f fps\n", frames, time, frames/time);
}

/*
====================
CL_TimeDemo_f

timedemo [demoname]
====================
*/
void CL_TimeDemo_f (void)
{
	if (cmd_source != src_command)
		return;

	if (Cmd_Argc() != 2)
	{
		Con_Printf ("timedemo <demoname> : gets demo speeds\n");
		return;
	}

	CL_PlayDemo_f ();
	
// cls.td_starttime will be grabbed at the second frame of the demo, so
// all the loading time doesn't get counted
	
	cls.timedemo = true;
	cls.td_startframe = host_framecount;
	cls.td_lastframe = -1;		// get a new message this frame
}

