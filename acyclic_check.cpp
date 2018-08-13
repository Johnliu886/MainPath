//
// acyclic_check.cpp
//

//
// Revision History:
// 2012/10/10 Modification  : added a new function simple_loop_check() to check two-way loops and self-citations
// 2012/10/12 Modification  : added to write the acyclic check status to "Acyclic check status.txt" file (for every node processed)
// 2012/11/16 Fixed problems: fixed problems in finding the loop, both functions is_acyclic() and simple_loop_check() are modified
//


#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "resource.h"
#include "network.h"
#include <windows.h>

#define ACYCLIC_CHECK

extern int nnodes;
extern struct NODE *nodes;
extern int nsources;
extern int *sources;	// memory to save index of all sources
extern FILE *logstream;

struct PN *nw;

int diffusion_path(int, int, struct PN *);
int is_acyclic(int, struct PN *);
int label_nodes(int, int, struct PN *);
int simple_loop_check(int, struct PN *, int, int *);
int form_paths(int, struct PN *);
int compare_paths(const void *, const void *);

static int level;	// used by diffusion_path() only
static int loop_cnt;
static int slevel;	// used by is_acyclic() only

#ifndef ACYCLIC_CHECK
int acyclic_check() { return 0; }
int create_network(int nnd) { return 0; }
int link_nodes(int ndx1, int ndx2, double wt, int mode, int mode2) { return 0; }
#endif

#ifdef ACYCLIC_CHECK
//
// check if there are cycles in a 1-mode directed network
//
int acyclic_check()
{
	int i;
	int nsloops;

	// conducting simple loop check first
	fwprintf(logstream, L"\nWorking on simple loop check ...\n");
	nsloops = simple_loop_check(nnodes, nw, nsources, sources);
	if (nsloops > 0)
		return -1;

	// check if the network is really acyclic, 
	// if it is, return immediately, 
	// if it is not, continue to find the loops
	if (is_acyclic(nnodes, nw) == 0)
		return 0;

#ifdef DEBUG
	int k;
	fwprintf(logstream, L"All:\n");
	for (i = 0; i < nnodes; i++)
	{
		fwprintf(logstream, L"Node %d:%s ", i+1, nw[i].name);
		fwprintf(logstream, L"%d: ", nw[i].degree);
		for (k = 0; k < nw[i].degree; k++)
			fwprintf(logstream, L"%d ", nw[i].nbrs[k]+1);
		fwprintf(logstream, L"\n");
	}	
	
	fwprintf(logstream, L"Inward:\n");
	for (i = 0; i < nnodes; i++)
	{
		fwprintf(logstream, L"Node %d:%s ", i+1, nw[i].name);
		fwprintf(logstream, L"%d: ", nw[i].in_deg);
		for (k = 0; k < nw[i].in_deg; k++)
			fwprintf(logstream, L"%d ", nw[i].in_nbrs[k]+1);
		fwprintf(logstream, L"\n");
	}

	fwprintf(logstream, L"Outward:\n");
	for (i = 0; i < nnodes; i++)
	{
		fwprintf(logstream, L"Node %d:%s ", i+1, nw[i].name);
		fwprintf(logstream, L"%d: ", nw[i].out_deg);
		for (k = 0; k < nw[i].out_deg; k++)
			fwprintf(logstream, L"%d ", nw[i].out_nbrs[k]+1);
		fwprintf(logstream, L"\n");
	}
#endif DEBUG

	SYSTEMTIME st;
	GetLocalTime(&st);
	fwprintf(logstream, L"\nThe network is determined to be cyclic.\n");
	fwprintf(logstream, L"\Complete loop check start: %d/%d/%d %d:%d:%d\n", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond); fflush(logstream);
	
	FILE *afile;
	_wfopen_s(&afile, L"Acyclic check status.log", L"w");
	fwprintf(afile, L"Total %d nodes.\n", nnodes); fclose(afile);
	level = 0; loop_cnt = 0;
	for (i = 0; i < nnodes; i++)
	{	
		_wfopen_s(&afile, L"Acyclic check status.log", L"a");
		fwprintf(afile, L"[%d %s]\n", i+1, nw[i].alias); fclose(afile);
		if (nw[i].out_deg == 0)
			continue;
		diffusion_path(i, i, nw);
	}

	GetLocalTime(&st);
	fwprintf(logstream, L"\nComplete loop check end  : %d/%d/%d %d:%d:%d\n\n", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond); fflush(logstream);

	if (loop_cnt != 0)
		return -1;
	else return 0;
}

//
// trace the diffusion path
// this is a recursive function
//
#define SIZE_DSTACK 1000		// maximum depth, increased from 500 to 1000, 2016/06/19
int dstack[SIZE_DSTACK];
int ds_top = 0;					// index of the top of the stack
int diffusion_path(int cur_nd, int org_nd, struct PN *nwk)
{
	int k, ds;
	int new_nd;

	level++;
	nwk[cur_nd].was_here = TRUE;
	dstack[ds_top++] = cur_nd;
	for (k = 0; k < nwk[cur_nd].out_deg; k++)
	{
		new_nd = nwk[cur_nd].out_nbrs[k];
		if (new_nd == org_nd)	// looped back to the original node, do not go further
		{
			loop_cnt++;
			fwprintf(logstream, L"Loop: ");
			for (ds = 0; ds < (ds_top-1); ds++)
				fwprintf(logstream, L"%s->", nwk[dstack[ds]].name);
			fwprintf(logstream, L"%s (", nwk[dstack[ds]].name);
			for (ds = 0; ds < (ds_top-1); ds++)
				fwprintf(logstream, L"%s->", nwk[dstack[ds]].alias);
			fwprintf(logstream, L"%s) |\n", nwk[dstack[ds]].alias);
			continue;
		}
		if (nwk[new_nd].out_deg == 0)	// reached a sink, do not go further
			continue;
		if (nwk[new_nd].was_here == TRUE)	// looped back to a node in the route, do not go further
			continue;
		diffusion_path(new_nd, org_nd, nwk);
	}
	//fwprintf(logstream, L"[%d %d] %d\n", org_nd, cur_nd, level); fflush(logstream);
	level--;
	nwk[cur_nd].was_here = FALSE;
	ds_top--;
	return 0;
}

//
// link two nodes
//
int link_nodes(int ndx1, int ndx2, double wt, int mode, int mode2)
{
	int i;
	int degree, in_deg, out_deg;
	int *tmp1;
	double *tmp2, *tmp3, *tmp4, *tmp5;

	// for node ndx1
	degree = nw[ndx1].degree;
	nw[ndx1].strength += wt;	// overall strength = total weight of their connnections 
	nw[ndx1].nbrs[degree] = ndx2;		// neighbor pointer
	nw[ndx1].weight[degree] = wt;		// weighting of this particular link
	nw[ndx1].degree++;					// overall degree, number of immediate neighbors 
	if (nw[ndx1].degree == nw[ndx1].cur_mem)	// used up the memory allocated
	{
		tmp1 = (int *)Jmalloc((nw[ndx1].cur_mem + N_INCREMENT) * sizeof(int), L"link_nodes: tmp1");
		tmp2 = (double *)Jmalloc((nw[ndx1].cur_mem + N_INCREMENT) * sizeof(double), L"link_nodes: tmp2");
		nw[ndx1].cur_mem += N_INCREMENT;
		for (i = 0; i < nw[ndx1].degree; i++) { tmp1[i] = nw[ndx1].nbrs[i] ; tmp2[i] = nw[ndx1].weight[i]; }
		Jfree(nw[ndx1].nbrs, L"link_nodes: nw[ndx1].nbrs"); Jfree(nw[ndx1].weight, L"link_nodes: nw[ndx1].nbrs");
		nw[ndx1].nbrs = tmp1; nw[ndx1].weight = tmp2;
	}

	out_deg = nw[ndx1].out_deg;
	nw[ndx1].out_strength += wt;	// outward strength
	nw[ndx1].out_nbrs[out_deg] = ndx2;			// neighbor pointer
	nw[ndx1].out_weight[out_deg] = wt;	// outward weight, weight = inward + outward 
	nw[ndx1].out_deg++;	// out-degree
	if (nw[ndx1].out_deg == nw[ndx1].out_cur_mem)	// used up the memory allocated
	{
		tmp1 = (int *)Jmalloc((nw[ndx1].out_cur_mem + N_INCREMENT) * sizeof(int), L"link_nodes: tmp1");
		tmp2 = (double *)Jmalloc((nw[ndx1].out_cur_mem + N_INCREMENT) * sizeof(double), L"link_nodes: tmp2");
		tmp3 = (double *)Jmalloc((nw[ndx1].out_cur_mem + N_INCREMENT) * sizeof(double), L"link_nodes: tmp3");
		tmp4 = (double *)Jmalloc((nw[ndx1].out_cur_mem + N_INCREMENT) * sizeof(double), L"link_nodes: tmp4");
		tmp5 = (double *)Jmalloc((nw[ndx1].out_cur_mem + N_INCREMENT) * sizeof(double), L"link_nodes: tmp5");
		nw[ndx1].out_cur_mem += N_INCREMENT;
		for (i = 0; i < nw[ndx1].out_deg; i++) { tmp1[i] = nw[ndx1].out_nbrs[i] ; tmp2[i] = nw[ndx1].out_weight[i]; tmp3[i] = nw[ndx1].out_spx[i]; tmp4[i] = nw[ndx1].out_dissim[i];  tmp5[i] = nw[ndx1].out_relevancy[i];}
		Jfree(nw[ndx1].out_nbrs, L"link_nodes: nw[ndx1].out_nbrs"); Jfree(nw[ndx1].out_weight, L"link_nodes: "); 
		Jfree(nw[ndx1].out_spx, L"link_nodes: nw[ndx1].out_spx"); Jfree(nw[ndx1].out_dissim, L"link_nodes: nw[ndx1].out_dissim");
		Jfree(nw[ndx1].out_relevancy, L"link_nodes: nw[ndx1].out_relevancy");
		nw[ndx1].out_nbrs = tmp1; nw[ndx1].out_weight = tmp2; nw[ndx1].out_spx = tmp3;  nw[ndx1].out_dissim = tmp4; nw[ndx1].out_relevancy = tmp5;
	}

	// for node ndx2
	if (nw[ndx2].degree == 0)
		wcscpy_s(nw[ndx2].name, MAX_NODE_NAME-1, nodes[ndx2].id);
	degree = nw[ndx2].degree;
	nw[ndx2].strength += wt;	// overall strength = total weight of their connnections 
	nw[ndx2].nbrs[degree] = ndx1;		// neighbor pointer
	nw[ndx2].weight[degree] = wt;		// weighting of this particular link
	nw[ndx2].degree++;					// overall degree, number of immediate neighbors 
	if (nw[ndx2].degree == nw[ndx2].cur_mem)	// used up the memory allocated
	{
		tmp1 = (int *)Jmalloc((nw[ndx2].cur_mem + N_INCREMENT) * sizeof(int), L"link_nodes: tmp1");
		tmp2 = (double *)Jmalloc((nw[ndx2].cur_mem + N_INCREMENT) * sizeof(double), L"link_nodes: tmp2");
		nw[ndx2].cur_mem += N_INCREMENT;
		for (i = 0; i < nw[ndx2].degree; i++) { tmp1[i] = nw[ndx2].nbrs[i] ; tmp2[i] = nw[ndx2].weight[i]; }
		Jfree(nw[ndx2].nbrs, L"link_nodes: nw[ndx2].nbrs"); Jfree(nw[ndx2].weight, L"link_nodes: nw[ndx2].weight");
		nw[ndx2].nbrs = tmp1; nw[ndx2].weight = tmp2;
	}

	in_deg = nw[ndx2].in_deg;
	nw[ndx2].in_strength += wt;// inward strength
	nw[ndx2].in_nbrs[in_deg] = ndx1;	// neighbor pointer
	nw[ndx2].in_weight[in_deg] = wt;	// inward weight 
	nw[ndx2].in_deg++;					// in-degree
	if (nw[ndx2].in_deg == nw[ndx2].in_cur_mem)	// used up the memory allocated
	{
		tmp1 = (int *)Jmalloc((nw[ndx2].in_cur_mem + N_INCREMENT) * sizeof(int), L"link_nodes: tmp1");
		tmp2 = (double *)Jmalloc((nw[ndx2].in_cur_mem + N_INCREMENT) * sizeof(double), L"link_nodes: tmp2");
		tmp3 = (double *)Jmalloc((nw[ndx2].in_cur_mem + N_INCREMENT) * sizeof(double), L"link_nodes: tmp3");
		tmp4 = (double *)Jmalloc((nw[ndx2].in_cur_mem + N_INCREMENT) * sizeof(double), L"link_nodes: tmp4");
		tmp5 = (double *)Jmalloc((nw[ndx2].in_cur_mem + N_INCREMENT) * sizeof(double), L"link_nodes: tmp5");
		nw[ndx2].in_cur_mem += N_INCREMENT;
		for (i = 0; i < nw[ndx2].in_deg; i++) { tmp1[i] = nw[ndx2].in_nbrs[i] ; tmp2[i] = nw[ndx2].in_weight[i]; tmp3[i] = nw[ndx2].in_spx[i]; tmp4[i] = nw[ndx2].in_dissim[i]; tmp5[i] = nw[ndx2].in_relevancy[i];}
		Jfree(nw[ndx2].in_nbrs, L"link_nodes: nw[ndx2].in_nbrs"); Jfree(nw[ndx2].in_weight, L"link_nodes: nw[ndx2].in_weight"); 
		Jfree(nw[ndx2].in_spx, L"link_nodes: nw[ndx2].in_spx"); Jfree(nw[ndx2].in_dissim, L"link_nodes: nw[ndx2].in_dissim");
		Jfree(nw[ndx2].in_relevancy, L"link_nodes: nw[ndx2].in_relevancy");
		nw[ndx2].in_nbrs = tmp1; nw[ndx2].in_weight = tmp2; nw[ndx2].in_spx = tmp3; nw[ndx2].in_dissim = tmp4; nw[ndx2].in_relevancy = tmp5;
	}

	return 0;
}

//
// create a network
//
int create_network(int nnd)
{
	int i;

	nw = (struct PN *)Jmalloc(nnd * sizeof(struct PN), L"create_network: nw");
	if (nw == NULL) return MSG_NOT_ENOUGH_MEMORY;

	for (i = 0; i < nnd; i++) // do necessary initialization
	{ 
		wcscpy_s(nw[i].name, MAX_NODE_NAME-1, nodes[i].id);
		wcscpy_s(nw[i].alias, MAX_ALIAS-1, nodes[i].name);
		nw[i].ndx = i;			// added 2013/10/31
		nw[i].mainp = FALSE;
		nw[i].special = FALSE;
		nw[i].branch = FALSE;
		nw[i].degree = 0;		// overall degree, number of immediate neighbors 
		nw[i].strength = 0.0;	// overall strength = total weight of their connnections 
		nw[i].nbrs = (int *)Jmalloc(N_BASE * sizeof(int), L"create_network: nw[i].nbrs");				// neighbor pointer
		nw[i].weight = (double *)Jmalloc(N_BASE * sizeof(double), L"create_network: nw[i].weight");		// weighting of this particular link
		nw[i].cur_mem = N_BASE;// current memory allocated
		nw[i].in_deg = 0;		// in-degree
		nw[i].in_strength = 0.0;		// inward strength
		nw[i].in_nbrs = (int *)Jmalloc(N_BASE * sizeof(int), L"create_network: nw[i].in_nbrs");			// neighbor pointer
		nw[i].in_weight = (double *)Jmalloc(N_BASE * sizeof(double), L"create_network: nw[i].in_weight");	// inward weight 
		nw[i].in_relevancy = (double *)Jmalloc(N_BASE * sizeof(double), L"create_network: nw[i].in_relevancy");	// inward relevancy
		nw[i].in_spx = (double *)Jmalloc(N_BASE * sizeof(double), L"create_network: nw[i].in_spx");		// inward SPC, SPLC or SPNP 
		nw[i].in_dissim = (double *)Jmalloc(N_BASE * sizeof(double), L"create_network: nw[i].in_dissim");	// dissimilarity with the inward nodes 
		nw[i].in_cur_mem = N_BASE;	// current memory allocated
		nw[i].out_deg = 0;		// out-degree
		nw[i].out_strength = 0.0;	// outward strength
		nw[i].out_nbrs = (int *)Jmalloc(N_BASE * sizeof(int), L"create_network: nw[i].out_nbrs");			// neighbor pointer
		nw[i].out_weight = (double *)Jmalloc(N_BASE * sizeof(double), L"create_network: nw[i].out_weight");	// outward weight, weight = inward + outward 
		nw[i].out_relevancy = (double *)Jmalloc(N_BASE * sizeof(double), L"create_network: nw[i].out_relevancy");	// outward relevancy
		nw[i].out_spx = (double *)Jmalloc(N_BASE * sizeof(double), L"create_network: nw[i].out_spx");		// outward SPC, SPLC or SPNP 
		nw[i].out_dissim = (double *)Jmalloc(N_BASE * sizeof(double), L"create_network: nw[i].out_dissim");	// dissimilarity with the outward nodes 
		nw[i].out_cur_mem = N_BASE;// current memory allocated
		nw[i].was_here = FALSE;
	}	

	return 0;
}
#endif ACYCLIC_CHECK

//
// check if the given network is acyclic
// it does not point out the nodes that form the cycles
// return 0 if the network is acyclic, return -1 otherwise
//
#define NEVER_VISITED -2
#define IS_VISITED -1
int is_acyclic(int nnodes, struct PN *nw)
{
	int i, j, k, m;
	_int64 spx;

	for (i = 0; i < nnodes; i++) nw[i].was_here = NEVER_VISITED;	// indicate "was not here" 
		
#ifdef OBSOLETE	// became obsolete on 2012/11/16
	// search begins from all sources
	for (m = 0; m < nsources; m++)	
	{
		i = sources[m];
		slevel = 0;	
		if (label_nodes(i, i, nw) == -1)
			return -1;		
	}
#endif OBSOLETE
	// the above codes are changed to the following codes on 2012/11/16, has to go over all the nodes
	for (i = 0; i < nnodes; i++)	
	{
		slevel = 0;	
		if (label_nodes(i, i, nw) == -1)
			return -1;		
	}

	return 0;
}

//
// given a node, label the node with the source ID that leading the current trace
// recursive, depth-first algorithm
//
int label_nodes(int cur_source, int nid, struct PN *nwk)
{
	int j, k;
	int new_nd;

	if (nwk[nid].was_here == IS_VISITED) return 0;

	slevel++;
	nwk[nid].was_here = cur_source;
	for (k = 0; k < nwk[nid].out_deg; k++)
	{
		j = nwk[nid].out_nbrs[k];
		if (nwk[j].was_here == cur_source)	// trace to previous labeled node, loop encountered
			return -1;
		if (label_nodes(cur_source, j, nwk) == -1)
			return -1;
	}
	slevel--;

	nwk[nid].was_here = IS_VISITED;	// change the status to IS_VISITED
	return 0;
}

//
// check whether there are two-way loops in the network
//
#define NMSIZE 16
struct TPATH
{
	int direction;	// forward/backward
	int from;
	int to;
	wchar_t nm[NMSIZE] ;// paths expressed in text
};
static int npaths;
static struct TPATH *paths; 
int simple_loop_check(int nnodes, struct PN *nw, int nsrcs, int *srcs)
{
	int i, k, m;
	int nloops, ndups;

	paths = (struct TPATH *)Jmalloc(nnodes * 30 * 2 * sizeof(struct TPATH), L"simple_loop_check: paths");	// estimate 30 times the number of nodes, then doubles it

	for (i = 0; i < nnodes; i++) nw[i].was_here = FALSE;	// clear the "was_here" flag 		
	// search begins from all sources
	npaths = 0;

#ifdef OBSOLETE	// became obsolete on 2012/11/16
	for (m = 0; m < nsrcs; m++)	
	{
		i = srcs[m];
		slevel = 0;	
		form_paths(i, nw);
	}
#endif OBSOLETE
	// the above codes are changed to the following codes on 2012/11/16, has to go over all the nodes
	for (i = 0; i < nnodes; i++)	
	{
		slevel = 0;	
		form_paths(i, nw);
	}

	fwprintf(logstream, L"npaths = %d\n", npaths);

	// append the reversed paths to the end of the original path array
	for (k = 0; k < npaths; k++)
	{
		paths[k].direction = 0;
		paths[npaths+k].from = paths[k].to;
		paths[npaths+k].to = paths[k].from;
		paths[npaths+k].direction = 1;
		swprintf(paths[k].nm, NMSIZE, L"%07d %07d", paths[k].from, paths[k].to);
		swprintf(paths[npaths+k].nm, NMSIZE, L"%07d %07d", paths[npaths+k].from, paths[npaths+k].to);
	}

	qsort((void *)paths, (size_t)2*npaths, sizeof(struct TPATH), compare_paths);

#ifdef DEBUG
	for (k = 0; k < 2*npaths; k++)
		fwprintf(logstream, L"%s %s\n", nw[paths[k].from].name, nw[paths[k].to].name);
#endif DEBUG

	wchar_t prev[NMSIZE];
	nloops = 0; ndups = 0;
	wcscpy(prev, paths[0].nm);
	for (k = 1; k < 2*npaths; k++)
	{
		if (wcscmp(paths[k].nm, prev) == 0)	// repitition means there is a loop or duplicate links here
		{
			if (paths[k-1].direction == 0 && paths[k].direction == 0) // both in forward direction, duplicate links
			{
				fwprintf(logstream, L"Duplicate link: %s->%s (%s->%s)\n", nw[paths[k-1].from].name, nw[paths[k-1].to].name, nw[paths[k-1].from].alias, nw[paths[k-1].to].alias);
				ndups++;
			}
			else if (paths[k-1].direction == 1 && paths[k].direction == 1) // both in backward direction, duplicate links, but is identified in the above check
			{
				// do nothing
			}
			else // direction for both are different, two-way loops and self-citation
			{
				fwprintf(logstream, L"Loop: %s->%s (%s->%s)\n", nw[paths[k-1].from].name, nw[paths[k-1].to].name, nw[paths[k-1].from].alias, nw[paths[k-1].to].alias);
				nloops++;
			}
		}
		wcscpy(prev, paths[k].nm);
	}
	fwprintf(logstream, L"Number of duplicate links: %d\n", ndups);
	fwprintf(logstream, L"Number of links in simple loops: %d\n", nloops);

	Jfree(paths, L"simple_loop_check: paths");

	return nloops;
}

//
// given a node, establish paths
// recursive, depth-first algorithm
//
int form_paths(int nid, struct PN *nwk)
{
	int j, k;
	int new_nd;

	if (nwk[nid].was_here == TRUE) return 0;

	slevel++;
	nwk[nid].was_here = TRUE;
	for (k = 0; k < nwk[nid].out_deg; k++)
	{
		j = nwk[nid].out_nbrs[k];
		paths[npaths].from = nid;
		paths[npaths++].to = j;
		form_paths(j, nwk);
	}
	slevel--;

	return 0;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_paths(const void *n1, const void *n2)
{
	struct TPATH *t1, *t2;
	
	t1 = (struct TPATH *)n1;
	t2 = (struct TPATH *)n2;
	if (wcscmp(t2->nm, t1->nm) < 0)
		return 1;
	else if (wcscmp(t2->nm, t1->nm) == 0)
		return 0;
	else return -1;
}
