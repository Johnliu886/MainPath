//
// subnetwork.cpp
//

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "resource.h"
#include "network.h"

extern FILE *logstream;

int find_distances(int, struct PN *, int);
int find_outward_distances(int, struct PN *);
int find_inward_distances(int, struct PN *);

//
// give center node index, find the subnetwork 'level' handshakes from the center
// write subnetwork to a file in Pajek format
//
int output_subnet(struct PN *ipn, int nn, int center, int level, int direction, wchar_t *dpath)
{
	int i, j, k, depth;
	int nnn;			// number of nodes, new network 
	int nnbrs, nout_nbrs, nin_nbrs;
	int dcount, out_dcount, in_dcount;
	wchar_t line[LBUF_SIZE];
	wchar_t oname[FNAME_SIZE];
	FILE *ostream;
	struct PN *npn;		// pointer to the new network 
	struct PN *pn;
	int deg;
	double weight;

	// duplicate the projection network
	pn = (struct PN *)malloc(nn * sizeof(struct PN));
	if (pn == NULL) return MSG_NOT_ENOUGH_MEMORY;
	for (i = 0; i < nn; i++) pn[i] = ipn[i];	/* duplicate the network */
	// allocate memory to each nbrs[], weight[], etc.
	for (i = 0; i < nn; i++)
	{	
		pn[i].nbrs = (int *)malloc(pn[i].degree*sizeof(int));	
		if (pn[i].nbrs == NULL) return MSG_NOT_ENOUGH_MEMORY;
		pn[i].in_nbrs = (int *)malloc(pn[i].in_deg*sizeof(int));	
		if (pn[i].in_nbrs == NULL) return MSG_NOT_ENOUGH_MEMORY;
		pn[i].out_nbrs = (int *)malloc(pn[i].out_deg*sizeof(int));	
		if (pn[i].out_nbrs == NULL) return MSG_NOT_ENOUGH_MEMORY;
		pn[i].weight = (double *)malloc(pn[i].degree*sizeof(double));	
		if (pn[i].weight == NULL) return MSG_NOT_ENOUGH_MEMORY;
		pn[i].in_weight = (double *)malloc(pn[i].in_deg*sizeof(double));	
		if (pn[i].in_weight == NULL) return MSG_NOT_ENOUGH_MEMORY;
		pn[i].out_weight = (double *)malloc(pn[i].out_deg*sizeof(double));	
		if (pn[i].out_weight == NULL) return MSG_NOT_ENOUGH_MEMORY;
		pn[i].in_spx = (double *)malloc(pn[i].in_deg*sizeof(double));	
		if (pn[i].in_spx == NULL) return MSG_NOT_ENOUGH_MEMORY;
		pn[i].out_spx = (double *)malloc(pn[i].out_deg*sizeof(double));	
		if (pn[i].out_spx == NULL) return MSG_NOT_ENOUGH_MEMORY;
		pn[i].in_dissim = (double *)malloc(pn[i].in_deg*sizeof(double));	
		if (pn[i].in_dissim == NULL) return MSG_NOT_ENOUGH_MEMORY;
		pn[i].out_dissim = (double *)malloc(pn[i].out_deg*sizeof(double));	
		if (pn[i].out_dissim == NULL) return MSG_NOT_ENOUGH_MEMORY;
	}
	// assign values to nbrs[], weight[], etc.
	for (i = 0; i < nn; i++)	// copy data over
	{
		for (k = 0; k < pn[i].degree; k++)
		{
			pn[i].nbrs[k] = ipn[i].nbrs[k];
			pn[i].weight[k] = ipn[i].weight[k];
		}
		for (k = 0; k < pn[i].in_deg; k++)
		{
			pn[i].in_nbrs[k] = ipn[i].in_nbrs[k];
			pn[i].in_weight[k] = ipn[i].in_weight[k];
			pn[i].in_spx[k] = ipn[i].in_spx[k];
			pn[i].in_dissim[k] = ipn[i].in_dissim[k];
		}
		for (k = 0; k < pn[i].out_deg; k++)
		{
			pn[i].out_nbrs[k] = ipn[i].out_nbrs[k];
			pn[i].out_weight[k] = ipn[i].out_weight[k];
			pn[i].out_spx[k] = ipn[i].out_spx[k];
			pn[i].out_dissim[k] = ipn[i].out_dissim[k];
		}
	}

	j = pn[center].comp - 1;	// index of the component this node belongs to
	for (i = 0; i < nn; i++) pn[i].level = -1;	// initialize for each node
	depth = find_outward_distances(center, pn);		// find and mark distances outward from this node	
	if (depth == -1)	return -1;
	depth = find_inward_distances(center, pn);		// find and mark distances inward to this node
	if (depth == -1)	return -1;

	nnn = 0;
	for (i = 0; i < nn; i++)	// find the size of the new network
		if (pn[i].comp == j + 1 && pn[i].level != -1 && pn[i].level <= level)	// check only within current component
			nnn++;

	npn = (struct PN *)malloc(nnn * sizeof(struct PN));
	if (npn == NULL) return MSG_NOT_ENOUGH_MEMORY;
	for (i = 0, k = 0; i < nn; i++)	// make the new network, 1st pass 
	{
		if (pn[i].comp == j + 1 && pn[i].level != -1 && (pn[i].level <= level))	// check only within current component
		{
			npn[k] = pn[i];
			pn[i].indx = k;	npn[k].indx = i;	// establish cross reference 
			k++;
		}
		else
			pn[i].indx = -1;	// excluded from the new network 
	}

	for (i = 0; i < nnn; i++)	// make the new network, 2nd pass, adjust reference to neighbors
	{
		dcount = out_dcount = in_dcount = 0;
		if (direction == DIRECT_BOTH) 
		{
			for (k = 0; k < npn[i].degree; k++)
			{
				nnbrs = pn[npn[i].nbrs[k]].indx; // find new indies in the new network
				if (nnbrs != -1)		// part of the new network 
				{
					npn[i].nbrs[dcount] = nnbrs;
					npn[i].weight[dcount] = npn[i].weight[k];
					dcount++;
				}
			}
			npn[i].degree = dcount;
		}
		if (direction == DIRECT_BOTH || direction == DIRECT_INWARD)
		{		
			for (k = 0; k < npn[i].in_deg; k++)
			{
				nin_nbrs = pn[npn[i].in_nbrs[k]].indx; // find new indies in the new network
				if (nin_nbrs != -1)		// part of the new network 
				{
					npn[i].in_nbrs[in_dcount] = nin_nbrs;
					npn[i].in_weight[in_dcount] = npn[i].in_weight[k];
					npn[i].in_spx[in_dcount] = npn[i].in_spx[k];
					npn[i].in_dissim[in_dcount] = npn[i].in_dissim[k];
					in_dcount++;
				}
			}
			npn[i].in_deg = in_dcount;
		}
		if (direction == DIRECT_BOTH || direction == DIRECT_OUTWARD)
		{		
			for (k = 0; k < npn[i].out_deg; k++)
			{				
				nout_nbrs = pn[npn[i].out_nbrs[k]].indx; // find new indies in the new network
				if (nout_nbrs != -1)		// part of the new network 
				{
					npn[i].out_nbrs[out_dcount] = nout_nbrs;
					npn[i].out_weight[out_dcount] = npn[i].out_weight[k];
					npn[i].out_spx[out_dcount] = npn[i].out_spx[k];
					npn[i].out_dissim[out_dcount] = npn[i].out_dissim[k];
					out_dcount++;
				}
			}
			npn[i].out_deg = out_dcount;
		}
	}

	// create the net file 
	swprintf_s(oname, FNAME_SIZE, L"%s%s L%d.net", dpath, pn[center].alias, level);
	if ((_wfopen_s(&ostream, oname, L"w")) != 0) { fwprintf(logstream, L"Can not open file \"%s\".\n", oname); return -1; }
	else fwprintf(logstream, L"File \"%s\" was opened successfully.\n", oname);

	// write the "*vertice" line
	swprintf_s(line, LBUF_SIZE, L"*vertices %d\n", nnn);
	if (fputws(line, ostream) < 0) {	fwprintf(logstream, L"Can not write to file \"%s\".\n", oname); return -1; }
	// write vertice list 
	// Pajek requires that first vertex is indexed in "1" rather than "0" 
	for (i = 0; i < nnn; i++)
	{
		swprintf_s(line, LBUF_SIZE, L"%d \"%s\"\n", i+1, npn[i].alias);
		fputws(line, ostream);
	}

	// write the "*edges" line 
	swprintf_s(line, LBUF_SIZE, L"*arcs\n");
	if (fputws(line, ostream) < 0) { fwprintf(logstream, L"Can not write to file \"%s\".\n", oname); return -1; }

	// write edge list to the output file 
	for (i = 0; i < nnn; i++)
	{
		if (direction == DIRECT_BOTH || direction == DIRECT_OUTWARD)
		{
			for (k = 0; k < npn[i].out_deg; k++)
			{
				if (npn[i].out_spx[k] != 0)
					swprintf(line, LBUF_SIZE, L"%d %d %f\n", i+1, npn[i].out_nbrs[k]+1, npn[i].out_spx[k]);
				else continue;
				if (fputws(line, ostream) < 0) { fwprintf(logstream, L"Can not write to file \"%s\".\n", oname); return -1; }				
			}
		}
		if (direction == DIRECT_BOTH || direction == DIRECT_INWARD)
		{
			for (k = 0; k < npn[i].in_deg; k++)
			{
				if (npn[i].in_spx[k] != 0)
					swprintf(line, LBUF_SIZE, L"%d %d %f\n", npn[i].in_nbrs[k]+1, i+1, npn[i].in_spx[k]);
				else continue;
				if (fputws(line, ostream) < 0) { fwprintf(logstream, L"Can not write to file \"%s\".\n", oname); return -1; }				
			}
		}
	}

	// close the net file 
	if(fclose(ostream)) { fwprintf(logstream, L"Can not close file \"%s\".\n", oname); return -1; }
	else fwprintf(logstream, L"File \"%s\" was closed successfully.\n", oname);

	free(npn); 
	free(pn);

	return 0;
}

//
// find and mark distances of a given vertex to all other vertices, using the breadth-first algorithm
// given the starting vertex and the network array
// return longest distance (level), or -1 if queue overflow
// NOTE: assume that the network is non-directional
//
#define MAX_QUEUE 10000
int q[MAX_QUEUE];
int find_distances(int st, struct PN p[], int direction)
{
	int iput, iget;		/* index to the queue */
	int i, j, icur;
	int ndata;			/* number of data in queue */
	int level;			/* this indicates the distance from the starting vertex */
	int deg;
	double weight;

	iput = iget = 0;
	ndata = 0;
	p[st].level = 0; 
	/* put starting vertex into the queue */
	q[iput++] = st;	
	ndata++;
	while (iput != iget)	/* until the queue is empty */
	{
		/* get from the queue, first-in-first out */
		icur = q[iget++];	
		ndata--;
		if (ndata < 0) { fwprintf(logstream, L"ERROR: queue process error (breadth-first algorithm)\n"); return -1; }
		if (iget == MAX_QUEUE)	iget = 0;
		level = p[icur].level + 1;
		if (direction == DIRECT_BOTH) deg = p[icur].degree;
		else if (direction == DIRECT_OUTWARD) deg = p[icur].out_deg;
		else deg = p[icur].in_deg;
		for (i = 0; i < deg; i++)
		{
			if (direction == DIRECT_BOTH) j = p[icur].nbrs[i];
			else if (direction == DIRECT_OUTWARD) j = p[icur].out_nbrs[i];	/* index of the neighbors */
			else j = p[icur].in_nbrs[i];
			if (p[j].level == -1)	/* not yet traversed */
			{			
				if (direction == DIRECT_BOTH) weight = p[icur].weight[i];
				else if (direction == DIRECT_OUTWARD) weight = p[icur].out_weight[i];	/* index of the neighbors */
				else weight = p[icur].in_weight[i];
				if (weight == 0)	/* abnormal case, assume no connection if the weight is zero */
				{
					continue;	/* do nothing */
				}
				p[j].level = level;
				/* put into the queue, for later processing */
				q[iput++] = j;
				ndata++;
				if (ndata > MAX_QUEUE) { fwprintf(logstream, L"ERROR: queue overflow (breadth-first algorithm)\n"); return -1; }
				if (iput == MAX_QUEUE)	iput = 0;
			}
		}
	}
	return level;
}

//
// this function finds outward distances from a given node
//
int find_outward_distances(int st, struct PN p[])
{
	int iput, iget;		/* index to the queue */
	int i, j, icur;
	int ndata;			/* number of data in queue */
	int level;			/* this indicates the distance from the starting vertex */

	iput = iget = 0;
	ndata = 0;
	p[st].level = 0; 
	/* put starting vertex into the queue */
	q[iput++] = st;	
	ndata++;
	while (iput != iget)	/* until the queue is empty */
	{
		/* get from the queue, first-in-first out */
		icur = q[iget++];	
		ndata--;
		if (ndata < 0) { fwprintf(logstream, L"ERROR: queue process error (breadth-first algorithm)\n"); return -1; }
		if (iget == MAX_QUEUE)	iget = 0;
		level = p[icur].level + 1;
		for (i = 0; i < p[icur].out_deg; i++)
		{
			j = p[icur].out_nbrs[i];	/* index of the neighbors */
			if (p[j].level == -1)	/* not yet traversed */
			{
				if (p[icur].out_weight[i] == 0)	/* abnormal case, assume no connection if the weight is zero */
				{
					continue;	/* do nothing */
				}
				p[j].level = level;
				/* put into the queue, for later processing */
				q[iput++] = j;
				ndata++;
				if (ndata > MAX_QUEUE) { fwprintf(logstream, L"ERROR: queue overflow (breadth-first algorithm)\n"); return -1; }
				if (iput == MAX_QUEUE)	iput = 0;
			}
		}
	}
	return level;
}

//
// this function finds inward distances from a given node
//
int find_inward_distances(int st, struct PN p[])
{
	int iput, iget;		/* index to the queue */
	int i, j, icur;
	int ndata;			/* number of data in queue */
	int level;			/* this indicates the distance from the starting vertex */

	iput = iget = 0;
	ndata = 0;
	p[st].level = 0; 
	/* put starting vertex into the queue */
	q[iput++] = st;	
	ndata++;
	while (iput != iget)	/* until the queue is empty */
	{
		/* get from the queue, first-in-first out */
		icur = q[iget++];	
		ndata--;
		if (ndata < 0) { fwprintf(logstream, L"ERROR: queue process error (breadth-first algorithm)\n"); return -1; }
		if (iget == MAX_QUEUE)	iget = 0;
		level = p[icur].level + 1;
		for (i = 0; i < p[icur].in_deg; i++)
		{
			j = p[icur].in_nbrs[i];	/* index of the neighbors */
			if (p[j].level == -1)	/* not yet traversed */
			{
				if (p[icur].in_weight[i] == 0)	/* abnormal case, assume no connection if the weight is zero */
				{
					continue;	/* do nothing */
				}
				p[j].level = level;
				/* put into the queue, for later processing */
				q[iput++] = j;
				ndata++;
				if (ndata > MAX_QUEUE) { fwprintf(logstream, L"ERROR: queue overflow (breadth-first algorithm)\n"); return -1; }
				if (iput == MAX_QUEUE)	iput = 0;
			}
		}
	}
	return level;
}

//
// create ".json" (JavaScript Object Notation) file for the given network
// for all nodes, find the subnetwork 'level' handshakes from the center
//
int create_json(struct PN *ipn, int nn, int level, int direction, wchar_t *dpath)
{
	int i, j, k, depth;
	int center;
	int nnn;			// number of nodes, new network 
	int nnbrs, nout_nbrs, nin_nbrs;
	int dcount, out_dcount, in_dcount;
	wchar_t line[LBUF_SIZE];
	wchar_t oname[FNAME_SIZE];
	FILE *ostream;
	struct PN *npn;		// pointer to the new network 
	struct PN *pn;
	int deg;
	double weight;

#ifdef NOT_YET_READY
	center = 25;

	// duplicate the projection network
	pn = (struct PN *)malloc(nn * sizeof(struct PN));
	if (pn == NULL) return MSG_NOT_ENOUGH_MEMORY;
	for (i = 0; i < nn; i++) pn[i] = ipn[i];	/* duplicate the network */
	// allocate memory to each nbrs[], weight[], etc.
	for (i = 0; i < nn; i++)
	{	
		pn[i].nbrs = (int *)malloc(pn[i].degree*sizeof(int));	
		if (pn[i].nbrs == NULL) return MSG_NOT_ENOUGH_MEMORY;
		pn[i].in_nbrs = (int *)malloc(pn[i].in_deg*sizeof(int));	
		if (pn[i].in_nbrs == NULL) return MSG_NOT_ENOUGH_MEMORY;
		pn[i].out_nbrs = (int *)malloc(pn[i].out_deg*sizeof(int));	
		if (pn[i].out_nbrs == NULL) return MSG_NOT_ENOUGH_MEMORY;
		pn[i].weight = (double *)malloc(pn[i].degree*sizeof(double));	
		if (pn[i].weight == NULL) return MSG_NOT_ENOUGH_MEMORY;
		pn[i].in_weight = (double *)malloc(pn[i].in_deg*sizeof(double));	
		if (pn[i].in_weight == NULL) return MSG_NOT_ENOUGH_MEMORY;
		pn[i].out_weight = (double *)malloc(pn[i].out_deg*sizeof(double));	
		if (pn[i].out_weight == NULL) return MSG_NOT_ENOUGH_MEMORY;
		pn[i].in_spx = (double *)malloc(pn[i].in_deg*sizeof(double));	
		if (pn[i].in_spx == NULL) return MSG_NOT_ENOUGH_MEMORY;
		pn[i].out_spx = (double *)malloc(pn[i].out_deg*sizeof(double));	
		if (pn[i].out_spx == NULL) return MSG_NOT_ENOUGH_MEMORY;
		pn[i].in_dissim = (double *)malloc(pn[i].in_deg*sizeof(double));	
		if (pn[i].in_dissim == NULL) return MSG_NOT_ENOUGH_MEMORY;
		pn[i].out_dissim = (double *)malloc(pn[i].out_deg*sizeof(double));	
		if (pn[i].out_dissim == NULL) return MSG_NOT_ENOUGH_MEMORY;
	}
	// assign values to nbrs[], weight[], etc.
	for (i = 0; i < nn; i++)	// copy data over
	{
		for (k = 0; k < pn[i].degree; k++)
		{
			pn[i].nbrs[k] = ipn[i].nbrs[k];
			pn[i].weight[k] = ipn[i].weight[k];
		}
		for (k = 0; k < pn[i].in_deg; k++)
		{
			pn[i].in_nbrs[k] = ipn[i].in_nbrs[k];
			pn[i].in_weight[k] = ipn[i].in_weight[k];
			pn[i].in_spx[k] = ipn[i].in_spx[k];
			pn[i].in_dissim[k] = ipn[i].in_dissim[k];
		}
		for (k = 0; k < pn[i].out_deg; k++)
		{
			pn[i].out_nbrs[k] = ipn[i].out_nbrs[k];
			pn[i].out_weight[k] = ipn[i].out_weight[k];
			pn[i].out_spx[k] = ipn[i].out_spx[k];
			pn[i].out_dissim[k] = ipn[i].out_dissim[k];
		}
	}

	j = pn[center].comp - 1;	// index of the component this node belongs to
	for (i = 0; i < nn; i++) pn[i].level = -1;	// initialize for each node
	depth = find_outward_distances(center, pn);		// find and mark distances outward from this node	
	if (depth == -1)	return -1;
	depth = find_inward_distances(center, pn);		// find and mark distances inward to this node
	if (depth == -1)	return -1;

	nnn = 0;
	for (i = 0; i < nn; i++)	// find the size of the new network
		if (pn[i].comp == j + 1 && pn[i].level != -1 && pn[i].level <= level)	// check only within current component
			nnn++;

	npn = (struct PN *)malloc(nnn * sizeof(struct PN));
	if (npn == NULL) return MSG_NOT_ENOUGH_MEMORY;
	for (i = 0, k = 0; i < nn; i++)	// make the new network, 1st pass 
	{
		if (pn[i].comp == j + 1 && pn[i].level != -1 && (pn[i].level <= level))	// check only within current component
		{
			npn[k] = pn[i];
			pn[i].indx = k;	npn[k].indx = i;	// establish cross reference 
			k++;
		}
		else
			pn[i].indx = -1;	// excluded from the new network 
	}

	for (i = 0; i < nnn; i++)	// make the new network, 2nd pass, adjust reference to neighbors
	{
		dcount = out_dcount = in_dcount = 0;
		if (direction == DIRECT_BOTH) 
		{
			for (k = 0; k < npn[i].degree; k++)
			{
				nnbrs = pn[npn[i].nbrs[k]].indx; // find new indies in the new network
				if (nnbrs != -1)		// part of the new network 
				{
					npn[i].nbrs[dcount] = nnbrs;
					npn[i].weight[dcount] = npn[i].weight[k];
					dcount++;
				}
			}
			npn[i].degree = dcount;
		}
		if (direction == DIRECT_BOTH || direction == DIRECT_INWARD)
		{		
			for (k = 0; k < npn[i].in_deg; k++)
			{
				nin_nbrs = pn[npn[i].in_nbrs[k]].indx; // find new indies in the new network
				if (nin_nbrs != -1)		// part of the new network 
				{
					npn[i].in_nbrs[in_dcount] = nin_nbrs;
					npn[i].in_weight[in_dcount] = npn[i].in_weight[k];
					npn[i].in_spx[in_dcount] = npn[i].in_spx[k];
					npn[i].in_dissim[in_dcount] = npn[i].in_dissim[k];
					in_dcount++;
				}
			}
			npn[i].in_deg = in_dcount;
		}
		if (direction == DIRECT_BOTH || direction == DIRECT_OUTWARD)
		{		
			for (k = 0; k < npn[i].out_deg; k++)
			{				
				nout_nbrs = pn[npn[i].out_nbrs[k]].indx; // find new indies in the new network
				if (nout_nbrs != -1)		// part of the new network 
				{
					npn[i].out_nbrs[out_dcount] = nout_nbrs;
					npn[i].out_weight[out_dcount] = npn[i].out_weight[k];
					npn[i].out_spx[out_dcount] = npn[i].out_spx[k];
					npn[i].out_dissim[out_dcount] = npn[i].out_dissim[k];
					out_dcount++;
				}
			}
			npn[i].out_deg = out_dcount;
		}
	}

	// create the JSON file 
	swprintf_s(oname, FNAME_SIZE, L"%s L%d.json", dpath, level);
	if ((_wfopen_s(&ostream, oname, L"wt, ccs=UTF-8")) != 0) { fwprintf(logstream, L"Can not open file \"%s\".\n", oname); return -1; }
	else fwprintf(logstream, L"File \"%s\" was opened successfully.\n", oname);

	// write edge list to the output file 
	int ik, icnt;
	swprintf(line, LBUF_SIZE, L"[");
	fputws(line, ostream);
	//swprintf(line, LBUF_SIZE, L"{\"name\":\"%s\", \"weight\": %d, \"children\":[", ipn[center].alias, 1);
	//fputws(line, ostream);
	for (i = 0; i < nnn; i++)
	{
		if (npn[i].out_deg != 0)
			swprintf(line, LBUF_SIZE, L"{\"name\":\"%s\",\"children\":[", npn[i].alias);
		else
			continue;
		fputws(line, ostream);
		if (direction != DIRECT_BOTH || direction == DIRECT_OUTWARD)
		{
			icnt = 0;
			for (k = 0; k < npn[i].out_deg; k++)
			{
				if (npn[i].out_spx[k] != 0)
				{
					ik = npn[i].out_nbrs[k];
					if (icnt == 0)
						swprintf(line, LBUF_SIZE, L"{\"name\":\"%s\", \"weight\": %d}", npn[ik].alias, npn[ik].out_spx[k]);
					else
						swprintf(line, LBUF_SIZE, L", {\"name\":\"%s\", \"weight\": %d}", npn[ik].alias, npn[ik].out_spx[k]);
					fputws(line, ostream); 	
					icnt++;
					//swprintf(line, LBUF_SIZE, L"%d %d %f\n", i+1, npn[i].out_nbrs[k]+1, npn[i].out_spx[k]);
				}
			}
		}
#ifdef XXX
		else if (direction == DIRECT_BOTH)
		{
			for (k = 0; k < npn[i].degree; k++)
			{
				if (npn[i].spx[k] != 0)
					swprintf(line, LBUF_SIZE, L"%d %d %f\n", npn[i].nbrs[k]+1, i+1, npn[i].spx[k]);
				else continue;
				if (fputws(line, ostream) < 0) { fwprintf(logstream, L"Can not write to file \"%s\".\n", oname); return -1; }				
			}
		}
#endif XXX
		swprintf(line, LBUF_SIZE, L"]},\n");
		fputws(line, ostream);		
	}
	swprintf(line, LBUF_SIZE, L"]");
	fputws(line, ostream);
	swprintf(line, LBUF_SIZE, L"]");
	fputws(line, ostream);
	// close the JSON file 
	if(fclose(ostream)) { fwprintf(logstream, L"Can not close file \"%s\".\n", oname); return -1; }
	else fwprintf(logstream, L"File \"%s\" was closed successfully.\n", oname);

#ifdef XXX
	// create the net file 
	swprintf_s(oname, FNAME_SIZE, L"%s%s L%d.net", dpath, pn[center].alias, level);
	if ((_wfopen_s(&ostream, oname, L"w")) != 0) { fwprintf(logstream, L"Can not open file \"%s\".\n", oname); return -1; }
	else fwprintf(logstream, L"File \"%s\" was opened successfully.\n", oname);

	// write the "*vertice" line
	swprintf_s(line, LBUF_SIZE, L"*vertices %d\n", nnn);
	if (fputws(line, ostream) < 0) {	fwprintf(logstream, L"Can not write to file \"%s\".\n", oname); return -1; }
	// write vertice list 
	// Pajek requires that first vertex is indexed in "1" rather than "0" 
	for (i = 0; i < nnn; i++)
	{
		swprintf_s(line, LBUF_SIZE, L"%d \"%s\"\n", i+1, npn[i].alias);
		fputws(line, ostream);
	}

	// write the "*edges" line 
	swprintf_s(line, LBUF_SIZE, L"*arcs\n");
	if (fputws(line, ostream) < 0) { fwprintf(logstream, L"Can not write to file \"%s\".\n", oname); return -1; }

	// write edge list to the output file 
	for (i = 0; i < nnn; i++)
	{
		if (direction == DIRECT_BOTH || direction == DIRECT_OUTWARD)
		{
			for (k = 0; k < npn[i].out_deg; k++)
			{
				if (npn[i].out_spx[k] != 0)
					swprintf(line, LBUF_SIZE, L"%d %d %f\n", i+1, npn[i].out_nbrs[k]+1, npn[i].out_spx[k]);
				else continue;
				if (fputws(line, ostream) < 0) { fwprintf(logstream, L"Can not write to file \"%s\".\n", oname); return -1; }				
			}
		}
		if (direction == DIRECT_BOTH || direction == DIRECT_INWARD)
		{
			for (k = 0; k < npn[i].in_deg; k++)
			{
				if (npn[i].in_spx[k] != 0)
					swprintf(line, LBUF_SIZE, L"%d %d %f\n", npn[i].in_nbrs[k]+1, i+1, npn[i].in_spx[k]);
				else continue;
				if (fputws(line, ostream) < 0) { fwprintf(logstream, L"Can not write to file \"%s\".\n", oname); return -1; }				
			}
		}
	}

	// close the net file 
	if(fclose(ostream)) { fwprintf(logstream, L"Can not close file \"%s\".\n", oname); return -1; }
	else fwprintf(logstream, L"File \"%s\" was closed successfully.\n", oname);
#endif XXX

	free(npn); 
	free(pn);
#endif NOT_YET_REAY

#define TEMP
#ifdef TEMP
	// create the file 
	swprintf_s(oname, FNAME_SIZE, L"%s.csv", dpath);
	if ((_wfopen_s(&ostream, oname, L"wt, ccs=UTF-8")) != 0) { fwprintf(logstream, L"Can not open file \"%s\".\n", oname); return -1; }
	else fwprintf(logstream, L"File \"%s\" was opened successfully.\n", oname);

	int ik;
	// write edge list to the output file 
	swprintf(line, LBUF_SIZE, L"source,target,value,,\n");
	fputws(line, ostream);
	for (i = 0; i < nn; i++)
	{
		for (k = 0; k < ipn[i].out_deg; k++)
		{
			if (ipn[i].out_spx[k] != 0)
			{
				ik = ipn[i].out_nbrs[k];
				swprintf(line, LBUF_SIZE, L"%s,%s,%.1f,,\n", ipn[i].alias, ipn[ik].alias, ipn[i].out_spx[k]);
			}
			else continue;
			if (fputws(line, ostream) < 0) { fwprintf(logstream, L"Can not write to file \"%s\".\n", oname); return -1; }				
		}
		//for (k = 0; k < ipn[i].in_deg; k++)
		//{
		//	if (ipn[i].in_spx[k] != 0)
		//	{
		//		ik = ipn[i].in_nbrs[k];
		//		swprintf(line, LBUF_SIZE, L"%s,%s,%.1f\n", ipn[ik].alias, ipn[i].alias, ipn[i].in_spx[k]);
		//	}
		//	else continue;
		//	if (fputws(line, ostream) < 0) { fwprintf(logstream, L"Can not write to file \"%s\".\n", oname); return -1; }				
		//}
	}
	// close the net file 
	if(fclose(ostream)) { fwprintf(logstream, L"Can not close file \"%s\".\n", oname); return -1; }
#endif TEMP

	return 0;
}