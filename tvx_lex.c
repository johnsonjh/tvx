/* -------------------------- tvx_lex.c --------------------------- */
#include "tvx_defs.ic"

/* -------------------------- GLOBALS GO HERE -------------------------------*/
#define EXTERN

#include "tvx_glbl.ic"

   char clower(),cupper();

/* =============================>>> CHECKOS <<<============================= */
  checkos()
  {
	/* check if ok operating system */
#ifdef MSDOS
    if ((bdos(0x30,0) & 0xff) < 2)		/* !!! cii-86 dependent */
      {
	remark("TVX requires MS-DOS 2.0 or later");
	exit();
      }
#endif
  }

/* =============================>>> STARTM <<<============================= */
  startm()
  {
    prompt(START_IDM);
    prompt(VERSION); prompt("Terminal: ");
    remark(cversn);
    remark("");
    starthelp();
  }

/* =============================>>> STARTHELP <<<============================= */
  starthelp()		/* print start help message */
  {
	remark(START_HELPM); remark("");
  }


/* =============================>>> MEMORY <<<============================= */
  memory()
  { /* memory - print memory left */
 
    SLOW int nxt;
    SLOW unsigned int tmp;

    char value[10],msg[85],*cp;
 
    if (bakflg)
	return;			/* don't do this during logfile input */

    nxt=0;			/* where message goes */
    cp = (*dest_file ? dest_file : orig_file);
    strcopy(cp,max(strlen(cp)-36,0),msg,&nxt);	/* the file name */
    if (nxt <= 14)
     {
       strcopy(VERSION,0,msg,&nxt); /* TVX */
       strcopy(cversn,0,msg,&nxt);  /* terminal type */
     }
 
    strcopy(" Free chars:",0,msg,&nxt);	/* add ' Free chars: ' */

#ifdef LASL
    tmp = max(nxtsav - nxtchr - BUFFLIMIT,0);
#else
    tmp = nxtsav - nxtchr;
#endif

#ifdef INT16
    if (tmp > 30000)		/* handle "neg" size */
      {
	msg[nxt++] = '+';
	tmp -= 30000;
      }
#endif
    itoa(tmp,value);
    strcopy(value,0,msg,&nxt);	/* the value */
 
    strcopy(" Line:",0,msg,&nxt);	/* add ' Line: ' */
    itoa(curlin,value);
    strcopy(value,0,msg,&nxt);
    strcopy(" of ",0,msg,&nxt);
    itoa(nxtlin-1,value);
    strcopy(value,0,msg,&nxt);	/* add the count */
 
 
    tverr(msg); 	/* display line on screen */
 }
 
/* =============================>>> SHOSET  <<<============================= */
  shoset()
  {  /* show repeat buffer, help if available */

#ifdef HELP
    static char rp[2];
    FAST int i;
    SLOW char *cp, *msg;
    SLOW int fields, oldtty, hnum;
    SLOW unsigned tmp;

#ifdef TVX_CMDSET
#ifdef FULLHELP
    struct help_msg 
      {
	char *hmsg;
	char Vmsg;
      };

    static struct help_msg cmddes[] =	/* messages for help */
      {
	{"nApnd to sv buff", VSAPPEND},	{" Buffer beg     ",	VTOP},
	{" File beg",	     VFBEGIN},	{"nChange chars   ",	VCHANGE},
	{"nDown line      ", VDOWNLINE},{"nDown column",	VDOWNCOL},
	{" Buffer end     ", VBOTTOM},	{"nEdit rpt buffer",	VEDITRPT},
	{"nFind",            VSEARCH},	{" Find cross-buff",	VNEXT},
	{" Get save buffer", VGET},	{" Unkill lastline",	VUNKILL},
	{"nHalf page      ", VHALFP},	{"nInsert (to ESC)",	VINSERT},
	{" Jump back",	     VJUMP},	{"nKill character ",	VDELNEXT},
	{"nKill line      ", VKILLALL},	{"nLeft",		VLEFT},
	{" Memory status  ", VMEMORY},	{"nNote location  ",	VNOTELOC},
	{"nReset loc",	     VRETNOTE},	{"nOpen line      ",	VOPENLINE},
	{" Call Opr system", VSYSTEM},	{"nPage",		VFLIP},
	{" Print screen   ", VPRINTS},	{"nRight          ",    VRIGHT},
	{" Restore rpt buf", VSTORERPT},{"nSave lines     ",	VSAVE},
	{"nTidy, fill text", VTIDY},	{" Abort",		VABORT},
	{"nUp             ", VUPLINE},	{"nUp column      ",	VUPCOL},
	{" Verify screen",   VVERIFY},	{"nWrite buffer   ",	VWPAGE},
	{" Exit, end edit ", VQUIT},	{"nYank file",		VYANK},
	{"nDel prev char  ", VDELLAST},	{"nFind again     ",	VSAGAIN},
	{" Del last",	     VREMOVE},	{" Change last    ",	VRMVINS},
	{" Del to line beg", VKILLPREV},{" Del to line end",	VKILLREST},
	{" Line begining  ", VBEGLINE},	{" Line end       ",	VENDLINE},
	{"nWord right",	     VMVWORD},	{"nWord left      ",	VMVBWORD},
	{"nRepeat again   ", VXREPEAT},	{"nk Exec rpt k n times",VEXECRPT},
	{"n p Set param p ", VSETPARS},	{" Help           ",	VHELP},
	{" Insert find pat", VINSPAT},	{"nToggle case    ",	VFOLDCASE},
	{"/",0}			/* last variable entry */
      };
#endif
#endif

    if (bakflg)
	return;			/* don't do this during logfile input */

    oldtty = ttymode;
    ttymode = FALSE;
    if (!oldtty)
	tvclr();

    prompt("Parameter : cur val (1=y, 0=n)    Prev 16 cmds:");
    for (hnum = 0,i = old_cindex ; hnum < 16 ; ++hnum)
      {
	shocout(old_cmds[i]);
	i = ++i % 16;
      }

    remark("");
    prompt("A-Autoindent: "); wtint(autoin);
    prompt("  C-Cut mode: "); wtint(cut_mode);
    prompt("  E-Expand tabs: "); wtint(tabspc);
    prompt("  T-TTY mode: "); wtint(ttymode); remark("");

    prompt("F-Find: ignore case: "); 
	if (xcases)
	    tvcout('0');
	else
	    tvcout('1');

    prompt("    M-Match wild cards: "); wtint(use_wild); remark("");
    prompt("U-User wild card set: |");
    for (cp = user_set ; *cp ; ++cp)
      {
	shocout(*cp);
      }
    remark("|");

    prompt("D disp line:"); wtint(dsplin); 
    prompt("  S-Scroll window:"); wtint(scroll);
    prompt("  V-Virtual window:"); wtint(tvlins); 
	prompt("/"); wtint(tvhardlines);
    prompt("  W-Wrap width:"); wtint(wraplm); remark("");
    prompt("O-Output file: "); prompt(dest_file);
    prompt("  (input file is: ");prompt(orig_file); remark(")");
    remark("");

    prompt("Find: |");
    for (cp = sbuff ; *cp ; ++cp)
      {
	shocout(*cp);
      }
    remark("|");
    remark("");
    prompt("Max chars: ");
    tmp = mxbuff;
#ifdef INT16
    if (tmp > 30000)		/* handle "neg" size */
      {
	prompt("30000+");
	tmp -= 30000;
      }
#endif
    wtint(tmp); prompt("  Max lines: ");    wtint(mxline);
    prompt("  Cur line: "); wtint(curlin);
    if (usecz)
	prompt("    ^Z for EOF");
    remark("");
    remark("");

    prompt("R-Repeat buffer: ");wtint(rptuse+1);
    remark("   All repeat buffers : <contents>:");
    for (i = 0 ; i < REPEATBUFS ; ++ i)
      {
	fields = 5;
	shocout('#') ; shocout(i+'1') ; prompt(": <");
	for (cp = &rptbuf[i][0] ; *cp ; ++cp)
	  {
	    shocout(*cp);
	    ++fields;		/* how many letters */
	    if (*cp < ' ')
		++fields;
	    if (fields >= (tvcols - 6))
	      {
		prompt("+more+");
		break;
	      }
	  }
	remark(">");
      }

    ttymode = oldtty;
    memory();

#ifdef FULLHELP
    tvxy(1,22);

    ask("Press space to exit, anything else for command list ",rp,1);

    if (*rp == ' ')
      {
	ttymode = oldtty;
	verify(1);
	return;
      }

    if (!oldtty)
	tvclr();

/* ------------------------------ TVX ----------------------------------- */
#ifdef TVX_EM
    remark("Commands (n => count allowed):");
#endif
#ifdef TVX0M_EM
    remark("Commands (n => count allowed) (Non ^ cmds prefixed by ESC):");
#endif

#ifdef TVX_CMDSET
    for (hnum = fields = 0  ; ; ++hnum )
      {
	prompt("   ");
	cp = cmddes[hnum].hmsg;
	if (*cp == '/')	/* end of variable list */
	    break;
	else
	    shocout(*cp);	/* show n or blank */
	msg = ++cp;		/* where message is, skipping 'n' field */
	while (*cp)		/* ship to lex value */
	    ++cp;
	i = cmddes[hnum].Vmsg;		/* get the lexical index */
	shocout(cupper(lexsym[i]));	/* show the command */
	if (lexsym[i] >= ' ')
	    shocout(' ');	/* skip space for no '^' */

	shocout(' ');		/* space to separate */
	prompt(msg);		/* and show the message */
	if (++fields == 3)	/* bump fields, see if need newline */
	  {
	    fields = 0;
	    remark("");
	  }
      }
    if (fields != 0)
	remark("");
#endif		
#ifdef TVX_EM
    remark("   n<>$$:Rpt loop        @:Invoke cmd file      $:Escape");
#endif
#ifdef TVX0M_EM
    remark("  $n<>$$:Rpt loop       $@:Invoke cmd file      $:Escape");
#endif

/* ------------------------------ VI ----------------------------------- */
#ifdef VI_EM
    remark("Commands (n => count allowed):");
remark("");
remark("n^B:scrn up     n^D:half scrn   n^F:scrn dwn    ^G:status     ^L:refresh");
remark("n^U:up half scr n!:tidy         n#p:ex rpt p n   $:line end    *:ins last pat");
remark("/:search         =:help           ?:rev search  @:exec rpt lp");
remark("A:apnd ln end   C:chng rest ln  D:del rest line  nG:Goto ln n  H:buff beg");
remark("I:ins ln beg    J:join lines    nK:up: line beg  L:buff end    nM:ret to mark");
remark("N:rev find nxt  nO:open above   P:put sv buf abv");
remark("q:'tvx' prefix: q!:op system    qb:file begin    qu:limited undo"); 
remark("nq:p:set par p nqe:ed rpt buf   qj:jump back     qp:put file   qy: yank file");
remark("                qs:print scr    nqw:write buf   nqr:restore rpt q/:x-buff find");
remark("nX:del prev ch  nY:apnd sv buf  Z:[Z:exit,A:abort]             ^:line begin");
remark("a:append        nb:word left    c:chng [c,<sp>,^,$,/]      nd:del [d,<sp>,^,$,/]");
remark("ni:insert       nh:left         nj:down in col   nk:up line in col");
remark("nl:right        nm:mark loc     n:find next pat  no:open after");
remark("p:put sv buf after              r:replace 1 chr  ns:substitute");
remark("nw:word right   nx:delete chr   ny:yank to save buffer          n~:change case");
    remark("");
remark("n<>$$:Rpt loop  _:Invoke cmd file   $:Escape");
    remark("'c/' and 'd/' are used to change/delete last thing found");
#endif

/* ------------------------------ EMAX ----------------------------------- */
#ifdef EMAX_EM
remark("(n means count allowed: counts entered after <esc> or <ctrl-u>)");
remark("");
remark("^A:line beg    n^B:back chrs                 n^C:extended tvx commands:");
remark("");
remark("[n^A:apnd sv   ^B: file beg   n^C:chng chrs  n^E:Ed rpt bf   n^F:fill        ]");
remark("[ ^G:no op    n^H:half scrn    ^J:jmp back    ^K:kill 'last'  n^N:next ln    ]");
remark("[n^M:mark loc n^P:Prev ln      ^R:Restore rbf ^W:write buff    ;:search again]");
remark("[n~:chng case  nG:Goto line    nH:Half pg up  nI:ins val n     L: Print scrn ]");
remark("[nM:rtn 2 mrk  nP:Put sv bf     S:x buff find  U:unkill 1 ln nVp:set val(par)]");
remark("[ R:Read file into sv buff      W:write sv buff to file   *:insert found pat ]");
remark("");
remark("n^D:del chr    ^E:line end    n^F:fwrd chr   n^H:del prev ch  n^K:kill line");
remark(" ^L:repaint   n^N:next ln     n^O:open ln    n^P:prev ln       ^R:rev search");
remark(" ^S:search     ^U:specify n  n^V:page dwn   ^X^C:abort edit");
remark("^X^B:status  n^XE:ex. rpt bf  ^Y:yank sv bf   ^Z:write, exit");
remark(" $%:cmd file   $!:op. sys.   n$#p:ex rpt p   n$(:rpt loop      $<:buff begin");
remark(" $>:buff end   $?:help        n$B:back word  n$F:fwd word     n$V:page up");
remark("");
remark("$ = <esc>,  ')$$' ends repeat loops");
#endif


#ifdef GEMDOS
    remark("Find wild cards:  {TVX command '(' toggles 25/50 line display}");
#else
    remark("Find wild cards:");
#endif
remark("^A-alphanumeric  ^D-digit  ^L-letter  ^O-other,(not-^A)  ^P-punctuation");
remark("^X-any character ^U-user set  -- ^W-word of ^..  ^N-not in word of ^..");

#endif

    if (!oldtty)
	tvxy(1,24);
    ask("Press any key to resume ",rp,1);

    ttymode = oldtty;

    verify(1);
#else				/* HELP not defined */
    tverr(&rptbuf[rptuse][0]);
#endif
  }

/* =============================>>> SHOCOUT <<<============================= */
  shocout(c)
  char c;
  {

    if (c < ' ')
      {
	ttwt('^'); ttwt(c + '@');
      }
    else
	ttwt(c);
  }

/* =============================>>> LEXGET <<<============================= */
  lexget(chr)
  char *chr;
  {  /* lexget - get a character for lex, possibly from repeat buffer */

    SLOW char tmp;
l10:
    if (rptcnt[rptuse] > 0)	/* in a repeat buffer? */
      {
	*chr=rptbuf[rptuse][nxtrpt[rptuse]];	/* pick up from repeat buffer */
	if (*chr == 0) 	/* at end of rpt buff */
	  {
	    nxtrpt[rptuse] = 0;	/* start <> loop over */
	    if (--rptcnt[rptuse] == 0 && !echof)	/* all done with loop */
	      {
		echof = TRUE;	/* turn on echo again */
		newscr();	/* update screen after repeat */
	      }
	    goto l10;		/* loop again */
	  }
	++nxtrpt[rptuse];	/* bump to next char in loop */
      }
    else			/* not in loop, get from keyboard */
      {
	gkbd(&tmp);	/* picks up one character from the keyboard */
	*chr = old_cmds[old_cindex] = tmp;
	old_cindex = ++old_cindex % 16;
      }
  }

/* =============================>>> LEX    <<<============================= */
  lex(lexval,lexcnt,lexchr,parsok)
  int *lexval,*lexcnt,parsok;
  char *lexchr;
  { /* ##  lex - gets command input from terminal, and scans for
       #  its lexical value.  Returns a count if given.  Also handles
       #  repeat loops. */
 
    SLOW int count, lex_default;
    FAST int i;
    SLOW int neg, newln;

    static char chr,cmdchr,tchr;
#ifdef EMAX_EM
    SLOW char had_cu;
#endif

    lex_default = TRUE;

    if (!parsok)		/* abort if error in <> */
      {
	if (rptcnt[rptuse] > 0)	/* in loop? */
	  {
	    newscr();	/* clear screen, send message */
	    tverrb("<> not complete ");
	  }
	rptcnt[rptuse]=0;	/* abort loop if going */
	nxtrpt[rptuse]=0;
      }
l10:
    for (;;) 
      { 			/* need this loop to support <> */
	count = 1;		/* default count is 1 */
	lexget(&chr);		/* fetch a character */
#ifdef EMAX_EM
	had_cu = FALSE;		/* not ^U initially */
CU_COUNT:
	if (chr == 21)		/* ctrl-u count detect */
	  {
	    if (had_cu)
		fixend();
	    count *= 4;		/* assume 4 by default */
	    tvmsg("Arg: ",FALSE); /* echo Arg: 4' */
	    wtint(count);
	    had_cu = TRUE;	/* remember we had control-u */
	    lex_default = FALSE; /* and not the default */
	    lexget(&chr);
	  }
#endif
#ifdef NOMODE_LEX
	if ( ((chr & 0177) >= ' ' || chr == TAB || chr == CR)
	  && (chr & 0200) == 0)
	  {
#ifdef EMAX_EM
	    if (had_cu)
	      {
		had_cu = FALSE;
		fixend();
		if ((chr >= '0' && chr <= '9') || chr == '-')
		    goto EMAX_CNT;
	      }
	    for (i = 0 ; i < count ; ++i)	/* ^U times */
#endif
	    ins_chr((int) chr);	/* simply insert the character */
	    continue;		/* get next character */
	  }
	if (chr == escape_chr)	/* if an escape, simply get next char */
	    lexget(&chr);
#endif

	if (rptcnt[rptuse]>0 && chr == loop_beg)	/* detect nesting */
	  {
	    nxtrpt[rptuse] = 0 ; rptcnt[rptuse] = 0 ; echof=TRUE;
	    newscr();	/* update anything so far */
	    tverrb("No loop nesting ");
	    continue;
	  }
 
#ifdef EMAX_EM
EMAX_CNT:
#endif
	if ((chr>='0' && chr<='9') || chr=='-')	/* a number */
	  {
	    count = 0;  lex_default = FALSE;
	    neg = FALSE;	/* handle negative counts */
	    if (chr=='-')
		neg=TRUE;
	    else 
		count = chr-'0';	/* convert to int value */
	    for (;;) 
	      {
		if (rptcnt[rptuse] > 0)	/* have to handle rptbuf special */
		  {
		    if ((chr=rptbuf[rptuse][nxtrpt[rptuse]]) == 0)
			break;
		    ++nxtrpt[rptuse];
		  }
		else 
		    lexget(&chr);
		if (chr>='0' && chr<='9')	/* another number? */
		    count = count*10+chr-'0';
		else			/* done with number */
		    break;
	      }
	    if (neg)			/* fix if it was negative */
		count = min(-count ,-1);
	  }	/* end count arg if */

#ifdef EMAX_EM
	if (chr == 21)		/* another ^U? */
	    goto CU_COUNT;
	if (had_cu)
	    fixend();	/* count changes */
#endif

#ifdef VI_EM
	cmdchr = chr;		/* vi emulator is case sensitive */
#else
	cmdchr = clower(chr);	/* fold to one case */
#endif
	if (cmdchr == loop_beg)	/* starting a loop? */
	  {
	    lex_default = TRUE;			/* don't let lex count be def */
	    rptcnt[rptuse] = (count < 0) ? (-count) : count;	/* save count */
	    ins_mode = TRUE;			/* so ttymode works */
	    tvmsg("repeat[",FALSE);		/* echo 'repeat[k]: n<' */
	    wtint(rptuse+1); prompt("]: ");
	    wtint(rptcnt[rptuse]);

	    tvcout(loop_beg);
#ifdef SCR_BUF
	    ttflush();
#endif
	    nxtrpt[rptuse]=0;	/* begin inserting at beginning */
	    newln = FALSE;	/* no new line echo yet */
	    do			/* fetch repeat chars until get > */
	      {
		gkbd(&chr);	/* fetch a char */
		if (chr==delkey)	/* allow editing */
		  {
		    if (nxtrpt[rptuse] > 0)	/* don't go past start */
		      {
			--nxtrpt[rptuse];	/* wipe out last */
			if ((tchr = rptbuf[rptuse][nxtrpt[rptuse]])==CR)
			  {
			    ctrlch(CR);	/* going to newline */
			    newln = TRUE;		/* new line now */
			  }
			else if (!newln)
			  {
			    tvcout(BACKSPACE);	/* back over character */
			    tvcout(' ');
			    tvcout(BACKSPACE);
			    if (tchr < ' ' && tchr != 27)
			      {
				tvcout(BACKSPACE);	/* back over char */
				tvcout(' ');
				tvcout(BACKSPACE);
			      }
			  }
			else		/* have passed new line start */
			  {
			    ctrlch(rptbuf[rptuse][nxtrpt[rptuse]]);
			    tvcout('\\');
			  }
		      }
		    else
			tvcout(BELL);	/* trying to rubout too much */
#ifdef SCR_BUF
		    ttflush();
#endif
		    continue;
		  }
		else		/* a control character detected */
		    ctrlch(chr);	/* echo */
#ifdef SCR_BUF
		ttflush();
#endif
 
		rptbuf[rptuse][nxtrpt[rptuse]]=chr;	/* stuff in current rpt buff. */
		++nxtrpt[rptuse];	/* bump count */
		if (nxtrpt[rptuse] >= 100)	/* only allow 100 chars! */
		  {
		    newscr();
		    tverrb("100 chars only");
		    nxtrpt[rptuse]=0 ; rptcnt[rptuse]=0;
		    ins_mode = FALSE;
		    goto l10;	/* bail out */
		  }
	      }
	    while (!( chr==ESC && rptbuf[rptuse][nxtrpt[rptuse]-2]==ESC &&
	      rptbuf[rptuse][nxtrpt[rptuse]-3]==loop_end));	/* end do loop */

	    ins_mode = FALSE;		/* get ttymode right */

	    if (rptcnt[rptuse] > 1)	/* positive count? */
		echof = FALSE;	/* turn off echoing */
	    else if (newln)
		verify();	/* need to fix up screen because of newline */
	    else		/* 0 or 1 count */
	      {
		fixend();
		tvhdln();	/* get back where we were */
	      }

	    rptbuf[rptuse][nxtrpt[rptuse]-3] = 0;	/* mark with eos */
	    nxtrpt[rptuse]=0;	/* back for scan now */
	    continue;		/* now execute the loop */
	  }
#ifdef VB
	else if (cmdchr == lexsym[0])	/*$$$	indirect files! */
	  {
	    opnatf();
	    continue;
	  }
#endif
	for (i=0 ; synofr[i]!=0 ; ++i)
	  if (synofr[i]==cmdchr)
	     cmdchr=synoto[i];		/* allow synonyms */
 
	*lexval = UNKNOWN;	/* assume unknown command */
	for (i = 1 ; i<= LEXVALUES ; ++i)	/* scan all possible cmds */
	    if (cmdchr == lexsym[i])	/* found it */
	      {
		*lexval = i;
		break;
	      }
	*lexcnt = count;		/* return good stuff */
	*lexchr = chr;
	return (lex_default);		/* let know if gave back default */
      }					/* end of for(;;) */
  }
/* -------------------------- tvx_lex.c --------------------------- */
