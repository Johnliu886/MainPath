//
// SPx.cpp
//
// Calculate SPC, SPLC and SPNP for an acyclic network
//
// 2012/10/03 Fixed problem  : fixed a very special case: an isolated node is cited by itself, in function topological_sort()
// 2013/05/23 Added function : added a new traversal count option SPGD(). This requires to calculate nw[i].decay_backward in the functoon get_backward()
//                             also, rename the original SPDC() to SPAD().
// 2014/01/07 Added fucntion : added codes to handle SPHD
// 2016/08/11 Added function : Added code to check if there are negative SPX values (in the function SPx()). If there are, leave MainPath program immediately. 
//                             This is due to an integer (_int64) overflow occurs while calculating traversal weights.
// 2018/05/03 Fixed problem  : SPX calculation is changed from integer (_int64) to floating point (double). 
//                             This resolve the SPX overflow problem, yet does not seem to slow down the calculation.
//

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <math.h>
#include "resource.h"
#include "network.h"
#include <windows.h>

//
// NOTE: according to MSDN "Double Structure" article, 
//       maximum presicion for double is 15 decimal digits, and
//       maximum presision for int64 is 19 decimal digits.
//       there is a potential problem in converting from int64 to double!
//
static int slevel;				// used in depth-first search
static _int64 paths_forward;	// used in find_paths_forward()
static _int64 paths_backward;	// used in find_paths_backward()
static _int64 nodes_forward;	// used in find_nodes_forward()
static _int64 nodes_backward;	// used in find_nodes_backward()
static _int64 sinks_forward;	// used in find_sinks_forward()
static _int64 sources_backward;	// used in find_sources_forward()
static int *t_order;			// point to the index of nodes
static int cur_p;				// current position in t_order array
static int forward_cnt;			// the count of calling find_paths_forward() or find_nodes_forward()
static int backward_cnt;		// the count of calling find_paths_backward() or find_nodes_backward()

int SPC(int, struct PN *);
int SPLC(int, struct PN *);
int SPNP(int, struct PN *);
int find_paths_forward(int, struct PN *);
int find_paths_backward(int, struct PN *);
int find_nodes_forward(int, struct PN *);
int find_nodes_backward(int, struct PN *);
int find_sinks_forward(int, struct PN *);
int find_sources_backward(int, struct PN *);
int topological_sort(int, struct PN *);
int mark_nodes(int, struct PN *);
int get_forward(int, struct PN *);
int get_backward(int, struct PN *, double);
int	find_depth_for_each_link(int, int, struct PN *);
extern int SPAD(int, struct PN *, double);
extern int SPGD(int, struct PN *);
extern int SPHD(int, struct PN *);

extern int nsources;
extern int *sources;	// memory to save index of all sources
extern FILE *logstream;

//
// calculate SPx for all arcs
//
int SPx(int stype, int nnodes, struct PN *nw, double decay)
{
	int i, ret;
	SYSTEMTIME st;
	GetLocalTime(&st);
	fwprintf(logstream, L"\nSPx calculation start: %d/%d/%d %d:%d:%d\n", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	
#ifdef USING_TOPOLOGICAL_SORT
	// conduct topological sort
	t_order = (int *)Jmalloc(nnodes * sizeof(int), L"SPx: t_order");
	if (t_order == NULL) return MSG_NOT_ENOUGH_MEMORY;
	for (i = 0; i < nnodes; i++) { t_order[i] = -1; nw[i].t_order = -1; nw[i].level = -1; }
	ret = topological_sort(nnodes, nw);	
	if (ret != 0) return -1;
#ifdef DEBUG
	fwprintf(logstream, L"\nTopological sort results (in node id order):\n");
	for (i = 0; i < nnodes; i++)
	{
		if (nw[i].t_order == -1) fwprintf(logstream, L"%d [%d]: %s\n", i, nw[i].t_order, nw[i].alias);
		else fwprintf(logstream, L"%d %d: %s\n", i, nw[i].t_order, nw[i].alias);
		fflush(logstream);
	}
	fwprintf(logstream, L"\n\n");
	fwprintf(logstream, L"\nTopological sort results (in topological order):\n");
	for (i = 0; i < nnodes; i++)
		fwprintf(logstream, L"%d %d: %s\n", i, t_order[i], nw[t_order[i]].alias);
#endif DEBUG

	get_forward(nnodes, nw);
	get_backward(nnodes, nw, decay);

#endif USING_TOPOLOGICAL_SORT

#ifdef OKAY_BUT_SLOW
	// initialize "forward" and "backward"
	for (i = 0; i < nnodes; i++) { nw[i].forward = nw[i].backward = -1; }
	forward_cnt = backward_cnt = 0;
#endif OKAY_BUT_SLOW

	switch (stype)
	{
	case S_SPC:
		SPC(nnodes, nw);
		break;
	case S_SPLC:
		SPLC(nnodes, nw);
		break;
	case S_SPNP:
		SPNP(nnodes, nw);
		break;
	case S_SPAD:
		SPAD(nnodes, nw, decay);
		break;
	case S_SPGD:
		SPGD(nnodes, nw);	// for SPGD, decay ratio has been considered in the function get_backward()
		break;
	case S_SPHD:
		SPHD(nnodes, nw);	// for SPHD, there is no decay ratio
		break;
	default:
		break;
	}

	// following codes are added 2016/08/11, to detect overflow out of _int64
	int k;
	for (i = 0; i < nnodes; i++)
	{
		for (k = 0; k < nw[i].out_deg; k++)
		{
			if (nw[i].out_spx[k] < 0.0) // overflow detected
			{	
				fwprintf(logstream, L"Error: SPX value overflow.\n");
				Jfree(t_order, L"SPx: t_order");
				return MSG_SPX_OVERFLOW;
			}
		}
	}

	GetLocalTime(&st);
	fwprintf(logstream, L"SPx calculation end  : %d/%d/%d %d:%d:%d\n\n", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	Jfree(t_order, L"SPx: t_order");

	return 0;
}

#ifdef USING_TOPOLOGICAL_SORT
//
// topological sort
// algorithm source: Wikipedia article "Topological Sorting"
// return -1 if the network is not acyclic
//
int topological_sort(int nnodes, struct PN *nw)
{
	int i, j, k, m;

	for (i = 0; i < nnodes; i++) nw[i].was_here = FALSE;	// clear the "was_here" flag 
		
	cur_p = 0;
	// search begins from all sources
	for (m = 0; m < nsources; m++)	
	{
		i = sources[m];
		slevel = 0;	
		mark_nodes(i, nw);
	}

	// assign the 'isolates' with the lowest order
	for (i = 0; i < nnodes; i++) 
	{ 
		if (nw[i].type == T_ISOLATE && cur_p < nnodes) // condition "cur_p < nnodes" is added on 2011/12/22 to make sure that t_order[] array is not overflowed
		{
			nw[i].t_order = cur_p;	// added 2012/10/03
			nw[i].level = slevel;	// added 2013/04/25
			t_order[cur_p++] = i;
			nw[i].was_here = TRUE;
		}
		else if (nw[i].type == T_INTERMEDIATE && nw[i].out_deg == 1 && nw[i].out_nbrs[0] == i) // a very special case: an isolated node is cited by itself, added 2012/10/03
		{
			nw[i].t_order = cur_p;	
			nw[i].level = slevel;	// added 2013/04/25
			t_order[cur_p++] = i;
			nw[i].was_here = TRUE;
		}
	}

	return 0;
}

//
// given a node, mark nodes
// recursive, depth-first algorithm
//
int mark_nodes(int nid, struct PN *nwk)
{
	int j, k;
	int new_nd;

	if (nwk[nid].was_here == TRUE) return 0;

	slevel++;
	nwk[nid].was_here = TRUE;
	for (k = 0; k < nwk[nid].out_deg; k++)
	{
		j = nwk[nid].out_nbrs[k];
		mark_nodes(j, nwk);
	}
	nwk[nid].t_order = cur_p;	// added 2012/10/03
	nwk[nid].level = slevel;	// added 2013/04/25
	t_order[cur_p++] = nid;
	slevel--;

	return 0;
}

//
// get the number of paths and nodes forward 
//
int get_forward(int nnodes, struct PN *nw)
{	
	int i, j, k, m, p;

	for (p = 0; p < nnodes; p++)
	{
		i = t_order[p];		// in the order of the result of topological sort
		if (nw[i].out_deg == 0)	
		{ 
			nw[i].nodes_forward = 1.0;	// set to 1 for a sink
			nw[i].paths_forward = 1.0;	// set to 1 for a sink
		}
		else
		{
			nw[i].paths_forward = nw[i].nodes_forward = 0;
			for (k = 0; k < nw[i].out_deg; k++)
			{
				j = nw[i].out_nbrs[k];
				nw[i].paths_forward += nw[j].paths_forward;
				nw[i].nodes_forward += nw[j].nodes_forward;
			}
			//nw[i].nodes_forward++; // plus itself 
			nw[i].nodes_forward += 1.0; // plus itself 
		}
	}
#ifdef DEBUG
	fwprintf(logstream, L"\nget_forward() results:\n");
	for (i = 0; i < nnodes; i++)
	{			
		fwprintf(logstream, L"%s: %.0f %.0f\n", nw[i].alias, (double)nw[i].paths_forward, (double)nw[i].nodes_forward);
		fflush(logstream);
	}
	fwprintf(logstream, L"\n\n");
#endif DEBUG

	return 0;
}

//
// get the number of paths and nodes backward 
//
int get_backward(int nnodes, struct PN *nw, double decay)
{	
	int i, j, k, m, p;

	for (p = 0; p < nnodes; p++)		// take one node from t_order[], one by one
	{
		i = t_order[nnodes - p - 1];	// in the reverse order of the result of topological sort
		if (nw[i].in_deg == 0)	
		{ 
			nw[i].nodes_backward = 1.0;	// set to 1 for a source
			nw[i].paths_backward = 1.0;	// set to 1 for a source
			nw[i].decay_backward = 1.0;	// set to 1 for a source
		}
		else
		{
			nw[i].paths_backward = nw[i].nodes_backward = 0.0;
			nw[i].decay_backward = 0.0;
			for (k = 0; k < nw[i].in_deg; k++)
			{
				j = nw[i].in_nbrs[k];
				nw[i].paths_backward += nw[j].paths_backward;
				nw[i].nodes_backward += nw[j].nodes_backward;
				nw[i].decay_backward += decay * nw[j].decay_backward;
			}
			//nw[i].nodes_backward++; // plus itself 
			nw[i].nodes_backward += 1.0; // plus itself 
			nw[i].decay_backward += 1.0; // plus itself
		}
	}
#ifdef DEBUG
	fwprintf(logstream, L"\nget_backward() results:\n");
	for (i = 0; i < nnodes; i++)
	{			
		fwprintf(logstream, L"%s: %.0f %.0f\n", nw[i].alias, (double)nw[i].paths_backward, (double)nw[i].nodes_backward);
		fflush(logstream);
	}
	fwprintf(logstream, L"\n\n");
#endif DEBUG

	return 0;
}

//
// Search Path Count
//
int SPC(int nnodes, struct PN *nw)
{
	int i, j, k, m, p;
	double spx;	// changed from _int64 to double, 2018/05/03

	for (i = 0; i < nnodes; i++)
	{
		for (k = 0; k < nw[i].out_deg; k++)
		{
			j = nw[i].out_nbrs[k];
			spx = nw[j].paths_forward * nw[i].paths_backward;
			nw[i].out_spx[k] = (double)spx;
			for (m = 0; m < nw[j].in_deg; m++)
			{
				if (nw[j].in_nbrs[m] == i)
					nw[j].in_spx[m] = (double)spx;
			}
		}
	}

	return 0;
}

//
// Search Path Link Count
//
int SPLC(int nnodes, struct PN *nw)
{
	int i, j, k, m, p;
	double spx;	// changed from _int64 to double, 2018/05/03

	for (i = 0; i < nnodes; i++)
	{
		for (k = 0; k < nw[i].out_deg; k++)
		{
			j = nw[i].out_nbrs[k];
			//spx = nw[j].paths_forward * nw[i].nodes_backward;
			spx = nw[j].paths_forward * nw[i].nodes_backward;
			nw[i].out_spx[k] = (double)spx;
			for (m = 0; m < nw[j].in_deg; m++)
			{
				if (nw[j].in_nbrs[m] == i)
					nw[j].in_spx[m] = (double)spx;
			}
		}
	}

	return 0;
}

//
// Search Path Node Pair
//
int SPNP(int nnodes, struct PN *nw)
{
	int i, j, k, m, p;
	double spx;	// changed from _int64 to double, 2018/05/03

	for (i = 0; i < nnodes; i++)
	{
		for (k = 0; k < nw[i].out_deg; k++)
		{
			j = nw[i].out_nbrs[k];
			spx = nw[j].nodes_forward * nw[i].nodes_backward;
			nw[i].out_spx[k] = (double)spx;
			for (m = 0; m < nw[j].in_deg; m++)
			{
				if (nw[j].in_nbrs[m] == i)
					nw[j].in_spx[m] = (double)spx;
			}
		}
	}

	return 0;
}

#endif USING_TOPOLOGICAL_SORT

#ifdef OKAY_BUT_SLOW
//
// Search Path Count
//
int SPC(int nnodes, struct PN *nw)
{
	int i, j, k, m;
	_int64 spx;

	for (i = 0; i < nnodes; i++)
	{
		for (k = 0; k < nw[i].out_deg; k++)
		{
			paths_forward = 0; slevel = 0; 
			j = nw[i].out_nbrs[k];
			find_paths_forward(j, nw); forward_cnt++;
			paths_backward = 0; slevel = 0; 
			find_paths_backward(i, nw); backward_cnt++;
			spx = paths_forward * paths_backward;
			nw[i].out_spx[k] = (double)spx;
			for (m = 0; m < nw[j].in_deg; m++)
			{
				if (nw[j].in_nbrs[m] == i)
					nw[j].in_spx[m] = (double)spx;
			}
		}
	}

#ifdef NATURAL_LOG
	// take natural log of all SPx values
	for (i = 0; i < nnodes; i++)
	{
		for (k = 0; k < nw[i].out_deg; k++)
		{ 
			j = nw[i].out_nbrs[k];
			nw[i].out_spx[k] = log(nw[i].out_spx[k]);
		}
	}
#endif NATURAL_LOG

#ifdef DEBUG_SPX
	fwprintf(logstream, L"SPC:\n");
	for (i = 0; i < nnodes; i++)
	{
		for (k = 0; k < nw[i].out_deg; k++)
		{ 
			j = nw[i].out_nbrs[k];
			fwprintf(logstream, L"%s->%s: %.2f\n", nw[i].name, nw[j].name, nw[i].out_spx[k]);
		}
	}
#endif DEBUG_SPX

	return 0;
}

//
// Search Path Link Count
//
int SPLC(int nnodes, struct PN *nw)
{
	int i, j, k, m;
	_int64 spx;


	for (i = 0; i < nnodes; i++)
	{
		for (k = 0; k < nw[i].out_deg; k++)
		{
			paths_forward = 0; slevel = 0; 
			j = nw[i].out_nbrs[k];
			find_paths_forward(j, nw);
			nodes_backward = 1; slevel = 0; // initialize to 1 to include self
			find_nodes_backward(i, nw);
			spx = paths_forward * nodes_backward;
			nw[i].out_spx[k] = (double)spx;
			for (m = 0; m < nw[j].in_deg; m++)
			{
				if (nw[j].in_nbrs[m] == i)
					nw[j].in_spx[m] = (double)spx;
			}
		}
	}

#ifdef DEBUG_SPX
	fwprintf(logstream, L"SPLC:\n");
	for (i = 0; i < nnodes; i++)
	{
		for (k = 0; k < nw[i].out_deg; k++)
		{ 
			fwprintf(logstream, L"%s->%s: %.2f\n", nw[i].name, nw[nw[i].out_nbrs[k]].name, nw[i].out_spx[k]);
		}
	}
#endif DEBUG_SPX

	return 0;
}

//
// Search Path Node Pair
//
int SPNP(int nnodes, struct PN *nw)
{
	int i, j, k, m;
	_int64 spx;

	for (i = 0; i < nnodes; i++)
	{
		for (k = 0; k < nw[i].out_deg; k++)
		{
			nodes_forward = 1; slevel = 0;	// initialize to 1 to include self
			j = nw[i].out_nbrs[k];
			find_nodes_forward(j, nw);
			nodes_backward = 1; slevel = 0; // initialize to 1 to include self
			find_nodes_backward(i, nw);
			spx = nodes_forward * nodes_backward;
			nw[i].out_spx[k] = (double)spx;
			for (m = 0; m < nw[j].in_deg; m++)
			{
				if (nw[j].in_nbrs[m] == i)
					nw[j].in_spx[m] = (double)spx;
			}
		}
	}

#ifdef DEBUG_SPX
	fwprintf(logstream, L"SPNP:\n");
	for (i = 0; i < nnodes; i++)
	{
		for (k = 0; k < nw[i].out_deg; k++)
		{ 
			fwprintf(logstream, L"%s->%s: %.2f\n", nw[i].name, nw[nw[i].out_nbrs[k]].name, nw[i].out_spx[k]);
		}
	}
#endif DEBUG_SPX

	return 0;
}

//
// given a node, find all nodes in the paths leading to sinks
// 
// recursive, depth-first algorithm
//
int find_nodes_forward(int nid, struct PN *nwk)
{
	int k;
	int new_nd;

	// set node count to 1 if the given node is a sink
	if (nwk[nid].out_deg == 0)	{ nodes_forward = 1; return 0; }

	slevel++;
	for (k = 0; k < nwk[nid].out_deg; k++)
	{
		nodes_forward++;
		new_nd = nwk[nid].out_nbrs[k];
		if (nwk[new_nd].out_deg == 0) continue;	// reached a sink, do not go further
		find_nodes_forward(new_nd, nwk);
	}
	slevel--;

	return 0;
}

//
// given a node, find all nodes in the paths trace back to sources 
// recursive, depth-first algorithm
//
int find_nodes_backward(int nid, struct PN *nwk)
{
	int k;
	int new_nd;

	// set node count to 1 if the given node is a source
	if (nwk[nid].in_deg == 0)	{ nodes_backward = 1; return 0; }

	slevel++;
	for (k = 0; k < nwk[nid].in_deg; k++)
	{
		nodes_backward++;
		new_nd = nwk[nid].in_nbrs[k];
		if (nwk[new_nd].in_deg == 0) continue;	// reached a source, do not trace back further
		find_nodes_backward(new_nd, nwk);
	}
	slevel--;

	return 0;
}

//
// given a node, find all paths end at a sink (ahead of the node) 
// recursive, depth-first algorithm
//
int find_paths_forward(int nid, struct PN *nwk)
{
	int k;
	int new_nd;

	// set path count to 1 if the given node is a sink
	if (nwk[nid].out_deg == 0)	{ paths_forward = 1; return 0; }

	slevel++;
	for (k = 0; k < nwk[nid].out_deg; k++)
	{
		new_nd = nwk[nid].out_nbrs[k];
		if (nwk[new_nd].out_deg == 0)	// reached a sink, do not go further
		{
			paths_forward++;
			continue;
		}
		find_paths_forward(new_nd, nwk);
	}
	slevel--;

	return 0;
}

//
// given a node, find all paths coming from a source 
// recursive, depth-first algorithm
//
int find_paths_backward(int nid, struct PN *nwk)
{
	int k;
	int new_nd;

	// set path count to 1 if the given node is a source
	if (nwk[nid].in_deg == 0)	{ paths_backward = 1; return 0; }

	slevel++;
	for (k = 0; k < nwk[nid].in_deg; k++)
	{
		new_nd = nwk[nid].in_nbrs[k];
		if (nwk[new_nd].in_deg == 0)	// reached a source, stop
		{
			paths_backward++;
			continue;
		}
		find_paths_backward(new_nd, nwk);
	}
	slevel--;

	return 0;
}
#endif OKAY_BUT_SLOW

#ifdef NOT_USED_ANYMORE
//
// given a node, find all sinks ahead of the node (in the flow)
// recursive, depth-first algorithm
//
int find_sinks_forward(int nid, struct PN *nwk)
{
	int k;
	int new_nd;

	slevel++;
	for (k = 0; k < nwk[nid].out_deg; k++)
	{
		new_nd = nwk[nid].out_nbrs[k];
		if (nwk[new_nd].out_deg == 0)	// reached a sink, do not go further
		{
			if (nwk[new_nd].was_here == FALSE)
			{
				nwk[new_nd].was_here = TRUE;
				sinks_forward++;
			}
			continue;
		}
		find_sinks_forward(new_nd, nwk);
	}
	slevel--;

	return 0;
}

//
// given a node, find all sources before the node (in the flow)
// recursive, depth-first algorithm
//
int find_sources_backward(int nid, struct PN *nwk)
{
	int k;
	int new_nd;

	slevel++;
	for (k = 0; k < nwk[nid].in_deg; k++)
	{
		new_nd = nwk[nid].in_nbrs[k];
		if (nwk[new_nd].in_deg == 0)	// reached a source, do not trace back further
		{
			if (nwk[new_nd].was_here == FALSE)
			{
				nwk[new_nd].was_here = TRUE;
				sources_backward++;
			}
			continue;
		}
		find_sources_backward(new_nd, nwk);
	}
	slevel--;

	return 0;
}
#endif NOT_USED_ANYMORE