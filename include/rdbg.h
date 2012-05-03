/* Remote kernel debug protocol */
enum
{
	Terr='0',	/* not sent */
	Rerr,
	Tmget,
	Rmget,
	Tmput,
	Rmput,

	Tspid,	/* obsolete */
	Rspid,	/* obsolete */
	Tproc,
	Rproc,
	Tstatus,
	Rstatus,
	Trnote,
	Rrnote,

	Tstartstop,
	Rstartstop,
	Twaitstop,
	Rwaitstop,
	Tstart,
	Rstart,
	Tstop,
	Rstop,
	Tkill,
	Rkill,

	Tcondbreak,
	Rcondbreak,

	RDBMSGLEN = 10	/* tag byte, 9 data bytes */
};
