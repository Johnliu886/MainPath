// 
// assignee.cpp
//
// Author: John Liu
// 
// this file contains function that handles assignee network
//
//

//
// Revision History:
// 2012/06/10  Basic function works
// 2013/11/02  Modify the assignee network: 
//             1. changed the name to "Assignee cross-citation.net".  
//             2. include only assignees of the patents in the nw[] network (originally, it includes all assignees in the given dataset)
// 2016/12/29 Added partition information to the co-assignee network 
// 2018/03/16 Added a rationale check before calling link_nodes_anw(), this is added when working on TIP data with around 60,000 patents
// 2018/05/27 Enhanced the create_assignee_network() function so that it can also create "Assignee cross-citation 1st.paj"
//

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include "resource.h"
#include "network.h"

extern int nnodes;
extern struct PN *nw;
extern int nwos;
extern struct WOS *wos;
extern int nasgns;
extern struct ASSIGNEES *assignees;	// for patent data only
extern FILE *logstream;

int nanw;
struct PN *anw;

int link_nodes_anw(int, int);
int create_network_anw(int);
extern int asgnname_search(struct ASSIGNEES *, int, wchar_t *);
extern int is_university(wchar_t *);

//
// create the assignee cross-citaton network
// type=0 ==> output assignee cross-citation network that contains all assignees
// type=1 ==> output assignee cross-citation network that contains only 1st assignees
//
int create_assignee_network(wchar_t *dpath, int type)
{
	int i, k, sj, tj, sk, tk, sm, tm;
	int deg;
	FILE *ostream;
	int na;
	int ndx;	
	wchar_t oname[FNAME_SIZE];
	struct ASSIGNEES *a;

	// create a new assignee array, this array is different from the original one in that it consists of only assignees in the nw[] patent network
	a = (struct ASSIGNEES *)Jmalloc(nasgns * sizeof(struct ASSIGNEES), L"create_assignee_network: a");
	for (sj = 0; sj < nasgns; sj++) assignees[sj].flag = 0;	
	for (i = 0; i < nnodes; i++)	// for all nodes
	{
		sm = nw[i].ndx2wos;		
		if (type == 0)	// want all assignees in the network
		{
			for (sk = 0; sk < wos[sm].nde; sk++)	// for all assignees
			{
				sj = wos[sm].DE[sk];
				assignees[sj].flag++;	// mark those assignees that are in the nw[]
			}
		}
		else if (type == 1)	// want only 1st assignees in the network
		{
				sj = wos[sm].DE[0];
				// NOTE: for type==1 type==0 has modified wos[].DE[] so that
				//       the some of the indices here are -1 (which means that they are the assignees not in the network)
				if (sj >= 0)	
					assignees[sj].flag++;	// mark those assignees that are 1st assignees and in the nw[]
		}
	}
	tj = 0;
	for (sj = 0; sj < nasgns; sj++)
	{
		if (assignees[sj].flag > 0)	// copy only those that are flagged
			a[tj++] = assignees[sj];
	}
	na = tj;
	if (type == 0)
		fwprintf(logstream, L"Assignees (all) = %d, assignees (citation network only) = %d\n", nasgns, na);
	else if (type == 1)
		fwprintf(logstream, L"Assignees (citation network only) = %d, assignees (1st) = %d\n", nasgns, na);
	// have to adjust all reference to the old assignee array to new assignee array	
	for (i = 0; i < nnodes; i++)	// for all nodes
	{
		sm = nw[i].ndx2wos;		
		for (sk = 0; sk < wos[sm].nde; sk++)	// for all assignees
		{
			sj = wos[sm].DE[sk];
			//fwprintf(logstream, L"***%s\n", assignees[sj].name); fflush(logstream);
			if (sj >= 0)	// only for selected assinees
			{
				ndx = asgnname_search(a, na, assignees[sj].name);
				wos[sm].DE[sk] = ndx;
			}
		}
	}
	nasgns = na;	// replace the delete the original assignees array
	for (sj = 0; sj < na; sj++)
		assignees[sj] = a[sj];	
	Jfree(a, L"create_assignee_network: a");

	nanw = nasgns;
	create_network_anw(nasgns);

	for (i = 0; i < nnodes; i++)	// for all nodes (sources)
	{
		//fwprintf(logstream, L"i=%d %s\n", i, nw[i].name);
		sm = nw[i].ndx2wos;
		for (k = 0; k < nw[i].out_deg; k++)	// for all their outward neighbors (targets)
		{ 
			//fwprintf(logstream, L"  k=%d %s\n", k, nw[nw[i].out_nbrs[k]].name); fflush(logstream);
			tm = nw[nw[i].out_nbrs[k]].ndx2wos;
			for (sk = 0; sk < wos[sm].nde; sk++)	// for all assignees of the source node
			{
				sj = wos[sm].DE[sk];
				for (tk = 0; tk < wos[tm].nde; tk++)	// for all assignees of the target nodes
				{
					tj = wos[tm].DE[tk];
					//fwprintf(logstream, L"     %s -> %s\n", assignees[sj].name, assignees[tj].name);
					if (sj >= 0 && tj >= 0)	// added 2018/03/16
						link_nodes_anw(sj, tj);
				}
			}
		}
	}

	if (type == 0)
		swprintf(oname, L"%s\\Assignee cross-citation.paj", dpath);
	else if (type == 1)
		swprintf(oname, L"%s\\Assignee cross-citation 1st.paj", dpath);
	if (_wfopen_s(&ostream, oname, L"wt, ccs=UTF-8") != 0) 
		return MSG_OFILE_CANNOTOPEN;
	if (type == 0)
		fwprintf(ostream, L"*Network Assignee cross-citation\n"); 
	else if (type == 1)
		fwprintf(ostream, L"*Network Assignee cross-citation 1st\n"); 
	fwprintf(ostream, L"*Vertices %d\n", nasgns); 
	for (i = 0; i < nasgns; i++)	
	{
		fwprintf(ostream, L"%d \"%s\"\n", i+1, assignees[i].name);
		fflush(ostream);
	}
	fwprintf(ostream, L"*Arcs\n");
	for (i = 0; i < nasgns; i++)
	{
		for (k = 0; k < anw[i].out_deg; k++)
			fwprintf(ostream, L"%d %d %.0f\n", i+1, anw[i].out_nbrs[k]+1, anw[i].out_weight[k]);
	}
	fwprintf(ostream, L"*Partition created by MainPath program, 1: University, 2: Non-University\n");
	fwprintf(ostream, L"*Vertices %d\n", nasgns);
	for (i = 0; i < nasgns; i++)
	{
		if (is_university(assignees[i].name))
			fwprintf(ostream, L"1\n");
		else
			fwprintf(ostream, L"2\n");
	}

	fclose(ostream);

	return 0;
}

//
// link two nodes
//
int link_nodes_anw(int ndx1, int ndx2)
{
	int i, k;
	int degree, in_deg, out_deg;
	int *tmp1;
	double *tmp2, *tmp3, *tmp4;
	int flag;

	//fwprintf(logstream, L"     *%d->%d [%d %d] [%d %d]\n", ndx1, ndx2, anw[ndx1].degree, anw[ndx1].out_deg, anw[ndx2].degree, anw[ndx2].in_deg);
	// for node ndx1
	degree = anw[ndx1].degree;
	flag = 0;
	for (k = 0; k < degree; k++)
	{
		if (anw[ndx1].nbrs[k] == ndx2)	// the link is already there, increase the strength and weight
		{
			flag = 1;
			break;
		}
	}
	if (flag == 1)	// existing link
	{
		anw[ndx1].strength += 1.0;	 
		anw[ndx1].weight[k] += 1.0;		
	}
	else	// new connection
	{
		anw[ndx1].strength += 1.0;	// overall strength = total weight of their connnections 
		anw[ndx1].nbrs[degree] = ndx2;		// neighbor pointer
		anw[ndx1].weight[degree] = 1.0;		// weighting of this particular link
		anw[ndx1].degree++;					// overall degree, number of immediate neighbors 
		if (anw[ndx1].degree == anw[ndx1].cur_mem)	// used up the memory allocated
		{
			tmp1 = (int *)malloc((anw[ndx1].cur_mem + N_INCREMENT) * sizeof(int));
			tmp2 = (double *)malloc((anw[ndx1].cur_mem + N_INCREMENT) * sizeof(double));
			anw[ndx1].cur_mem += N_INCREMENT;
			for (i = 0; i < anw[ndx1].degree; i++) { tmp1[i] = anw[ndx1].nbrs[i] ; tmp2[i] = anw[ndx1].weight[i]; }
			free(anw[ndx1].nbrs); free(anw[ndx1].weight);
			anw[ndx1].nbrs = tmp1; anw[ndx1].weight = tmp2;
		}
	}

	out_deg = anw[ndx1].out_deg;
	flag = 0;
	for (k = 0; k < out_deg; k++)
	{
		if (anw[ndx1].out_nbrs[k] == ndx2)	// the link is already there, increase the strength and weight
		{
			flag = 1;
			break;
		}
	}
	if (flag == 1)	// existing link
	{
		anw[ndx1].out_strength += 1.0;	 
		anw[ndx1].out_weight[k] += 1.0;		
	}
	else	// new connection
	{
		anw[ndx1].out_strength += 1.0;	// outward strength
		anw[ndx1].out_nbrs[out_deg] = ndx2;			// neighbor pointer
		anw[ndx1].out_weight[out_deg] = 1.0;	// outward weight, weight = inward + outward 
		anw[ndx1].out_deg++;	// out-degree
		if (anw[ndx1].out_deg == anw[ndx1].out_cur_mem)	// used up the memory allocated
		{
			tmp1 = (int *)malloc((anw[ndx1].out_cur_mem + N_INCREMENT) * sizeof(int));
			tmp2 = (double *)malloc((anw[ndx1].out_cur_mem + N_INCREMENT) * sizeof(double));
			tmp3 = (double *)malloc((anw[ndx1].out_cur_mem + N_INCREMENT) * sizeof(double));
			tmp4 = (double *)malloc((anw[ndx1].out_cur_mem + N_INCREMENT) * sizeof(double));
			anw[ndx1].out_cur_mem += N_INCREMENT;
			for (i = 0; i < anw[ndx1].out_deg; i++) { tmp1[i] = anw[ndx1].out_nbrs[i] ; tmp2[i] = anw[ndx1].out_weight[i]; tmp3[i] = anw[ndx1].out_spx[i]; tmp4[i] = anw[ndx1].out_dissim[i]; }
			free(anw[ndx1].out_nbrs); free(anw[ndx1].out_weight); free(anw[ndx1].out_spx); free(anw[ndx1].out_dissim);
			anw[ndx1].out_nbrs = tmp1; anw[ndx1].out_weight = tmp2; anw[ndx1].out_spx = tmp3;  anw[ndx1].out_dissim = tmp4;
		}
	}

	// for node ndx2
	if (anw[ndx2].degree == 0)
		wcscpy_s(anw[ndx2].name, MAX_ASSIGNEE_NAME, assignees[ndx2].name);
	degree = anw[ndx2].degree;
	flag = 0;
	for (k = 0; k < degree; k++)
	{
		if (anw[ndx2].nbrs[k] == ndx1)	// the link is already there, increase the strength and weight
		{
			flag = 1;
			break;
		}
	}
	if (flag == 1)	// existing link
	{
		anw[ndx2].strength += 1.0;	 
		anw[ndx2].weight[k] += 1.0;		
	}
	else	// new connection
	{
		anw[ndx2].strength += 1.0;	// overall strength = total weight of their connnections 
		anw[ndx2].nbrs[degree] = ndx1;		// neighbor pointer
		anw[ndx2].weight[degree] = 1.0;		// weighting of this particular link
		anw[ndx2].degree++;					// overall degree, number of immediate neighbors 
		if (anw[ndx2].degree == anw[ndx2].cur_mem)	// used up the memory allocated
		{
			tmp1 = (int *)malloc((anw[ndx2].cur_mem + N_INCREMENT) * sizeof(int));
			tmp2 = (double *)malloc((anw[ndx2].cur_mem + N_INCREMENT) * sizeof(double));
			anw[ndx2].cur_mem += N_INCREMENT;
			for (i = 0; i < anw[ndx2].degree; i++) { tmp1[i] = anw[ndx2].nbrs[i] ; tmp2[i] = anw[ndx2].weight[i]; }
			free(anw[ndx2].nbrs); free(anw[ndx2].weight);
			anw[ndx2].nbrs = tmp1; anw[ndx2].weight = tmp2;
		}
	}

	in_deg = anw[ndx2].in_deg;	
	flag = 0;
	for (k = 0; k < in_deg; k++)
	{
		if (anw[ndx2].in_nbrs[k] == ndx1)	// the link is already there, increase the strength and weight
		{
			flag = 1;
			break;
		}
	}
	if (flag == 1)
	{
		anw[ndx2].in_strength += 1.0;	 
		anw[ndx2].in_weight[k] += 1.0;		
	}
	else	// new connection
	{
		anw[ndx2].in_strength += 1.0;// inward strength
		anw[ndx2].in_nbrs[in_deg] = ndx1;	// neighbor pointer
		anw[ndx2].in_weight[in_deg] = 1.0;	// inward weight 
		anw[ndx2].in_deg++;					// in-degree
		if (anw[ndx2].in_deg == anw[ndx2].in_cur_mem)	// used up the memory allocated
		{
			tmp1 = (int *)malloc((anw[ndx2].in_cur_mem + N_INCREMENT) * sizeof(int));
			tmp2 = (double *)malloc((anw[ndx2].in_cur_mem + N_INCREMENT) * sizeof(double));
			tmp3 = (double *)malloc((anw[ndx2].in_cur_mem + N_INCREMENT) * sizeof(double));
			tmp4 = (double *)malloc((anw[ndx2].in_cur_mem + N_INCREMENT) * sizeof(double));
			anw[ndx2].in_cur_mem += N_INCREMENT;
			for (i = 0; i < anw[ndx2].in_deg; i++) { tmp1[i] = anw[ndx2].in_nbrs[i] ; tmp2[i] = anw[ndx2].in_weight[i]; tmp3[i] = anw[ndx2].in_spx[i]; tmp4[i] = anw[ndx2].in_dissim[i]; }
			free(anw[ndx2].in_nbrs); free(anw[ndx2].in_weight); free(anw[ndx2].in_spx); free(anw[ndx2].in_dissim);
			anw[ndx2].in_nbrs = tmp1; anw[ndx2].in_weight = tmp2; anw[ndx2].in_spx = tmp3; anw[ndx2].in_dissim = tmp4;
		}
	}

	return 0;
}

//
// create a network
//
int create_network_anw(int nnd)
{
	int i;

	anw = (struct PN *)malloc(nnd * sizeof(struct PN));
	if (anw == NULL) return MSG_NOT_ENOUGH_MEMORY;

	for (i = 0; i < nnd; i++) // do necessary initialization
	{ 
		wcscpy_s(anw[i].name, sizeof(anw[i].name), assignees[i].name);
		wcscpy_s(anw[i].alias, sizeof(anw[i].alias), assignees[i].name);
		anw[i].ndx = i;		// added 2013/11/01
		anw[i].mainp = FALSE;
		anw[i].special = FALSE;
		anw[i].branch = FALSE;
		anw[i].degree = 0;		// overall degree, number of immediate neighbors 
		anw[i].strength = 0.0;	// overall strength = total weight of their connnections 
		anw[i].nbrs = (int *)malloc(N_BASE * sizeof(int));				// neighbor pointer
		anw[i].weight = (double *)malloc(N_BASE * sizeof(double));		// weighting of this particular link
		anw[i].cur_mem = N_BASE;// current memory allocated
		anw[i].in_deg = 0;		// in-degree
		anw[i].in_strength = 0.0;		// inward strength
		anw[i].in_nbrs = (int *)malloc(N_BASE * sizeof(int));			// neighbor pointer
		anw[i].in_weight = (double *)malloc(N_BASE * sizeof(double));	// inward weight 
		anw[i].in_spx = (double *)malloc(N_BASE * sizeof(double));		// inward SPC, SPLC or SPNP 
		anw[i].in_dissim = (double *)malloc(N_BASE * sizeof(double));	// dissimilarity with the inward nodes 
		anw[i].in_cur_mem = N_BASE;	// current memory allocated
		anw[i].out_deg = 0;		// out-degree
		anw[i].out_strength = 0.0;	// outward strength
		anw[i].out_nbrs = (int *)malloc(N_BASE * sizeof(int));			// neighbor pointer
		anw[i].out_weight = (double *)malloc(N_BASE * sizeof(double));	// outward weight, weight = inward + outward 
		anw[i].out_spx = (double *)malloc(N_BASE * sizeof(double));		// outward SPC, SPLC or SPNP 
		anw[i].out_dissim = (double *)malloc(N_BASE * sizeof(double));	// dissimilarity with the outward nodes 
		anw[i].out_cur_mem = N_BASE;// current memory allocated
		anw[i].was_here = FALSE;
	}	

	fflush(logstream);

	return 0;
}
