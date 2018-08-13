//
// SPDC.cpp
//
// Calculate SPAD (was SPDC) and SPGD for an acyclic network
//
// 2013/04/03 Coding being
// 2013/04/30 Basic function works
// 2013/05/04 Fixed a problem in calculation SPDC at odd numbers such as 0.3, 0.28. Add only if kstrength is positive.
// 2013/05/23 Renamed SPDC to SPAD and added a new traversal count SPGD.
// 2013/06/01 Fixed an over count problem in calculating SPAD
// 2013/07/17 Added codes to calculate "paths_forward" for SPAD algorithm
// NOT IMPLEMENTED ==> 2013/12/29 Changed SPAD algorithm when knowledge strength reduce to exactly zero along the chain. 
// NOT IMPLEMENTED           WAS: stop branching out
// NOT IMPLEMENTED           NOW: branching out one more step as if knowledge strength still has some effect
// NOT IMPLEMENTED           When deacy=1.0, this will make the SPAD of each link the number of their citations
// 2014/01/08 Added codes to handle SPHD, including function SPHD(), decay_paths_forward_SPHD()
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

struct TLINK
{
	int i;	// node index
	int k;	// neighbor index
	double kstrength;	// knowledge strength, for SPAD and SPHD calculaton
};

#define MAX_PCHAIN 200
static struct TLINK pchain[MAX_PCHAIN];
static int slevel;				// used in depth-first search
static double kstrength;		// knowledge strength

int decay_paths_forward_SPGD(int, struct PN *, double);
int decay_paths_forward_SPHD(int, struct PN *);

extern FILE *logstream;


//
// Search Path Harmonic Decay 
// assuming that knowledge decay in ratios according to the hormonic sequence (1, 1/2, 1/3, 1/4, ...)
// NOTE: the results of the topological sort is not used in this algorithm such that this function is extremely for large networks
//
int SPHD(int nnodes, struct PN *nw)
{
	int i, j, k, m;

	FILE *plogstream;
	_wfopen_s(&plogstream, L"Process log.txt", L"w");
	fclose(plogstream);

	for (i = 0; i < nnodes; i++)	// initialization
	{
		for (k = 0; k < nw[i].out_deg; k++)
			nw[i].out_spx[k] = 0.0;
		for (k = 0; k < nw[i].in_deg; k++)
			nw[i].in_spx[k] = 0.0;
		// reset paths_forward, even though it is already calculated in the function get_forward(), added 2013/07/16
		if (nw[i].out_deg == 0)	
			nw[i].paths_forward = 1;	// set to 1 for a sink
		else
			nw[i].paths_forward = 0;	
	}

	for (i = 0; i < nnodes; i++)
	{
		_wfopen_s(&plogstream, L"Process log.txt", L"a+");
		fwprintf(plogstream, L"Working on node %d\n", i); 
		fclose(plogstream);
		slevel = 0; kstrength = 1.0;
		//fwprintf(logstream, L"[%s]\n", nw[i].alias);
		decay_paths_forward_SPHD(i, nw); 
	}

	// set the in_spx[], because decay_paths_forward_SPHD() only calculate out_spx[]
	for (i = 0; i < nnodes; i++)
	{
		for (k = 0; k < nw[i].out_deg; k++)
		{
			j = nw[i].out_nbrs[k];
			for (m = 0; m < nw[j].in_deg; m++)
			{
				if (nw[j].in_nbrs[m] == i)
					nw[j].in_spx[m] = nw[i].out_spx[k];
			}
		}
	}

	return 0;
}

//
// given a node, find all forwarding paths, the search end when a sink is hit 
// recursive, depth-first algorithm
// this function uses the exhaustive search approach (slow but works!)
//
int decay_paths_forward_SPHD(int nid, struct PN *nwk)
{
	int k, p;
	int new_nd;

	// do nothing if the given node is a sink
	if (nwk[nid].out_deg == 0)	return 0;

	kstrength = 1.0 / (slevel + 1);	// when slevel=0, kstrength=1
	slevel++;
	for (k = 0; k < nwk[nid].out_deg; k++)
	{
		new_nd = nwk[nid].out_nbrs[k];	
		pchain[slevel].i = nid;
		pchain[slevel].k = k;	
		pchain[slevel].kstrength = kstrength;
		//fwprintf(logstream, L"     %d[%d->%d]%f\n", slevel, nid, k, kstrength);
		if (nwk[new_nd].out_deg == 0)	// reached a sink
		{
			for (p = 1; p < slevel; p++)	// the chain begins at slevel=1
			{
				if (p == 1)	// want to increase paths_forward only when the node is the begining node of the begining link of the chain
					nwk[pchain[p].i].paths_forward++;	// this is only used to display the "effective outflow paths"
				nwk[pchain[p].i].out_spx[pchain[p].k] += pchain[p].kstrength;
				//fwprintf(logstream, L"$$$ %s=>%s p=%d, kstrength=%.2f\n", nwk[pchain[p].i].alias, nwk[nwk[pchain[p].i].out_nbrs[pchain[p].k]].alias, p, pchain[p].kstrength);
			}
			if (kstrength > 0)	// end of the chain, add only if kstrength is positive
			{
				if (slevel == 1)		// increase paths_forward only only if there were no chains
					nwk[nid].paths_forward++;			// this is only used to display the "effective outflow paths"
				nwk[nid].out_spx[k] += kstrength;
				//fwprintf(logstream, L"###%s=>%s slevel=%d, kstrength=%.2f\n", nwk[nid].alias, nwk[new_nd].alias, slevel, kstrength);
			}
		}
		else
		{
			decay_paths_forward_SPHD(new_nd, nwk);
			kstrength = 1.0 / slevel;	// slevel will note be 0 here
		}
	}
	slevel--;

	return 0;
}

//
// Search Path Arithmetic Decay (was Search Path Decay Count)
// assuming that knowledge decay by a certain amount after acrossing a document (reduce by a certain amount)
// NOTE: the results of the topological sort is not used in this algorithm such that this function is extremely for large networks
//
int SPAD(int nnodes, struct PN *nw, double decay)
{
	int i, j, k, m;

	FILE *plogstream;
	_wfopen_s(&plogstream, L"Process log.txt", L"w");
	fclose(plogstream);

	for (i = 0; i < nnodes; i++)	// initialization
	{
		for (k = 0; k < nw[i].out_deg; k++)
			nw[i].out_spx[k] = 0.0;
		for (k = 0; k < nw[i].in_deg; k++)
			nw[i].in_spx[k] = 0.0;
		// reset paths_forward, even though it is already calculated in the function get_forward(), added 2013/07/16
		if (nw[i].out_deg == 0)	
			nw[i].paths_forward = 1;	// set to 1 for a sink
		else
			nw[i].paths_forward = 0;	
	}

	for (i = 0; i < nnodes; i++)
	{
		_wfopen_s(&plogstream, L"Process log.txt", L"a+");
		fwprintf(plogstream, L"Working on node %d\n", i); 
		fclose(plogstream);
		slevel = 0; kstrength = 1.0;
		//fwprintf(logstream, L"[%s]\n", nw[i].alias);
		decay_paths_forward_SPGD(i, nw, decay); 
	}

	// set the in_spx[], because decay_paths_forward_SPGD() only calculate out_spx[]
	for (i = 0; i < nnodes; i++)
	{
		for (k = 0; k < nw[i].out_deg; k++)
		{
			j = nw[i].out_nbrs[k];
			for (m = 0; m < nw[j].in_deg; m++)
			{
				if (nw[j].in_nbrs[m] == i)
					nw[j].in_spx[m] = nw[i].out_spx[k];
			}
		}
	}

	return 0;
}

//
// given a node, find all forwarding paths, the search end when disemination decay to zero 
// recursive, depth-first algorithm
// this function uses the exhaustive search approach (slow but works!)
//
int decay_paths_forward_SPGD(int nid, struct PN *nwk, double decay)
{
	int k, p;
	int new_nd;

	// do nothing if the given node is a sink
	if (nwk[nid].out_deg == 0)	return 0;

	if (slevel != 0)	// so that the strength of the 1st diffusion is always 1 (the value when it first enters the function)
		kstrength -= decay;
	slevel++;
	for (k = 0; k < nwk[nid].out_deg; k++)
	{
		new_nd = nwk[nid].out_nbrs[k];	
		pchain[slevel].i = nid;
		pchain[slevel].k = k;	
		pchain[slevel].kstrength = kstrength;
		//if (nwk[new_nd].out_deg == 0 || kstrength <= 0)	// this was the code before 2013/06/01, over count the SPAD value
		if (nwk[new_nd].out_deg == 0 || (kstrength - decay) <= 0)	// reached a sink, or forsee that kstrength<0 next in the chain, do not go further, modified 2013/06/01
		//if (nwk[new_nd].out_deg == 0 || (kstrength - decay) < 0)	// changed the 2nd check from "<=" to "<" 2013/12/29, see Revision History for explanation ==> NOT IMPLEMENTED!!
		{
			for (p = 1; p < slevel; p++)	// the chain begins at slevel=1
			{
				if (p == 1)	// want to increase paths_forward only when the node is the begining node of the begining link of the chain, added 2013/07/17 
					nwk[pchain[p].i].paths_forward++;	// this is only used to display the "effective outflow paths", added 2013/07/
				nwk[pchain[p].i].out_spx[pchain[p].k] += pchain[p].kstrength;
				//fwprintf(logstream, L"$$$ %s=>%s p=%d, kstrength=%.2f\n", nwk[pchain[p].i].alias, nwk[nwk[pchain[p].i].out_nbrs[pchain[p].k]].alias, p, pchain[p].kstrength);
			}
			if (kstrength > 0)	// end of the chain, add only if kstrength is positive, add this check 2013/05/04
			{
				if (slevel == 1)		// increase paths_forward only only if there were no chains, added 2013/07/17
					nwk[nid].paths_forward++;			// this is only used to display the "effective outflow paths", added 2013/07/17
				nwk[nid].out_spx[k] += kstrength;
				//fwprintf(logstream, L"###%s=>%s slevel=%d, kstrength=%.2f\n", nwk[nid].alias, nwk[new_nd].alias, slevel, kstrength);
			}
			continue;
		}
		decay_paths_forward_SPGD(new_nd, nwk, decay);
	}
	slevel--;
	kstrength += decay;

	return 0;
}

#ifdef USING_TOPOLOGICAL_SORT
//
// Search Path Geometric Decay
// assuming that knowledge decay by a certain ratio after acrossing a document
//
int SPGD(int nnodes, struct PN *nw)
{
	int i, j, k, m, p;
	double spx;	// !!!!!NOTE: this may overflow for a large network

	for (i = 0; i < nnodes; i++)
	{
		for (k = 0; k < nw[i].out_deg; k++)
		{
			j = nw[i].out_nbrs[k];
			spx = nw[j].paths_forward * nw[i].decay_backward;  // !!!!!NOTE: this may overflow for a large network, need 80bit floating to solve the problem
			nw[i].out_spx[k] = spx;
			nw[i].out_spx[k] = (double)spx;
			for (m = 0; m < nw[j].in_deg; m++)
			{
				if (nw[j].in_nbrs[m] == i)
					nw[j].in_spx[m] = spx;
			}
		}
	}

	return 0;
}

#endif USING_TOPOLOGICAL_SORT