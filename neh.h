//#include "fmod/fmod.h"
#include <xmp.h>

#define NEH_VERSION        3.08

// System types
#define TYPE_GAME          1
#define TYPE_DEMO          0
#define TYPE_BOTH          2

extern int NehGameType, midasplaying;

// CVars
extern  cvar_t   r_waterripple, r_nowaterripple, gl_notrans, gl_rsmoke, nospr32;
extern  cvar_t   r_interpolate_model_animation, r_interpolate_model_transform, slowmo;
extern  cvar_t   r_interpolate_model_weapon;
extern  cvar_t   gl_fogenable, gl_fogdisable, gl_fogdensity, gl_fogred, gl_foggreen, gl_fogblue;
extern  cvar_t   gl_skyclip, gl_skyfix;
extern  cvar_t   gl_glows, tweak, modvolume, cutscene;
extern	qboolean gl_do_skyfix;

// Graphical Junk
extern  float   model_alpha;
extern  int     smoketexture, DoFullbright, num_sfx;

// Evaluation Shortcuts
extern int eval_gravity, eval_items2, eval_alpha, eval_fullbright;
extern int eval_idealpitch, eval_pitch_speed;

// Sound Stuff
#define	MAX_SFX	(2 * MAX_SOUNDS) //1024
extern sfx_t    *known_sfx;
extern int      num_sfx, num_sfxorig;



// Speed improvement :)
#define GETEDICTFIELDVALUE(ed, fieldoffset) (fieldoffset ? (eval_t *)((char *)&ed->v + pr_ChkEField ("GETEDICTFIELDVALUE", fieldoffset)) : NULL)
#define bound(min,num,max) (num >= min ? (num < max ? num : max) : min)
#define PlaneDist(point,plane) ((plane)->type < 3 ? (point)[(plane)->type] : DotProduct((point), (plane)->normal))
#define PlaneDiff(point,plane) ((plane)->type < 3 ? (point)[(plane)->type] - (plane)->dist : DotProduct((point), (plane)->normal) - (plane)->dist)

int MSG_ReadChar (void);
int MSG_ReadByte (void);
float MSG_ReadAngle (void);
float MSG_ReadCoord (void);

//#define MSG_ReadChar() (msg_readcount >= net_message.cursize ? (msg_badread = true, -1) : (signed char)net_message.data[msg_readcount++])
//#define MSG_ReadByte() (msg_readcount >= net_message.cursize ? (msg_badread = true, -1) : (unsigned char)net_message.data[msg_readcount++])
//#define MSG_ReadAngle() (MSG_ReadChar() * (360.0f / 256.0f))
//#define MSG_ReadCoord() (MSG_ReadShort() * 0.125f)


// Various function prototypes
void FindEdictFieldOffsets();
void MOD_stop (void);
void MOD_SetVolume_f (void);
void MOD_done (void);
void Neh_Init(void);
void Neh_BeginFrame(void);
void Neh_EndFrame(void);
void Neh_GameStart (void);
void SHOWLMP_drawall();
void SHOWLMP_clear();
void SHOWLMP_decodehide();
void SHOWLMP_decodeshow();
void R_SparkShower (vec3_t org, vec3_t dir, int count, int type);
void DzipInit (void);
void uncompress_file (FILE *, char *, char *);

int COM_FindOnly(char *filename);
float VectorNormalizeLength (vec3_t v);         // returns vector length

// Network Protocol Stuff
#define U_TRANS        (1<<15)
#define	svc_showlmp			35		// [string] slotname [string] lmpfilename [coord] x [coord] y
#define	svc_hidelmp			36		// [string] slotname
#define	svc_skybox			37		// [string] skyname

#define svc_skyboxsize                  50              // [coord] size (default is 4096)
#define svc_fog				51		// [byte] enable <optional past this point, only included if enable is true> [float] density [byte] red [byte] green [byte] blue


#define TE_EXPLOSION3           16
#define TE_LIGHTNING4           17
#define TE_SMOKE                18
#define TE_NEW1                 19
#define TE_NEW2                 20

#define EF_NODRAW                               16
#define EF_BLUE					64
#define EF_RED					128
