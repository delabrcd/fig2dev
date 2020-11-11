/*
 * Fig2dev: Translate Fig code to various Devices
 * Parts Copyright (c) 1989-2015 by Brian V. Smith
 * Parts Copyright (c) 2015-2019 by Thomas Loimer
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and documentation
 * files (the "Software"), including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense and/or sell copies
 * of the Software, and to permit persons who receive copies from any such
 * party to do so, with the only requirement being that the above copyright
 * and this permission notice remain intact.
 *
 */

/*
 * readgif.c: import gif into PostScript
 *
 */

/* Some of the following code is extracted from giftoppm.c,
   from the pbmplus package */

/* +-------------------------------------------------------------------+ */
/* | Copyright 1990, David Koblas.                                     | */
/* |   Permission to use, copy, modify, and distribute this software   | */
/* |   and its documentation for any purpose and without fee is hereby | */
/* |   granted, provided that the above copyright notice appear in all | */
/* |   copies and that both that copyright notice and this permission  | */
/* |   notice appear in supporting documentation.  This software is    | */
/* |   provided "as is" without express or implied warranty.           | */
/* +-------------------------------------------------------------------+ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <limits.h>

#include "fig2dev.h"	/* includes "bool.h" */
#include "object.h"	/* does #include <X11/xpm.h> */
#include "readpics.h"

extern	int	 _read_pcx(FILE *pcxfile, F_pic *pic);		/* readpcx.c */
static bool	 ReadColorMap(FILE *, unsigned int,
				unsigned char cmap[3][MAXCOLORMAPSIZE]);
static bool	 DoGIFextension(FILE *, int);
static int	 GetDataBlock(FILE *, unsigned char *);

#define LOCALCOLORMAP		0x80
#define	ReadOK(file,buffer,len)	(fread(buffer, len, 1, file) != 0)
#define BitSet(byte, bit)	(((byte) & (bit)) == (bit))

#define LM_to_uint(a,b)			(((b)<<8)|(a))

struct {
	unsigned int	Width;
	unsigned int	Height;
	unsigned int	BitPixel;
	unsigned char	ColorMap[3][MAXCOLORMAPSIZE];
	unsigned int	ColorResolution;
	unsigned int	Background;
	unsigned int	AspectRatio;
} GifScreen;

struct {
	int	transparent;
	int	delayTime;
	int	inputFlag;
	int	disposal;
} Gif89 = { -1, -1, -1, 0 };

/* return codes:  1 : success
		  0 : invalid file
*/

int
read_gif(char *filename, int filetype, F_pic *pic, int *llx, int *lly)
{
	char		 buf[512];
	char		 *realname, *cmd;
	FILE		*file, *giftopcx;
	int		 i, stat;
	int		 useGlobalColormap;
	unsigned int	 bitPixel;
	unsigned char	 c;
	char		 version[4];
	unsigned char    transp[3]; /* RGB of transparent color (if any) */

	/* open the file */
	if ((file = open_picfile(filename,&filetype,false,&realname)) == NULL) {
		free(realname);
		fprintf(stderr,"No such GIF file: %s\n", filename);
		return 0;
	}

	/* first read header to look for any transparent color extension */

	*llx = *lly = 0;

	if (! ReadOK(file,buf,6)) {
		return 0;
	}

	if (strncmp((char*)buf,"GIF",3) != 0) {
		return 0;
	}

	strncpy(version, (char*)(buf + 3), 3);
	version[3] = '\0';

	if ((strcmp(version, "87a") != 0) && (strcmp(version, "89a") != 0)) {
		free(realname);
		fprintf(stderr,"Unknown GIF version %s\n",version);
		return 0;
	}

	if (! ReadOK(file,buf,7)) {
		free(realname);
		return 0;		/* failed to read screen descriptor */
	}

	GifScreen.Width           = LM_to_uint(buf[0],buf[1]);
	GifScreen.Height          = LM_to_uint(buf[2],buf[3]);
	GifScreen.BitPixel        = 2<<(buf[4]&0x07);
	GifScreen.ColorResolution = (((((int)buf[4])&0x70)>>3)+1);
	GifScreen.Background      = (unsigned int) buf[5];
	GifScreen.AspectRatio     = (unsigned int) buf[6];

	if (BitSet(buf[4], LOCALCOLORMAP)) {	/* Global Colormap */
		if (!ReadColorMap(file,GifScreen.BitPixel,pic->cmap)) {
			free(realname);
			return 0;	/* error reading global colormap */
		}
	}

	/* assume no transparent color for now */
	Gif89.transparent =  -1;

	for (;;) {
		if (! ReadOK(file,&c,1)) {
			free(realname);
			return 0;	/* EOF / read error on image data */
		}

		if (c == ';') {		/* GIF terminator, finish up */
			break;		/* all done */
		}

		if (c == '!') {		/* Extension */
		    if (! ReadOK(file,&c,1))
			fprintf(stderr,
				"GIF read error on extension function code\n");
		    (void) DoGIFextension(file, c);
		    continue;
		}

		if (c != ',') {		/* Not a valid start character */
			continue;
		}

		if (! ReadOK(file,buf,9)) {
			free(realname);
			return 0;      /* couldn't read left/top/width/height */
		}

		useGlobalColormap = ! BitSet(buf[8], LOCALCOLORMAP);

		bitPixel = 1<<((buf[8]&0x07)+1);

		if (! useGlobalColormap) {
		    if (!ReadColorMap(file, bitPixel, pic->cmap)) {
			free(realname);
			fprintf(stderr, "error reading local GIF colormap\n");
			return 0;
		    }
		}
		break;			/* image starts here, header is done */
	}

	/* output PostScript comment */
	fprintf(tfp, "%% Originally from a GIF File: %s\n\n", pic->file);

	/* save transparent indicator */
	pic->transp = Gif89.transparent;
	/* and RGB values */
	if (pic->transp != -1) {
	    transp[RED]   = pic->cmap[RED][pic->transp];
	    transp[GREEN] = pic->cmap[GREEN][pic->transp];
	    transp[BLUE]  = pic->cmap[BLUE][pic->transp];
	}

	/* reposition the file at the beginning */
	fseek(file, 0, SEEK_SET);

	/* now call giftopnm and ppmtopcx */
	if ((cmd = malloc(strlen(realname) + 50)) == NULL) {
		fputs("Not enough memory to store command string.\n", stderr);
		free(realname);
		return 0;
	}
	sprintf(cmd, "giftopnm -quiet %s | ppmtopcx -quiet 2>/dev/null",
		realname);
	free(realname);
	if ((giftopcx = popen(cmd, "r")) == 0) {
		fprintf(stderr, "Cannot open pipe from giftopnm | ppmtopcx.\n");
		free(cmd);
		return 0;
	}
	free(cmd);
	/* now call _read_pcx to read the pcx file */
	stat = _read_pcx(giftopcx, pic);
	/* close file */
	pclose(giftopcx);

	/* now match original transparent colortable index with possibly new
	   colortable from ppmtopcx */
	if (pic->transp != -1) {
	    for (i=0; i<pic->numcols; i++) {
		if (pic->cmap[RED][i]   == transp[RED] &&
		    pic->cmap[GREEN][i] == transp[GREEN] &&
		    pic->cmap[BLUE][i]  == transp[BLUE])
			break;
	    }
	    if (i < pic->numcols)
		pic->transp = i;
	}

	return stat;
}

static bool
ReadColorMap(FILE *fd, unsigned int number,
		unsigned char cmap[3][MAXCOLORMAPSIZE])
{
	unsigned int	i;
	unsigned char	rgb[3];

	for (i = 0; i < number; ++i) {
	    if (! ReadOK(fd, rgb, sizeof(rgb))) {
		fprintf(stderr,"bad GIF colormap\n" );
		return false;
	    }
	    cmap[RED][i]   = rgb[RED];
	    cmap[GREEN][i] = rgb[GREEN];
	    cmap[BLUE][i]  = rgb[BLUE];
	}
	return true;
}

static bool
DoGIFextension(FILE *fd, int label)
{
	static unsigned char buf[256];
	char	    *str;

	switch (label) {
	    case 0x01:		/* Plain Text Extension */
		str = "Plain Text Extension";
		break;
	    case 0xff:		/* Application Extension */
		str = "Application Extension";
		break;
	    case 0xfe:		/* Comment Extension */
		str = "Comment Extension";
		while (GetDataBlock(fd, buf) != 0) {
			; /* GIF comment */
		}
		return false;
	    case 0xf9:		/* Graphic Control Extension */
		str = "Graphic Control Extension";
		(void) GetDataBlock(fd, (unsigned char*) buf);
		Gif89.disposal    = (buf[0] >> 2) & 0x7;
		Gif89.inputFlag   = (buf[0] >> 1) & 0x1;
		Gif89.delayTime   = LM_to_uint(buf[1],buf[2]);
		if ((buf[0] & 0x1) != 0)
			Gif89.transparent = buf[3];

		while (GetDataBlock(fd, buf) != 0)
			;
		return false;
	    default:
		str = (char *) buf;
		sprintf(str, "UNKNOWN (0x%02x)", label);
		break;
	}

	while (GetDataBlock(fd, buf) != 0)
		;

	return false;
}

int	ZeroDataBlock = false;

static int
GetDataBlock(FILE *fd, unsigned char *buf)
{
	unsigned char	count;

	/* error in getting DataBlock size */
	if (! ReadOK(fd,&count,1)) {
		return -1;
	}

	ZeroDataBlock = count == 0;

	/* error in reading DataBlock */
	if ((count != 0) && (! ReadOK(fd, buf, count))) {
		return -1;
	}

	return count;
}
