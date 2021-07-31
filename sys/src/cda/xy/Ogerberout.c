void
gerberout(void)
{
	Side *s, **xp;
	int nxp, xpsize, *xedge, nxe;
	int i, j, k, y0, y1, ymin, ymax;
	int owind, wind = 0;

	if(goo == 0){
		fprint(2, "gerberout: no goo to mash\n");
		return;
	}
	if(goo->n == 0)
		return;

	xpsize = 32;
	xp = malloc(xpsize*sizeof(Side *));
	xedge = malloc(xpsize*sizeof(int));

	qsort(goo->o, goo->n, sizeof(Side *), ysidecompare);

	s = goo->o[0];
	ymin = s->ymin;
	ymax = ymin;
	for(i=0; i<goo->n; i++){
		s = goo->o[i];
		if(s->ymax > ymax)
			ymax = s->ymax;
	}
	if(debug)
		Bprint(&out, "ymin=%d, ymax=%d\n", ymin, ymax);
	if(debug>1)for(i=0; i<goo->n; i++){
		s = goo->o[i];
		Bprint(&out, "%d %c %d %d %d\n",
			i, (s->sense>0?'+':'-'), s->x, s->ymin, s->ymax);
	}
	i = 0;
	for(y0 = ymin; y0 < ymax; y0 = y1){
		while(i < goo->n){
			s = goo->o[i];
			if(y0 < s->ymax)
				break;
			i++;
		}
		j = i;
		while(j < goo->n){
			s = goo->o[j];
			if(y0 < s->ymin)
				break;
			j++;
		}
		if(j <  goo->n){
			s = goo->o[j];
			y1 = s->ymin;
		}else
			y1 = ymax;
		for(k=i; k<j; k++){
			s = goo->o[k];
			if(y0 < s->ymin && s->ymin < y1){
				y1 = s->ymin;
				break;
			}
			if(y0 < s->ymax && s->ymax < y1)
				y1 = s->ymax;
		}
		if(debug>1)
			Bprint(&out, "[%d, %d) [%d, %d)\n",i, j, y0, y1);
		nxp = j-i;
		if(nxp > xpsize){
			xpsize = nxp;
			free(xedge);
			free(xp);
			xp = malloc(xpsize*sizeof(Side *));
			xedge = malloc(xpsize*sizeof(int));
		}
		nxp = 0;
		for(k=i; k<j; k++){
			s = goo->o[k];
			if(y0 < s->ymax)
				xp[nxp++] = s;
		}
		qsort(xp, nxp, sizeof(Side *), xsidecompare);
		nxe = 0;
		for(k=0; k<nxp; k++){
			s = xp[k];
			if(debug>1)
				Bprint(&out, "\t%d %c %d %d %d\n", k,
					(s->sense>0?'+':'-'), s->x,
					s->ymin, s->ymax);
			owind = wind;
			wind += s->sense;
			if((owind <= 0) == (wind <= 0))
				continue;
			if(nxe == 0 || s->x != xedge[nxe-1]){
				xedge[nxe++] = s->x;
				continue;
			}
			if(((nxe&1) == 0) != (wind <= 0))
				--nxe;
			/*if(owind <= 0 && wind > 0 || owind > 0 && wind <= 0){
				if(nxe == 0 || s->x != xedge[nxe-1])
					xedge[nxe++] = s->x;
			}*/
		}
		if(wind != 0)
			error("winding number botch");
		for(k=0; k<nxe; k+=2){
			gerberrect(Rxy(xedge[k], y0, xedge[k+1], y1));
		}
	}
}
