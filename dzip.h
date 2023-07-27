#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#ifdef WIN32
#include <windows.h>
#endif

#ifndef NO_ZLIB
#ifdef WIN32
#include "zlib/zlib.h"
#else
#include "zlib.h"
#endif
#endif

void Sys_Error (char *error, ...);
void COM_StripExtension (char *in, char *out);
char *COM_FileExtension (char *in);
void COM_DefaultExtension (char *path, char *extension);
void Con_Print (char *txt);
void Con_Printf (char *fmt, ...);

typedef char n_char;
#define char unsigned char

#define MAX_ENT 1024
#define MAJOR_VERSION 2
#define MINOR_VERSION 1
#define INITCRC 0xffffffff

enum { TYPE_NORMAL, TYPE_DEMV1, TYPE_TXT, TYPE_PAK, TYPE_DZ, TYPE_DEM,
	TYPE_NEHAHRA, TYPE_LAST };

enum  { SW_LIST, SW_EXTRACT, SW_VERIFY, SW_DEBUG, 
	SW_FORCE, SW_CRC, SW_LOSSY, NUM_SWITCHES };

enum {
	DEM_bad, DEM_nop, DEM_disconnect, DEM_updatestat, DEM_version,
	DEM_setview, DEM_sound, DEM_time, DEM_print, DEM_stufftext,
	DEM_setangle, DEM_serverinfo, DEM_lightstyle, DEM_updatename,
	DEM_updatefrags, DEM_clientdata, DEM_stopsound, DEM_updatecolors,
	DEM_particle, DEM_damage, DEM_spawnstatic, DEM_spawnbinary,
	DEM_spawnbaseline, DEM_temp_entity, DEM_setpause, DEM_signonnum,
	DEM_centerprint, DEM_killedmonster, DEM_foundsecret,
	DEM_spawnstaticsound, DEM_intermission, DEM_finale,
	DEM_cdtrack, DEM_sellscreen, DEM_cutscene, DZ_longtime,
/* nehahra */
	DEM_showlmp = 35, DEM_hidelmp, DEM_skybox, DZ_showlmp
};


typedef struct {
	char voz, pax;
	char ang0, ang1, ang2;
	char vel0, vel1, vel2;
	long items;
	char uk10, uk11, invbit;
	char wpf, av, wpm;
	int health;
	char am, sh, nl, rk, ce, wp;
	int force;
} cdata_t;

typedef struct { 
	int ptr;	/* start of file inside dz */
	int size;	/* v1: intermediate size. v2: compressed size */
	int real;	/* uncompressed size */
	short len;	/* length of name */
	short pak;
	unsigned long crc;
	int type;
	int date;
	int inter;	/* v2: intermediate size */
	char *name;
} direntry_t;

typedef struct {
	char modelindex, frame;
	char colormap, skin;
	char effects;
	char ang0, ang1, ang2;
	char new, present, active;
	char fullbright;	// nehahra
	int org0, org1, org2;
	int od0, od1, od2;
	int force;
	float alpha;		// nehahra
} ent_t;

typedef struct {
	char name[56];
	int ptr;
	int len;
} pakentry_t;

extern cdata_t oldcd, newcd;
extern ent_t base[MAX_ENT], oldent[MAX_ENT], newent[MAX_ENT];
extern direntry_t *directory;

int bplus (int, int);
void check_dzip_err (char *);
int check_filetype (int);
void compress_file (char *);
void copy_msg (int);
void create_clientdata_msg ();
void crc_init ();
void dem_compress (char *, int);
void dem_copy_ue ();
int dem_uncompress ();
void demv1_dxentities ();
void dem_uncompress_init (int);
void discard_msg (int);
void end_zlib_compression ();
void *fsmalloc (size_t);
int get_filetype (char *);
int getfiledate (char *);
void init_zlib_compression ();
void insert_msg (void *, int);
void list_dzip_file (char *);
void make_crc (void *, int);
FILE *open_create (char *);
int OpenDZFile (FILE *, char *);
void read_direntry (FILE *,direntry_t *);
void read_file (FILE *, int *, int);
void read_input (FILE *, void *, int);
void safe_fwrite (void *, size_t, FILE *);
void setfiledate (char *, int);
void uncompress_file (FILE *, char *, char *);
void write_file (void *, int);
void write_output (void *, int);

#ifdef DEBUG
void dem_debug (char *);
#endif

#define malloc fsmalloc

extern char dem_updateframe;
extern char copybaseline;
extern int maxent, lastent, sble;
extern int maj_ver, min_ver;	// of the current dz file
extern int p_blocksize;
extern int numfiles;
extern int intersize, totalsize;
extern int demotime;
extern long dzPakPos;
extern int entlink[];
extern long dem_gametime;
extern long outlen;
extern char *inblk, *outblk, *tmpblk;
extern char *inptr;
extern char flag[];
extern FILE *infile, *outfile;
extern unsigned long crcval;

#ifndef BIG_ENDIAN

#define getshort(x) (*(short*)(x))
#define getlong(x) (*(long*)(x))
#define getfloat(x) (*(float*)(x))
#define cnvlong(x) (x)

#else

short getshort (char *);
long getlong (char *);
float getfloat (char *);
#define cnvlong(x) getlong((char*)(&x))

#endif

#ifndef NO_ZLIB
#define Z_BUFFER_SIZE 16384
extern z_stream zip_stream;
extern char *zbuf;
extern int zip_err, ztotal;
extern int zlevel;
#endif

#ifdef GUI
void GuiProgressMsg (char *, ...);
void GuiProgress (int);
FILE *GuiGetUncompressOutfile (direntry_t *);
extern char GuiAbort;
#endif
