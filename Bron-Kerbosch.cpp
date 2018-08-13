// 
// Bron-Kerbosch.cpp
//
// Author: John Liu
// 
// this file contains function that finds maximal cliques of a network node
//

//
// Revision History:
// 2012/11/xx Modification  : basic function works
//

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <conio.h>
#include "resource.h"
#include "network.h"

static int blevel;

extern FILE *logstream;
int compare_numbers(const void *, const void *);

int Bron_Kerbosch(int, int *, int, int *, int, int *, int, struct PN *);
int set_union(int, int *, int, int *, int *);
int set_intersection(int, int *, int, int *, int *);

//
// given a node in a network, find all its maximal cliques
//
int max_cliques(int id, int nnodes, struct PN *nw)
{
	int ncliques;
	int *R, *P, *X;
	int nR, nP, nX;
	int i, k;

	R = (int *)malloc(nnodes * sizeof(int));
	if (R == NULL) return MSG_NOT_ENOUGH_MEMORY;
	P = (int *)malloc(nnodes * sizeof(int));
	if (P == NULL) return MSG_NOT_ENOUGH_MEMORY;
	X = (int *)malloc(nnodes * sizeof(int));
	if (X == NULL) return MSG_NOT_ENOUGH_MEMORY;

	ncliques = 0;

	i = id;
	nP = nw[i].degree;
	for (k = 0; k < nP; k++)
		P[k] = nw[i].nbrs[k];
	nR = nX = 0;	// P and X are empty in the begining
	blevel = 0;
//#ifdef DEBUG
	fwprintf(logstream, L"%d Bron_Kerbosch({", blevel);
	for (k = 0; k < nR; k++) fwprintf(logstream, L"%d, ", R[k]);
	fwprintf(logstream, L"}, {"); fflush(logstream);
	for (k = 0; k < nP; k++) fwprintf(logstream, L"%d, ", P[k]);
	fwprintf(logstream, L"}, {"); fflush(logstream);
	for (k = 0; k < nX; k++) fwprintf(logstream, L"%d, ", X[k]);
	fwprintf(logstream, L"})\n"); fflush(logstream);
//#endif DEBUG
	Bron_Kerbosch(nR, R, nP, P, nX, X, nnodes, nw);

	free(R); free(P); free(X);

	return ncliques;
}

//
//
//
int Bron_Kerbosch(int nR, int *R, int nP, int *P, int nX, int *X, int nnodes, struct PN *nw)
{
	int i, k;
	int idmax, max, pivot;
	int *RR, *PP, *XX;
	int nV, V[1], nNV, *NV;
	int nPUX, *PUX;
	int flag;
	int nRR, nPP, nXX;

	blevel++;
	if (nP == 0 && nX == 0)	// a maximal clique is found
	{
		fwprintf(logstream, L"Maximal clique: ");
		for (i = 0; i < nR; i++)
			fwprintf(logstream, L"%d ", R[i]);
		fwprintf(logstream, L"\n");
		blevel--;
		return 0;
	}

	qsort((void *)R, (size_t)nR, sizeof(int), compare_numbers);
	qsort((void *)P, (size_t)nP, sizeof(int), compare_numbers);
	qsort((void *)X, (size_t)nX, sizeof(int), compare_numbers);
	// find the union of P and X
	PUX = (int *)malloc((nP + nX) * sizeof(int));
	if (PUX == NULL) return MSG_NOT_ENOUGH_MEMORY;
	nPUX = set_union(nP, P, nX, X, PUX);
	// find a vertex in the union with the largest out-degree (the pivot vertex u)
	max = nw[PUX[nPUX-1]].degree; idmax = nPUX-1; 
	for (i = nPUX-1; i >= 0; i--)	// in the reverse order so that the pivot index is close to the begining of the array
	{ 
		if (nw[PUX[i]].degree >= max) 
		{ 
			max = nw[PUX[i]].degree; 
			idmax = i; 
		} 
	}
	pivot = PUX[idmax];

	while (nP > 0)	// for each vertex v in P, but not a neighbor of the pivot vertex u
	{
		nV = 1; V[0] = P[0];	// {v}, the set contain only the vertex v
		flag = 0;
		for (k = 0; k < nw[pivot].degree; k++)
		{
			if (V[0] == nw[pivot].nbrs[k]) { flag = 1; break; }
		}
		if (flag == 1)	// this vertex v is a neighbor of the pivot, ignore it
		{
			nP--; for (k = 0; k < nP; k++) P[k] = P[k+1];
			continue;
		}
		RR = (int *)malloc(nnodes * sizeof(int));	if (RR == NULL) return MSG_NOT_ENOUGH_MEMORY;
		PP = (int *)malloc(nnodes * sizeof(int));	if (PP == NULL) return MSG_NOT_ENOUGH_MEMORY;
		XX = (int *)malloc(nnodes * sizeof(int));	if (XX == NULL) return MSG_NOT_ENOUGH_MEMORY;
		
		nNV = nw[V[0]].degree;
		NV = (int *)malloc(nNV * sizeof(int));	if (NV == NULL) return MSG_NOT_ENOUGH_MEMORY;
		for (k = 0; k < nNV; k++) NV[k] = nw[V[0]].nbrs[k];
		qsort((void *)NV, (size_t)nNV, sizeof(int), compare_numbers);
		fwprintf(logstream, L"Neighbors of %d {", V[0]);
		for (k = 0; k < nNV; k++) fwprintf(logstream, L"%d, ", NV[k]);
		fwprintf(logstream, L"}\n"); fflush(logstream);
		nRR = set_union(nR, R, nV, V, RR);	// the union of R and {v}
		nPP = set_intersection(nP, P, nNV, NV, PP);	// the intersection P and N(v), N(v) is a set consists of the neighbors of v
		nXX = set_intersection(nX, X, nNV, NV, XX);	// the intersection X and N(v)
//#ifdef DEBUG
		for (k = 0; k < blevel; k++) fwprintf(logstream, L"  ");
		fwprintf(logstream, L"%d Bron_Kerbosch({", blevel);
		for (k = 0; k < nRR; k++) fwprintf(logstream, L"%d, ", RR[k]);
		fwprintf(logstream, L"}, {"); fflush(logstream);
		for (k = 0; k < nPP; k++) fwprintf(logstream, L"%d, ", PP[k]);
		fwprintf(logstream, L"}, {"); fflush(logstream);
		for (k = 0; k < nXX; k++) fwprintf(logstream, L"%d, ", XX[k]);
		fwprintf(logstream, L"})\n"); fflush(logstream);
//#endif DEBUG
		Bron_Kerbosch(nRR, RR, nPP, PP, nXX, XX, nnodes, nw);
		free(RR); free(PP); free(XX);
		// find P \ {v}, the set P with v removed
		nP--; for (k = 0; k < nP; k++) P[k] = P[k+1];
		// find X U {v}, the union of X and {v}
		nX = set_union(nX, X, nV, V, X);	// the union X and {v}
	}

	free(PUX);
	blevel--;

#ifdef TEST
	int ret;
	int i;

	int ns1 = 9;
	int s1[9] = {1, 2, 3, 4, 5, 6, 7, 9, 20};
	int ns2 = 8;
	int s2[8] = {2, 4, 6, 8, 10, 12, 14, 16};
	int s1s2[100];

	ret = set_union(ns1, s1, ns2, s2, s1s2);
	fwprintf(logstream, L"\nUnion\n");
	for (i = 0; i < ret; i++)
		fwprintf(logstream, L"[%d] ", s1s2[i]);

	ret = set_intersection(ns1, s1, ns2, s2, s1s2);
	fwprintf(logstream, L"\nIntersection\n");
	for (i = 0; i < ret; i++)
		fwprintf(logstream, L"[%d] ", s1s2[i]);
#endif TEST

	return 0;
}

//
// given two arrays, retrun an arrary that contains their union
// assuming that the input arrays are sorted
// return the size of the result
//
int set_union(int ns1, int *s1, int ns2, int *s2, int *s1s2)
{
	int *p1, *p2, *p1p2;
	int *e1, *e2;
	int i, ns1s2;

	p1 = s1; e1 = p1 + ns1;
	p2 = s2; e2 = p2 + ns2;
	p1p2 = s1s2; ns1s2 = 0;

	while (TRUE)
	{
		if (p1 == e1)
		{
			while (p2 < e2) { *p1p2++ = *p2++; ns1s2++; }
			return ns1s2;
		}
		if (p2 == e2) 
		{
			while (p1 < e1) { *p1p2++ = *p1++; ns1s2++; }
			return ns1s2;
		}

		if (*p1 < *p2) { *p1p2++ = *p1++; ns1s2++; }
		else if (*p2 < *p1) { *p1p2++ = *p2++; ns1s2++; }
		else { *p1p2++ = *p1++; p2++; ns1s2++; }
	} 
}

//
// given two arrays, retrun an arrary that contains their intersection
// assuming that the input arrays are sorted
// return the size of the result
//
int set_intersection(int ns1, int *s1, int ns2, int *s2, int *s1s2)
{
	int *p1, *p2, *p1p2;
	int *e1, *e2;
	int i, ns1s2;

	p1 = s1; e1 = p1 + ns1;
	p2 = s2; e2 = p2 + ns2;
	p1p2 = s1s2; ns1s2 = 0;

	while (p1 != e1 && p2 != e2)
	{
		if (*p1 < *p2) p1++;
		else if (*p2 < *p1) p2++;
		else { *p1p2++ = *p1++; p2++; ns1s2++; }
	} 

	return ns1s2;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_numbers(const void *n1, const void *n2)
{
	int *t1, *t2;
	
	t1 = (int *)n1;
	t2 = (int *)n2;
	if (*t2 < *t1)
		return 1;
	else if (*t2 == *t1)
		return 0;
	else return -1;
}