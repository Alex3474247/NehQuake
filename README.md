# NehQuake
Nehahra engine 3.08 that is meaned to play Nehahra partial conversion for Quake.

I cut off the FMOD dependency out of Nehahra 3.08 source and replaced it with something modern, for example libxmp, but libxmp doesn't play the modules itself, only via the SDL.
And now Nehahra source is fully GPL compatible (libxmp is free and the SDL too).
I have stopped at libxmp >= 4.5 (to decode Nehahra .xm and .s3m music to PCM data) and SDL2 libraries (SDL2 is for playing the PCM data directly).
I also changed the protocol number to be different to Bengt's protocol so that HUD can display values more than 255 (up to 999, as in Darkplaces), this is taken from FitzQuake.
Also changed max_savegames to 48.
All of the modified code is signed with my nickname.

Credits: Ender, the primary Nehahra engine developer (for 2.54 source), and Bengt Jardrup (for 3.08 source which I have used and updated).
And this page - how to use libxmp api with SDL: http://xmp.sourceforge.net/libxmp.html#sdl-example 

In 2023, July 26 I moved from SDL 1.2 to SDL 2.0 because there was no music in the game on the Windows 10 64bit system.
zlib zconf.h file will appear when it builds with CMake in the "build" folder.
You should compile it in Visual Studio 2019 (I use a free Community edition), this engine is lightweight, with a relatively small amount of code compared to modern engines, for example Darkplaces and it supports ALL the features that were in the latest Nehahra release including dynamic music triggered by the QuakeC (progs.dat) commands "playmod" and "stopmod".
Uses: zlib, SDL2+libxmp to play tracker music and directx 7 sdk libraries and headers for input, sound effects and video.
libxmp, zlib and SDL2 needs to be compiled from source code because the VS project is set up to build dirs Debug/Release, then the source should compile.
