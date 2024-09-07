/* ------------------------ tvx_lib.c ---------------------------- */
#include "tvx_defs.ic"
#include "tvx_glbl.ic"

#ifdef COMPILESTANDARD
#define STANDARD	/* the set of standard functions TVX use */
#endif

#define LOCAL static	/* make locals to this module */

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

/* following are some non-standard routines required by TVX */

/* =============================>>> STCOPY <<<============================= */
  stcopy(from, i, to, j)
  char from[],to[];
  BUFFINDEX i,*j;
  { /* ## stcopy string, increment j */

    BUFFINDEX k1, k2;
 
    k2 = *j;
    for (k1 = i; from[k1] ; )
      {
	to[k2++] = from[k1++];
      }
    to[k2] = 0;
    *j = k2;
  }

/* =============================>>> STRCOPY <<<============================= */
  strcopy(from, i, to, j)
  char from[],to[];
  int i,*j;
  { /* ## stcopy string, increment j */

    FAST int k1, k2;
 
    k2 = *j;
    for (k1 = i; from[k1] ; )
      {
	to[k2++] = from[k1++];
      }
    to[k2] = 0;
    *j = k2;
  }

#ifndef GEMDOS
/* =============================>>> MIN <<<============================= */
  min(v1,v2)
  int v1,v2;
  {
    return (v1 > v2 ? v2 : v1);
  }

/* =============================>>> MAX <<<============================= */
  max(v1,v2)
  int v1,v2;
  {
    return (v1 > v2 ? v1 : v2);
  }
#endif

/*=============================>>> CLOWER  <<<================================*/
  char clower(ch)
  char ch;
  {
    return ((ch >='A' && ch<='Z') ? ch + ' ' : ch);
  }

/*=============================>>> CUPPER  <<<================================*/
  char cupper(ch)
  char ch;
  {
    return ((ch >= 'a' && ch <= 'z') ? ch - ' ' : ch);
  }

/* =========================>>> LOWER  <<<==============================*/
  lower(str)
  char str[];
  {
    FAST int i;

    for (i=0 ; str[i] ; ++i)
	str[i]=clower(str[i]);

  }

/* ===========================>>> PRINTC <<<============================== */
  printc(chr)
  char chr;
  { /* send one character to the printer */

#ifdef MSDOS
	bdos(5,chr);	/* cp/m, ms-dos version */
#endif
#ifdef OSCPM
	bdos(5,chr);	/* cp/m, ms-dos version */
#endif
#ifdef GEMDOS
	gemdos(5,chr);	/* gemdos version */
#endif
  }

/*=============================>>> PROMPT <<<================================*/
  prompt(msg)
  char msg[];
  {
    SLOW int i;
    i = strlen(msg);
    ttwtln(msg,i);
#ifdef SCR_BUF
    ttflush();
#endif
  }

/*=============================>>> QUIT <<<================================*/
  quit()
  {
   exit(0);
  }

/*=============================>>> RINDX  <<<================================*/
  rindx(str, c)
  char c, str[];
  {  /* rindx - find last occurrence character  c  in string  str */

    FAST int i,j;
 
    j = -1;
    for (i = 0 ; str[i] != 0; i++)
        if (str[i] == c)
            j = i;
    return (j);
  }

/*=============================>>> REMARK <<<================================*/
  remark(msg)
  char msg[];
  {
    prompt(msg);
    ttwt(CR);
#ifdef USELF
    ttwt(LF);
#endif
#ifdef SCR_BUF
    ttflush();
#endif
  }

/*=============================>>> UPPER  <<<================================*/
  upper(str)
  char str[];
  {
    static int i;

    for (i=0 ; str[i] ; ++i)
	str[i]=cupper(str[i]);
  }

/*=============================>>> WTINT  <<<================================*/
  wtint(intg)
  int intg;
  {
    char chrep[10];
    itoa(intg,chrep);
    prompt(chrep);
  }

/*=============================>>> LREPLY <<<================================*/
  lreply(msg,maxc)
  char msg[];
  int maxc;
  {
    reply(msg,maxc);
    lower(msg);
  }

/*=============================>>> UREPLY <<<================================*/
  ureply(msg,maxc)
  char msg[];
  int maxc;
  {
    reply(msg,maxc);
    upper(msg);
  }

/*=============================>>> REPLY <<<================================*/
  reply(msg,maxc)
  char msg[];
  int maxc;
  {
#define CBS 8		/* Backspace */
#define CDL1 21		/* ^U */
#define CDL2 24		/* ^X */
#define CABORT 3	/* ^C */
#define CRET 13		/* cr */
#define CESCAPE	27	/* ESC to allow any char to be entered */
#define BKSPC 8

    static char ch, rp;
    static int i;
    SLOW int oldtty;

    oldtty = ttymode;
    ttymode = FALSE;		/* change to regular mode */

    for (i = 0 ; i < maxc ; )	/* i -> next char */
      {
	ch = ttrd_();		/* read the character */
	if (ch == CESCAPE)	/* literal next */
	  {
	    ch = ttrd_();
	    goto ESC_CONT;
 	  }
	if (ch == CBS)		/* back space */
	  {
	    if (i > 0)		/* must be something to delete */
	      {
		--i;		/* wipe out char */
		ttwt(BKSPC); ttwt(' '); ttwt(BKSPC);
		if (msg[i] < ' ')	/* double echo ^ chrs */
		  {
		    ttwt(BKSPC); ttwt(' '); ttwt(BKSPC);
		  }
	      }
#ifdef SCR_BUF
	    ttflush();
#endif
	  }
#ifdef USE_WIPE
	else if (ch == CDL1 || ch == CDL2)	/* wipe whole line */
	  {
	    i = 0;		/* set for loop ++ */
	    remark("#");
	    prompt("Re-enter? ");
	  }
#endif
	else if (ch == CABORT && !ins_mode)
	  {
	    remark("^C");
	    prompt("Exit to operating system - are you sure? (y/n) ");
	    rp = ttrd_();
	    if (rp == 'y' || rp =='Y')
	     {
		remark("y");
		reset();			/* need to reset things */
		exit(0);
	     }
	    remark("n");
	    msg[i] = 0;
	    prompt("Re-enter? "); prompt(msg);		/* re-echo */
	  }
	else if (ch == CRET)		/* ret, so done */
	  {
	    remark("");
	    msg[i] = 0;
	    ttymode = oldtty;
	    return;
	  }
	else
	  {
ESC_CONT:
	    msg[i++] = ch;
	    msg[i] = 0;			/* always 0 terminate */
	    if (ch < ' ')
	      {
		ch += '@';
		ttwt('^');
	      }
	    ttwt(ch);			/* echo char */
#ifdef SCR_BUF
	    ttflush();
#endif
	  }
      } /* end for */

    ttymode = oldtty;
    remark("");
  }

/* ============================>>> TTRD_   <<<================================ */
  ttrd_()
  {
    SLOW char tc;
#ifdef RD_FROM_CONSOLE_DIRECTLY
#ifdef OSCPM
    while (!(tc = bdos(6,-1)))		/* cp/m implementation */
	;
#endif
#ifdef MSDOS
    tc = bdos(7,-1);		/* ms-dos implementation */
#endif
#ifdef GEMDOS
    tc = gemdos(7);		/* ms-dos implementation */
#endif
#ifdef UNIX
    tc = ttrd();
#endif
#else
    gkbd(&tc);			/* this should work */
#endif

    return (tc & 0377);

  }

/*=============================>>> RDINT <<<================================*/
  rdint(val)
  int *val;
  {
    char chrrep[12];
    reply(chrrep,11);
    *val = atoi(chrrep);
    return;
  }

/* =============================>>> ITOA   <<<============================= */
  itoa(intg, str)
  int intg;
  char str[];
  {  /* itoa - convert integer  int  to char string in  str */
 
    FAST int i;
    int d, intval, j;
    char k;
    static char digits[] = "0123456789";
 
    intval = intg >= 0 ? intg : (-intg);
    str[0] = 0;
    i = 0;
    do
      {				/* generate digits */
        i++;
        d = intval % 10;	/* mod 10 */
        str[i] = digits[d];
        intval = intval / 10;
      }
    while (intval != 0);
    if (intg < 0)
      {				/* then sign */
        str[++i] = '-';
      }
    for (j = 0 ; j < i ; j++ )
      {				/* then reverse */
        k = str[i];
        str[i--] = str[j];
        str[j] = k;
      }
  }

/* ------------------------------------------------------------------------- */
#ifdef STANDARD

/* ============================>>> ATOI   <<<================================ */
  atoi(in)
  char in[];
  {  /* atoi - convert string : Ascii machines! */
 
    FAST int i;
    int d, val, neg;
    
    for (i=0 ; in[i] == ' ' || in[i] == '\t' ; i++)
        ;
    if (in[i] == '-')		/* look for negative */
      {
	i++;
	neg=1;
      }
    else
	neg=0;
    for (val = 0; in[i] ; i++)
      {
	if (in[i]<'0' || in[i]>'9')
	    break;
	d = in[i]-'0';
        val = 10 * val + d;
      }
    if (neg)
	val = (-val);
    return (val);
  }
#endif
/* ------------------------ tvx_lib.c ---------------------------- */
