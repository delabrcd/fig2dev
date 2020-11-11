
/*
 * Fig2dev: Translate Fig code to various Devices
 * Parts Copyright (c) 2002 by Anthony Starks
 * Parts Copyright (c) 2002-2006 by Martin Kroeker
 * Parts Copyright (c) 2002-2015 by Brian V. Smith
 * Parts Copyright (c) 2015-2020 by Thomas Loimer
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
 *
 * genvdx.c: convert fig to VDX
 *
 *  Author:  Kyle Narod
 *  Created: 6 November 2020

 * Comments from svg left in for reference
 *	* Correct color values;
 *	  hex codes see Fig_color_names in fig2dev.c, or, identical,
	  colorNames in xfig/src/resources.c
 *		colorNames	Fig_color_names	svg name
 *	0	"black"		"#000000"	black
 *	1	"blue"		"#0000ff"	blue
 *	2	"green"		"#00ff00"	lime
 *	3	"cyan"		"#00ffff"	cyan
 *	4	"red"		"#ff0000"	red
 *	5	"magenta"	"#ff00ff"	magenta
 *	6	"yellow"	"#ffff00"	yellow
 *	7	"white"		"#ffffff"	white
 *	8	"#000090"	"#000090" 144
 *	9	"#0000b0"	"#0000b0" 176
 *	10	"#0000d0"	"#0000d0" 208
 *	11	"#87ceff"	"#87ceff" 135 206
 *	12	"#009000"	"#009000"
 *	13	"#00b000"	"#00b000"
 *	14	"#00d000"	"#00d000"
 *	15	"#009090"	"#009090"
 *	16	"#00b0b0"	"#00b0b0"
 *	17	"#00d0d0"	"#00d0d0"
 *	18	"#900000"	"#900000"
 *	19	"#b00000"	"#b00000"
 *	20	"#d00000"	"#d00000"
 *	21	"#900090"	"#900090"
 *	22	"#b000b0"	"#b000b0"
 *	23	"#d000d0"	"#d000d0"
 *	24	"#803000"	"#803000" 128 48
 *	25	"#a04000"	"#a04000" 160 64
 *	26	"#c06000"	"#c06000" 192 96
 *	27	"#ff8080"	"#ff8080"
 *	28	"#ffa0a0"	"#ffa0a0"
 *	29	"#ffc0c0"	"#ffc0c0"
 *	30	"#ffe0e0"	"#ffe0e0"
 *	31	"gold"	        "#ffd700"	gold	(255, 215, 0)
 *
 */

/* Copied  from gensvg.c */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef	HAVE_STRINGS_H
#include <strings.h>
#endif
#include <math.h>

#include "fig2dev.h"	/* includes bool.h and object.h */
#include "object.h"	/* includes X11/xpm.h */
#include "bound.h"
#include "creationdate.h"
#include "messages.h"
#include "pi.h"

static const char* vdx_dash_string(int);

/* Copied from gensvg.c */
//Header for the vdx file
#define PREAMBLE "<?xml version=\"1.231\" encoding=\"UTF-8\" standalone=\"no\"?>"
//Max chars per line in the vdx file used for formatting the points list
#define	VDX_LINEWIDTH	76

char *type_str; /*string value of an object's type*/

/*
*	Takes in an int which corresponds to the index of hex colors. 
* 	Returns the corresponding hex color.
*/
static unsigned int
rgbColorVal(int colorIndex)
{				/* taken from genptk.c */
    unsigned int rgb;
    static unsigned int rgbColors[NUM_STD_COLS] = {
	0x000000, 0x0000ff, 0x00ff00, 0x00ffff, 0xff0000, 0xff00ff,
	0xffff00, 0xffffff, 0x00008f, 0x0000b0, 0x0000d1, 0x87cfff,
	0x008f00, 0x00b000, 0x00d100, 0x008f8f, 0x00b0b0, 0x00d1d1,
	0x8f0000, 0xb00000, 0xd10000, 0x8f008f, 0xb000b0, 0xd100d1,
	0x803000, 0xa14000, 0xb46100, 0xff8080, 0xffa1a1, 0xffbfbf,
	0xffe0e0, 0xffd600
    };

    if (colorIndex == DEFAULT)
	rgb = rgbColors[0];
    else if (colorIndex < NUM_STD_COLS)
	rgb = rgbColors[colorIndex];
    else
	rgb = ((user_colors[colorIndex - NUM_STD_COLS].r & 0xff) << 16)
	    | ((user_colors[colorIndex - NUM_STD_COLS].g & 0xff) << 8)
	    | (user_colors[colorIndex - NUM_STD_COLS].b & 0xff);
    return rgb;
}
/*
* Copied from gensvg.c Used for adjusting the line width. Did not use explicitly
*/
static double
linewidth_adj(int linewidth)
{
   /* Adjustment as in genps.c */
   return linewidth <= THICK_SCALE ?
		linewidth / 2. : (double)(linewidth-THICK_SCALE);
}

/*
* Copied from gensvg.c, used for print_vdxcomments below
*/
void
put_sanitized_char_vdx(int c)
{
	switch (c) {
	case '<':
		fputs("&lt;", tfp);
		break;
	case '>':
		fputs("&gt;", tfp);
		break;
	case '&':
		fputs("&amp;", tfp);
		break;
	default:
		fputc(c, tfp);
	}
}

/*
* Copied from gensvg.c. Used to output the comments from an object to the output file
* Takes in string to go before the comments, the comments itself and then a string to go after the comments
*/
void
print_vdxcomments(char *s1, F_comment *comments, char *s2)
{
	unsigned char	*c;
	while (comments) {
		fputs(s1, tfp);
		for (c = (unsigned char *)comments->comment; *c; ++c)
			put_sanitized_char_vdx((int)*c);
		fputs(s2, tfp);
		comments = comments->next;
	}
}

/*
* Copied from gensvg.c Did not use explicitly, but I believe this is for using this driver from command line.
* Did not update it, so this will act exactly how gensvg.c does.
*/
void
genvdx_option(char opt, char *optarg)
{
    switch (opt) {
	case 'G':		/* ignore language and grid */
	case 'L':
	    break;
	case 'z':
	    (void) strcpy (papersize, optarg);
	    paperspec = true;
	    break;
	default:
	    put_msg (Err_badarg, opt, "vdx");
	    exit (1);
    }
}

/*
* Start function for the driver. Prints header comments.
*/
void
genvdx_start(F_compound *objects)
{
    const struct paperdef	*pd;
    int     pagewidth = -1, pageheight = -1;
    int     vw, vh;
    char    date_buf[CREATION_TIME_LEN];

    fprintf(tfp, "%s\n", PREAMBLE);
    fprintf(tfp, "<!-- Creator11: %s Version %s -->\n",
		  prog, PACKAGE_VERSION);

    if (creation_date(date_buf))
	fprintf(tfp, "<!-- CreationDate: %s -->\n", date_buf);
    fprintf(tfp, "<!-- Magnification: %.3g -->\n", mag);

	//Copied fro gensvg.c
	//Calculates dimensions of the canvas
	 if (paperspec) {
		/* convert paper size from ppi to inches */
		for (pd = paperdef; pd->name != NULL; ++pd)
			if (strcasecmp(papersize, pd->name) == 0) {
			pagewidth = pd->width;
			pageheight = pd->height;
			strcpy(papersize, pd->name);	/* use the "nice" form */
			break;
			}
		if (pagewidth < 0 || pageheight < 0) {
			(void) fprintf(stderr, "Unknown paper size `%s'\n", papersize);
			exit(1);
		}
		if (landscape) {
			vh = pagewidth;
			vw = pageheight;
		} else {
			vw = pagewidth;
			vh = pageheight;
		}
    } else {
		vw = ceil((urx - llx) * 72. * mag / ppi);
		vh = ceil((ury - lly) * 72. * mag / ppi);
    }

	fputs("<!-- Splines are not handled by fig2dev as a whole, so splines appear as Polygons-->\n", tfp);
	fputs("<!-- PenColor and FillColor values are hex numbers and the patterns for FillStyles are represented as integers. For what these numbers mean consult /xfig/doc/FORMAT3.2-->\n", tfp);
	fputs("<!-- Points are displayed in fig units. To convert to cm divide by 2.22 repeating -->\n", tfp);
    fputs("<Canvas\n", tfp);
    fprintf(tfp,
	"\twidth=\"%dpt\"\n\theight=\"%dpt\"\n\tviewBox=\"%d %d %d %d\">\n",
	vw, vh, llx, lly, urx - llx , ury - lly);
}

/*
* End function for driver. Prints ending tags
*/
int
genvdx_end(void)
{
    fprintf(tfp, "</Canvas>\n");
    return 0;
}

/*
* Line Function for the driver. Takes in a line and prints out all details for the Line
*/
void
genvdx_line(F_line *l)
{
    char	chars;
    int		px,py;
    int		px2,py2,width,height,rotation;
    F_point	*p;

    if (l->type == T_PIC_BOX ) {

		switch(l->pic->subtype){
			case P_EPS:
				type_str = "EPS/PS";
				break;
			case P_XBM:
				type_str = "XBM";
				break;
			case P_XPM:
				type_str = "XPM";
				break;
			case P_GIF:
				type_str = "GIF";
				break;
			case P_JPEG:
				type_str = "JPEG";
				break;
			case P_PCX:
				type_str = "PCX";
				break;
			case P_PPM:
				type_str = "PPM";
				break;
			case P_TIF:
				type_str = "TIFF";
				break;
			case P_PNG:
				type_str = "PNG";
				break;
			default:
				type_str = "PDF";
				break;
		}


		fprintf(tfp, "\t<POLYLINE\n\t\tType=\"Picture Object\"");
		print_vdxcomments("\n\t\tComments=\"", l->comments, "\" ");
		//TODO: FIX A XFIG BUG FOR SUBTYPES
		fprintf(tfp, "\n\t\tSubtype=\"%d\"", l->pic->subtype);
		fprintf(tfp, "\n\t\tPenColor=\"%d\"", rgbColorVal(l->pen_color));
		fprintf(tfp,"\n\t\txlink:href=\"file://%s\"",l->pic->file);
		fprintf(tfp,"\n\t\tDepth=\"%d\"", l->depth);
		fprintf(tfp, "\n\t\tPoints=\"");
		chars += '\t';
		chars += '\t';
		for(p = l->points; p->next; p = p->next){
			chars += fprintf(tfp, "(%d,%d)",p->x, p->y);
			if(p->next->next){
				chars += fprintf(tfp, ",");
			}

			if(chars > VDX_LINEWIDTH){
				fputs("\n\t\t", tfp);
				chars = 0;
			}
		}
		//Calculate width and length by subtracting x and y coords of opposite corners
		int w = l->points->next->next[0].x-l->points[0].x;
		int len = l->points->next->next[0].y-l->points[0].y;
		fprintf(tfp, "\"");
		fprintf(tfp, "\n\t\tWidth=\"%d\"", w);
		fprintf(tfp, "\n\t\tLength=\"%d\"",len);
		fprintf(tfp,"/>\n");

		return;
    }

    if (l->thickness <= 0 && l->fill_style == UNFILLED &&
			!l->for_arrow && !l->back_arrow)
	return;

    /* l->type == T_BOX, T_ARC_BOX, T_POLYGON or T_POLYLINE */

    /*Determines the int value for the type and sets the string equivilent*/
    switch (l->type){
	    case T_BOX: 
		    type_str = "Box";
		    break;
	    case T_ARC_BOX:
		    type_str = "ArcBox";
		    break;
	    case T_POLYGON:
		    type_str = "Polygon";
		    break;
	    case T_POLYLINE:
		    type_str = "Polyline";
		    break;
	    default: 
		    type_str = "type";
    }

	fprintf(tfp, "\t<POLYLINE \n\t\tType=\"%s\" ",type_str);
	print_vdxcomments("\n\t\tComments=\"", l->comments, "\" ");
	fprintf(tfp, "\n\t\tWidth=\"%d\"", l->thickness/15);
    fprintf(tfp, "\n\t\tDepth=\"%d\" ",l->depth);
	fprintf(tfp, "\n\t\tPenColor=\"#%6.6x\"",rgbColorVal(l->pen_color));
	fprintf(tfp, "\n\t\tFillColor=\"#%6.6x\"",rgbColorVal(l->fill_color));
	fprintf(tfp, "\n\t\tFillStyle=\"%d\"",l->fill_style);
	fprintf(tfp, "\n\t\tLineStyle=\"%s\"",vdx_dash_string(l->style));
	fprintf(tfp, "\n\t\tDot_Dash_Length=\"%1.1f\"",l->style_val);
	if(l->type == T_ARC_BOX)
		fprintf(tfp, "\n\t\tCornerRadius=\"%d\"", l->radius/15);
	fprintf(tfp, "\n\t\tPoints=\"");
	chars += '\t';
	chars += '\t';
	F_point last;
	for(p = l->points; p->next; p = p->next){
		chars += fprintf(tfp, "(%d,%d)",p->x, p->y);
		if(p->next->next){
			chars += fprintf(tfp, ",");
		}else{
			last = p->next[0];
		}

		if(chars > VDX_LINEWIDTH){
			fputs("\n\t\t", tfp);
			chars = 0;
		}
	}
	chars += fprintf(tfp, "(%d,%d)", p->x, p->y);
	fprintf(tfp, "\"");
	//Print Arrows info here
	if(l->for_arrow || l->back_arrow){
		//If there are any arrow objects to be added, close <ARC with >
		fprintf(tfp,">\n");
		if(l->for_arrow){
			fputs("\n\t\t<FrontArrow", tfp);
			fprintf(tfp, "\n\t\t\tType=\"%d\"\n\t\t\tStyle=\"%d\"", l->for_arrow->type, l->for_arrow->style);
			fprintf(tfp, "\n\t\t\tThickness=\"%1.1f\"", l->for_arrow->thickness/15);
			fprintf(tfp, "\n\t\t\tWidth=\"%1.1f\"", l->for_arrow->wid/15);
			fprintf(tfp, "\n\t\t\tLength=\"%1.1f\"", l->for_arrow->ht/15);
			fprintf(tfp, "\n\t\t\tPoint=\"(%d,%d)\"", last.x, last.y);
			fputs("/>\n", tfp);
		}

		if(l->back_arrow){
			fputs("\n\t\t<BackArrow", tfp);
			fprintf(tfp, "\n\t\t\tType=\"%d\"\n\t\t\tStyle=\"%d\"", l->back_arrow->type, l->back_arrow->style);
			fprintf(tfp, "\n\t\t\tThickness=\"%1.1f\"", l->back_arrow->thickness/15);
			fprintf(tfp, "\n\t\t\tWidth=\"%1.1f\"", l->back_arrow->wid/15);
			fprintf(tfp, "\n\t\t\tLength=\"%1.1f\"", l->back_arrow->ht/15);
			fprintf(tfp, "\n\t\t\tPoint=\"(%d,%d)\"", l->points[0].x, l->points[0].y);
			fputs("/>\n", tfp);
		}
		fputs("\t</POLYLINE>\n", tfp);
	}else{
		fputs("/>\n",tfp);
	}
}

/*
* Spline function for the driver. Fig2dev doesn't actually use splines so splines are represented as Polygons
* Edited for completeness and in case splines are at one point called
*/
void
genvdx_spline( /* not used by fig2dev */
	F_spline *s)
{
	char chars;
    F_point *p;
	
    switch(s->type){
	case T_OPEN_APPROX:
		type_str = "Open approximated spline";
		break;
	case T_CLOSED_APPROX:
		type_str = "Closed approximated spline";
		break;
	case T_OPEN_INTERPOLATED:
		type_str = "Open interpolated spline";
		break;
	case T_CLOSED_INTERPOLATED:
		type_str = "Closed interpolated spline";
		break;

	//TODO:Figure out Xsplines
	// case T_OPEN_XSPLINE:
	// 	type_str = ""
	// T_CLOSED_XSPLINE

    }

    fprintf(tfp, "\t<SPLINE\n\tType=\"%s\"",type_str);
	print_vdxcomments("\n\tComments=\"", s->comments, "\" ");
    fprintf(tfp, "\n\t\tDepth=\"%d\" ",s->depth);
	fprintf(tfp, "\n\t\tWidth=\"%d\"", s->thickness/15);
	fprintf(tfp, "\n\t\tPenColor=\"#%6.6x\"",rgbColorVal(s->pen_color));
	fprintf(tfp, "\n\t\tFillColor=\"#%6.6x\"",rgbColorVal(s->fill_color));
	fprintf(tfp, "\n\t\tFillStyle=\"%d\"", s->fill_style);
	fprintf(tfp, "\n\t\tLineStyle=\"%s\"",vdx_dash_string(s->style));
	fprintf(tfp, "\n\t\tDot_Dash_Length=\"%1.1f\"",s->style_val);
	fprintf(tfp, "\n\t\tPoints=\"");
	F_point last;
	chars += '\t';
	chars += '\t';
	for(p = s->points; p->next; p = p->next){
		chars += fprintf(tfp, "(%d,%d)",p->x, p->y);
		if(p->next->next){
			chars += fprintf(tfp, ",");
		}else{
			last = p->next[0];
		}

		if(chars > VDX_LINEWIDTH){
			fputs("\n\t\t", tfp);
			chars = 0;
		}
	}
	chars += fprintf(tfp, "(%d,%d)", p->x, p->y);
	fprintf(tfp, "\"");
	
	//Print Arrows info here
	if(s->for_arrow || s->back_arrow){
		//If there are any arrow objects to be added, close <ARC with >
		fprintf(tfp,">\n");
		if(s->for_arrow){
			fputs("\n\t\t<FrontArrow", tfp);
			fprintf(tfp, "\n\t\t\tType=\"%d\"\n\t\t\tStyle=\"%d\"", s->for_arrow->type, s->for_arrow->style);
			fprintf(tfp, "\n\t\t\tThickness=\"%1.1f\"", s->for_arrow->thickness/15);
			fprintf(tfp, "\n\t\t\tWidth=\"%1.1f\"", s->for_arrow->wid/15);
			fprintf(tfp, "\n\t\t\tLength=\"%1.1f\"", s->for_arrow->ht/15);
			fprintf(tfp, "\n\t\t\tPoint=\"(%d,%d)\"", last.x, last.y);
			fputs("/>\n", tfp);
		}

		if(s->back_arrow){
			fputs("\n\t\t<BackArrow", tfp);
			fprintf(tfp, "\n\t\t\tType=\"%d\"\n\t\t\tStyle=\"%d\"", s->back_arrow->type, s->back_arrow->style);
			fprintf(tfp, "\n\t\t\tThickness=\"%1.1f\"", s->back_arrow->thickness/15);
			fprintf(tfp, "\n\t\t\tWidth=\"%1.1f\"", s->back_arrow->wid/15);
			fprintf(tfp, "\n\t\t\tLength=\"%1.1f\"", s->back_arrow->ht/15);
			fprintf(tfp, "\n\t\t\tPoint=\"(%d,%d)\"", s->points[0].x, s->points[0].y);
			fputs("/>\n", tfp);
		}
		fputs("\t</SPLINE>\n", tfp);
	}else{
		fputs("/>\n",tfp);
	}
}

/*
* Arc function for driver. Takes in an arc and prints out all of its details.
*/
void
genvdx_arc(F_arc *a)
{
    bool    has_clip = false;
    double  radius;
    double  x, y, angle, dx, dy;
    F_pos   forw1, forw2, back1, back2;

    if (a->fill_style == UNFILLED && a->thickness <= 0 &&
	    !a->for_arrow && !a->back_arrow)
	return;

	switch(a->type){
		case T_OPEN_ARC:
			type_str = "Open";
			break;
		case T_PIE_WEDGE_ARC:
			type_str = "Pie-Wedge";
			break;
	}

    fprintf(tfp, "\t<ARC \n\t\tType=\"Arc Drawing: Specified by 3 Points\"");
    print_vdxcomments("\n\t\tComments=\"", a->comments, "\" ");
	fprintf(tfp, "\n\t\tArcType=\"%s\"", type_str);
	fprintf(tfp, "\n\t\tWidth=\"%d\"", a->thickness/15);
   	fprintf(tfp, "\n\t\tDepth=\"%d\" ",a->depth);
	fprintf(tfp, "\n\t\tPenColor=\"#%6.6x\"",rgbColorVal(a->pen_color));
	fprintf(tfp, "\n\t\tFillColor=\"#%6.6x\"",rgbColorVal(a->fill_color));
	fprintf(tfp, "\n\t\tFillStyle=\"%d\"", a->fill_style);
	fprintf(tfp, "\n\t\tLineStyle=\"%s\"",vdx_dash_string(a->style));
	fprintf(tfp, "\n\t\tDot_Dash_Length=\"%1.1f\"",a->style_val);
	fprintf(tfp, "\n\t\tPoints=\"");
	for(int i = 0; i < 3; i++){
		fprintf(tfp, "(%d,%d)",a->point[i].x, a->point[i].y);
		if(i<2){
			fprintf(tfp, ",");
		}
	}
	fprintf(tfp, "\"");
	fprintf(tfp, "\n\t\tCenter=\"(%2.2f,%2.2f)\"", a->center.x, a->center.y);

    dx = a->point[0].x - a->center.x;
    dy = a->point[0].y - a->center.y;
    radius = sqrt(dx * dx + dy * dy);
    fprintf(tfp, "\n\t\tRadius=\"%f\" ",radius);

    x = (a->point[0].x-a->center.x) * (a->point[2].x-a->center.x) +
		(a->point[0].y-a->center.y) * (a->point[2].y-a->center.y);
    y = (a->point[0].x-a->center.x) * (a->point[2].y-a->center.y) -
		(a->point[0].y-a->center.y) * (a->point[2].x-a->center.x);

    if (x == 0.0 && y == 0.0)
		angle=0.0;
    else
		angle = atan2(y,x);
    if (angle < 0.0) angle += 2.*M_PI;
    	angle *= 180./M_PI;
    if (a->direction == 1)
		angle = 360. - angle;
    fprintf(tfp, "\n\t\tAngle=\"%f\"",angle); 
	
	//Print Arrows info here
	if(a->for_arrow || a->back_arrow){
		//If there are any arrow objects to be added, close <ARC with >
		fprintf(tfp,">\n");
		if(a->for_arrow){
			fputs("\n\t\t<FrontArrow", tfp);
			fprintf(tfp, "\n\t\t\tType=\"%d\"\n\t\t\tStyle=\"%d\"", a->for_arrow->type, a->for_arrow->style);
			fprintf(tfp, "\n\t\t\tThickness=\"%1.1f\"", a->for_arrow->thickness/15);
			fprintf(tfp, "\n\t\t\tWidth=\"%1.1f\"", a->for_arrow->wid/15);
			fprintf(tfp, "\n\t\t\tLength=\"%1.1f\"", a->for_arrow->ht/15);
			fprintf(tfp, "\n\t\t\tPoint=\"(%d,%d)\"", a->point[2].x, a->point[2].y);
			fputs("/>\n", tfp);
		}

		if(a->back_arrow){
			fputs("\n\t\t<BackArrow", tfp);
			fprintf(tfp, "\n\t\t\tType=\"%d\"\n\t\t\tStyle=\"%d\"", a->back_arrow->type, a->back_arrow->style);
			fprintf(tfp, "\n\t\t\tThickness=\"%1.1f\"", a->back_arrow->thickness/15);
			fprintf(tfp, "\n\t\t\tWidth=\"%1.1f\"", a->back_arrow->wid/15);
			fprintf(tfp, "\n\t\t\tLength=\"%1.1f\"", a->back_arrow->ht/15);
			fprintf(tfp, "\n\t\t\tPoint=\"(%d,%d)\"", a->point[0].x, a->point[0].y);
			fputs("/>\n", tfp);
		}
		fputs("\t</ARC>\n", tfp);
	}else{
		fputs("/>\n",tfp);
	}
}

/*
* Ellipse/Circle function for driver. Takes in an ellipse (circles are of the same "object" with a different type)
* Prints all data for the ellipse/circle to the output file.
*/ 
void
genvdx_ellipse(F_ellipse *e)
{
    char *header;

	switch(e->type){
		case T_ELLIPSE_BY_DIA:
			type_str = "Ellipse specified by diameter";
			header = "ELLIPSE";
			break;
		case T_ELLIPSE_BY_RAD:
			type_str = "Ellipse specified by radii";
			header = "ELLIPSE";
			break;
		case T_CIRCLE_BY_DIA:
			type_str = "Circle specified by diameter";
			header = "CIRCLE";
			break;
		case T_CIRCLE_BY_RAD:
			type_str = "Circle specified by radius";
			header = "CIRCLE";
			break;
	}

	fprintf(tfp, "\t<%s\n\t\tType=\"%s\" ",header,type_str);
	print_vdxcomments("\n\t\tComments=\"", e->comments, "\" ");
	fprintf(tfp, "\n\t\tWidth=\"%d\"", e->thickness/15);
    fprintf(tfp, "\n\t\tDepth=\"%d\" ",e->depth);
	fprintf(tfp, "\n\t\tPenColor=\"#%6.6x\"",rgbColorVal(e->pen_color));
	fprintf(tfp, "\n\t\tFillColor=\"#%6.6x\"",rgbColorVal(e->fill_color));
	fprintf(tfp, "\n\t\tFillStyle=\"%d\"", e->fill_style);
	fprintf(tfp, "\n\t\tLineStyle=\"%s\"",vdx_dash_string(e->style));
	fprintf(tfp, "\n\t\tDot_Dash_Length=\"%1.1f\"",e->style_val);
	fprintf(tfp, "\n\t\tAngle=\"%2.2f\"", (e->angle * (180/3.14159)));
	fprintf(tfp, "\n\t\tCenter=\"(%2.2d,%2.2d)\"", e->center.x, e->center.y);
	if(e->type == T_ELLIPSE_BY_DIA || e->type == T_ELLIPSE_BY_RAD)
		fprintf(tfp, "\n\t\tRadii=\"x:%2.2d, y:%2.2d\"", e->radiuses.x, e->radiuses.y);
	if(e->type == T_CIRCLE_BY_DIA || e->type == T_CIRCLE_BY_RAD)
		fprintf(tfp, "\n\t\tRadius=\"%2.2d\"", e->radiuses.x);
	fprintf(tfp,"/>\n");
}

/*
* Text function for the driver. Takes in a text object and prints all of its details to the output file.
*/
void
genvdx_text(F_text *t)
{
	unsigned char *cp;
	int ch;
	const char *anchor[3] = { "start", "middle", "end" };
	const char *family[9] = { "Times", "AvantGarde",
		"Bookman", "Courier", "Helvetica", "Helvetica Narrow",
		"New Century Schoolbook", "Palatino", "Times,Symbol"
	};
	int x = t->base_x ;
	int y = t->base_y ;
#ifdef NOSUPER
	int dy = 0;
#endif

	fprintf(tfp, "\t<Text\n\t\tType=\"Text\"");
	print_vdxcomments("\n\t\tComments=\"", t->comments, "\"");
	fprintf(tfp, "\n\t\tSize=\"%d\"", (int)ceil(t->size));
	fprintf(tfp, "\n\t\tPenColor=\"#%6.6x\"", rgbColorVal(t->color));
	fprintf(tfp, "\n\t\tDepth=\"%d\" ",t->depth);
	fprintf(tfp, "\n\t\tAngle=\"%2.2f\"", (t->angle * (180/3.14159)));
	// Dev choice to omit flags and justification
	// fprintf(tfp, "\n\t\tFlags=\"%d\"", t->flags);
	fprintf(tfp, "\n\t\tx=\"%d\"", x);
	fprintf(tfp, "\n\t\ty=\"%d\"", y);
	fprintf(tfp, "\n\t\tFontFamily=\"%s\"",family[t -> font / 4]);
	fprintf(tfp, "\n\t\tText=\"%s\"",t->cstring);
	fprintf(tfp,"/>\n");
}

/*
* Function that takes in a LineStyle's integer value and returns the matching string
*/
static const char*
vdx_dash_string(int style)
{
	switch(style) {
	case 0:
	default:
		return "Solid";
	case 1:
		return "Dashed";
	case 2:
		return "Dashed-Small";
	case 3:
		return "Dashed-Dot";
	case 4:
		return "Dashed-Dotx2";
	case 5:
		return "Dashed-Dotx3";
	}
}

/* driver defs */

struct driver dev_vdx = {
	genvdx_option,
	genvdx_start,
	gendev_nogrid,
	genvdx_arc,
	genvdx_ellipse,
	genvdx_line,
	genvdx_spline,
	genvdx_text,
	genvdx_end,
	INCLUDE_TEXT
};