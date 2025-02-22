/* x11_osview: a clone of gr_osview */
/* Copyright (C) 2025, Ellie Neills */
/* This program is a work in progress */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
/* #include <X11/Xos.h> */

#include <stdio.h>
/* #include <stdlib.h> */

int main(int argc, char *argv[]) {

	Display *disp;
	Window win;
	XEvent event;
	GC gc;
	XGCValues vals;
	unsigned short win_w, win_h, bar_x, bar_y, bar_w, bar_h, pos;
	float us = 0, sy = 0, id = 0;
	int i, j, c;
	char *win_name = "x11_osview";
	XSizeHints *size_hints;
	XTextProperty title;

	if(!(disp = XOpenDisplay(0))) {		/* Use $DISPLAY */
		fprintf(stderr, "Error: Can't connect to X server\n" 
			"Have you enabled X forwarding? (try 'ssh -X')\n");
		return(1);
	}

	win_w = DisplayHeight(disp, 0) / 1.5;
	win_h = win_w / 4;	/* 1:4 aspect ratio */

	win = XCreateSimpleWindow(disp, RootWindow(disp, 0), 0, 0, win_w,
		win_h, 4, 0, 0x808080);	/* Use screen 0 of the display */

	size_hints = XAllocSizeHints();
	size_hints->flags = PMinSize | PResizeInc;
	size_hints->min_width = 200;
	size_hints->min_height = 50;
	size_hints->width_inc = 10;
	XSetWMNormalHints(disp, win, size_hints);

	XStringListToTextProperty(&win_name, 1, &title);
	XSetWMName(disp, win, &title);

	/* Program is output only, so no input events needed */
	XSelectInput(disp, win, ExposureMask | StructureNotifyMask);

	XMapWindow(disp, win);

	do {
		XNextEvent(disp, &event);
	} while (event.type != Expose);		/* Wait for first Expose event */

	/* NOW we can draw into the window */

	bar_x = 20;
	bar_y = win_h * 0.2;
	bar_w = win_w - 40;
	bar_h = win_h * 0.7;
	gc = XCreateGC(disp, win, 0, &vals);
	XSetLineAttributes(disp, gc, 2, 0, 0, 0);

	while(1) {

		/* Check for window resize (and other things) */

		while(XCheckMaskEvent(disp, StructureNotifyMask, &event)) {
			win_w = event.xconfigure.width;	/* assume window was resized */
			win_h = event.xconfigure.height;
			bar_y = win_h * 0.2;
			bar_w = win_w - 40;
			bar_h = win_h * 0.7;
			printf("%dx%d\n", win_w, win_h);
			XClearWindow(disp, win);	/* avoids redraw glitch */
		}

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

		pos = bar_x + 1;	/* Reset position */
		XSetForeground(disp, gc, 0x0000FF);	/* Blue */
		XFillRectangle(disp, win, gc, pos, bar_y + 1,
			us * bar_w, (bar_h * 0.9) - 1);

		pos = pos + (us * bar_w);
		XSetForeground(disp, gc, 0xFF0000);	/* Red */
		XFillRectangle(disp, win, gc, pos, bar_y + 1,
			sy * bar_w, (bar_h * 0.9) - 1);

		pos = pos + (sy * bar_w);
		XSetForeground(disp, gc, 0x00FF00);	/* Green */
		XFillRectangle(disp, win, gc, pos, bar_y + 1,
			bar_w - pos + 20, (bar_h * 0.9) - 1);

		XFlush(disp);
		usleep(50000); /* This is bad practice, but it's simple */

		printf("Queue: %d\n", XEventsQueued(disp, win));

		FILE *fp;
		fp = fopen("/proc/stat", "r");

		int c, i, j;
		char stats[10][20];
		int user, sys, idle, wait, irq;
		float total = 0;
		int old_sys, old_user, old_idle;
		float old_total;

		for(i = 0; i < 11; i++){
			j = 0;
			while((c = getc(fp)) != ' ') {
				stats[i][j++] = c;
			}
			stats[i][j] = '\0';
		}
		for(i = 0; i < 11; i++)
			printf("%d: %s\n", i, stats[i]);

		user = atoi(stats[2]) + atoi(stats[3]);
		sys = atoi(stats[4]);
		idle = atoi(stats[5]);
		wait = atoi(stats[6]);
		irq = atoi(stats[8]);
		printf("\nuser: %d\n", user - old_user);
		printf("sys : %d\n", sys - old_sys);
		printf("idle: %d\n", idle - old_idle);
		/* printf("wait: %d\n", wait);
		printf("irq: %d\n", irq); */
		total = user + sys + idle + wait + irq;
		/* printf("tot: %f\n", total);
		printf("old: %f\n", old_total); */
		printf("dif: %f\n", total - old_total);

		us = (user - old_user) / 448.0;
		sy = (sys - old_sys) / 448.0;
		id = (idle - old_idle) / 448.0;
		printf("us: %f\n", us);
		printf("sy: %f\n", sy);
		printf("id: %f\n", id);

		old_total = total;
		old_sys = sys;
		old_idle = idle;
		old_user = user;

		fclose(fp);
		usleep(1000000);
	}
}
