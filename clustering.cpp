//
// clustering.cpp
//
// this file incdlues functions to cluster citation network and author network
//

// History
//
// 2015/10/07 moved citation network clustering codes from the file "searchpath.cpp" and "text2Pajek.cpp" and create codes to cluster co-author network
// 2015/10/08 completed the new function cluster_coauthor_network()
// 2015/10/28 fixed problems in writing cluster data to the created subdirectory (for non-WOS data) [WOS data is okay]
// 2016/01/11 again, fixed problems in writing cluster data to the created subdirectory
// 2016/03/09 added code to exit the program if the execution of GroupFinder failed
// 2016/03/19 added codes to call readwrite_TaiwanTD()
// 2016/03/20 modified the wfopen call options so that it opens in UTF-8 format
// 2016/04/23 added to call readwrite_TCI()
// 2016/07/06 added new function cluster_coword_doc_network()
// 2016/10/13 Added codes to support WEBPAT3 data	
// 2017/03/26 Added functions: cluster_coassignee_network(), shrink_coassignee_network()
// 2017/05/05 Added to call readwrite_LAWSNOTE()
// 2017/10/20 Added to write out papers that are written by authors in certain groups
// 2017/11/04 Fixed a paper-missing problem on the 2017/10/20 modification. 
//                  Was not considering the fact that a paper can belong to multiple author groups.
// 2018/07/21 Added byear/eyear information to the clustering directory name and data file name
//

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>	// for the CreateDirectory() function
#include "resource.h"
#include "network.h"
#include "clustering.h"

int nrgroups;
struct RESEARCH_GROUP *rgroups;

extern int nwos;
extern struct WOS *wos;
extern int nnodes;
extern struct PN *nw;
extern int naus;
extern struct AUTHORS *authors;	// author name array
extern int nasgns;
extern struct ASSIGNEES *assignees;	// for patent data only
extern int full_record_type;
extern FILE *logstream;

int compare_pnaliasname(const void *, const void *);
int aliasname_search(struct PN *, int, wchar_t *);
int compare_mcnt(const void *, const void *);
int compare_author(const void *, const void *);
int compare_assignee(const void *, const void *);
int compare_groupid(const void *, const void *);
int compare_asgn_groupid(const void *, const void *);
int compare_ndx(const void *, const void *);

extern int compare_nwname(const void *, const void *);
extern int readwrite_WOS(wchar_t *, wchar_t *, int, int);						// based on the nw[] array 
extern int readwrite_WOS_based_on_wos_array(wchar_t *, wchar_t *, int, int);	// based on the wos[] array 
extern int readwrite_Scopus(wchar_t *, wchar_t *, int, int);
extern int readwrite_Scopus_direct(wchar_t *, wchar_t *, int, int);
extern int readwrite_TIP(wchar_t *, wchar_t *, int, int);
extern int readwrite_WEBPAT2(wchar_t *, wchar_t *, int, int);
extern int readwrite_WEBPAT3(wchar_t *, wchar_t *, int, int);
extern int readwrite_USPTO(wchar_t *, wchar_t *, int, int);
extern int readwrite_LAWSNOTE(wchar_t *, wchar_t *, int, int);
extern int parse_vertices(wchar_t *, wchar_t *);
extern int extract_partition(int, struct RESEARCH_GROUP *, int, struct AUTHORS *);
extern int extract_partition_assignee(int, struct RESEARCH_GROUP *, int, struct ASSIGNEES *);
extern int author_to_Pajek_file(wchar_t *, int, struct AUTHORS *, wchar_t *);
extern int rgroup_to_Pajek_file(wchar_t *, int, struct RESEARCH_GROUP *, wchar_t *);
extern double network_density_author(int, struct AUTHORS *); 
extern double weighted_degree_centralization_author(int, struct AUTHORS *);
extern double degree_centralization_traditional_author(int, struct AUTHORS *);
extern int consolidate_group_coutries(int, struct RESEARCH_GROUP *, int, struct AUTHORS  *);
extern int consolidate_group_coutries_assignee(int, struct RESEARCH_GROUP *, int, struct ASSIGNEES  *);
extern int shrink_coauthor_network(int, struct AUTHORS *, int, struct RESEARCH_GROUP *);	// added 2015/10/22
extern int shrink_coassignee_network(int, struct ASSIGNEES *, int, struct RESEARCH_GROUP *);	// added 2017/03/26
extern int weigted_degree_centrality_rgroups(int, struct RESEARCH_GROUP *);					// added 2015/10/22
extern int degree_centrality_rgroups(int, struct RESEARCH_GROUP *);					// added 2015/10/29
extern int readwrite_TaiwanTD(wchar_t *, wchar_t *, int, int);	// added 2016/03/19
extern int readwrite_TCI(wchar_t *, wchar_t *, int, int);	// added 2016/04/23
extern int readwrite_TCI_direct(wchar_t *, wchar_t *, int, int);	// added 2016/07/06

#define TYPE_CITATION_NETWORK	1
#define TYPE_COAUTHOR_NETWORK	2
#define TYPE_COWORD_DOC_NETWORK	3
#define TYPE_COASSIGNEE_NETWORK	4

//
// execute external executable program GroupFinder
//
int execute_GroupFinder(wchar_t *pname, int groupfinderoptions)
{
	int ret;
	wchar_t fname[FNAME_SIZE];
	wchar_t options[FNAME_SIZE];
	wchar_t cmd[50+FNAME_SIZE];

	if (groupfinderoptions == GF_UNDIRECTED_UNWEIGHTED)
		swprintf(options, L"Undirected Unweighted");
	else if (groupfinderoptions == GF_DIRECTED_UNWEIGHTED)
		swprintf(options, L"Unweighted");
	else if (groupfinderoptions == GF_UNDIRECTED_WEIGHTED)
	{
		swprintf(options, L"Undirected");
	}
	else if (groupfinderoptions == GF_DIRECTED_WEIGHTED)
	{
		swprintf(options, L"");
	}

	wcscpy(fname, pname);
    //wcstombs(fname, pname, FNAME_SIZE);
	swprintf(cmd, L"GroupFinder \"%s\" %s", fname, options);	// enforce "undirected" or "unweighted", i.e., ignore the direction or weight
	ret = _wsystem(cmd);

	//mbstowcs(tmps, cmd, FNAME_SIZE);
	if (ret != 0)
	{
		fwprintf(logstream, L"Execution of [%s] failed.\n", cmd);
		exit(-1);	// added 2016/03/09
	}
	else
		fwprintf(logstream, L"Execution of [%s] successfully returned.\n", cmd);

	return ret;
}

#define MAX_NNAME_LENGTH 500
struct NNAME {
	wchar_t name[MAX_NNAME_LENGTH];
	int partition;
};
//
// read the results of GroupFinder program
// GroupFinder results is in the partition section of the Pajek file
//
int read_GroupFinder_results(wchar_t *fname, int type)
{
	int backslash;
	int nn;
	wchar_t oname[FNAME_SIZE];
	wchar_t *sp, *tp;
	FILE *fstream;
	wchar_t ln[SBUF_SIZE];
	wchar_t *line;
	wchar_t tmp[SBUF_SIZE];
	wchar_t nname[SBUF_SIZE];
	struct NNAME *node_name;
	int i, j, state, direction;

	// prepare the name for the Pajek file (a "GF " is added at the beginning of the file name)
	sp = fname; tp = oname; backslash = 0;
	while (*sp != L'\0') { if (*sp == '\\') backslash++; *tp++ = *sp++; }	// go to the end of the line, and check if there is backslashes in the name
	if (backslash == 0) // no backslash in name
		swprintf_s(oname, FNAME_SIZE, L"GF %s", fname);	// added "GF "
	else	// names in long format
	{
		*tp = '\0';
		while (*sp != L'\\') { sp--; tp--; }	// trace back to the last backslash
		tp++; sp++; *tp++ = L'G'; *tp++ = L'F'; *tp++ = ' ';	// pad "GF "
		while (*sp != L'\0') { *tp++ = *sp++; } // copy to the end of the line
		*tp++ = '\0';	
	}

	// open the Pajek file
	if (_wfopen_s(&fstream, oname, L"rt, ccs=UTF-8") != 0)  { fwprintf(logstream, L"Cannot open file \"%s\".\n", oname); return -1; }		
	else fwprintf(logstream, L"File \"%s\" was opened successfully.\n", oname);
		
	// first pass, get the number of nodes and check if the network is directed
	while (TRUE)
	{		
		if(fgetws(ln, SBUF_SIZE, fstream) == NULL)
			break;
		sp = &ln[0]; while (*sp == ' ' || *sp == '\t') sp++;
		line = sp;
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		if (wcsncmp(line, L"*Vertices", 9) == 0)
		{
			swscanf(line, L"%s %d", tmp, &nn);
			node_name = (struct NNAME *)malloc((int)nn * sizeof(struct NNAME));
			if (node_name == NULL) return -1;
		}
		if (wcsncmp(line, L"*Arcs", 5) == 0 || wcsncmp(line, L"*arcs", 5) == 0)
		{
			direction = 1;
			break;
		}
		if (wcsncmp(line, L"*Edges", 6) == 0 || wcsncmp(line, L"*edges", 6) == 0)
		{
			direction = 0;
			break;
		}
	}

	rewind(fstream);
	i = 0; state = 0;
	while (TRUE)
	{		
		if(fgetws(ln, SBUF_SIZE, fstream) == NULL)
			break;
		sp = &ln[0]; while (*sp == ' ' || *sp == '\t') sp++;
		line = sp;
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		if (wcsncmp(line, L"*Vertices", 9) == 0 && state == 0)
		{
			state = 1;
			continue;
		}
		else if ((wcsncmp(line, L"*Arcs", 5) == 0 || wcsncmp(line, L"*Edges", 6) == 0 || wcsncmp(line, L"*arcs", 5) == 0 || wcsncmp(line, L"*edges", 6) == 0) && state == 1)
		{
			state = 2; i = 0;
			continue;
		}
		else if ((wcsncmp(line, L"*Partition", 10) == 0 || wcsncmp(line, L"*partition", 10) == 0) && state == 2)
		{
			state = 3; i = 0;
			continue;
		}
		else if (wcsncmp(line, L"*Vertices", 9) == 0 && state == 3)
		{
			state = 4; i = 0;
			continue;
		}
		if (state == 1)
		{
			parse_vertices(line, nname);
			wcscpy(node_name[i].name, nname);
			i++;
		}
		else if (state == 2)
			i++;	
		else if (state == 4)
		{
			swscanf(line, L"%d", &node_name[i].partition);
			i++;
			if (i >= nn) break;
		}
	}

	if (type == TYPE_CITATION_NETWORK)
	{
		for (i = 0; i < nnodes; i++)
		{
			nw[i].partition = node_name[i].partition;	// the order in the Pajek file is the same as that in the nw[] array
		}
	}
	else if (type == TYPE_COAUTHOR_NETWORK)
	{		
		for (i = 0, j = 0; i < naus; i++)
		{
			if (authors[i].cnt1 != 0)	// being 1st author
			{
				authors[i].groupid = node_name[j++].partition;	// the order in the Pajek file is the same as that in the nw[] array
			}
			else 
				authors[i].groupid = 999999;	// not 1st first author
		}
	}
	else if (type == TYPE_COWORD_DOC_NETWORK)
	{
		for (i = 0; i < nwos; i++)
		{
			wos[i].partition = node_name[i].partition;	// the order in the Pajek file is the same as that in the nw[] array
		}
	}

	free(node_name);

	return 0;
}

//
// this function read a main path Pajek file and then replace the partition section with the results of the GroupFinder program
//
int mainpaths_with_partition(wchar_t *fname)
{
	int backslash;
	int nn;
	wchar_t oname[FNAME_SIZE];
	wchar_t *sp, *tp;
	FILE *fstream;
	wchar_t ln[SBUF_SIZE];
	wchar_t *line;
	wchar_t tmp[SBUF_SIZE];
	wchar_t nname[SBUF_SIZE];
	struct NNAME *node_name;
	int i, state, direction;
	int ndx;
	FILE *ostream;	
	FILE *istream;

	// open the original Pajek file
	if (_wfopen_s(&istream, fname, L"rt, ccs=UTF-8") != 0) { fwprintf(logstream, L"Cannot open file \"%s\".\n", fname); return -1; }		
	else fwprintf(logstream, L"File \"%s\" was opened successfully.\n", fname);

	// prepare the name for the output Pajek file (a "GF " is added at the beginning of the file name)
	sp = fname; tp = oname; backslash = 0;
	while (*sp != L'\0') { if (*sp == '\\') backslash++; *tp++ = *sp++; }	// go to the end of the line, and check if there is backslashes in the name
	if (backslash == 0) // no backslash in name
		swprintf_s(oname, FNAME_SIZE, L"GF %s", fname);	// added "GF "
	else	// names in long format
	{
		*tp = '\0';
		while (*sp != L'\\') { sp--; tp--; }	// trace back to the last backslash
		tp++; sp++; *tp++ = L'G'; *tp++ = L'F'; *tp++ = ' ';	// pad "GF "
		while (*sp != L'\0') { *tp++ = *sp++; } // copy to the end of the line
		*tp++ = '\0';	
	}

	// create the new Pajek file 
	if ((_wfopen_s(&ostream, oname, L"wt, ccs=UTF-8")) != 0) { fwprintf(logstream, L"Can not open file \"%s\".\n", oname); return -1; }
	else fwprintf(logstream, L"File \"%s\" was opened successfully.\n", oname);

	// 1st pass, copy up to the "*Vertices" line (after the "*partition" line)
	i = 0; state = 0;
	while (TRUE) {		
		if(fgetws(ln, SBUF_SIZE, istream) == NULL)
			break;
		sp = &ln[0]; while (*sp == ' ' || *sp == '\t') sp++;
		line = sp;
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		if (wcsncmp(line, L"*Vertices", 9) == 0 && state == 0)
		{
			swscanf(line, L"%s %d", tmp, &nn);
			node_name = (struct NNAME *)malloc((int)nn * sizeof(struct NNAME));
			if (node_name == NULL) return -1;
			state = 1;
		}
		else if ((wcsncmp(line, L"*Arcs", 5) == 0 || wcsncmp(line, L"*Edges", 6) == 0 || wcsncmp(line, L"*arcs", 5) == 0 || wcsncmp(line, L"*edges", 6) == 0) && state == 1)
			state = 2;
		else if (wcsncmp(line, L"*Partition", 10) == 0 && state == 2)
			break;
		if (fputws(line, ostream) < 0) { fwprintf(logstream, L"Cannot write to file \"%s\".\n", oname); return -1; }
	}

	// 2nd pass, obtain node names
	rewind(istream);
	i = 0; state = 0;
	while (TRUE)
	{		
		if(fgetws(ln, SBUF_SIZE, istream) == NULL)
			break;
		sp = &ln[0]; while (*sp == ' ' || *sp == '\t') sp++;
		line = sp;
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ' ' || line[0] == '\t')
			continue;
		if (wcsncmp(line, L"*Vertices", 9) == 0 && state == 0)
		{
			state = 1;
			continue;
		}
		else if ((wcsncmp(line, L"*Arcs", 5) == 0 || wcsncmp(line, L"*Edges", 6) == 0 || wcsncmp(line, L"*arcs", 5) == 0 || wcsncmp(line, L"*edges", 6) == 0) && state == 1)
		{
			state = 2; i = 0;
			continue;
		}
		else if (wcsncmp(line, L"*", 1) == 0 && state == 2)
			break;
		if (state == 1)
		{
			parse_vertices(line, nname);
			wcscpy(node_name[i].name, nname);
			i++;
		}
		else if (state == 2)
			break;
	}

	// write the partition information
	fwprintf(ostream, L"*Partition created by the GroupFinder program\n");
	fwprintf(ostream, L"*Vertices %d\n", nn);
	qsort((void *)nw, (size_t)nnodes, sizeof(struct PN), compare_pnaliasname);	// sort according to alias name
	for (i = 0; i < nn; i++)
	{
		ndx = aliasname_search(nw, nnodes, node_name[i].name);
		fwprintf(ostream, L"%d\n", nw[ndx].partition);
	}
	qsort((void *)nw, (size_t)nnodes, sizeof(struct PN), compare_nwname);		// sort back to the original order

	// close both files 
	if (fclose(ostream)) { fwprintf(logstream, L"Cannot close file \"%s\".\n", oname); return -1; }
	else fwprintf(logstream, L"File \"%s\" was closed successfully.\n", oname);
	if (fclose(istream)) { fwprintf(logstream, L"Cannot close file \"%s\".\n", fname); return -1; }
	else fwprintf(logstream, L"File \"%s\" was closed successfully.\n", fname);

	return 0;
}

//
// cluster the given citation network
// large portion of the codes in this function is moved from the function mainpath() on 2015/10/07
//
int cluster_citation_network(wchar_t *nfname, int groupfinderoptions, wchar_t *dpath, wchar_t *frecordname, wchar_t *tname, wchar_t *citationname)
{
	int i, ii, kk, ret;
	int byear, eyear, ndx;
	wchar_t tmpname[FNAME_SIZE];
	wchar_t dirname[FNAME_SIZE];
	wchar_t citname[FNAME_SIZE];
	wchar_t destname[FNAME_SIZE];
	wchar_t tmps[FNAME_SIZE], *sp;

	// execute GroupFinder program
	execute_GroupFinder(nfname, groupfinderoptions);
	// read the results of the GroupFinder program
	read_GroupFinder_results(nfname, TYPE_CITATION_NETWORK);
	fwprintf(logstream, L"\nCitation network clustering results:\n");
	fwprintf(logstream, L"\Partition\tIndex\tID\tAlias\n");
	for (i = 0; i < nnodes; i++)	// printf the clustering results, added 2014/05/15
		fwprintf(logstream, L"%d\t%d\t%s\t%s\n", nw[i].partition, i, nw[i].name, nw[i].alias);
	fwprintf(logstream, L"\n");
	struct CMEMBER *memb;	//  following codes are added 2014/05/14
	memb = (struct CMEMBER *)Jmalloc(nnodes * sizeof(struct CMEMBER), L"mainpath: memb");
	for (i = 0; i < nnodes; i++) { memb[i].id = i; memb[i].mcnt = 0; }
	for (i = 0; i < nnodes; i++) memb[nw[i].partition].mcnt++;
	qsort((void *)memb, (size_t)nnodes, sizeof(struct CMEMBER), compare_mcnt);

	//for (i = 0; i < nnodes; i++)
	//fwprintf(logstream, L"%d %d\n", i, memb[i].mcnt);
	//for (i = 0; i < nwos; i++) fwprintf(logstream, L"%s\n", wos[i].docid);

	for (i = 0; i < 20; i++)	// write out data for the top 10 clusters, changed to top 20 clusters, 2015/08/20
	{
		if (memb[i].mcnt <= 2) continue;	// ignore small clusters
		if (i >= nnodes) continue;
		// find the beginning and ending years for the cluster, this code block is added 2018/07/21
		byear = 2200; eyear = 1800;
		for (ii = 0; ii < nnodes; ii++)
		{
			if (nw[ii].partition == memb[i].id)
			{
				ndx = nw[ii].ndx2wos;
				if (wos[ndx].year <= byear) byear = wos[ndx].year;
				if (wos[ndx].year >  eyear) eyear = wos[ndx].year;
			}
		}
		// make a cluster directory, added 2015/08/20, added byear/eyear information, 2018/07/21
		swprintf(dirname, L"%s\Cluster%d-%d %d-%d", dpath, memb[i].id, memb[i].mcnt, byear, eyear);
		if (CreateDirectory(dirname, NULL) != 0)	// the directory does not exist
			fwprintf(logstream, L"Directory \"%s\" created successfully.\n\n", dirname);
		else
			fwprintf(logstream, L"Directory \"%s\" already exist.\n\n", dirname);
		if (full_record_type == WOS_DATA)
		{
			swprintf(tmpname, L"%s\\Cluster%d-%d %d-%d.txt", dirname, memb[i].id, memb[i].mcnt, byear, eyear);	// modified 2018/07/21
			readwrite_WOS(frecordname, tmpname, RW_PARTITION, memb[i].id);
		}
		else if (full_record_type == SCOPUS_DATA)
		{
			swprintf(tmpname, L"%s\\Cluster%d-%d %d-%d.csv", dirname, memb[i].id, memb[i].mcnt, byear, eyear);	// modified 2018/07/21
			readwrite_Scopus(frecordname, tmpname, RW_PARTITION, memb[i].id);
		}
		else if (full_record_type == THOMSON_INNOVATION_DATA)
		{
			swprintf(tmpname, L"%s\\Cluster%d-%d %d-%d.csv", dirname, memb[i].id, memb[i].mcnt, byear, eyear);	// modified 2018/07/21
			readwrite_TIP(frecordname, tmpname, RW_PARTITION, memb[i].id);
		}
		else if (full_record_type == WEBPAT2_DATA)
		{
			swprintf(tmpname, L"%s\\Cluster%d-%d %d-%d.csv", dirname, memb[i].id, memb[i].mcnt, byear, eyear);	// modified 2018/07/21
			readwrite_WEBPAT2(frecordname, tmpname, RW_PARTITION, memb[i].id);
		}
		else if (full_record_type == WEBPAT3_DATA)
		{
			swprintf(tmpname, L"%s\\Cluster%d-%d %d-%d.csv", dirname, memb[i].id, memb[i].mcnt, byear, eyear);	// modified 2018/07/21
			readwrite_WEBPAT3(frecordname, tmpname, RW_PARTITION, memb[i].id);
		}
		else if (full_record_type == USPTO_DATA)
		{
			swprintf(tmpname, L"%s\\Cluster%d-%d %d-%d.txt", dirname, memb[i].id, memb[i].mcnt, byear, eyear);	// modified 2018/07/21
			readwrite_USPTO(frecordname, tmpname, RW_PARTITION, memb[i].id);
		}
		else if (full_record_type == TAIWAN_TD_DATA)	// added 2016/03/19
		{
			swprintf(tmpname, L"%s\\Cluster%d-%d %d-%d.txt", dirname, memb[i].id, memb[i].mcnt, byear, eyear);	// modified 2018/07/21
			readwrite_TaiwanTD(frecordname, tmpname, RW_PARTITION, memb[i].id);
		}
		else if (full_record_type == TCI_DATA)	// added 2016/04/23
		{
			swprintf(tmpname, L"%s\\Cluster%d-%d %d-%d.csv", dirname, memb[i].id, memb[i].mcnt, byear, eyear);	// modified 2018/07/21
			readwrite_TCI(frecordname, tmpname, RW_PARTITION, memb[i].id);
		}
		else if (full_record_type == LAWSNOTE_DATA)	// added 2017/05/05
		{
			swprintf(tmpname, L"%s\\Cluster%d-%d %d-%d.csv", dirname, memb[i].id, memb[i].mcnt, byear, eyear);	// modified 2018/07/21
			readwrite_LAWSNOTE(frecordname, tmpname, RW_PARTITION, memb[i].id);
		}
		// also copy the citation file to the directory, added 2015/08/20
		wcscpy(tmps, citationname);
		sp = tmps; while (*sp != '\0') sp++;	// move to the end of the file name
		sp -= 4; *sp ='\0';	// ignore the ".paj." at the end
		swprintf(citname, L"%s.txt", tmps);	// restore the original citation file name
		wcscpy(tmps, citname);
		sp = tmps; while (*sp != '\0') sp++;	// move to the end of the file name
		while (*sp != '\\') sp--;				// move back to the begining of the citation file name
		swprintf(destname, L"%s%s", dirname, sp);	// restore the original citation file name
		ret = CopyFile(citname, destname, FALSE);
		if (ret == 0) fwprintf(logstream, L"\nFailed to copy file [%s]==>[%s].\n\n", citname, destname);
		else fwprintf(logstream, L"\nSuccessfully copied file [%s]==>[]%s.\n\n", citname, destname);
	}

	return 0;
}

//
// cluster the given coauthor network
//
int cluster_coauthor_network(wchar_t *nfname, int groupfinderoptions, wchar_t *dpath, wchar_t *frecordname, wchar_t *tname)
{
	int i, j, k, p, ret;
	wchar_t citname[FNAME_SIZE];
	wchar_t destname[FNAME_SIZE];
	wchar_t tmps[FNAME_SIZE], *sp;

	// execute GroupFinder program
	execute_GroupFinder(nfname, groupfinderoptions);
	// read the results of the GroupFinder program
	read_GroupFinder_results(nfname, TYPE_COAUTHOR_NETWORK);
	fwprintf(logstream, L"\nCoauthor network clustering results:\n");
	fwprintf(logstream, L"\GroupID\tAuthors\tIndex (all authors)\tIndex (1st authors)\n");
	for (i = 0, j = 0; i < naus; i++)	// print the clustering results
	{
		if (authors[i].cnt1 != 0)
		{
			fwprintf(logstream, L"%d\t%s\t%d\t%d\n", authors[i].groupid, authors[i].name, i, j);
			j++;
		}
	}
	fwprintf(logstream, L"\n");

	// sort according to author[].groupid
	qsort((void *)authors, (size_t)naus, sizeof(struct AUTHORS), compare_groupid);
	// 1st pass, find the number of groups
	k = 0; int cnt = 0; int prev_id = 0; 
	for (i = 0; i < naus; i++)	
	{
		if (authors[i].groupid != prev_id)
		{
			//fwprintf(logstream, L"Group %d, size = %d\n", j, cnt);
			prev_id = authors[i].groupid;
			cnt = 1; k++;
		}
		else 
			cnt++;
		if (authors[i].groupid == 999999)	// hit non-1st-authors
			break;
	}
	nrgroups = k;
	fwprintf(logstream, L"Number of research groups %d\n", nrgroups);
	rgroups = (struct RESEARCH_GROUP *)Jmalloc(nrgroups * sizeof(struct RESEARCH_GROUP), L"cluster_coauthor_network: rgroups");
	for (i = 0; i < nrgroups; i++)	// initialization 
	{
		rgroups[i].np = 0;
		rgroups[i].byear = 9999;
		rgroups[i].eyear = 0;
	}
	// 2nd pass, allocate memory for "scholars" of each research group
	k = 0; cnt = 0; prev_id = 0; 
	for (i = 0; i < naus; i++)	
	{
		if (authors[i].groupid != prev_id)
		{
			rgroups[k].nscholars = cnt;
			rgroups[k].scholars = (int *)Jmalloc(cnt * sizeof(int), L"cluster_coauthor_network: rgroups[k].scholars");	
			prev_id = authors[i].groupid;
			cnt = 1; k++;
		}
		else 
			cnt++;
		if (authors[i].groupid == 999999)	// hit non-1st-authors
			break;
	}
	// 3rd pass, save the details of group information to "rgroups"
	k = 0; cnt = 0; prev_id = 0; 
	for (i = 0; i < naus; i++)	
	{
		if (authors[i].groupid != prev_id)
		{
			//fwprintf(logstream, L"Group %d, size = %d\n", k, cnt); fflush(logstream);
			rgroups[k].nscholars = cnt;
			prev_id = authors[i].groupid;
			k++; cnt = 0;
			if (authors[i].groupid == 999999)	// hit non-1st-authors
				break;
			rgroups[k].scholars[cnt] = authors[i].ndx;	// .ndx has the index in original order
			cnt++;
		}
		else 
		{
			rgroups[k].scholars[cnt] = authors[i].ndx;	// .ndx has the index in original order
			cnt++;
		}
	}

	// sort back to the order of author[].name
	qsort((void *)authors, (size_t)naus, sizeof(struct AUTHORS), compare_author);

	// establish the author group linkages among group nodes (it does the same thing as shrink the coauthor network)
	shrink_coauthor_network(naus, authors, nrgroups, rgroups);	
	degree_centrality_rgroups(nrgroups, rgroups);
	weigted_degree_centrality_rgroups(nrgroups, rgroups);

	wchar_t tmpname[FNAME_SIZE];	
	wchar_t dirname[FNAME_SIZE];
	swprintf(dirname, L"%s\\Author Subnetworks", dpath);
	if (CreateDirectory(dirname, NULL) != 0)	// the directory does not exist
		fwprintf(logstream, L"Directory \"%s\" created successfully.\n\n", dirname);
	else
		fwprintf(logstream, L"Directory \"%s\" already exist.\n\n", dirname);

	swprintf(tmps, L"research groups");
	swprintf(dirname, L"%s\\Author Subnetworks", dpath);
	swprintf(tmpname, L"%s\\Research Group.paj", dirname);
	rgroup_to_Pajek_file(tmpname, nrgroups, rgroups, tmps);

#ifdef DEBUG
	for (i = 0; i < nrgroups; i++)
	{
		fwprintf(logstream, L"Group %d [%d]:", i, rgroups[i].nscholars);
		for (k = 0; k < rgroups[i].nscholars; k++)
			fwprintf(logstream, L" %d[%s]", rgroups[i].scholars[k], authors[rgroups[i].scholars[k]].name); fflush(logstream);
		fwprintf(logstream, L"\n");
	}
#endif DEBUG
	
	wchar_t ddirname[FNAME_SIZE];
	// extract, write out, and calculate the properties of the subnetwork network
	for (i = 0; i < nrgroups; i++)
	{
		extract_partition(i, rgroups, naus, authors);	// this function set the pointer rgroups[i].athrs and its contents
		int total_academic_age = 0;
		for (k = 0; k < rgroups[i].nscholars; k++)
		{
			rgroups[i].np += authors[rgroups[i].scholars[k]].np;	// sum the number of papers
			total_academic_age += (authors[rgroups[i].scholars[k]].eyear - authors[rgroups[i].scholars[k]].byear) + 10;
			if (authors[rgroups[i].scholars[k]].byear < rgroups[i].byear)
				rgroups[i].byear = authors[rgroups[i].scholars[k]].byear;
			if (authors[rgroups[i].scholars[k]].eyear > rgroups[i].eyear)
				rgroups[i].eyear = authors[rgroups[i].scholars[k]].eyear;
#ifdef MOVED_DOWN
			for (p = 0; p < authors[rgroups[i].scholars[k]].np; p++)	// added 2017/10/20, assign each WOS paper a group ID
			{
				wos[authors[rgroups[i].scholars[k]].paper[p]].partition = i;	// assign partition to the wos[] array
				if (wos[authors[rgroups[i].scholars[k]].paper[p]].ndx2nw != -1)
					nw[wos[authors[rgroups[i].scholars[k]].paper[p]].ndx2nw].partition = i;	// assign partition to the nw[] array 
			}
#endif MOVED_DOWN
		}		
		rgroups[i].average_academic_age = (double)total_academic_age / rgroups[i].nscholars;
		// do the following calculation only for groups of size greater than 5
		if (rgroups[i].nscholars < 5) continue;	 
		// codes for creating new directories are added 2017/10/20
		swprintf(ddirname, L"%s\\Author Subnetworks\\Group%d-%d", dpath, i, rgroups[i].nscholars);
		if (CreateDirectory(ddirname, NULL) != 0)	// the directory does not exist
			fwprintf(logstream, L"Directory \"%s\" created successfully.\n\n", ddirname);
		else
			fwprintf(logstream, L"Directory \"%s\" already exist.\n\n", ddirname);
		rgroups[i].density = network_density_author(rgroups[i].nscholars, rgroups[i].athrs);
		rgroups[i].centralization = degree_centralization_traditional_author(rgroups[i].nscholars, rgroups[i].athrs);
		rgroups[i].centralization2 = weighted_degree_centralization_author(rgroups[i].nscholars, rgroups[i].athrs);
		consolidate_group_coutries(i, rgroups, naus, authors);	// put together country informatio for the current group
		//fwprintf(logstream, L"density=%.6f, %d, %d\n", rgroups[i].density, rgroups[i].nscholars, rgroups[i].nlinks);
		swprintf(tmps, L"Partition %d of the coauthor network", i);
		swprintf(tmpname, L"%s\\Author subnetwork %d-%d.net", ddirname, i, rgroups[i].nscholars);
		author_to_Pajek_file(tmpname, rgroups[i].nscholars, rgroups[i].athrs, tmps);
	}

	for (i = 0; i < nrgroups; i++)	// added 2017/10/20, write out papers that are written by authors in certain groups
	{
		if (full_record_type == WOS_DATA)	// WOS_DATA only for now
		{
			// NOTE: it is necessary to reassign partition when operating on each author group, because a paper can belong to multiple author groups
			for (j = 0; j < nwos; j++) wos[j].partition = -1;	// initialize when operating on each group, added 2017/11/04
			for (j = 0; j < nnodes; j++) nw[j].partition = -1;	// initialize when operating on each group, added 2017/11/04
			for (k = 0; k < rgroups[i].nscholars; k++)
			{
				for (p = 0; p < authors[rgroups[i].scholars[k]].np; p++)	// added 2017/10/20, assign each WOS paper a group ID
				{
					wos[authors[rgroups[i].scholars[k]].paper[p]].partition = i;	// assign partition to the wos[] array
					if (wos[authors[rgroups[i].scholars[k]].paper[p]].ndx2nw != -1)
						nw[wos[authors[rgroups[i].scholars[k]].paper[p]].ndx2nw].partition = i;	// assign partition to the nw[] array 
				}
			}
			wchar_t pdataname[FNAME_SIZE];
			if (rgroups[i].nscholars < 5) continue;	 
			swprintf(ddirname, L"%s\\Author Subnetworks\\Group%d-%d", dpath, i, rgroups[i].nscholars);
			swprintf(pdataname, L"%s\\Papers%d-%d.txt", ddirname, i, rgroups[i].nscholars);
			readwrite_WOS_based_on_wos_array(frecordname, pdataname, RW_PARTITION, i);
		}
	}

	return 0;
}


//
// cluster the given coassignee network
// added 2017/03
//
int cluster_coassignee_network(wchar_t *nfname, int groupfinderoptions, wchar_t *dpath, wchar_t *frecordname, wchar_t *tname)
{
	int i, j, k, ret;
	wchar_t citname[FNAME_SIZE];
	wchar_t destname[FNAME_SIZE];
	wchar_t tmps[FNAME_SIZE], *sp;

	// execute GroupFinder program
	execute_GroupFinder(nfname, groupfinderoptions);
	// read the results of the GroupFinder program
	read_GroupFinder_results(nfname, TYPE_COASSIGNEE_NETWORK);
	fwprintf(logstream, L"\nCoassignee network clustering results:\n");
	fwprintf(logstream, L"\GroupID\tAssignees\tIndex (all assignees)\tIndex (1st assignees)\n");
	for (i = 0, j = 0; i < nasgns; i++)	// print the clustering results
	{
		if (assignees[i].cnt1 != 0)
		{
			fwprintf(logstream, L"%d\t%s\t%d\t%d\n", assignees[i].groupid, assignees[i].name, i, j);
			j++;
		}
	}
	fwprintf(logstream, L"\n");

	// sort according to assignees[].groupid
	qsort((void *)assignees, (size_t)nasgns, sizeof(struct ASSIGNEES), compare_asgn_groupid);
	// 1st pass, find the number of groups
	k = 0; int cnt = 0; int prev_id = 0; 
	for (i = 0; i < nasgns; i++)	
	{
		if (assignees[i].groupid != prev_id)
		{
			//fwprintf(logstream, L"Group %d, size = %d\n", j, cnt);
			prev_id = assignees[i].groupid;
			cnt = 1; k++;
		}
		else 
			cnt++;
		if (assignees[i].groupid == 999999)	// hit non-1st-authors
			break;
	}
	nrgroups = k;
	fwprintf(logstream, L"Number of assignee groups %d\n", nrgroups);
	rgroups = (struct RESEARCH_GROUP *)Jmalloc(nrgroups * sizeof(struct RESEARCH_GROUP), L"cluster_coassignee_network: rgroups");
	for (i = 0; i < nrgroups; i++)	// initialization 
	{
		rgroups[i].np = 0;
		rgroups[i].byear = 9999;
		rgroups[i].eyear = 0;
	}
	// 2nd pass, allocate memory for "assignees" of each research group
	k = 0; cnt = 0; prev_id = 0; 
	for (i = 0; i < nasgns; i++)	
	{
		if (assignees[i].groupid != prev_id)
		{
			rgroups[k].nscholars = cnt;
			rgroups[k].scholars = (int *)Jmalloc(cnt * sizeof(int), L"cluster_coassignee_network: rgroups[k].scholars");	
			prev_id = assignees[i].groupid;
			cnt = 1; k++;
		}
		else 
			cnt++;
		if (assignees[i].groupid == 999999)	// hit non-1st-assignees
			break;
	}
	// 3rd pass, save the details of group information to "rgroups"
	k = 0; cnt = 0; prev_id = 0; 
	for (i = 0; i < nasgns; i++)	
	{
		if (assignees[i].groupid != prev_id)
		{
			//fwprintf(logstream, L"Group %d, size = %d\n", k, cnt); fflush(logstream);
			rgroups[k].nscholars = cnt;
			prev_id = assignees[i].groupid;
			k++; cnt = 0;
			if (assignees[i].groupid == 999999)	// hit non-1st-assignees
				break;
			rgroups[k].scholars[cnt] = assignees[i].ndx;	// .ndx has the index in original order
			cnt++;
		}
		else 
		{
			rgroups[k].scholars[cnt] = assignees[i].ndx;	// .ndx has the index in original order
			cnt++;
		}
	}

	// sort back to the order of author[].name
	qsort((void *)assignees, (size_t)nasgns, sizeof(struct ASSIGNEES), compare_assignee);

	// establish the assignee group linkages among group nodes (it does the same thing as shrink the coassignee network)
	shrink_coassignee_network(nasgns, assignees, nrgroups, rgroups);	
	degree_centrality_rgroups(nrgroups, rgroups);
	weigted_degree_centrality_rgroups(nrgroups, rgroups);

	wchar_t tmpname[FNAME_SIZE];	
	wchar_t dirname[FNAME_SIZE];
	swprintf(dirname, L"%s\Assignee Subnetworks", dpath);
	if (CreateDirectory(dirname, NULL) != 0)	// the directory does not exist
		fwprintf(logstream, L"Directory \"%s\" created successfully.\n\n", dirname);
	else
		fwprintf(logstream, L"Directory \"%s\" already exist.\n\n", dirname);

	swprintf(tmps, L"assignee groups");
	swprintf(dirname, L"%s\Assignee Subnetworks", dpath);
	swprintf(tmpname, L"%s\\Assignee Group.paj", dirname);
	rgroup_to_Pajek_file(tmpname, nrgroups, rgroups, tmps);

#ifdef DEBUG
	for (i = 0; i < nrgroups; i++)
	{
		fwprintf(logstream, L"Group %d [%d]:", i, rgroups[i].nscholars);
		for (k = 0; k < rgroups[i].nscholars; k++)
			fwprintf(logstream, L" %d[%s]", rgroups[i].scholars[k], authors[rgroups[i].scholars[k]].name); fflush(logstream);
		fwprintf(logstream, L"\n");
	}
#endif DEBUG

	// extract, write out, and calculate the properties of the subnetwork network
	for (i = 0; i < nrgroups; i++)
	{
		extract_partition_assignee(i, rgroups, nasgns, assignees);	// this function set the pointer rgroups[i].athrs and its contents
		int total_academic_age = 0;
		for (k = 0; k < rgroups[i].nscholars; k++)
		{
			rgroups[i].np += authors[rgroups[i].scholars[k]].np;	// sum the number of papers
			total_academic_age += (assignees[rgroups[i].scholars[k]].eyear - assignees[rgroups[i].scholars[k]].byear) + 10;
			if (assignees[rgroups[i].scholars[k]].byear < rgroups[i].byear)
				rgroups[i].byear = assignees[rgroups[i].scholars[k]].byear;
			if (assignees[rgroups[i].scholars[k]].eyear > rgroups[i].eyear)
				rgroups[i].eyear = assignees[rgroups[i].scholars[k]].eyear;
		}
		rgroups[i].average_academic_age = (double)total_academic_age / rgroups[i].nscholars;
		// do the following calculation only for groups of size greater than 5
		if (rgroups[i].nscholars < 5) continue;	 
		rgroups[i].density = network_density_author(rgroups[i].nscholars, rgroups[i].athrs);
		rgroups[i].centralization = degree_centralization_traditional_author(rgroups[i].nscholars, rgroups[i].athrs);
		rgroups[i].centralization2 = weighted_degree_centralization_author(rgroups[i].nscholars, rgroups[i].athrs);
		consolidate_group_coutries_assignee(i, rgroups, nasgns, assignees);	// put together country informatio for the current group
		//fwprintf(logstream, L"density=%.6f, %d, %d\n", rgroups[i].density, rgroups[i].nscholars, rgroups[i].nlinks);
		swprintf(tmps, L"Partition %d of the coassignee network", i);
		swprintf(tmpname, L"%s\\Assignee subnetwork %d-%d.net", dirname, i, rgroups[i].nscholars);
		author_to_Pajek_file(tmpname, rgroups[i].nscholars, rgroups[i].athrs, tmps);
	}

	return 0;
}

//
// shrink the co-author network according to the partition
// this function actually establishes the linkage among group nodes
// NOTE: only 1st authors are considered
//
struct NEIGHBOURS
{
	int    ndx;		// the id of neighbors, index to this RESEARCH_GROUP list
	double weight;	// weight of this particular link
};
int shrink_coauthor_network(int naus, struct AUTHORS *authors, int nrgroups, struct RESEARCH_GROUP *rgroups)
{
	int i, j, k, ik, ikj, ii;
	int cntx, cnt1;
	double cnt2;
	int tempsize;
	struct NEIGHBOURS *temp, *temp2;

	tempsize = 5000;
	temp  = (struct NEIGHBOURS *)Jmalloc(tempsize * sizeof(struct NEIGHBOURS), L"shrink_coauthor_network: temp");
	temp2 = (struct NEIGHBOURS *)Jmalloc(tempsize * sizeof(struct NEIGHBOURS), L"shrink_coauthor_network: temp2");

	for (i = 0; i < nrgroups; i++)	// for all the groups
	{
		cntx = 0;
		for (k = 0; k < rgroups[i].nscholars; k++)	// for all the scholars in the group i
		{
			ik = rgroups[i].scholars[k];	
			for (j = 0; j < authors[ik].degree; j++)	// for all the neighbors of the scholar k in the group i
			{
				ikj = authors[ik].nbrs[j];
				//if (i == 6)
				//{
				//fwprintf(logstream, L"==>[%d %d %d %d] %d %f\n", i, k, j, ikj, authors[ikj].groupid, authors[ik].weight[j]);
				//fwprintf(logstream, L"%s->%s\n", authors[ik].name, authors[ikj].name);
				//}
				//if (authors[ikj].groupid != i && authors[ikj].cnt1 != 0)	// not in the same group and not 1st authors				
				// IMPORTANT NOTE: here self-authoring to the group is counted in, 
				// we do this because Pajek does the same, this makes it easier to comapre with the results of Pajek
				if (authors[ikj].cnt1 != 0)	// is a 1st author
				{
					if (authors[ikj].groupid == i && ik < ikj)	// if he neighbor is in the same group, take only the link in one direction to avoid duplcattion
						continue;
					temp[cntx].ndx = authors[ikj].groupid;
					temp[cntx].weight = authors[ik].weight[j];
					cntx++;
					//if (i == 6)
					//fwprintf(logstream, L"   [%d %d %d %d] %d %f\n", i, k, j, ikj, authors[ikj].groupid, authors[ik].weight[j]);
				}
			}
		}
		if (cntx == 0) continue;	// this group has no outside connections
		//if (i == 6)
		//{
		//for (ii = 0; ii < cntx; ii++)
		//	fwprintf(logstream, L"### %d %d=>%d, %f\n", ii, i+1, temp[ii].ndx+1, temp[ii].weight);
		//}

		// consolidate the neighbors
		qsort((struct NEIGHBOURS *)temp, (size_t)cntx, sizeof(struct NEIGHBOURS), compare_ndx);
		cnt1 = 0; cnt2 = 0.0;
		int prev = temp[0].ndx;
		for (ii = 0; ii < cntx; ii++)
		{
			if (temp[ii].ndx != prev)	// hit a new neighbour
			{
				temp2[cnt1].ndx = prev;
				temp2[cnt1].weight = cnt2;
				cnt2 = temp[ii].weight; cnt1++;
				prev = temp[ii].ndx;
			}
			else
				cnt2 += temp[ii].weight;
		}
		temp2[cnt1].ndx = prev;
		temp2[cnt1].weight = cnt2;
		rgroups[i].degree = cnt1 + 1;
		// copy the results over
		rgroups[i].nbrs = (int *)Jmalloc(rgroups[i].degree * sizeof(int), L"shrink_coauthor_network: rgroups[i].nbrs");
		rgroups[i].weight = (double *)Jmalloc(rgroups[i].degree * sizeof(double), L"shrink_coauthor_network: rgroups[i].weight");
		for (ii = 0; ii < rgroups[i].degree; ii++)
		{
			rgroups[i].nbrs[ii] = temp2[ii].ndx;
			rgroups[i].weight[ii] = temp2[ii].weight;
			//if (i == 6)
			//fwprintf(logstream, L"==>   %d[%d, %f]\n", i, rgroups[i].nbrs[ii], rgroups[i].weight[ii]);
		}
	}

	Jfree(temp, L"shrink_coauthor_network: temp");
	Jfree(temp2, L"shrink_coauthor_network: temp2");

	return 0;
}

//
// shrink the co-assignee network according to the partition
// this function actually establishes the linkage among group nodes
// NOTE: all assignees are considered
//
int shrink_coassignee_network(int naus, struct ASSIGNEES *assignees, int nrgroups, struct RESEARCH_GROUP *rgroups)
{
	int i, j, k, ik, ikj, ii;
	int cntx, cnt1;
	double cnt2;
	int tempsize;
	struct NEIGHBOURS *temp, *temp2;

	tempsize = 5000;
	temp  = (struct NEIGHBOURS *)Jmalloc(tempsize * sizeof(struct NEIGHBOURS), L"shrink_coassignee_network: temp");
	temp2 = (struct NEIGHBOURS *)Jmalloc(tempsize * sizeof(struct NEIGHBOURS), L"shrink_coassignee_network: temp2");

	for (i = 0; i < nrgroups; i++)	// for all the groups
	{
		cntx = 0;
		for (k = 0; k < rgroups[i].nscholars; k++)	// for all the scholars in the group i
		{
			ik = rgroups[i].scholars[k];	
			for (j = 0; j < assignees[ik].degree; j++)	// for all the neighbors of the scholar k in the group i
			{
				ikj = assignees[ik].nbrs[j];		
				// IMPORTANT NOTE: here self-authoring to the group is counted in, 
				// we do this because Pajek does the same, this makes it easier to comapre with the results of Pajek
				if (assignees[ikj].groupid == i && ik < ikj)	// if the neighbor is in the same group, take only the link in one direction to avoid duplcattion
					continue;
				temp[cntx].ndx = assignees[ikj].groupid;
				temp[cntx].weight = assignees[ik].weight[j];
				cntx++;
			}
		}
		if (cntx == 0) continue;	// this group has no outside connections

		// consolidate the neighbors
		qsort((struct NEIGHBOURS *)temp, (size_t)cntx, sizeof(struct NEIGHBOURS), compare_ndx);
		cnt1 = 0; cnt2 = 0.0;
		int prev = temp[0].ndx;
		for (ii = 0; ii < cntx; ii++)
		{
			if (temp[ii].ndx != prev)	// hit a new neighbour
			{
				temp2[cnt1].ndx = prev;
				temp2[cnt1].weight = cnt2;
				cnt2 = temp[ii].weight; cnt1++;
				prev = temp[ii].ndx;
			}
			else
				cnt2 += temp[ii].weight;
		}
		temp2[cnt1].ndx = prev;
		temp2[cnt1].weight = cnt2;
		rgroups[i].degree = cnt1 + 1;
		// copy the results over
		rgroups[i].nbrs = (int *)Jmalloc(rgroups[i].degree * sizeof(int), L"shrink_coassignee_network: rgroups[i].nbrs");
		rgroups[i].weight = (double *)Jmalloc(rgroups[i].degree * sizeof(double), L"shrink_coassignee_network: rgroups[i].weight");
		for (ii = 0; ii < rgroups[i].degree; ii++)
		{
			rgroups[i].nbrs[ii] = temp2[ii].ndx;
			rgroups[i].weight[ii] = temp2[ii].weight;
		}
	}

	Jfree(temp, L"shrink_coassignee_network: temp");
	Jfree(temp2, L"shrink_coassignee_network: temp2");

	return 0;
}

//
// shrink the author citation network according to the partition
// this function actually establishes the linkage among group nodes
// NOTE: only 1st authors are considerate, actually, the linkage information are available only among the 1st authors
//
int shrink_author_citation_network(int naus, struct AUTHORS *authors, int nrgroups, struct RESEARCH_GROUP *rgroups)
{
	int i, j, k, ik, ikj, ii;
	int cntx, cnt1;
	double cnt2;
	int tempsize;
	struct NEIGHBOURS *temp, *temp2;

	tempsize = naus * 10;	// this is an estimation
	temp  = (struct NEIGHBOURS *)Jmalloc(tempsize * sizeof(struct NEIGHBOURS), L"shrink_author_citation_network: temp");
	temp2 = (struct NEIGHBOURS *)Jmalloc(tempsize * sizeof(struct NEIGHBOURS), L"shrink_author_citation_network: temp2");
	//for (i = 0; i < tempsize; i++) temp[i].weight = 0.0;
	
	// work on inward citation links
	for (i = 0; i < nrgroups; i++)	// for all the groups
	{
		cntx = 0;
		for (k = 0; k < rgroups[i].nscholars; k++)	// for all the scholars in the group i
		{
			ik = rgroups[i].scholars[k];
			for (j = 0; j < authors[ik].ctn_indegree; j++)	// for all the neighbors of the scholar k in the group i
			{
				ikj = authors[ik].ctn_innbrs[j];
				//if (authors[ikj].groupid != i && authors[ikj].cnt1 != 0)	// not in the same group and is the 1st author
				// IMPORTANT NOTE: here self-citation to the group is counted in, 
				// we do this because Pajek does the same, this makes it easier to comapre with the results of Pajek
				if (authors[ikj].cnt1 != 0)	// is the 1st author
				{
					temp[cntx].ndx = authors[ikj].groupid;
					temp[cntx].weight = authors[ik].ctn_inweight[j];
					cntx++;
				}
			}
		}
		if (cntx == 0) continue;	// this group has no inward connections
		// consolidate the inward neighbors
		qsort((struct NEIGHBOURS *)temp, (size_t)cntx, sizeof(struct NEIGHBOURS), compare_ndx);
		cnt1 = 0; cnt2 = 0.0;
		int prev = temp[0].ndx;
		for (ii = 0; ii < cntx; ii++)
		{
			if (temp[ii].ndx != prev)	// hit a new neighbour
			{
				temp2[cnt1].ndx = prev;
				temp2[cnt1].weight = cnt2;
				cnt2 = temp[ii].weight; cnt1++;
				prev = temp[ii].ndx;
			}
			else
				cnt2 += temp[ii].weight;
		}
		temp2[cnt1].ndx = prev;
		temp2[cnt1].weight = cnt2;
		rgroups[i].ctn_indegree = cnt1 + 1;
		// copy the results over
		rgroups[i].ctn_innbrs = (int *)Jmalloc(rgroups[i].ctn_indegree * sizeof(int), L"shrink_author_citation_network: rgroups[i].ctn_innbrs");
		rgroups[i].ctn_inweight = (double *)Jmalloc(rgroups[i].ctn_indegree * sizeof(double), L"shrink_author_citation_network: rgroups[i].ctn_inweight");
		for (ii = 0; ii < rgroups[i].ctn_indegree; ii++)
		{
			rgroups[i].ctn_innbrs[ii] = temp2[ii].ndx;
			rgroups[i].ctn_inweight[ii] = temp2[ii].weight;;
		}
	}
	// work on outward citation links
	for (i = 0; i < nrgroups; i++)	// for all the groups
	{
		cntx = 0;
		for (k = 0; k < rgroups[i].nscholars; k++)	// for all the scholars in the group i
		{
			ik = rgroups[i].scholars[k];	
			for (j = 0; j < authors[ik].ctn_outdegree; j++)	// for all the neighbors of the scholar k in the group i
			{
				ikj = authors[ik].ctn_outnbrs[j];
				//if (authors[ikj].groupid != i && authors[ikj].cnt1 != 0)	// not in the same group and not 1st authors
				// IMPORTANT NOTE: here self-citation to the group is counted in, 
				// we do this because Pajek does the same, this makes it easier to comapre with the results of Pajek
				if (authors[ikj].cnt1 != 0)	// not in the same group and not 1st authors
				{
					temp[cntx].ndx = authors[ikj].groupid;
					temp[cntx].weight = authors[ik].ctn_outweight[j];
					cntx++;
				}
			}
		}
		if (cntx == 0) continue;	// this group has no outward connections
		// consolidate the outward neighbors
		qsort((struct NEIGHBOURS *)temp, (size_t)cntx, sizeof(struct NEIGHBOURS), compare_ndx);
		cnt1 = 0; cnt2 = 0.0;
		int prev = temp[0].ndx;
		for (ii = 0; ii < cntx; ii++)
		{
			if (temp[ii].ndx != prev)	// hit a new neighbour
			{
				temp2[cnt1].ndx = prev;
				temp2[cnt1].weight = cnt2;
				cnt2 = temp[ii].weight; cnt1++;
				prev = temp[ii].ndx;
			}
			else
				cnt2 += temp[ii].weight;
		}
		temp2[cnt1].ndx = prev;
		temp2[cnt1].weight = cnt2;
		rgroups[i].ctn_outdegree = cnt1 + 1;
		// copy the results over
		rgroups[i].ctn_outnbrs = (int *)Jmalloc(rgroups[i].ctn_outdegree * sizeof(int), L"shrink_author_citation_network: rgroups[i].ctn_outnbrs");
		rgroups[i].ctn_outweight = (double *)Jmalloc(rgroups[i].ctn_outdegree * sizeof(double), L"shrink_author_citation_network: rgroups[i].ctn_outweight");
		for (ii = 0; ii < rgroups[i].ctn_outdegree; ii++)
		{
			rgroups[i].ctn_outnbrs[ii] = temp2[ii].ndx;
			rgroups[i].ctn_outweight[ii] = temp2[ii].weight;;
		}
	}

	Jfree(temp, L"shrink_author_citation_network: temp");
	Jfree(temp2, L"shrink_author_citation_network: temp2");

	return 0;
}

//
// cluster the given document projection of a two-mode coword network
//
int cluster_coword_doc_network(wchar_t *nfname, int groupfinderoptions, wchar_t *dpath, wchar_t *frecordname, wchar_t *tname)
{
	int i, ret;
	wchar_t tmpname[FNAME_SIZE];
	wchar_t dirname[FNAME_SIZE];
	wchar_t citname[FNAME_SIZE];
	wchar_t destname[FNAME_SIZE];
	wchar_t tmps[FNAME_SIZE], *sp;

	// execute GroupFinder program
	execute_GroupFinder(nfname, groupfinderoptions);
	// read the results of the GroupFinder program
	read_GroupFinder_results(nfname, TYPE_COWORD_DOC_NETWORK);
	fwprintf(logstream, L"\nCoword (document projection) network clustering results:\n");
	fwprintf(logstream, L"\Partition\tIndex\tAlias\tDocID\n");
	for (i = 0; i < nwos; i++)	// printf the clustering results
		fwprintf(logstream, L"%d\t%d\t%s\t%s\n", wos[i].partition, i, wos[i].alias, wos[i].docid);
	fwprintf(logstream, L"\n");
	struct CMEMBER *memb;	//  following codes are added 2014/05/14
	memb = (struct CMEMBER *)Jmalloc(nwos * sizeof(struct CMEMBER), L"mainpath: memb");
	for (i = 0; i < nwos; i++) { memb[i].id = i; memb[i].mcnt = 0; }
	for (i = 0; i < nwos; i++) memb[wos[i].partition].mcnt++;
	qsort((void *)memb, (size_t)nwos, sizeof(struct CMEMBER), compare_mcnt);

	//for (i = 0; i < nnodes; i++)
	//fwprintf(logstream, L"%d %d\n", i, memb[i].mcnt);
	//for (i = 0; i < nwos; i++) fwprintf(logstream, L"%s\n", wos[i].docid);

	for (i = 0; i < 20; i++)	// write out data for the top 20 clusters
	{
		if (memb[i].mcnt <= 2) continue;	// ignore small clusters
		if (i >= nwos) continue;
		// make a cluster directory	
		swprintf(dirname, L"%s\Cluster%d-%d", dpath, memb[i].id, memb[i].mcnt);
		if (CreateDirectory(dirname, NULL) != 0)	// the directory does not exist
			fwprintf(logstream, L"Directory \"%s\" created successfully.\n\n", dirname);
		else
			fwprintf(logstream, L"Directory \"%s\" already exist.\n\n", dirname);

		if (full_record_type == SCOPUS_DATA)
		{
			swprintf(tmpname, L"%s\\Cluster%d-%d.csv", dirname, memb[i].id, memb[i].mcnt);	
			readwrite_Scopus_direct(frecordname, tmpname, RW_PARTITION, memb[i].id);
		}
		else if (full_record_type == TCI_DATA)
		{
			swprintf(tmpname, L"%s\\Cluster%d-%d.csv", dirname, memb[i].id, memb[i].mcnt);	
			readwrite_TCI_direct(frecordname, tmpname, RW_PARTITION, memb[i].id);
		}
#ifdef XXX
		if (full_record_type == WOS_DATA)
		{ 
			swprintf(tmpname, L"%s\\Cluster%d-%d.txt", dirname, memb[i].id, memb[i].mcnt);
			readwrite_WOS(frecordname, tmpname, RW_PARTITION, memb[i].id);
		}
		else if (full_record_type == SCOPUS_DATA)
		{
			swprintf(tmpname, L"%s\\Cluster%d-%d.csv", dirname, memb[i].id, memb[i].mcnt);	// fixed problem 2016/01/11
			readwrite_Scopus_direct(frecordname, tmpname, RW_PARTITION, memb[i].id);
		}
		else if (full_record_type == THOMSON_INNOVATION_DATA)
		{
			swprintf(tmpname, L"%s\\Cluster%d-%d.csv", dirname, memb[i].id, memb[i].mcnt);	// fixed problem 2016/01/11
			readwrite_TIP(frecordname, tmpname, RW_PARTITION, memb[i].id);
		}
		else if (full_record_type == WEBPAT2_DATA)
		{
			swprintf(tmpname, L"%s\\Cluster%d-%d.csv", dirname, memb[i].id, memb[i].mcnt);	// fixed problem 2016/01/11
			readwrite_WEBPAT2(frecordname, tmpname, RW_PARTITION, memb[i].id);
		}
		else if (full_record_type == USPTO_DATA)
		{
			swprintf(tmpname, L"%s\\Cluster%d-%d.txt", dirname, memb[i].id, memb[i].mcnt);	// fixed problem 2016/01/11
			readwrite_USPTO(frecordname, tmpname, RW_PARTITION, memb[i].id);
		}
		else if (full_record_type == TAIWAN_TD_DATA)
		{
			swprintf(tmpname, L"%s\\Cluster%d-%d.txt", dirname, memb[i].id, memb[i].mcnt);
			readwrite_TaiwanTD(frecordname, tmpname, RW_PARTITION, memb[i].id);
		}
		else if (full_record_type == TCI_DATA)
		{
			swprintf(tmpname, L"%s\\Cluster%d-%d.csv", dirname, memb[i].id, memb[i].mcnt);	
			readwrite_TCI(frecordname, tmpname, RW_PARTITION, memb[i].id);
		}
#endif
	}

	return 0;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_pnaliasname(const void *n1, const void *n2)
{
	struct PN *t1, *t2;
	
	t1 = (struct PN *)n1;
	t2 = (struct PN *)n2;
	if (wcscmp(t2->alias, t1->alias) < 0)
		return 1;
	else if (wcscmp(t2->alias, t1->alias) == 0)
		return 0;
	else return -1;
}

//
// use binary search to find the proper position of an alias name in a PN array
//
int aliasname_search(struct PN d[], int num, wchar_t *str)
{
	int low, high, cur;

	if (wcscmp(str, d[0].alias) < 0)
		return -1;
	low = 0;
	high = num;
	for (;;)
	{
		cur = (low + high) / 2;
		if (wcscmp(str, d[cur].alias) == 0)
			return cur;
		if (cur == low)
		{
			if (wcscmp(str, d[high].alias) == 0)
				return high;
			else 
				return -1;
		}
		else if (wcscmp(str, d[cur].alias) > 0)
			low = cur;
		else
			high = cur;
	}
}

//
// this fucntion is to be called by qsort() only
// 
int compare_mcnt(const void *n1, const void *n2)
{
	struct CMEMBER *t1, *t2;
	
	t1 = (struct CMEMBER *)n1;
	t2 = (struct CMEMBER *)n2;
	if (t2->mcnt > t1->mcnt)
		return 1;
	else if (t2->mcnt == t1->mcnt)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_groupid(const void *n1, const void *n2)
{
	struct AUTHORS *t1, *t2;
	
	t1 = (struct AUTHORS *)n1;
	t2 = (struct AUTHORS *)n2;
	if (t2->groupid < t1->groupid)
		return 1;
	else if (t2->groupid == t1->groupid)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_asgn_groupid(const void *n1, const void *n2)
{
	struct ASSIGNEES *t1, *t2;
	
	t1 = (struct ASSIGNEES *)n1;
	t2 = (struct ASSIGNEES *)n2;
	if (t2->groupid < t1->groupid)
		return 1;
	else if (t2->groupid == t1->groupid)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_ndx(const void *n1, const void *n2)
{
	struct NEIGHBOURS *t1, *t2;
	
	t1 = (struct NEIGHBOURS *)n1;
	t2 = (struct NEIGHBOURS *)n2;
	if (t2->ndx < t1->ndx)
		return 1;
	else if (t2->ndx == t1->ndx)
		return 0;
	else return -1;
}