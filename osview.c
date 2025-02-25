/* x11_osview: a clone of gr_osview */
/* Copyright (C) 2025, Ellie Neills */
/* This program is a work in progress */

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
/* #include <X11/Xos.h> */
#include <string.h>

#define RED 0xFF0000
#define GREEN 0x00FF00
#define BLUE 0x0000FF
#define YELLOW 0xFFFF00
#define CYAN 0x00FFFF
#define GRAY 0x808080

Display *disp;
Window win;
XEvent event;
GC gc;
unsigned short win_w, win_h, bar_x, bar_y, bar_w, bar_h, pos;
float us, sy, id, wa, in;

int main(int argc, char *argv[]) {

    int colours[] = { 0, 0, BLUE, RED, YELLOW, CYAN, GREEN };
    char *strings[] = { "", "CPU Usage:", "user", "sys", "intr", "wait",
        "idle" };
    short resize = 1;
    unsigned short frames = 0;
	XSizeHints *size_hints;
    XFontStruct *font;
	XTextProperty title;
	XGCValues vals;
    XSetWindowAttributes winattr;
	void drawbar(void);
	unsigned int user, sys, idle, wait, irq, total = 0;
	unsigned int old_sys, old_user, old_idle, old_wait, old_irq;
    char hostname[20];
    char *host = hostname;
	FILE *fp;
	int c, i, j;
	char stats[10][20];

    gethostname(&hostname, 19);

	if(!(disp = XOpenDisplay(0))) {		/* Use $DISPLAY */
		fprintf(stderr, "Error: Can't connect to X server\n" 
			"Have you enabled X forwarding? (try 'ssh -X')\n");
		return(1);
	}

	win_w = DisplayHeight(disp, 0) / 1.5;
	win_h = win_w / 8;	/* 1:8 aspect ratio */

	win = XCreateSimpleWindow(disp, RootWindow(disp, 0), 0, 0, win_w,
		win_h, 4, 0, 0x808080);	/* Use screen 0 of the display */

	size_hints = XAllocSizeHints();
	size_hints->flags = PMinSize | PResizeInc;
	size_hints->min_width = 350;
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

		for(i = 0; i < 11; i++){
			j = 0;
			while((c = getc(fp)) != ' ') {
				stats[i][j++] = c;
			}
			stats[i][j] = '\0';
		}

		user = atoi(stats[2]) + atoi(stats[3]);
		sys = atoi(stats[4]);
		idle = atoi(stats[5]);
		wait = atoi(stats[6]);
		irq = atoi(stats[8]);

		total = (user - old_user) + (sys - old_sys) + (idle - old_idle) +
			(irq - old_irq) + (wait - old_wait);

		us = (user - old_user) / (float) total;
		sy = (sys - old_sys) / (float) total;
		id = (idle - old_idle) / (float) total;
		wa = (wait - old_wait) / (float) total;
		in = (irq - old_irq) / (float) total;

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
                printf("%dx%d\n", win_w, win_h);
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
                printf("resized\n");
            }
            resize = 0;
            printf("redrawing outline\n");
            XClearWindow(disp, win);	/* avoids redraw glitch */
            /* Draw outlines */
            XDrawRectangle(disp, win, DefaultGC(disp, 0),
                bar_x, bar_y, bar_w, bar_h);
            XFillRectangle(disp, win, DefaultGC(disp, 0),
                bar_x, bar_y + (bar_h * 0.9), bar_w, (bar_h * 0.1) + 1);

            /* Draw markers every 10 percent */
            XSetForeground(disp, gc, 0x808080);
            for(pos = bar_x + 2; pos < (bar_x + bar_w); pos += (bar_w * 0.1)){
                XDrawLine(disp, win, gc, pos,
                    bar_y + (bar_h * 0.9), pos, bar_y + bar_h);

            }
            XSetForeground(disp, gc, 0); /* Black */
            pos = 20;

            for(i = 1; i <= 6; i++) {
                XSetForeground(disp, gc, colours[i]);
                XDrawString(disp, win, gc, pos += (font->ascent *
                    strlen(strings[i-1]) * 0.7), font->ascent, strings[i],
                    strlen(strings[i]));
            }
        }

		if(frames > 1)	/* Skip first 2 frames until sane values loaded */
			drawbar();

		usleep(500000); /* This is bad practice, but it's simple */

		/* save previous values */
		old_sys = sys;
		old_idle = idle;
		old_user = user;
		old_wait = wait;
		old_irq = irq;
	}
}

void drawbar(void) {

		pos = bar_x + 1;	/* Reset position */
		XSetForeground(disp, gc, BLUE);
		XFillRectangle(disp, win, gc, pos, bar_y + 1,	/* user */
			us * bar_w, (bar_h * 0.9) - 1);

		pos = pos + (us * bar_w);
		XSetForeground(disp, gc, RED);
		XFillRectangle(disp, win, gc, pos, bar_y + 1,	/* system */
			sy * bar_w, (bar_h * 0.9) - 1);

		pos = pos + (sy * bar_w);
		XSetForeground(disp, gc, YELLOW);
		XFillRectangle(disp, win, gc, pos, bar_y + 1,	/* irq */
			in * bar_w, (bar_h * 0.9) - 1);

		pos = pos + (in * bar_w);
		XSetForeground(disp, gc, CYAN);
		XFillRectangle(disp, win, gc, pos, bar_y + 1,	/* iowait */
			wa * bar_w, (bar_h * 0.9) - 1);

		pos = pos + (wa * bar_w);
		XSetForeground(disp, gc, GREEN);
		XFillRectangle(disp, win, gc, pos, bar_y + 1,	/* idle */
			bar_w - pos + 20, (bar_h * 0.9) - 1);

		XFlush(disp);
}
