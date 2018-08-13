// 
// co-author.cpp
//
// Author: John Liu
// 
// this file contains function that assembles co-author data and writes the result into a Pajek file
//
//

// Revision History:
// 2014/08/23 fixed a problem in connecting authors, always directing from authors with smaller ID to larger ID
// 2014/10/29 added a type indicator to the augument of the coauthor() function
// 2015/10/29 added codes to establish coauthor links (function link_author_nodes_coauthor())
// 2015/12/19 fixed a bug in the function coauthor(), the program crashed when there are no coauthors.
// 2016/01/28 changed the file opening to Unicode mode (ccs=UTF-8)
//

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include "resource.h"
#include "network.h"

int link_author_nodes_coauthor(int, int, double);
int compare_ap(const void *, const void *);

wchar_t coauthorfilename[FNAME_SIZE];	// this is a dirty global, be careful!
extern int author_to_Pajek_file(wchar_t *, int, struct AUTHORS *);

extern FILE *logstream;
extern int nwos;
extern struct WOS *wos;
extern int naus;
extern struct AUTHORS *authors;	// author name array

//
// assemble co-author data and write the result into a Pajek file
// type=0 ==> output coauthor network that contains all authors
// type=1 ==> output coauthor network that contains only authors that have been 1st author
//
#define OUTPUT_COAUTHOR_INFO
int coauthor(wchar_t *fname, int type)
{
#ifdef OUTPUT_COAUTHOR_INFO
	int i, j, k, m;
	int ndx1, ndx2;				
	struct APAIR *ap;
	int *first_authors, n1st;
	int nap;	// number of author pairs
	int cnt;
	int aj, ak;
	wchar_t prev_name[APSIZE];
	FILE *ostream;

	// initialization for coauthor network, added 2015/10/09	
	if (type == 1)	// only when establishing relationships among the 1st authors
	{
		for (i = 0; i < naus; i++) 
		{		
			authors[i].degree = 0;		// overall degree, number of immediate neighbors  
			authors[i].nbrs = (int *)Jmalloc(N_SBASE * sizeof(int), L"coauthor: authors[i].nbrs");			// neighbor pointer
			authors[i].weight = (double *)Jmalloc(N_SBASE * sizeof(double), L"coauthor: authors[i].weight");		// weighting of this particular link
			authors[i].cur_mem = N_SBASE;// current memory allocated
		}
	}

	first_authors = (int *)malloc(naus * sizeof(int));	// this array contain indices to wos[] array
	ap = (struct APAIR *)malloc(naus * 30 * sizeof(struct APAIR)); // 30 is an estimation	
	if (ap == NULL) return MSG_NOT_ENOUGH_MEMORY;

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

	m = 0;	// total number of author pairs
	for (i = 0; i < nwos; i++)
	{
		for (j = 0; j < wos[i].nau; j++)
		{
			for (k = j + 1; k < wos[i].nau; k++)
			{
				aj = wos[i].author[j];
				ak = wos[i].author[k];
				if (type == 0)	// want network for all authors
				{
					//fwprintf(logstream, L"%s %d %d: %06d %06d\n", wos[i].alias, j, k, wos[i].author[j]+1, wos[i].author[k]+1); fflush(logstream);
					if (aj != ak)	// ignore the strange situation of self co-authoring
					{
						if (aj <= ak)	// added 2014/08/23
							swprintf(ap[m].names, L"%06d %06d", aj+1, ak+1);
						else
							swprintf(ap[m].names, L"%06d %06d", ak+1, aj+1);
						m++;
					}
				}
				else if (type == 1)	// want network for 1st authors
				{
					if (authors[aj].ndx1 != -1 && authors[ak].ndx1 != -1)
					{					
						if (aj != ak)	// ignore the strange situation of self co-suthoring
						{
							if (authors[aj].ndx1 <= authors[ak].ndx1)	
								swprintf(ap[m].names, L"%06d %06d", authors[aj].ndx1+1, authors[ak].ndx1+1);
							else
								swprintf(ap[m].names, L"%06d %06d", authors[ak].ndx1+1, authors[aj].ndx1+1);
							m++;
						}
					}

				}
			}
		}
	}

	if (type == 0)
		fwprintf(logstream, L"Total number of author pairs (all authors)=%d\n", m);
	else if (type == 1)
		fwprintf(logstream, L"Total number of author pairs (1st author only)=%d\n", m);
	nap = m;

	qsort((void *)ap, (size_t)nap, sizeof(struct APAIR), compare_ap);

	// prepare the name for the output file 
	wchar_t oname[FNAME_SIZE], tname[FNAME_SIZE], *sp, *tp;
	wchar_t cname[FNAME_SIZE];
	int backslash;	
	if (type == 0)
		wcscpy(cname, L"Co-author all.paj");
	else
		wcscpy(cname, L"Co-author 1st.paj");
	sp = fname; tp = tname; backslash = 0;
	while (*sp != L'\0') { if (*sp == '\\') backslash++; *tp++ = *sp++; }	// go to the end of the line, and check if there is backslashes in the name
	if (backslash == 0) // no backslash in name
		swprintf_s(oname, FNAME_SIZE, cname);	
	else	// names in long format
	{
		*tp = '\0';
		while (*sp != L'\\') { sp--; tp--; }	// trace back to the last backslash
		*tp = '\0';
		swprintf_s(oname, FNAME_SIZE, L"%s\\%s", tname, cname);
	}

	if (type != 0)	// for 1st author network
		wcscpy(coauthorfilename, oname);	// this will be used by the function cluster_coauthor_network()

	// open the output file
	if (_wfopen_s(&ostream, oname, L"wt, ccs=UTF-8") != 0)	// modified 2016/01/28
		return MSG_OFILE_CANNOTOPEN;

	fwprintf(ostream, L"*Network Co-author\n"); 
	// write out the "*vertice" line 
	// write vertice list, Pajek requires that first vertex is indexed in "1" rather than "0"
	if (type == 0)
	{
		fwprintf(ostream, L"*Vertices %d\n", naus);
		for (i = 0; i < naus; i++) fwprintf(ostream, L"%d \"%s\"\n", i+1, authors[i].name);
	}
	else if (type == 1)
	{
		fwprintf(ostream, L"*Vertices %d\n", n1st);
		for (i = 0; i < n1st; i++) fwprintf(ostream, L"%d \"%s\"\n", i+1, authors[first_authors[i]].name);
	}

	fwprintf(ostream, L"*Edges\n");

	// consolidate duplicate author pairs, write to output file at the same time
	prev_name[0] = '\0';
	k = 0; cnt = 1;
	for (i = 0; i < nap; i++)
	{
		if (wcscmp(ap[i].names, prev_name) != 0)	// hit a new name
		{
			if (k > 0) 
			{
				fwprintf(ostream, L"%s %d\n", prev_name, cnt);	
				if (type == 1)	// only when establishing relationships among the 1st authors
				{
					swscanf(prev_name, L"%d %d", &ndx1, &ndx2);		// added 2015/10/10
					link_author_nodes_coauthor(first_authors[ndx1-1], first_authors[ndx2-1], cnt);	// added 2015/10/10, NOTE: Pajek sequence starts from 1
				}
			}
			wcscpy_s(prev_name, APSIZE, ap[i].names); 
			cnt = 1; k++;
		}
		else
			cnt++;
	}	
	if (prev_name[0] != '\0')	// added the check for prev_name[0], 2015/12/19
	{
		fwprintf(ostream, L"%s %d\n", prev_name, cnt);
		if (type == 1)	// only when establishing relationships among the 1st authors
		{
			swscanf(prev_name, L"%d %d", &ndx1, &ndx2);		// added 2015/10/10
			link_author_nodes_coauthor(first_authors[ndx1-1], first_authors[ndx2-1], cnt);	// added 2015/10/10, NOTE: Pajek sequence starts from 1
		}
	}

	fclose(ostream);
	Jfree(first_authors, L"coauthor: first_authors");
	Jfree(ap, L"coauthor: ap");
#endif OUTPUT_COAUTHOR_INFO

	//if (type == 0)	// only when type = 0
	//	author_to_Pajek_file(L"TESTA.paj", naus, authors, L"Coauthor-All");

	return 0;
}


//
// link two author nodes according to their co-author relationships
// created 2015/10/10
//
int link_author_nodes_coauthor(int ndx1, int ndx2, double wt)
{
	int i;
	int degree;
	int *tmp1;
	double *tmp2;

	// for node ndx1
	degree = authors[ndx1].degree;
	authors[ndx1].nbrs[degree] = ndx2;		// neighbor pointer
	authors[ndx1].weight[degree] = wt;		// weight of this particular link
	authors[ndx1].degree++;					// number of immediate neighbors 
	if (authors[ndx1].degree == authors[ndx1].cur_mem)	// used up the memory allocated
	{
		tmp1 = (int *)Jmalloc((authors[ndx1].cur_mem + N_INCREMENT) * sizeof(int), L"link_author_nodes_coauthor: tmp1");
		tmp2 = (double *)Jmalloc((authors[ndx1].cur_mem + N_INCREMENT) * sizeof(double), L"link_author_nodes_coauthor: tmp2");
		authors[ndx1].cur_mem += N_INCREMENT;
		for (i = 0; i < authors[ndx1].degree; i++) { tmp1[i] = authors[ndx1].nbrs[i]; tmp2[i] = authors[ndx1].weight[i]; }
		Jfree(authors[ndx1].nbrs, L"link_author_nodes_coauthor: authors[ndx1].nbrs"); Jfree(authors[ndx1].weight, L"link_author_nodes_coauthor: authors[ndx1].weight");
		authors[ndx1].nbrs = tmp1; authors[ndx1].weight = tmp2;
	}

	// for node ndx2
	degree = authors[ndx2].degree;
	authors[ndx2].nbrs[degree] = ndx1;		// neighbor pointer
	authors[ndx2].weight[degree] = wt;		// weighting of this particular link
	authors[ndx2].degree++;					// overall degree, number of immediate neighbors 
	if (authors[ndx2].degree == authors[ndx2].cur_mem)	// used up the memory allocated
	{
		tmp1 = (int *)Jmalloc((authors[ndx2].cur_mem + N_INCREMENT) * sizeof(int), L"link_author_nodes_coauthor: tmp1");
		tmp2 = (double *)Jmalloc((authors[ndx2].cur_mem + N_INCREMENT) * sizeof(double), L"link_author_nodes_coauthor: tmp2");
		authors[ndx2].cur_mem += N_INCREMENT;
		for (i = 0; i < authors[ndx2].degree; i++) { tmp1[i] = authors[ndx2].nbrs[i]; tmp2[i] = authors[ndx2].weight[i]; }
		Jfree(authors[ndx2].nbrs, L"link_author_nodes_coauthor: authors[ndx2].nbrs"); Jfree(authors[ndx2].weight, L"link_author_nodes_coauthor: authors[ndx2].weight");
		authors[ndx2].nbrs = tmp1; authors[ndx2].weight = tmp2;
	}

	return 0;
}


//
// this fucntion is to be called by qsort() only
// 
int compare_ap(const void *n1, const void *n2)
{
	struct APAIR *t1, *t2;
	
	t1 = (struct APAIR *)n1;
	t2 = (struct APAIR *)n2;
	if (wcscmp(t2->names, t1->names) < 0)
		return 1;
	else if (wcscmp(t2->names, t1->names) == 0)
		return 0;
	else return -1;
}