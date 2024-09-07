
/* -------------------------- tvx_cfg.c --------------------------- */
#include "tvx_defs.ic"

#define BL remark("")

    char synofr[20] =	/* from table */
      {' ',13,']',000,000,000,000,000,000,000,00,00,00,00,00,00,00,00,00,00};
    char synoto[20] =		/* translate to table */
      {'r','d','{',00,000,0,000,00,00,000,00,00,00,00,00,00,00,00,00,00};
    char funkey = 0;		/* leading char for function key */
    char funchar[50] =	/* code sent by function key */
      {
	000, 000, 000, 000, 000, 000, 000, 000, 000, 000,
	000, 000, 000, 000, 000, 000, 000, 000, 000, 000,
	000, 000, 000, 000, 000, 000, 000, 000, 000, 000,
	000, 000, 000, 000, 000, 000, 000, 000, 000, 000,
	000, 000, 000, 000, 000, 000, 000, 000, 000, 000,
      };
    char funcmd[50] =	/* equivalent command */
      {
	000, 000, 000, 000, 000, 000, 000, 000, 000, 000,
	000, 000, 000, 000, 000, 000, 000, 000, 000, 000,
	000, 000, 000, 000, 000, 000, 000, 000, 000, 000,
	000, 000, 000, 000, 000, 000, 000, 000, 000, 000,
	000, 000, 000, 000, 000, 000, 000, 000, 000, 000,
      };

/* define standard command set */

    char lexsym[LEXVALUES+1] = { E0, 'r', 'l', 'd', 'u',
      4, 21, 8, 'k', 'i', 11, '"', '\'', ',', '.', 'f', 6, 'p',
      'b', 'e', 2, 'v', 'o', '/', 's', 'g', 23, 25, 24, ';', '&',
      'm', ':', '=', 7, 9, '{', 'a', 16, '?', 'h', 20, 'c', 'j', 't',
      'n', 14, 15, 5, 18, '#', '*', '(', ')','~' };


    char autoin, dsplin, scroll, xcases, warplm, wildch, funesc, cut_mode;
    char rp[80];

    FILE *f, *fopen();
    char cupper(), clower();

  main()
  {

    SLOW int i, val, retcode;

    cls();
    remark("Standard TVX define a configuration file -- Version 2/27/86");
    BL;
    for (;;)
      {
	prompt("Enter name of file to save configuration in: ");
	reply(rp,79);
	if ((f = fopen(rp,FILEWRITE)) == 0)
	    continue;
	else
	    break;
      }


    cls();
    shoset();
    BL;
    for (;;)
      {
	BL;
	prompt("Use standard command definitions? (y/n) ");
	lreply(rp,10);
	if (*rp == 'y')
	  {
	    goto LEXDONE;
	  }
	else if (*rp == 'n')
	    break;
      }

CMDAGAIN:
    retcode = 1;		/* start with things ok */
    for (;;)
      {
	short_cls();
	shoset();
	if (retcode < 0)
	    remark("Invalid value supplied for new command.  Try again!");
	if ((retcode = set()) == 0)
	    break;
      }
    short_cls();
    shoset();
    remark("");
    prompt("Are you finished setting commands? (y/n) ");
    ureply(rp,2);
    if (*rp != 'Y')
	goto CMDAGAIN;
    

    

LEXDONE:
    for (i=0 ; i <= LEXVALUES ; ++i)	/* write out lex symbols */
      {
	fputc(lexsym[i],f);		/* write to file */
      }
	syno();
	funkeys();

    cls();
    prompt("Use autoindent (n default) (y/n): ");
    lreply(rp,10);
    if (*rp == 'y')
	fputc(1,f);
    else 
	fputc(0,f);

    BL;
    prompt("Home display line: (1-66, 16 default): ");
    rdint(&val);
    if (val > 66 || val <= 0)
	fputc(16,f);
    else
    	fputc(val,f);

    BL;
    prompt("Scroll window (0 default): ");
    rdint(&val);
    if (val > 24)
	val = 0;
    fputc(val,f);

    BL;
    prompt("Find case (e=exact,a=any, any default): "); 
    lreply(rp,10);
    if (*rp == 'e')
        fputc(1,f);
    else
        fputc(0,f);

    BL;

    prompt("Auto line wrap width (0 default): ");
    rdint(&val);
    if (val > 79)
	val = 0;
    fputc(val,f);

    BL;
    prompt("Use wild cards (y default) (y/n)? ");
    lreply(rp,10);
    if (*rp == 'n')
	fputc(0,f);
    else 
	fputc(1,f);

    BL;
    prompt("Use BACKUP.LOG file (n default) (y/n)? ");
    lreply(rp,10);
    if (*rp == 'y')
        fputc(1,f);
    else
        fputc(0,f);
    
    BL;
prompt("Use 'cut mode' (killed lines to save buffer too)? (n default) (y/n)? ");
    lreply(rp,10);
    if (*rp == 'y')
        fputc(1,f);
    else
        fputc(0,f);

#ifdef MSDOS
    BL;
    remark("The editor can recognize Ctrl-z as EOF, or it can ignore ^Z and");
    remark("just use the standard MS-DOS end of file mark.");
    prompt("Should the editor recognize Ctrl-Z as EOF? (n default) (y/n) ");
    lreply(rp,10);
    if (*rp == 'y')
        fputc(1,f);
    else
        fputc(0,f);
#endif
#ifdef GEMDOS
    fputc(1,f);		/* use ^Z on gemdos by default */
#endif

    cls();
    remark("Configuration file created.");
    fclose(f);
  }

/* =============================>>> SHOSET  <<<============================= */
  shoset()
  {  /* show repeat buffer, help if available */

    static char rp[2];
    FAST int i;
    SLOW char *cp, *msg;
    SLOW int fields, oldtty, hnum;
    SLOW unsigned tmp;

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
	{" Insert find pat",	 VINSPAT},{"nChange case      ",VFOLDCASE},
	{"/",0}			/* last variable entry */
      };

remark("Commands settable by this program:  (n => count allowed):");
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
    remark("");
remark("Note: <> repeat, @ command file, and ESCAPE cannot be changed.");
  }

/* =============================>>> SHOCOUT <<<============================= */
  shocout(c)
  char c;
  {

    if (c < ' ')
      {
	printf("^%c",c + '@');
      }
    else
	printf("%c",c);
  }

/* ===============================>>> FUNKEYS <<<========================*/
  funkeys()
  {

    SLOW int j,i,val;
    SLOW int fun;

FAGAIN:
    cls();
    remark("You may now define up to 49 function keys to be translated to");
    remark("commands OR letters.  This translation will take place before");
    remark("the editor gets the character at any level  -- thus the translation");
    remark("will apply equally to command mode and insert mode.  The translation");
    remark("assumes each function key generates a 2 character sequence.  The");
    remark("first character is an 'escape' character that must be the same for");
    remark("each key.  If the 'escape' character is really ESC, then you must");
    remark("also define one function key to have ESC as its translation value.");
    remark("When asked for the function key, simply press the key, followed by");
    remark("RETURN.  Enter either the character or decimal value of the translation.");

    for (i = 0 ; i < 50 ; ++i)
      {
	funchar[i] = funcmd[i] = 0;
      }

    BL;
    prompt("Do you want to define any function keys? (y/n) ");
    lreply(rp,10);
    if (*rp == 'n')
	goto WTFUN;

    BL;
    remark("Now, please press ANY function key so the program can identify");
    prompt("the 'function key escape' code (followed by a RETURN): ");
    reply(rp,10);
    funesc = *rp;		/* this should be the escape char */
    if (funesc == 27)
      {
	BL;
	remark("IMPORTANT:  You MUST define a function key to send an ESCAPE (decimal 27).");
	remark("If you don't, then you won't be able to end insert mode or repeat loops.");
	remark("The program doesn't have logic to make sure you do this, so don't forget!");
	BL;
      }

    for (i = 0 ; i < 50 ; ++i)
      {
FUNAGAIN:
	prompt("Press function key to define (RETURN only to exit): ");
 	rp[1] = 0;
	reply(rp,10);
	fun = rp[1];
	if (rp[1] == 0)
	    break;
	for (j = 0 ; j < 50 ; ++j)
	  {
	    if (fun == funchar[j])
	      {
		remark("That's been used already, try again.");
		goto FUNAGAIN;
	      }
	  }
	funchar[i] = fun;
	prompt("Now enter the character/command it gets translated to: ");
	reply(rp,10);
	val = getval(rp);
	funcmd[i] = val;
      }

    cls();
remark("Functions have been defined. You can start over if you made any mistakes.");
    remark("");
    prompt("Are they ok? (y/n) ");
    lreply(rp,10);
    if (*rp == 'n')
	goto FAGAIN;

WTFUN:
    for (i = 0 ; i < 50 ; ++i)
      {
	fputc(funchar[i],f);
      }
    for (i = 0 ; i < 50 ; ++i)
      {
	fputc(funcmd[i],f);
      }
    fputc(funesc,f);
  }
  

/* ===============================>>> GETVAL <<<========================*/
  getval(str)
  char *str;
  {
    /* return one byte value */

    if (*str >= '0' && *str <= '9')
	return (atoi(str));
    else
	return (*str & 0377);
  }

/* ===============================>>> SET <<<========================*/
  set()
  {
	/* set newlex[indx] to a new value */

    SLOW int val,i, oldi;

SAGAIN:
prompt("Enter current command to change (key or decimal value, RETURN to exit) ");
    reply(rp,10);
    val = clower(getval(rp));

    if (val == 0)
      {
	return 0;
      }
    for (oldi = 1 ; oldi <= LEXVALUES ; ++oldi)
      {
	if (val == lexsym[oldi])
	  {
	    goto HAVEIT;
	  }
      }
    
    return -1;	
    
HAVEIT:
    prompt("Enter NEW command (key or decimal value, RETURN to exit) ");
    reply(rp,10);
    val = clower(getval(rp));
    if (val == 0 || val == '@' || val == 27 || val == '<' || val == '>')
	return -1;
    for (i = 1 ; i <= LEXVALUES ; ++i)
      {
	if (val == lexsym[i])
	  {
	    return -1;		/* duplicate */
	  }
      }
    lexsym[oldi] = val;
    return 1;
  }

/* ===============================>>> SYNO <<<========================*/
  syno()
  {

    SLOW int j, i, valfrom, valto, found;
    
SAGAIN:
    cls();
    remark("You may now define up to 19 synonyms.  For example, you might");
    remark("want to define a space to be a synonym for right, or RETURN");
    remark("the same as down.  You must use unused values, however.  You");
    remark("can't use a existing command as a synonym.  You may enter the");
    remark("character followed by a RETURN, or the decimal value of the key.");

    for (i = 0 ; i < 20 ; ++i)
      {
	synofr[i] = synoto[i] = 0;
      }

    for (i = 0 ; i < 19 ; ++i)
      {
SYNAGAIN:
	BL;
	prompt("Enter the new synonym (RETURN when done): ");
	reply(rp,10);
	valfrom = getval(rp);
	if (valfrom == 0)
	    break;
	for (j = 1 ; j <= LEXVALUES ; ++j)
	  {
	    if (lexsym[j] == valfrom)
	      {
		remark("That is already a command! Try again.");
		goto SYNAGAIN;
	      }
	  }
	prompt("Enter the equivalent command: ");
	reply(rp,10);
	valto = getval(rp);
	for (j = 1, found = FALSE ; j <= LEXVALUES ; ++j)
	  {
	    if (lexsym[j] == valto)
	        found = TRUE;
	  }
	if (!found)
	  {
	    remark("That is not a defined command. Try again.");
	    goto SYNAGAIN;
	  }

	synofr[i] = valfrom;
	synoto[i] = valto;
      }
    cls();

remark("Synonyms have been defined. You can start over if you made any mistakes.");
    remark("");
    prompt("Are they ok? (y/n) ");
    lreply(rp,10);
    if (*rp == 'n')
	goto SAGAIN;

    for (i = 0 ; i < 20 ; ++i)
        fputc(synofr[i],f);
    for (i = 0 ; i < 20 ; ++i)
        fputc(synoto[i],f);

  }

/* ===============================>>> CLS <<<========================*/
  cls()
  {
    int i;
    for (i = 0 ; i < 25 ; ++i)
	remark("");
  }

/* ===============================>>> short_CLS <<<========================*/
  short_cls()
  {
    int i;
    for (i = 0 ; i < 10 ; ++i)
	remark("");
  }

#define EXTENDED	/* my own extended lib functions */
/* #define STANDARD	/* the set of standard functions i use */
#define LOCAL static	/* make all local globals, i think */

#ifdef EXTENDED
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
    SLOW int i;

    for (i=0 ; str[i] ; ++i)
	str[i]=clower(str[i]);

  }

/*=============================>>> PROMPT <<<================================*/
  prompt(msg)
  char msg[];
  {
    printf("%s",msg);
  }


/*=============================>>> REMARK <<<================================*/
  remark(msg)
  char msg[];
  {
    printf("%s\n",msg);
  }

/*=============================>>> UPPER  <<<================================*/
  upper(str)
  char str[];
  {
    static int i;

    for (i=0 ; str[i] ; ++i)
	str[i]=cupper(str[i]);
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
#ifdef UNIX
    gets(msg);
#else
    mreply(msg,maxc);
#endif
  }

/*=============================>>> RDINT <<<================================*/
  rdint(val)
  int *val;
  {
    char chrrep[12];
    reply(chrrep,11);
    *val = atoi(chrrep);
  }
#endif
#ifndef UNIX
/*=============================>>> MREPLY <<<================================*/
  mreply(msg,maxc)
  char msg[];
  int maxc;
  {
#define CBS 8		/* Backspace */
#define CDL1 21		/* ^U */
#define CDL2 24		/* ^X */
#define CABORT 3	/* ^C */
#define CRET 13		/* cr */
#define BACKSPACE 8

    static char ch, rp;
    static int i;


    for (i = 0 ; i < maxc ; )	/* i -> next char */
      {
	ch = ttrd_(); 	/* read the character */
	if (ch == CBS)		/* back space */
	  {
	    if (i > 0)		/* must be something to delete */
	      {
		--i;		/* wipe out char */
		ttwt_(BACKSPACE); ttwt_(' '); ttwt_(BACKSPACE);
		if (msg[i] < ' ')	/* double echo ^ chrs */
		  {
		    ttwt_(BACKSPACE); ttwt_(' '); ttwt_(BACKSPACE);
		  }
	      }
	  }
#ifdef USE_WIPE
	else if (ch == CDL1 || ch == CDL2)	/* wipe whole line */
	  {
	    i = 0;		/* set for loop ++ */
	    remark("#");
	    prompt("Re-enter? ");
	  }
#endif
	else if (ch == CABORT)
	  {
	    remark("^C");
	    prompt("Exit to operating system - are you sure? (y/n) ");
	    rp = ttrd_();
	    if (rp == 'y' || rp =='Y')
	     {
		remark("y");
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
	    return;
	  }
	else
	  {
	    msg[i++] = ch;
	    msg[i] = 0;			/* always 0 terminate */
	    if (ch < ' ')
	      {
		ch += '@';
		ttwt_('^');
	      }
	    ttwt_(ch);			/* echo char */
	  }
      } /* end for */

    remark("");
  }

/*=============================>>> ttrd_ <<<================================*/
  ttrd_()
  {
#ifdef MSDOS
	return (bdos(7,-1) & 0377);
#endif
#ifdef GEMDOS
	return (gemdos(7) & 0377);
#endif
  }

/*=============================>>> ttwt_ <<<================================*/
  ttwt_(c)
  char c;
  {
    fprintf(stderr,"%c",c);
  }
#endif
/* -------------------------- tvx_cfg.c --------------------------- */
