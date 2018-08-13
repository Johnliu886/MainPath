//
// Floyd-Warshall algorithm
//

//
// Revision History:
// 2012/09/24 Fixed problem : changed the expression of the huge number to 999999E+30 (since the largest number for mantissa is only 6 digits)
// 2015/02/04 Added open MP codes
// 2015/05/18 Commented out all open MP codes, those codes caused the software failed on other PCs 
//            (however, 32bit version does not seem to have the same problem)
//

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "resource.h"
#include "network.h"
// NOTE: including "omp.h" causes the software built running only on the PC that has installed "Visual Studio 2005"
//#include "omp.h"	// John, 2015/02/04

#define HUGE_NUMBER 999999E+30    // was 999999999999999999.0 before 2012/09/24

int getpath(int, int, int, struct FWDIST *, short *, struct SPATH *);
int path_check(int, int, int, struct FWDIST *, short *);
int floyd_warshall(int, double *, struct FWDIST *, short *);
int compare_dist(const void *, const void *);
int count_links_in_a_path(int, int, int, struct FWDIST *, short *);
int getpath2(int, int, int, struct FWDIST *, short *);

int gplevel;
int gplevel2;
extern FILE *logstream;

//
// given a set of start points and a set of end points, find the paths with the smallest overall weight (i.e. largest overall SPx)
//
struct SHORTESTPATH
{
	int st;
	int end;
	int ndx;
};
int find_shortest_paths(int nn, struct FWDIST *fw, short *fw_midpt, int ns, int *st, int ne, int *end, int *npaths, struct SPATH *q)
{
	int i, j, cnt;
	int nlinks;
	int qsize;
	int tie;
	struct FWDISTANCE *tfw;
	struct SHORTESTPATH spaths[MAX_SHORTESTPATH];
	extern struct PN *nw;

	// find paths between start and end points with the smallest overall weight (i.e. largest overall SPx)
	tfw = (struct FWDISTANCE *)Jmalloc(ns * ne * sizeof(struct FWDISTANCE), L"find_shortest_paths: tfw");	
	if (tfw == NULL) return MSG_NOT_ENOUGH_MEMORY;
	for (i = 0; i < ns; i++) 
	{
		for (j = 0; j < ne; j++)
		{
#ifdef TAKE_AVERAGE
			nlinks = count_links_in_a_path(st[i], end[j], nn, fw, fw_midpt);
			//fwprintf(logstream, L"%d %s->%s\n", nlinks, nw[st[i]].alias, nw[end[j]].alias);
			//fflush(logstream);
			if (nlinks == 0)
				tfw[i*ne+j].d.dist = (float)HUGE_NUMBER;
			else
				tfw[i*ne+j].d.dist = fw[st[i]*nn+end[j]].dist / nlinks;
#else
			tfw[i*ne+j].d.dist = fw[st[i]*nn+end[j]].dist;
#endif TAKE_AVERAGE
			tfw[i*ne+j].i = (short)st[i];
			tfw[i*ne+j].j = (short)end[j];
		}
	}

	qsort((void *)tfw, (size_t)ns*ne, sizeof(struct FWDISTANCE), compare_dist);

#ifdef DEBUG
	for (i = 0; i <ns*ne; i++)
		fwprintf(logstream, L"***%d (%d,%d) %f\n", i, tfw[i].i, tfw[i].j, tfw[i].d.dist);
#endif DEBUG

#ifdef DEBUG
	// print out the distance results
	extern struct PN *nw;
	float tmp;
	for (i = 0; i < nn; i++) 
	{
		for (j = 0; j < nn; j++)
		{
			if (fw[i*nn+j].dist == (float)HUGE_NUMBER) tmp = -1; else tmp = fw[i*nn+j].dist;
			fwprintf(logstream, L"***** [%s->%s %.2f:%d] ", nw[i].alias, nw[j].alias, tmp, fw_midpt[i*nn+j]+1);
		}
		fwprintf(logstream, L"\n");
	}
	fwprintf(logstream, L"\n");
	for (i = 0; i < 10; i++)
	{
		if (tfw[i].d.dist == (float)HUGE_NUMBER) tmp = -1; else tmp = tfw[i].d.dist;
		fwprintf(logstream, L"##### {%s->%s %.2f:%d}\n", nw[tfw[i].i].alias, nw[tfw[i].j].alias, tmp, fw_midpt[i]+1);
	}
#endif DEBUG

	if (*npaths > (ns * ne))
		*npaths = ns * ne;
	cnt = 0; i = 0; tie = FALSE;
	//while (i < *npaths || tie == TRUE)
	while ((i < *npaths && cnt < (ns*ne)) || tie == TRUE)	// modified 2011/07/01
	{
		if (tfw[cnt].i != tfw[cnt].j)	// skip the diagonal elements
		{
			spaths[i].st = (int)tfw[cnt].i;
			spaths[i].end = (int)tfw[cnt].j;
			i++;
			if (i >= MAX_SHORTESTPATH)
				break;
			tie = FALSE;
			if (tfw[cnt+1].d.dist == tfw[cnt].d.dist)
				tie = TRUE;
		}
		cnt++;
	}
	qsize = i;

#ifdef DEBUG
	for (i = 0; i < *npaths; i++)
	{ fwprintf(logstream, L"{%d %d} ", spaths[i].st, spaths[i].end); fflush(logstream); }
#endif DEBUG

	// get the path 
	int a, b;
	for (i = 0; i < qsize; i++)
	{
		a = spaths[i].st;
		b = spaths[i].end;
		q[i].len = 0; q[i].hit_a_sink = TRUE;
		q[i].tcnt = (double)fw[a*nn+b].dist;
		q[i].acnt = (double)fw[a*nn+b].dist;
		gplevel = 0;
		if (fw_midpt[a*nn+b] == -1)	// no middle point (no connection)
		{
			q[i].len = 0;
			continue;
		}
		getpath(a, b, nn, fw, fw_midpt, &q[i]);	
	}

	Jfree(tfw, L"find_shortest_paths: tfw");
	*npaths = qsize;

	return 0;
}

//
// return the number of links in a path
//	
int link_cnt;
int count_links_in_a_path(int src, int trgt, int nn, struct FWDIST *fw, short *fw_midpt)
{
	int i;

	link_cnt = 0;
	gplevel2 = 0;
	if (fw_midpt[src*nn+trgt] == -1)	// no middle point (no connection)
		return link_cnt;
	getpath2(src, trgt, nn, fw, fw_midpt);

	return link_cnt;
}

//
// given two points, expand to find shortest path between them (from a to b)
// this function is recursive
// accumulate the number of links along the parh
//
int getpath2(int a, int b, int nn, struct FWDIST *fw, short *fw_midpt)
{
	int k;

	gplevel2++;
	k = fw_midpt[a*nn+b];	// middle point
	if (a == k)	// there is a direct path
	{
		link_cnt++;
		//fwprintf(logstream, L"%d=>", a);
	}
	else
	{
		getpath2(a, k, nn, fw, fw_midpt);
		getpath2(k, b, nn, fw, fw_midpt);
	}
	gplevel2--;
	if (gplevel2 == 0)
	{
		//fwprintf(logstream, L"%d\n", b);
	}

	return 0;
}

//
// given two points, get the shortest path between them (from a to b)
// this function is recursive
//
int getpath(int a, int b, int nn, struct FWDIST *fw, short *fw_midpt, struct SPATH *qq)
{
	int k;

	gplevel++;
	k = fw_midpt[a*nn+b];	// middle point
	if (a == k)	// there is a direct path
	{
		qq->seq[qq->len] = a;
		qq->spx[qq->len++] = (double)fw[a*nn+b].dist;
		//fwprintf(logstream, L"%d->", a);
	}
	else
	{
		getpath(a, k, nn, fw, fw_midpt, qq);
		getpath(k, b, nn, fw, fw_midpt, qq);
	}
	gplevel--;
	if (gplevel == 0)
	{
		qq->seq[qq->len] = b;
		qq->spx[qq->len++] = (double)fw[a*nn+b].dist;
		//fwprintf(logstream, L"%d\n", b);
	}

	return 0;
}

//
// this function finds the minimum weight path among all node pairs in the network
//
struct SPATH qq;
int floyd_warshall(int nn, double *cmat, struct FWDIST *fw, short *fw_midpt)
{
	__int64 indx, indx2, indx3, inn, knn;	// 2015/02/07
	int i, j, k;	

	//initialize data structures
	
	// NOTE: follwoing OpenMP codes slow down rather than speed up the processing, the reasons are unknown, 2015/02/07
	//#pragma omp parallel
	//{
	//#pragma omp for private (j, inn, indx)
	for (i = 0; i < nn; i++) 
	{
		inn = i * nn;
		for (j = 0; j < nn; j++)
		{
			indx = inn + j;
			fw[indx].dist = 0.0; 
			fw_midpt[indx] = -1;
			//fw[i*nn+j].i = i; fw[i*nn+j].j = j;
		}
	//}
	}

	//initialization
	//#pragma omp parallel
	//{
	//#pragma omp for private (j, inn, indx)
	for (i = 0; i < nn; i++) 
	{		
		inn = i * nn;
		for (j = 0; j < nn; j++) 
		{
			indx = inn + j;
			if (cmat[indx] != 0.0)
				fw[indx].dist = (float)cmat[indx];
			else
				fw[indx].dist = (float)HUGE_NUMBER; // no direct link
			if (i == j)  // diagonal elements
				fw[indx].dist = 0.0;
			if (fw[indx].dist != 0.0 && fw[indx].dist < (float)HUGE_NUMBER)
				fw_midpt[indx] = i;
		}
	//}
	}

 	// Main loop
	for (k = 0; k < nn; k++)
	{
		knn = k * nn;
		//#pragma omp parallel
		//{
		//#pragma omp for private (j, inn, knn, indx, indx2, indx3)
		for (i = 0; i < nn; i++) 
		{
			inn = i * nn;
			indx3 = inn + k;
			for (j = 0; j < nn; j++) 
			{
				indx  = inn + j;
				indx2 = knn + j;
				if (fw[indx3].dist == (float)HUGE_NUMBER || fw[indx2].dist == (float)HUGE_NUMBER)	// if one of the link has no connection, don't bother
					continue;
				if (fw[indx].dist > (fw[indx3].dist + fw[indx2].dist)) 
				{			
					fw[indx].dist = fw[indx3].dist + fw[indx2].dist;
					fw_midpt[indx] = k; 
				}
			}
		}
		//}
	}

#ifdef PATH_CHECK
	for (i = 0; i < nn; i++)
	{
		for (j = 0; j < nn; j++)
		{
			gplevel = 0;
			qq.len = 0;
			if (fw_midpt[i*nn+j] == -1)	// no middle point (no connection)
				continue;
			path_check(i, j, nn, fw);
		}
	}
#endif PATH_CHECK

	return 0;
}

//
// this routine is designed for debugging purpose
// given two points, get the shortest path between them (from a to b)
// this function is recursive
//
int path_check(int a, int b, int nn, struct FWDIST *fw, short *fw_midpt)
{
	int k;

	gplevel++;
	k = fw_midpt[a*nn+b];	// middle point
	if (k == -1) // !!!!! something is wrong, display current status !!!!
	{
		int i;
		fwprintf(logstream, L"%d[%d-%d %.2f] ", gplevel, a, b, fw[a*nn+b].dist); 
		for (i = 0; i < qq.len; i++)
			fwprintf(logstream, L"%d->", qq.seq[i]); 
		fwprintf(logstream, L"\n");
		fflush(logstream);
		return 0;
	}
	if (a == k)	// there is a direct path
	{
		qq.seq[qq.len] = a;
		qq.spx[qq.len++] = (double)fw[a*nn+b].dist;
		//fwprintf(logstream, L"%d->", a+1);
	}
	else
	{
		path_check(a, k, nn, fw, fw_midpt);
		path_check(k, b, nn, fw, fw_midpt);
	}
	gplevel--;
	if (gplevel == 0)
	{
		qq.seq[qq.len] = b;
		qq.spx[qq.len++] = (double)fw[a*nn+b].dist;
		//fwprintf(logstream, L"%d\n", b+1);
	}

	return 0;
}


//
// this fucntion is to be called by qsort() only
// 
int compare_dist(const void *n1, const void *n2)
{
	struct FWDISTANCE *t1, *t2;
	
	t1 = (struct FWDISTANCE *)n1;
	t2 = (struct FWDISTANCE *)n2;
	if (t2->d.dist < t1->d.dist)
		return 1;
	else if (t2->d.dist == t1->d.dist)
		return 0;
	else return -1;
}