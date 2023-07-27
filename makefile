!include <win32.mak>
#!include <winopt.mak>
all:nehahra.exe
LIBS=dxguid.lib mgllt.lib winmm.lib wsock32.lib opengl32.lib glu32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comctl32.lib libc.lib zlib.lib fmod/fmod.lib
NCFLAGS=/nologo /W2 /Ob2gi /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "GLQUAKE" /c /I "include" /MT /G5As
PORTABLE_OBJS= \
 console.obj \
 keys.obj \
 cl_demo.obj \
 cl_input.obj \
 cl_main.obj \
 cl_parse.obj \
 cl_tent.obj \
 pr_cmds.obj \
 pr_edict.obj \
 pr_exec.obj \
 sv_main.obj \
 sv_move.obj \
 sv_phys.obj \
 sv_user.obj \
 snd_mem.obj \
 snd_mix.obj \
 snd_dma.obj \
 host.obj \
 host_cmd.obj \
 menu.obj \
 net_main.obj \
 net_vcr.obj \
 r_vars.obj \
 r_part.obj \
 chase.obj \
 cmd.obj \
 common.obj \
 crc.obj \
 cvar.obj \
 d_vars.obj \
 mathlib.obj \
 sbar.obj \
 view.obj \
 wad.obj \
 world.obj \
 zone.obj

WIN_OBJS = \
 neh.obj \
 snd_win.obj \
 net_loop.obj \
 net_win.obj \
 net_dgrm.obj \
 net_wins.obj \
 net_wipx.obj \
 net_wipx.obj \
 sys_win.obj \
 in_win.obj \
 cd_win.obj \
 conproc.obj \
 gl_rmain.obj \
 gl_rmisc.obj \
 gl_vidnt.obj \
 gl_draw.obj \
 gl_mesh.obj \
 gl_model.obj \
 gl_refrag.obj \
 gl_rlight.obj \
 gl_warp.obj \
 gl_screen.obj \
 gl_rsurf.obj \
 snd_mixa.obj \
 math.obj \
 worlda.obj \
 sys_x86.obj

DZIP_OBJS = \
 dzipcrc.obj \
 dzipdecode.obj \
 dzipun.obj \
 dzipmain.obj

OBJS=$(PORTABLE_OBJS) $(WIN_OBJS) $(DZIP_OBJS)
nehahra.exe:     $(OBJS)
        $(link) /nologo /subsystem:windows /incremental:no /machine:I386 -out:nehahra.exe *.obj $(LIBS) /nodefaultlib:libc
.c.obj:
    $(cc) $(NCFLAGS) -DGLQUAKE $*.c
