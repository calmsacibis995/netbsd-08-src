/* prtinf.c
   Routines to gather information about ports and dialers from
   configuration files.

   Copyright (C) 1991, 1992 Ian Lance Taylor

   This file is part of the Taylor UUCP package.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   The author of the program may be contacted at ian@airs.com or
   c/o AIRS, P.O. Box 520, Waltham, MA 02254.

   $Log: prtinf.c,v $
   Revision 1.1.1.1  1993/03/21 09:53:13  cgd
   initial import of 386bsd-0.1 sources

   Revision 1.20  1992/04/02  22:51:09  ian
   Add gcc 2.0 format checking to ulog, and fixed discovered problems

   Revision 1.19  1992/03/28  21:47:55  ian
   David J. MacKenzie: allow backslash to quote newline in config files

   Revision 1.18  1992/03/10  21:47:39  ian
   Added protocol command for ports

   Revision 1.17  1992/03/08  04:56:21  ian
   Peter da Silva: added ``lockname'' command for ports

   Revision 1.16  1992/03/03  06:06:48  ian
   T. William Wells: don't complain about missing configuration files

   Revision 1.15  1992/02/08  03:54:18  ian
   Include <string.h> only in <uucp.h>, added 1992 copyright

   Revision 1.14  1992/01/14  04:04:17  ian
   Chip Salzenberg: strcmp is a macro on AIX

   Revision 1.13  1992/01/11  17:11:11  ian
   Hannu Strang: avoid compiler bug by not using -> in address constant

   Revision 1.12  1991/12/22  20:57:57  ian
   Added externs for strcasecmp or strncasecmp

   Revision 1.11  1991/12/17  23:14:08  ian
   T. William Wells: allow dialer complete and abort to be chat scripts

   Revision 1.10  1991/12/17  17:08:02  ian
   Marc Unangst: allow true and false for boolean strings as documented

   Revision 1.9  1991/12/15  03:42:33  ian
   Added tprocess_chat_cmd for all chat commands, and added CMDTABTYPE_PREFIX

   Revision 1.8  1991/12/09  18:26:33  ian
   Richard Todd: handle "#" command generated by multiple dialer files

   Revision 1.7  1991/12/02  21:25:36  ian
   Niels Baggesen: protocol-parameter for ports and dialers didn't work

   Revision 1.6  1991/11/30  22:55:03  ian
   Marty Shannon: blew up if more than one port in port file!

   Revision 1.5  1991/11/14  03:20:13  ian
   Added seven-bit and reliable commands to help when selecting protocols

   Revision 1.4  1991/11/13  20:38:00  ian
   Added TCP port type for connections over TCP

   Revision 1.3  1991/11/11  23:47:24  ian
   Added chat-program to run a program to do a chat script

   Revision 1.2  1991/11/11  16:59:05  ian
   Eliminate fread_port_info, allow NULL pflock arg to ffind_port

   Revision 1.1  1991/09/10  19:40:31  ian
   Initial revision

   */

#include "uucp.h"

#if USE_RCS_ID
char prtinf_rcsid[] = "$Id: prtinf.c,v 1.1.1.1 1993/03/21 09:53:13 cgd Exp $";
#endif

#include <ctype.h>

#include "port.h"

/* External functions.  */
extern int strcasecmp ();

/* Ports are defined in a file.  This file has to be both flexible and
   easy to use.  The format is described in a separate documentation
   file.  */

#if HAVE_TAYLOR_CONFIG

/* The string names of the port types.  This array corresponds to the
   tporttype enumeration.  */

static const char *azPtype_names[] =
{
  "stdin",
  "modem",
  "direct",
#if HAVE_TCP
  "tcp",
#endif
};

#define CPORT_TYPES (sizeof azPtype_names / sizeof azPtype_names[0])

/* The port structure used by the command tables.  */

static struct sport sPort;

/* The generic port command table.  */

static enum tcmdtabret tpexit P((int argc, char **argv, pointer pvar,
				 const char *zerr));
static enum tcmdtabret tpdup_type P((int argc, char **argv,
				     pointer pvar, const char *zerr));
static enum tcmdtabret tpproto_param P((int argc, char **argv,
					pointer pvar, const char *zerr));
static enum tcmdtabret tpseven_bit P((int argc, char **argv,
				      pointer pvar, const char *zerr));
static enum tcmdtabret tpreliable P((int argc, char **argv,
				     pointer pvar, const char *zerr));

static struct scmdtab asPort_cmds[] =
{
  { "type", CMDTABTYPE_FN | 2, NULL, tpdup_type },
  { "protocol", CMDTABTYPE_STRING, (pointer) &sPort.zprotocols, NULL },
  { "protocol-parameter", CMDTABTYPE_FN | 0, NULL, tpproto_param },
  { "seven-bit", CMDTABTYPE_FN | 2, (pointer) &sPort.ireliable,
      tpseven_bit },
  { "reliable", CMDTABTYPE_FN | 2, (pointer) &sPort.ireliable,
      tpreliable },
  { "lockname", CMDTABTYPE_STRING, (pointer) &sPort.zlockname, NULL },
  { NULL, 0, NULL, NULL }
};

#define CPORT_CMDS (sizeof asPort_cmds / sizeof asPort_cmds[0])

/* The stdin port command table.  */

static struct scmdtab asPstdin_cmds[] =
{
#ifdef SYSDEP_STDIN_CMDS
  SYSDEP_STDIN_CMDS (sPort.u.sstdin.s),
#endif
  { NULL, 0, NULL, NULL }
};

#define CSTDIN_CMDS (sizeof asPstdin_cmds / sizeof asPstdin_cmds[0])

/* The modem port command table.  */

static enum tcmdtabret tpdialer P((int argc, char **argv, pointer pvar,
				   const char *zerr));
static enum tcmdtabret tpbaud_range P((int argc, char **argv,
				       pointer pvar,
				       const char *zerr));

static struct scmdtab asPmodem_cmds[] =
{
  { "device", CMDTABTYPE_STRING, (pointer) &sPort.u.smodem.zdevice, NULL },
  { "baud", CMDTABTYPE_LONG, (pointer) &sPort.u.smodem.ibaud, NULL },
  { "speed", CMDTABTYPE_LONG, (pointer) &sPort.u.smodem.ibaud, NULL },
  { "baud-range", CMDTABTYPE_FN | 3, NULL, tpbaud_range },
  { "speed-range", CMDTABTYPE_FN | 3, NULL, tpbaud_range },
  { "carrier", CMDTABTYPE_BOOLEAN, (pointer) &sPort.u.smodem.fcarrier, NULL },
  { "dial-device", CMDTABTYPE_STRING, (pointer) &sPort.u.smodem.zdial_device,
      NULL },
  { "dialer", CMDTABTYPE_FN | 0, (pointer) &sPort.u.smodem.qdialer,
      tpdialer },
  { "dialer-sequence", CMDTABTYPE_FULLSTRING,
      (pointer) &sPort.u.smodem.zdialer, NULL },
#ifdef SYSDEP_MODEM_CMDS
  SYSDEP_MODEM_CMDS (sPort.u.smodem.s);
#endif
  { NULL, 0, NULL, NULL }
};

#define CMODEM_CMDS (sizeof asPmodem_cmds / sizeof asPmodem_cmds[0])

/* The direct port command table.  */

static struct scmdtab asPdirect_cmds[] =
{
  { "device", CMDTABTYPE_STRING, (pointer) &sPort.u.sdirect.zdevice, NULL },
  { "baud", CMDTABTYPE_LONG, (pointer) &sPort.u.sdirect.ibaud, NULL },
  { "speed", CMDTABTYPE_LONG, (pointer) &sPort.u.sdirect.ibaud, NULL },
#ifdef SYSDEP_DIRECT_CMDS
  SYSDEP_DIRECT_CMDS (sPort.u.sdirect.s);
#endif
  { NULL, 0, NULL, NULL }
};

#define CDIRECT_CMDS (sizeof asPdirect_cmds / sizeof asPdirect_cmds[0])

#if HAVE_TCP

/* The TCP port command table.  */

static struct scmdtab asPtcp_cmds[] =
{
  { "service", CMDTABTYPE_STRING, (pointer) &sPort.u.stcp.zport, NULL },
  { NULL, 0, NULL, NULL }
};

#define CTCP_CMDS (sizeof asPtcp_cmds / sizeof asPtcp_cmds[0])

#else /* ! HAVE_TCP */

#define CTCP_CMDS (0)

#endif /* ! HAVE_TCP */

/* A little routine to exit when another port command is seen.  This
   will cause confusion if somebody types ``port port foo'' in the
   system file.  */

/*ARGSUSED*/
static enum tcmdtabret
tpexit (argc, argv, pvar, zerr)
     int argc;
     char **argv;
     pointer pvar;
     const char *zerr;
{
  return CMDTABRET_EXIT;
}

/* A little routine to give a warning about duplicate type commands.  */

/*ARGSUSED*/
static enum tcmdtabret
tpdup_type (argc, argv, pvar, zerr)
     int argc;
     char **argv;
     pointer pvar;
     const char *zerr;
{
  ulog (LOG_ERROR, "%s: type: Ignoring extraneous command", zerr);
  return CMDTABRET_FREE;
}

/* Handle protocol parameter settings for ports.  */

/*ARGSUSED*/
static enum tcmdtabret
tpproto_param (argc, argv, pvar, zerr)
     int argc;
     char **argv;
     pointer pvar;
     const char *zerr;
{
  return tadd_proto_param (&sPort.cproto_params, &sPort.qproto_params,
			   zerr, argc - 1, argv + 1);
}

/* Handle the ``seven-bit'' command for ports or dialers.  */

/*ARGSUSED*/
static enum tcmdtabret
tpseven_bit (argc, argv, pvar, zerr)
     int argc;
     char **argv;
     pointer pvar;
     const char *zerr;
{
  char b;
  boolean fset;
  int *pi = (int *) pvar;

  b = argv[1][0];
  if (b == 'y' || b == 'Y' || b == 't' || b == 'T')
    fset = TRUE;
  else if (b == 'n' || b == 'N' || b == 'f' || b == 'F')
    fset = FALSE;
  else
    {
      ulog (LOG_ERROR, "%s: %s: %s: Bad boolean", zerr, argv[0], argv[1]);
      return CMDTABRET_FREE;
    }

  *pi |= RELIABLE_SPECIFIED;
  if (fset)
    *pi |= RELIABLE_EIGHT;
  else
    *pi &=~ RELIABLE_EIGHT;

  return CMDTABRET_FREE;
}

/* Handle the ``reliable'' command for ports or dialers.  */

/*ARGSUSED*/
static enum tcmdtabret
tpreliable (argc, argv, pvar, zerr)
     int argc;
     char **argv;
     pointer pvar;
     const char *zerr;
{
  char b;
  boolean fset;
  int *pi = (int *) pvar;

  b = argv[1][0];
  if (b == 'y' || b == 'Y' || b == 't' || b == 'T')
    fset = TRUE;
  else if (b == 'n' || b == 'N' || b == 'f' || b == 'F')
    fset = FALSE;
  else
    {
      ulog (LOG_ERROR, "%s: %s: %s: Bad boolean", zerr, argv[0], argv[1]);
      return CMDTABRET_FREE;
    }

  *pi |= RELIABLE_SPECIFIED;
  if (fset)
    *pi |= RELIABLE_RELIABLE;
  else
    *pi &=~ RELIABLE_RELIABLE;

  return CMDTABRET_FREE;
}

/* Handle the ``baud-range'' command for a modem port.  */

/*ARGSUSED*/
static enum tcmdtabret
tpbaud_range (argc, argv, pvar, zerr)
     int argc;
     char **argv;
     pointer pvar;
     const char *zerr;
{
  /* This should do more error checking.  */
  sPort.u.smodem.ilowbaud = atol (argv[1]);
  sPort.u.smodem.ihighbaud = atol (argv[2]);
  return CMDTABRET_FREE;
}

/* Process a port command found while reading the system information
   file.  */

enum tcmdtabret
tprocess_port_cmd (argc, argv, pvar, zerr)
     int argc;
     char **argv;
     pointer pvar;
     const char *zerr;
{
  struct sport **pqport = (struct sport **) pvar;
  enum tcmdtabret tret;
  const struct scmdtab *qcmds;
  int i;

  if (*pqport == NULL)
    {
      enum tporttype ttype;
      struct sport *qnew;
      boolean fgottype;

      if (strcasecmp (argv[0], "type") != 0)
	{
	  ttype = PORTTYPE_MODEM;
	  fgottype = FALSE;
	}
      else
	{
	  if (argc != 2)
	    {
	      ulog (LOG_ERROR, "%s: port type: Wrong number of arguments",
		    zerr);
	      return CMDTABRET_FREE;
	    }

	  for (i = 0; i < CPORT_TYPES; i++)
	    if (strcasecmp (argv[1], azPtype_names[i]) == 0)
	      break;

	  if (i >= CPORT_TYPES)
	    {
	      ulog (LOG_ERROR, "%s: Unknown port type", zerr);
	      return CMDTABRET_FREE;
	    }
	  
	  ttype = (enum tporttype) i;
	  fgottype = TRUE;
	}

      qnew = (struct sport *) xmalloc (sizeof (struct sport));
      qnew->zname = NULL;
      qnew->ttype = ttype;
      qnew->zprotocols = NULL;
      qnew->cproto_params = 0;
      qnew->qproto_params = NULL;
      qnew->zlockname = NULL;

      /* Note that we do not set RELIABLE_SPECIFIED; this just sets
	 defaults, so that ``seven-bit true'' does not imply
	 ``reliable false''.  */
      qnew->ireliable = RELIABLE_RELIABLE | RELIABLE_EIGHT;

      switch (ttype)
	{
	case PORTTYPE_STDIN:
#ifdef SYSDEP_STDIN_INIT
	  SYSDEP_STDIN_INIT (&qnew->u.sstdin.s);
#endif
	  break;
	case PORTTYPE_MODEM:
	  qnew->u.smodem.zdevice = NULL;
	  qnew->u.smodem.zdial_device = NULL;
	  qnew->u.smodem.ibaud = 0L;
	  qnew->u.smodem.ilowbaud = 0L;
	  qnew->u.smodem.ihighbaud = 0L;
	  qnew->u.smodem.fcarrier = TRUE;
	  qnew->u.smodem.zdialer = NULL;
	  qnew->u.smodem.qdialer = NULL;
#ifdef SYSDEP_MODEM_INIT
	  SYSDEP_MODEM_INIT (&qnew->u.smodem.s);
#endif
	  break;
	case PORTTYPE_DIRECT:
	  qnew->u.sdirect.zdevice = NULL;
	  qnew->u.sdirect.ibaud = -1;
#ifdef SYSDEP_DIRECT_INIT
	  SYSDEP_DIRECT_INIT (&qnew->u.sdirect.s);
#endif
	  break;
#if HAVE_TCP
	case PORTTYPE_TCP:
	  qnew->u.stcp.o = -1;
	  qnew->u.stcp.zport = "uucp";
	  break;
#endif /* HAVE_TCP */
	}

      *pqport = qnew;

      if (fgottype)
	return CMDTABRET_FREE;
    }

  /* See if this command is one of the generic ones.  */
  for (i = 0; i < CPORT_CMDS - 1; i++)
    {
      if (strcasecmp (argv[0], asPort_cmds[i].zcmd) == 0)
	{
	  sPort = **pqport;
	  tret = tprocess_one_cmd (argc, argv, asPort_cmds, zerr, 0);
	  **pqport = sPort;
	  return tret;
	}
    }

  /* Now check the port commands for specific types.  */
  switch ((*pqport)->ttype)
    {
    case PORTTYPE_STDIN:
      qcmds = asPstdin_cmds;
      break;
    case PORTTYPE_MODEM:
      qcmds = asPmodem_cmds;
      break;
    case PORTTYPE_DIRECT:
      qcmds = asPdirect_cmds;
      break;
#if HAVE_TCP
    case PORTTYPE_TCP:
      qcmds = asPtcp_cmds;
      break;
#endif /* HAVE_TCP */
    default:
#if DEBUG > 0
      ulog (LOG_FATAL, "tprocess_port_cmd: Can't happen");
#endif
      return CMDTABRET_FREE;
    }

  sPort = **pqport;
  tret = tprocess_one_cmd (argc, argv, qcmds, zerr,
			   CMDFLAG_WARNUNRECOG);
  **pqport = sPort;
  return tret;
}

/* Fill in an scmdtab structure to pass every possible port command
   to tprocess_port_cmd.  The argument must be large enough.  */

static void uport_cmdtab P((struct scmdtab *qcmds, struct sport **pqnew));

static void
uport_cmdtab (qcmds, pqnew)
     struct scmdtab *qcmds;
     struct sport **pqnew;
{
  int i;
  int ibase;

  ibase = 0;
  for (i = 0; i < CPORT_CMDS - 1; i++)
    {
      qcmds[ibase + i].zcmd = asPort_cmds[i].zcmd;
      qcmds[ibase + i].itype = CMDTABTYPE_FN | 0;
      qcmds[ibase + i].pvar = (pointer) pqnew;
      qcmds[ibase + i].ptfn = tprocess_port_cmd;
    }	 
  ibase += CPORT_CMDS - 1;
  for (i = 0; i < CSTDIN_CMDS - 1; i++)
    {
      qcmds[ibase + i].zcmd = asPstdin_cmds[i].zcmd;
      qcmds[ibase + i].itype = CMDTABTYPE_FN | 0;
      qcmds[ibase + i].pvar = (pointer) pqnew;
      qcmds[ibase + i].ptfn = tprocess_port_cmd;
    }
  ibase += CSTDIN_CMDS - 1;
  for (i = 0; i < CMODEM_CMDS - 1; i++)
    {
      qcmds[ibase + i].zcmd = asPmodem_cmds[i].zcmd;
      qcmds[ibase + i].itype = CMDTABTYPE_FN | 0;
      qcmds[ibase + i].pvar = (pointer) pqnew;
      qcmds[ibase + i].ptfn = tprocess_port_cmd;
    }
  ibase += CMODEM_CMDS - 1;
  for (i = 0; i < CDIRECT_CMDS - 1; i++)
    {
      qcmds[ibase + i].zcmd = asPdirect_cmds[i].zcmd;
      qcmds[ibase + i].itype = CMDTABTYPE_FN | 0;
      qcmds[ibase + i].pvar = (pointer) pqnew;
      qcmds[ibase + i].ptfn = tprocess_port_cmd;
    }
  ibase += CDIRECT_CMDS - 1;
#if HAVE_TCP
  for (i = 0; i < CTCP_CMDS - 1; i++)
    {
      qcmds[ibase + i].zcmd = asPtcp_cmds[i].zcmd;
      qcmds[ibase + i].itype = CMDTABTYPE_FN | 0;
      qcmds[ibase + i].pvar = (pointer) pqnew;
      qcmds[ibase + i].ptfn = tprocess_port_cmd;
    }
  ibase += CTCP_CMDS - 1;
#endif /* HAVE_TCP */
  qcmds[ibase].zcmd = NULL;
}

#endif /* HAVE_TAYLOR_CONFIG */

#if HAVE_TAYLOR_CONFIG

/* Save a port name so that we can use it when looking for a port
   with a particular baud rate.  */

static enum tcmdtabret tpport_for_baud P((int argc, char **argv,
					  pointer pvar, const char *zerr));

static const char *zport_for_baud_name;

/*ARGSUSED*/
static enum tcmdtabret
tpport_for_baud (argc, argv, pvar, zerr)
     int argc;
     char **argv;
     pointer pvar;
     const char *zerr;
{
  zport_for_baud_name = argv[1];
  return CMDTABRET_EXIT;
}

#endif /* HAVE_TAYLOR_CONFIG */

/* Find and lock a port.  The arguments specify a port name and a baud
   rate, and the port found must match both.  If the name passed in is
   NULL, all ports match.  If the baud rate passed in is 0, all ports
   match.  The lock routine is passed in so that it doesn't have to be
   linked in by programs which don't need this call (yes, it's a
   hack).  The freport argument is actually only used for testing
   purposes.  */

boolean
ffind_port (zfind, ibaud, ihighbaud, qport, pflock, freport)
     const char *zfind;
     long ibaud;
     long ihighbaud;
     struct sport *qport;
     boolean (*pflock) P((struct sport *, boolean fin));
     boolean freport;
{
  boolean ffound;

  if (ihighbaud == 0)
    ihighbaud = ibaud;

  ffound = FALSE;

#if HAVE_TAYLOR_CONFIG
  if (zPortfile == NULL)
    {
      boolean fmore;

      /* Only warn about a missing file if we're not going to read the
	 V2 or BNU files.  */
      fmore = FALSE;
#if HAVE_V2_CONFIG
      if (fV2)
	fmore = TRUE;
#endif
#if HAVE_BNU_CONFIG
      if (fBnu)
	fmore = TRUE;
#endif
      if (! fmore)
	{
	  ulog (LOG_ERROR, "%s%s: file not found", NEWCONFIGLIB,
		PORTFILE);
	  return FALSE;
	}
    }
  else
    {
      struct smulti_file *qmulti;
      struct scmdtab ascmds[CPORT_CMDS + CSTDIN_CMDS + CMODEM_CMDS +
			    CDIRECT_CMDS + CTCP_CMDS + 1];
      struct sport *qnew;
      const char *zname;

      qmulti = qmulti_open (zPortfile);
      if (qmulti != NULL)
	{
	  /* Set up such that each command is routed to
	     tprocess_port_cmd.  Route "port" to tpport_for_baud,
	     which will record the name of the port.  */
	  qnew = NULL;

	  ascmds[0].zcmd = "port";
	  ascmds[0].itype = CMDTABTYPE_FN | 2;
	  ascmds[0].pvar = NULL;
	  ascmds[0].ptfn = tpport_for_baud;
	  uport_cmdtab (ascmds + 1, &qnew);

	  /* Look for the first command in the file (which should be
	     ``port'') to get the name of the first port.  */

	  zport_for_baud_name = NULL;

	  uprocesscmds ((FILE *) NULL, qmulti, ascmds,
			(const char *) NULL, CMDFLAG_BACKSLASH);

	  if (qnew != NULL)
	    {
	      (void) fmulti_close (qmulti);
	      ulog (LOG_ERROR, "First command in port file is not `port'");
	      return FALSE;
	    }

	  /* Look through the files until we can't find any more.  */

	  zname = zport_for_baud_name;
	  while (zname != NULL)
	    {
	      zport_for_baud_name = NULL;
	      uprocesscmds ((FILE *) NULL, qmulti, ascmds,
			    (const char *) NULL, CMDFLAG_BACKSLASH);

	      if (qnew != NULL
		  && (zfind == NULL
		      || strcmp (zfind, zname) == 0)
		  && (ibaud == 0
		      || (qnew->ttype == PORTTYPE_MODEM
			  && ((qnew->u.smodem.ibaud == 0
			       && qnew->u.smodem.ilowbaud == 0
			       && qnew->u.smodem.ihighbaud == 0)
			      || (qnew->u.smodem.ibaud >= ibaud
				  && qnew->u.smodem.ibaud <= ihighbaud)
			      || (qnew->u.smodem.ilowbaud <= ihighbaud
				  && qnew->u.smodem.ihighbaud >= ibaud)))
		      || (qnew->ttype == PORTTYPE_DIRECT
			  && (qnew->u.sdirect.ibaud == 0
			      || qnew->u.sdirect.ibaud == ibaud))))
		{
		  ffound = TRUE;
		  qnew->zname = zname;
		  if (pflock == NULL
		      || (*pflock) (qnew, FALSE))
		    {
		      *qport = *qnew;
		      xfree ((pointer) qnew);
		      (void) fmulti_close (qmulti);
		      return TRUE;
		    }
		}

	      xfree ((pointer) qnew);
	      qnew = NULL;

	      zname = zport_for_baud_name;
	    }

	  (void) fmulti_close (qmulti);
	}
    }
#endif /* HAVE_TAYLOR_CONFIG */

#if HAVE_V2_CONFIG
  if (fV2)
    {
      if (fv2_find_port (zfind, ibaud, ihighbaud, qport, pflock, &ffound))
	return TRUE;
    }
#endif /* HAVE_V2_CONFIG */

#if HAVE_BNU_CONFIG
  if (fBnu)
    {
      if (fbnu_find_port (zfind, ibaud, ihighbaud, qport, pflock, &ffound))
	return TRUE;
    }
#endif /* HAVE_BNU_CONFIG */

  if (freport)
    {
      if (ffound)
	ulog (LOG_ERROR, "All matching ports in use");
      else
	ulog (LOG_ERROR, "No matching ports defined");
    }

  return FALSE;
}

#if HAVE_TAYLOR_CONFIG

/* Read dialer commands.  */

static struct sdialer sPdialer;

/* The dialer command table.  */

static enum tcmdtabret tpdtr_toggle P((int argc, char **argv, pointer pvar,
				       const char *zerr));
static enum tcmdtabret tpcomplete P((int argc, char **argv, pointer pvar,
				     const char *zerr));
static enum tcmdtabret tpdialer_proto_param P((int argc, char **argv,
					       pointer pvar,
					       const char *zerr));

static struct scmdtab asPdialer_cmds[] =
{
  { "dialer", CMDTABTYPE_FN | 2, NULL, tpexit },
  { "#", CMDTABTYPE_FN | 0, NULL, tpexit },
  { "chat", CMDTABTYPE_PREFIX | 0, (pointer) &sPdialer.schat,
      tprocess_chat_cmd },
  { "dialtone", CMDTABTYPE_STRING, (pointer) &sPdialer.zdialtone, NULL },
  { "pause", CMDTABTYPE_STRING, (pointer) &sPdialer.zpause, NULL },
  { "carrier", CMDTABTYPE_BOOLEAN, (pointer) &sPdialer.fcarrier, NULL },
  { "carrier-wait", CMDTABTYPE_INT, (pointer) &sPdialer.ccarrier_wait, NULL },
  { "dtr-toggle", CMDTABTYPE_FN | 0, NULL, tpdtr_toggle },
  { "complete", CMDTABTYPE_FN | 2, (pointer) &sPdialer.scomplete,
      tpcomplete },
  { "complete-chat", CMDTABTYPE_PREFIX, (pointer) &sPdialer.scomplete,
      tprocess_chat_cmd },
  { "abort", CMDTABTYPE_FN | 2, (pointer) &sPdialer.sabort, tpcomplete },
  { "abort-chat", CMDTABTYPE_PREFIX, (pointer) &sPdialer.sabort,
      tprocess_chat_cmd },
  { "protocol-parameter", CMDTABTYPE_FN | 0, NULL, tpdialer_proto_param },
  { "seven-bit", CMDTABTYPE_FN | 2, (pointer) &sPdialer.ireliable,
      tpseven_bit },
  { "reliable", CMDTABTYPE_FN | 2, (pointer) &sPdialer.ireliable,
      tpreliable },
  { NULL, 0, NULL, NULL }
};

#define CDIALER_CMDS (sizeof asPdialer_cmds / sizeof asPdialer_cmds[0])

/* Handle the dtr-toggle command.  */

/*ARGSUSED*/
static enum tcmdtabret
tpdtr_toggle (argc, argv, pvar, zerr)
     int argc;
     char **argv;
     pointer pvar;
     const char *zerr;
{
  char b;

  if (argc < 2 || argc > 3)
    {
      ulog (LOG_ERROR, "%s: %s: Wrong number of arguments", zerr, argv[0]);
      return CMDTABRET_FREE;
    }
  
  b = argv[1][0];
  if (b == 'y' || b == 'Y' || b == 't' || b == 'T')
    sPdialer.fdtr_toggle = TRUE;
  else if (b == 'n' || b == 'N' || b == 'f' || b == 'F')
    sPdialer.fdtr_toggle = FALSE;
  else
    {
      ulog (LOG_ERROR, "%s: %s: Bad boolean", zerr, argv[0]);
      return CMDTABRET_FREE;
    }

  if (argc == 3)
    {
      b = argv[2][0];
      if (b == 'y' || b == 'Y' || b == 't' || b == 'T')
	sPdialer.fdtr_toggle_wait = TRUE;
      else if (b == 'n' || b == 'N' || b == 'f' || b == 'F')
	sPdialer.fdtr_toggle_wait = FALSE;
      else
	{
	  ulog (LOG_ERROR, "%s: %s: Bad boolean", zerr, argv[0]);
	  return CMDTABRET_FREE;
	}
    }

  return CMDTABRET_FREE;
}

/* Handle the complete and abort commands.  These just turn a string
   into a trivial chat script.  */

/*ARGSUSED*/
static enum tcmdtabret
tpcomplete (argc, argv, pvar, zerr)
     int argc;
     char **argv;
     pointer pvar;
     const char *zerr;
{
  struct schat_info *qchat = (struct schat_info *) pvar;

  qchat->zchat = (char *) xmalloc (strlen (argv[1]) + sizeof "\"\" ");
  sprintf (qchat->zchat, "\"\" %s", argv[1]);
  return CMDTABRET_FREE;
}

/* Handle protocol parameters for dialers.  */

/*ARGSUSED*/
static enum tcmdtabret
tpdialer_proto_param (argc, argv, pvar, zerr)
     int argc;
     char **argv;
     pointer pvar;
     const char *zerr;
{
  return tadd_proto_param (&sPdialer.cproto_params, &sPdialer.qproto_params,
			   zerr, argc - 1, argv + 1);
}

/* Process one dialer command.  */

static enum tcmdtabret tprocess_dialer_cmd P((int argc, char **argv,
					      pointer pvar,
					      const char *zerr));

static enum tcmdtabret
tprocess_dialer_cmd (argc, argv, pvar, zerr)
     int argc;
     char **argv;
     pointer pvar;
     const char *zerr;
{
  struct sdialer **pq = (struct sdialer **) pvar;
  enum tcmdtabret tret;

  if (*pq == NULL)
    {
      struct sdialer *qnew;

      qnew = (struct sdialer *) xmalloc (sizeof (struct sdialer));
      qnew->zname = NULL;
      INIT_CHAT (&qnew->schat);
      qnew->zdialtone = ",";
      qnew->zpause = ",";
      qnew->fcarrier = TRUE;
      qnew->ccarrier_wait = 60;
      qnew->fdtr_toggle = FALSE;
      qnew->fdtr_toggle_wait = FALSE;
      INIT_CHAT (&qnew->scomplete);
      INIT_CHAT (&qnew->sabort);
      qnew->cproto_params = 0;
      qnew->qproto_params = NULL;

      /* Note that we do not set RELIABLE_SPECIFIED; this just sets
	 defaults, so that ``seven-bit true'' does not imply
	 ``reliable false''.  */
      qnew->ireliable = RELIABLE_RELIABLE | RELIABLE_EIGHT;

      *pq = qnew;
    }
  
  sPdialer = **pq;
  tret = tprocess_one_cmd (argc, argv, asPdialer_cmds, zerr,
			   CMDFLAG_WARNUNRECOG);
  **pq = sPdialer;
  return tret;
}

/* Handle a dialer command seen in the port file.  */

static enum tcmdtabret
tpdialer (argc, argv, pvar, zerr)
     int argc;
     char **argv;
     pointer pvar;
     const char *zerr;
{
  if (argc < 2)
    {
      ulog (LOG_ERROR, "%s: %s: Wrong number of arguments", zerr, argv[0]);
      return CMDTABRET_FREE;
    }
  
  if (argc > 2)
    return tprocess_dialer_cmd (argc - 1, argv + 1, pvar, zerr);

  sPort.u.smodem.zdialer = argv[1];

  return CMDTABRET_CONTINUE;
}

/* Read the information for a particular item.  */

struct splook
{
  const char *zlook;
  boolean ffound;
};

/* Look for a particular dialer.  */

static enum tcmdtabret tplook P((int argc, char **argv, pointer pvar,
				 const char *zerr));

/*ARGSUSED*/
static enum tcmdtabret
tplook (argc, argv, pvar, zerr)
     int argc;
     char **argv;
     pointer pvar;
     const char *zerr;
{
  struct splook *q = (struct splook *) pvar;

  if (strcmp (q->zlook, argv[1]) == 0)
    {
      q->ffound = TRUE;
      return CMDTABRET_FREE_AND_EXIT;
    }
  return CMDTABRET_FREE;
}

#endif /* HAVE_TAYLOR_CONFIG */

/* Read information about a specific dialer.  */

boolean
fread_dialer_info (zdialer, qdialer)
     const char *zdialer;
     struct sdialer *qdialer;
{
#if HAVE_TAYLOR_CONFIG

  /* If we haven't got a dialer file, then only complain if we aren't
     going to try the BNU dialer file.  */
  if (zDialfile == NULL)
    {
#if HAVE_BNU_CONFIG
      if (! fBnu)
#endif
	{
	  ulog (LOG_ERROR, "%s%s: file not found", NEWCONFIGLIB, DIALFILE);
	  return FALSE;
	}
    }
  else
    {
      struct smulti_file *qmulti;
      struct scmdtab as[2];
      struct splook s;
      struct sdialer *qnew;
      struct scmdtab ascmds[CDIALER_CMDS];
      int i;

      qmulti = qmulti_open (zDialfile);
      if (qmulti != NULL)
	{
	  s.zlook = zdialer;
	  s.ffound = FALSE;

	  as[0].zcmd = "dialer";
	  as[0].itype = CMDTABTYPE_FN | 2;
	  as[0].pvar = (pointer) &s;
	  as[0].ptfn = tplook;

	  as[1].zcmd = NULL;

	  uprocesscmds ((FILE *) NULL, qmulti, as, (const char *) NULL,
			CMDFLAG_BACKSLASH);

	  if (s.ffound)
	    {
	      qnew = NULL;

	      for (i = 0; i < CDIALER_CMDS; i++)
		{
		  ascmds[i].zcmd = asPdialer_cmds[i].zcmd;
		  if (TTYPE_CMDTABTYPE (asPdialer_cmds[i].itype)
		      == CMDTABTYPE_PREFIX)
		    ascmds[i].itype = CMDTABTYPE_PREFIX | 0;
		  else
		    ascmds[i].itype = CMDTABTYPE_FN | 0;
		  ascmds[i].pvar = (pointer) &qnew;
		  ascmds[i].ptfn = tprocess_dialer_cmd;
		}

	      uprocesscmds ((FILE *) NULL, qmulti, ascmds,
			    (const char *) NULL,
			    CMDFLAG_WARNUNRECOG | CMDFLAG_BACKSLASH);

	      (void) fmulti_close (qmulti);

	      if (qnew == NULL)
		{
		  ulog (LOG_ERROR, "No information for dialer %s", zdialer);
		  return FALSE;
		}

	      *qdialer = *qnew;
	      xfree ((pointer) qnew);

	      return TRUE;
	    }
    
	  (void) fmulti_close (qmulti);
	}
    }
#endif /* HAVE_TAYLOR_CONFIG */

#if HAVE_BNU_CONFIG
  if (fBnu)
    {
      if (fbnu_read_dialer_info (zdialer, qdialer))
	return TRUE;
    }
#endif /* HAVE_BNU_CONFIG */

  ulog (LOG_ERROR, "%s: No such dialer", zdialer);
  return FALSE;
}
