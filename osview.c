/* x11_osview: a clone of gr_osview */
/* Copyright (C) 2025, Ellie Neills */

#define _DEFAULT_SOURCE
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define GREEN 6
#define GRAY 7

Display *disp;
Window win;
XEvent event;
GC gc;
unsigned short win_w, win_h, bar_x, bar_y, bar_w, bar_h, pos;
char *colours[] = { "black", "blue", "red", "yellow", "cyan", "magenta",
    "green", "gray" };
float values[6];
short i;
XColor screen_col[8], exact_col;
XColor temp_col;

int main(int argc, char *argv[]) {

    char *strings[] = { "CPU Usage: ", "user ", "sys ", "intr ",
        "wait ", "vm ", "idle " };
	XSizeHints *size_hints;
    XWMHints *wm_hints;
    XFontStruct *font;
	XTextProperty title;
	XGCValues vals;
    XSetWindowAttributes winattr;
	void drawbar(void);
    unsigned int data[6];
    unsigned int old_data[6];
    unsigned int total = 0;
    unsigned int frames = 0;
    char hostname[20];
    char *host = hostname;
	char stats[10][20];
	FILE *fp;
	int c;
    short j, resize = 1;

	if(!(disp = XOpenDisplay(0))) {		/* Use $DISPLAY */
		fprintf(stderr, "Error: Can't connect to X server\n" 
			"Have you enabled X forwarding? (try 'ssh -X')\n");
		return(1);
	}

    gethostname(hostname, 19);

	win_w = DisplayHeight(disp, 0) / 1.5;
	win_h = win_w / 8;	/* 1:8 aspect ratio */

    /* Store the colours _locally_ */
    for(i = 0; i < 8; i++)
        XAllocNamedColor(disp, DefaultColormap(disp, 0), colours[i],
            &screen_col[i], &exact_col);

	win = XCreateSimpleWindow(disp, RootWindow(disp, 0), 0, 0, win_w,
		win_h, 4, 0, screen_col[GRAY].pixel); /* Use screen 0 of display */

	size_hints = XAllocSizeHints();
	size_hints->flags = PMinSize | PResizeInc;
	size_hints->min_width = 320;
	size_hints->min_height = 50;
	size_hints->width_inc = 10;
	XSetWMNormalHints(disp, win, size_hints);

	XStringListToTextProperty(&host, 1, &title);
	XSetWMName(disp, win, &title);

	/* Program is output only, so no input events needed */
	XSelectInput(disp, win, ExposureMask | StructureNotifyMask);

    winattr.backing_store = Always; /* Modern systems have _plenty_ RAM */
    /* X Server is not guaranteed to honour this request! */
    XChangeWindowAttributes(disp, win, CWBackingStore, &winattr);

    /* Don't steal focus - WM may ignore this */
    wm_hints = XAllocWMHints();
    wm_hints->flags = InputHint;
    wm_hints->input = False;
    XSetWMHints(disp, win, wm_hints);

	XMapWindow(disp, win);

	do {
		XNextEvent(disp, &event);
	} while (event.type != Expose);		/* Wait for first Expose event */

	/* NOW we can draw into the window */
	bar_x = 20;
	bar_w = win_w - 40;
	bar_h = win_h - 40;
	gc = XCreateGC(disp, win, 0, &vals);
	XSetLineAttributes(disp, gc, 2, 0, 0, 0);

	while(1) {

		frames++;

		fp = fopen("/proc/stat", "r");

		for(i = 0; i < 12; i++){
			j = 0;
			while((c = getc(fp)) != ' ')
				stats[i][j++] = c;
			stats[i][j] = '\0';
		}

		data[0] = atoi(stats[2]) + atoi(stats[3]); /* user */
		data[1] = atoi(stats[4]); /* sys */
		data[2] = atoi(stats[7]) + atoi(stats[8]); /* irqs */
		data[3] = atoi(stats[6]); /* iowait */
        data[4] = atoi(stats[10]) + atoi(stats[11]); /* guest */
		data[5] = atoi(stats[5]); /* idle */

        total = 0;
        for(i = 0; i < 6; i++)
            total += (data[i] - old_data[i]);

        for(i = 0; i < 6; i++)
            values[i] = (data[i] - old_data[i]) / (float) total;

		fclose(fp);

		/* Check for window resize (and other things) */
        /* CheckMaskEvent is used instead of CheckTypedEvent so that the */
        /* event queue is cleared. Use of usleep() makes that a problem */

		while(XCheckMaskEvent(disp, StructureNotifyMask, &event) &&
        event.xconfigure.width) {
            /* Only change if width >0. Avoids desktop switch bug */
            if((win_w != event.xconfigure.width) ||
            (win_h != event.xconfigure.height)) {
                win_w = event.xconfigure.width;
                win_h = event.xconfigure.height;
                bar_w = win_w - 40;
                bar_h = win_h - 40;
                resize = 1;
            }
		}

        /* Only re-draw text & outlines on expose/resize */
        /* This reduces network traffic to the X server */
        /* resize is intially 1, so this will execute first time through */
        while(XCheckTypedEvent(disp, Expose, &event) || resize) {
            
            if(resize) {
            if(win_h < 70)
                    font = XLoadQueryFont(disp, "-*-times-*-i-*-140-100-*");
                else if(win_h < 120)
                    font = XLoadQueryFont(disp, "-*-times-*-i-*-180-100-*");
                else
                    font = XLoadQueryFont(disp, "-*-times-*-i-*-240-100-*");
                XSetFont(disp, gc, font->fid);
                bar_y = 1.1 * (font->ascent + font->descent);

                bar_h = win_h - (1.8 * (font->ascent));
            }
            resize = 0;
            XClearWindow(disp, win);	/* avoids redraw glitch */
            /* Draw outlines */
            XDrawRectangle(disp, win, DefaultGC(disp, 0),
                bar_x, bar_y, bar_w, bar_h);
            XFillRectangle(disp, win, DefaultGC(disp, 0),
                bar_x, bar_y + (bar_h * 0.9), bar_w, (bar_h * 0.1) + 1);

            /* Draw markers every 10 percent */
            XSetForeground(disp, gc, screen_col[GRAY].pixel);
            for(pos = bar_x + 2; pos < (bar_x + bar_w); pos += (bar_w * 0.1)){
                XDrawLine(disp, win, gc, pos,
                    bar_y + (bar_h * 0.9), pos, bar_y + bar_h);
            }

            /* Draw text */
            pos = 20;
            for(i = 0; i < 7; i++) {
                XSetForeground(disp, gc, screen_col[i].pixel);
                XDrawString(disp, win, gc, pos , font->ascent, strings[i],
                    strlen(strings[i]));
                pos += XTextWidth(font, strings[i], strlen(strings[i]));
            }
        }

		if(frames > 1)	/* Skip first 2 frames until sane values loaded */
			drawbar();

		usleep(500000); /* This is bad practice, but it's simple */

		/* save previous values */
        for(i = 0; i < 6; i++) {
            old_data[i] = data[i];
        }
	}
}

void drawbar(void) {

		pos = bar_x + 1;	/* Reset position */
        for(i = 0; i < 5; i++) {
            XSetForeground(disp, gc, screen_col[i+1].pixel);
            XFillRectangle(disp, win, gc, pos, bar_y + 1,
                values[i] * bar_w, (bar_h * 0.9) - 1);
            pos = pos + (values[i] * bar_w);
        }

		XSetForeground(disp, gc, screen_col[GREEN].pixel);  /* green */
		XFillRectangle(disp, win, gc, pos, bar_y + 1,	/* idle */
			bar_w - pos + 20, (bar_h * 0.9) - 1);

		XFlush(disp);
}
