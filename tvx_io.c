/* ------------------------ tvx_io.c ---------------------------- */
#include "tvx_defs.ic"
#include "tvx_glbl.ic"

#define SWITCH '-'
#define FILESEP '.'

#ifdef MSDOS
#define TEMPEXT ".$$1"		/* name of temporary file */
#define BACKEXT ".BAK"		/* name of backup file */
#endif

#ifdef OSCPM
#define TEMPEXT ".$$1"		/* name of temporary file */
#define BACKEXT ".BAK"		/* name of backup file */
#endif

#ifdef GEMDOS
#define TEMPEXT ".Z1X"		/* name of temporary file */
#define BACKEXT ".BAK"		/* name of backup file */
#endif

#ifdef UNIX
#define BACKEXT ".B"		/* name of backup file */
#endif

    FILE *fopen();

/* local globals (used by tv terminal driver section) */

    static int linptr; /* common "linot" */
    static char linout[242];

    static char stemp[FNAMESIZE+1];

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	FILE IO section

   File handling algorithm:

	The original name specified on command line is called orig_file.
	It will remain untouched throughout the editing session.  At the
	very end (normal exit), it will be renamed to the ".BAK" name.

	source_file is the current name of the file with source.  It will
	orignally be the same as orig_file, but may change to a generated
	scratch name depending on the operating system.  source_file is
	always the lastest fully written version of the file (like after
	file beginning, for example). 

	work_file is the output file.  On normal exit, this is the
	file renamed to dest_file.  On buffer beginning, it will be
	moved to source_file, possibly after renameing.
	
	dest_file is the ultimate destination file.  This name is not
	actually used until the final rename on normal exit.  It is
	checked to be sure it is a valid name to be opened, however.

   +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

/* =============================>>> ABORT <<<============================= */
  abort()
  {    /* abort - close files, abort operation */

    char rply[4];

    tvclr();
    ask("Abort, are you sure? ",rply,1);
    if (clower(rply[0]) != 'y')
      {
	verify(1);
	return;
      }
    abort2();
  }

/* =============================>>> ABORT2 <<<============================= */
  abort2()
  {
    clobak();
    tvclr();

    if (!newfil)
	fclose(infile);
    if (!rdonly)
	fclose(outfile);

    if (strcmp(orig_file,source_file) != 0)
      {
	prompt("File begin used, intermediate edits in: ");
	remark(source_file);
      }
    unlink(work_file);		/* delete the work file */

    reset();
    quit();
  }

/* =============================>>> FBEG   <<<============================= */
  int fbeg()
  { /* fbeg - go back to file top */
 
    SLOW int fbegv;
 
    if (rdonly)
      {
	tverrb("Can't: R/O");	/* can't do this for read only access */
	return (FALSE);
      }

    for (wtpage(1) ; rdpage() ; wtpage(1) )	/* write out rest */
	;

    if ((! newfil))
      {
	fclose(infile);			/* close source_file */
      }
    if (usecz)
	fputc(ENDFILE,outfile);

    fclose(outfile);			/* close work_file */

/* files closed now, re-open */ 
 
    newfil = FALSE;		/* not a new file any more */

    strcpy(source_file,work_file);	/* new source file */
    temp_name(work_file,FALSE);		/* make a new temporary name */

    if (!(infile = fopen(source_file,FILEREAD)))
	goto l900;
    else
	ineof = FALSE;

    unlink(work_file);			/* get rid of previous copies */

    if (!(outfile = fopen(work_file,FILEWRITE)))
      {
	goto l900;
      }
 
    fbegv=rdpage();		/* read in new buffer */
    newscr();
    return (fbegv);

l900: tverrb("Error re-opening");
    return (FALSE);
  }
 
/* =============================>>> FILE_EXIT <<<============================= */
  file_exit()
  { /* close the input and output files, rename */
 
    SLOW int i;

    if (!newfil)		/* don't close input if new file */
      {
	fclose(infile);
      }

    while (!rdonly && !*dest_file)
      {
	remark("No name for output file has been specified.");
	prompt("Enter new name for output file: ");
	reply(dest_file,FNAMESIZE);
      }

    if (!rdonly)	/* don't close output if read only access */
      {
	if (usecz)
	    fputc(ENDFILE,outfile);
	set_mode(outfile);		/* set output mode if can */
	fclose(outfile);

    /*	orig_file has the name to be renamed to .bak
	work_file has the file name we want to be dest_name
    */
	if (strcmp(orig_file,dest_file) == 0)	/* make backup version */
	  {
	    strcpy(stemp,orig_file);
#ifndef COMMA_BAK
	    if ((i = rindx(stemp,FILESEP)) > 0)	/* see if extenstion */
		scopy(BACKEXT,0,stemp,i);		/* make .bak */
	    else
	      {
		scopy(BACKEXT,0,stemp,strlen(stemp));	/* just add on */
	      }
#else
	    i = rindx(orig_file,'/')+1;
	    scopy(".,",0,stemp,i);
	    scopy(orig_file,i,stemp,strlen(stemp));
#endif

	    unlink(stemp);			/* delete previous generation */
	    ren_file(orig_file,stemp);		/* rename the file */
	    if (!makebackup)			/* don't want to keep old one */
		unlink(stemp);	/* delete it if don't want backup file */
	  }

	if (strcmp(orig_file,source_file) != 0)	/* delete intermediate file */
	    unlink(source_file);


	while (infile = fopen(dest_file,FILEREAD))	/* output exists? */
	  {
	    fclose(infile);
	    prompt("Output file "); prompt(dest_file);
	    prompt(" already exists.  Overwrite it? (y/n) ");
	    ureply(stemp,1);
	    if (*stemp == 'Y')
	      {
		unlink(dest_file);
		break;
	      }
	    prompt("Enter new name for output file: ");
	    reply(dest_file,FNAMESIZE);
	  }

	ren_file(work_file,dest_file);		/* finally, rename last file */
      }

  }

/* =============================>>> FOPENX  <<<============================= */
  fopenx(argc,argv)
  int argc;
  char *argv[];
  {  /* open the input file
	This routine picks up file name from the user, creates a backup
	version in some appropriate manner, and opens the file for input
	and output. */
 
    SLOW int iswval, iswbeg, argnum, set_ttymode;
    SLOW char ch, tmpc;
    char rply[4];
 
    usebak = logdef;		/* if useing backup log file */

    ttymode = FALSE;		/* not in tty mode, so io ok */
    ttynext = 1000;		/* force read */

    if (argc <= 1)
      {
	remark("Usage: tvx filename [-b -i -l -o=f -r -s -t -w -# {-z -c=f}]");
#ifdef FULLHELP
	remark("");
	starthelp();		/* print start help message */
	prompt(" Options: "); remark(VERSION);
	remark("  -[no]b : backup file   -[no]i : autoindent");
	remark("  -[no]l : make command log file");
	remark("  -o=outputfile          -r : read only");
	remark("  -s : big save buff     -[no]w : word processing mode");
	remark("  -t : tty edit mode     -# : set virtual window lines to #");
#ifdef MSDOS
	remark("  -[no]z : use control-z for end of file");
#endif
#ifdef CONFIGFILE
#ifdef MSDOS
	remark("  -c=configfile        -c : use /bin/config.tvx");
#endif
#ifdef GEMDOS
	remark("  -c=configfile        -c : use /bin/config.tvx");
#endif
#ifdef OSCPM
	remark("  -c=configfile        -c : use A:config.tvx");
#endif
#endif
#ifdef UNIX
	remark("  {options not available for unix}");
#endif
#endif
	remark("");
	reset();
	quit();
      }

    newfil=				/* assume opening an old file */
    rdonly = FALSE;			/* assume not read only */
    makebackup = MAKE_BACKUP;		/* default value for make a backup */
    blimit = BUFFLIMIT;
 
    for (argnum = 1 ; argnum < argc ; ++argnum)
      {
	strcpy(stemp,argv[argnum]);	/* pick up the file name or switch */
REDO:
	if (stemp[0] == SWITCH)		/* switch in form "/R filename" only */
	  {
	    iswbeg=1;		/* start at 1 */
	    iswval = TRUE;
	    if (clower(stemp[1]) == 'n' && clower(stemp[2]) == 'o')
	      {
		iswval = FALSE ; iswbeg = 3 ;
	      }

	    ch = clower(stemp[iswbeg]);		/* get the char */
	    if (ch == 'r')		/* read only access */
		rdonly=iswval;
	    else if (ch == 'i')		/* auto indent */
		autoin = iswval;
	    else if (ch == 'w')		/* word processing mode */
	      {
		if (iswval)
		    wraplm = 70;
		else
		    wraplm = 0;
	      }
	    else if (ch == 'l')		/* log file */
		usebak = iswval;
	    else if (ch == 'b')
		makebackup = iswval;	/* make a backup file */
	    else if (ch == 'z')
		usecz = iswval;
	    else if (ch == 'o' && (stemp[iswbeg+1] == '=' ||
	      stemp[iswbeg+1] == ':'))	/* specifying output */
	      {
		if (!iswval)		/* wrong order! */
		  {
		    remark("Bad -O= switch");
		    quit();
		  }
		scopy(stemp,iswbeg+2,dest_file,0);  /* remember name */
	      }
#ifdef CONFIGFILE
	    else if (stemp[iswbeg] == 'c' && stemp[iswbeg+1] == 0) /* default cfg */
	      {
		strcpy(stemp,cfgname);
	  	goto REDO;
	      }
	    else if (stemp[iswbeg] == 'c' && (stemp[iswbeg+1] == '=' ||
	      stemp[iswbeg+1] == ':'))	/* specifying config */
	      {
		expand_name(&stemp[iswbeg+2]);
		if ((bkuin = fopen(&stemp[iswbeg+2],FILEREAD))==0)
		  {
		    remark("Can't open configuration file.");
		    continue;
		  }
		rdcfg(lexsym,LEXVALUES+1);
		rdcfg(synofr,20);
		rdcfg(synoto,20);
		rdcfg(funchar,50);
		rdcfg(funcmd,50);
		rdcfg(&funkey,1);
		rdcfg(&tmpc,1); autoin = (int) tmpc;
		rdcfg(&tmpc,1); ddline = (int) tmpc;
		rdcfg(&tmpc,1); dscrl = (int) tmpc;
		rdcfg(&tmpc,1); dxcase = (int) tmpc;
		rdcfg(&tmpc,1); wraplm = (int) tmpc;
		rdcfg(&tmpc,1); use_wild = (int) tmpc;
		rdcfg(&tmpc,1); usebak = (int) tmpc;
		logdef = usebak;
		rdcfg(&tmpc,1); cut_mode = (int) tmpc;
#ifdef MSDOS
		rdcfg(&tmpc,1); usecz = (int) tmpc;
#endif
#ifdef GEMDOS
		rdcfg(&tmpc,1); usecz = (int) tmpc;
#endif
		fclose(bkuin);
	      }
#endif
	    else if (ch == 's')	/* big save buffer */
	      {
		if (!iswval)
		    blimit=BUFFLIMIT;
		else
		    blimit=BUFFLIMIT*3;
	      }
#ifndef VTERM
	    else if (ch == 't')	/* tty mode */
		set_ttymode = iswval;	/* make a backup file */
#endif
	    else if (ch >= '0' && ch <= '9')	/* making a virtual window */
	      {
		tvlins = atoi(&stemp[iswbeg]);	/* get virtual screen size */
		if (tvlins < 3 || tvlins > tvhardlines)	/* invalid window */
		  {
		    remark("Invalid window size");
		    tvlins = tvhardlines;
		  }
		else
		  {
		    ddline = (tvlins / 2) + 1;	/* fix home line */
		    setdscrl();
		  }
	      }
	    else				/* illegal switch */
	      {
		prompt("Unknown switch -"); ttwt(ch);
		prompt(": Ignore and continue? (y/n) ");
		ureply(rply,1);
		if (*rply != 'Y')
		  {
		    reset();  quit();
		  }
	      }
	  }
	else			/* must have a file name */
	  {
	    strcpy(orig_file,stemp);
	  }
      }		/* end for */
 
/*	now open file properly - make copies to all 4 names */

GETNAME:
	while (!*orig_file)
	  {
	    ask("Edit file? ",orig_file,FNAMESIZE);
	  }

	expand_name(orig_file);		/* expand on unix */

	if (!(infile = fopen(orig_file,FILEREAD)))	/* can open? */
	  {
	    prompt("Create file "); prompt(orig_file);
	    prompt("? (y/n) ");
	    ureply(rply,1);
	    if (*rply != 'Y')
	      {
		*orig_file = 0; goto GETNAME;
	      }
	    if (*dest_file)
		remark("New file, -o= switch ignored");
	    *dest_file = 0;
	    newfil = TRUE;		/* a new file */
	    rdonly = FALSE;
	  }

/* orig_file now has the name of the source file, and it might be open */

	ineof = FALSE;
	strcpy(source_file,orig_file);	/* copy to other names */
	strcpy(work_file,orig_file);
	if (!*dest_file)		/* no -o specified */
	    strcpy(dest_file,orig_file);


	if (!newfil)			/* not new file */
	  {
	    fclose(infile);		/* close orig file */
	    if (!(infile = fopen(source_file,FILEREAD)))	/* re-open */
	      {
		remark("Internal editor error, aborting");
		exit(100);
	      }
	    get_mode(infile);		/* get mode of original file */
	  }
	else
	  {
	    *orig_file = *source_file = 0; 
	  }

/* now see if we can make an output file */
	
	if (!rdonly)
	  {
	    temp_name(work_file,TRUE);	/* make into a temporary name 1st time */
	    if ((outfile = fopen(work_file,FILEREAD)))
	      {
		/* this code is needed when the temp_name might not be
		   unique - which happens when you push (^O) and try to
		   edit a file with the same main name but perhaps a different
		   extension - the temp file will be the same, and the child
		   version of tvx will then delete the temprorary file created
		   by the parent.  This can happen again if fbeg, but let's
		   assume the 'y' applies forever.
		*/
		fclose(outfile);	/* close up the file */
		prompt("Work file already exists: ");
		remark(work_file);
		prompt("Erase it and continue with editing? (y/n) ");
		ureply(rply,1);
		if (*rply != 'Y')
		  {
		    reset();
		    exit(100);		/* abnormal exit */
		  }
	      }

	    unlink(work_file);	/* get rid if already there */

	    if (!(outfile = fopen(work_file,FILEWRITE)))
	      {
		prompt("Unable to create output work file: ");
		remark(work_file);
		if (!newfil)
		  {
		    prompt("Continue in read only mode? (y/n) ");
		    ureply(rply,1);
		    if (*rply != 'Y')
		      {
			fclose(infile);
			reset();
			exit(100);		/* abnormal exit */
		      }
		  }
		*dest_file = *work_file = 0;
		rdonly = TRUE;
	      }
	  }
	else
	  {
	    *dest_file = *work_file = 0;
	  }

    ttymode = force_tty ? TRUE : set_ttymode;	/* now safe to set ttymode */
  }

/* =============================>>> setdscrl <<<============================= */
  setdscrl()
  {	/* compute a new value for dscrl */

    if (dscrl == 0)
	return;			/* if already 0, don't change */
    dscrl = tvlins / 3;
    if ((ddline + dscrl) >= tvlins)	/* looks ugly if hits last line */
	dscrl--;
    if (dscrl < 0)		/* don't allow this */
	dscrl = 0;
  }

#ifdef CONFIGFILE
/* =============================>>> RDCFG <<<============================= */
  rdcfg(toset,cnt)
  char *toset;
  int cnt;
    {	/* read cnt vals from bkuin */

    FAST int i,val;

    for (i = 0 ; i < cnt ; ++i)
      {
	if ((val = fgetc(bkuin)) == EOF)
	 {
	    remark("Invalid configuration file, aborting");
	    fclose(bkuin);
	    quit();
	 }
	*toset++ = val;	/* replace with new commands */
      }
  }
#endif

/* =============================>>> ADDFIL <<<============================= */
  int addfil(rw)
  int rw;
  {  /* addfil - add contents of external file to save buffer
	positive means read into buffer, negative means write save buffer */

    SLOW int chr;
    SLOW int limit;

    SLOW BUFFINDEX fromch;
    SLOW int i;
    SLOW FILE *f;

    if (rw >= 0)	/* read a file */
      {
	if (!gbgcol(nxtchr))	/* gc first */
	  {
	    newscr();
	    tverrb("No save room");
	    return (FALSE);
	  }

	tvclr();
#ifdef LASL
	ask("Read external filename: ",stemp,FNAMESIZE);
#else
	ask("Yank filename: ",stemp,FNAMESIZE);
#endif

	expand_name(stemp);			/* expand on some systems */

	if (!(f = fopen(stemp,FILEREAD)) || !*stemp)
	  {
	    newscr();
#ifdef LASL
	    tverrb(" Unable to open external file ");
#else
	    tverrb(" Unable to open yank file ");
#endif
	    return (FALSE);
	 }

	savlin=0 ; savlen=0 ; nxtsav =mxbuff ;	/* clear out save buffer */

	limit = max(nxtchr,mxbuff/2) + ALMOSTOUT;
	do
	  {
	    if ((chr = getchr(f)) < 0)
	      {
		newscr();
		fclose(f);
		return (TRUE);
	      }
	    if (chr == NEWLINE)
	      {
#ifdef FILELF
		getchr(f);
#endif
		chr=ENDLINE;
		++savlin;
	      }
	    *(buff+nxtsav--) = chr;
	    if (nxtsav <= limit)
	      {
		newscr();
		tverrb("File only partly read");
		break;
	      }
	  }
	while (1);
	fclose(f);
	return (TRUE);
      }

    /* --------------- to here, then writing from save buffer --------------*/


    if (nxtsav==mxbuff)		/* nothing to save */
      {
 	tverrb("Save buffer empty!");
	return (TRUE);
      }

    tvclr();
    ask("Write to external filename: ",stemp,FNAMESIZE);

    expand_name(stemp);			/* expand on some systems */

    if (!(f = fopen(stemp,FILEWRITE)) || !*stemp)
      {
	newscr();
	tverrb(" Unable to open external file ");
	return (FALSE);
      }

    
/*   # move down line to make space for new */
    fromch = mxbuff;		/* where taking saved stuff from */
    for (i = 0 ; i < savlin ; ++i)
      {
	for ( ; ; )		/* scan save buffer */
	  {
	     if ((chr = *(buff+fromch--)) == ENDLINE)
	      {
		fputc(NEWLINE,f);
#ifdef FILELF
		fputc(LF,f);
#endif
		break;
	      }
	    else
		fputc(chr,f);
	  }
      }

    if (usecz)
	fputc(ENDFILE,f);
    fclose(f);
    newscr();
    return (TRUE);

  }

/*=============================>>> SCOPY  <<<================================*/
  scopy(old,oldbeg,new,newbeg)
  char old[], new[];
  int oldbeg,newbeg;
  {
    while (old[oldbeg])
	new[newbeg++]=old[oldbeg++];
    new[newbeg] = 0;
  }

/* **************************************************************************

	Following code is for non-unix systems

 **************************************************************************** */
#ifndef UNIX
/* =============================>>> get_mode <<<============================= */
  get_mode(f)
  FILE *f;
  {		/* gets access mode of open file f */
  }

/* =============================>>> set_mode <<<============================= */
  set_mode(f)
  FILE *f;
  {		/* sets access mode of open file f */
  }

/* ==========================>>> expand_name <<<============================ */
  expand_name(n)
  char *n;
  {		/* expands unix file names */
  }

/* =============================>>> ren_file <<<=========================== */
  ren_file(old,new)
  char *old, *new;
  {
#ifndef GEMDOS
    if (rename(old,new) != 0)
      {
	prompt(old) ; prompt(" not renamed to "); remark(new);
      }
#endif
#ifdef GEMDOS
    gemdos(0x56,0,old,new);	/* can't find C version */
#endif
  }

/* =============================>>> temp_name <<<=========================== */
  temp_name(n,first)
  char *n;
  int first;
  {
	/* generates a temporary name from n.  Depending on value of
	   first, it will either add a 1 or 2 to name */

    SLOW int i;

    if (first)
      {
	if ((i = rindx(n,FILESEP)) > 0)	/* see if extenstion */
	    scopy(TEMPEXT,0,n,i);		/* make .bak */
	else
	  {
	    scopy(TEMPEXT,0,n,strlen(n));	/* just add on */
	  }
      }
    else
      {
	i = strlen(n);
	if (n[i-1] == '1')
	    n[i-1] = '2';
	else
	    n[i-1] = '1';
      }
  }
#endif

/* **************************************************************************

	This section is for the version supporting command logfile
	backup.  The code necessary for this version is included here,
	and may be compiled by defining VB to be a blank.

 **************************************************************************** */
 
/* =============================>>> OPNBAK <<<============================= */
  opnbak()
  { 
	/* opnbak - open the backup log file
	   if VB defined as ' ', then backup version created */
 
#ifdef VB
 
    if (! usebak)
      {
	bakflg = FALSE;
	return;
      }

    bkuout = fopen(BACKUPNAME,FILEWRITE);
    bakpos = 1;
#endif
 
  }
 
/* =============================>>> PUTBAK <<<============================= */
  putbak(chr)
  char chr;
  { /* putbak - put a character into the backup file */
 
#ifdef VB
    static char copy;

    if (! usebak)
	return;
    copy=chr;
    if (copy < 32 || copy == '@' || copy==delkey)
      {
	fputc('@',bkuout);
	bakcrlf();
	if (copy < 32)
	    copy += '@';
	else if (copy==delkey)
	    copy = '?'; 	/* let @? be rubout */
      }
    fputc(copy,bkuout);
    bakcrlf();
#endif
  }
 
#ifdef VB
/* =============================>>> BAKCRLF <<<============================= */
  bakcrlf()
  {    /* conditionally put a cr/lf to backup file */

    if (++bakpos > 63)
      {
	fputc(NEWLINE,bkuout);
#ifdef FILELF
	fputc(LF,bkuout);
#endif
	bakpos = 1;
      }
  }
#endif

/* =============================>>> CLOBAK <<<============================= */
  clobak()
  {

#ifdef VB
    if (! usebak)
	return;
    fputc(NEWLINE,bkuout);
#ifdef FILELF
    fputc(LF,bkuout);
#endif
    if (usecz)
	fputc(ENDFILE,bkuout);

    fclose(bkuout);
#endif
  }
 
/* =============================>>> GETBAK <<<============================= */
  getbak(chr)
  char *chr;
  {  /* get one char from back up file if there */

#ifdef VB
    SLOW int ich;

l10:
    if ((ich = getchr(bkuin)) < 0 || ich == ENDFILE)
      {
l15:	fclose(bkuin);
	*chr=0;			/* harmless character */
	bakflg=FALSE;
	verify();
	return;
      }
    if (ich == NEWLINE)
	goto l10;
#ifdef FILELF
    if (ich == LF)
	goto l10;
#endif
    *chr=ich;
    if (ich=='@')
      {
l20:    if ((ich = getchr(bkuin)) < 0 || ich == ENDFILE)
	  {
	    goto l15;
	  }
	if (ich == NEWLINE)
	    goto l20;
#ifdef FILELF
	if (ich == LF)
	    goto l20;
#endif
	*chr=ich;
	if (ich == '?')
	    *chr=delkey;
	else if (*chr != '@')
	    *chr= ich - '@';
      }
#endif
  }
 
/* =============================>>> OPNATF <<<============================= */
  opnatf()
  { /* open an indirect command file */
 
#ifdef VB

    tvclr();
 
    ask("Name of command file: ",stemp,FNAMESIZE);
		/* read in the file name from the terminal */

    expand_name(stemp);

    if (!*stemp)
      {
	verify();
	return;
      }

    if (!(bkuin = fopen(stemp,FILEREAD)))
      {
	verify();
	tverrb("Bad @ name");
	return;
      }
    bakflg=TRUE;
    newscr();
#endif
  }

/* **************************************************************************

	This section contains code to write and read buffers of data

 **************************************************************************** */

/* =============================>>> RDPAGE <<<============================= */
  int rdpage()
  { /* rdpage - read in file up to buffer limit
       only place text read from edited file */
 
    SLOW int chr;
    SLOW int l,newlns;
#ifdef GETSIO
    char inbuff[256];
    FAST char *bp;	/* ptr to inbuff */
    SLOW int do_read;	/* flag if need to read */

    do_read = TRUE;	/* need to do read first time */
#endif
 
    if (newfil)		/* can't read in when a new file */
      {
	return (FALSE);
      }
    if (nxtlin > mxline || nxtchr > mxbuff-130)	/* error check */
      {
	tverrb("Lines filled ");
	return (FALSE);
      }
 
    newlns=0;			/* begin at the beginning */

    while (mxline-nxtlin > LINELIMIT  && nxtsav-nxtchr > blimit && !ineof)
      { 			/* read in while have room */
#ifdef GETSIO
	if (do_read)
	  {
	    if (fgets(inbuff,255,infile) == NULL)
	      {
		ineof = TRUE;
		break;
	      }
	    do_read = FALSE;	/* a line has been read */
	    bp = inbuff;	/* point to beginning of buffer */
	  }
	chr = *bp++;		/* "read" the character */
#else
	if ((chr = fgetc(infile)) == EOF)
	  {
	    ineof = TRUE;
	    break;
	  }
#endif

	if (chr == ENDFILE && usecz)
	  {
	    ineof = TRUE;
	    break;
	  }
#ifdef FILELF
	if (chr == LF)
	    continue;
#endif
	*(buff+nxtchr) = BEGLINE;
	*(lines+nxtlin) = nxtchr++;
	++newlns ;
	    
	while (chr != NEWLINE)		/* store a line */
	  {
	    *(buff+nxtchr++) = chr;
#ifdef GETSIO
	    chr = *bp++;		/* "read" the character */
#else
	    if ((chr = fgetc(infile)) == EOF)
	      {
		ineof = TRUE;
		break;
	      }
#endif
	    if (chr == ENDFILE && usecz)
	      {
		ineof = TRUE;
		break;
	      }
	  }	/* end of while != NEWLINE */
#ifdef GETSIO
	do_read = TRUE;
#endif

	*(buff+nxtchr++) = ENDLINE;
	++nxtlin;
      }

    if (nxtlin > 1)		/* we really read something */
      {
	curlin=1;		/* point to top of char */
	curchr = *(lines+1)+1;	/* point to first character */
      }
    return (newlns > 0) ;
  }

/* =============================>>> WTPAGE <<<============================= */
  wtpage(whow)
  int whow;
  { /* wtpage - write out contents of text buffer, and clear line list */
 
    FAST int i;
    FAST char *chrp;
    SLOW char *lim;
    SLOW int wlimit;
#ifdef GETSIO
    char outbuff[256];
    FAST char *bp;	/* ptr to outbuff */
    SLOW int buff_len;
#endif
 
    if (whow < 0)		/* allow writing partial buffer */
	wlimit = curlin - 1;
    else
	wlimit = nxtlin -1;

    if (nxtlin <= 1 || rdonly)
      {
	tverr("Empty buffer");
	goto zapb;
      }

    if (whow < 0)
	tverr("Writing partial buffer");
    else
	tverr("Writing buffer");
 
    tvhdln();
 
    for (i = 1 ; i <= wlimit ; ++i)
      {
	chrp = buff + (*(lines+i)+1);	/* ptr to first char of line */
#ifdef GETSIO
	bp = outbuff;			/* pt to buffer */
	buff_len = 0;
	while (*chrp != ENDLINE && buff_len < 253)
	  {
	    *bp++ = *chrp++;		/* copy character */
	  }
	*bp++ = NEWLINE;
#ifdef FILELF
	*bp++ = LF;
#endif
	*bp = 0;			/* end of string */
	fputs(outbuff,outfile);		/* and write all at once */
#else
	while (*chrp != ENDLINE)
	  {
	    fputc(*chrp++, outfile);
	  }
	fputc(NEWLINE,outfile);
#ifdef FILELF
	fputc(LF,outfile);
#endif

#endif
      }

zapb:

    if (whow < 0)
      {
	killin(-(curlin-1));	/* kill to top of buffer */
	if (!gbgcol(nxtchr))	/* gc first */
	  {
	    newscr();
	    tverrb("Warning: no extra room created");
	    return (FALSE);
	  }
	return (TRUE);
      }
    else
      {
	lim = buff + nxtsav;
	for (chrp=buff ; chrp < lim ; *chrp++ = GARBAGE)
	    ;
	tvdlin =			/* start on first line again */
	nxtlin =			/* reset to initial state */
	nxtchr = 1;
	curchr =
	curlin=0;
	return (TRUE);
      }
  }

/* **************************************************************************

    This section contains misc. stuff likely to be operating system dependent

 **************************************************************************** */

/* ===========================>>> OPSYSTEM <<<============================== */
  opsystem()
  {
#ifdef MSDOS			/* !!! cii-86 dependent */

    char rp[80];

MS_AGAIN:
    tvclr();
    ask("DOS command (any key to resume edit when done): ",rp,79);
    remark("");
    if (system(rp) != 0)
      {
    	tvxy(1,1);
	ask("Sorry, but couldn't find COMMAND.COM.",rp,1);
      }
    else
      {
	tvxy(1,1);
	ask("",rp,1);
	if (*rp == '!')
	   goto MS_AGAIN;
      }
    verify(1);
#endif
#ifdef UNIX
    unix_sys();
#endif
#ifdef GEMDOS
    return;
#endif
  }

#ifndef UNIX
/* ===========================>>> TTINIT <<<============================== */
  ttinit()
  { /*  this routine could be used to set up keyboard input, perhaps
	turning on non-echoed input */
    return;
  }

/* ===========================>>> TTCLOS <<<============================== */
  ttclos()
  {  /* this routine could undo anything ttinit() did */
    return;
  }
#endif

#ifndef VTERM
/* ===========================>>> TTRD <<<============================== */
  ttrd()
  { /* this routine is called to read one unechoed char from the keyboard */

  static int tc, i;
  static char chr;

RDTOP:
    if (ttymode)
	tc = rdtty();		/* get a char from the tty */
    else
      {

#ifdef OSCPM
    while (!(tc = bdos(6,-1)))		/* cp/m implementation */
	;
#endif
#ifdef MSDOS
    tc = bdos(7,-1);		/* ms-dos implementation  (!!! cii-86) */
#endif
#ifdef GEMDOS
    tc = gemdos(7);		/* GEMDOS application */
#endif
#ifdef UNIX
    tc = ttrd_unix();
#endif
       }

    chr = tc & 0377;

    if (chr == funkey)			/* function key */
      {
	if (ttymode)
	  {
	    tc = rdtty();		/* get a char from the tty */
	  }
	else
	  {
#ifdef OSCPM
	while (!(tc = bdos(6,-1)))		/* cp/m implementation */
	    ;
#endif
#ifdef MSDOS
	tc = bdos(7,-1);		/* ms-dos implementation */
#endif
#ifdef GEMDOS
    tc = gemdos(7);		/* GEMDOS application */
#endif
#ifdef UNIX
	tc = ttrd_unix();
#endif
	  }
	chr = tc & 0377;
	for (i = 0 ; i < 50 && funchar[i] ; ++i)
	  {
	    if (chr == funchar[i])
	      {
		tc = funcmd[i] & 0377;
		return (tc);
	      }
	  }
	goto RDTOP;			/* ignore invalid function keys */
      }
    tc = chr & 0377;
    return (tc);

  }
#endif

#ifndef UNIX
/* ===========================>>> TTWT <<<============================== */
  ttwt(chr)
  char chr;
  { /*  this routine is called to write one char to the keyboard
	It also interprets print direction */

    if (ttymode)
	return;
    dispch(chr);	/* cp/m, ms-dos version */
    if (useprint)
	printc(chr);
  }
#endif

#ifdef MSDOS
#define DEFGETCHR
#endif

#ifdef OSCPM
#define DEFGETCHR
#endif

#ifdef GEMDOS
#define DEFGETCHR
#endif

#ifdef DEFGETCHR
/* ===========================>>> GETCHR <<<============================== */
  getchr(filnum)
  FILE *filnum;
  {  /* get a character from filnum */

#define EOFBYTE 26

    FAST int ichr;

    if (((ichr = fgetc(filnum)) == EOFBYTE))
      {
	if (usecz)
	    return (EOF);
      }

    return (ichr);
  }
#endif

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    TVX TERMINAL DRIVER  for various terminals

   +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
 

/* =============================>>> TRMINI <<<============================= */
  trmini()
  {  /* initialize term if necessary */

    sendcs(cinit);
    tvclr();
  }

/* =============================>>> reset <<<============================= */
  reset()
  {
    sendcs(cendit);
    ttclos();
  }

/* =============================>>> ttyverify <<<============================= */
  ttyverify(knt)
  int knt;
  {
    SLOW BUFFINDEX oldline, oldchr, limit;		/* current position */

    oldline = curlin; oldchr = curchr;	/* remember where we were */

    ttymode = FALSE;			/* enable output stuff */

    if (knt < 0)			/* type some above */
      {
	curchr = 0;
	curlin = curlin + knt ;		/* back n lines */
	if (curlin < 1)
	   curlin = 1;
	while (curlin < oldline)	/* write out the lines */
	    ttyline(curlin++);	/* write line, no cursor */
      }
    else
      {
	ttyline(curlin);		/* type current line */
	curchr = 0;			/* this turns off cursor */
	limit = oldline + knt - 1;
	if (limit >= nxtlin)
		limit = nxtlin - 1;
	while (++curlin <= limit)
	    ttyline(curlin);
      }
    curchr = oldchr;
    curlin = oldline;
    ttymode = TRUE;
  }

/* =============================>>> ttyline <<<============================= */
  ttyline(linenr,cursor)
  BUFFINDEX linenr;
  {
    SLOW BUFFINDEX chrc;
    SLOW int outlen;
    
    chrc = *(lines+linenr)+1;	/* point to first character in line */
    outlen = 0;			/* nothing out yet */
    for ( ; ; )
      {
	if (chrc == curchr)	/* at cursor */
	  {
	    outlen += 2;
	    if (outlen > 78)	/* line wrap */
	      {
		remark("");
		ttwt('_');
		outlen = 3;
	      }
	    ttwt('/'); ttwt('\\');
	  }
    	if (*(buff+chrc) == ENDLINE)	/* done */
	    break;
	outlen += ttywtch(*(buff+chrc));	/* watch for line wrap */
	if (outlen > 78)
	  {
	    remark("");
	    ttwt('_');
	    outlen = 1;
	  }
	++chrc;			/* next character */
      }
    remark("");
  }
  
/* =============================>>> ttywtch <<<============================= */
  ttywtch(chr)
  char chr;
  {
    if (chr >= ' ')		/* regular character */
      {
	ttwt(chr);
	return 1;
      }
    else			/* control character */
      {
	ttwt('^');
	ttwt(chr+'@');
	return 2;
      }
  }
  
/* =============================>>> rdtty <<<============================= */
  rdtty(knt)
  int knt;
  {		/* fake rdtt for ttymode - only called when in ttymode */

#define RDBUFFSIZE 81
    static char rdtbuf[RDBUFFSIZE];

RDTOP:
    ttymode = FALSE;			/* take out of ttymode for echo */
    if (ttynext >= RDBUFFSIZE)		/* need to read a line */
      {
	if (ins_mode)			/* different prompts for modes */
	    prompt("+");
	else
	    prompt("tvx>");
	reply(rdtbuf,80);		/* read a line */
	ttynext = 0;			/* reset pointer */
      }
    ttymode = TRUE;			/* no echo again */
    if (rdtbuf[ttynext] == 0)		/* end of buffer */
      {
	ttynext = 1000;
	if (ins_mode)
	    return (CR);		/* return a carriage return for ins */
	else
	    goto RDTOP;			/* read another line */
      }
    else
      {
	return (rdtbuf[ttynext++]);	/* return character */
      }
  }

/* =============================>>> TVPLIN <<<============================= */
  tvplin(chrptr)
  BUFFINDEX chrptr;
  { /* tvplin - put line beginning at chrptr
		will only type what will fit on screen (using xout) */
 
    SLOW char tmp;
    SLOW int linlen, origx;
    SLOW BUFFINDEX i;
 
#ifdef ULBD
    SLOW int ul, bd, useul, usebd;

    ul = bd = useul = usebd = FALSE;
#endif

    last_col_out = linptr = 0;
    origx = xoutcm;			/* save x so can get true linelen */
    for (i=chrptr; *(buff+i)!=ENDLINE && xoutcm <= 240; ++i)
      {
#ifdef NO_EXTEND_CHAR
	if ((*(buff+i) < ' ' && *(buff+i) >= 0) || (*(buff+i) & 0x80) )
		/* control character? */
#else
	if (*(buff+i)<' ' && *(buff+i) >= 0)	/* control character? */
#endif
	  {
	    if (*(buff+i) == TAB)
	      {
		if (tabspc > 0)
		  {
		    do 
		      {
			linout[linptr++] = ' ';	/* replace with blanks */
			++xoutcm;
		      }
		    while ( ((xoutcm-1) % tabspc) != 0);
		  }
		else
		  {
		    linout[linptr++] = '^';
		    linout[linptr++] = 'I';
		    xoutcm += 2;
		  }
		continue;
	      }
	    else		/*  other control character */
	      {
		linout[linptr++] = (*(buff+i) & 0x80) ? '~' : '^';
		++xoutcm;
		if (xoutcm==tvcols && *(buff+i) != ENDLINE)
		    continue;

/*  #$$$	ascii machines!!!! */
		tmp = *(buff+i);
		if ((tmp &= 0x7f) < ' ')	/* ok to mix extended, ctrl */
		    tmp += '@';
		linout[linptr++]=tmp;

#ifdef ULBD
		if ( *(buff+i)==TOGUNDERLINE && cundlb[0] != 0)
		  {
		    if (ul)
		      {
			strcopy(cundle,0,linout,&linptr);
			ul = FALSE;
		      }
		    else
		      {
			strcopy(cundlb,0,linout,&linptr);
			useul = TRUE;
			ul = TRUE;
		      }
		  }
		if (*(buff+i) == TOGBOLD && cboldb[0] != 0)
		  {
		    if (bd)
		      {
			strcopy(cbolde,0,linout,&linptr);
			bd = FALSE;
		      }
		    else
		      {
			strcopy(cboldb,0,linout,&linptr);
			usebd = TRUE;
			bd = TRUE;
		      }
		  }
#endif		  
	      }
	  } /*# end if control character */
	else 
	  {
	    linout[linptr++] = *(buff+i);
	  }
	++xoutcm;
      }

    if (*(buff+chrptr-1)==BEGLINE)		/* write whole line */
      {
	last_col_out = linlen = min(tvcols,linptr-leftmg+1);
	if (linlen > 0)
	  {
	    tvlout(&linout[leftmg-1],linlen);
	  }
      }
    else
      {
	linlen = min(tvcols-origx+1,linptr);
	last_col_out = linlen + origx - 1;
	if (linlen > 0)
	    tvlout(linout,linlen);
      }
#ifdef ULBD
    if (useul)
	sendcs(cundle);
    if (usebd)
	sendcs(cbolde);
#endif
	
  }
 
/* =============================>>> TVLOUT <<<============================= */
  tvlout(chrbuf,lenbuf)
  char chrbuf[];
  int lenbuf;
  {  /* tvlout - intercepts tvcout calls to use line I/O */
 
    if (!(echof && !bakflg))
	return;
    ttwtln(chrbuf,lenbuf);	/* write out whole line */
  }
 
/* =============================>>> TVTYPE <<<============================= */
  tvtype(ibeg,icnt)
  int ibeg,icnt;
  { /* tytype - type icnt lines starting at lines[ibeg]
		no cr/lf on the last line */
 
    FAST int i,lim;
    SLOW BUFFINDEX start;
 
    if (!echof)
	return;
    xoutcm=tvx;
    lim = ibeg+icnt-1;

    for (i = ibeg ; i<=lim && i<nxtlin ; ++i)
      {
	start = (*(lines+i))+1;
	tvplin(start);	/* type out a line */
	xoutcm=1;
	if (celin[0] && last_col_out < tvcols)
	    tvelin();	/* erase rest of line */
	if ( i != lim )
	  {
	    tvcout(CR);
#ifdef USELF
	    tvcout(LF);
#endif
	  }
      }
#ifdef SCR_BUF
    ttflush();
#endif
  }

/* =============================>>> SCRPRINT <<<============================= */
  scrprint()
  {	/* print screen on printer */

#ifndef UNIX
 
   SLOW beg, cnt;

    tvclr();		/* clear screen first */
    finddl(&beg, &cnt);
    useprint = TRUE;	/* enable printing */
    tvtype(beg,cnt);	/* and display/print */
    printc(CR);		/* force closing cr/lf */
#ifdef USELF
    printc(LF);
#endif
    useprint = FALSE;
#endif
    verify(1);		/* reset screen */
  }
 
/* =============================>>> VERIFY <<<============================= */
  verify(knt)
  int knt;
  { /* verify - rewrite the screen or type current line with cursor */

    SLOW int xf;
 
    if (ttymode)
	ttyverify(knt);
    else
      {
	newscr();
	xf = findx();
	tvxy(xf,tvy);	/* reset cursor to current position */
      }
#ifdef SCR_BUF
    ttflush();
#endif
  }

/* =============================>>> CSRCMD <<<============================= */
  csrcmd()
  {
    ins_mode = FALSE;		/* let world know in command mode */
    sendcs(ccsrcm);
#ifdef SCR_BUF
    ttflush();
#endif
  }

/* =============================>>> CSRINS <<<============================= */
  csrins()
  {
    SLOW int oldx,oldy,oldxot;

    ins_mode = TRUE;		/* in insert mode */
    sendcs(ccsrin);

    if (tvdlin != tvhardlines)
      {
    	oldx = tvx; oldy = tvy; oldxot = xoutcm;
	tvmsg("### Insert Mode ###",FALSE);
	tvxy(oldx,oldy);
	xoutcm = oldxot;
      }
#ifdef SCR_BUF
    ttflush();
#endif
  }
  
/* **************************************************************************

   tv screen primitives follow

*************************************************************************** */
 
/* =============================>>> TVBOTB <<<============================= */
  tvbotb(n)
  int n;
  {  /* tvbotb - make n blank lines at the bottom of the screen */
 
    FAST int i,j;
 
/*  All versions  control sequences */
 
    if (n >= tvlins)
      {
	tvclr();
      }
    else 
      {
	tvxy(1,tvhardlines);	/* go to real last line */
	for (i = 1 ; i <= n ; ++i)	/* and write n blank lines */
	  {
	    sendcs(cbotb);
	    if (dsp_mem)
		tvelin();	/* might scroll non-blank line */
	  }
	j=tvlins-n+1;	/* home to virtual last line */
	tvxy(1,j);	/* position at first new blank line */
      }
#ifdef SCR_BUF
    ttflush();
#endif
  }
 
/* =============================>>> TVCLR  <<<============================= */
  tvclr()
  {  /* tvclr - clear the entire screen and home */
 
    if (cclears[0])
	sendcs(cclears);
    else
      {
	tvxy(1,1);
	tvescr();
      }
#ifdef SCR_BUF
    ttflush();
#endif
  }
 
/* =============================>>> TVCOUT <<<============================= */
  tvcout(chr)
  char chr;
  {  /* tvcout - send one character to the terminal */
 
    if (echof && !bakflg)
	ttwt(chr);
  }
 
/* =============================>>> TVELIN <<<============================= */
  tvelin()
  {  /* tvelin - erase the rest of the current line */
 
    sendcs(celin);
  }
 
/* =============================>>> TVESCR <<<============================= */
  tvescr()
  {  /* tvescr - erase from current cursor position to end of screen */
 
    SLOW int oldx,oldy;
    FAST int i;

    if (cescr[0])
	sendcs(cescr);
    else
      {
	oldx = tvx ; oldy = tvy ;
	tvelin();
	for (i = oldy+1 ; i <= tvhardlines ; ++i)
	  {
	    tvxy(1,i);
	    tvelin();
	  }
	tvxy(oldx,oldy);
      }
  }
 
/* =============================>>> TVINSL <<<============================= */
  tvinsl()
  {  /* tvinsl - insert line, handle virtual screen size */
 
    SLOW int oldx,oldy;
    FAST int i;

    oldx = tvx ; oldy = tvy ;
    sendcs(ciline);
    if (tvlins != tvhardlines)
      {
	tvxy(1,tvlins+1);	/* kill scrolled down line */
	tvelin();
	tvxy(oldx,oldy);
      }
  }
 
/* =============================>>> TVTOPB <<<============================= */
  tvtopb(n)
  int n;
  {  /* tvtopb - create n blank lines at the top of the screen */
 
    FAST int i;

    if (! ctopb[0])
	return;
    tvxy(1,1);		/* home first */
    if ( n >= tvlins)
	tvescr();	/* simply erase the screen */
    else
      {
	for (i = 1 ; i <= n ; ++i)
	  {
	    sendcs(ctopb);
	    if (dsp_mem)		/* non blank line might be scrolled */
		tvelin();
	  }
	if (tvlins != tvhardlines)
	  {
	    tvxy(1,tvlins+1);	/* kill scrolled down line */
	    tvelin();
	    tvxy(1,1);
	  }
      }
  }
 
/* =============================>>> TVXY   <<<============================= */
  tvxy(ix,iy)
  int ix,iy;
  {  /* tvxy - position cursor at position x,y 
		x=0 is left most column
		y=0 is top most line	*/
 
#ifdef TERMCAP			/* TERMCAP different */

    tvx=ix;
    tvy=iy;
    tcapxy(ix,iy);		/* call termcap version of xy */

#else				/* generic version of tvxy */

    SLOW int x,y, coord1, coord2;
    FAST int i;
    SLOW char chrrep[4];
 
    x = min(ix+addx,tvcols+addx);	/* column is addx */
    y = iy+addy;			/* same for row */
    tvx = ix;
    tvy = iy;

    sendcs(cxybeg);		/* opening control sequence */
    if (cxy1st == 'l')
      {
	coord1 = y ; coord2 = x;
      }
    else
      {
	coord1 = x ; coord2 = y;
      }

    if (cxychr)
      {
	itoa(coord1,chrrep);
	sendcs(chrrep);
      }
    else
	tvcout(coord1);

    sendcs(cxymid);		/* middle control sequence */

    if (cxychr)
      {
	itoa(coord2,chrrep);
	sendcs(chrrep);
      }
    else
	tvcout(coord2);

    sendcs(cxyend);		/* send terminating sequence */

#endif				/* end of gerneric version */
  }

/* =============================>>> SENDCS <<<============================= */
  sendcs(cs)
  char cs[];
  {	/* send a control sequencs to terminal */

    FAST int i;

#ifndef UNIX

    for (i = 0 ; cs[i] ; ++i)
	tvcout(cs[i]);
#else				/* unix version */

#ifdef TERMCAP			/* termcap uses special output */
    tcapcs(cs);			/* send control string to termcap */
#else
    i = strlen(cs);
    tvlout(cs,i);
#endif				/* terminal specific unix version */

#endif				/* end of unix version */
  
  }

/* =============================>>> GKBD   <<<============================= */
  gkbd(chr)
  char *chr;
  {  /* gkbd - get one character from the keyboard */
 
#ifdef VB
    if (!bakflg)
      {
#endif
	do 
	  {
	    *chr = ttrd();	/* read only if non-backup version */
	  }
	while (*chr == 0);	/* ignore EOS character */
#ifdef VB
      }
    else
	getbak(chr);
    putbak(*chr);	/* save to backup file */
#endif
  }

#ifndef UNIX
/* =============================>>> TTWTLN <<<============================= */
  ttwtln(chrbuf,len)
  char chrbuf[];
  int len;
  {  /*  write one line to terminal, generic version, unix uses its own */
 
    FAST int i;

#ifndef GEMDOS
    for (i = 0 ; i < len ; i++)
	ttwt(chrbuf[i]);
#else
    char oldc;
    oldc = chrbuf[len];		/* I'm not sure just who calls ttwtln */
    chrbuf[len] = 0;		/* so be safe, be sure 0 terminated */
    gemdos(9,chrbuf);		/* gemdos write line to terminal */
    chrbuf[len] = oldc;		/* restore, just in case */
#endif
  } 
#endif

#ifdef OSCPM
/* ===========================>>> DISPCH <<<============================== */
  dispch(chr)
  char chr;
  {

	bdos(2,chr);	/* cp/m, ms-dos version */
  }
/* =============================>>> USER_1 <<<============================= */
  user_1(knt)
  int knt;
  {
    return (TRUE);
  }

/* =============================>>> USER_2 <<<============================= */
  user_2(knt)
  int knt;
  {
    return (TRUE);
  }
#endif

#ifdef MSDOS
#ifndef IBMPC
/* ===========================>>> DISPCH <<<============================== */
  dispch(chr)
  char chr;
  {

	bdos(2,chr);	/* cp/m, ms-dos version */
  }
#endif
/* =============================>>> USER_1 <<<============================= */
  user_1(knt)
  int knt;
  {
    return (TRUE);
  }

/* =============================>>> USER_2 <<<============================= */
  user_2(knt)
  int knt;
  {
    return (TRUE);
  }
#endif

#ifdef GEMDOS
/* ===========================>>> DISPCH <<<============================== */
  dispch(chr)
  char chr;
  {

	gemdos(2,chr);	/* cp/m, ms-dos version */
  }

/* =============================>>> USER_1 <<<============================= */
  user_1(knt)
  int knt;
  {
	/* toggle screen res */

    if (tvhardlines > 25)	/* already in 50 line mode */
      {
	if (rez25())		/* make sure not color */
	  {
	    tvhardlines = tvlins = 25;
	    ddline = 13;
	  }
      }
    else			/* in 25 line mode */
      {
	if (rez50())		/* make sure not color */
	  {
	    tvhardlines = tvlins = 50;
	    ddline = 26;
	  }
      }

    setdscrl();			/* reset scroll region */
    tvidefs();			/* reset defaults */
    verify(1);
    return (TRUE);
  }

/* =============================>>> USER_2 <<<============================= */
  user_2(knt)
  int knt;
  {
    return (TRUE);
  }
#endif
/* ------------------------ tvx_io.c ---------------------------- */
