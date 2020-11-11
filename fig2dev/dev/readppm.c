/*
 * Fig2dev: Translate Fig code to various Devices
 * Parts Copyright (c) 1989-2015 by Brian V. Smith
 * Parts Copyright (c) 2015-2018 by Thomas Loimer
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
 * readppm.c: import ppm into PostScript
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <limits.h>

#include "fig2dev.h"
#include "object.h"	/* does #include <X11/xpm.h> */
#include "xtmpfile.h"

extern	int	_read_pcx(FILE *pcxfile, F_pic *pic);	/* readpcx.c */

/* return codes:  1 : success
		  0 : failure
*/

int
read_ppm(FILE *file, int filetype, F_pic *pic, int *llx, int *lly)
{
	(void)	filetype;
	char	buf[BUFSIZ];
	char	pcxname[L_xtmpnam + 17] = "f2dtmppcxXXXXXX";
	FILE	*giftopcx;
	int	stat;
	size_t	size;

	*llx = *lly = 0;
	/* output PostScript comment */
	fprintf(tfp, "%% Originally from a PPM File: %s\n\n", pic->file);

	/* make name for temp output file */
	if ((giftopcx = xtmpfile(pcxname)) == 0) {
		fprintf(stderr, "Cannot create temporary file %s\n", pcxname);
		return 0;
	}
	/* make command to convert gif to pcx into temp file */
	sprintf(buf, "ppmtopcx >%s 2>/dev/null", pcxname);
	if ((giftopcx = popen(buf,"w" )) == 0) {
		fprintf(stderr, "Cannot open pipe to giftoppm\n");
		remove(pcxname);
		return 0;
	}
	while ((size=fread(buf, 1, BUFSIZ, file)) != 0) {
		fwrite(buf, size, 1, giftopcx);
	}
	/* close pipe */
	pclose(giftopcx);
	if ((giftopcx = fopen(pcxname, "rb")) == NULL) {
		fprintf(stderr, "Cannot open temporary output file %s\n",
			pcxname);
		return 0;
	}
	/* now call _read_pcx to read the pcx file */
	stat = _read_pcx(giftopcx, pic);
	pic->transp = -1;
	/* close file */
	fclose(giftopcx);
	/* remove temp file */
	remove(pcxname);

	return stat;
}
