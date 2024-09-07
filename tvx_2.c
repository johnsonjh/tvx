/* ========================================================================

	tvx_2.c - Part 2 of main TVX code 

============================================================================ */

#include "tvx_defs.ic"		/* note tv_defs will #include stdio.h */
#include "tvx_glbl.ic"

/* =============================>>> KILLIN <<<============================= */
  killin(cnt)
  int cnt;
  { /* ##  killin - kill cnt lines */

    SLOW int i,lim;
    SLOW int from,to,ityp,istrt;
    SLOW int s_curlin, s_curchr, s_echo;

    if (cut_mode)		/* does kill "cut" */
      {
	s_echo = echof;
	echof = FALSE;		/* turn off echo */
	s_curlin = curlin;	/* save line */
	save(cnt,FALSE);	/* save those lines */
	curlin = s_curlin;
	echof = s_echo;		/* restore things */
      }

    if (cnt+curlin >= nxtlin || (curlin == nxtlin-1 && cnt >= 0))
      { 			/* special case: deleting rest of buffer */
	svklin(nxtlin-1);
	for (i = curlin ; i <= nxtlin-1 ; )
	    kline(*(lines+i++));
	nxtlin = curlin--;
	if (curlin > 0)
	  {
	    curchr = *(lines+curlin)+1;
	    newscr();
	  }
	else
	  {
	    curchr = 0;
	    tvclr();
	  }
#ifdef SCR_BUF
	ttflush();
#endif
	return;
      }

    if (cnt < 0)		/* negative kill */
      {
	cnt = min(-cnt,curlin-1);	/* all upwards? */
	dwnlin(-cnt);		/* go up that far */
      }

    if (cnt != 0)
      {
	range(cnt,&to,&from);	/* calculate the line numbers to kill */

	curlin=to;		/* remember new current line */

	svklin(from);	/* save one line */
	for (i = to ; i <= from ; )		/* mark lines deleted */
	    kline(*(lines+i++));

	lim = min(nxtlin-1,mxline);
	for (++from ; from <= lim ; )
	  {
	    *(lines+to++) = *(lines+from++);	/* copy next line number */
	  }

	nxtlin=to;
	if (nxtlin == curlin)
	    --curlin;		/* don't go past end */
	curchr = *(lines+curlin)+1;	/* remember new current character */

	if (cnt >= 0 && curlin+(tvlins-tvdlin) < nxtlin &&
	  tvdlin < tvlins)	/* killing down */
	  {
	    tvxy(1,tvy);	/* get to start of line */
	    ityp=min(tvlins-tvdlin+1,nxtlin-curlin);
	    if (cnt!=1 || !ckline[0])
	      {
		tvescr();	/* erase the screen */
		istrt=curlin;
	      }
	    else
	      {
		sendcs(ckline);
		istrt=curlin+ityp-1;
		tvxy(1,tvlins);
		ityp=1;
	      }
	    tvtype(istrt,ityp);
	    tvhdln();	/* home to display line */
	  }
	else if ( cnt != 1)	/* neg and > 1 too complicated */
	    newscr();			/* kill up, just retype whole screen */
	else if (nxtlin < tvlins)	/* top part of screen */
	  {
	    if (*ckline)		/* kill line defined */
	      {
		tvxy(1,tvy);		/* get to start of line */
		sendcs(ckline);		/* just need to kill the line */
		tvhdln();
	      }
	    else
		newscr();		/* rewrite it all */
	  }
	else if (tvdlin < tvlins)	/* must be in last part of buffer */
	  {
	    if (*ckline && *ctopb)	/* kill line & topb defined */
	      {
		tvxy(1,tvy);		/* get to start of line */
		sendcs(ckline);		/* kill the line */
		if (curlin-tvdlin > 0)	/* something to scroll */
		  {
		    tvtopb(1);		/* scroll down one line */
		    tvtype(curlin-tvdlin,1);	/* type the offscreen line */
		    tvdlin++;		/* will start display on next line */
		  }
		tvhdln();
	      }
	    else
		newscr();		/* rewrite it all */
	  }
	else		/* if all else fails */
	    newscr();
      }
#ifdef SCR_BUF
    ttflush();
#endif
  }

/* =============================>>> KLINE  <<<============================= */
  kline(ptr)
  BUFFINDEX ptr;
  {  /* kline - kill off the line beginning at buff position ptr */

    SLOW BUFFINDEX i;

    for (i=ptr; *(buff+i) != ENDLINE ; )	/* insert GARBAGE to kill */
	*(buff+i++) = GARBAGE;

    *(buff+i) = GARBAGE;		/* kill the endline */
  }

/* =============================>>> KPREV  <<<============================= */
  kprev()
  { /* kprev - kill from cursor to beginning of line */

    FAST int chrs;

    svklin(curlin);				/* save one line */
    chrs = curchr - *(lines+curlin) - 1;	/* how much to delete */
    if (chrs > 0)
	delnxt(-chrs);	/* won't cause a combine, so don't worry */
  }

/* =============================>>> KREST  <<<============================= */
  krest()
  { /* krest - kill the rest of the line, not including cursor and ENDLINE */

    SLOW int chrs;
    SLOW BUFFINDEX i;

    svklin(curlin);	/* save one line */
    chrs=0;
    for (i=curchr; *(buff+i)!=ENDLINE; ++i)
	++chrs; 	/* count how much to delete */
    if (chrs > 0)
	delnxt(chrs);	/* won't cause combine, so don't worry */
  }

/* =============================>>> NEATEN <<<============================= */
  int neaten(count)
  int count;
  {  /* neaten - fill lines to current margin */

    SLOW int oldef, i;
    SLOW BUFFINDEX linbeg;
    SLOW int retval;

    retval = TRUE;
    oldef = echof;
    if (count > 1)
	echof = FALSE;
    if (wraplm <= 1 || curlin >= nxtlin-1)
	goto l900;		/* work only if wrap limit turned on */

    for (i=1 ; i<=count ; ++i)
      {
	beglin();		/* start at beginning of line */
	if (curlin >= nxtlin-1)
	    goto l900;

	/* don't neaten leading space, cr, period or tab */

	if (*(buff+curchr) == '.')
	  {
	    dwnlin(1);
	    continue;		/* skip dot commands */
	  }

	while (*(buff+curchr)== ' ' || *(buff+curchr)==ENDLINE
	  || *(buff+curchr) == 9)
	  {
	    right(1);	/* skip this line */
	  }

	do
	  {
	    if (*(buff+curchr) == ENDLINE)
	      {
		if (tvx+leftmg < wraplm)	/* combine lines! */
		  {
		    linbeg = *(lines+curlin+1)+1;
			/* pt to first char of next line */
		    if (*(buff+linbeg) == ' ' || *(buff+linbeg) == ENDLINE
		      || *(buff+linbeg) == 9 || *(buff+linbeg) == '.')
		      {
			dwnlin(1);
			break;	/* no more combining */
		      }
		    if (! neat1(1,32))
			goto l990;
		    goto NEATNEXT;	/* tab over another word */
		  }
		else
		  {
		    dwnlin(1);	/* no more combining on line */
		    break;
		  }
	      }

NEATNEXT:
	    if (*(buff+curchr-1)==' ' && tvx+leftmg >= wraplm)	/* change to cr */
	      {
		if (! neat1(-1,CR))	/* delete the blank */
		    goto l990;
		break;
	      }
	    wordr(1);
	  } /*# end of the repeat */
	while (1);
      } /*# end of the for	 */
l900:
    echof = oldef;
    if (oldef && count > 1)
	newscr();
#ifdef SCR_BUF
    else
	ttflush();
#endif

    return (retval);

l990:				/* failure return */
    retval = FALSE;
    goto l900;
  }

/* =============================>>> NEAT1  <<<============================= */
  neat1(dir, val)
  int dir, val;
  {  /* change character dir to val */

    SLOW int oldwrp;

    oldwrp = wraplm;
    wraplm = 0;
    if (! delnxt(dir))
	goto l900;
    if (! ins_chr(val))
	goto l900;
    wraplm = oldwrp;
    return (TRUE);
l900:
    wraplm = oldwrp;
    return (FALSE);
  }

/* =============================>>> NEWSCR <<<============================= */
  newscr()
  { /* newscr - retype entire screen, updating cursor position if necessary */

   SLOW int ibeg,cnt;

    if (tvlins != tvhardlines || nxtlin-1 <= tvlins)
	/* two kinds of screen rewrite */
	tvclr();			/* clear the screen and home */
    else
	tvxy(1,1);

    finddl(&ibeg,&cnt); 	/* calculate where it will go */
    tvtype(ibeg,cnt);		/* type it out */
    tvhdln();			/* home to display line */
#ifdef SCR_BUF
    ttflush();
#endif
  }

/* =============================>>> OPENLN <<<============================= */
  openln(cnt)
  int cnt;
  {  /* openln - open a new line */

    FAST int i;
    SLOW int pcnt, oldauto;

    oldauto = autoin; autoin = FALSE;	/* don't allow autoindent */
    pcnt = cnt >= 0 ? cnt : (-cnt);	/* only allow positive opens */
    for (i=1; i<=pcnt; ++i)
	ins_chr(CR);	/* insert right number of newlines */
    dwnlin(-pcnt);	/* and goto beginning of the opened line */
    endlin();
    autoin = oldauto;
#ifdef SCR_BUF
    ttflush();
#endif
  }

/* =============================>>> RANGE  <<<============================= */
  range(cnt,lbeg,lend)
  int cnt,*lbeg,*lend;
  { /* determine a legal line number range given cnt */

    if (cnt <= 0)
      {
	*lbeg=max(curlin+cnt,1);
	*lend=curlin;
	if (cnt < 0)
	   *lend = (*lend)-1;
      }
    else
      {
	*lbeg=curlin;
	*lend=min(nxtlin-1,curlin+cnt-1);
      }
 }

/* =============================>>> RIGHT  <<<============================= */
  right(cnt)
  int cnt;
  {  /* move cursor right cnt characters
	newlines count as one character */

    FAST int change,i;

    change=0;			/* nochange yet */
    if (cnt > 0)
      {
	for (i = 1 ; i <= cnt ; ++i)
	  {
	    if (*(buff+curchr)==ENDLINE)
	      {
		if (curlin+1 >= nxtlin)
		    break;		/* don't go beyond end! */
		++curlin;
		++change;		/* we've gone down one line */
		curchr = *(lines+curlin)+1;
	      }
	    else
		++curchr;
	  }
      }
    else if (cnt < 0)
      {
	cnt=(-cnt);
	for (i = 1 ; i <= cnt ; ++i)
	  {
	    --curchr;
	    if (*(buff+curchr) == BEGLINE)
	      {
		if (curlin > 1)
		  {
		    --curlin;
		    --change;
		    for (curchr = *(lines+curlin) ; *(buff+curchr)!=ENDLINE ;
		      ++curchr)
			;	/* bump curchr to end of the line */
		  }
		else
		  {
		    ++curchr;
		    break;
		  }
	      }
	  }
      }
    if (change != 0)		/* don't do unnecessary change */
	update(change);
    tvhdln();
#ifdef SCR_BUF
    ttflush();
#endif
  }

/* =============================>>> SEARCH <<<============================= */
  search(lexcnt,iarg)
  int lexcnt,iarg;
  { /* search - search down in buffer for a pattern */

    SLOW char chr,c0,c1,c2;
    static int slines;
    SLOW int oldx,oldy,oldlin;
    SLOW int change, searchv, lininc, newln, fold_wild;
    SLOW int l,lbeg,is;
    SLOW BUFFINDEX ib, bbeg, oldpos, nbbeg;
    FAST int i;

    SLOW int how_match, set_len;	/* how wild card matching happens */
    char *cp, *s_getset();
    SLOW int w_len,inset,extra_len;		/* length of match */

    static int lastsb = 0;

    lininc = (lexcnt >= 0 ) ? 1 : (-1);
    searchv = FALSE;
    newln = FALSE;		/* haven't rubbed out 2nd line */

    oldpos = curchr;		/* need to remember for -f */
    oldx = tvx ; oldy = tvy ; oldlin = curlin;

    ins_mode = TRUE;		/* so ttymode can echo right */

    if (! iarg)
	goto l100;		/* get arg form search buffer */

    tvmsg("Find?",FALSE);

    if (! grptch(&chr))
	goto l9000;

    slines=1;			/* only one line so far */
    for (i = 0; chr != ESC && i < 100; ++i)	/* read in the pattern */
      {
	if (chr == delkey && rptcnt[rptuse] <= 0) /* edit interactive input */
	  {
	    --i;		/* adjust i for for loop ++i */
	    if (i >= 0)		/* wipe out chars on screen */
	      {
		if (sbuff[i] == 0)	/* newline */
		  {
		     --slines; ctrlch(CR); newln = TRUE;
		  }
		else
		  {
		    if (newln)
		      {
			tvcout('\\');
			ctrlch(sbuff[i]);
		      }
		    else
		      {
			tvcout(BACKSPACE);
			tvcout(' ');
			tvcout(BACKSPACE);
			if (sbuff[i] < ' ' && sbuff[i] != 27)
			  {
			    tvcout(BACKSPACE);
			    tvcout(' ');
			    tvcout(BACKSPACE);
			  }
		      }
		  }
		--i;		/* wipe the character */
	      }
#ifdef SCR_BUF
	    ttflush();
#endif
	    gkbd(&chr);		/* get new char */
	    continue;
	  }
	sbuff[i]=chr;		/* stuff it away */
	if (chr == LF)
	  {
#ifdef USELF
	    tvcout(chr);	/*$$$ to ignore lfs in cr/lf systems */
#endif
	  }
	else if (chr == CR)
	  {
	    if (rptcnt[rptuse] <= 0)
		ctrlch(chr);
	    ++slines;
	    sbuff[i]=0;	/* end of a line */
	  }
	else
	    ctrlch(chr);	/* echo character, handline control chars */

#ifdef SCR_BUF
	    ttflush();
#endif
/*# fetch the next character */
	if (! grptch(&chr))
	    goto l9000;
      }

    tvcout('$');	/* echo the escape */
    tvxy(oldx,oldy);	/* return to old location */

    if (i>0)			/* we got a new pattern */
      {
	lastsb=i-1;		/* last valid character */
	sbuff[i] = 0;		/* make sure an EOS */
      }
    fixend();

l100:
    extra_len = 0;
    if (lininc < 0)
	endlin();
    bbeg = curchr;		/* start from current character first time */
    c0 = sbuff[0];		/* initial char of pattern */
    if (!xcases)			/* get initial character of pattern */
	c0 = (c0 >= 'A' && c0 <= 'Z') ? c0+' ' : c0;

    for (l = curlin ; l < nxtlin && l ; l += lininc)  /* l is same as l != 0 */
      {
	if ( !c0 )		/* !c0 same as c0 == 0 */
	  {
	    if (lastsb == 0)	/* matching cr only? */
	      {
		dwnlin(1);	/* go down one line */
		newscr();	/* screen needs updating */
		goto l8000;
	      }
	    else
	      {
		for (ib = bbeg; *(buff+ib); ++ib)
		    ;
		goto l1000;
	      }
	  }

l900:
	if (c0 < ' ')	/* save time, check if might be w.c. once */
	  {
	    ib = bbeg;
	    if (*(buff+ib))
		goto l1000;
	  }

	for (ib=bbeg; *(buff+ib); ++ib)	/* search current line */
	  {
	    c2 = *(buff+ib);	/* next char of buffer */
	    if (!xcases)
		c2 = (c2 >= 'A' && c2 <= 'Z') ? c2+' ' : c2;
	
	    if (c2 != c0)
		continue;		/* first characters don't match */
	    else if (lastsb == 0)
	      { 		/* one character pattern */
		curchr = ib+1;
		curlin = l;
		goto l5000;	/* successful match */
	      }
	    else
	      {
		if ((c1 = sbuff[1]) < ' ')	/* possible wild? */
		    goto l1000;
		c2 = *(buff+ib+1);
		if (! xcases)	/* fold to lower case */
		  {
		    c1 = (c1 >= 'A' && c1 <= 'Z') ? c1+' ' : c1;
		    c2 = (c2 >= 'A' && c2 <= 'Z') ? c2+' ' : c2; /* clower */
		  }
		if ( c1 != c2 )
		    continue;	/* first two don't match */
		else
		    goto l1000;	/* first two match, so possibility */
	      }
	  }

/*    # fall thru => no match on this line */
l950:
	bbeg = *(lines+l+lininc)+1;	/* next beginning character */
	continue;			/* go check next line */
					
l1000:				/* we have two characters matching! */
	nbbeg = ib;		/* beginning of possible match in buff */
	lbeg = l; 		/* current line we are searching */
	how_match = 1;		/* assume exact match */
	for (is = -1 ; ++is <= lastsb ; )
	  {
	    if ((c1 = sbuff[is]) < ' ')		/* possible wild card */
	      {
		if (c1 == W_span)
		  {
		    extra_len--;
		    how_match = 2;		/* span match */
		    continue;			/* keep scanning search pat */
		  }
		else if (c1 == W_skip)		/* skip? */
		  {
		    extra_len--;
		    how_match = 0;		/* skip match */
		    continue;			/* keep scanning search pat */
		  }
		else if ((cp = s_getset(c1,&set_len,&fold_wild)) == NULL)	/* not wild */
		    goto NOT_WILD;		/* continue normally */
		 
	/* ok, to here, then have possible wild card match */
	
		w_len = 0;

		for ( ; ; )
		  {
		    chr = *(buff + nbbeg);	/* pick up char */
		    if (fold_wild)		/* fold if not user */
			chr = clower(chr);
		    if (chr == ENDLINE)		/* break on end of line */
			break;			/* get out */
	
		    inset = s_inset(chr,cp,set_len);	/* see if in set */
		    if ((how_match > 0 && inset) || (how_match == 0 && !inset))
		      {
			nbbeg++; 		/* bump to next char */
			++w_len;
			if (how_match == 1)
		            break;		/* only once on mode 1 */
		      }
		    else
			break;
		  }

		if (w_len <= 0)
		  {
		    ++bbeg; 	/* this part of line doesn't match */
		    extra_len = 0;
		    if (c0 == 0)
			goto l950;
		    else
			goto l900;	/* try rest of current line */
		  }

	/* to here, then exit from wild card match */
		extra_len += w_len - 1;
		how_match = 1;			/* back to 1 again */
		continue;		/* leave cursor on 1st unmatched */
	      }

NOT_WILD:
	    c2 = *(buff+nbbeg);
	    if (! xcases)	/* fold to lower case */
	      {
		c1 = (c1 >= 'A' && c1 <= 'Z') ? c1+' ' : c1;
		c2 = (c2 >= 'A' && c2 <= 'Z') ? c2+' ' : c2; /* clower */
	      }

	    if ( c1 != c2 )
	      {
		extra_len = 0;
		++bbeg; 	/* this part of line doesn't match */
		if (c0 == 0)
		    goto l950;
		else
		    goto l900;	/* try rest of current line */
	      }

	/* regular matched sequence */

	    if (*(buff+nbbeg)==0 && lbeg+1 < nxtlin)
	      {
		++lbeg;
		nbbeg = *(lines+lbeg)+1;	/* point to next character */
	      }
	    else
		++nbbeg;
	  }

/*#  fall through => found the pattern */
	curlin = lbeg;
	curchr = nbbeg;

l5000:
	change = curlin-oldlin;	/* compute real line change */
	if ((slines > 1 && iarg) || tvdlin == tvlins || newln)
	    newscr();
	else
	    update(change);
	goto l8000;
      }
    curchr = oldpos;		/* reset things */
    tvxy(oldx, oldy);
    if (slines > 1 && iarg)
	newscr();		/* patch up screen */
    pat_buff[0] = 0;
    tverrb("Not found ");	/* announce failure a little */
    goto l9000;

l8000:				/* success return */
    oldlen = lastsb+1+extra_len;		/* remember the length */
    save_pat();		/* save the find pattern */
    savlen = (-1);			/* haven't saved lines */
    searchv = TRUE;

l9000:
    ins_mode = FALSE;
#ifdef SCR_BUF
    ttflush();
#endif
    return (searchv);
  }

/* =============================>>> S_GETSET <<<============================= */
  char *s_getset(wildchr,set_len,fold)
  char wildchr;		/* wild card character */
  int *set_len, *fold;		/* length of set, fold flag */
  {
    static char sets[] =		/* possible sets */
      {
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
	'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
	'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '.', ',', '?', '!',
	'[', ']', '{', '}', '(', ')', '<', '>', '\'','"',
	'+', '-', '/', '*', '=', '@', '#', '$', '%', '^',
	'&', '_', '~', '`', '|', '\\', ' ', 9, ';', ':', 0
      };

    struct wild_set
      {
	char wch;
	int s_start, s_len;
      };

    static struct wild_set wild_cards[] =
      {
	{ W_letter,  0, 26 },	/* ^L is a letter, starts at 0, 26 long */
	{ W_digit, 26, 10 },	/* ^D is digit, starts at 26, 10 long */
	{ W_alpha,  0, 36 },	/* ^A is alpha numeric, start at 0, 36 long */
	{ W_punc, 36,  4 },	/* ^P is punctuation, at 36, 4 long */
	{ W_anything,  0, 70 },	/* ^X is any thing, whole set */
	{ W_others, 36, 34 },	/* ^O is non-alphanum, start at 36, 32 long */
	{ 0 ,  0,  0 }	/* end of set */
      };

    SLOW int i;
    
    *fold = FALSE;		/* assume won't fold */
    if (!use_wild)
	return NULL;		/* not there if not using! */

    for (i = 0 ; wild_cards[i].wch ; ++i)	/* scan list */
      {
	if (wildchr == wild_cards[i].wch)	/* found the one we want */
	  {
	    *set_len = wild_cards[i].s_len;
	    *fold = TRUE;
	    return (&sets[ wild_cards[i].s_start ]);
	  }
      }
    if (wildchr == W_user)
      {
	*set_len = strlen(user_set);
	return user_set;
      }
    else
	return NULL;

  }
  
/* =============================>>> S_inset <<<============================= */
  s_inset(c2,cp,set_len)
  char c2, *cp;
  int set_len;
  {
    FAST int i;

    for (i = 0 ; i < set_len ; ++i)
	if (c2 == *(cp+i))
	    return TRUE;
    return FALSE;
  }
  
/* =============================>>> SETPAR <<<============================= */
  setpar(val,chr)
  int val;
  char chr;
  { /* setpar - reset varoius parameters
		syntax for setpar is [val]:<let>, where [val] is the new value
		of the parameter, : is the setpar command, and <let> is the
		letter of the parameter to set. */

    chr = clower(chr);

    switch (chr)
      {
   	case 'a':		/* set auto indent */
 	    autoin = val > 0;
	    break;

	case 'c':		/* cut mode */
 	    cut_mode = val > 0;
	    break;

   	case 'e':		/* expand tabs */
 	    tabspc = max(val,0);
	    verify(1);		/* need to redisplay */
	    break;

	case 'd':		/* display line */
	    if (val < 1 || val > tvlins)
		tverrb("Bad par val");
	    else
	      {
		dsplin=val;
		verify(1);
	      }
	    break;

   	case 'f':		/* set find mode */
	    xcases = val <= 0;
	    break;

	case 'm':		/* match wild cards */
	    use_wild = val;
	    break;

	case 'o':
	    if (rdonly)
	      {
		tverrb("Can't :o, R/O");
		break;
	      }
	    tvclr();
	    ask("New output file name: ",dest_file,FNAMESIZE);
	    if (*dest_file)
		rdonly = FALSE;
	    verify(1);
	    break;

	case 's':		/* scroll lines */
	    if (val < 0 || val > dsplin-1)
		tverrb("Bad par val");
	    else
		scroll=val;
	    break;

	case 't':		/* tty mode */
	    tvclr();
	    ttymode = val;
	    ttynext = 1000;
	    verify(1);
	    break;

	case 'r':		/* change repeat buffer in use */
	    if (val < 1 || val > REPEATBUFS)
		tverrb("Bad par val");
	    else
		rptuse=val-1;	/* adjust for 0 index int */
	    break;

	case 'u':
	    tvclr();
	    ask("Enter user wild card set: ",user_set,39);
	    verify(1);
	    break;

	case 'v':		/* virtual window size */
	    if (val < 3 || val > tvhardlines)
		tverrb("Bad par val");
	    else
	      {
		tvlins = val;			/* set to new display line */
		ddline = (tvlins / 2) + 1;	/* fix home line */
		setdscrl();			/* set scroll value */
		dsplin = ddline;		/* reset these */
		scroll = dscrl;
		verify(1);			/* update the screen */
	      }
	    break;

	case 'w':		/* change wrap width */
	    wraplm=val;
	    break;

	default:
	    tverrb("Bad par name");
      }
#ifdef SCR_BUF
    ttflush();
#endif
  }

/* =============================>>> SNEXT  <<<============================= */
  snext(lexcnt,iarg)
  int lexcnt,iarg;
  { /* snext - find a text pattern across page boundaries */

    SLOW int ihow,pagout;

    if (lexcnt < 0)
      {
	tverrb("Search fails");
	return (FALSE);
      }
	
    ihow=iarg;			/* make a local copy */
    pagout=FALSE;
    for(;;)
      {
	if (! search(lexcnt,ihow))
	  {
	    wtpage(1);		/* write out current page */
	    ihow=FALSE;		/* don't reread search pattern */
	    pagout=TRUE;
	    if (! rdpage())
	      {
		tvclr();	/* no more text */
		tverrb("Search fails");
		return (FALSE);
	      }
	  }
	else			/* found it */
	  {
	    if (pagout)
		newscr();
#ifdef SCR_BUF
	    else
		ttflush();
#endif
	    return (TRUE);
	  }
      }
  }

/* =============================>>> STORE_RPT <<<============================= */
  store_rpt(dummy)
  int dummy;
  {	/* start at current cursor position, insert into repeat buffer
	   identified until find >$$ or exceed size limit, deleting as it goes */

    SLOW char chr;
    SLOW int saved, i, val;

    beglin();		/* start by moving to beginning of current line */

    if ((chr = *(buff+curchr)) != '#')	/* get current char, which must be # */
      {
	tverrb("Not a valid rpt buff");
	return (FALSE);	
      }
    val = *(buff+curchr+1)-'0';		/* pick up buffer number */

    if (!chk_rpt_nr(val))
	return FALSE;

    delnxt(4);				/* delete the #n:< */

    --val;		/* change to relative */

    saved = 0;				/* no previous chars */
    for (i = 0 ;  ; ++i)
      {
	chr = *(buff+curchr);		/* get the character */

	if (chr == ESC && i > 1 && rptbuf[val][i-1] == ESC &&
	    rptbuf[val][i-2] == loop_end)
	  {
	    rptbuf[val][i-2] = 0;	/* set to 0 */
	    nxtrpt[val] = 0;
	    delnxt(2);			/* delete the 27 and following CR */
#ifdef SCR_BUF
	    ttflush();
#endif
	    return TRUE;
	  }
	if (++saved > 99)
	  {
	    tverrb("Only 100 chars in rpt");
	    return FALSE;
	  }
	if (chr == ENDLINE)
	    chr = CR;
	rptbuf[val][i] = chr;			/* save the char */
	delnxt(1);			/* and delete it */
      }
  }

/* =============================>>> SVKLIN <<<============================= */
  svklin(lin)
  int lin;
  { /* svklin - save one line that will be killed */

    SLOW BUFFINDEX from,to;

    to=0;
    for (from= *(lines+lin)+1; *(buff+from)!=ENDLINE; ++from)
      {
	unkbuf[to]= *(buff+from);	/* put it in unkill buffer */
	to = min(130,to+1);
      }
    unkbuf[to]=0;
  }

/* =============================>>> TOPPAG <<<============================= */
  toppag()
  { /* toppag - move cursor to top of the page */

    curlin=1;
    curchr = *(lines+1)+1;		/* first char of buffer */
    newscr();
  }

/* =============================>>> TVIDEFS <<<============================= */
  tvidefs()
  { /* initialize these AFTER opening, defaults set by -c */

    dsplin=ddline;
    scroll=dscrl;
    xcases=dxcase;
  }

/* =============================>>> TVINIT <<<============================= */
  tvinit()
  { /* perform initializations needed for tv edit */

    FAST BUFFINDEX i;
    FAST char *chrp;
    SLOW char *lim;
    char *malloc();

#ifdef MSDOS
    BUFFINDEX coreleft();		/* !!! cii-86 dependent */
#endif

/*	This is a time eater if a big buffer -- if your loader inits
	mem to some known value, it might be possible to change GARBAGE
	to that value (be sure no other conflicts, like EOS == 0) 	*/

/* try for maximum size buffer */

#ifndef GEMDOS
    if ((lines = (BUFFINDEX *) malloc((MAXLINE+1)*sizeof(BUFFINDEX)))
       == NULL)		/* line pointer array */
	exit(1);
#else
if ((lines=(BUFFINDEX *)malloc((unsigned int)((MAXLINE+1)*sizeof(BUFFINDEX))) )
       == NULL)		/* line pointer array */
	exit(1);
#endif

#ifdef UNIX
    for (mxbuff=MAXBUFF ; (buff = malloc(mxbuff+2))==NULL ; mxbuff -= 1000)
	;			/* text buffer pointer */
#endif
#ifdef OSCPM
    for (mxbuff=MAXBUFF ; (buff = malloc(mxbuff+2))==NULL ; mxbuff -= 1000)
	;			/* text buffer pointer */
#endif
#ifdef GEMDOS
    for (mxbuff = 60000L ; (buff = malloc((unsigned int) (mxbuff+2)))==NULL
      ; mxbuff -= 1000L)
	;			/* text buffer pointer */
#endif
#ifdef MSDOS			/* *** Cii-86 C compiler dependent! */

    /* cii-86 requires you to manually leave some memory left over
       for the I/O routines to use.  'coreleft' does this. Sigh. */

    if ((mxbuff = (coreleft() - 5000) ) > MAXBUFF)
	mxbuff = MAXBUFF;
    for ( ; (buff = malloc(mxbuff+2))==NULL ; mxbuff -= 1000)
	;			/* text buffer pointer */
#endif

    mxline = MAXLINE;

    lim = buff + mxbuff;
    for (chrp = buff ; chrp <= lim ; *chrp++ = GARBAGE )
	;	/* mark as all garbage */

    curlin =			/* init some stuff */
    oldlen =
    curchr = 0;

    xoutcm = leftmg = nxtlin = nxtchr = tvdlin = 1;
    *(buff+mxbuff) = ENDLINE;	/* needs to be ENDLINE for save buffer */
    nxtsav=mxbuff;		/* point to end of the buffer */

    pat_buff[0] = 0;		/* null pattern buffer */


    savlin = savlen = (-1);
    for (i = 0 ; i < REPEATBUFS ; ++i)
      {  			/* fix repeat buffers to initial state */
	rptbuf[i][0] = rptcnt[i]= nxtrpt[i] = rptbuf[i][1] = 0;
      }
    rptuse=0;			/* start with first repeat buff */
    bakflg = FALSE;
    ineof =
    echof = TRUE;
  }

/* =============================>>> TVERR  <<<============================= */
  tverr(str)
  char str[];
  { /* tverr -	display an error message on the last line of the screen
       		always writes on screen, returns to old position */

    SLOW int oldx,oldy,oldxot,oldech;

    waserr = TRUE;
    oldx=tvx ; oldy=tvy ; oldxot=xoutcm ; oldech=echof;

    ttynext = 1000;		/* force new read */

    echof = TRUE;			/* always echo! */
    tvmsg(str,TRUE); 		/* print the message part */
    tvxy(oldx,oldy);
    xoutcm=oldxot;
    echof=oldech;		/* restore to what it was */
#ifdef SCR_BUF
    ttflush();
#endif
  }

/* =============================>>> TVERRB <<<============================= */
  tverrb(str)
  char str[];
  { /* tverrb - display an error message on the last line of the screen
      		always writes on screen, returns to old position */

    sendcs(cerrbg);
    tverr(str);
    sendcs(cerred);
#ifdef SCR_BUF
    ttflush();
#endif
  }

/* =============================>>> TVHDLN <<<============================= */
  tvhdln()
  { /* tvhdln - home to display line */

    SLOW int xf;
    xf = findx();
    tvxy(xf,tvdlin);
  }

/* =============================>>> TVMSG  <<<============================= */
  tvmsg(str,intty)
  char str[];
  int intty;
  { /* tvmsg - display a message on the last line of the screen */

    FAST int i;
    SLOW int oldtty;

    tvxy(1,tvhardlines);
    tvelin();
    
    oldtty = ttymode;
    if (ttymode && intty)
      {
	ttymode = FALSE;
	prompt(">");
      }

    for (i=0; str[i]; ctrlch(str[i++]))
	;

    if (oldtty)		/* end with < if was ttymode */
	remark("<");

    ttymode = oldtty;
#ifdef SCR_BUF
    ttflush();
#endif
  }

/* =============================>>> TVTYLN <<<============================= */
  tvtyln(chrptr)
  BUFFINDEX chrptr;
  { /* tvtyln - type a line on the screen without cr/lf */

#ifdef ULBD
    FAST BUFFINDEX i;

    if (cundlb[0] || cboldb[0])	/* check for underline/bold */
      {
	for (i = *(lines+curlin)+1 ; *(buff+i)!=ENDLINE ; ++i)
	    if (*(buff+i)==TOGUNDERLINE || *(buff+i)==TOGBOLD)
	      {
		tvxy(1,tvy);
		xoutcm=1;
		tvplin(*(lines+curlin)+1);
		return;
	      }
      }
#endif
    xoutcm=tvx;
    tvplin(chrptr);
#ifdef SCR_BUF
    ttflush();
#endif
  }

/* =============================>>> UNKILL <<<============================= */
  int unkill()
  { /* unkill the single last line killed */

    SLOW char chrval;
    FAST int i;

    openln(1);		/* put the CR 1st - makes update pretty */
    for (i=0; unkbuf[i]; ++i)
      {
	chrval = unkbuf[i];
	if (! ins_chr(chrval))	/* unkill slowly by using insert */
	  {
	    return (FALSE);
	  }
      }
    dwnlin(1);		/* back to where we were */
    return (TRUE);
  }

/* =============================>>> UPDATE <<<============================= */
  update(change)
  int change;
  { /* update - update the screen when line position has changed
		will not be used if any alterations have been made */

    SLOW int abschg,bscrol;

    if (! echof)
	return;
    abschg =  change;

    bscrol = (ctopb[0]==0) ? 0 : scroll;

    if (change < 0)			/* have to scroll screen down */
      {
	abschg = (-change);
	if (tvdlin-abschg < 1)
	    newscr();
	else if (curlin < tvdlin)	/* won't fit exactly */
	  {
	    if (tvdlin >= dsplin-scroll && abschg!=1)
	      {
		tvclr();		/* clear the screen */
		tvtype(1,tvlins);	/* type out a screen */
	      }
	    tvdlin=curlin;
	  }
	else if (tvdlin-abschg >= dsplin-scroll)
	    tvdlin -= abschg;
	else
	  {
	    if (tvdlin > dsplin-scroll)
	      { 			/* moving up from below display line */
		abschg=dsplin-scroll-(tvdlin-abschg);
		tvdlin=dsplin-scroll;	/* update */
	      }
	    if (ctopb[0]==0)		/* can't do reverse linefeeds */
		newscr();		/* no choice, redraw screen */
	    else
	      {
		tvtopb(abschg);		/* make blank lines at top */
		tvtype(curlin-tvdlin+1,abschg);	/* fill in */
	      }
	  }
      }
    else if (change > 0)		/* have to scroll screen up */
	if ((tvdlin+change>tvlins && tvdlin<dsplin+bscrol) || change>=tvlins)
	    newscr();
	else if (tvdlin < dsplin+bscrol || nxtlin-1 <= tvlins)
	    if (tvdlin+change > dsplin+bscrol && nxtlin-1 > tvlins)
		newscr();
	    else
		tvdlin += change;
	else if (nxtlin-curlin<=tvlins-tvdlin)	/* on bottom part */
	  {
	    if (tvdlin<=dsplin+bscrol && abschg!=1)
	      {
		tvclr();		/* rewrite whole screen */
		tvtype(nxtlin-tvlins,tvlins);
	      }
	    tvdlin=min(tvlins,nxtlin-1)-(nxtlin-curlin)+1;
	  }
	else
	  {
	    tvbotb(abschg);		/* make room */
	    tvxy(1,tvlins-abschg+1);	/* home to right line */
	    tvtype(curlin+tvlins-tvdlin-abschg+1,abschg);  /* fix up screen */
	    if (tvdlin < dsplin+bscrol)
		tvdlin=dsplin;
	  }
    tvhdln();
#ifdef SCR_BUF
    ttflush();
#endif
  }

/* =============================>>> WORDR  <<<============================= */
  wordr(cnt)
  int cnt;
  {  /* wordr - move cursor over words */

    SLOW int lim,words,incr,lenmov;

    lenmov=0;
    if (cnt<0)
      {
	incr = (-1);		/* change */
	lim = (-cnt);
      }
    else if (cnt == 0)
      {
	incr = -1;
	lim = 0;
      }
    else 
      {
	incr = 1; 
	lim = cnt;
      }

    for (words=1; words<=lim; ++words)
      {
	if ((*(buff+curchr)==ENDLINE && incr>0) ||
	    (*(buff+curchr-1)==BEGLINE && incr<0) )
	  {
	    if (curlin+incr==nxtlin || curlin+incr<1)
		break;		/* at a buffer limit, so quit */
	    dwnlin(incr);	/* move up or down */
	    lenmov += incr;
	    if (cnt<0)
		endlin();
	    continue;		/* move to next word */
	  }

/* ok, first, skip over word characters */
	while (wrdchr(*(buff+curchr)))
	  {
	    if (*(buff+curchr-1)==BEGLINE && incr<=0)
		goto l100;
	    else
	      {
		curchr += incr;
		lenmov += incr;
	      }
	  }

/* now skip over remaining non word chars */
	while (! wrdchr(*(buff+curchr)))
	   {
	    if ((*(buff+curchr)==ENDLINE && incr>0) || (*(buff+curchr-1)==BEGLINE &&
	      incr<0))
		break;
	    else
	      {
		curchr += incr;
		lenmov += incr;
	      }
	  }
l100: ;
      }

    if (incr < 0)		/* move cursor to beginning of word */
	while (wrdchr(*(buff+curchr-1)))
	  {
	    curchr += incr;
	    lenmov += incr;
	  }
    tvhdln();
#ifdef SCR_BUF
    ttflush();
#endif
    oldlen = lenmov ; savlen=(-1) ;
  }

/* =============================>>> WRDCHR <<<============================= */
  int wrdchr(chr)
  char chr;
  { /* wrdchr - determine if a character is a "word" type character */

    if ((chr>='a' && chr <= 'z') || (chr >= 'A' && chr <= 'Z') ||
      (chr >= '0' && chr <= '9'))
	return (TRUE);
    else
 	return (FALSE);
  }
