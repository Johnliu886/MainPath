// 
// co-assignee.cpp
//
// Author: John Liu
// 
// this file contains function that assembles co-assignee data and writes the result into a Pajek file
//
//

// Revision History:
// 2016/12/24 Basic function works
// 2016/12/29 Added partition information to the co-assignee network 
// 2017/02/25 Changed the partition information from 1/2, to 1/2/3/4 (1: US university, 2: non-US universiy, 3: US non-university, 4: non-US non-univeristy)
// 2017/03/27 Added function prepare_coassignee_data()
// 2018/05/08 Added code to check if university-industry has been detected before assigning partition
// 2018/06/23 Added check to avoid errors from strnage assignee data (make sure wos[i].DE[k] >= 0)
//

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include "resource.h"
#include "network.h"

int link_assignee_nodes_coassignee(int, int, double);

wchar_t coassigneefilename[FNAME_SIZE];	// this is a dirty global, be careful!
extern struct LOCATION location2[]; 

extern FILE *logstream;
extern int nwos;
extern struct WOS *wos;
extern int nasgns;
extern int check_UI;
extern struct ASSIGNEES *assignees;	// for patent data only
extern int compare_ap(const void *, const void *);
extern int is_university(wchar_t *);

//
// assemble co-assignee data and write the result into a Pajek file
// type=0 ==> output coassignee network that contains all assignees
// type=1 ==> output coassignee network that contains only assignees that have been 1st assignee
// in contast to the coauthor() function that create a network contains only the 1st authors, 
//    coassignee() create a network that contains all assignees
//
#define OUTPUT_COASSIGNEE_INFO
int coassignee(wchar_t *fname, int type)
{
#ifdef OUTPUT_COASSIGNEE_INFO
	int i, j, k, m;
	int ndx1, ndx2;				
	struct APAIR *ap;
	int *first_assignees, n1st;
	int nap;	// number of assignee pairs
	int cnt;
	int loc;
	int aj, ak;
	wchar_t prev_name[APSIZE];
	FILE *ostream;

	// initialization for coassignee network
	if (type == 0)	// only when establishing relationships among the 1st assignees
	{
		for (i = 0; i < nasgns; i++) 
		{		
			assignees[i].degree = 0;		// overall degree, number of immediate neighbors  
			assignees[i].nbrs = (int *)Jmalloc(N_SBASE * sizeof(int), L"coassignee: assignees[i].nbrs");			// neighbor pointer
			assignees[i].weight = (double *)Jmalloc(N_SBASE * sizeof(double), L"coassignee: assignees[i].weight");		// weighting of this particular link
			assignees[i].cur_mem = N_SBASE;// current memory allocated
		}
	}

	first_assignees = (int *)malloc(nasgns * sizeof(int));	// this array contain indices to wos[] array
	ap = (struct APAIR *)malloc(nasgns * 30 * sizeof(struct APAIR)); // 30 is an estimation	
	if (ap == NULL) return MSG_NOT_ENOUGH_MEMORY;

	k = 0;
	for (i = 0; i < nasgns; i++)
	{
		if (assignees[i].cnt1 > 0)
		{
			assignees[i].ndx1 = k;
			first_assignees[k] = i;
			k++;
		}
		else
			assignees[i].ndx1 = -1;
	}
	n1st = k;	// the number of assignees that have been 1st assignee

	m = 0;	// total number of assignee pairs
	for (i = 0; i < nwos; i++)
	{
		for (j = 0; j < wos[i].nde; j++)
		{
			for (k = j + 1; k < wos[i].nde; k++)
			{
				aj = wos[i].DE[j];
				ak = wos[i].DE[k];
				if (aj < 0 || ak < 0) continue;	// added 2018/06/23
				if (type == 0)	// want network for all assignees
				{
					//fwprintf(logstream, L"%s %d %d: %06d %06d\n", wos[i].alias, j, k, wos[i].DE[j]+1, wos[i].DE[k]+1); fflush(logstream);
					if (aj != ak)	// ignore the strange situation of self co-assigneeing
					{
						if (aj <= ak)	// added 2014/08/23
							swprintf(ap[m].names, L"%06d %06d", aj+1, ak+1);
						else
							swprintf(ap[m].names, L"%06d %06d", ak+1, aj+1);
						m++;
					}
				}
				else if (type == 1)	// want network for 1st assignees
				{
					if (assignees[aj].ndx1 != -1 && assignees[ak].ndx1 != -1)
					{					
						if (aj != ak)	// ignore the strange situation of self co-suthoring
						{
							if (assignees[aj].ndx1 <= assignees[ak].ndx1)	
								swprintf(ap[m].names, L"%06d %06d", assignees[aj].ndx1+1, assignees[ak].ndx1+1);
							else
								swprintf(ap[m].names, L"%06d %06d", assignees[ak].ndx1+1, assignees[aj].ndx1+1);
							m++;
						}
					}

				}
			}
		}
	}

	if (type == 0)
		fwprintf(logstream, L"Total number of assignee pairs (all assignees)=%d\n", m);
	else if (type == 1)
		fwprintf(logstream, L"Total number of assignee pairs (1st assignee only)=%d\n", m);
	nap = m;

	qsort((void *)ap, (size_t)nap, sizeof(struct APAIR), compare_ap);

	// prepare the name for the output file 
	wchar_t oname[FNAME_SIZE], tname[FNAME_SIZE], *sp, *tp;
	wchar_t cname[FNAME_SIZE];
	int backslash;	
	if (type == 0)
		wcscpy(cname, L"Co-assignee all.paj");
	else
		wcscpy(cname, L"Co-assignee 1st.paj");
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

	if (type != 0)	// for 1st assignee network
		wcscpy(coassigneefilename, oname);	// this will be used by the function cluster_coassignee_network()

	// open the output file
	if (_wfopen_s(&ostream, oname, L"wt, ccs=UTF-8") != 0)	// modified 2016/01/28
		return MSG_OFILE_CANNOTOPEN;

	fwprintf(ostream, L"*Network Co-assignee\n"); 
	// write out the "*vertice" line 
	// write vertice list, Pajek requires that first vertex is indexed in "1" rather than "0"
	if (type == 0)
	{
		fwprintf(ostream, L"*Vertices %d\n", nasgns);
		for (i = 0; i < nasgns; i++) fwprintf(ostream, L"%d \"%s\"\n", i+1, assignees[i].name);
	}
	else if (type == 1)
	{
		fwprintf(ostream, L"*Vertices %d\n", n1st);
		for (i = 0; i < n1st; i++) fwprintf(ostream, L"%d \"%s\"\n", i+1, assignees[first_assignees[i]].name);
	}

	fwprintf(ostream, L"*Edges\n");

	// consolidate duplicate assignee pairs, write to output file at the same time
	prev_name[0] = '\0';
	k = 0; cnt = 1;
	for (i = 0; i < nap; i++)
	{
		if (wcscmp(ap[i].names, prev_name) != 0)	// hit a new name
		{
			if (k > 0) 
			{
				fwprintf(ostream, L"%s %d\n", prev_name, cnt);	
				if (type == 0)	// only when establishing relationships among all assignees
				{
					swscanf(prev_name, L"%d %d", &ndx1, &ndx2);
					//link_assignee_nodes_coassignee(first_assignees[ndx1-1], first_assignees[ndx2-1], cnt);	
					link_assignee_nodes_coassignee(ndx1-1, ndx2-1, cnt);	
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
		if (type == 0)	// only when establishing relationships among all assignees
		{
			swscanf(prev_name, L"%d %d", &ndx1, &ndx2);	
			//link_assignee_nodes_coassignee(first_assignees[ndx1-1], first_assignees[ndx2-1], cnt);	
			link_assignee_nodes_coassignee(ndx1-1, ndx2-1, cnt);	
		}
	}

	if (check_UI == 0)	// do not add partition information, added 2018/05/08
	{
		fclose(ostream);
		Jfree(first_assignees, L"coassignee: first_assignees");
		Jfree(ap, L"coassignee: ap");
		return 0;
	}

	// partition information is added 2016/12/29
	fwprintf(ostream, L"*Partition created by MainPath program, 1: US university, 2: non-US universiy, 3: US non-university, 4: non-US non-univeristy\n");
	if (type == 0)
	{
		fwprintf(ostream, L"*Vertices %d\n", nasgns);
		for (i = 0; i < nasgns; i++) 
		{
			if (is_university(assignees[i].name))
			{
				if (wcscmp(location2[assignees[i].location].name, L"US") == 0)	// added 2017/02/25
					loc = 1;
				else
					loc = 2;
			}
			else
			{
				if (wcscmp(location2[assignees[i].location].name, L"US") == 0)	// added 2017/02/25
					loc = 3;
				else
					loc = 4;
			}
			fwprintf(ostream, L"%d\n", loc);
		}
	}
	else if (type == 1)
	{
		fwprintf(ostream, L"*Vertices %d\n", n1st);
		for (i = 0; i < n1st; i++) 
		{
			//fwprintf(logstream, L"#####[%s]\n",location2[assignees[i].location].name);
			if (is_university(assignees[i].name))
			{
				if (wcscmp(location2[assignees[i].location].name, L"US") == 0)	// added 2017/02/25
					loc = 1;
				else
					loc = 2;
			}
			else
			{
				if (wcscmp(location2[assignees[i].location].name, L"US") == 0)	// added 2017/02/25
					loc = 3;
				else
					loc = 4;
			}
			fwprintf(ostream, L"%d\n", loc);
		}
	}

	fclose(ostream);
	Jfree(first_assignees, L"coassignee: first_assignees");
	Jfree(ap, L"coassignee: ap");
#endif OUTPUT_COASSIGNEE_INFO

	return 0;
}

//
// write coassignee info to a bipartite paper-assignee relationship file
// coassignee() writes coassignee network into Pajek files
// prepare_coassignee_data() writes only a relationship list file
// this function is added 2017/03/27
//
int prepare_coassignee_data(wchar_t *dpath)
{
	int i, j, k;
	FILE *castream;
	wchar_t fname[FNAME_SIZE];

	swprintf(fname, FNAME_SIZE, L"%s Coassignee.txt", dpath);
	_wfopen_s(&castream, fname, L"wt, ccs=UTF-8");

	for (i = 0; i < nwos; i++)
	{
		for (k = 0; k < wos[i].nde; k++)
		{
			j = wos[i].DE[k];
			if (j < 0)
				fwprintf(castream, L"%s\t%s\t%d\n", wos[i].docid, L"unknown", 1);
			else
				fwprintf(castream, L"%s\t%s\t%d\n", wos[i].docid, assignees[wos[i].DE[k]].name, 1);
		}
	}

	fclose(castream);

	return 0;
}

//
// link two assignee nodes according to their co-assignee relationships
//
int link_assignee_nodes_coassignee(int ndx1, int ndx2, double wt)
{
	int i;
	int degree;
	int *tmp1;
	double *tmp2;

	// for node ndx1
	degree = assignees[ndx1].degree;
	assignees[ndx1].nbrs[degree] = ndx2;		// neighbor pointer
	assignees[ndx1].weight[degree] = wt;		// weight of this particular link
	assignees[ndx1].degree++;					// number of immediate neighbors 
	if (assignees[ndx1].degree == assignees[ndx1].cur_mem)	// used up the memory allocated
	{
		tmp1 = (int *)Jmalloc((assignees[ndx1].cur_mem + N_INCREMENT) * sizeof(int), L"link_assignee_nodes_coassignee: tmp1");
		tmp2 = (double *)Jmalloc((assignees[ndx1].cur_mem + N_INCREMENT) * sizeof(double), L"link_assignee_nodes_coassignee: tmp2");
		assignees[ndx1].cur_mem += N_INCREMENT;
		for (i = 0; i < assignees[ndx1].degree; i++) { tmp1[i] = assignees[ndx1].nbrs[i]; tmp2[i] = assignees[ndx1].weight[i]; }
		Jfree(assignees[ndx1].nbrs, L"link_assignee_nodes_coassignee: assignees[ndx1].nbrs"); Jfree(assignees[ndx1].weight, L"link_assignee_nodes_coassignee: assignees[ndx1].weight");
		assignees[ndx1].nbrs = tmp1; assignees[ndx1].weight = tmp2;
	}

	// for node ndx2
	degree = assignees[ndx2].degree;
	assignees[ndx2].nbrs[degree] = ndx1;		// neighbor pointer
	assignees[ndx2].weight[degree] = wt;		// weighting of this particular link
	assignees[ndx2].degree++;					// overall degree, number of immediate neighbors 
	if (assignees[ndx2].degree == assignees[ndx2].cur_mem)	// used up the memory allocated
	{
		tmp1 = (int *)Jmalloc((assignees[ndx2].cur_mem + N_INCREMENT) * sizeof(int), L"link_assignee_nodes_coassignee: tmp1");
		tmp2 = (double *)Jmalloc((assignees[ndx2].cur_mem + N_INCREMENT) * sizeof(double), L"link_assignee_nodes_coassignee: tmp2");
		assignees[ndx2].cur_mem += N_INCREMENT;
		for (i = 0; i < assignees[ndx2].degree; i++) { tmp1[i] = assignees[ndx2].nbrs[i]; tmp2[i] = assignees[ndx2].weight[i]; }
		Jfree(assignees[ndx2].nbrs, L"link_assignee_nodes_coassignee: assignees[ndx2].nbrs"); Jfree(assignees[ndx2].weight, L"link_assignee_nodes_coassignee: assignees[ndx2].weight");
		assignees[ndx2].nbrs = tmp1; assignees[ndx2].weight = tmp2;
	}

	return 0;
}

