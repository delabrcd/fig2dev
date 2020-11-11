/*
 * Fig2dev: Translate Fig code to various Devices
 * Parts Copyright (c) 1989-2007 by Brian V. Smith
 * Parts Copyright (c) 2015-2017 by Thomas Loimer
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
 * readtif.c: import tiff into PostScript
 *
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "fig2dev.h"
#include "object.h"	/* does #include <X11/xpm.h> */

extern	int	_read_pcx(FILE *pcxfile, F_pic *pic);	/* readpcx.c */

/* return codes:  1 : success
		  0 : invalid file
*/

/*
 * tifftopnm needs a seekable file as input, no pipe. See man tifftopnm(0).
 */
int
read_tif(char *filename, int filetype, F_pic *pic, int *llx, int *lly)
{
	(void)	filetype;
	char	*cmd;
	FILE	*tiftopcx;
	int	 stat;

	*llx = *lly = 0;
	/* output PostScript comment */
	fprintf(tfp, "%% Originally from a TIFF File: %s\n\n", pic->file);

	/* allocate buffer and make command to convert tif to pnm then to pcx */
	if ((cmd = malloc(strlen(filename) + 50)) == 0) {
		fprintf(stderr,
	"Cannot allocate memory for the command line to read tiff file %s.\n",
			filename);
		return 0;
	}
	sprintf(cmd, "tifftopnm %s 2>/dev/null | ppmtopcx 2>/dev/null",
		filename);
	if ((tiftopcx = popen(cmd, "r")) == 0) {
		fputs("Cannot open pipe to tifftopnm or ppmtopcx\n", stderr);
		return 0;
	}
	free(cmd);
	/* now call _read_pcx to read the pcx file */
	stat = _read_pcx(tiftopcx, pic);
	pic->transp = -1;
	/* close the pipe */
	fclose(tiftopcx);

	return stat;
}
