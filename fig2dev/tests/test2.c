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

/*
 * test2.c: Check, whether read_pdf() finds the bounding box of a pdf file.
 * Author: Thomas Loimer, 2019-12-14
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "bool.h"
#include "object.h"

/* the function to be tested, in $(top_srcdir)/fig2dev/dev/readeps.c */
extern int	read_pdf(FILE *file, int type, F_pic *pic, int *llx, int *lly);

/* symbols that are needed when calling read_pdf() */
int		urx = 0;
int		ury = 0;
int		metric = 0;
FILE		*tfp;
const struct _paperdef		/* from fig2dev.h */
{
	char	*name;
	int	width;
	int	height;
}	paperdef[1] = {{"letter", 8, 12}};


void	put_msg(const char *fmt, const char *file, const char *size)
{
	fprintf(stderr, fmt, file, size);
}

int
main(int argc, char *argv[])
{
	(void)	argc;
	int	llx = -1;
	int	lly = -1;
	FILE	*file;
	F_pic	pic;

	tfp = stdout;
	pic.file = argv[1];

	file = fopen(argv[1], "rb");
	if (file == NULL) {
		fprintf(stderr, "Test file %s not found.\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	if (read_pdf(file, 0, &pic, &llx, &lly) == 1 &&
			pic.bit_size.x != 10 && pic.bit_size.y != 10) {
		fprintf(stdout, "read_pdf found: width = %d, height = %d\n",
				pic.bit_size.x, pic.bit_size.y);
		exit(EXIT_SUCCESS);
	} else {
		exit(EXIT_FAILURE);
	}
}
