/* -------------------------- tvx_ptch.c --------------------------- */
#include "tvx_defs.ic"

#define EXTERN
#include "tvx_glbl.ic"

/*=======================================================================

    tvpatch - program to patch tvx with config.tvx file

	first version 6/19/84
	7/25/84 - fixed to correspond to tvx version, add extra command
	2/14/85 - version to correspond with rest
	5/15/85 - again, batched to correspond
	9/20/85 - fixed for new version of tvx, unix added

======================================================================= */
 
#ifndef UNIX
    char filein[] = "A:TVX.EXE";
    char fileout[]= "A:TEMP1.$$$";
#else
    char filein[80] = "tvx";
    char fileout[80]= "tvx_temp";
#endif
    char config[80];


/*  define our general control item structure for general patching */

#define BL remark("")
#define RMK remark
#define PR prompt
 
    char clower(), cupper();
    extern char *malloc();
    extern FILE *fopen();
    FILE *tvxin, *cfgin, *tvxout;

/* =============================>>> MAIN   <<<============================= */
  main()
  {
 
    char ans[80];

TOP:
    cls();
    RMK("TVX_PTCH - Version 11/12/85");
    BL;
RMK("  This program is used to permanently alter TVX to match the options");
RMK("selected with the TVX_CNFG program.  It will read in the configuration");
RMK("file you specify (such as CONFIG.TVX), and patch TVX to reflect those");
RMK("values.  Then you won't need to use the '-c' switch when using TVX.");
    BL;
RMK("*** You may press CONTROL-C at any time to cancel this installation. ***");

    do 
      {
	BL;
#ifndef UNIX
	PR("On which drive is TVX.EXE located? (A, B, ...): ");
	ureply(ans,10);
	filein[0] = ans[0];
#endif
	if ( !(tvxin = fopen(filein,FILEREAD)))
	  {
	    PR("TVX not found on specified drive, try again: ");
	    RMK(filein);
#ifdef UNIX
	    PR("Please enter name of tvx executable file: ");
	    reply(filein,79);
#endif
	    continue;
	  }
	fclose(tvxin);
	break;
      }
    while (1);

#ifndef UNIX
    fileout[0] = cupper(ans[0]);
#endif
 
    do 
      {
	BL;
	PR("Enter the name of the configuration file to use: ");
	reply(config,79);
	if ( !(cfgin = fopen(config,FILEREAD)))
	  {
	    RMK("Configuration not found on specified drive, try again.");
	    continue;
	  }

	rdcfg(lexsym,LEXVALUES+1);
	rdcfg(synofr,20);
	rdcfg(synoto,20);
	rdcfg(funchar,50);
	rdcfg(funcmd,50);
	rdcfg(&funkey,1);
	rdcfg(&autoin,1);
	rdcfg(&ddline,1);
	rdcfg(&dscrl,1);
	rdcfg(&dxcase,1);
	rdcfg(&wraplm,1);
	rdcfg(&use_wild,1);
	rdcfg(&usebak,1);
	logdef = usebak;
	rdcfg(&cut_mode,1);
#ifdef MSDOS
	rdcfg(&usecz,1);
#endif
	fclose(cfgin);
	break;
      }
    while (1);
 
    BL;
    RMK("TVX is being modified to match your choices.");
    RMK("This may take several minutes.");
    BL;
 
    fpatch(filein);	/* patch tvx */
 
    cls();
    RMK("Modification completed.  TVX is ready to use without the -c now.");
    BL;
  }

/* =============================>>> RDCFG <<<============================= */
  rdcfg(toset,cnt)
  char *toset;
  int cnt;
    {	/* read cnt vals from cfgin */

    FAST int i,val;

    for (i = 0 ; i < cnt ; ++i)
      {
	if ((val = fgetc(cfgin)) == EOF)
	 {
	    remark("Invalid configuration file, aborting");
	    fclose(cfgin);
	    exit(999);
	 }
	*toset++ = val;	/* replace with new commands */
      }
  }

/* =============================>>> FPATCH <<<============================= */
  fpatch(fn)
  char *fn;
  {
    static int byt;
    static int i;
    static int didpatch;
    static char *begptr;	/* patch area pointers */
 
    prompt("Patching "); remark(filein);

#ifndef UNIX
    fn[0] = fileout[0];		/* set drive */
#endif
    didpatch = FALSE;
    if (!(tvxin = fopen(fn,FILEREAD)))
      {
	PR("Unable to find file to patch: "); PR(fn);
        RMK(".  Aborting to operating system.");
	exit(999);
      }
    if (!(tvxout = fopen(fileout,FILEWRITE)))
      {
	PR("Unable to create new file, aborting: ");
	RMK(fileout);
	exit(999);
      }
 
    while ((byt = fgetc(tvxin)) != EOF)
      {
	fputc(byt,tvxout);
	if (byt == '#')			/* look for first sharp */
	  {
	    for (i = 1 ; i <= 4 ; ++i)
	      {
		if ((byt = fgetc(tvxin)) == EOF)
		    goto l900;
		fputc(byt,tvxout);	/* echo */
		if (byt != '#')
		    goto l800;
	      }
	    byt = fgetc(tvxin);		/* should be : next */
	    fputc(byt,tvxout);
	    if (byt != ':')
		goto l800;

/*   fall thru means found patch area -- code to patch follows */

	    for (begptr = (char *) &addx ; begptr < (char *) &endpatch ;
	      ++begptr)
	      {
		if ((byt = fgetc(tvxin)) == EOF) /* read byte from file */
		    goto l900;
		fputc(*begptr,tvxout); /* replace with byte from my area */
	      }
	    didpatch = TRUE;
	  }
l800:	byt = byt;		/* compiler bug */
      }


l900:
    fclose(tvxin);
    fclose(tvxout);
    if (!didpatch)
      {
	RMK("*********  ERROR ********");
	RMK("The file just checked was not a proper version of the program!");
	RMK("Please check that your are using a valid copy of the");
	RMK("program file supplied with this initialization program!");
	RMK("Unable to make patch, aborting");
	exit(999);
      }
    unlink(fn);
    fn[0] = fileout[0];		/* fix the drive */
    if (rename(fileout,fn) != 0)
      {
	RMK("Unable to rename temporary patch file");
	exit(999);
      }
  }

/* =============================>>> OK <<<============================= */
  ok(msg)
  char *msg;
  {
    char rp[11];
    PR(msg); PR(" (y/n) ");
    lreply(rp,10);
    return (rp[0] == 'y');
  }

/* ============================>>> RVALID <<<=========================== */
  rvalid(chr,okstr)
  char chr,*okstr;
  {
    /* sees if chr is in okstr */
   
    SLOW int i;
    SLOW char ch;

    ch = clower(chr);
    while (*okstr)
      {
	if (ch == clower(*okstr++))
	    return TRUE;
      }
    return FALSE;
  }
  
/* =============================>>> CLS  <<<============================= */
  cls()
  {
    int i;
 
    for (i = 0  ; i < 25 ; ++i)
	BL;
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
    FAST int i;

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
#endif
#ifdef MSDOS
    gets(msg,maxc,stdin);
#endif
#ifdef GEMDOS
    gemdos(0x0a,msg);
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
/* -------------------------- tvx_ptch.c --------------------------- */
