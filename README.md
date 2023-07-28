# NehQuake
Nehahra engine 3.08 that is meaned to play Nehahra partial conversion for Quake.

I cut off the FMOD dependency out of Nehahra 3.08 source and replace it with something modern, for example libxmp, but libxmp doesn't play the modules itself, only via the SDL.
And now Nehahra source is fully GPL compatible (libxmp is free and the SDL too).
I have stopped at libxmp >= 4.5 (to decode Nehahra .xm and .s3m music to PCM data) and SDL2 libraries (SDL2 is for playing the PCM data directly).
I also changed the protocol number to be different to Bengt's protocol so that HUD can display values more than 255 (up to 999, as in Darkplaces), this is taken from FitzQuake.
Also changed max_savegames to 48.
All of the modified code is signed with my nickname.

Credits: Ender, the primary Nehahra engine developer (for 2.54 source), and Bengt Jardrup (for 3.08 source which I have used and updated).
And this page - how to use libxmp api with SDL: http://xmp.sourceforge.net/libxmp.html#sdl-example 

In 2023, July 26 I moved from SDL 1.2 to SDL 2.0 because there was no music in the game on the Windows 10 64bit system.
zlib zconf.h file will appear when it builds with CMake in the "build" folder.
You should compile it in Visual C++ 6.0, Visual Studio 2005 or 2008 because this engine is as ancient as WinQuake 1.09, this engine is lightweight, with a relatively small amount of code compared to modern engines, for example Darkplaces and it supports ALL the features that were in the latest Nehahra release including dynamic music triggered by the QuakeC (progs.dat) commands "playmod" and "stopmod".
Uses: zlib, SDL2+libxmp to play tracker music and directx 7 sdk libraries and headers for sound effects and video.
Now the libxmp, zlib and SDL2 are submodules because they are suitable for an ancient Visual C++ 6.0, Visual Studio 2005 or 2008 except directx 7 old ancient sdk libraries and headers (see vid_win.c source file). I'm using SDL 2.0.8 headers and libs, not the latest version from github. You will possibly need a Windows 98 or a Windows XP virtual machine to compile it.
libxmp, zlib and SDL2 needs to be compiled from source code because the dsp project is set up to build dirs Debug/Release, then the source should compile.
