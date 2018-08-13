//
// simulation.cpp
//
// applying random simulation to measure the importance of a link
//

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <math.h>
#include <time.h>
#include "resource.h"
#include "network.h"
#include <windows.h>

int search_next(int);

extern int nnodes;
extern struct PN *nw;
extern FILE *logstream;

//
// random simulation
//
int link_importance()
{
	double r;
	int nsimul;
	int i, k, m, ix, kx;

	for (i = 0; i < nnodes; i++)	// initialization
	{
		for (k = 0; k < nw[i].in_deg; k++)
			nw[i].in_spx[k] = 0.0;
	}

	srand((unsigned)clock());

	nsimul = 10000000;

	for (i = 0; i < nsimul; i++)
	{
		r = (double)rand() / (RAND_MAX + 1);
		m = nnodes * r;	// pick a node, m is its id, 0 =< m < nnodes
		search_next(m);
	}

	// synchronize out_spx[] with in_spx[]
	for (i = 0; i < nnodes; i++)	
	{
		for (k = 0; k < nw[i].out_deg; k++)
		{
			ix = nw[i].out_nbrs[k];
			for (kx = 0; kx < nw[ix].in_deg; kx++)
			{
				if (nw[ix].in_nbrs[kx] == i)
					nw[i].out_spx[k] = nw[ix].in_spx[kx];
			}
		}
	}

	return 0;
}

//
// search further from a given node
// return the id of the nexe node
//
int search_next(int i)
{
	int m, x, ret;	
	double r;
	
	if (nw[i].in_deg == 0) return -1;

	r = (double)rand() / (RAND_MAX + 1);
	m = nw[i].in_deg * r;	// randomly pick a node, m is its index, 0 =< m < nw[i].out_deg
	x = nw[i].in_nbrs[m];	// the next node
	nw[i].in_spx[m]++;
	ret = search_next(x);
		return ret;
}
