I had to cut off the FMOD out of Nehahra 3.08 source and replace it with something modern, for example libXMP, but libXMP doesn't play the modules itself, only via the SDL.
And now Nehahra source is fully GPL compatible (libxmp is free and SDL too, at least in 2021).
I have stopped at libxmp 4.5 (to decode Nehahra .xm and .s3m music to PCM data) and SDL2 libraries (SDL2 is for playing the PCM data directly).
I also changed the protocol number to be different to Bengt's protocol so that HUD can display values more than 255 (up to 999, as in Darkplaces), this is taken from FitzQuake.
Also changed max_savegames to 48.
All of the modified code is signed with my nickname.

SDL and libxmp headers and libraries are imported from the latest Quakespasm engine.
All needed files are included in this archive to compile the engine without problems.

Credits: Ender, the primary Nehahra engine developer (for 2.54 source), and Bengt Jardrup (for 3.08 source which I have used and updated).
And this page - how to use libxmp api: http://xmp.sourceforge.net/libxmp.html#sdl-example 
In 2023, July 26 I have moved from SDL 1.2 to SDL 2.0 because there was no music in the game.