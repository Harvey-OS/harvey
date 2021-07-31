TEXT	vectors(SB), $0
	WORD	$0x4eb9			/* JSR	start (never to return) */
	WORD	$0x0400
	WORD	$0x0400
	WORD	$0x0000
TEXT	vectors0<>(SB), $0
/* origin	0008, vector 2	*/
	LONG	buserror(SB)		/* bus error */
	LONG	illegal(SB)		/* address error */
	LONG	illegal(SB)		/* illegal(SB) instruction */
	LONG	illegal(SB)		/* zero divide */
	LONG	illegal(SB)		/* chk, chk2 instruction */
	LONG	illegal(SB)		/* cptrapcc, trapcc, trapv instruction */
	LONG	illegal(SB)		/* privilege violation */
	LONG	illegal(SB)		/* trace */
	LONG	illegal(SB)		/* line 1010 emulator */
	LONG	illegal(SB)		/* line 1111 emulator */
	LONG	illegal(SB)		/* unassigned */
	LONG	illegal(SB)		/* coprocessor protocol violation */
	LONG	illegal(SB)		/* format error */
	LONG	illegal(SB)		/* uninitialized interrupt */
/* origin	0040, vector 16 */
	LONG	illegal(SB)		/* 16 */
	LONG	illegal(SB)		/* 17 */
	LONG	illegal(SB)		/* 18 */
	LONG	illegal(SB)		/* 19 */
	LONG	illegal(SB)		/* 20 */
	LONG	illegal(SB)		/* 21 */
	LONG	illegal(SB)		/* 22 */
	LONG	illegal(SB)		/* 23 */
	LONG	illegal(SB)		/* 24 */
/* origin	0064, vector 25 */
	LONG	illegal(SB)		/* level 1 autovector */
	LONG	illegal(SB)		/* level 2 autovector */
	LONG	auto3trap(SB)		/* level 3 autovector */
	LONG	illegal(SB)		/* level 4 autovector */
	LONG	auto5trap(SB)		/* level 5 autovector */
	LONG	auto6trap(SB)		/* level 6 autovector */
	LONG	illegal(SB)		/* level 7 autovector */
/* origin	0080, vector 32 */
	LONG	systrap(SB)		/* trap #0 */
	LONG	cacrtrap(SB)		/* trap #1 */
	LONG	illegal(SB)		/* trap #2 */
	LONG	illegal(SB)		/* trap #3 */
	LONG	illegal(SB)		/* trap #4 */
	LONG	illegal(SB)		/* trap #5 */
	LONG	illegal(SB)		/* trap #6 */
	LONG	illegal(SB)		/* trap #7 */
	LONG	illegal(SB)		/* trap #8 */
	LONG	illegal(SB)		/* trap #9 */
	LONG	illegal(SB)		/* trap #10 */
	LONG	illegal(SB)		/* trap #11 */
	LONG	illegal(SB)		/* trap #12 */
	LONG	illegal(SB)		/* trap #13 */
	LONG	illegal(SB)		/* trap #14 */
	LONG	illegal(SB)		/* trap #15 */
/* origin	00C0, vector 48 */
	LONG	illegal(SB)		/* fpcp branch or set on unordered cond. */
	LONG	illegal(SB)		/* fpcp inexact result */
	LONG	illegal(SB)		/* fpcp divide by zero */
	LONG	illegal(SB)		/* fpcp underflow */
	LONG	illegal(SB)		/* fpcp operand error */
	LONG	illegal(SB)		/* fpcp overflow */
	LONG	illegal(SB)		/* fpcp signaling nan */
	LONG	illegal(SB)		/* unassigned, reserved */
	LONG	illegal(SB)		/* pmmu configuration */
	LONG	illegal(SB)		/* pmmu illegal(SB) operation */
	LONG	illegal(SB)		/* pmmu access level violation */
/* origin	00EC, vector 59 */
	LONG	illegal(SB)		/* vector #59 */
	LONG	illegal(SB)		/* vector #60 */
	LONG	illegal(SB)		/* vector #61 */
	LONG	illegal(SB)		/* vector #62 */
	LONG	illegal(SB)		/* vector #63 */
	LONG	illegal(SB)		/* vector #64 */
	LONG	illegal(SB)		/* vector #65 */
	LONG	illegal(SB)		/* vector #66 */
	LONG	illegal(SB)		/* vector #67 */
	LONG	illegal(SB)		/* vector #68 */
	LONG	illegal(SB)		/* vector #69 */
	LONG	illegal(SB)		/* vector #70 */
	LONG	illegal(SB)		/* vector #71 */
	LONG	illegal(SB)		/* vector #72 */
	LONG	illegal(SB)		/* vector #73 */
	LONG	illegal(SB)		/* vector #74 */
	LONG	illegal(SB)		/* vector #75 */
	LONG	illegal(SB)		/* vector #76 */
	LONG	illegal(SB)		/* vector #77 */
	LONG	illegal(SB)		/* vector #78 */
	LONG	illegal(SB)		/* vector #79 */
	LONG	illegal(SB)		/* vector #80 */
	LONG	illegal(SB)		/* vector #81 */
	LONG	illegal(SB)		/* vector #82 */
	LONG	illegal(SB)		/* vector #83 */
	LONG	illegal(SB)		/* vector #84 */
	LONG	illegal(SB)		/* vector #85 */
	LONG	illegal(SB)		/* vector #86 */
	LONG	illegal(SB)		/* vector #87 */
	LONG	illegal(SB)		/* vector #88 */
	LONG	illegal(SB)		/* vector #89 */
	LONG	illegal(SB)		/* vector #90 */
	LONG	illegal(SB)		/* vector #91 */
	LONG	illegal(SB)		/* vector #92 */
	LONG	illegal(SB)		/* vector #93 */
	LONG	illegal(SB)		/* vector #94 */
	LONG	illegal(SB)		/* vector #95 */
	LONG	illegal(SB)		/* vector #96 */
	LONG	illegal(SB)		/* vector #97 */
	LONG	illegal(SB)		/* vector #98 */
	LONG	illegal(SB)		/* vector #99 */
	LONG	illegal(SB)		/* vector #100 */
	LONG	illegal(SB)		/* vector #101 */
	LONG	illegal(SB)		/* vector #102 */
	LONG	illegal(SB)		/* vector #103 */
	LONG	illegal(SB)		/* vector #104 */
	LONG	illegal(SB)		/* vector #105 */
	LONG	illegal(SB)		/* vector #106 */
	LONG	illegal(SB)		/* vector #107 */
	LONG	illegal(SB)		/* vector #108 */
	LONG	illegal(SB)		/* vector #109 */
	LONG	illegal(SB)		/* vector #110 */
	LONG	illegal(SB)		/* vector #111 */
	LONG	illegal(SB)		/* vector #112 */
	LONG	illegal(SB)		/* vector #113 */
	LONG	illegal(SB)		/* vector #114 */
	LONG	illegal(SB)		/* vector #115 */
	LONG	illegal(SB)		/* vector #116 */
	LONG	illegal(SB)		/* vector #117 */
	LONG	illegal(SB)		/* vector #118 */
	LONG	illegal(SB)		/* vector #119 */
	LONG	illegal(SB)		/* vector #120 */
	LONG	illegal(SB)		/* vector #121 */
	LONG	illegal(SB)		/* vector #122 */
	LONG	illegal(SB)		/* vector #123 */
	LONG	illegal(SB)		/* vector #124 */
	LONG	illegal(SB)		/* vector #125 */
	LONG	illegal(SB)		/* vector #126 */
	LONG	illegal(SB)		/* vector #127 */
	LONG	illegal(SB)		/* vector #128 */
	LONG	illegal(SB)		/* vector #129 */
	LONG	illegal(SB)		/* vector #130 */
	LONG	illegal(SB)		/* vector #131 */
	LONG	illegal(SB)		/* vector #132 */
	LONG	illegal(SB)		/* vector #133 */
	LONG	illegal(SB)		/* vector #134 */
	LONG	illegal(SB)		/* vector #135 */
	LONG	illegal(SB)		/* vector #136 */
	LONG	illegal(SB)		/* vector #137 */
	LONG	illegal(SB)		/* vector #138 */
	LONG	illegal(SB)		/* vector #139 */
	LONG	illegal(SB)		/* vector #140 */
	LONG	illegal(SB)		/* vector #141 */
	LONG	illegal(SB)		/* vector #142 */
	LONG	illegal(SB)		/* vector #143 */
	LONG	illegal(SB)		/* vector #144 */
	LONG	illegal(SB)		/* vector #145 */
	LONG	illegal(SB)		/* vector #146 */
	LONG	illegal(SB)		/* vector #147 */
	LONG	illegal(SB)		/* vector #148 */
	LONG	illegal(SB)		/* vector #149 */
	LONG	illegal(SB)		/* vector #150 */
	LONG	illegal(SB)		/* vector #151 */
	LONG	illegal(SB)		/* vector #152 */
	LONG	illegal(SB)		/* vector #153 */
	LONG	illegal(SB)		/* vector #154 */
	LONG	illegal(SB)		/* vector #155 */
	LONG	illegal(SB)		/* vector #156 */
	LONG	illegal(SB)		/* vector #157 */
	LONG	illegal(SB)		/* vector #158 */
	LONG	illegal(SB)		/* vector #159 */
	LONG	illegal(SB)		/* vector #160 */
	LONG	illegal(SB)		/* vector #161 */
	LONG	illegal(SB)		/* vector #162 */
	LONG	illegal(SB)		/* vector #163 */
	LONG	illegal(SB)		/* vector #164 */
	LONG	illegal(SB)		/* vector #165 */
	LONG	illegal(SB)		/* vector #166 */
	LONG	illegal(SB)		/* vector #167 */
	LONG	illegal(SB)		/* vector #168 */
	LONG	illegal(SB)		/* vector #169 */
	LONG	illegal(SB)		/* vector #170 */
	LONG	illegal(SB)		/* vector #171 */
	LONG	illegal(SB)		/* vector #172 */
	LONG	illegal(SB)		/* vector #173 */
	LONG	illegal(SB)		/* vector #174 */
	LONG	illegal(SB)		/* vector #175 */
	LONG	illegal(SB)		/* vector #176 */
	LONG	illegal(SB)		/* vector #177 */
	LONG	illegal(SB)		/* vector #178 */
	LONG	illegal(SB)		/* vector #179 */
	LONG	illegal(SB)		/* vector #180 */
	LONG	illegal(SB)		/* vector #181 */
	LONG	illegal(SB)		/* vector #182 */
	LONG	illegal(SB)		/* vector #183 */
	LONG	illegal(SB)		/* vector #184 */
	LONG	illegal(SB)		/* vector #185 */
	LONG	illegal(SB)		/* vector #186 */
	LONG	illegal(SB)		/* vector #187 */
	LONG	illegal(SB)		/* vector #188 */
	LONG	illegal(SB)		/* vector #189 */
	LONG	illegal(SB)		/* vector #190 */
	LONG	illegal(SB)		/* vector #191 */
	LONG	illegal(SB)		/* vector #192 */
	LONG	illegal(SB)		/* vector #193 */
	LONG	illegal(SB)		/* vector #194 */
	LONG	illegal(SB)		/* vector #195 */
	LONG	illegal(SB)		/* vector #196 */
	LONG	illegal(SB)		/* vector #197 */
	LONG	illegal(SB)		/* vector #198 */
	LONG	illegal(SB)		/* vector #199 */
	LONG	illegal(SB)		/* vector #200 */
	LONG	illegal(SB)		/* vector #201 */
	LONG	illegal(SB)		/* vector #202 */
	LONG	illegal(SB)		/* vector #203 */
	LONG	illegal(SB)		/* vector #204 */
	LONG	illegal(SB)		/* vector #205 */
	LONG	illegal(SB)		/* vector #206 */
	LONG	illegal(SB)		/* vector #207 */
	LONG	illegal(SB)		/* vector #208 */
	LONG	illegal(SB)		/* vector #209 */
	LONG	illegal(SB)		/* vector #210 */
	LONG	illegal(SB)		/* vector #211 */
	LONG	illegal(SB)		/* vector #212 */
	LONG	illegal(SB)		/* vector #213 */
	LONG	illegal(SB)		/* vector #214 */
	LONG	illegal(SB)		/* vector #215 */
	LONG	illegal(SB)		/* vector #216 */
	LONG	illegal(SB)		/* vector #217 */
	LONG	illegal(SB)		/* vector #218 */
	LONG	illegal(SB)		/* vector #219 */
	LONG	illegal(SB)		/* vector #220 */
	LONG	illegal(SB)		/* vector #221 */
	LONG	illegal(SB)		/* vector #222 */
	LONG	illegal(SB)		/* vector #223 */
	LONG	illegal(SB)		/* vector #224 */
	LONG	illegal(SB)		/* vector #225 */
	LONG	illegal(SB)		/* vector #226 */
	LONG	illegal(SB)		/* vector #227 */
	LONG	illegal(SB)		/* vector #228 */
	LONG	illegal(SB)		/* vector #229 */
	LONG	illegal(SB)		/* vector #230 */
	LONG	illegal(SB)		/* vector #231 */
	LONG	illegal(SB)		/* vector #232 */
	LONG	illegal(SB)		/* vector #233 */
	LONG	illegal(SB)		/* vector #234 */
	LONG	illegal(SB)		/* vector #235 */
	LONG	illegal(SB)		/* vector #236 */
	LONG	illegal(SB)		/* vector #237 */
	LONG	illegal(SB)		/* vector #238 */
	LONG	illegal(SB)		/* vector #239 */
	LONG	illegal(SB)		/* vector #240 */
	LONG	illegal(SB)		/* vector #241 */
	LONG	illegal(SB)		/* vector #242 */
	LONG	illegal(SB)		/* vector #243 */
	LONG	illegal(SB)		/* vector #244 */
	LONG	illegal(SB)		/* vector #245 */
	LONG	illegal(SB)		/* vector #246 */
	LONG	illegal(SB)		/* vector #247 */
	LONG	illegal(SB)		/* vector #248 */
	LONG	illegal(SB)		/* vector #249 */
	LONG	illegal(SB)		/* vector #250 */
	LONG	illegal(SB)		/* vector #251 */
	LONG	illegal(SB)		/* vector #252 */
	LONG	illegal(SB)		/* vector #253 */
	LONG	illegal(SB)		/* vector #254 */
	LONG	illegal(SB)		/* vector #255 */
/*	origin 400 */
/*	start(SB) follows immediately */
