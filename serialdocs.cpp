//
// serialdocs.cpp
//
// this file incdlues functions to handle the case of serial documents
//

// History
//
// 2017/05/31 basic function works
// 2018/01/31 added codes to increase memory when combining the links of family (serial) documents
// 2018/02/01 modify codes to assign the serialdoc label using the document with the highest TC
// 2018/02/02 fixed a problem in assigning serialdoc label (was assigned to wrong index)
// 2018/05/08 added codes to suuport THOMSON_INNOVATION_DATA
// 2018/05/09 added codes to suuport Scopus_DATA
// 2018/06/29 largely improved and corrected codes in the function modify_PN_on_serial_docs()
//

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include "resource.h"
#include "network.h"

struct NNDX 
{
	int ndx; 
};

int reorganize_neighbours(int, struct PN *);
int compare_nndx(const void *, const void *);

extern FILE *logstream;

extern int nwos;
extern struct WOS *wos;
extern int full_record_type;
extern int nname_search(struct PN *, int, wchar_t *);

//
// modify the given network according to the given serial document array
// after modification, all inward and outward pointer of serial documents are moved to the last document in the serial document array
// total number of the network is not changed, all serial documents, except the one representative, become isolated nodes
//
int modify_PN_on_serial_docs(struct PN *nw, int nnodes, struct SERIALDOCS *serialdoc, struct ATABLE *atable)
{
	int i, j, k, mi, mk, ii, jj, kk, pp, p;
	int rep;	// representative node of the serial documents
	int sdoc;
	int within, linked;
	int degree, in_deg, out_deg;
	int id_serialdoc;
	int ns;
	int maxtc, imaxtc;
	int *tmp1;
	double *tmp2, *tmp3, *tmp4, *tmp5;

	ns = serialdoc->nd;
	if (full_record_type == LAWSNOTE_DATA)
	{
		rep = serialdoc->ndx[ns-1];	// assign representative document
		//fwprintf(logstream, L"@@@@@ %d %d[%s]\n", ns, rep, nw[rep].name); fflush(logstream);
		// change the label of the node by combing all documents' label
		for (i = (ns-2); i >= 0; i--)
		{
			wcscat(nw[rep].alias, L"&");		// the following two lines are modified 2018/02/02, need further review!!!!!
			wcscat(nw[rep].alias, serialdoc->docname[i]);
			//fwprintf(logstream, L"i = %d, rep=%d [%s]\n", i, rep, nw[rep].alias); fflush(logstream);
		}
	}
	else if (full_record_type == WOS_DATA || full_record_type == THOMSON_INNOVATION_DATA || full_record_type == SCOPUS_DATA)
	{
		// for WOS data
		id_serialdoc = _wtoi(serialdoc->sdocid);
		// assign the label for the resulting node, use the node with the highest TC (citation)
		sdoc = serialdoc->ndx[0];
		imaxtc = 0;
		maxtc = wos[nw[sdoc].ndx2wos].tc;
		for (i = 1; i < ns; i++)
		{
			sdoc = serialdoc->ndx[i];
			nw[sdoc].id_serialdoc = id_serialdoc;
			if (wos[nw[sdoc].ndx2wos].tc > maxtc)
			{
				maxtc = wos[nw[sdoc].ndx2wos].tc;
				imaxtc = i;
			}
		}
		rep = serialdoc->ndx[imaxtc];	// assign the node with the highest TC (citation) as representative document
		//swprintf(nw[rep].alias, L"FM%d_%d_%s", id_serialdoc, ns, nw[serialdoc->ndx[imaxtc]].alias);	// modified 2018/02/02
		wchar_t temp[MAX_ALIAS];	// added 2018/05/08
		swprintf(temp, L"FM%d_%d_%s", id_serialdoc, ns, nw[serialdoc->ndx[imaxtc]].alias);	// added 2018/05/08
		wcscpy(nw[rep].alias, temp);	// added 2018/05/08
		// write out the paper/patent family lable and its contents
		fwprintf(logstream, L"\nPaper/patent family %s includes:\n", temp);
		for (i = 0; i < ns; i++)
		{
			sdoc = serialdoc->ndx[i];
			if (sdoc != rep)
				fwprintf(logstream, L"[%s]\t", nw[sdoc].alias);
			else
				fwprintf(logstream, L"[%s]\t", wos[nw[sdoc].ndx2wos].alias);
		}
		fwprintf(logstream, L"\n");
	}
	wcscpy(atable[nw[rep].ndx2wos].alias, nw[rep].alias);	// change alias in the atable[]
	wcscpy(wos[nw[rep].ndx2wos].alias, nw[rep].alias);	// change alias in the wos[] as well

	// 
	degree = nw[rep].degree; in_deg = nw[rep].in_deg; out_deg = nw[rep].out_deg;
	for (i = 0; i < ns; i++)	// for each document in this family
	{
		if (serialdoc->ndx[i] == rep) continue; // no need to work on the representative node, added 2018/06/28
		sdoc = serialdoc->ndx[i];	// get document index in the nw[] array
		if (sdoc != -1)
		{
			for (k = 0; k < nw[sdoc].degree; k++)	// for each neighbour of this document
			{
				mi = nw[sdoc].nbrs[k];
				if (mi == rep) // do nothing if this neighbour is rep, the link is removed later, added 2018/06/29
					continue;
				for (j = 0; j < nw[mi].degree; j++)	// link (sdoc,k) is the same as the link (mi,j), this is moved as (rep, new), which points to mi
				{
					if (nw[mi].nbrs[j] == sdoc)
					{
						nw[mi].nbrs[j] = rep;		// change to point to the representative node, but everything else stays the same
						nw[rep].nbrs[degree] = mi;
						nw[rep].weight[degree] = nw[mi].weight[j]; 
						nw[rep].strength += nw[mi].weight[j];
						degree++;	// this may overflow, thus need to check if it is needed to get more memory
						//if (degree >= nw[rep].cur_mem)
						//	fwprintf(logstream, L"DEG$$$$$ %d %d\n", degree, nw[rep].cur_mem);	
						if (degree == nw[rep].cur_mem)	// added 2018/01/31
						{
							tmp1 = (int *)Jmalloc((nw[rep].cur_mem + N_INCREMENT) * sizeof(int), L"modify_PN_on_serial_docs: tmp1");
							tmp2 = (double *)Jmalloc((nw[rep].cur_mem + N_INCREMENT) * sizeof(double), L"modify_PN_on_serial_docs: tmp2");
							nw[rep].cur_mem += N_INCREMENT;
							for (ii = 0; ii < degree; ii++) { tmp1[ii] = nw[rep].nbrs[ii] ; tmp2[ii] = nw[rep].weight[ii]; }
							Jfree(nw[rep].nbrs, L"modify_PN_on_serial_docs: nw[rep].nbrs"); Jfree(nw[rep].weight, L"modify_PN_on_serial_docs: nw[rep].nbrs");
							nw[rep].nbrs = tmp1; nw[rep].weight = tmp2;
						}
						break; 
					}
				}
			}
			nw[rep].degree = degree;
			nw[sdoc].degree = 0;
			// do the same for in_deg
			for (k = 0; k < nw[sdoc].in_deg; k++)
			{
				mi = nw[sdoc].in_nbrs[k];;
				if (mi == rep) // do nothing if this neighbour is rep, the link is removed later, added 2018/06/29
					continue;	
				for (j = 0; j < nw[mi].out_deg; j++)
				{
					if (nw[mi].out_nbrs[j] == sdoc)
					{
						nw[mi].out_nbrs[j] = rep;		// change to point to the representative node
						nw[rep].in_nbrs[in_deg] = mi;
						nw[rep].in_weight[in_deg] = nw[mi].out_weight[j]; 
						nw[rep].in_strength += nw[mi].out_weight[j];
						in_deg++;
						//if (in_deg >= nw[rep].in_cur_mem)
						//	fwprintf(logstream, L"INDEG$$$$$ %d %d\n", in_deg, nw[rep].in_cur_mem);
						if (in_deg == nw[rep].in_cur_mem)	// added 2018/01/31
						{
							tmp1 = (int *)Jmalloc((nw[rep].in_cur_mem + N_INCREMENT) * sizeof(int), L"modify_PN_on_serial_docs: tmp1");
							tmp2 = (double *)Jmalloc((nw[rep].in_cur_mem + N_INCREMENT) * sizeof(double), L"modify_PN_on_serial_docs: tmp2");
							tmp3 = (double *)Jmalloc((nw[rep].in_cur_mem + N_INCREMENT) * sizeof(double), L"modify_PN_on_serial_docs: tmp3");
							tmp4 = (double *)Jmalloc((nw[rep].in_cur_mem + N_INCREMENT) * sizeof(double), L"modify_PN_on_serial_docs: tmp4");
							tmp5 = (double *)Jmalloc((nw[rep].in_cur_mem + N_INCREMENT) * sizeof(double), L"modify_PN_on_serial_docs: tmp5");
							nw[rep].in_cur_mem += N_INCREMENT;
							for (ii = 0; ii < in_deg; ii++) { tmp1[ii] = nw[rep].in_nbrs[ii] ; tmp2[ii] = nw[rep].in_weight[ii]; tmp3[ii] = nw[rep].in_spx[ii]; tmp4[ii] = nw[rep].in_dissim[ii]; tmp5[ii] = nw[rep].in_relevancy[ii];}
							Jfree(nw[rep].in_nbrs, L"modify_PN_on_serial_docs: nw[rep].in_nbrs"); Jfree(nw[rep].in_weight, L"modify_PN_on_serial_docs: nw[rep].in_weight"); 
							Jfree(nw[rep].in_spx, L"modify_PN_on_serial_docs: nw[rep].in_spx"); Jfree(nw[rep].in_dissim, L"modify_PN_on_serial_docs: nw[rep].in_dissim");
							Jfree(nw[rep].in_relevancy, L"modify_PN_on_serial_docs: nw[rep].in_relevancy");
							nw[rep].in_nbrs = tmp1; nw[rep].in_weight = tmp2; nw[rep].in_spx = tmp3; nw[rep].in_dissim = tmp4; nw[rep].in_relevancy = tmp5;
						}
						break; 
					}
				}
			}
			nw[rep].in_deg = in_deg;
			nw[sdoc].in_deg = 0;
			// do the same for out_deg
			for (k = 0; k < nw[sdoc].out_deg; k++)
			{
				mi = nw[sdoc].out_nbrs[k];;
				if (mi == rep) // do nothing if this neighbour is rep, the link is removed later, added 2018/06/29
					continue;	
				for (j = 0; j < nw[mi].in_deg; j++)
				{
					if (nw[mi].in_nbrs[j] == sdoc)
					{
						nw[mi].in_nbrs[j] = rep;		// change to point to the representative node
						nw[rep].out_nbrs[out_deg] = mi;
						nw[rep].out_weight[out_deg] = nw[mi].in_weight[j]; 
						nw[rep].out_strength += nw[mi].in_weight[j];
						out_deg++;
						//if (out_deg >= nw[rep].out_cur_mem)
						//	fwprintf(logstream, L"OUTDEG$$$$$ %d %d\n", out_deg, nw[rep].out_cur_mem);	
						if (out_deg == nw[rep].out_cur_mem)	// added 2018/01/31
						{
							tmp1 = (int *)Jmalloc((nw[rep].out_cur_mem + N_INCREMENT) * sizeof(int), L" modify_PN_on_serial_docs: tmp1");
							tmp2 = (double *)Jmalloc((nw[rep].out_cur_mem + N_INCREMENT) * sizeof(double), L"modify_PN_on_serial_docs: tmp2");
							tmp3 = (double *)Jmalloc((nw[rep].out_cur_mem + N_INCREMENT) * sizeof(double), L"modify_PN_on_serial_docs: tmp3");
							tmp4 = (double *)Jmalloc((nw[rep].out_cur_mem + N_INCREMENT) * sizeof(double), L"modify_PN_on_serial_docs: tmp4");
							tmp5 = (double *)Jmalloc((nw[rep].out_cur_mem + N_INCREMENT) * sizeof(double), L"modify_PN_on_serial_docs: tmp5");
							nw[rep].out_cur_mem += N_INCREMENT;
							for (ii = 0; ii < out_deg; ii++) { tmp1[ii] = nw[rep].out_nbrs[ii] ; tmp2[ii] = nw[rep].out_weight[ii]; tmp3[ii] = nw[rep].out_spx[ii]; tmp4[ii] = nw[rep].out_dissim[ii];  tmp5[ii] = nw[rep].out_relevancy[ii];}
							Jfree(nw[rep].out_nbrs, L"modify_PN_on_serial_docs: nw[rep].out_nbrs"); Jfree(nw[rep].out_weight, L" modify_PN_on_serial_docs: "); 
							Jfree(nw[rep].out_spx, L"modify_PN_on_serial_docs: nw[rep].out_spx"); Jfree(nw[rep].out_dissim, L" modify_PN_on_serial_docs: nw[rep].out_dissim");
							Jfree(nw[rep].out_relevancy, L" modify_PN_on_serial_docs: nw[rep].out_relevancy");
							nw[rep].out_nbrs = tmp1; nw[rep].out_weight = tmp2; nw[rep].out_spx = tmp3;  nw[rep].out_dissim = tmp4; nw[rep].out_relevancy = tmp5;
						}
						break; 
					}
				}
			}
			nw[rep].out_deg = out_deg;
			nw[sdoc].out_deg = 0;
		}
	}

	// need to remove the duplicated links around representative node, added 2018/06/29
	reorganize_neighbours(rep, nw);
	for (k = 0; k < nw[rep].degree; k++)
		reorganize_neighbours(nw[rep].nbrs[k], nw);
	
	// then, a special treatment for the representative node
	// remove all the family nodes to and from the representative node
	int ndeg, nin_deg, nout_deg;
	ndeg = 0;
	for (k = 0; k < nw[rep].degree; k++)
	{
		mi = nw[rep].nbrs[k];
		within = 0;
		for (j = 0; j < ns; j++)
		{
			if (mi == serialdoc->ndx[j])	
			{
				within = 1;
				break;
			}
		}
		if (!within)
			nw[rep].nbrs[ndeg++] = nw[rep].nbrs[k];
	}
	nw[rep].degree = ndeg;
	// do the same for in_deg
	nin_deg = 0;
	for (k = 0; k < nw[rep].in_deg; k++)
	{
		mi = nw[rep].in_nbrs[k];
		within = 0;
		for (j = 0; j < ns; j++)
		{
			if (mi == serialdoc->ndx[j])	
			{
				within = 1;
				break;
			}
		}
		if (!within)
			nw[rep].in_nbrs[nin_deg++] = nw[rep].in_nbrs[k];
	}
	nw[rep].in_deg = nin_deg;	
	// do the same for out_deg
	nout_deg = 0;
	for (k = 0; k < nw[rep].out_deg; k++)
	{
		mi = nw[rep].out_nbrs[k];
		within = 0;
		for (j = 0; j < ns; j++)
		{
			if (mi == serialdoc->ndx[j])	
			{
				within = 1;
				break;
			}
		}
		if (!within)
			nw[rep].out_nbrs[nout_deg++] = nw[rep].out_nbrs[k];
	}
	nw[rep].out_deg = nout_deg;

	return 0;
}

//
// remove the duplicated links in a node
// added 2018/06/29
//
int reorganize_neighbours(int rep, struct PN *nw)
{
	int k;
	int ndeg, nin_deg, nout_deg, nndarray;
	int prev;
	struct NNDX *ndarray;

	if (nw[rep].degree > 0)
	{
		nndarray = nw[rep].degree;
		ndarray = (struct NNDX *)Jmalloc(nndarray * sizeof(struct NNDX), L"reorganize_neighbours: narray");
		if (ndarray == NULL) return MSG_NOT_ENOUGH_MEMORY;
		for (k = 0; k < nw[rep].degree; k++)
			ndarray[k].ndx = nw[rep].nbrs[k];	
		qsort((void *)ndarray, (size_t)nndarray, sizeof(struct NNDX), compare_nndx);
		prev = nw[rep].nbrs[0] = ndarray[0].ndx;
		ndeg = 1;
		for (k = 1; k < nndarray; k++)
		{
			if (ndarray[k].ndx != prev)
			{
				nw[rep].nbrs[ndeg] = ndarray[k].ndx;
				nw[rep].weight[ndeg] += nw[rep].weight[k]; 							
				nw[rep].strength += nw[rep].weight[k];
				ndeg++;
				prev = ndarray[k].ndx;
			}			
		}
		nw[rep].degree = ndeg;
		Jfree(ndarray, L"reorganize_neighbours: narray");
	}	
	if (nw[rep].in_deg > 0)
	{
		nndarray = nw[rep].in_deg;
		ndarray = (struct NNDX *)Jmalloc(nndarray * sizeof(struct NNDX), L"reorganize_neighbours: narray");
		if (ndarray == NULL) return MSG_NOT_ENOUGH_MEMORY;
		for (k = 0; k < nw[rep].in_deg; k++)
			ndarray[k].ndx = nw[rep].in_nbrs[k];
		qsort((void *)ndarray, (size_t)nndarray, sizeof(struct NNDX), compare_nndx);
		prev = nw[rep].in_nbrs[0] = ndarray[0].ndx;
		nin_deg = 1;
		for (k = 1; k < nndarray; k++)
		{
			if (ndarray[k].ndx != prev)
			{
				nw[rep].in_nbrs[nin_deg] = ndarray[k].ndx;
				nw[rep].in_weight[nin_deg] += nw[rep].in_weight[k]; 							
				nw[rep].in_strength += nw[rep].in_weight[k];
				nin_deg++;
				prev = ndarray[k].ndx;
			}
		}
		nw[rep].in_deg = nin_deg;
		Jfree(ndarray, L"reorganize_neighbours: narray");
	}	
	if (nw[rep].out_deg > 0)
	{
		nndarray = nw[rep].out_deg;
		ndarray = (struct NNDX *)Jmalloc(nndarray * sizeof(struct NNDX), L"reorganize_neighbours: narray");
		if (ndarray == NULL) return MSG_NOT_ENOUGH_MEMORY;
		for (k = 0; k < nw[rep].out_deg; k++)
			ndarray[k].ndx = nw[rep].out_nbrs[k];	
		qsort((void *)ndarray, (size_t)nndarray, sizeof(struct NNDX), compare_nndx);
		prev = nw[rep].out_nbrs[0] = ndarray[0].ndx;
		nout_deg = 1;
		for (k = 1; k < nndarray; k++)
		{
			if (ndarray[k].ndx != prev)
			{
				nw[rep].out_nbrs[nout_deg] = ndarray[k].ndx;
				nw[rep].out_weight[nout_deg] += nw[rep].out_weight[k]; 							
				nw[rep].out_strength += nw[rep].out_weight[k];
				nout_deg++;
				prev = ndarray[k].ndx;
			}
		}
		nw[rep].out_deg = nout_deg;
		Jfree(ndarray, L"reorganize_neighbours: narray");
	}

	return 0;
}

//
// display the content of serialdoc[] array 
//
int display_serialdocs(int ns, struct SERIALDOCS *serialdoc)
{
	int i, k;

#ifdef DEBUG
	for (i = 0; i < ns; i++)
	{
		fwprintf(logstream, L"id=%s, nd=%d\n", serialdoc[i].sdocid, serialdoc[i].nd);
		for (k = 0; k < serialdoc[i].nd; k++)
			fwprintf(logstream, L"[%d %s] ", serialdoc[i].ndx[k], serialdoc[i].docname[k]);
		fwprintf(logstream, L"\n");
	}

	fflush(logstream);
#endif DEBUG

	return 0;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_nndx(const void *n1, const void *n2)
{
	struct NNDX *t1, *t2;
	
	t1 = (struct NNDX *)n1;
	t2 = (struct NNDX *)n2;
	if (t2->ndx < t1->ndx)
		return 1;
	else if (t2->ndx == t1->ndx)
		return 0;
	else return -1;
}
