/* vars.h -- variables for dungeon */

/* These variable definitions are really ugly because they are actually
 * translations of FORTRAN common blocks.  In the original FORTRAN source
 * the common blocks were included many times by each function that
 * needed them; I have changed this to define them all in this one file,
 * and include this in every source file.  It's less modular, and it
 * makes everything slower to compile, but it's easier on me.
 * A few structures used only by the parsing routines are in parse.h.
 */

#ifndef VARS_H
#define VARS_H

#ifndef EXTERN
#define EXTERN extern
#endif

EXTERN struct {
    integer prsa, prsi, prso;
    logical prswon;
    integer prscon;
} prsvec_;

#define prsvec_1 prsvec_

EXTERN struct {
    integer oflag, oact, oslot, oprep, oname;
} orphs_;

#define orphs_1 orphs_
#define orp ((integer *)&orphs_1)

EXTERN struct {
    integer lastit;
} last_;

#define last_1 last_

EXTERN struct {
    integer winner, here;
    logical telflg;
} play_;

#define play_1 play_

EXTERN struct {
    integer rlnt, rdesc1[200], rdesc2[200], rexit[200], ractio[200],
	    rval[200], rflag[200];
} rooms_;

#define rooms_1 rooms_
#define eqr ((integer *)&rooms_1 + 1)
#define rrand ((integer *)&rooms_1 + 601)

/* Room flags */

#define RSEEN (32768)
#define RLIGHT (16384)
#define RLAND (8192)
#define RWATER (4096)
#define RAIR (2048)
#define RSACRD (1024)
#define RFILL (512)
#define RMUNG (256)
#define RBUCK (128)
#define RHOUSE (64)
#define RNWALL (32)
#define REND (16)

EXTERN const struct {
    integer whous, lroom, cella, mtrol, maze1, mgrat, maz15, fore1, fore3, 
	    clear, reser, strea, egypt, echor, tshaf, bshaf, mmach, dome, 
	    mtorc, carou, riddl, lld2, temp1, temp2, maint, blroo, treas, 
	    rivr1, rivr2, rivr3, mcycl, rivr4, rivr5, fchmp, falls, mbarr, 
	    mrain, pog, vlbot, vair1, vair2, vair3, vair4, ledg2, ledg3, 
	    ledg4, msafe, cager, caged, twell, bwell, alice, alism, alitr, 
	    mtree, bkent, bkvw, bktwi, bkvau, bkbox, crypt, tstrs, mrant, 
	    mreye, mra, mrb, mrc, mrg, mrd, fdoor, mrae, mrce, mrcw, mrge, 
	    mrgw, mrdw, inmir, scorr, ncorr, parap, cell, pcell, ncell, cpant,
	    cpout, cpuzz;
} rindex_
#ifdef INIT
	= { 2, 8, 9, 10, 11, 25, 30, 31, 33, 36, 40, 42, 44, 49, 61, 76,
	    77, 79, 80, 83, 91, 94, 96, 97, 100, 102, 103, 107, 108, 109, 
	    101, 112, 113, 114, 120, 119, 121, 122, 126, 127, 128, 129, 130, 
	    131, 132, 133, 135, 140, 141, 142, 143, 144, 145, 146, 147, 148, 
	    151, 153, 154, 155, 157, 158, 159, 160, 161, 162, 163, 164, 165, 
	    166, 167, 171, 172, 173, 174, 176, 177, 179, 182, 183, 184, 185, 
	    186, 188, 189, 190 }
#endif
	;

#define rindex_1 rindex_

EXTERN const struct {
    integer xmin, xmax, xdown, xup, xnorth, xsouth, xenter, xexit, xeast, 
	    xwest;
} xsrch_
#ifdef INIT
	= { 1024, 16384, 10240, 9216, 1024, 5120, 13312, 14336, 3072, 
	    7168 }
#endif
	;

#define xsrch_1 xsrch_

EXTERN struct {
    integer olnt, odesc1[220], odesc2[220], odesco[220], oactio[220], oflag1[
	    220], oflag2[220], ofval[220], otval[220], osize[220], ocapac[220]
	    , oroom[220], oadv[220], ocan[220], oread[220];
} objcts_;

#define objcts_1 objcts_
#define eqo ((integer *)&objcts_1 + 1)

EXTERN struct {
    integer r2lnt, oroom2[20], rroom2[20];
} oroom2_;

#define oroom2_1 oroom2_

/* Object flags (oflags) */

#define VISIBT (32768)
#define READBT (16384)
#define TAKEBT (8192)
#define DOORBT (4096)
#define TRANBT (2048)
#define FOODBT (1024)
#define NDSCBT (512)
#define DRNKBT (256)
#define CONTBT (128)
#define LITEBT (64)
#define VICTBT (32)
#define BURNBT (16)
#define FLAMBT (8)
#define TOOLBT (4)
#define TURNBT (2)
#define ONBT (1)
#define FINDBT (32768)
#define SLEPBT (16384)
#define SCRDBT (8192)
#define TIEBT (4096)
#define CLMBBT (2048)
#define ACTRBT (1024)
#define WEAPBT (512)
#define FITEBT (256)
#define VILLBT (128)
#define STAGBT (64)
#define TRYBT (32)
#define NOCHBT (16)
#define OPENBT (8)
#define TCHBT (4)
#define VEHBT (2)
#define SCHBT (1)

EXTERN const struct {
    integer garli, food, gunk, coal, machi, diamo, tcase, bottl, water, rope, 
	    knife, sword, lamp, blamp, rug, leave, troll, axe, rknif, keys, 
	    ice, bar, coffi, torch, tbask, fbask, irbox, ghost, trunk, bell, 
	    book, candl, match, tube, putty, wrenc, screw, cyclo, chali, 
	    thief, still, windo, grate, door, hpole, leak, rbutt, raili, pot, 
	    statu, iboat, dboat, pump, rboat, stick, buoy, shove, ballo, 
	    recep, guano, brope, hook1, hook2, safe, sslot, brick, fuse, 
	    gnome, blabe, dball, tomb, lcase, cage, rcage, spher, sqbut, 
	    flask, pool, saffr, bucke, ecake, orice, rdice, blice, robot, 
	    ftree, bills, portr, scol, zgnom, egg, begg, baubl, canar, bcana, 
	    ylwal, rdwal, pindr, rbeam, odoor, qdoor, cdoor, num1, num8, 
	    warni, cslit, gcard, stldr, hands, wall, lungs, sailo, aviat, 
	    teeth, itobj, every, valua, oplay, wnort, gwate, master;
} oindex_
#ifdef INIT
	= { 2, 3, 4, 5, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
	    20, 21, 23, 30, 26, 33, 34, 35, 36, 39, 42, 45, 46, 47, 48, 51, 
	    54, 55, 56, 57, 58, 59, 61, 62, 63, 65, 66, 71, 78, 79, 75, 85, 
	    86, 87, 88, 89, 90, 92, 94, 96, 98, 99, 97, 101, 102, 103, 105, 
	    107, 109, 110, 111, 112, 113, 119, 123, 124, 125, 126, 127, 132, 
	    133, 134, 137, 138, 139, 140, 141, 142, 145, 148, 149, 151, 152, 
	    154, 155, 156, 157, 158, 159, 161, 164, 171, 172, 173, 175, 178, 
	    185, 186, 187, 188, 189, 200, 198, 201, 196, 202, 197, 192, 194, 
	    195, 193, 205, 209, 215 }
#endif
	;

#define oindex_1 oindex_

EXTERN struct {
    integer clnt, ctick[25], cactio[25];
    logical cflag[25];
} cevent_;

#define cevent_1 cevent_
#define eqc ((integer *)&cevent_1 + 1)

EXTERN const struct {
    integer cevcur, cevmnt, cevlnt, cevmat, cevcnd, cevbal, cevbrn, cevfus, 
	    cevled, cevsaf, cevvlg, cevgno, cevbuc, cevsph, cevegh, cevfor, 
	    cevscl, cevzgi, cevzgo, cevste, cevmrs, cevpin, cevinq, cevfol;
} cindex_
#ifdef INIT
	= { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 
	    18, 19, 20, 21, 22, 23, 24 }
#endif
	;

#define cindex_1 cindex_

EXTERN struct {
    integer alnt, aroom[4], ascore[4], avehic[4], aobj[4], aactio[4], astren[
	    4], aflag[4];
} advs_;

#define advs_1 advs_
#define eqa ((integer *)&advs_1 + 1)

EXTERN const struct {
    integer astag;
} aflags_
#ifdef INIT
	= { 32768 }
#endif
	;

#define aflags_1 aflags_

EXTERN const struct {
    integer player, arobot, amastr;
} aindex_
#ifdef INIT
	= { 1, 2, 3 }
#endif
	;

#define aindex_1 aindex_

EXTERN const struct {
    integer cintw, deadxw, frstqw, inxw, outxw, walkiw, fightw, foow, meltw, 
	    readw, inflaw, deflaw, alarmw, exorcw, plugw, kickw, wavew, 
	    raisew, lowerw, rubw, pushw, untiew, tiew, tieupw, turnw, breatw, 
	    knockw, lookw, examiw, shakew, movew, trnonw, trnofw, openw, 
	    closew, findw, waitw, spinw, boardw, unboaw, takew, invenw, fillw,
	     eatw, drinkw, burnw, mungw, killw, attacw, swingw, walkw, tellw, 
	    putw, dropw, givew, pourw, throww, digw, leapw, stayw, follow, 
	    hellow, lookiw, lookuw, pumpw, windw, clmbw, clmbuw, clmbdw, 
	    trntow;
} vindex_
#ifdef INIT
	= { 1, 2, 3, 4, 5, 6, 7, 8, 101, 100, 102, 103, 104, 105, 106, 
	    107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 
	    120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 
	    133, 134, 135, 136, 137, 138, 139, 141, 140, 142, 143, 144, 145, 
	    146, 147, 148, 89, 91, 73, 85, 151, 152, 153, 154, 155, 156, 157, 
	    158, 159 }
#endif
	;

#define vindex_1 vindex_

EXTERN struct {
    logical trollf, cagesf, bucktf, caroff, carozf, lwtidf, domef, glacrf, 
	    echof, riddlf, lldf, cyclof, magicf, litldf, safef, gnomef, 
	    gnodrf, mirrmf, egyptf, onpolf, blabf, brieff, superf, buoyf, 
	    grunlf, gatef, rainbf, cagetf, empthf, deflaf, glacmf, frobzf, 
	    endgmf, badlkf, thfenf, singsf, mrpshf, mropnf, wdopnf, mr1f, 
	    mr2f, inqstf, follwf, spellf, cpoutf, cpushf;
    integer btief, binff, rvmnt, rvclr, rvcyc, rvsnd, rvgua, orrug, orcand, 
	    ormtch, orlamp, mdir, mloc, poleuf, quesno, nqatt, corrct, lcell, 
	    pnumb, acell, dcell, cphere;
} findex_;

#define findex_1 findex_
#define flags ((logical *)&findex_1)
#define switch_ ((integer *)&findex_1 + 46)

EXTERN struct {
    integer dbgflg, prsflg, gdtflg;
} debug_;

#define debug_1 debug_

EXTERN struct {
    integer thfpos;
    logical thfflg, thfact, swdact;
    integer swdsta;
} hack_;

#define hack_1 hack_

EXTERN struct {
    integer vlnt, villns[4], vprob[4], vopps[4], vbest[4], vmelee[4];
} vill_;

#define vill_1 vill_
#define eqv ((integer *)&vill_1 + 1)

EXTERN struct {
    integer moves, deaths, rwscor, mxscor, mxload, ltshft, bloc, mungrm, hs, 
	    egscor, egmxsc;
} state_;

#define state_1 state_

EXTERN struct {
    integer xtype, xroom1, xstrng, xactio, xobj;
} curxt_;

#define curxt_1 curxt_
#define xflag ((integer *)&curxt_1 + 4)

EXTERN const struct {
    integer xrmask, xdmask, xfmask, xfshft, xashft, xelnt[4], xnorm, xno, 
	    xcond, xdoor, xlflag;
} xpars_
#ifdef INIT
	= { 255, 31744, 3, 256, 256, { 1, 2, 3, 3 }, 1, 2, 3, 4, 32768 }
#endif
	;

#define xpars_1 xpars_

EXTERN struct {
    integer mbase, strbit;
} star_;

#define star_1 star_

EXTERN struct {
    integer inlnt;
    char inbuf[78];
} input_;

#define input_1 input_

EXTERN struct {
    integer fromdr, scolrm, scolac;
    const integer scoldr[8], scolwl[12];
} screen_
#ifdef INIT
	= { 0, 0, 0, { 1024, 153, 5120, 154, 3072, 152, 7168, 151 },
	    { 151, 207, 3072, 152, 208, 7168, 153, 206, 5120, 154, 205,
	      1024 } }
#endif
	;

#define screen_1 screen_

EXTERN struct {
    integer mlnt, mrloc, rtext[1050];
} rmsg_;

#define rmsg_1 rmsg_

EXTERN const struct {
    integer vmaj, vmin, vedit;
} vers_
#ifdef INIT
	= { 2, 7, 'B' }
#endif
	;

#define vers_1 vers_

EXTERN struct {
    integer pltime, shour, smin, ssec;
} time_;

#define time_1 time_

EXTERN const struct {
    integer hfactr;
} hyper_
#ifdef INIT
	= { 500 }
#endif
	;

#define hyper_1 hyper_

EXTERN struct {
    integer xlnt, travel[900];
} exits_;

#define exits_1 exits_

EXTERN struct {
    const integer cpdr[16], cpwl[8];
    integer cpvec[64];
} puzzle_
#ifdef INIT
	= { { 1024, -8, 2048, -7, 3072, 1, 4096, 9, 5120, 8, 6144, 7, 
	      7168, -1, 8192, -9 },
	    { 205, -8, 206, 8, 207, 1, 208, -1 },
            { 1,  1,  1,  1,  1,  1, 1, 1,
              1,  0, -1,  0,  0, -1, 0, 1,
              1, -1,  0,  1,  0, -2, 0, 1,
	      1,  0,  0,  0,  0,  1, 0, 1,
	      1, -3,  0,  0, -1, -1, 0, 1,
	      1,  0,  0, -1,  0,  0, 0, 1,
	      1,  1,  1,  0,  0,  0, 1, 1,
	      1,  1,  1,  1,  1,  1, 1, 1 } }
#endif
	;

#define puzzle_1 puzzle_

EXTERN const struct {
    const integer batdrp[9];
} bats_
#ifdef INIT
	= { 66, 67, 68, 69, 70, 71, 72, 65, 73 }
#endif
	;

#define bats_1 bats_

#endif
