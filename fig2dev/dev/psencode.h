/*
 * Fig2dev: Translate Fig code to various Devices
 * Copyright (c) 1991 by Micah Beck
 * Parts Copyright (c) 1985-1988 by Supoj Sutanthavibul
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

#ifndef PSENCODE_H
#define PSENCODE_H

#include "bool.h"

extern bool	psencode_header_done;
extern bool	transp_header_done;

extern void PSencode_header(void);
extern void PStransp_header(void);
/* typedef unsigned char byte
 * extern long PSencode(int Width, int Height, int Transparent, int Ncol,
 *		byte R[], byte G[], byte B[], unsigned char *data);	*/
extern long PSencode(int Width, int Height, int Transparent, int Ncol,
		unsigned char R[], unsigned char G[], unsigned char B[],
		unsigned char *data);
extern void PSrgbimage(FILE *file, int width, int height, unsigned char *data);

#endif /* PSENCODE_H */
