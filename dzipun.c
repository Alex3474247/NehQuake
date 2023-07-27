#include "dzip.h"

FILE *pakfile;
pakentry_t *pakdir;
int paknum, pakptr;
char flag[5];

void initpakfile (direntry_t *de)
{
	if (flag[SW_VERIFY]) 
		{ paknum = 0; pakfile = NULL; return; }
	pakdir = malloc(de->pak * sizeof(pakentry_t));
	paknum = *(int*)"PACK";
	if (pakfile) safe_fwrite(&paknum,4,pakfile);
	if (pakfile) safe_fwrite(&paknum,4,pakfile);
	if (pakfile) safe_fwrite(&paknum,4,pakfile);
	paknum = 0; pakptr = 12;
}

void closepakfile (direntry_t *de)
{
	int tmp;

	if (!pakfile) { free(pakdir); return; }

	safe_fwrite(pakdir,paknum*sizeof(pakentry_t),pakfile);	
	fseek(pakfile,4,SEEK_SET);
	tmp = cnvlong(pakptr);
	safe_fwrite(&tmp,4,pakfile);
	paknum *= 64;
	tmp = cnvlong(paknum);
	safe_fwrite(&tmp,4,pakfile);
	free(pakdir);
	fclose(pakfile);
	if (maj_ver != 1) setfiledate(de[-paknum/64].name,de->date);
	pakfile = NULL;
}

void init_zlib_decompression ()
{
	zip_stream.zalloc = (alloc_func)0;
	zip_stream.zfree = (free_func)0;
	zip_stream.opaque = (voidpf)0;

	zip_err = inflateInit(&zip_stream);
	zip_stream.avail_in = p_blocksize + 1;
	check_dzip_err("inflateinit");
}

void end_zlib_decompression ()
{
	inflateEnd(&zip_stream);
}

int check_filetype (int type)
{
        if (type >= TYPE_LAST) Sys_Error("unknown type of file");
	return (type == TYPE_DEM || type == TYPE_DEMV1
		|| type == TYPE_NEHAHRA);
}

void uncompress_file (FILE *handle, char *src, char *outname)
{
	int i, inlen, ptr, readptr, eofptr, blocksize, j;
	char demomode;
	direntry_t *de;
        char *action = "extracting";
        char demname[128]; // MAX_OSPATH

        OpenDZFile(handle, src);
//        de->type = TYPE_NEHAHRA;

	printf("%s created using v%u.%u\n", src, maj_ver, min_ver);
//        maj_ver = 1;

	inlen = 0;
	if (maj_ver == 1)
	{
		ptr = directory[numfiles-1].ptr + directory[numfiles-1].size;
		readptr = totalsize = 12;
		init_zlib_decompression();
	}

	COM_StripExtension (src, demname);
	COM_DefaultExtension (demname, ".dem");

	for (i = 0, j = -1; i < numfiles; i++)
	{
		de = directory + i;

		if (!stricmp(de->name, demname))
			break; // Found requested demo
		
		if (j == -1 && !stricmp(COM_FileExtension(de->name), "dem"))
			j = i; // Remember first demo in archive
	}

	// If requested demo not found, unpack first demo
	if (i == numfiles && j != -1)
		strcpy (demname, (directory + j)->name);

	for (i = 0; i < numfiles; i++)
	{
		de = directory + i;

		// Only unpack the .dem file
		if (stricmp(de->name, demname))
			continue;

		crcval = INITCRC;
		fseek(handle, dzPakPos + de->ptr, SEEK_SET);
		if (!de->pak)
			printf("%s %s",action,de->name);
		fflush(stdout);
		if (de->type == TYPE_PAK)
		{
			printf("%s %s:",action,de->name);
			if (flag[SW_VERIFY])
				pakfile = NULL;
			else
			{
			#ifdef GUI
				pakfile = GuiGetUncompressOutfile(de);
				if (!pakfile && GuiAbort) return;
			#else
                                pakfile = open_create(outname);
			#endif
			}
			if (pakfile || flag[SW_VERIFY]) printf("\n");
                        initpakfile(de);
			i++; de++;
			outfile = pakfile;
		}
		demomode = check_filetype(de->type);
                outfile = open_create(outname);
                if (demomode) dem_uncompress_init(TYPE_NEHAHRA);

		if (maj_ver > 1)
		{
			init_zlib_decompression();
			readptr = totalsize = 0;
			ptr = eofptr = de->inter;
			ztotal = de->size;
		}
		else
			eofptr = de->ptr + de->size;

		while (readptr < eofptr)
		{
                        read_file(handle,&inlen,ptr);

			if (demomode)
				blocksize = dem_uncompress();
			else
			{
				blocksize = totalsize - readptr;
				if (totalsize >= eofptr)
					blocksize = eofptr - readptr;
				write_output(inblk,blocksize);
			}
			if (blocksize != p_blocksize)
				memcpy(inblk,inblk+blocksize,p_blocksize-blocksize);
			readptr += blocksize;
			inlen -= blocksize;
		}

		if (!de->pak && outfile)
		{
                        fclose(outfile);
//                        if (maj_ver > 1) setfiledate(de->name,de->date);
		}
//                demotime =de->date;

		if (pakfile && (i+1 == numfiles || !directory[i+1].pak
			|| directory[i+1].type == TYPE_PAK))
			closepakfile(de);

//                if (crcval != de->crc) 
//                        if (flag[SW_CRC])
//                                fprintf(stderr,": warning: CRC error!\n");
//                        else
//                                Sys_Error("CRC checksum error! Archive is broken!");

		if (flag[SW_VERIFY]) printf(": ok\n");
		else printf("\n");

		if (maj_ver > 1) end_zlib_decompression();
	}
	if (maj_ver == 1) end_zlib_decompression();

	for (i = 0; i < numfiles; free(directory[i++].name));
	free(directory);
        fclose(handle);
}
