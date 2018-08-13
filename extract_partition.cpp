//
// extract_partition.cpp
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
#include "resource.h"
#include "network.h"
#include "clustering.h"

extern FILE *logstream;
extern struct AUTHORS *authors;	// author name array

//
// given a group id (partition id), extract the subnetwork (of a coauthor network)
//
int extract_partition(int groupid, struct RESEARCH_GROUP *rgroups, int naus, struct AUTHORS *athrs)
{
	int i, j, k, gk;
	int gnaus;
	int nlinks;
	struct AUTHORS *gathrs;

	gnaus = rgroups[groupid].nscholars;
	gathrs = (struct AUTHORS *)malloc(gnaus * sizeof(struct AUTHORS));
	if (gathrs == NULL) return MSG_NOT_ENOUGH_MEMORY;

	// copy the data over and establish the cross reference
	for (i = 0, j = 0; i < naus; i++)
	{
		if (athrs[i].groupid == groupid)
		{
			gathrs[j] = athrs[i];
			gathrs[j].xndx = i;		// cross reference, partition to full array
			athrs[i].xndx = j;		// cross reference, full array to partition
			j++;
		}
		else
			athrs[i].xndx = -1;		// does not belong to the partition
	}

	// adjust the contents of "nbrs" and "weight" pointers
	nlinks = 0;
	for (j = 0; j < gnaus; j++)
	{
		i = gathrs[j].xndx;
		gk = 0;
		for (k = 0; k < athrs[i].degree; k++)
		{
			if (athrs[athrs[i].nbrs[k]].xndx != -1)
			{
				gathrs[j].nbrs[gk] = athrs[athrs[i].nbrs[k]].xndx;
				gathrs[j].weight[gk] = athrs[i].weight[k];
				gk++;
			}
		}
		gathrs[j].degree = gk;
		nlinks += gk;
	}

	rgroups[groupid].athrs = gathrs;

	return 0;
}

//
// given a group id (partition id), extract the subnetwork (of a coassignee network)
//
int extract_partition_assignee(int groupid, struct RESEARCH_GROUP *rgroups, int nasgns, struct ASSIGNEES *asgns)
{
	int i, j, k, gk;
	int gnasgns;
	int nlinks;
	struct ASSIGNEES *gasgns;

	gnasgns = rgroups[groupid].nscholars;
	gasgns = (struct ASSIGNEES *)malloc(gnasgns * sizeof(struct ASSIGNEES));
	if (gasgns == NULL) return MSG_NOT_ENOUGH_MEMORY;

	// copy the data over and establish the cross reference
	for (i = 0, j = 0; i < nasgns; i++)
	{
		if (asgns[i].groupid == groupid)
		{
			gasgns[j] = asgns[i];
			gasgns[j].xndx = i;		// cross reference, partition to full array
			asgns[i].xndx = j;		// cross reference, full array to partition
			j++;
		}
		else
			asgns[i].xndx = -1;		// does not belong to the partition
	}

	// adjust the contents of "nbrs" and "weight" pointers
	nlinks = 0;
	for (j = 0; j < gnasgns; j++)
	{
		i = gasgns[j].xndx;
		gk = 0;
		for (k = 0; k < asgns[i].degree; k++)
		{
			if (asgns[asgns[i].nbrs[k]].xndx != -1)
			{
				gasgns[j].nbrs[gk] = asgns[asgns[i].nbrs[k]].xndx;
				gasgns[j].weight[gk] = asgns[i].weight[k];
				gk++;
			}
		}
		gasgns[j].degree = gk;
		nlinks += gk;
	}

	rgroups[groupid].asgns = gasgns;

	return 0;
}

//
// write the given author network to a Pajek file
//
int author_to_Pajek_file(wchar_t *fname, int naus, struct AUTHORS *athrs, wchar_t *msg)
{
	int i, k;
	FILE *ostream;

	//for (i = 0; i < naus; i++)
	//	fwprintf(logstream, L"%d, d=%d\n", i+1, athrs[i].degree);

	// prepare the name for the output file 
	wchar_t oname[FNAME_SIZE], tname[FNAME_SIZE], *sp, *tp;
	wchar_t cname[FNAME_SIZE];
	int backslash;	
#ifdef XXX
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
#endif XXX
	wcscpy(oname, fname);

	// open the output file
	if (_wfopen_s(&ostream, oname, L"w") != 0) 
		return MSG_OFILE_CANNOTOPEN;

	fwprintf(ostream, L"*Network %s\n", msg); 
	// write out the "*vertice" line 
	// write vertice list, Pajek requires that first vertex is indexed in "1" rather than "0"
		fwprintf(ostream, L"*Vertices %d\n", naus);
		for (i = 0; i < naus; i++) fwprintf(ostream, L"%d \"%s\"\n", i+1, athrs[i].name);

	fwprintf(ostream, L"*Edges\n");

	for (i = 0; i < naus; i++)
	{
		//fwprintf(logstream, L"%d, degree=%d\n", i+1, athrs[i].degree);
		for (k = 0; k < athrs[i].degree; k++)
		{
			if (i < athrs[i].nbrs[k]) // to avoid duplicating a link
				fwprintf(ostream, L"%06d %06d %.0f\n", i+1, athrs[i].nbrs[k]+1, athrs[i].weight[k]);
		}
	}
	fclose(ostream);

	return 0;
}


//
// write the given research group network to a Pajek file (according to co-author linkages)
//
int rgroup_to_Pajek_file(wchar_t *fname, int nrgroups, struct RESEARCH_GROUP *rgroups, wchar_t *msg)
{
	int i, k;
	FILE *ostream;
	wchar_t oname[FNAME_SIZE];

	wcscpy(oname, fname);

	// open the output file
	if (_wfopen_s(&ostream, oname, L"w") != 0) 
		return MSG_OFILE_CANNOTOPEN;

	fwprintf(ostream, L"*Network %s\n", msg); 
	// write out the "*vertice" line 
	// write vertice list, Pajek requires that first vertex is indexed in "1" rather than "0"
	fwprintf(ostream, L"*Vertices %d\n", nrgroups);
	for (i = 0; i < nrgroups; i++) fwprintf(ostream, L"%d \"#Group%d\"\n", i+1, i);

	fwprintf(ostream, L"*Edges\n");

	for (i = 0; i < nrgroups; i++)
	{
		//fwprintf(logstream, L"%d, degree=%d\n", i+1, athrs[i].degree);
		for (k = 0; k < rgroups[i].degree; k++)
		{
			if (i <= rgroups[i].nbrs[k]) // to avoid duplicating a link, but want to include self-links
				fwprintf(ostream, L"%06d %06d %.0f\n", i+1, rgroups[i].nbrs[k]+1, rgroups[i].weight[k]);
		}
	}

	fwprintf(ostream, L"*Partition created by MainPath/GroupFinder program\n");
	fwprintf(ostream, L"*Vertices %d\n", nrgroups);
	for (i = 0; i < nrgroups; i++)
		fwprintf(ostream, L"%d\n", i);

	fclose(ostream);

	return 0;
}

//
// write the given research group network to a Pajek file (according to citation linkages)
//
int rgroup_citation_to_Pajek_file(wchar_t *fname, int nrgroups, struct RESEARCH_GROUP *rgroups, wchar_t *msg)
{
	int i, k;
	FILE *ostream;
	wchar_t oname[FNAME_SIZE];

	wcscpy(oname, fname);

	// open the output file
	if (_wfopen_s(&ostream, oname, L"w") != 0) 
		return MSG_OFILE_CANNOTOPEN;

	fwprintf(ostream, L"*Network %s\n", msg); 
	// write out the "*vertice" line 
	// write vertice list, Pajek requires that first vertex is indexed in "1" rather than "0"
	fwprintf(ostream, L"*Vertices %d\n", nrgroups);
	for (i = 0; i < nrgroups; i++) fwprintf(ostream, L"%d \"#Group%d\"\n", i+1, i);

	fwprintf(ostream, L"*Arcs\n");

	for (i = 0; i < nrgroups; i++)
	{
		for (k = 0; k < rgroups[i].ctn_indegree; k++)
			fwprintf(ostream, L"%06d %06d %.0f\n", rgroups[i].ctn_innbrs[k]+1, i+1, rgroups[i].ctn_inweight[k]);
		//for (k = 0; k < rgroups[i].ctn_outdegree; k++)
		//	fwprintf(ostream, L"%06d %06d %.0f\n", i+1, rgroups[i].ctn_outnbrs[k]+1, rgroups[i].ctn_outweight[k]);
	}

	fwprintf(ostream, L"*Partition created by MainPath/GroupFinder program\n");
	fwprintf(ostream, L"*Vertices %d\n", nrgroups);
	for (i = 0; i < nrgroups; i++)
		fwprintf(ostream, L"%d\n", i);

	fclose(ostream);

	return 0;
}
