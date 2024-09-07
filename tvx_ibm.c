/* ------------------------- tvx_ibm.c -------------------------- */
/*
	This is the interface to the PC's ROM BIOS.  It could
	be bypassed by using the ANSI.SYS driver, but that would
	be SLOW!  This version is cii-c86 dependent, mainly using
	the routine sysint, for example:
		  sysint(0x10, &rin, &rout);
		int. number^     ^registers in and out
	If possible, these calls should be replaced with direct
	assembly language calls.  The overhead of sysint is high
	and noticeably slows down screen update.
*/

#define FALSE 0
#define TRUE 1
/*	Interface to IBM ROM BIOS INT10 screen control.  Following
	control codes are defined:

	^@,0	^A,1	^B,2	^C,3
	other, erslin, erseos, inslin
	^D,4	^E,5	^F,6	^G,7
	undlon, undloff, dellin, bell
	^H,8	^I,9	^J,10	^K,11
	backsp,	tab, linefeed, boldon
	^L,12	^M,13	^N,14	^O,15
	boldoff, enter, reversoff, blinkoff
	^P,16	^Q,17	^R,18	^S,19, ^T,20
	reverson, blinkon, setxy, cursor1, initcursor
 */
#define m_normal 07	/* white on black */
#define m_underline 01	/* normal, underlined */
#define m_reverse 0x70
#define m_dim 0xF7	/* dim display */
#define m_bright 0x08	/* bright display */
#define m_blink	0x80	/* blink for errors */
#define m_noblink 0x7F


/* ============================ dispch ============================== */
  dispch(chin)
  int chin;
  {

    static int ch;
    struct regval
      {
	unsigned int ax;
	unsigned int bx;
	unsigned int cx;
	unsigned int dx;
	unsigned int si;
	unsigned int di;
	unsigned int ds;
	unsigned int es;
      };
    struct regval rin, rout;

	/* data structures for screen control */
    static int ich;
    static int maxcol = 79;		/*  max col, counting from 0 */
    static int savechar;
    static int initflg = FALSE;
    static int curpage = 0;		/* current display page (internal) */
    static int curmode = 7;		/* white on black */
    static int curcol;			/* col and row, preserve this order */
    static int currow;			/* so can load in one instruction */
    static int curstate = 0;		/* 0: accepting chars
					   1: waiting for row
					   2: waiting for col */
    static int rowpend = 0;		/* to save pending row */
    static int initcursor;		/* initial cursor */
    static int altcursor;
    static int color;		/* mono or color */

    ch = chin & 0xff;

    if (!initflg)
      {
	rin.ax = 0x0F00;		/* ah is 15, get video state */
	sysint(0x10, &rin, &rout);	/* int 10h */
	maxcol = ((rout.ax >> 8) & 0xff) - 1;	/* make relative value (0-79) */
  	curpage = (rout.bx >> 8) & 0xff;	/* the active page */
	rin.ax = 0x0300;		/* read cursor position */
	sysint(0x10, &rin, &rout);	/* int 10h */
	curcol = rout.dx & 0xff;	/* low order is col */
	currow = (rout.dx >> 8) & 0xff;
	sysint(0x11,&rin,&rout);	/* get system configuration */
	color = (rout.ax & 0x30) != 0x30;
	if (!color)
	  {
	    initcursor = 0x0c0d;	/* 12, 13 - avoids PC ROM bug! */
	    altcursor = 0x060d;		/* 6,13 for insert */
	  }
	else
	  {
	    initcursor = 0x0607;	/* current cursor mode, color */
	    altcursor = 0x0307;		/* half block */
	  }
	initflg = TRUE;
      }


    if (curstate != 0)		/* waiting for row or col? */
      {
	if (curstate == 1)
	  {
		/* in state 1, so this is row */
	    rowpend = ch;		/* save pending row */
	    curstate = 2;		/* now in wait state */
	    return;
	  }
	else		/* waiting for column */
	  {
	    ich = ch - ' ';	/* convert to absolute */
	    if (ich > maxcol)
		ich = maxcol;
	    curcol = ich;	/* remember column */
	    rowpend -= ' ';	/* convert row */
	    if (rowpend > 24)
		rowpend = 24;
	    currow = rowpend;
	    rin.dx = (currow << 8) | curcol;
	    rin.bx = curpage << 8;
	    rin.ax = 0x200;	/* 2 => set cursor */
	    sysint(0x10, &rin, &rout);	/* int 10h */
	    curstate = 0;
	    return;
	  }
      }
    else
      {
	if (ch >= ' ')
	    goto SHOWCHAR;	/* slight optimization */

	switch (ch)
	  {
	    case 1:		/* erase from cursor to end of line */
erslin:
		rin.cx = rin.dx = currow << 8;		/* set row */
		rin.cx |= curcol;		/* set col */
		rin.dx |= maxcol;		/* blank to max col */
		rin.ax = 0x600;	      /* scroll active page up, blank section */
		rin.bx = curmode << 8;
		sysint(0x10, &rin, &rout);	/* int 10h */
		return;

	    case 2:		/* erase from cursor to end of screen */
		/* first, earase current row */
		rin.cx = rin.dx = currow << 8;		/* set row */
		rin.cx |= curcol;		/* set col */
		rin.dx |= maxcol;		/* blank to max col */
		rin.ax = 0x600;	      /* scroll active page up, blank section */
		rin.bx = curmode << 8;
		sysint(0x10, &rin, &rout);	/* int 10h */
		if (currow >= 24)	/* on bottom row now? */
		    return;
		rin.cx = (currow + 1) << 8; 	/* next row, col 0 */
		rin.dx = 0x1800 | maxcol;
		rin.ax = 0x0600;		/* 6: scroll 0: blank */
		sysint(0x10, &rin, &rout);	/* int 10h */
		return;

	    case 3:		/* insert a blank line at cursor */
		if (currow < 24)
		  {
		    rin.cx = (currow << 8);
		    rin.dx = 0x1800 | maxcol;	/* define window to scroll */
		    rin.bx = curmode << 8;
		    rin.ax = 0x0701;		/* one line, scroll down */
		    sysint(0x10, &rin, &rout);	/* int 10h */
		  }
		curcol = 0;		/* home to line beginning */
		rin.dx = currow << 8;	/* dh = currow, dl = 0 */
		rin.bx = curpage << 8;
		rin.ax = 0x0200;	/* reset cursor position */
		sysint(0x10, &rin, &rout);	/* int 10h */
		if (currow >= 24)	/* special case */
		    goto erslin;
		return;

	    case 4:		/* underline on */
		curmode = (curmode & 0x88) | m_underline;
		return;

	    case 5:		/* underline off */
		curmode = (curmode & 0x88) | m_normal;
		return;

	    case 6:			/*  kill line cursor is on */
		rin.cx = currow << 8;	/* define window to scroll */
		rin.dx = 0x1800 | maxcol;
		rin.ax = 0x0601;		/* one line (al), scroll up (6) */
		rin.bx = curmode << 8;
		sysint(0x10, &rin, &rout);	/* int 10h */
		curcol = 0 ;		/* home to line beginning */
		rin.dx = currow << 8;
		rin.bx = curpage << 8;
		rin.ax = 0x0200;
		sysint(0x10, &rin, &rout);	/* int 10h */
		return;

	    case 7:		/* bell */
		bdos(2,ch);
		return;

	    case 8:		/* backspace */
		if (curcol <= 0)
		    return;
		--curcol;
		rin.bx = curpage << 8;
		rin.dx = (currow << 8) | curcol;
		rin.ax = 0x0200;	/* set cursor pos */
		sysint(0x10, &rin, &rout);	/* int 10h */
		return;

	    case 9:		/* tab */
		ch = ' ';
		goto SHOWCHAR;

	    case 10:		/* line feed, scroll if bottom */
		if (currow < 24)
		  {
		    rin.dx = (++currow << 8) | curcol;
		    rin.bx = curpage << 8;
		    rin.ax = 0x0200;		/* set cursor */
		    sysint(0x10, &rin, &rout);	/* int 10h */
		  }
		else
		  {
			/* need to scroll up */
		    rin.ax = 0x0601;	/* scroll up (6) 1 line (1) */
		    rin.cx = 0;		/* upper right */
		    rin.dx = 0x1800 | maxcol;
		    rin.bx = curmode << 8;
		    sysint(0x10, &rin, &rout);	/* int 10h */
		  }
		return;

	    case 11:		/* bold on */
		curmode |= m_bright;
		return;

	    case 12:		/* bold off */
		curmode &= m_dim;
		return;

	    case 13:		/* CR, include erase end of line */
		if (curcol >= maxcol)
		    goto NOBLANK;
		rin.cx = rin.dx = currow << 8;		/* set row */
		rin.cx |= curcol;		/* set col */
		rin.dx |= maxcol;		/* blank to max col */
		rin.ax = 0x0600;	/* scroll up, blank section */
		rin.bx = curmode << 8;
		sysint(0x10, &rin, &rout);	/* int 10h */
NOBLANK:
		curcol = 0;
	        rin.dx = (currow << 8);
		rin.bx = curpage << 8;
		rin.ax = 0x0200;
		sysint(0x10, &rin, &rout);	/* int 10h */
		return;

	    case 14:		/* reverse off */
		curmode = (curmode & 0x88) | m_normal ;
		return;

	    case 15:		/* blink off */
		curmode &= m_noblink;
		return;

	    case 16:		/* reverse on */
		curmode = (curmode & 0x88) | m_reverse;
		return;

	    case 17:		/* blink on */
		curmode |= m_blink;
		return;

	    case 18:		/* set xy */
		curstate = 1;
		return;

	    case 19:			/* change cursor */
		rin.ax = 0x0100;	/* set cursor type */
		rin.cx = altcursor;	/* half block */
		sysint(0x10, &rin, &rout);
		return;

	    case 20:			/* change cursor */
		rin.ax = 0x0100;	/* set cursor type */
		rin.cx = initcursor;	/* original */
		sysint(0x10, &rin, &rout);
		return;


	    default:		/* show char */
SHOWCHAR:
		if (curcol > maxcol)	/* update column */
		    return;
		rin.ax = 0x0900 | ch;		/* display char */
		rin.bx = (curpage << 8) | curmode;
		rin.cx = 1;
		sysint(0x10, &rin, &rout);	/* int 10h */
		++curcol;
		rin.dx = (currow << 8) | curcol;
		rin.ax = 0x0200;
		sysint(0x10, &rin, &rout);	/* int 10h */
		return;
	  }	/* end of switch */
      }		/* end of else */
  }
