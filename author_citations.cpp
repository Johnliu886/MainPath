//
// author_citations.cpp
//

//
// Revision History:
// 
// 2015/06/23 Basic function works
// 2015/07/16 Added codes to handle the case for 1st authors only network (type= 0, all authors, type=1, 1st authors only) 
// 2015/10/20 added codes to establish author citation links 
// 2015/10/29 Changed take in self-citation, this is to be consistent with "rgroups", which also contains self-citations
// 2016/01/19 Modification  : changed the file opening to Unicode mode (ccs=UTF-8)
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
extern int nnodes;
extern struct PN *nw;
extern int nwos;
extern struct WOS *wos;
extern int naus;
extern struct AUTHORS *authors;	// author name array

int link_author_nodes_citation(int, int, double);
int compare_ctstr(const void *, const void *);

struct AUTHOR_CITATIONS
{
	wchar_t str[14];
	int cnt;
};

//
// write author citations to a Pajek file
//
int write_author_citations(wchar_t *dpath, int type)
{
	int i, k, w1, w2, c1, c2, wc1, wc2;
	int ndx1, ndx2;	
	int ncites;
	wchar_t fname[FNAME_SIZE];
	wchar_t wline[LBUF_SIZE+1];
	FILE *acstream;
	struct AUTHOR_CITATIONS *ct;
	int *first_authors, n1st;

	// initialization for cocitation network, added 2015/10/20
	if (type == 1)	// only when establishing relationships among the 1st authors
	{
		for (i = 0; i < naus; i++) 
		{		
			authors[i].ctn_indegree = 0;		// inward degree, number of immediate neighbors  
			authors[i].ctn_innbrs = (int *)Jmalloc(N_SBASE * sizeof(int), L"write_author_citations: authors[i].ctn_innbrs");			// neighbor pointer
			authors[i].ctn_inweight = (double *)Jmalloc(N_SBASE * sizeof(double), L"write_author_citations: authors[i].ctn_inweight");	// weighting of this particular link
			authors[i].ctn_incur_mem = N_SBASE;// current memory allocated
			authors[i].ctn_outdegree = 0;		// outward degree, number of immediate neighbors  
			authors[i].ctn_outnbrs = (int *)Jmalloc(N_SBASE * sizeof(int), L"write_author_citations: authors[i].ctn_outnbrs");			// neighbor pointer
			authors[i].ctn_outweight = (double *)Jmalloc(N_SBASE * sizeof(double), L"write_author_citations: authors[i].ctn_outweight");// weighting of this particular link
			authors[i].ctn_outcur_mem = N_SBASE;// current memory allocated
		}
	}

	// establish the first author array and its cross indices to authors[]
	first_authors = (int *)malloc(naus * sizeof(int));	// this array contain indices to wos[] array
	k = 0;
	for (i = 0; i < naus; i++)
	{
		if (authors[i].cnt1 > 0)
		{
			authors[i].ndx1 = k;
			first_authors[k] = i;
			k++;
		}
		else
			authors[i].ndx1 = -1;
	}
	n1st = k;	// the number of authors that have been 1st author

	// count the maximal possible size
	ncites = 0;
	for (i = 0; i < nnodes; i++)
	{
		w1 = nw[i].ndx2wos;
		for (k = 0; k < nw[i].in_deg; k++)
		{
			w2 = nw[nw[i].in_nbrs[k]].ndx2wos;
			ncites += wos[w1].nau * wos[w2].nau;			
		}
	}
	//fwprintf(logstream, L"write_author_citations(): ncites=%d\n", ncites);

	// allocate memory
	ct = (struct AUTHOR_CITATIONS *)Jmalloc(ncites * sizeof(struct AUTHOR_CITATIONS), L"write_author_citations: ct");
	if (ct == NULL) return MSG_NOT_ENOUGH_MEMORY;

	// prepare the citation list
	ncites = 0;
	for (i = 0; i < nnodes; i++)
	{
		w1 = nw[i].ndx2wos;
		for (k = 0; k < nw[i].in_deg; k++)
		{
			w2 = nw[nw[i].in_nbrs[k]].ndx2wos;
			for (c1 = 0; c1 < wos[w1].nau; c1++)
			{
				for (c2 = 0; c2 < wos[w2].nau; c2++)
				{
					wc1 = wos[w1].author[c1];
					wc2 = wos[w2].author[c2];
					if (type == 0)	// want network for all authors
					{
						// NOTE: take in self-citation starting from 2015/10/29, this is to be consistent with "rgroups", which also contains self-citations
						//if (wc1 != wc2) // ignore self-citations
						//{
							swprintf(ct[ncites].str, L"%06d %06d", wc2+1, wc1+1);
							//fwprintf(logstream, L"[%06d]%s [%06d]%s\n", wos[w2].author[c2]+1, authors[wos[w2].author[c2]].name, wos[w1].author[c1]+1, authors[wos[w1].author[c1]].name);
							ncites++;
						//}
					}
					else if (type == 1)	// want network for 1st authors
					{
						if (authors[wc1].ndx1 != -1 && authors[wc2].ndx1 != -1) // both are 1st authors
						{	
							// NOTE: take in self-citation starting from 2015/10/29, this is to be consistent with "rgroups", which also contains self-citations			
							//if (wc1 != wc2) // ignore self-citations
							//{
								swprintf(ct[ncites].str, L"%06d %06d", authors[wc2].ndx1+1, authors[wc1].ndx1+1);
								//fwprintf(logstream, L"[%06d]%s [%06d]%s\n", authors[wc2].ndx1+1, authors[wc2].name, authors[wc1].ndx1+1, authors[wc1].name);
								ncites++;
							//}
						}
					}
				}
			}
		}
	}

	if (type == 0)
		fwprintf(logstream, L"write_author_citations(): total number of citations (all authors)=%d\n", ncites);
	else if (type == 1)
		fwprintf(logstream, L"write_author_citations(): total number of citations (1st author only)=%d\n", ncites);

	if (type == 0)
		swprintf(fname, FNAME_SIZE, L"%s author citatons all.paj", dpath);
	else if (type == 1)
		swprintf(fname, FNAME_SIZE, L"%s author citatons 1st.paj", dpath);
	_wfopen_s(&acstream, fname, L"wt, ccs=UTF-8");	// modified 2016/01/19

	// write out the "*Network" line
	swprintf_s(wline, LBUF_SIZE, L"*Network prepared by MainPath program (WARNING: the accuracy of the network is not fully verified as of 2015/06/23.\n");
	fwprintf(acstream, L"%s", wline); 

	// write out the "*vertice" line 
	if (type == 0)
	{
		swprintf_s(wline, LBUF_SIZE, L"*Vertices %d\n", naus);
		fwprintf(acstream, L"%s", wline); 
		for (i = 0; i < naus; i++) fwprintf(acstream, L"%d \"%s\"\n", i+1, authors[i].name); 
	}
	else if (type == 1)
	{
		swprintf_s(wline, LBUF_SIZE, L"*Vertices %d\n", n1st);
		fwprintf(acstream, L"%s", wline); 
		for (i = 0; i < n1st; i++) fwprintf(acstream, L"%d \"%s\"\n", i+1, authors[first_authors[i]].name);
	}

	swprintf_s(wline, LBUF_SIZE, L"*Arcs\n");
	fwprintf(acstream, L"%s", wline); 

	// consolidate and print the list
	int nct, tcnt;
	wchar_t prev[20];
	qsort((void *)ct, (size_t)ncites, sizeof(struct AUTHOR_CITATIONS), compare_ctstr);	
	//for (i = 0; i < ncites; i++) fwprintf(logstream, L"%d %s\n", i, ct[i].str);
	nct = 0; tcnt = 1; prev[0] = '\0';
	for (k = 0; k < ncites; k++)
	{
		if (wcscmp(ct[k].str, prev) == 0)
			tcnt++;
		else
		{
			if (k != 0)
			{
				fwprintf(acstream, L"%s %d\n", prev, tcnt);
				swscanf(prev, L"%d %d", &ndx1, &ndx2);				// added 2015/10/20
				if (type == 1)	// only when establishing relationship among the 1st authors
					link_author_nodes_citation(first_authors[ndx1-1], first_authors[ndx2-1], tcnt);	// added 2015/10/20, NOTE: Pajek sequence starts from 1
				//fwprintf(logstream, L"%s %d\n", prev, tcnt);
				tcnt = 1; nct++;
			}
			wcscpy(prev, ct[k].str);
		}
	}
	fwprintf(acstream, L"%s %d\n", prev, tcnt);
	swscanf(prev, L"%d %d", &ndx1, &ndx2);				// added 2015/10/20
	if (type == 1)	// only when establishing relationship among the 1st authors
		link_author_nodes_citation(first_authors[ndx1-1], first_authors[ndx2-1], tcnt);	// added 2015/10/20, NOTE: Pajek sequence starts from 1
	//fwprintf(logstream, L"%s %d\n", prev, tcnt);

	// write out the partition information
	if (type == 1)
	{
		fwprintf(acstream, L"*Partition created by MainPath program\n");
		fwprintf(acstream, L"*Vertices %d\n", n1st);
		for (i = 0; i < n1st; i++) 
			fwprintf(acstream, L"%d\n", authors[first_authors[i]].groupid);
	}

	fclose(acstream);
	Jfree(first_authors, L"write_author_citations: first_authors");
	Jfree(ct, L"write_author_citations: ct");


	return 0;
}


//
// link two author nodes according to their citation relationships
// created 2015/10/21
//
int link_author_nodes_citation(int ndx1, int ndx2, double wt)
{
	int i;
	int degree;
	int *tmp1;
	double *tmp2;

	// for node ndx1
	degree = authors[ndx1].ctn_outdegree;
	authors[ndx1].ctn_outnbrs[degree] = ndx2;		// neighbor pointer
	authors[ndx1].ctn_outweight[degree] = wt;		// weight of this particular link
	authors[ndx1].ctn_outdegree++;				// number of immediate neighbors 
	if (authors[ndx1].ctn_outdegree == authors[ndx1].ctn_outcur_mem)	// used up the memory allocated
	{
		tmp1 = (int *)Jmalloc((authors[ndx1].ctn_outcur_mem + N_INCREMENT) * sizeof(int), L"link_author_nodes_citation: tmp1");
		tmp2 = (double *)Jmalloc((authors[ndx1].ctn_outcur_mem + N_INCREMENT) * sizeof(double), L"link_author_nodes_citation: tmp2");
		authors[ndx1].ctn_outcur_mem += N_INCREMENT;
		for (i = 0; i < authors[ndx1].ctn_outdegree; i++) { tmp1[i] = authors[ndx1].ctn_outnbrs[i]; tmp2[i] = authors[ndx1].ctn_outweight[i]; }
		Jfree(authors[ndx1].ctn_outnbrs, L"link_author_nodes_citation: authors[ndx1].ctn_outnbrs"); Jfree(authors[ndx1].ctn_outweight, L"link_author_nodes_citation: authors[ndx1].ctn_outweight");
		authors[ndx1].ctn_outnbrs = tmp1; authors[ndx1].ctn_outweight = tmp2;
	}

	// for node ndx2
	degree = authors[ndx2].ctn_indegree;
	authors[ndx2].ctn_innbrs[degree] = ndx1;		// neighbor pointer
	authors[ndx2].ctn_inweight[degree] = wt;		// weighting of this particular link
	authors[ndx2].ctn_indegree++;					// overall degree, number of immediate neighbors 
	if (authors[ndx2].ctn_indegree == authors[ndx2].ctn_incur_mem)	// used up the memory allocated
	{
		tmp1 = (int *)Jmalloc((authors[ndx2].ctn_incur_mem + N_INCREMENT) * sizeof(int), L"link_author_nodes_citation: tmp1");
		tmp2 = (double *)Jmalloc((authors[ndx2].ctn_incur_mem + N_INCREMENT) * sizeof(double), L"link_author_nodes_citation: tmp2");
		authors[ndx2].ctn_incur_mem += N_INCREMENT;
		for (i = 0; i < authors[ndx2].ctn_indegree; i++) { tmp1[i] = authors[ndx2].ctn_innbrs[i]; tmp2[i] = authors[ndx2].ctn_inweight[i]; }
		Jfree(authors[ndx2].ctn_innbrs, L"link_author_nodes_citation: authors[ndx2].ctn_innbrs"); Jfree(authors[ndx2].ctn_inweight, L"link_author_nodes_citation: authors[ndx2].ctn_inweight");
		authors[ndx2].ctn_innbrs = tmp1; authors[ndx2].ctn_inweight = tmp2;
	}

	return 0;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_ctstr(const void *n1, const void *n2)
{
	struct AUTHOR_CITATIONS *t1, *t2;
	
	t1 = (struct AUTHOR_CITATIONS *)n1;
	t2 = (struct AUTHOR_CITATIONS *)n2;
	if (wcscmp(t2->str, t1->str) < 0)
		return 1;
	else if (wcscmp(t2->str, t1->str) == 0)
		return 0;
	else return -1;
}