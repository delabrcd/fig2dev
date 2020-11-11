/*
 * Fig2dev: Translate fig code to various devices.
 * Copyright (c) 1991 by Micah Beck
 * Parts Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2015 by Brian V. Smith
 * Parts Copyright (c) 2017,2018 by Thomas Loimer
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
 * xtmpfile.c: create a temporary file
 * Author: Thomas Loimer, 2017
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>	/* mkstemp(), rand(), getenv() */
#include <string.h>	/* strcpy(), strlen() */
#if ! defined(HAVE_MKSTEMP) || ! defined(HAVE_FDOPEN)
#include <time.h>
#include <string.h>
#include <limits.h>
#include "bool.h"
#endif

#include "xtmpfile.h"


/*
 * Create a unique temporary filename from template and create and open the file
 * in a temporary directory for reading and writing. Return a file stream to
 * the file. After return, template points to the created filename.
 * The template must end with "XXXXXX" and should be of size L_xtmpnam. The
 * template must be large enough to hold, in addition to filename, the path to
 * the temporary directory.
 * Returns NULL on error.
 */

FILE *
xtmpfile(char *template)
{
	const char	mode[] = "wb+";
	int		i;
	size_t		n, t;
	char		buf[L_xtmpnam];
#if ! defined(P_tmpdir) || ! defined(HAVE_MKSTEMP) || ! defined(HAVE_FDOPEN)
	char 		*p;
#endif
#if ! defined(HAVE_MKSTEMP) || ! defined(HAVE_FDOPEN)
	FILE		*f = NULL;
	static bool	seeded = false;
#endif

	n = strlen(template);
	/* prepend tmpdir to template, otherwise stay in current dir */
#ifdef P_tmpdir
	t = strlen(P_tmpdir);
	if (n + t + 1 < (size_t)L_xtmpnam) {
		/* strcpy(buf, P_tmpdir "/"); */
		memcpy(buf, P_tmpdir, t);
		memcpy(buf + t, "/", (size_t)2);
#else
	/*
	 * The GetTempPath function in Visual Studio uses the first path found
	 * in TMP, TEMP, USERPROFILE, then the Windows folder, see
	 * https://msdn.microsoft.com/en-us/library/aa364992(VS.85).aspx
	 * (retrieved 2018-03-04).
	 */
	if ((p = (getenv("TMP") || getenv("TEMP"))) &&
			(t = strlen(p)) + n + 1 < (size_t)L_xtmpnam) {
		/* strcpy(buf, TMP||TEMP "/"); */
		memcpy(buf, p, t);
		memcpy(buf + t, "/", (size_t)2);
#endif
	} else if (n < (size_t)L_xtmpnam) {
		buf[0] = '\0';
	} else {
		fprintf(stderr, "Cannot create temporary file. "
				"Filename too long:\n%s\n", template);
		return NULL;
	}
	strcat(buf, template);
	strcpy(template, buf);

	/* Create and open a file that does not yet exist. */
#if defined(HAVE_MKSTEMP) && defined(HAVE_FDOPEN)
	if ((i = mkstemp(template)) == -1)
		return NULL;
	return fdopen(i, mode);
#else
	/* check input template */
	i = strlen(template);
	if (i <  7 || memcmp(p = template + i - 6, "XXXXXX", (size_t)6) != 0)
		return NULL;

	/* write a random sequence to the six XXXXXX */
	if (!seeded) {
		srand((unsigned int)time(NULL));
		seeded = true;
	}
	for (i = 1; f == NULL && i < 9; ++i) {
		sprintf(p, "%6x", (unsigned)(rand() & 0xffffff));
		if (fopen(template, "rb") == NULL)
			f = fopen(template, mode);
	}
	return f;
#endif
}
