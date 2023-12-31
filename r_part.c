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

#define MAX_PARTICLES           2048	    //1024 default max # of particles at one
					    // time
#define ABSOLUTE_MIN_PARTICLES	512	    // no fewer than this no matter what's
					    // on the command line
#define ABSOLUTE_MAX_PARTICLES	10000000

float smokealpha = 0.75;


// Colour Ramp
int ramp1[8] = {0x6f, 0x6d, 0x6b, 0x69, 0x67, 0x65, 0x63, 0x61};
int ramp2[8] = {0x6f, 0x6e, 0x6d, 0x6c, 0x6b, 0x6a, 0x68, 0x66};
int ramp3[8] = {0x6d, 0x6b, 6, 5, 4, 3};

// Texture places
int particle_tex, smoke_tex, flare_tex, rain_tex, blood_tex, bubble_tex;

// Particle lists and such
particle_t  *active_particles, *free_particles, *particles;
int          r_numparticles; // Current active particles

cvar_t	r_particles = {"r_particles","1"};

extern cvar_t neh_rsmoke;
extern byte  *targa_rgba;

void R_Smoke (vec3_t org, int color) {;} // Removed

void CreateParticleTexture(void)
{
	int     x,y,d;
	float   dx, dy, dz;
	byte    data[64][64][4]; // Was 64
    
	// We are now creating the default particle texture.
	// Arn't you lucky?
	particle_tex = texture_extension_number++;
	glBindTexture(GL_TEXTURE_2D, particle_tex);
    
	for (x=0 ; x<64; x++)
	{
		for (y=0 ; y<64 ; y++)
		{
			data[y][x][0] = data[y][x][1] = data[y][x][2] = 255;
			dx = x - 16; dy = y - 16;
			d = (255 - (dx*dx+dy*dy));
			if (d < 0) d = 0;
				data[y][x][3] = (byte) d;
		}
	}
	glTexImage2D (GL_TEXTURE_2D, 0, 4, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void CreateBloodTexture(void)
{
	int     x,y,d;
	float   dx, dy, dz;
	byte    data[64][64][4];
    
	// We are now creating the default blood texture.
    
	for (x=0 ; x<34 ; x+=2)
	{
		for (y=0 ; y<34 ; y+=2)
			data[y][x][0] = data[y][x][1] = data[y][x][2] = (rand()%64)+64;
	}
	
	for (x=0 ; x<32 ; x+=2)
	{
		for (y=0 ; y<32 ; y+=2)
		{
			data[y][x+1][0] = data[y][x+1][1] = data[y][x+1][2] = (int) (data[y][x][0] + data[y][x+2][0]) >> 1;
			data[y+1][x][0] = data[y+1][x][1] = data[y+1][x][2] = (int) (data[y][x][0] + data[y+2][x][0]) >> 1;
			data[y+1][x+1][0] = data[y+1][x+1][1] = data[y+1][x+1][2] = (int) (data[y][x][0] + data[y][x+2][0] + data[y+2][x][0] + data[y+2][x+2][0]) >> 2;
		}
	}
	    
	for (x=0 ; x<64 ; x++)
	{
		for (y=0 ; y<64 ; y++)
		{
			data[y][x][1] = data[y][x][2] = 0;
			dx = x - 16; dy = y - 16;
			d = (255 - (dx*dx+dy*dy));
			if (d < 0) d = 0;
				data[y][x][3] = (byte) d;
		}
	}

	
	blood_tex = texture_extension_number++;
	glBindTexture(GL_TEXTURE_2D, blood_tex);
	glTexImage2D (GL_TEXTURE_2D, 0, 4, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void CreateFlareTexture(void)
{
	int     x,y,d;
	float   dx, dy, dz;
	byte    data[64][64][4];
    
	flare_tex = texture_extension_number++;
	glBindTexture(GL_TEXTURE_2D, flare_tex);
    
	for (x=0 ; x<64 ; x++)
	{
		for (y=0 ; y<64 ; y++)
		{
			data[y][x][0] = data[y][x][1] = data[y][x][2] = 255;
			dx = x - 16; dy = y - 16;
			d = 2048 / (dx*dx+dy*dy+1) - 32;
			d = bound(0, d, 255);
			data[y][x][3] = (byte) d;
		}
	}
    
	glTexImage2D (GL_TEXTURE_2D, 0, 4, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void CreateRainTexture(void)
{
	int     x,y,d;
	float   dx, dy, dz;
	byte    data[64][64][4];
    
	rain_tex = texture_extension_number++;
	glBindTexture(GL_TEXTURE_2D, rain_tex);
    
	for (x=0 ; x<64 ; x++)
	{
		for (y=0 ; y<64 ; y++)
		{
			data[y][x][0] = data[y][x][1] = data[y][x][2] = 255;
			
			if (y < 24) // Stretch the upper half
			{
				dx = (x - 16)*2;
				dy = (y - 24)*2/3;
				d = (255 - (dx*dx+dy*dy))/2;
			}
			else
			{
				dx = (x - 16)*2;
				dy = (y - 24)*2;
				d = (255 - (dx*dx+dy*dy))/2;
			}
			
			if (d < 0) d = 0;
				data[y][x][3] = (byte) d;
		}
	}
    
	glTexImage2D (GL_TEXTURE_2D, 0, 4, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void CreateBubbleTexture(void)
{
	int     x,y,d = 0;
	float   dx, dy, dz, dot, f;
	byte    data[64][64][4];
	vec3_t  normal, light;
    
	bubble_tex = texture_extension_number++;
	glBindTexture(GL_TEXTURE_2D, bubble_tex);
    
	light[0] = 1;light[1] = 1;light[2] = 1;
	VectorNormalize(light);
	for (x=0 ; x<64 ; x++)
	{
		for (y=0 ; y<64 ; y++)
		{
			data[y][x][0] = data[y][x][1] = data[y][x][2] = 255;
			dx = x * (1.0 / 16.0) - 1.0;
			dy = y * (1.0 / 16.0) - 1.0;
			if (dx*dx+dy*dy < 1) // Sphere contact
			{
				dz = 1 - (dx*dx+dy*dy);
				f = 0;
				
				// Rump. Not ramp. rump. :)
				normal[0] = dx;normal[1] = dy;normal[2] = dz;
				VectorNormalize(normal);
				dot = DotProduct(normal, light);
				if (dot > 0.5) // interior reflection
					f += ((dot *  2) - 1);
				else if (dot < -0.5) // exterior reflection
					f += ((dot * -2) - 1);
				
				// front side
				normal[0] = dx;normal[1] = dy;normal[2] = -dz;
				VectorNormalize(normal);
				dot = DotProduct(normal, light);
				if (dot > 0.5) // interior reflection
					f += ((dot *  2) - 1);
				else if (dot < -0.5) // exterior reflection
					f += ((dot * -2) - 1);
				
				f *= 255;
				f = bound(0, f, 255);
				data[y][x][3] = (byte) d;
			}
			else
			{
				data[y][x][3] = 0;
			}
		}
	}
    
	glTexImage2D (GL_TEXTURE_2D, 0, 4, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void LoadSmokeTexture (void)
{
	int  width, height;
	byte *data;
	char name[MAX_OSPATH];

	sprintf (name, "sprites/smoke2");
	data = R_LoadImage (name, &width, &height, true);

	if (!data)
	{
		Con_Printf ("Couldn't load %s\n", name);
		width = height = 64;
		data = malloc (width * height * 4);
		memset (data, 0, width * height * 4); // Seems OK, kludge
	}

	smoke_tex = texture_extension_number++; 
	glBindTexture(GL_TEXTURE_2D, smoke_tex);
	glTexImage2D (GL_TEXTURE_2D, 0, gl_alpha_format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	free (data);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

// Create and Bind particle textures
void R_InitParticleTexture (void)
{
	CreateParticleTexture(); // Create the default particle texture
	CreateBloodTexture();    // Create the default blood texture
	CreateFlareTexture();    // Create the flare texture
	CreateBubbleTexture();   // Yeah, yeah..
	LoadSmokeTexture();      // Hehe, now for something completely different..
}

/*
===============
R_InitParticles
===============
*/
// Setup and allocate the whole particle stuff
void R_InitParticles (void)
{
	int		i;

	i = COM_CheckParm ("-particles");

	if (i)
	{
		r_numparticles = (int)(Q_atoi(com_argv[i+1]));
		if (r_numparticles < ABSOLUTE_MIN_PARTICLES)
			r_numparticles = ABSOLUTE_MIN_PARTICLES;
		else if (r_numparticles > ABSOLUTE_MAX_PARTICLES)
			r_numparticles = ABSOLUTE_MAX_PARTICLES;
	}
	else
	{
		r_numparticles = MAX_PARTICLES;
	}

	particles = (particle_t *)
			Hunk_AllocName (r_numparticles * sizeof(particle_t), "particles");

	R_InitParticleTexture ();

	Cvar_RegisterVariable (&r_particles);
}

/*
===============
NextFreeParticle
===============
*/
static particle_t *NextFreeParticle (void)
{
	particle_t *p = free_particles;

	if (p)
	{
//		if (p < particles || p >= particles + r_numparticles)
//			Sys_Error ("NextFreeParticle: free_particles index %d is outside particles array max %d", (p - particles) / sizeof(particle_t), r_numparticles - 1);
	    
//		if (active_particles && (active_particles < particles || active_particles >= particles + r_numparticles))
//			Sys_Error ("NextFreeParticle: active_particles index %d is outside particles array max %d", (active_particles - particles) / sizeof(particle_t), r_numparticles - 1);
	    
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
	}
	
	return p;
}

/*
===============
R_EntityParticles
===============
*/

#define NUMVERTEXNORMALS	162

extern float	r_avertexnormals[NUMVERTEXNORMALS][3];
vec3_t		avelocities[NUMVERTEXNORMALS];
float		beamlength = 16;
vec3_t		avelocity = {23, 7, 3};
float		partstep = 0.01;
float		timescale = 0.01;

void R_EntityParticles (entity_t *ent)
{
	int			count;
	int			i;
	particle_t	*p;
	float		angle;
	float		sr, sp, sy, cr, cp, cy;
	vec3_t		forward;
	float		dist;
	
	dist = 64;
	count = 50;

	if (!avelocities[0][0])
	{
		for (i=0 ; i<NUMVERTEXNORMALS*3 ; i++)
			avelocities[0][i] = (rand()&255) * 0.01;
	}


	for (i=0 ; i<NUMVERTEXNORMALS ; i++)
	{
		angle = cl.time * avelocities[i][0];
		sy = sin(angle);
		cy = cos(angle);
		angle = cl.time * avelocities[i][1];
		sp = sin(angle);
		cp = cos(angle);
		angle = cl.time * avelocities[i][2];
		sr = sin(angle);
		cr = cos(angle);
	
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;

		p = NextFreeParticle ();
		if (!p)
			return;

                p->texnum = flare_tex;
		p->scale = 2;
		p->alpha = 255;
		p->die = cl.time + 0.01;
		p->color = 0x6f;
		p->type = pt_explode;
		
		p->org[0] = ent->origin[0] + r_avertexnormals[i][0]*dist + forward[0]*beamlength;			
		p->org[1] = ent->origin[1] + r_avertexnormals[i][1]*dist + forward[1]*beamlength;			
		p->org[2] = ent->origin[2] + r_avertexnormals[i][2]*dist + forward[2]*beamlength;			
	}
}


/*
===============
R_ClearParticles
===============
*/
void R_ClearParticles (void)
{
	int		i;
	
	free_particles = &particles[0];
	active_particles = NULL;

	for (i=0 ;i<r_numparticles ; i++)
		particles[i].next = &particles[i+1];
	particles[r_numparticles-1].next = NULL;
}


void R_ReadPointFile_f (void)
{
	FILE	*f;
	vec3_t	org;
	int		r;
	int		c;
	particle_t	*p;
	char	name[MAX_OSPATH];
	
	sprintf (name,"maps/%s.pts", sv.name);

	COM_FOpenFile (name, &f);
	if (!f)
	{
		Con_Printf ("couldn't open %s\n", name);
		return;
	}
	
	Con_Printf ("Reading %s...\n", name);
	c = 0;
	for ( ;; )
	{
		r = fscanf (f,"%f %f %f\n", &org[0], &org[1], &org[2]);
		if (r != 3)
			break;
		c++;
		
		p = NextFreeParticle ();
		if (!p)
		{
			Con_Printf ("Not enough free particles\n");
			break;
		}
		
                p->texnum = particle_tex;
		p->scale = 2;
		p->alpha = 255;
		p->die = 99999;
		p->color = (-c)&15;
		p->type = pt_static;
		VectorCopy (vec3_origin, p->vel);
		VectorCopy (org, p->org);
	}

	fclose (f);
	Con_Printf ("%i points read\n", c);
}

/*
===============
R_ParseParticleEffect

Parse an effect out of the server message
===============
*/
void R_ParseParticleEffect (void)
{
	vec3_t		org, dir;
	int			i, count, msgcount, color;
	
	for (i=0 ; i<3 ; i++)
		org[i] = MSG_ReadCoord ();
	for (i=0 ; i<3 ; i++)
		dir[i] = MSG_ReadChar () * (1.0/16);
	msgcount = MSG_ReadByte ();
	color = MSG_ReadByte ();

	if (msgcount == 255)
		count = 1024;
	else
		count = msgcount;
	
	R_RunParticleEffect (org, dir, color, count);
}
	
/*
===============
R_ParticleExplosion

===============
*/
void R_ParticleExplosion (vec3_t org, int smoke)
{
	int			i, j, k;
	particle_t	*p;
	
//      for (i=0 ; i<1024 ; i++)
        for (i=0 ; i<512 ; i++)
	{
		p = NextFreeParticle ();
		if (!p)
			return;

                p->texnum = flare_tex;
                p->scale = 4+(rand()&7); // 4 was 12
		p->alpha = 192 + (rand()&63);
		p->die = cl.time + 5;

		p->color = ramp1[rand()&3];
		p->type = pt_fallfadespark;
		for (j=0 ; j<3 ; j++)
                {
//                        p->org[j] = org[j] + ((rand()&31)-16);
                          p->org[j] = org[j] + ((rand()&63)-32);
			p->vel[j] = (rand()&511)-256;
		}
		p->vel[j] += 120;
	}

	if (smoke)
	{
		for (i=0 ; i<32 ; i++)
		{
			p = NextFreeParticle ();
			if (!p)
				return;

                        p->texnum = smoke_tex;
			p->scale = 24;
			p->alpha = 255; // * r_smokealpha.value;
			p->die = cl.time + 2;
			p->type = pt_smoke;
			p->color = (rand()&7) + 8;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()%48)-24);
				p->vel[j] = (rand()&63)-32;
			}
		}
	}
}

/*
===============
R_ParticleExplosion2

===============
*/
void R_ParticleExplosion2 (vec3_t org, int colorStart, int colorLength)
{
	int			i, j;
	particle_t	*p;
	int			colorMod = 0;

	for (i=0; i<512; i++)
	{
		p = NextFreeParticle ();
		if (!p)
			return;

                p->texnum = flare_tex;
		p->scale = 8;
		p->alpha = 255;
		p->die = cl.time + 0.3;
		p->color = colorStart + (colorMod % colorLength);
		colorMod++;

		p->type = pt_blob;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((rand()%32)-16);
			p->vel[j] = (rand()%512)-256;
		}
	}
}

/*
===============
R_BlobExplosion

===============
*/
void R_BlobExplosion (vec3_t org)
{
	int			i, j;
	particle_t	*p;
	
	for (i=0 ; i<1024 ; i++)
	{
		p = NextFreeParticle ();
		if (!p)
			return;

                p->texnum = flare_tex;
		p->scale = 8;
		p->alpha = 255;
		p->die = cl.time + 1 + (rand()&8)*0.05;

		if (i & 1)
		{
			p->type = pt_blob;
			p->color = 66 + rand()%6;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()%32)-16);
				p->vel[j] = (rand()%512)-256;
			}
		}
		else
		{
			p->type = pt_blob2;
			p->color = 150 + rand()%6;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()%32)-16);
				p->vel[j] = (rand()%512)-256;
			}
		}
	}
}

/*
===============
R_RunParticleEffect

===============
*/
void R_RunParticleEffect (vec3_t org, vec3_t dir, int color, int count)
{
	int			i, j;
	particle_t	*p;

	for (i=0 ; i<count ; i++)
	{
		p = NextFreeParticle ();
		if (!p)
			return;

		if (count == 1024)
		{	// rocket explosion
                        p->texnum = flare_tex;
			p->scale = 8;
			p->alpha = 255;
			p->die = cl.time + 5;
			p->color = ramp1[0];
			p->ramp = rand()&3;
			if (i & 1)
			{
				p->type = pt_explode;
				for (j=0 ; j<3 ; j++)
				{
					p->org[j] = org[j] + ((rand()%32)-16);
					p->vel[j] = (rand()%512)-256;
				}
			}
			else
			{
				p->type = pt_explode2;
				for (j=0 ; j<3 ; j++)
				{
					p->org[j] = org[j] + ((rand()%32)-16);
					p->vel[j] = (rand()%512)-256;
				}
			}
		}
		else
		{
                        p->texnum = flare_tex;
			p->scale = 8;
			p->alpha = 255;
			p->die = cl.time + 0.1*(rand()%5);
			p->color = (color&~7) + (rand()&7);
			p->type = pt_static; //slowgrav;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()&15)-8);
				p->vel[j] = dir[j]*15;// + (rand()%300)-150;
			}
		}
	}
}

// LordHavoc: added this for spawning sparks/dust (which have strong gravity)
/*
===============
R_SparkShower

===============
*/
void R_SparkShower (vec3_t org, vec3_t dir, int count, int type)
{
	int			i, j;
	particle_t	*p;

	p = NextFreeParticle ();
	if (!p)
		return;

	if (type == 0) // sparks
	{
                p->texnum = smoke_tex;
		p->scale = 20;
		p->alpha = 255;
		p->color = (rand()&3)+12;
		p->type = pt_bulletpuff;
		p->die = cl.time + 1;
		VectorCopy(org, p->org);
		p->vel[0] = p->vel[1] = p->vel[2] = 0;
	}
	else // blood
	{
                p->texnum = blood_tex;
		p->scale = 24;
		p->alpha = 255;
		p->color = (rand()&3)+12;
		p->type = pt_bloodcloud;
		p->die = cl.time + 0.5;
		VectorCopy(org, p->org);
		p->vel[0] = p->vel[1] = p->vel[2] = 0;
		return;
	}
	for (i=0 ; i<count ; i++)
	{
		p = NextFreeParticle ();
		if (!p)
			return;

                p->texnum = flare_tex;
		p->scale = 5;
		p->alpha = 255;
		p->die = cl.time + 0.0625 * (rand()&15);
                p->type = pt_dust;
                p->ramp = (rand()&3);
                p->color = ramp1[(int)p->ramp];
                for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((rand()&7)-4);
			p->vel[j] = dir[j] + (rand()%192)-96;
                }
        }
}

void R_BloodShower (vec3_t mins, vec3_t maxs, float velspeed, int count)
{
	int			i, j;
	particle_t	*p;
	vec3_t		diff;
	vec3_t		center;
	vec3_t		velscale;

	VectorSubtract(maxs, mins, diff);
	center[0] = (mins[0] + maxs[0]) * 0.5;
	center[1] = (mins[1] + maxs[1]) * 0.5;
	center[2] = (mins[2] + maxs[2]) * 0.5;
	velscale[0] = velspeed * 2.0 / diff[0];
	velscale[1] = velspeed * 2.0 / diff[1];
	velscale[2] = velspeed * 2.0 / diff[2];
	
	for (i=0 ; i<count ; i++)
	{
		p = NextFreeParticle ();
		if (!p)
			return;

                p->texnum = blood_tex;
		p->scale = 24;
		p->alpha = 128 + (rand()&127);
		p->die = cl.time + 2; //0.015625 * (rand()%128);
		p->type = pt_fadespark;
		p->color = (rand()&3)+12;
//		p->color = 67 + (rand()&3);
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = diff[j] * (float) (rand()%1024) * (1.0 / 1024.0) + mins[j];
			p->vel[j] = (p->org[j] - center[j]) * velscale[j];
		}
	}
}

void R_ParticleCube (vec3_t mins, vec3_t maxs, vec3_t dir, int count, int colorbase, int gravity, int randomvel)
{
	int			i, j;
	particle_t	*p;
	vec3_t		diff;
	float		t;

	if (maxs[0] <= mins[0]) {t = mins[0];mins[0] = maxs[0];maxs[0] = t;}
	if (maxs[1] <= mins[1]) {t = mins[1];mins[1] = maxs[1];maxs[1] = t;}
	if (maxs[2] <= mins[2]) {t = mins[2];mins[2] = maxs[2];maxs[2] = t;}

	VectorSubtract(maxs, mins, diff);
	
	for (i=0 ; i<count ; i++)
	{
		p = NextFreeParticle ();
		if (!p)
			return;

                p->texnum = flare_tex;
		p->scale = 12;
		p->alpha = 255;
		p->die = cl.time + 1 + (rand()&15)*0.0625;
		if (gravity)
			p->type = pt_grav;
		else
			p->type = pt_static;
		p->color = colorbase + (rand()&3);
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = diff[j] * (float) (rand()&1023) * (1.0 / 1024.0) + mins[j];
			if (randomvel)
				p->vel[j] = dir[j] + (rand()%randomvel)-(randomvel*0.5);
			else
				p->vel[j] = 0;
		}
	}
}

void R_ParticleRain (vec3_t mins, vec3_t maxs, vec3_t dir, int count, int colorbase, int type)
{
	int			i, j;
	particle_t	*p;
	vec3_t		diff;
	vec3_t		org;
	vec3_t		adjust;
	vec3_t		vel;
	float		t, z;

	if (maxs[0] <= mins[0]) {t = mins[0];mins[0] = maxs[0];maxs[0] = t;}
	if (maxs[1] <= mins[1]) {t = mins[1];mins[1] = maxs[1];maxs[1] = t;}
	if (maxs[2] <= mins[2]) {t = mins[2];mins[2] = maxs[2];maxs[2] = t;}
	if (dir[2] < 0) // falling
	{
		t = (maxs[2] - mins[2]) / -dir[2];
		z = maxs[2];
	}
	else // rising??
	{
		t = (maxs[2] - mins[2]) / dir[2];
		z = mins[2];
	}
	if (t < 0 || t > 2) // sanity check
		t = 2;
	t += cl.time;

	VectorSubtract(maxs, mins, diff);
	
	for (i=0 ; i<count ; i++)
	{
		p = NextFreeParticle ();
		if (!p)
			return;

		vel[0] = dir[0] + (rand()&31) - 16;
		vel[1] = dir[1] + (rand()&31) - 16;
		vel[2] = dir[2] + (rand()&63) - 32;
		org[0] = diff[0] * (float) (rand()&1023) * (1.0 / 1024.0) + mins[0];
		org[1] = diff[1] * (float) (rand()&1023) * (1.0 / 1024.0) + mins[1];
		org[2] = z;

		p->scale = 6;
		p->alpha = 255;
		p->die = t;
		if (type == 1)
		{
                        p->texnum = particle_tex;
			p->type = pt_snow;
		}
		else // 0
		{
                        p->texnum = rain_tex;
			p->type = pt_static;
		}
		p->color = colorbase + (rand()&3);
		VectorCopy(org, p->org);
		VectorCopy(vel, p->vel);
	}
}


/*
===============
R_LavaSplash

===============
*/
void R_LavaSplash (vec3_t org)
{
	int			i, j, k;
	particle_t	*p;
	float		vel;
	vec3_t		dir;

	for (i=-16 ; i<16 ; i+=2)
		for (j=-16 ; j<16 ; j+=2)
			for (k=0 ; k<1 ; k++)
			{
				p = NextFreeParticle ();
				if (!p)
					return;
		
                                p->texnum = flare_tex;
				p->scale = 24;
				p->alpha = 255;
				p->die = cl.time + 2 + (rand()&31) * 0.02;
				p->color = 224 + (rand()&7);
				p->type = pt_slowgrav;
				
				dir[0] = j*8 + (rand()&7);
				dir[1] = i*8 + (rand()&7);
				dir[2] = 256;
	
				p->org[0] = org[0] + dir[0];
				p->org[1] = org[1] + dir[1];
				p->org[2] = org[2] + (rand()&63);
	
				VectorNormalize (dir);						
				vel = 50 + (rand()&63);
				VectorScale (dir, vel, p->vel);
			}
}

/*
===============
R_TeleportSplash

===============
*/
void R_TeleportSplash (vec3_t org)
{
	int			i, j, k, a;
	particle_t	*p;
	float		vel;
	vec3_t		dir;

	for (i=-24 ; i<24 ; i+=8)
		for (j=-24 ; j<24 ; j+=8)
			for (k=-24 ; k<32 ; k+=8)
			{
				p = NextFreeParticle ();
				if (!p)
					return;
		
                                p->texnum = flare_tex;
                                p->scale = 8;
				a = 1 + rand()&7;
				p->alpha = a * 32;
				p->die = cl.time + a * 0.125;
				p->color = 254; //8 + (rand()&7);
				p->type = pt_fadespark;
				
				p->org[0] = org[0] + i + (rand()&7);
				p->org[1] = org[1] + j + (rand()&7);
				p->org[2] = org[2] + k + (rand()&7);
	
				p->vel[0] = i*2 + (rand()%25) - 12;
				p->vel[1] = j*2 + (rand()%25) - 12;
				p->vel[2] = k*2 + (rand()%25) - 12 + 40;
			}
}

#if 1 // Why was this disabled before?
void R_RocketTrail (vec3_t start, vec3_t end, int type, entity_t *ent)
{
	vec3_t		vec;
	float		len;
	int		j, contents, bubbles;
	particle_t	*p;
	int		dec;
	static int	tracercount;

	if (!gl_rsmoke.value)
		return;

	contents = Mod_PointInLeaf(start, cl.worldmodel)->contents;
	//if (contents == CONTENTS_SKY || contents == CONTENTS_LAVA)
	//	return;

	VectorSubtract (end, start, vec);
        len = VectorNormalize(vec);
	if (type < 128)
		dec = 3;
	else
	{
		dec = 1;
		type -= 128;
	}
	bubbles = (contents == CONTENTS_WATER || contents == CONTENTS_SLIME);

	while (len > 0)
	{
		p = NextFreeParticle ();
		if (!p)
			return;
		
		p->vel[0] = p->vel[1] = p->vel[2] = 0;
		p->die = cl.time + 2;

		switch (type)
		{
			case 0:	// rocket trail
			case 1: // grenade trail
				if (bubbles)
				{
					dec = 4+(rand()&7);
                                        p->texnum = bubble_tex;
					p->scale = 6+(rand()&3);
					p->alpha = 255;
/*                                        if (neh_rsmoke.value)
                                                p->color = neh_rsmoke.value;
					else*/
						p->color = (rand()&3)+12;
					p->type = pt_bubble;
					p->die = cl.time + 2;
					for (j=0 ; j<3 ; j++)
					{
						p->vel[j] = (rand()&31)-16;
						p->org[j] = start[j] + ((rand()&3)-2);
					}
				}
				else
				{
					dec = 16; // sparse trail
                                        p->texnum = smoke_tex;
					p->scale = 12+(rand()&7);
					p->alpha = 192 + (rand()&63);
/*                                        if (neh_rsmoke.value)
                                                p->color = neh_rsmoke.value;
					else*/
						p->color = (rand()&3)+12;
					p->type = pt_smoke;
					p->die = cl.time + 2;
					VectorCopy(start, p->org);
				}
				break;


			case 2:	// blood
				dec = 16; // sparse trail
                                p->texnum = blood_tex;
				p->scale = 12+(rand()&7);
				p->alpha = 255;
//                                p->color = (rand()&3)+12;
                                p->color = 67 + (rand()&3);
				p->type = pt_bloodcloud;
				p->die = cl.time + 2;
				for (j=0 ; j<3 ; j++)
				{
					p->vel[j] = (rand()&15)-8;
					p->org[j] = start[j] + ((rand()&3)-2);
				}
				break;

			case 3:
			case 5:	// tracer
				dec = 6; // LordHavoc: sparser trail
                                p->texnum = flare_tex;
				p->scale = 6;
				p->alpha = 255;
				p->die = cl.time + 0.2; //5;
				p->type = pt_static;
				if (type == 3)
					p->color = 52 + ((tracercount&4)<<1);
				else
					p->color = 230 + ((tracercount&4)<<1);

				tracercount++;

				VectorCopy (start, p->org);
				if (tracercount & 1)
				{
					p->vel[0] = 30*vec[1];
					p->vel[1] = 30*-vec[0];
				}
				else
				{
					p->vel[0] = 30*-vec[1];
					p->vel[1] = 30*vec[0];
				}
				break;

			case 4:	// slight blood
				dec = 16; // sparse trail
                                p->texnum = blood_tex;
				p->scale = 12+(rand()&7);
				p->alpha = 255;
				p->color = (rand()&3)+12;
				p->type = pt_fadespark2;
				p->die = cl.time + 2;
				for (j=0 ; j<3 ; j++)
				{
					p->vel[j] = (rand()&15)-8;
					p->org[j] = start[j] + ((rand()&3)-2);
				}
				break;

			case 6:	// voor trail
				dec = 16; // sparse trail
                                p->texnum = flare_tex;
				p->scale = 12+(rand()&7);
				p->alpha = 255;
				p->color = 9*16 + 8 + (rand()&3);
				p->type = pt_fadespark2;
				p->die = cl.time + 2;
				for (j=0 ; j<3 ; j++)
				{
					p->vel[j] = (rand()&15)-8;
					p->org[j] = start[j] + ((rand()&3)-2);
				}
				break;

			case 7:	// Nehahra smoke tracer
				dec = 1000000; // only one puff
                                p->texnum = smoke_tex;
				p->scale = 12+(rand()&7);
				p->alpha = 255; //96;
				p->color = (rand()&3)+12;
				p->type = pt_smoke;
				p->die = cl.time + 1;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()&3)-2);
				break;
		}
		
		VectorMA (start, dec, vec, start);
		len -= dec;
	}
}

void R_RocketTrail2 (vec3_t start, vec3_t end, int color)
{
	vec3_t		vec;
	float		len;
	int			j;
	particle_t	*p;
	int			dec;
	static int	tracercount;


	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);
	while (len > 0)
	{
		len -= 3;

		p = NextFreeParticle ();
		if (!p)
			return;
		
		VectorCopy (vec3_origin, p->vel);

                p->texnum = flare_tex;
		p->scale = 16;
		p->alpha = 192;
		p->color = color;
		p->type = pt_smoke;
		p->die = cl.time + 1;
		VectorCopy(start, p->org);
//		for (j=0 ; j<3 ; j++)
//			p->org[j] = start[j] + ((rand()&15)-8);

		VectorAdd (start, vec, start);
	}
}

#else
void R_RocketTrail (vec3_t start, vec3_t end, int type, entity_t *ent)
{
	vec3_t		vec;
	float		len, dec, t, nt, speed;
	int			j, contents, bubbles;
	particle_t	*p;
	static int	tracercount;
//	if (!r_particles.value) return; // LordHavoc: particles are optional

	t = cl.oldtime;
	nt = cl.time;
	if (ent->trail_leftover < 0)
		ent->trail_leftover = 0;
	t += ent->trail_leftover;
	ent->trail_leftover -= (cl.time - cl.oldtime);
	if (t >= cl.time)
		return;

	contents = Mod_PointInLeaf(start, cl.worldmodel)->contents;
	if (contents == CONTENTS_SKY || contents == CONTENTS_LAVA)
		return;

	VectorSubtract (end, start, vec);
        len = VectorNormalize (vec);
	if (len <= 0.01f)
		return;
	speed = len / (nt - t);

	bubbles = (contents == CONTENTS_WATER || contents == CONTENTS_SLIME);

	while (t < nt)
	{
		p = NextFreeParticle ();
		if (!p)
			return;
		
		p->vel[0] = p->vel[1] = p->vel[2] = 0;
		p->die = cl.time + 2;

		switch (type)
		{
			case 0:	// rocket trail
			case 1: // grenade trail
				if (bubbles)
				{
					dec = 0.01f;
                                        p->texnum = bubble_tex;
					p->scale = 6+(rand()&3);
					p->alpha = 255;
                                        p->color = (rand()&3)+12;
					p->type = pt_bubble;
					p->die = cl.time + 2;
					for (j=0 ; j<3 ; j++)
					{
						p->vel[j] = (rand()&31)-16;
						p->org[j] = start[j] + ((rand()&3)-2);
					}
				}
				else
				{
					dec = 0.03f;
                                        p->texnum = smoke_tex;
					p->scale = 12+(rand()&7);
					p->alpha = 64 + (rand()&31);
                                        p->color = (rand()&3)+12;
					p->type = pt_smoke;
					p->die = cl.time + 2;
					VectorCopy(start, p->org);
				}
				break;


			case 2:	// blood
				dec = 0.03f;
                                p->texnum = blood_tex;
				p->scale = 20+(rand()&7);
				p->alpha = 255;
				p->color = (rand()&3)+68;
				p->type = pt_bloodcloud;
				p->die = cl.time + 2;
				for (j=0 ; j<3 ; j++)
				{
					p->vel[j] = (rand()&15)-8;
					p->org[j] = start[j] + ((rand()&3)-2);
				}
				break;

			case 3:
			case 5:	// tracer
				dec = 0.01f;
                                p->texnum = flare_tex;
				p->scale = 4;
				p->alpha = 255;
				p->die = cl.time + 0.2; //5;
				p->type = pt_static;
				if (type == 3)
					p->color = 52 + ((tracercount&4)<<1);
				else
					p->color = 230 + ((tracercount&4)<<1);

				tracercount++;

				VectorCopy (start, p->org);
				if (tracercount & 1)
				{
					p->vel[0] = 30*vec[1];
					p->vel[1] = 30*-vec[0];
				}
				else
				{
					p->vel[0] = 30*-vec[1];
					p->vel[1] = 30*vec[0];
				}
				break;

			case 4:	// slight blood
				dec = 0.03f; // sparse trail
                                p->texnum = blood_tex;
				p->scale = 20+(rand()&7);
				p->alpha = 255;
				p->color = (rand()&3)+68;
				p->type = pt_fadespark2;
				p->die = cl.time + 2;
				for (j=0 ; j<3 ; j++)
				{
					p->vel[j] = (rand()&15)-8;
					p->org[j] = start[j] + ((rand()&3)-2);
				}
				break;

			case 6:	// voor trail
				dec = 0.05f; // sparse trail
                                p->texnum = flare_tex;
				p->scale = 20+(rand()&7);
				p->alpha = 255;
				p->color = 9*16 + 8 + (rand()&3);
				p->type = pt_fadespark2;
				p->die = cl.time + 2;
				for (j=0 ; j<3 ; j++)
				{
					p->vel[j] = (rand()&15)-8;
					p->org[j] = start[j] + ((rand()&3)-2);
				}
				break;

			case 7:	// Nehahra smoke tracer
				dec = 0.14f;
                                p->texnum = smoke_tex;
				p->scale = 12+(rand()&7);
				p->alpha = 64;
				p->color = (rand()&3)+12;
				p->type = pt_smoke;
				p->die = cl.time + 1;
				for (j=0 ; j<3 ; j++)
					p->org[j] = start[j] + ((rand()&3)-2);
				break;
		}
		
		t += dec;
		dec *= speed;
		VectorMA (start, dec, vec, start);
	}
	ent->trail_leftover = t - cl.time;
}

void R_RocketTrail2 (vec3_t start, vec3_t end, int color, entity_t *ent)
{
	vec3_t		vec;
	float		len;
	particle_t	*p;
	static int	tracercount;

	VectorSubtract (end, start, vec);
        len = VectorNormalize (vec);
	while (len > 0)
	{
		len -= 3;

		p = NextFreeParticle ();
		if (!p)
			return;
		
		VectorCopy (vec3_origin, p->vel);

                p->texnum = flare_tex;
		p->scale = 16;
		p->alpha = 192;
		p->color = color;
		p->type = pt_smoke;
		p->die = cl.time + 1;
		VectorCopy(start, p->org);

		VectorAdd (start, vec, start);
	}
}
#endif

/*
===============
R_DrawParticles
===============
*/
extern	cvar_t	sv_gravity;

void R_DrawParticles (void)
{
	particle_t	*p, *kill;
	int		i, texnum, j = 0;
	float		grav, grav1, time1, time2, time3, dvel, frametime, scale, scale2;
	byte		*color24;
	vec3_t		up, right, uprightangles, forward2, up2, right2;
        vec3_t		v;
	
	if (!r_particles.value)
		return;

	// LordHavoc: early out condition
	if (!active_particles)
		return;

        texnum = particle_tex;
        glBindTexture(GL_TEXTURE_2D, texnum);
	glEnable (GL_BLEND);

// Ender: FIXME! Assume we have a lame card that doesn't support per-pixel
//        alpha.. Speed problem here..

//        if (isG200 || isRagePro)
//                glEnable(GL_ALPHA_TEST);
//        else
                glDisable(GL_ALPHA_TEST);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE); // disable zbuffer updates
	glShadeModel(GL_FLAT);
	glBegin (GL_TRIANGLES);

	VectorScale (vup, 1.5, up);
	VectorScale (vright, 1.5, right);

	uprightangles[0] = 0;
	uprightangles[1] = r_refdef.viewangles[1];
	uprightangles[2] = 0;
	AngleVectors (uprightangles, forward2, right2, up2);

	frametime = cl.time - cl.oldtime;
	time3 = frametime * 15;
	time2 = frametime * 10; // 15;
	time1 = frametime * 5;
	grav = (grav1 = frametime * sv_gravity.value) * 0.05;
	dvel = 4*frametime;

	for ( ;; ) 
	{
		kill = active_particles;
		if (kill && kill->die < cl.time)
		{
			active_particles = kill->next;
			kill->next = free_particles;
			free_particles = kill;
			continue;
		}
		break;
	}

	for (p=active_particles ; p ; p=p->next)
	{
		for ( ;; )
		{
			kill = p->next;
			if (kill && kill->die < cl.time)
			{
				p->next = kill->next;
				kill->next = free_particles;
				free_particles = kill;
				continue;
			}
			break;
		}
		
		// Improve sound when many particles
		if (++j % 8192 == 0)
			S_ExtraUpdateTime ();

		// Ender: Speedup? :)
		VectorSubtract(p->org, r_refdef.vieworg, v);
		if (DotProduct(v, v) >= 1024.0f) // 16.0f
		{
			if (p->texnum != texnum)
			{
				texnum = p->texnum;
				glEnd();
				glBindTexture(GL_TEXTURE_2D, texnum);
				glBegin(GL_TRIANGLES);
			}

			if ((texnum == particle_tex || texnum == flare_tex) && p->scale == 2 && p->alpha == 255)
			{
				// pointfile or entity particles
				// hack a scale up to keep particles from disapearing
				scale = (p->org[0] - r_origin[0])*vpn[0] + (p->org[1] - r_origin[1])*vpn[1]
					+ (p->org[2] - r_origin[2])*vpn[2];
				if (scale < 20)
					scale = 1;
				else
					scale = 1 + scale * 0.004;
				
				glColor3ubv ((byte *)&d_8to24table[(int)p->color]);
				glTexCoord2f (0,0);
				glVertex3fv (p->org);
				glTexCoord2f (1,0);
				glVertex3f (p->org[0] + up[0]*scale, p->org[1] + up[1]*scale, p->org[2] + up[2]*scale);
				glTexCoord2f (0,1);
				glVertex3f (p->org[0] + right[0]*scale, p->org[1] + right[1]*scale, p->org[2] + right[2]*scale);
			}
			else
			{
				scale = p->scale * -0.25;scale2 = p->scale * 0.75;
				
				color24 = (byte *) &d_8to24table[(int)p->color];
				if (texnum == smoke_tex)
					glColor4ub(color24[0], color24[1], color24[2], (byte) (p->alpha*smokealpha));
				else
					glColor4ub(color24[0], color24[1], color24[2], (byte) p->alpha);
                
				if (texnum == rain_tex) // rain streak
				{
					glTexCoord2f (0,0);
					glVertex3f (p->org[0] + right2[0]*scale , p->org[1] + right2[1]*scale , p->org[2] + right2[2]*scale );
					glTexCoord2f (1,0);
					glVertex3f (p->org[0] + right2[0]*scale , p->org[1] + right2[1]*scale , p->org[2] + right2[2]*scale );
					glTexCoord2f (0,1);
					glVertex3f (p->org[0] + right2[0]*scale2, p->org[1] + right2[1]*scale2, p->org[2] + right2[2]*scale2);
				}
				else
				{
					glTexCoord2f (0,0);
					// LordHavoc: centered particle sprites
					glVertex3f (p->org[0] + up[0]*scale  + right[0]*scale , p->org[1] + up[1]*scale  + right[1]*scale , p->org[2] + up[2]*scale  + right[2]*scale );
					glTexCoord2f (1,0);
					glVertex3f (p->org[0] + up[0]*scale2 + right[0]*scale , p->org[1] + up[1]*scale2 + right[1]*scale , p->org[2] + up[2]*scale2 + right[2]*scale );
					glTexCoord2f (0,1);
					glVertex3f (p->org[0] + up[0]*scale  + right[0]*scale2, p->org[1] + up[1]*scale  + right[1]*scale2, p->org[2] + up[2]*scale  + right[2]*scale2);
				}
			}
		}

		p->org[0] += p->vel[0]*frametime;
		p->org[1] += p->vel[1]*frametime;
		p->org[2] += p->vel[2]*frametime;
		
		switch (p->type)
		{
		case pt_static:
			break;
		case pt_fire:
			p->ramp += time1;
			if (p->ramp >= 6)
				p->die = -1;
			else
				p->color = ramp3[(int)p->ramp];
			p->vel[2] += grav;
			break;

		case pt_explode:
			p->ramp += time2;
			if (p->ramp >=8)
				p->die = -1;
			else
				p->color = ramp1[(int)p->ramp];
			for (i=0 ; i<3 ; i++)
				p->vel[i] += p->vel[i]*dvel;
			break;

		case pt_explode2:
			p->ramp += time3;
			if (p->ramp >=8)
				p->die = -1;
			else
				p->color = ramp2[(int)p->ramp];
			for (i=0 ; i<3 ; i++)
				p->vel[i] -= p->vel[i]*frametime;
			break;

		case pt_blob:
			for (i=0 ; i<3 ; i++)
				p->vel[i] += p->vel[i]*dvel;
			p->vel[2] -= grav;
			break;

		case pt_blob2:
			for (i=0 ; i<2 ; i++)
				p->vel[i] -= p->vel[i]*dvel;
			p->vel[2] -= grav;
			break;

		case pt_grav:
			p->vel[2] -= grav1;
			break;
		case pt_slowgrav:
			p->vel[2] -= grav;
			break;
// LordHavoc: gunshot spark showers
		case pt_dust:
			p->ramp += time1;
			p->scale -= frametime * 4;
			if (p->ramp >= 8 || p->scale <= 0)
				p->die = -1;
			else
				p->color = ramp3[(int)p->ramp];
			p->vel[2] -= grav1;
			break;
// LordHavoc: for smoke trails
		case pt_smoke:
			p->scale += frametime * 16;
			p->alpha -= frametime * 384;
			p->vel[2] += grav;
			if (p->alpha < 1)
				p->die = -1;
			break;
		case pt_snow:
			if (cl.time > p->time2)
			{
				p->time2 = cl.time + 0.4;
				p->vel[0] = (rand()&63)-32;
				p->vel[1] = (rand()&63)-32;
			}
			break;
		case pt_bulletpuff:
			p->scale -= frametime * 64;
			p->alpha -= frametime * 1024;
			p->vel[2] -= grav;
			if (p->alpha < 1 || p->scale < 1)
				p->die = -1;
			break;
		case pt_bloodcloud:
			p->scale -= frametime * 48;
			p->alpha -= frametime * 512;
			p->vel[2] -= grav;
			if (p->alpha < 1 || p->scale < 1)
				p->die = -1;
			break;
		case pt_fadespark:
			p->alpha -= frametime * 256;
			p->vel[2] -= grav;
			if (p->alpha < 1)
				p->die = -1;
			break;
		case pt_fadespark2:
			p->alpha -= frametime * 512;
			p->vel[2] -= grav;
			if (p->alpha < 1)
				p->die = -1;
			break;
		case pt_fallfadespark:
			p->alpha -= frametime * 256;
			p->vel[2] -= grav1;
			if (p->alpha < 1)
				p->die = -1;
			break;
		case pt_fallfadespark2:
			p->alpha -= frametime * 512;
			p->vel[2] -= grav1;
			if (p->alpha < 1)
				p->die = -1;
			break;
		case pt_bubble:
			p->vel[2] += grav1;
			if (p->vel[2] >= 100)
				p->vel[2] = 68+rand()&31;
			if (cl.time > p->time2)
			{
				p->time2 = cl.time + (rand()&7)*0.0625;
				p->vel[0] = (rand()&63)-32;
				p->vel[1] = (rand()&63)-32;
			}
			p->alpha -= frametime * 32;
			if (p->alpha < 1)
				p->die = -1;
			break;
		}
	}

	glEnd ();
	glShadeModel(GL_SMOOTH);
        glDepthMask(GL_TRUE); // enable zbuffer updates
	glDisable (GL_BLEND);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glColor3f(1,1,1);
}


