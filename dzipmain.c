#include <sys/stat.h>
#include <time.h>
#include "dzip.h"
#include <sys/utime.h>

int p_blocksize = 32768; //16000
FILE *outfile;
direntry_t *directory;
int numfiles;
int intersize, totalsize;

char *inblk, *outblk, *tmpblk;
char *inptr;
long outlen;

cdata_t oldcd, newcd;
ent_t base[MAX_ENT], oldent[MAX_ENT], newent[MAX_ENT];
int entlink[MAX_ENT];
char dem_updateframe;
char copybaseline;
long dem_gametime;
int maxent, lastent, sble;
int maj_ver, min_ver;
int demotime;
long dzPakPos; // Hack to make reading DZ work inside PAK files

#ifdef BIG_ENDIAN

short getshort(char *c)
{
	return (c[1] << 8) + c[0];
}

long getlong(char *c)
{
	return (c[3] << 24) + (c[2] << 16) + (c[1] << 8) + c[0];
}

float getfloat(char *c)
{
	long l = getlong(c);
	return *(float*)(&l);
}

#endif

z_stream zip_stream;
char *zbuf;
int zip_err;
int ztotal;
int zlevel = Z_DEFAULT_COMPRESSION;

void check_dzip_err (char *where)
{
	if (zip_err != Z_OK)
        Sys_Error("error in %s (%d)", where, zip_err);
}

void safe_fwrite (void *data, size_t size, FILE *f) {
	if (fwrite(data, 1, size, f) < size)
                Sys_Error("error while writing file: %s", strerror(errno));
}

void write_file (void *ptr, int len) {
	zip_stream.next_in = ptr;
	zip_stream.avail_in = len;

	while (zip_stream.avail_in)
	{
		zip_stream.next_out = zbuf;
		zip_stream.avail_out = Z_BUFFER_SIZE;
		zip_err = deflate(&zip_stream, Z_NO_FLUSH);
		check_dzip_err("writefile");
		safe_fwrite(zbuf,Z_BUFFER_SIZE - zip_stream.avail_out,outfile);
		totalsize += Z_BUFFER_SIZE - zip_stream.avail_out;
	}
	intersize += len;
}

void read_file (FILE *infile, int *len, int max) {
	int toread = (p_blocksize - *len > max - totalsize)?
				max - totalsize : p_blocksize - *len;
	if (!toread) return;

	zip_stream.next_out = inblk + *len;
	zip_stream.avail_out = toread;

	if (zip_stream.avail_in > (unsigned int) p_blocksize)
	{
		int bsize = (ztotal > Z_BUFFER_SIZE)? Z_BUFFER_SIZE : ztotal;
		ztotal -= bsize;
		fread(zbuf,bsize,1,infile);
		zip_stream.next_in = zbuf;
		zip_stream.avail_in = bsize;
	}
	zip_err = inflate(&zip_stream, Z_NO_FLUSH);
	if (zip_err != Z_STREAM_END && zip_err != Z_BUF_ERROR)
		check_dzip_err("inflate feed");

	while (zip_stream.avail_out)
	{
		int bsize = (ztotal > Z_BUFFER_SIZE)? Z_BUFFER_SIZE : ztotal;
		ztotal -= bsize;
		fread(zbuf,bsize,1,infile);
		zip_stream.next_in = zbuf;
		zip_stream.avail_in = bsize;
		zip_err = inflate(&zip_stream, Z_NO_FLUSH);
		if (zip_err == Z_STREAM_END) break;
		check_dzip_err("inflate feed more");
	}

	*len += toread;
	totalsize += toread;
}

// read in all the stuff from a dz file, after making sure it's ok
// gui version returns 0 if it's not, other versions won't return
int OpenDZFile (FILE *handle, char *src)
{
	int  id, i;

	dzPakPos = ftell (handle);

        fread(&id,4,1,handle); id = cnvlong(id);
	if ((id & 0xffff) != 'D' + ('Z' << 8))
	{
                Sys_Error("%s is not a dzip file",src);
		return 0;
	}
	maj_ver = (id >> 16) & 0xff;
	min_ver = (id >> 24) & 0xff;
	if (maj_ver > MAJOR_VERSION)
	{
                Sys_Error("%s was compressed with version %u, "
			"but I'm only version %u",
			src, maj_ver, MAJOR_VERSION);
		return 0;
	}

        fread(&i, 4, 1, handle); i = cnvlong(i);
        fread(&numfiles, 4, 1, handle); numfiles = cnvlong(numfiles);
	directory = malloc(numfiles * sizeof(direntry_t));
        fseek(handle, dzPakPos + i, SEEK_SET);
	if (maj_ver == 1) ztotal = i - 12;
	
	for (i = 0; i < numfiles; i++)
                read_direntry(handle, directory + i);
        fseek(handle, dzPakPos + 12, SEEK_SET);
	
	return 1;
}

// returns pointer to filename from a full path
char *GetFileFromPath (char *in)
{
	char *x = in - 1;

	while (*++x)
		if (*x == '/' || *x == '\\')
			in = x + 1;
	return in;
}

// returns a pointer directly to the period of the extension,
// or it there is none, to the 0 character at the end.
char *FileExtension (char *in)
{
	char *e = in + strlen(in);

	in = GetFileFromPath(in);
	while ((in = strchr(in, '.')))
		e = in++;

	return e;
}

int getfiledate (char *filename)
{
	struct stat filestats;
	struct tm *trec;

	stat(filename,&filestats);
	trec = localtime(&filestats.st_mtime);
	return (trec->tm_sec >> 1) + (trec->tm_min << 5)
		+ (trec->tm_hour << 11) + (trec->tm_mday << 16)
		+ (trec->tm_mon << 21) + ((trec->tm_year - 80) << 25);
}

void setfiledate (char *filename, int date)
{
	struct tm timerec;
	struct utimbuf tbuf;

	timerec.tm_hour = (date >> 11) & 0x1f;
	timerec.tm_min = (date >> 5) & 0x3f;
	timerec.tm_sec = (date & 0x1f) << 1;
	timerec.tm_mday = (date >> 16) & 0x1f;
	timerec.tm_mon = (date >> 21) & 0x0f;
	timerec.tm_year = ((date >> 25) & 0x7f) + 80;
	timerec.tm_isdst = -1;
	tbuf.actime = tbuf.modtime = mktime(&timerec);
	utime(filename,&tbuf);
}

#ifdef WIN32
#define strcasecmp stricmp
#endif

int get_filetype (char *name)
{
	char *ext = FileExtension(name);
	if (!strcasecmp(ext, ".dem"))
		return TYPE_DEM;
	if (!strcasecmp(ext, ".pak"))
		return TYPE_PAK;
	if (!strcasecmp(ext, ".dz"))
		return TYPE_DZ;
	return TYPE_NORMAL;
}

void read_input (FILE *infile, void *ptr, int len)
{
        fread(ptr,len,1,infile);
	make_crc(ptr,len);
}

void write_output (void *ptr, int len)
{
	if (outfile) safe_fwrite(ptr,len,outfile);
	make_crc(ptr,len);
}

void read_direntry(FILE *infile, direntry_t *de)
{
	char *s;
	int size = sizeof(direntry_t) - 4;
	if (maj_ver == 1) size -= 8; 

	fread(de,size,1,infile);
	de->ptr = cnvlong(de->ptr);
	de->size = cnvlong(de->size);
	de->real = cnvlong(de->real);
	de->len = cnvlong(de->len) & 0xffff;
	de->pak = cnvlong(de->pak) & 0xffff;
	de->crc = cnvlong(de->crc);
	de->type = cnvlong(de->type);
	de->date = cnvlong(de->date);
	de->inter = cnvlong(de->inter);
	s = malloc(de->len);
	fread(s,de->len,1,infile);
	de->name = s;
}

void write_direntry(direntry_t *de)
{
	de->ptr = cnvlong(de->ptr);
	de->size = cnvlong(de->size);
	de->real = cnvlong(de->real);
	de->len = cnvlong(de->len) & 0xffff;
	de->pak = cnvlong(de->pak) & 0xffff;
	de->crc = cnvlong(de->crc);
	de->type = cnvlong(de->type);
	de->date = cnvlong(de->date);
	de->inter = cnvlong(de->inter);
	safe_fwrite(de,sizeof(direntry_t)-4,outfile);
	de->len = cnvlong(de->len);
	safe_fwrite(de->name,de->len,outfile);
}

void WriteEndOfDZFile()
{
	int i, j;
	j = directory[numfiles - 1].ptr + directory[numfiles - 1].size;
	fseek(outfile,4,SEEK_SET);
	i = cnvlong(j); safe_fwrite(&i,4,outfile);
	i = cnvlong(numfiles); safe_fwrite(&i,4,outfile);
	fseek(outfile,j,SEEK_SET);

	for (i = 0; i < numfiles; i++)
		write_direntry(directory+i);
}

FILE *open_create(char *src)
{
	FILE *f;

	f = fopen(src,"wb+");
        if (!f) Con_Printf("Dzip: Could not create %s\n",src);
	return f;
}

void copy_msg(int i)
{
	memcpy(outblk + outlen, inptr, i);
	outlen += i;
	inptr += i;
}

void insert_msg(void *msg, int i)
{
	memcpy(outblk + outlen, msg, i);
	outlen += i;
}

void discard_msg(int i)
{
	inptr += i;
}

#undef malloc
void *fsmalloc (size_t i)
{
	void *m = malloc(i);
        if (!m) Sys_Error("Out of memory!");
	return m;		
}
#define malloc fsmalloc

void DzipInit (void)
{
	int oflag = 0;
	int fileargs = 0;
	char *optr = NULL;
	char **files;

	crc_init();
        directory = malloc(sizeof(direntry_t));
	inblk = malloc(p_blocksize);
	outblk = malloc(p_blocksize);
	tmpblk = malloc(p_blocksize);
	zbuf = malloc(Z_BUFFER_SIZE);
	files = malloc(fileargs * sizeof(char*));
}
