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
 * readpics.c: read image files
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>

#include "fig2dev.h"	/* includes "bool.h" */

/*
 * Open the file 'name' and return its type (real file=0, pipe=1) in 'type'.
 * Return the full name in the buffer pointed to by 'retname'.
 * The caller must free(*retname) after calling open_picfile()!
 * 'retname' will have a .gz, .z or .Z if the file is zipped/compressed. If the
 * caller cannot take a pipe (pipeok=false), and the file is compressed,
 * uncompress and return name with .z, .Z or .gz stripped.
 * The return value is the FILE stream.
 */

FILE *
open_picfile(char *name, int *type, bool pipeok, char **retname)
{
    char	*unc;			/* temp buffer for gunzip command */
    FILE	*fstream;		/* handle on file  */
    struct stat	 status;
    size_t	len;
    size_t	pos;

    *type = 0;
    len = strlen(name);
    *retname = malloc(len + 4);
//  *retname[0] = '\0';
    if (pipeok) {
	unc = malloc(len + 17);		/* "gunzip -q -c " name ".gz" */
	strcpy(unc, "gunzip -q -c ");	/* tell gunzip to output to stdout */
    } else {
	unc = malloc(len + 14);		/* "gunzip -q " name ".gz" */
	strcpy(unc, "gunzip -q ");
    }
    pos = strlen(unc);

    if (!stat(name, &status)) {
	/* see if the filename ends with .Z or .z or .gz */
	if ((len > 3 && !strcmp(".gz", name + (len-3))) ||
		    (len > 2 && (!strcmp(".z", name + (len-2)) ||
				 !strcmp(".Z", name + (len-2))))) {
	/* yes, make command to uncompress it */
	memcpy(unc + pos, name, len + 1);	/* strcat(unc, name) */
	*type = 1;
	} else {
	    /* use straight name */
	    *type = 0;
	}
	memcpy(*retname, name, len + 1);    /* strcpy(*retname, name) */
    } else {
	/* no, see if the file with .gz, .z or .Z appended exists */
	/* failing that, if there is an absolute path, strip it and
	   look in current directory */
	/* check for .gz */
	memcpy(*retname, name, len);
	memcpy(*retname + len, ".gz", (size_t) 4); /* retname = name".gz" */
	if (!stat(*retname, &status)) {
	    /* yes, found with .gz */
	    memcpy(unc + pos, *retname, len + 4);
	    *type = 1;
	} else {
	    /* no, check for .z */
	    memcpy(*retname + len, ".z", (size_t) 3);
	    if (!stat(*retname, &status)) {
		/* yes, found with .z */
		memcpy(unc + pos, *retname, len + 3);
		*type = 1;
	    } else {
		/* no, check for .Z */
		memcpy(*retname + len, ".Z", (size_t) 3);
		if (!stat(*retname, &status)) {
		    /* yes, found with .Z */
		    memcpy(unc + pos, *retname, len + 3);
		    *type = 1;
		} else {
		    char *p;
		    /* can't find it, if there is a path,
		       strip it and look in current directory */
		    if ((p = strrchr(name, '/'))) {
			/* yes, strip it off */
			/* strcpy(*retname, p + 1); */
			memcpy(*retname, p + 1, len -= (p - name));
			if (!stat(*retname, &status)) {
			    *type = 0;
			    memcpy(name, *retname, len);
			} else {
			    /* All is lost */
			    free(unc);
			    return NULL;
			}
		    }
		}
	    }
	}
    }

    /* if a pipe, but the caller needs a file, uncompress the file now */
    if (*type == 1 && !pipeok) {
	char *p;
	system(unc);
	if ((p = strrchr(*retname,'.'))) {
	    *p = '\0';		/* terminate name before last .gz, .z or .Z */
	}
	strcpy(name, *retname);
	/* force to plain file now */
	*type = 0;
    }

    if (*type == 0)
	fstream = fopen(name, "rb");
    else  /* *type == 1 */
	fstream = popen(unc, "r");

    free(unc);
    return fstream;
}

void
close_picfile(FILE *file, int type)
{
    if (type == 0)
	fclose(file);
    else
	pclose(file);
}
