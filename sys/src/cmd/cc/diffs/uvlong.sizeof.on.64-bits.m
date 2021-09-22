From geoff@collyer.net Mon Oct  5 02:11:48 PDT 2020
to: jas
subject: cc changes to make type of sizeof for 64-bit targets be uvlong
From: geoff@collyer.net (Geoff Collyer)
Date: Mon, 5 Oct 2020 02:11:47 -0700
MIME-Version: 1.0
Content-Type: text/plain; charset="US-ASCII"
Content-Transfer-Encoding: 7bit

I don't know if I sent you these before, or emphasised them, but I believe
that sizeof on 64-bit targets should be of type uvlong, or effectively
uintptr; uint or ulong are not sufficient.  These cc changes do that and
I've been running with them since sometime last year.

Fossil and venti may well need these changes in order to use > 4GB of
caches.


diff -c /n/dump/2019/0101/sys/src/cmd/cc/com.c ./com.c
/n/dump/2019/0101/sys/src/cmd/cc/com.c:62,67 - ./com.c:62,73
  }
  
  int
+ is64bitptr(void)
+ {
+ 	return ewidth[TIND] > ewidth[TLONG];	/* 64-bit pointers on target? */
+ }
+ 
+ int
  tcomo(Node *n, int f)
  {
  	Node *l, *r;
/n/dump/2019/0101/sys/src/cmd/cc/com.c:588,595 - ./com.c:594,602
  		n->op = OCONST;
  		n->left = Z;
  		n->right = Z;
- 		n->vconst = convvtox(n->type->width, TINT);
- 		n->type = types[TINT];
+ 		o = is64bitptr();
+ 		n->vconst = convvtox(n->type->width, o? TVLONG: TINT);
+ 		n->type = types[o? TVLONG: TINT];
  		break;
  
  	case OFUNC:
diff -c /n/dump/2019/0101/sys/src/cmd/cc/sub.c ./sub.c
/n/dump/2019/0101/sys/src/cmd/cc/sub.c:251,256 - ./sub.c:251,258
  	case BVLONG|BLONG|BINT|BSIGNED:
  		return types[TVLONG];
  
+ 	case BVLONG|BUNSIGNED:
+ 	case BVLONG|BINT|BUNSIGNED:
  	case BVLONG|BLONG|BUNSIGNED:
  	case BVLONG|BLONG|BINT|BUNSIGNED:
  		return types[TUVLONG];
/n/dump/2019/0101/sys/src/cmd/cc/sub.c:691,696 - ./sub.c:693,699
  	}
  	if(n->op == OSUB)
  	if(i == TIND && j == TIND) {
+ 		/* pointer subtraction */
  		w = n->right->type->link->width;
  		if(w < 1) {
  			snap(n->right->type->link);
/n/dump/2019/0101/sys/src/cmd/cc/sub.c:707,721 - ./sub.c:710,725
  		if(w < 1 || x < 1)
  			goto bad;
  		n->type = types[ewidth[TIND] <= ewidth[TLONG]? TLONG: TVLONG];
- 		if(1 && ewidth[TIND] > ewidth[TLONG]){
+ 		if(ewidth[TIND] > ewidth[TLONG]){	/* 64-bit mach? */
  			n1 = new1(OXXX, Z, Z);
  			*n1 = *n;
  			n->op = OCAST;
  			n->left = n1;
  			n->right = Z;
- 			n->type = types[TLONG];
+ 			n->type = types[TVLONG];  /* was TLONG, which seems wrong */
  		}
  		if(w > 1) {
+ 			/* scale down by width */
  			n1 = new1(OXXX, Z, Z);
  			*n1 = *n;
  			n->op = ODIV;

