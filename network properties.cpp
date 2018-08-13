//
// network perties.cpp
//

//
// Revision History:
// 
// 2015/10/29 Basic function works
//

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "resource.h"
#include "network.h"
#include "clustering.h"

extern FILE *logstream;


struct NETWORK_EDGES
{ 
	int from; 
	int to; 
	int weight;	// weight of the link
};	


int random_edge_list(int, int, struct NETWORK_EDGES *);
double weighted_degree_variation_from_edgelist(int, int, struct NETWORK_EDGES *);
double degree_variation_from_edgelist(int, int, struct NETWORK_EDGES *);
int compare_edge_from(const void *, const void *);
int compare_edge_to(const void *, const void *);

//
// calculate the degree centralies of each node in the research group network
// self-links are ignored
//
int degree_centrality_rgroups(int nrgroups, struct RESEARCH_GROUP *rgroups)
{
	int i, k;

	for (i = 0; i < nrgroups; i++)
	{
		rgroups[i].degcentrality = 0;
		for (k = 0; k < rgroups[i].degree; k++)
		{
			if (rgroups[i].nbrs[k] != i)
				rgroups[i].degcentrality += 1;
		}
	}

	return 0;
}

//
// calculate the weighted degree centralies of each node in the research group network
// self-links are ignored
//
int weigted_degree_centrality_rgroups(int nrgroups, struct RESEARCH_GROUP *rgroups)
{
	int i, k;

	for (i = 0; i < nrgroups; i++)
	{
		rgroups[i].wdegcentrality = 0;
		for (k = 0; k < rgroups[i].degree; k++)
		{
			if (rgroups[i].nbrs[k] != i)
				rgroups[i].wdegcentrality += rgroups[i].weight[k];
			else
				rgroups[i].weightedintralinks = rgroups[i].weight[k];
		}
	}

	return 0;
}

//
// calculate the density of a given author network (or subnetwork)
// self-link is not considered
//
double network_density_author(int naus, struct AUTHORS *athrs)
{
	int i, k;
	double density;
	int nlinks;

	nlinks = 0;
	for (i = 0; i < naus; i++)
	{
		for (k = 0; k < athrs[i].degree; k++)
		{
			if (athrs[i].nbrs[k] != i)	// excluding self-links
				nlinks++;
		}
	}
	nlinks = nlinks / 2;

	density = (double)nlinks * 2 / (double)(naus * (naus - 1));

	return density;
}

//
// calculate the 'weighted' degree centralization of a given author network (or subnetwork)
// use a definition created in this project, i.e., normalize against the variations of a random network with the same number of links
// it also takes link weight into consideration
//
double weighted_degree_centralization_author(int naus, struct AUTHORS *athrs)
{
	int i, j, k;
	int dmax;	// largest degree centrality
	double centralization;
	double variation1, variation2, varx;
	double strength;	// this can be think as 'weighted' number of edges
	struct NETWORK_EDGES *edges;

	// find the total strengthes of the network
	strength = 0.0;
	for (i = 0; i < naus; i++)
	{
		for (k = 0; k < athrs[i].degree; k++)
			strength += athrs[i].weight[k];
	}
	strength = strength / 2;	// link weight are counted twice in the above calculation

	// variation for the given network
	variation1 = 0.0; dmax = 0;
	for (i = 0; i < naus; i++)
	{
		if (athrs[i].degree > dmax)
		{
			dmax = athrs[i].degree;
			k = i;
		}
	}
	for (i = 0; i < naus; i++)
	{
		if (i != k)
			variation1 += (double)(dmax - athrs[i].degree);
	}

	// find the variations of random networks with the same 'strength'
	#define NRANDOM_NETWORKS 10000	// it was found that around 10000 times is needed for the average value to converge
	edges = (struct NETWORK_EDGES *)Jmalloc((int)strength * sizeof(struct NETWORK_EDGES), L"degree_centralization_author: edges");
	variation2 = 0.0;
	srand((unsigned)clock());
	for (j = 0; j < NRANDOM_NETWORKS; j++)
	{
		random_edge_list(naus, (int)strength, edges);
#ifdef DEBUG
		fwprintf(logstream, L"\n");
		for (i = 0; i < (int)strength; i++)
		{
			fwprintf(logstream, L"%03d=====>%03d\n", edges[i].from, edges[i].to);
		}
		fwprintf(logstream, L"\n");
#endif DEBUG
		// calculate degree centralization of this randomization
		varx = weighted_degree_variation_from_edgelist(naus, strength, edges);
		variation2 += varx;
		//if ((j % 100) == 0)
		//	fwprintf(logstream, L"j=%d, variation2=%.2f\n",j, variation2/j);
		//fwprintf(logstream, L"varx=%.2f, variation2=%.2f\n", varx, variation2);
	}
	Jfree(edges, L"degree_centralization_author: edges");

	variation2 = variation2 / NRANDOM_NETWORKS;
	centralization = variation1 / variation2;

	return centralization;
}

//
// find degree centralization from a given edge list
// NOTE: no actual network structure is needed, no normalization either
//
double weighted_degree_variation_from_edgelist(int nnodes, int strength, struct NETWORK_EDGES *edges)
{
	int i, k, prev, cnt;
	int dmax;	// largest degree centrality
	double variation;
	int *wdegs;	// weighted degrees

	// find the degree for each node first
	wdegs = (int *)Jmalloc(nnodes * sizeof(int), L"degree_variation_from_edgelist: degs");
	// sort the edge list, based on 'from'	
	qsort((void *)edges, (size_t)strength, sizeof(struct NETWORK_EDGES), compare_edge_from);
	prev = 0; cnt = 0;
	for (k = 0; k < nnodes; k++) wdegs[k] = 0;
	for (i = 0; i < strength; i++)
	{
		if (edges[i].from != prev)
		{
			wdegs[prev] += cnt;
			prev = edges[i].from;
			cnt = 1;
		}
		else
			cnt++;
	}
	wdegs[prev] += cnt; 
	// sort the results, based on 'to'
	qsort((void *)edges, (size_t)strength, sizeof(struct NETWORK_EDGES), compare_edge_to);
#ifdef DEBUG
	fwprintf(logstream, L"\n");
	for (i = 0; i < strength; i++)
	{
		fwprintf(logstream, L"%03d===>%03d\n", edges[i].from, edges[i].to);
	}
	fwprintf(logstream, L"\n");
#endif DEBUG
	prev = 0; cnt = 0;
	for (i = 0; i < strength; i++)
	{
		if (edges[i].to != prev)
		{
			wdegs[prev] += cnt;
			prev = edges[i].to;
			cnt = 1;
		}
		else
			cnt++;
	}
	wdegs[prev] += cnt;
#ifdef DEBUG
	fwprintf(logstream, L"[");
	for (k = 0; k < nnodes; k++) fwprintf(logstream, L"%d ", wdegs[k]);
	fwprintf(logstream, L"]\t");
#endif DEBUG

	// variation for the given network
	variation = 0.0; dmax = 0;
	for (i = 0; i < nnodes; i++)
	{
		if (wdegs[i] > dmax)
		{
			dmax = wdegs[i];
			k = i;
		}
	}
	for (i = 0; i < nnodes; i++)
	{
		if (i != k)
			variation += (double)(dmax - wdegs[i]);
	}

	Jfree(wdegs, L"degree_variation_from_edgelist: wdegs");

	return variation;
}

//
// calculate the degree centralization of a given author network (or subnetwork)
// use the traditional definition, i.e., normalize against the variations of a star network and does not take link weight into consideration
// NOTE: this function is not used because normalization on a star network is not good enough
//
double degree_centralization_traditional_author(int naus, struct AUTHORS *athrs)
{
	int i, k;
	int dmax;	// largest degree centrality
	double centralization;
	double variation1, variation2;

	// variation for the given network
	variation1 = 0.0; dmax = 0;
	for (i = 0; i < naus; i++)
	{
		if (athrs[i].degree > dmax)
		{
			dmax = athrs[i].degree;
			k = i;
		}
	}
	for (i = 0; i < naus; i++)
	{
		if (i != k)
			variation1 += (double)(dmax - athrs[i].degree);
	}

	// variation for the star network
	variation2 = 0.0;
	dmax = naus - 1;	// for a star network
	for (i = 0; i < (naus-1); i++)
		variation2 += (double)(dmax - 1);

	centralization = variation1 / variation2;

	return centralization;
}

//
// create a random link list 
// duplicate links between any two nodes are allowed
//
int random_edge_list(int nnodes, int nedges, struct NETWORK_EDGES *edges)
{
	int m;
	int ndx1, ndx2;

	if (nedges == 0) return 0;

	m = 0;
	while (TRUE)
	{
		ndx1 = (int)((double)nnodes * rand() / (RAND_MAX + 1));
		ndx2 = (int)((double)nnodes * rand() / (RAND_MAX + 1));
		if (ndx1 != ndx2)	// no self-looping
		{
			if (ndx2 > ndx1)	// make it always from low to high
			{
				edges[m].from = ndx1;
				edges[m].to = ndx2;
			}
			else
			{
				edges[m].from = ndx2;
				edges[m].to = ndx1;
			}
			m++;
			if (m == nedges) break;
		}
	}

	return 0;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_edge_from(const void *n1, const void *n2)
{
	struct NETWORK_EDGES *t1, *t2;
	
	t1 = (struct NETWORK_EDGES *)n1;
	t2 = (struct NETWORK_EDGES *)n2;
	if (t2->from < t1->from)
		return 1;
	else if (t2->from == t1->from)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_edge_to(const void *n1, const void *n2)
{
	struct NETWORK_EDGES *t1, *t2;
	
	t1 = (struct NETWORK_EDGES *)n1;
	t2 = (struct NETWORK_EDGES *)n2;
	if (t2->to < t1->to)
		return 1;
	else if (t2->to == t1->to)
		return 0;
	else return -1;
}