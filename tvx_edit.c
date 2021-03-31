/* ---------------------------------------------------------------------
	This module contains the main edit routine, which is
different for each editor being emulated.  The code that is unique
to each emulation is confined to the three .ic files, tvx_lex.c, and
this module.  The remainder of the tvx files are the same for all versions.


   This version of TVX Copyright (c) 1986 by Bruce E. Wampler

   Permission is hereby granted for free, unrestricted nonprofit
   use of this software.  Please feel free to modify, distribute,
   and share this software as long as you aren't making any money
   from it.  If you want to use this code in a profit making environment,
   please contact the author for permission.

--------------------------------------------------------------------- */

#include "tvx_defs.ic"		/* note tv_defs.ic will #include stdio.h */
#include "tvx_glbl.ic"

  char clower(),cupper();


#ifdef TVX_CMDSET		/* works for both tvx_em and tvx0m_em */
/* =============================>>> EDIT   <<<============================= */
  edit()
  { /*	edit - main editing routine */

    SLOW int lexval,lexcnt,succ, lastln, itmp;
    SLOW int noteloc[10], ni, lex_def;
    SLOW char lexchr;

    static int ins_set[] =		/* allowable commands for insert */
      {
	VINSERT, VOPENLINE, VQUIT, VABORT, VFBEGIN, VGET, VYANK, 0
      };

    static int jump_set[] =	/* commands to not reset jump memory */
      {
	VJUMP, VMEMORY, VHELP, VNOTELOC, VPRINTS, 0
      };
    SLOW char fchr;		/* temp char for prefixed commands */

    startm();
    remark("Reading file...");

    rdpage();			/* read a page into the buffer */

    tvclr();			/* clear the screen */

    if (curlin >= 1)
	tvtype(curlin,tvlins);	/* type out lines */

    tvxy(1,1);			/* and rehome the cursor */
    waserr = FALSE;		/* no errors to erase yet */

    if (curlin<1)
	tverr("Buffer empty");
#ifdef SCR_BUF
    ttflush();
#endif

    lexval = UNKNOWN;		/* so can remember 1st time through */
    useprint = FALSE;		/* not to printer */
    succ=TRUE;			/* assume success initially */

    lastln = 1	;		/* remember where we were */
    for (ni = 0 ; ni < 10 ; noteloc[ni++] = 1)
	;			/* init noteloc */
    do
      {
	oldlex = lexval;		/* remember last command */
	if (! succ)
	    echof = TRUE;		/* resume echo when error */
	lex_def = lex(&lexval,&lexcnt,&lexchr,succ);	/* get command input */
	if (waserr)
	    fixend();
	waserr=FALSE;
	succ=TRUE;
	if (lexval == UNKNOWN)
	  {
	    cmderr(lexchr);
	    succ = FALSE;	/* announce failure to lex */
	  }
	else
	  {
	    if (curlin < 1)	/* make sure legal command for empty buffer */
	      {
		if (!inset(lexval,ins_set))
		  {
		    tverrb("Can't, buffer empty. Insert 1st ");
		    succ=FALSE;
		    continue;
		  }
	      }
	    if (!inset(lexval,jump_set))
		lastln=curlin;		/* let user look at help w/o changing */

	    switch (lexval)
	    {
case 1: 			/* right */
	    right(lexcnt);
	    break;
case 2: 			/* left */
	    right(-lexcnt);
	    break;
case 3: 			/* down line */
	    dwnlin(lexcnt);
	    break;
case 4: 			/* up line */
	    dwnlin(-lexcnt);
	    break;
case 5: 			/* down in column */
	    dwncol(lexcnt);
	    break;
case 6: 			/* up in column */
	    dwncol(-lexcnt);
	    break;
case 7: 			/* delete last character */
	    succ = delnxt(-lexcnt);
	    break;
case 8: 			/* delete next character */
	    succ = delnxt(lexcnt);
	    break;
case 9: 			/* insert */
	    succ = insert(lexcnt,lex_def);
	    break;
case 10:			/* kill a line */
	    killin(lexcnt);
	    break;
case 11:			/* kill rest of line */
	    krest();
	    break;
case 12:			/* kill previous part of line */
	    kprev();
	    break;
case 13:			/* move to beginning of line */
	    beglin();
	    break;
case 14:			/* move to end of the line */
	    endlin();
	    break;
case 15:			/* search for a pattern */
	    succ = search(lexcnt,TRUE);
	    break;
case 16:			/* search for next part of a pattern */
	    succ = snext(lexcnt,TRUE);
	    break;
case 17:			/* flip screen */
	    dwnlin(min(lexcnt*tvlins,nxtlin-curlin+1));
	    break;
case 18:			/* goto top of page */
	    toppag();
	    break;
case 19:			/* goto to bottom of page */
	    botpag();
	    break;
case 20:			/* goto real beginning of the file */
	    succ = fbeg();
	    break;
case 21:			/* verify */
	    verify(lexcnt);
	    break;
case 22:			/* open new line */
	    openln(lexcnt);
#ifdef TVX_EM			/* don't need insert for modeless tvx */
	    succ = insert(1,TRUE); /* go into insert mode, insert mode */
#endif
	    break;
case 23:			/* delete last thing manipulated */
	    succ = rmvlst();
	    break;
case 24:			/* save lines in move buffer */
	    succ = save(lexcnt,FALSE);
	    break;
case 25:			/* get move buffer */
	    succ = getsav();
	    break;
case 26:			/* read in next page of file */
	    wtpage(lexcnt);	/* write out the current page */
	    succ = rdpage();	/* read in the next */
	    tvclr();
	    if (succ || lexcnt < 0)
		verify(1);
	    break;
case 27:			/* append external file to save buffer */
	    succ = addfil(lexcnt);
	    break;
case 28:			/* quit */
	    tvclr();
	    remark("Exit");
	    goto lquit;
case 29:			/* search again */
	    succ = search(lexcnt,FALSE); /* FALSE => don't read search string */
	    break;
case 30:			/* execute repeat buffer again */
	    if (lexcnt != 1)
		echof=FALSE;	/* turn off echo */
	    rptcnt[rptuse] = lexcnt > 0 ? lexcnt : (-lexcnt);
	    break;
case 31:			/* print memory status, etc. */
	    memory();
	    break;
case 32:			/* change a parameter */
	    if (!grptch(&fchr))
	      {
		succ = FALSE;
		break;
	      }
	    setpar(lexcnt,fchr);
	    break;
case 33:			/* remove last and enter insert mode */
	    if ((succ = rmvlst()))
		succ = insert(1,TRUE);
	    break;
case 34:			/* unkill last line killed */
	    succ = unkill();
	    break;
case 35:			/* jump over a word */
	    wordr(lexcnt);
	    break;
case 36:			/* neg jump over word */
	    wordr(-lexcnt);
	    break;
case 37:			/* append to save buffer */
	    succ = save(lexcnt,TRUE);
	    break;
case 38:			/* print screen */
	    scrprint();
	    break;
case 39:			/* show repeat buffer + help*/
	    shoset();
	    break;
case 40:			/* flip screen half page */
	    dwnlin( min((lexcnt*tvlins)/2 , nxtlin-curlin+1) );
	    break;
case 41:			/* abort */
	    abort();
	    break;
case 42:			/* change characters */
	    if ((succ = delnxt(lexcnt)))
		succ = insert(1,TRUE);
	    break;
case 43:			/* jump back to last location */
	    itmp = curlin;
	    curlin = lastln;
	    curchr = *(lines+curlin)+1;	/* point to the current character */
	    verify(1);
	    lastln = itmp;
	    break;
case 44:			/* tidy up screen */
	    succ = neaten(lexcnt);
	    break;
case 45:			/* save current location */
	    if (lexcnt < 1 || lexcnt > 9)
		lexcnt = 0;
	    noteloc[lexcnt] = curlin;
	    break;
case 46:			/* return to noted location */
	    itmp = curlin;
	    if (lexcnt < 1 || lexcnt > 9)
		lexcnt = 0;
	    if (noteloc[lexcnt] >= nxtlin)
	      {
		tverrb("Line no longer there ");
		noteloc[lexcnt] = curlin;
	      }
	    else
	      {
		curlin = noteloc[lexcnt];
		curchr = *(lines+curlin)+1; /* point to the current character */
		verify(1);
		lastln = itmp;
	      }
	    break;

case 47:
	    opsystem();		/* call operating system */
	    break;

case 48:
	    if (lex_def)		/* default 1 passed */
		lexcnt = rptuse + 1;	/* use current repeat loop */
	    succ = edit_rpt(lexcnt);	/* edit repeat buffer */
	    break;

case 49:
	    succ = store_rpt(lexcnt);	/* store repeat buffer */
	    break;

case 50:
	    succ = exec_rpt(lexcnt);	/* execute repeat buffer */
	    break;

case 51:
	    succ = ins_pat(lexcnt);
	    break;
case 52:
	    succ = user_1(lexcnt);	/* user function 1 */
	    break;

case 53:
	    succ = user_2(lexcnt);	/* user function 2 */
	    break;
case 54:				/* '~': change case */
	    foldcase(lexcnt);
	    break;
	    }  			/* end of switch */
	    continue;		/* next iteration of do loop */
	  } /* end of else */
#ifdef SCR_BUF
	ttflush();
#endif
      } /* end of do loop */
    while (1);

lquit:
    for ( wtpage(1) ; rdpage() ; wtpage(1) )	/* write whole file */
	;
    tvclr();
  }
#endif

#ifdef VI_EM			/* vi emulation */
/* =============================>>> EDIT   <<<============================= */
  edit()
  { /*	edit - main editing routine */

    SLOW int lexval,lexcnt,succ, lastln, itmp;
    SLOW int noteloc[10], ni, lex_def;
    SLOW char lexchr;

    static int ins_set[] =	/* MUCH more limited than tvx */
      {
	VINSERT, VENDZ, VTVX, VNOOP, 0
      };

    static int jump_set[] =	/* commands to not reset jump memory */
      {
	VTVX, VMEMORY, VHELP, VNOTELOC, VNOOP, 0
      };
    SLOW char fchr;		/* temp char for prefixed commands */
    static int dir_up = FALSE;	/* search down by default */

    startm();
    remark("Reading file...");

    rdpage();			/* read a page into the buffer */

    tvclr();			/* clear the screen */

    if (curlin >= 1)
	tvtype(curlin,tvlins);	/* type out lines */

    tvxy(1,1);			/* and rehome the cursor */
    waserr = FALSE;		/* no errors to erase yet */

    if (curlin<1)
	tverr("Buffer empty");
#ifdef SCR_BUF
    ttflush();
#endif

    lexval = UNKNOWN;		/* so can remember 1st time through */
    useprint = FALSE;		/* not to printer */
    succ=TRUE;			/* assume success initially */

    lastln = 1	;		/* remember where we were */
    for (ni = 0 ; ni < 10 ; noteloc[ni++] = 1)
	;			/* init noteloc */
    do
      {
	oldlex = lexval;		/* remember last command */
	if (! succ)
	    echof = TRUE;		/* resume echo when error */
	lex_def = lex(&lexval,&lexcnt,&lexchr,succ);	/* get command input */
	if (waserr)
	    fixend();
	waserr=FALSE;
	succ=TRUE;
	if (lexval == UNKNOWN)
	  {
	    cmderr(lexchr);
	    succ = FALSE;	/* announce failure to lex */
	  }
	else
	  {
	    if (curlin < 1)	/* make sure legal command for empty buffer */
	      {
		if (!inset(lexval,ins_set))
		  {
		    tverrb("Can't, buffer empty. Insert 1st ");
		    succ=FALSE;
		    continue;
		  }
	      }
	    if (!inset(lexval,jump_set))
		lastln=curlin;		/* let user look at help w/o changing */

/* these all started out in order, but... */
	    switch (lexval)
	    {
case 1: 			/* ^B: screen up */
	    dwnlin(min(-lexcnt*tvlins,nxtlin-curlin+1));
	    break;
case 2: 			/* ^D: half screen */
	    dwnlin( min((lexcnt*tvlins)/2 , nxtlin-curlin+1) );
	    break;
case 3: 			/* ^F: screen down */
	    dwnlin(min(lexcnt*tvlins,nxtlin-curlin+1));
	    break;
case 4: 			/* ^G: memory status */
	    memory();
	    break;
case 5: 			/* ^H: left (must be here to avoid
				    conflict with normal char del */
	    right(-lexcnt);
	    break;
case 6: 			/* ^L: verify screen */
	    verify(lexcnt);
	    break;
case 7: 			/* '=': help */
	    shoset();
	    break;
case 8: 			/* '!' - tidy */
	    succ = neaten(lexcnt);
	    break;
case 9: 			/* '#': execute macro n times */
	    succ = exec_rpt(lexcnt);	/* execute repeat buffer */
	    break;
case 10:			/* '$': end of current line */
	    endlin();
	    break;
case 11:			/* '*': insert last found pattern */
	    succ = ins_pat(lexcnt);
	    break;
case 12:			/* '/': search down */
	    dir_up = FALSE;
	    succ = search(1,TRUE);	/* searching down*/
	    break;
case 13:			/* ':': set parameter, just like tvx */
	    tverrb("Use ZZ to exit ");
	    break;
case 14:			/* 'J': join lines */
	    endlin();
	    delnxt(lexcnt);
	    succ = insert((int) ' ',FALSE);
	    break;
case 15:			/* '?': find upwards */
	    dir_up = TRUE;
	    succ = search(-1,TRUE);
	    break;
case 16:			/* '@': execute current repeat loop */
	    if (lexcnt != 1)
		echof=FALSE;	/* turn off echo */
	    rptcnt[rptuse] = lexcnt > 0 ? lexcnt : (-lexcnt);
	    break;
case 17:			/* 'A': append to end of line {.i} */
	    endlin();
	    succ = insert(1,TRUE); /* go into insert mode, insert mode */
	    break;
case 18:			/* 'C': changes rest of line {"i } */
	    krest();		/* kill rest of line */
	    succ = insert(1,TRUE); /* enter insert mode */
	    break;
case 19:			/* 'D': delete rest of the line {"} */
	    krest();		/* kill rest of line */
	    break;
case 20:			/* 'G': goes to line number n, or
				    end of buffer if no n */
	    if (lex_def)	/* no n supplied */
		botpag();
	    else
	      {
		toppag();	/* go to top of file */
		dwnlin(lexcnt - 1);	/* go to that line */
	      }
	    break;
case 21:			/* 'H': Beginning of buffer */
	    toppag();		/* go to top of buffer */
	    break;

case 22:			/* 'I': inserts a beginning of line */
	    beglin();
	    succ = insert(1,TRUE); /* go into insert mode */
	    break;
case 23:			/* 'J': like vi j - down in column */
	    dwncol(lexcnt);
	    break;
case 24:			/* 'K': like vi k - up in column */
	    dwncol(-lexcnt);
	    break;
case 25:			/* 'L': bottom line of file */
	    botpag();
	    beglin();
	    break;
case 26:			/* 'M': return to marked location */
	    itmp = curlin;
	    if (lexcnt < 1 || lexcnt > 9)
		lexcnt = 0;
	    if (noteloc[lexcnt] >= nxtlin)
	      {
		tverrb("Line no longer there ");
		noteloc[lexcnt] = curlin;
	      }
	    else
	      {
		curlin = noteloc[lexcnt];
		curchr = *(lines+curlin)+1; /* point to the current character */
		verify(1);
		lastln = itmp;
	      }
	    break;
case 27:			/* 'N': reverse find again */
	    succ = search((dir_up ? 1 : -1),FALSE);
			 /* FALSE => don't read search string */
	    break;
case 28:			/* 'O': open a line above current line */
	    beglin();
	    openln(lexcnt);
	    succ = insert(1,TRUE); /* go into insert mode, insert mode */
	    break;
case 29:			/* 'P': put save buffer above current line */
	    beglin();
	    succ = getsav();
	    break;
case 30:			/* 'T': tvx commands */
	    if (!grptch(&fchr))
	      {
		succ = FALSE;
		break;
	      }
	    fchr = clower(fchr);	/* lower case */
	    if (curlin < 1)	/* make sure legal command for empty buffer */
	      {
		if ( fchr != 'b')
		  {
		    tverrb("Can't, buffer empty. Insert 1st ");
		    succ=FALSE;
		    break;
		  }
	      }
	    switch (fchr)
	    {
	    case ':':			/* set parameter */
		if (!grptch(&fchr))
		  {
		    succ = FALSE;
		    break;
		  }
		setpar(lexcnt,fchr);
		break;
	    case '!':
		opsystem();		/* call operating system */
		break;
	    case 'b':			/* goto real beginning of the file */
		succ = fbeg();
		break;
	    case 'e':			/* edit repeat buffer */
		if (lex_def)		/* default 1 passed */
		    lexcnt = rptuse + 1;	/* use current repeat loop */
		succ = edit_rpt(lexcnt);	/* edit repeat buffer */
		break;
	    case 'j':			/* jump back to last location */
		itmp = curlin;
		curlin = lastln;
		curchr = *(lines+curlin)+1; /* point to the current character */
		verify(1);
		lastln = itmp;
		break;
	    case 'p':		/* put external file from save buffer */
		succ = addfil(-1);
		break;
	    case 'r':			/* restore repeat buffer */
		succ = store_rpt(lexcnt);	/* store repeat buffer */
		break;
	    case 's':			/* print screen */
		scrprint();
		break;
	    case 'u':			/* "undo" */
		succ = unkill();
		break;
	    case 'w':			/* read in next page of file */
		wtpage(lexcnt);	/* write out the current page */
		succ = rdpage();	/* read in the next */
		tvclr();
		if (succ || lexcnt < 0)
		    verify(1);
		break;
	    case 'y':		/* yank external file to save buffer */
		succ = addfil(1);
		break;
	    case '/':		/* search across buffers */
		succ = snext(1,TRUE);
		break;
	    case '(':		/* user 1 */
		succ = user_1(lexcnt);	/* user function 1 */
		break;
	    default:
		tverrb("Use !,b,e,j,p,r,s,u,w,y, or / with q ");
		break;
	    }
	    break;
case 31:			/* 'X': delete character before cursor */
	    succ = delnxt(-lexcnt);
	    break;
case 32:			/* 'Y': append to save buffer */
	    succ = save(lexcnt,TRUE);
	    break;
case 33:			/* 'Z': exit (ZZ: normal, ZA: abort) */
	    if (!grptch(&fchr))
	      {
		succ = FALSE;
		break;
	      }
	    switch (clower(fchr))
	    {
	    case 'z':		/* normal exit */
		tvclr();
		remark("Exit");
		goto lquit;
	    case 'a':		/* abort or terminate exit */
		abort();
		break;
	    case 27:		/* escape is no op */
		break;
	    default:
		tverrb("Use Z or A with Z");
		break;
	    }
	    break;
case 34:			/* '^': beginning of line */
	    beglin();
	    break;
case 35:			/* 'a': append text */
	    right(1);
	    succ = insert(lexcnt,lex_def);
	    break;
case 36:			/* 'b': word left */
	    wordr(-lexcnt);
	    break;
case 37:			/* 'c': change something */
	    if (!grptch(&fchr))
	      {
		succ = FALSE;
		break;
	      }
	    switch (clower(fchr))
	    {
	    case 'c':		/* change line */
		killin(lexcnt);	/* remove the line */
		openln(1);
		succ = insert(1,TRUE); /* go into insert mode */
		break;
	    case ' ':		/* change one character */
		delnxt(lexcnt);
		succ = insert(1,TRUE); /* go into insert mode */
		break;
	    case '^':		/* to beginning of line */
		kprev();
		succ = insert(1,TRUE); /* go into insert mode */
		break;
	    case '$':		/* to beginning of line */
		krest();
		succ = insert(1,TRUE); /* go into insert mode */
		break;
	    case '/':		/* delete last thing, enter insert */
		if ((succ = rmvlst()))
		    succ = insert(1,TRUE);
		break;
	    case 27:		/* escape is no op */
		break;
	    default:
		tverrb("Use c,<sp>,^,$, or / with c ");
		break;
	    }
	    break;
case 38:			/* 'd': delete d, <sp>, ^, or $ */
	    if (!grptch(&fchr))
	      {
		succ = FALSE;
		break;
	      }
	    switch (clower(fchr))
	    {
	    case 'd':		/* delete line */
		killin(lexcnt);	/* remove the line */
		break;
	    case ' ':		/* delete character */
		delnxt(lexcnt);
		break;
	    case '0':
	    case '^':		/* to beginning of line */
		kprev();
		break;
	    case '$':		/* to beginning of line */
		krest();
		break;
	    case '/':		/* delete last thing */
		succ = rmvlst();
		break;
	    case 27:		/* escape is no op */
		break;
	    default:
		tverrb("Use d,<sp>,^,$, or / with d ");
		break;
	    }
	    break;
case 39:			/* ^U: half screen up */
	    dwnlin( min((-lexcnt*tvlins)/2 , nxtlin-curlin+1) );
	    break;
case 40:			/* 'i': insert */
	    succ = insert(1,lex_def);
	    break;
case 41:			/* 'j': down to line beginning */
	    dwnlin(lexcnt);
	    break;
case 42:			/* 'k': up to line beginning */
	    dwnlin(-lexcnt);
	    break;
case 43:			/* 'l': right */
	    right(lexcnt);
	    break;
case 44:			/* 'm': mark location n */
	    if (lexcnt < 1 || lexcnt > 9)
		lexcnt = 0;
	    noteloc[lexcnt] = curlin;
	    break;
case 45:			/* 'n': find next (in last direction) */
	    succ = search((dir_up ? -1 : 1),FALSE);
	    break;
case 46:			/* 'o': open following line {do} */
	    dwnlin(1);
	    openln(lexcnt);
	    succ = insert(1,TRUE); /* go into insert mode */
	    break;
case 47:			/* 'p': put save buffer after cur line */
	    dwnlin(1);
	    succ = getsav();
	    break;
case 48:			/* 'r': replace next char */
	    if (!grptch(&fchr))
	      {
		succ = FALSE;
		break;
	      }
	    delnxt(lexcnt);		/* delete count character */
	    succ = insert((int)fchr,FALSE);
#ifndef VI_MODS
	    right(-1);			/* and back over char replaced */
#endif
	    break;
case 49:				/* 's': substitute */
	    if ((succ = delnxt(lexcnt)))
		succ = insert(1,TRUE);
	    break;
case 50:				/* ESC: no operation */
	    break;
case 51:				/* 'w': advance word */
	    wordr(lexcnt);
	    break;
case 52:				/* 'x': delete char */
	    succ = delnxt(lexcnt);
	    break;
case 53:				/* 'y': yank to save buffer */
	    succ = save(lexcnt,FALSE);
	    break;
case 54:				/* '~': change case */
	    foldcase(lexcnt);
	    break;
	    }  			/* end of switch */
	    continue;		/* next iteration of do loop */
	  } /* end of else */
#ifdef SCR_BUF
	ttflush();
#endif
      } /* end of do loop */
    while (1);

lquit:
    for ( wtpage(1) ; rdpage() ; wtpage(1) )	/* write whole file */
	;
    tvclr();
  }
#endif

#ifdef EMAX_EM		/* emacs modeless editor emulation */
/* =============================>>> EDIT   <<<============================= */
  edit()
  { /*	edit - main editing routine */

    SLOW int lexval,lexcnt,succ, lastln, itmp;
    SLOW int noteloc[10], ni, lex_def;
    SLOW char lexchr;

    static int ins_set[] =	/* MUCH more limited than tvx */
      {
	VQUIT, VTVX, VNOOP, VEXTEND, VOPENLINE, VGET, 0
      };

    static int jump_set[] =	/* commands to not reset jump memory */
      {
	VTVX, VHELP, VNOOP, 0
      };

    static int c_jump_set[] =	/* jump set for ^C */
      {
	7, 10, 13, 'l', 0
      };

    SLOW char fchr;		/* temp char for prefixed commands */

    startm();
    remark("Reading file...");

    rdpage();			/* read a page into the buffer */

    tvclr();			/* clear the screen */

    if (curlin >= 1)
	tvtype(curlin,tvlins);	/* type out lines */

    tvxy(1,1);			/* and rehome the cursor */
    waserr = FALSE;		/* no errors to erase yet */

    if (curlin<1)
	tverr("Buffer empty");
#ifdef SCR_BUF
    ttflush();
#endif

    lexval = UNKNOWN;		/* so can remember 1st time through */
    useprint = FALSE;		/* not to printer */
    succ=TRUE;			/* assume success initially */

    lastln = 1 ;	/* remember where we were */
    for (ni = 0 ; ni < 10 ; noteloc[ni++] = 1)
	;			/* init noteloc */
    do
      {
	oldlex = lexval;		/* remember last command */
	if (! succ)
	    echof = TRUE;		/* resume echo when error */
	lex_def = lex(&lexval,&lexcnt,&lexchr,succ);	/* get command input */
	if (waserr)
	    fixend();
	waserr=FALSE;
	succ=TRUE;
	if (lexval == UNKNOWN)
	  {
	    cmderr(lexchr);
	    succ = FALSE;	/* announce failure to lex */
	  }
	else
	  {
	    if (curlin < 1)	/* make sure legal command for empty buffer */
	      {
		if (!inset(lexval,ins_set))
		  {
		    tverrb("Can't, buffer empty. Insert 1st ");
		    succ=FALSE;
		    continue;
		  }
	      }
	    if (!inset(lexval,jump_set))
		lastln = curlin;	/* let user look at help w/o changing */

	    switch (lexval)
	    {
case 1: 			/* ^A: Cursor to start of line */
	    beglin();
	    break;
case 2: 			/* ^B: left */
	    right(-lexcnt);
	    break;
case 3: 			/* ^C: Command (tvx) */
	    if (!grptch(&fchr))
	      {
		succ = FALSE;
		break;
	      }
	    fchr = clower(fchr);	/* lower case */
	    if (curlin < 1)	/* make sure legal command for empty buffer */
	      {
		if ( fchr != 2 && fchr != 'r')
		  {
		    tverrb("Can't, buffer empty. Insert 1st ");
		    succ=FALSE;
		    break;
		  }
	      }
	    if (!inset((int)fchr,c_jump_set))
		lastln = curlin;	/* reset jump loc if not special */

	    switch (fchr)
	    {
	    case 1:			/* ^A: append to save buffer */
		lexval = VSAPPEND;
		succ = save(lexcnt,TRUE);
		break;
	    case 2:			/* ^B: goto real begin of the file */
		succ = fbeg();
		break;
	    case 5:			/* ^E: edit repeat buffer */
		if (lex_def)		/* default 1 passed */
		    lexcnt = rptuse + 1;	/* use current repeat loop */
		succ = edit_rpt(lexcnt);	/* edit repeat buffer */
		break;
	    case 6:			/* ^F: fill (tidy) */
		succ = neaten(lexcnt);
		break;
	    case 7:			/* ^G: no op */
		break;
	    case 8:			/* ^H: half screen down */
		dwnlin( min((lexcnt*tvlins)/2 , nxtlin-curlin+1) );
		break;
	    case 10:			/* ^J: jump back to last location */
		itmp = curlin;
		curlin = lastln;
		curchr = *(lines+curlin)+1;	/* point to the current char */
		verify(1);
		lastln = itmp;
		break;
	    case 11:			/* ^K: kill last thing */
		succ = rmvlst();
		break;
	    case 13:			/* ^M: Mark current loc */
		if (lexcnt < 1 || lexcnt > 9)
		    lexcnt = 0;
		noteloc[lexcnt] = curlin;
		break;
	    case 14:			/* ^N: move to next line */
		dwnlin(lexcnt);
		break;
	    case 16:			/* ^P: beginning of prev line */
		dwnlin(-lexcnt);
		break;
	    case 18:			/* ^R: restore repeat buffer */
		succ = store_rpt(lexcnt);	/* store repeat buffer */
		break;
	    case 23:			/* ^W: write buffer */
		wtpage(lexcnt);	/* write out the current page */
		succ = rdpage();	/* read in the next */
		tvclr();
		if (succ || lexcnt < 0)
		    verify(1);
		break;
	    case '(':		/* user 1 */
		succ = user_1(lexcnt);	/* user function 1 */
		break;
	    case '~':			/* ~: change case */
		foldcase(lexcnt);
		break;
	    case '*':			/* * insert found pattern */
		succ = ins_pat(lexcnt);
		break;
	    case ';':			/* ;: search again */
		lexval = VSAGAIN;
		succ = search(1,FALSE);
		break;
	    case 'g':			/* goto line number n */
		toppag();	/* go to top of file */
		dwnlin(lexcnt - 1);	/* go to that line */
		break;
	    case 'h':			/* H: half screen up */
		dwnlin( min((-lexcnt*tvlins)/2 , nxtlin-curlin+1) );
		break;
	    case 'i':
		succ = insert(lexcnt,lex_def);
		break;
	    case 'l':			/* print screen */
		scrprint();
		break;
	    case 'm':			/* m: return to marked loc */
		itmp = curlin;
		if (lexcnt < 1 || lexcnt > 9)
		    lexcnt = 0;
		if (noteloc[lexcnt] >= nxtlin)
		  {
		    tverrb("Line no longer there ");
		    noteloc[lexcnt] = curlin;
		  }
		else
		  {
		    curlin = noteloc[lexcnt];
		    curchr = *(lines+curlin)+1; /* point to the current char */
		    verify(1);
		    lastln = itmp;
		  }
		break;
	    case 'p':			/* p: put line ins save buffer */
		lexval = VSAVE;
		succ = save(lexcnt,FALSE);
		break;
	    case 'r':		/* read external file to save buffer */
		succ = addfil(1);
		break;
	    case 's':		/* search across buffers */
		lexval = VNEXT;
		succ = snext(1,TRUE);
		break;
	    case 'u':			/* "undo" */
		succ = unkill();
		break;
	    case 'v':			/* v: set value */
		 if (!grptch(&fchr))
		  {
		    succ = FALSE;
		    break;
		  }
		setpar(lexcnt,fchr);
		break;
	    case 'w':		/* write save buffer to file*/
		succ = addfil(-1);
		break;
	    default:
		tverrb("Not valid command for ^C prefix ");
		break;
	    }
	    break;
case 4: 			/* ^D: Delete next char */
	    succ = delnxt(lexcnt);
	    break;
case 5: 			/* ^E: End of line */
	    endlin();
	    break;
case 6: 			/* ^F: right (forward) */
	    right(lexcnt);
	    break;
case 7: 			/* ^G: noop */
	    break;
case 8: 			/* ^H: delete backwards */
	    succ = delnxt(-lexcnt);
	    break;
case 9: 			/* ^K: kill line */
	    if (lex_def)	/* no n supplied */
	      {
		if (*(buff+curchr) == ENDLINE)	/* + new line if there */
		    delnxt(1);
		else
		    krest();	/* delete rest of the line, or newline */
	      }
	    else if (lexcnt == 0)	/* to beginning if 0 */
		kprev();
	    else
		killin(lexcnt);
	    break;
case 10:			/* ^L: verify */
	    verify(1);
	    break;
case 11:			/* ^N: next line (down in col) */
	    dwncol(lexcnt);
	    break;
case 12:			/* ^O: open line */
	    openln(lexcnt);
	    break;
case 13:			/* ^P: previous line (up in col) */
	    dwncol(-lexcnt);
	    break;
case 14:			/* ^R: reverse search */
	    succ = search(-1,TRUE);
	    break;
case 15:			/* ^S: normal search */
	    succ = search(1,TRUE);
	    break;
case 16:			/* ^V: forward pages */
	    dwnlin(min(lexcnt*tvlins,nxtlin-curlin+1));
	    break;
case 17:			/* ^X: extended command */
	    if (!grptch(&fchr))
	      {
		succ = FALSE;
		break;
	      }
	    fchr = clower(fchr);	/* lower case */
	    if (curlin < 1)	/* make sure legal command for empty buffer */
	      {
		if ( fchr != 3)
		  {
		    tverrb("Can't, buffer empty. Insert 1st ");
		    succ=FALSE;
		    break;
		  }
	      }
	    switch (fchr)
	    {
	    case 2:
		memory();
		break;
	    case 3:			/* ^C: abort */
		abort();
		break;
	    case 'e':			/* execute repeat loop */
		if (lexcnt != 1)
		    echof=FALSE;	/* turn off echo */
		rptcnt[rptuse] = lexcnt > 0 ? lexcnt : (-lexcnt);
		break;
	    default:
		tverrb("Not valid command for ^X prefix ");
		break;
	    }
	
	    break;
case 18:			/* ^Y: Yank back save buffer */
	    succ = getsav();
	    break;
case 19:			/* ^Z: save and exit */
	    tvclr();
	    remark("Save and exit");
	    goto lquit;
case 20:			/* !: call os */
	    opsystem();		/* call operating system */
	    break;

/*	these have been prefixed by escape */
case 21:			/* #: execute repeat loop again */
	    succ = exec_rpt(lexcnt);	/* execute repeat buffer */
	    break;
case 22:			/* >: end of buffer */
	    botpag();
	    break;
case 23:			/* <: top of buffer */
	    toppag();
	    break;
case 24:			/* b: backward words */
	    wordr(-lexcnt);
	    break;
case 25:			/* f: forward words */
	    wordr(lexcnt);
	    break;
case 26:			/* v: backward pages */
	    dwnlin(min(-lexcnt*tvlins,nxtlin-curlin+1));
	    break;
case 27:			/* ?: help */
	    shoset();
	    break;
	    }  			/* end of switch */
	    continue;		/* next iteration of do loop */
	  } /* end of else */
#ifdef SCR_BUF
	ttflush();
#endif
      } /* end of do loop */
    while (1);

lquit:
    for ( wtpage(1) ; rdpage() ; wtpage(1) )	/* write whole file */
	;
    tvclr();
  }
#endif

/* =============================>>> DWNCOL <<<============================= */
  dwncol(cnt)
  int cnt;
  { /* dwncol - move down in column */

    SLOW int curcol,l,oldef,needns;

    needns = FALSE;
    if (leftmg > 1)		/* handle right virtual screen different */
      {
	oldef=echof;
	needns = TRUE;
	echof = FALSE;
      }

    if (oldlex==VDOWNCOL || oldlex==VUPCOL)	/* several in a row? */
	curcol=oldcol;		/* pick up old value */
    else
      {
	curcol = curchr - *(lines+curlin);	/* calculate the current column */
	oldcol = curcol;
      }
    dwnlin(cnt);		/* go down given lines */
    if (curlin>=1 && curlin<nxtlin && curcol>1)	/* not at ends? */
      {
	l = strlen(buff + ((*(lines+curlin)) + 1) );
	right(min(curcol-1,l));
      }

    if (needns)			/* needed new screen */
      {
	echof=oldef;
	newscr();
      }
  }

/* =============================>>> RMVLST <<<============================= */
  int rmvlst()
  {  /* rmvlst - delete the previous thing found or manipulated
	length of oldlen is set by insert, find, and save
	may also use savlen if set by save */

    SLOW int oldech;
#ifdef TVX_CMDSET
    static int rmv_set[] =
      {
	VSEARCH, VNEXT, VSAVE, VGET, VSAGAIN, VSAPPEND,
	VMVWORD, VMVBWORD, 0
      };
#endif
#ifdef VI_EM
    static int rmv_set[] =
      {
	VSEARCH, VSAVE, VGET, VSAGAIN, VSAPPEND, VPUT,
	VMVWORD, VMVBWORD, VRSEARCH, VRSAGAIN, 0
      };
#endif
#ifdef EMAX_EM
    static int rmv_set[] =
      {
	VSEARCH, VNEXT, VSAVE, VGET, VSAGAIN, VSAPPEND,
	VMVWORD, VMVBWORD, VRSEARCH, 0
      };
#endif

    if (!inset(oldlex,rmv_set))
	return (FALSE);

    if (savlen > 0)
      {
	if (curlin == nxtlin-1 && slastl != 0)
	  {
	    --savlen;	/* reduce the count */
	    if (savlen > 0)
	      {
		oldech = echof;
		echof = FALSE;
		killin(-savlen);	/* kill off previous lines */
		echof = oldech;
	      }
	    killin(1);		/* kill the last line */
	  }
	else
	    killin(-savlen);	/* kill off savlen lines */
      }
    else if (oldlen != 0)
      {
	if (! delnxt(-oldlen))
	    return (FALSE);
      }
    oldlen = 0;			/* don't allow multiple deletes! */
    savlen = (-1);
    return (TRUE);
  }

/* =============================>>> SAVE   <<<============================= */
  int save(cnt,app)
  int cnt,app;
  { /* save - save cnt lines in save buffer */

    SLOW int l,lend;
    SLOW BUFFINDEX from;

    if (curlin == nxtlin-1 && slastl!=0)
      {
	tverrb("Can't save last line twice! ");
	return (FALSE);
      }
    if (cnt < 0)
	return (FALSE);

    oldlen = 0;			/* use savlin instead */

    if ((oldlex != VSAVE && !app) || cnt == 0)
      { 			/* if new save, cnt == 0 and not appending */
	slastl=0;
	savlin=0;		/* haven't saved anything */
	savlen=0;
	nxtsav=mxbuff;	/* start saving at end */
	if (cnt == 0)
	  {
	    return (TRUE);
	  }
      }

    if (oldlex != VSAPPEND && app)	/* need to reset count for append */
	savlen=0;

    lend=min(curlin+cnt-1 ,nxtlin-1);
    for (l=curlin; l <= lend ; ++l)
      {
	if (nxtsav-nxtchr < ALMOSTOUT)	/* make space if need and can */
	    if (!gbgcol() || (nxtsav-nxtchr) < ALMOSTOUT)
	      {
		tverrb("No save room ");
		return (FALSE);
	      }

	from = *(lines+l)+1;		/* first character of the line */
	do
	  {
	    *(buff+nxtsav--) = *(buff+from++);
	  }
	while (*(buff+from-1)!=ENDLINE);
	++savlin;		/* keep track of the length */
	++savlen;		/* savlen for rmvlst */
	if (curlin==nxtlin-1)	/* don't save last line twice! */
	  {
	    slastl=1;
	    break;
	  }
	dwnlin(1);	/* move to next line on screen for + only */
      }
#ifdef SCR_BUF
    ttflush();
#endif
    return (TRUE);
  }
