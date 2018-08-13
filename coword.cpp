//
// coword.cpp
//

//
// Revision History:
// 
// 2015/06/12 basic function works
// 2016/01/25 changed the file opening to Unicode mode (ccs=UTF-8)
// 2016/07/xx added new function coword(), this function replaces the function prepare_coword_data()
//            => coword() writes coword network into a Pajek file
//            => prepare_coword_data() writes only a relationship-list file
// 2016/07/18 added to return immediately (do nothing) if "Keyword report.txt" file is not provided, added 2016/07/18
// 2016/09/24 enlarged the allocation size of kwp[]
// 2017/12/23 enlarged the allocation size of kwp[] again, and
//              stop providing document projection file for large file
//
// 

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "resource.h"
#include "network.h"

int compare_kwid(const void *, const void *);
int compare_kwp(const void *, const void *);
int compare_dcp(const void *, const void *);

extern FILE *logstream;
extern int nwos;
extern struct WOS *wos;
extern struct TKEYWORDS *tword; 
extern int nkreport;	// number of keywords in the "Keyword report.txt" file

extern struct TKEYWORDS *tkeyword;	// title keywords
extern int ntkeywords;

wchar_t coworddocfilename[FNAME_SIZE];	// this is a dirty global, be careful!

extern bool is_in_keyword_report(wchar_t *kname);


struct KWBUF 
{
	int id;
};


#define KWPSIZE 14
struct KWPAIR 
{
	wchar_t names[KWPSIZE];	// keyword pair
};

#define DCPSIZE 14
struct DCPAIR 
{
	wchar_t names[DCPSIZE];	// document pair
};


struct RKEYWORD	// keyword report keywords
{
	int ndx;	// index to the keyword-report keyword array
	int ndocs;	// number of documents that include this keyword
	int *docs;	// array of document ids
};

//
// assemble co-word data and write the result into a Pajek file
// two Pajek files are created (row and column projections of the two-mode coword network)
//
int coword(wchar_t *fname)
{
	int i, j, k, m;
	int ndx1, ndx2;				
	struct KWPAIR *kwp;		
	struct DCPAIR *dcp;
	struct RKEYWORD *rkeyword;
	int nkwp;	// number of keyword pairs
	int ndcp;	// number of document pairs
	int cnt;
	int aj, ak;
	int raj, rak;
	wchar_t prev_name[KWPSIZE];
	FILE *ostream;

	if (nkreport == 0) return 0;	// do nothing if "Keyword report.txt" file is not provided, added 2016/07/18
//
// 1st step, create network among words
//
	kwp = (struct KWPAIR *)Jmalloc((nkreport * nkreport * nwos / 2) * sizeof(struct KWPAIR), L"coword: kwp"); // the size is an estimation, enlarged 2016/09/24, 2017/12/23
	if (kwp == NULL) return MSG_NOT_ENOUGH_MEMORY;

	k = 0;
	m = 0;	// total number of keyword pairs
	for (i = 0; i < nwos; i++)
	{
		for (j = 0; j < wos[i].ntkws; j++)
		{
			for (k = j + 1; k < wos[i].ntkws; k++)
			{
				aj = wos[i].tkws[j];
				ak = wos[i].tkws[k];
				if (aj != ak)	// ignore the strange situation of two same keywords in a paper
				{
					raj = tkeyword[aj].cross_ndx; 
					rak = tkeyword[ak].cross_ndx; 
					if (raj != -1 && rak != -1)
					{
						//fwprintf(logstream, L"#####[%s] [%s]\n", tword[raj].name, tword[rak].name); fflush(logstream);
						if (aj <= ak)	
						{
							//fwprintf(logstream, L"%d &&&&& %06d %06d\n", m, raj+1, rak+1); fflush(logstream);
							swprintf(kwp[m].names, L"%06d %06d", raj+1, rak+1);
						}
						else
						{
							//fwprintf(logstream, L"%d ##### %06d %06d\n", m, rak+1, raj+1); fflush(logstream);
							swprintf(kwp[m].names, L"%06d %06d", rak+1, raj+1);
						}
						m++;
					}
				}
			}
		}
	}

	fwprintf(logstream, L"Total number of keyword pairs= %d\n", m);
	nkwp = m;

	qsort((void *)kwp, (size_t)nkwp, sizeof(struct KWPAIR), compare_kwp);

	// prepare the name for the output files
	wchar_t cname[FNAME_SIZE];	wchar_t oname[FNAME_SIZE]; 
	wchar_t cname2[FNAME_SIZE]; wchar_t oname2[FNAME_SIZE];
	wchar_t tname[FNAME_SIZE], *sp, *tp;
	int backslash;	
	wcscpy(cname, L"Co-word word.paj");
	wcscpy(cname2, L"Co-word document.paj");
	sp = fname; tp = tname; backslash = 0;
	while (*sp != L'\0') { if (*sp == '\\') backslash++; *tp++ = *sp++; }	// go to the end of the line, and check if there is backslashes in the name
	if (backslash == 0) // no backslash in name
	{
		swprintf_s(oname, FNAME_SIZE, cname);	
		swprintf_s(oname2, FNAME_SIZE, cname2);	
	}
	else	// names in long format
	{
		*tp = '\0';
		while (*sp != L'\\') { sp--; tp--; }	// trace back to the last backslash
		*tp = '\0';
		swprintf_s(oname, FNAME_SIZE, L"%s\\%s", tname, cname);
		swprintf_s(oname2, FNAME_SIZE, L"%s\\%s", tname, cname2);
	}
	//wcscpy(cowordfilename, oname);	// this will be used by the function cluster_coword_network()

	// open the output file
	if (_wfopen_s(&ostream, oname, L"wt, ccs=UTF-8") != 0)	
		return MSG_OFILE_CANNOTOPEN;

	fwprintf(ostream, L"*Network Co-word (word projection)\n"); 
	// write out the "*vertice" line 
	// write vertice list, Pajek requires that first vertex is indexed in "1" rather than "0"
	fwprintf(ostream, L"*Vertices %d\n", nkreport);
	for (i = 0; i < nkreport; i++) fwprintf(ostream, L"%d \"%s\"\n", i+1, tword[i].name);

	fwprintf(ostream, L"*Edges\n");

	// consolidate duplicate keyword pairs, write to output file at the same time
	prev_name[0] = '\0';
	k = 0; cnt = 1;
	for (i = 0; i < nkwp; i++)
	{
		if (wcscmp(kwp[i].names, prev_name) != 0)	// hit a new name
		{
			if (k > 0) 
			{
				fwprintf(ostream, L"%s %d\n", prev_name, cnt);	
				swscanf(prev_name, L"%d %d", &ndx1, &ndx2);		
				//link_kword_nodes_coword(first_authors[ndx1-1], first_authors[ndx2-1], cnt);	
			}
			wcscpy_s(prev_name, KWPSIZE, kwp[i].names); 
			cnt = 1; k++;
		}
		else
			cnt++;
	}	
	if (prev_name[0] != '\0')	// added the check for prev_name[0]
	{
		fwprintf(ostream, L"%s %d\n", prev_name, cnt);
		swscanf(prev_name, L"%d %d", &ndx1, &ndx2);	
		//link_kword_nodes_coword(first_authors[ndx1-1], first_authors[ndx2-1], cnt);
	}

	fclose(ostream);
	Jfree(kwp, L"coword: kwp");

//
// 2nd step, create network among documents
//
	// need to establish an keyword array that points back to documents
	rkeyword = (struct RKEYWORD *)Jmalloc(nkreport * sizeof(struct RKEYWORD), L"coword: rkeyword"); 	
	if (rkeyword == NULL) return MSG_NOT_ENOUGH_MEMORY;
	for (i = 0; i < nkreport; i++) // initialization
	{ 
		rkeyword[i].ndx = i; 
		rkeyword[i].ndocs = 0; 
		rkeyword[i].docs = (int *)Jmalloc(nwos * sizeof(int), L"coword: rkeyword[i].docs");
	}	
	m = 0; 
	for (i = 0; i < nwos; i++)
	{
		for (j = 0; j < wos[i].ntkws; j++)
		{
			aj = wos[i].tkws[j];
			raj = tkeyword[aj].cross_ndx;	// raj is index in the tword[] array
			if (raj != -1)	// takek only those keywords listed in the "Keyword report.txt"
			{
				rkeyword[raj].docs[rkeyword[raj].ndocs] = i;
				rkeyword[raj].ndocs++;
			}
		}
	}

#ifdef DEBUG
	for (i = 0; i < nkreport; i++)
	{
		fwprintf(logstream, L"\n-----%d\t%d [%s]\n", i, rkeyword[i].ndocs, tword[rkeyword[i].ndx].name);
		for (j = 0; j < rkeyword[i].ndocs; j++)
			fwprintf(logstream, L"%s\t", wos[rkeyword[i].docs[j]].alias);
	}
#endif DEBUG

	if (nwos > 2000) return 0;	// do not provide document projection file for large file, added 2017/12/23

	dcp = (struct DCPAIR *)Jmalloc((nkreport * nwos * 50) * sizeof(struct DCPAIR), L"coword: dcp"); // the size is an estimation, it can be huge for large network	
	if (dcp == NULL) return MSG_NOT_ENOUGH_MEMORY;
	k = 0;
	m = 0;	// total number of document pairs
	for (i = 0; i < nkreport; i++)
	{
		for (j = 0; j < rkeyword[i].ndocs; j++)
		{
			for (k = j + 1; k < rkeyword[i].ndocs; k++)
			{
				aj = rkeyword[i].docs[j];
				ak = rkeyword[i].docs[k];
				if (aj != ak)	// ignore the strange situation of two same documents in a keyword
				{
					//fwprintf(logstream, L"@@@@@%d\t%d\t%d\t%d\t%d\t[%s] [%s]\n", i, j, k, aj, ak, wos[aj].alias, wos[ak].alias); fflush(logstream);
					if (aj <= ak)	
						swprintf(dcp[m].names, L"%06d %06d", aj+1, ak+1);
					else
						swprintf(dcp[m].names, L"%06d %06d", ak+1, aj+1);
					m++;
				}
			}
		}
	}

	fwprintf(logstream, L"Total number of document pairs= %d\n", m);
	ndcp = m;

	qsort((void *)dcp, (size_t)ndcp, sizeof(struct DCPAIR), compare_dcp);

	wcscpy(coworddocfilename, oname2);	// this will be used by the function cluster_coword_doc_network()

	// open the output file
	if (_wfopen_s(&ostream, oname2, L"wt, ccs=UTF-8") != 0)	
		return MSG_OFILE_CANNOTOPEN;

	fwprintf(ostream, L"*Network Co-word (document projection)\n"); 
	// write out the "*vertice" line 
	// write vertice list, Pajek requires that first vertex is indexed in "1" rather than "0"
	fwprintf(ostream, L"*Vertices %d\n", nwos);
	for (i = 0; i < nwos; i++) fwprintf(ostream, L"%d \"%s\"\n", i+1, wos[i].alias);

	fwprintf(ostream, L"*Edges\n");

	// consolidate duplicate document pairs, write to output file at the same time
	prev_name[0] = '\0';
	k = 0; cnt = 1;
	for (i = 0; i < ndcp; i++)
	{
		if (wcscmp(dcp[i].names, prev_name) != 0)	// hit a new name
		{
			if (k > 0) 
			{
				fwprintf(ostream, L"%s %d\n", prev_name, cnt);	
				swscanf(prev_name, L"%d %d", &ndx1, &ndx2);		
				//link_kword_nodes_coword(first_authors[ndx1-1], first_authors[ndx2-1], cnt);	
			}
			wcscpy_s(prev_name, DCPSIZE, dcp[i].names); 
			cnt = 1; k++;
		}
		else
			cnt++;
	}	
	if (prev_name[0] != '\0')	// added the check for prev_name[0]
	{
		fwprintf(ostream, L"%s %d\n", prev_name, cnt);
		swscanf(prev_name, L"%d %d", &ndx1, &ndx2);	
		//link_kword_nodes_coword(first_authors[ndx1-1], first_authors[ndx2-1], cnt);
	}

	fclose(ostream);
	Jfree(dcp, L"coword: dcp");

	return 0;
}

//
// link two keyword nodes according to their co-word relationships
//
int link_kword_nodes_coword(int ndx1, int ndx2, double wt)
{
	int i;
	int degree;
	int *tmp1;
	double *tmp2;

#ifdef XXX
	// for node ndx1
	degree = words[ndx1].degree;
	tkeyword[ndx1].nbrs[degree] = ndx2;		// neighbor pointer
	tkeyword[ndx1].weight[degree] = wt;		// weight of this particular link
	tkeyword[ndx1].degree++;					// number of immediate neighbors 
	if (tkeyword[ndx1].degree == tkeyword[ndx1].cur_mem)	// used up the memory allocated
	{
		tmp1 = (int *)Jmalloc((tkeyword[ndx1].cur_mem + N_INCREMENT) * sizeof(int), L"link_word_nodes_coword: tmp1");
		tmp2 = (double *)Jmalloc((tkeyword[ndx1].cur_mem + N_INCREMENT) * sizeof(double), L"link_word_nodes_coword: tmp2");
		tkeyword[ndx1].cur_mem += N_INCREMENT;
		for (i = 0; i < tkeyword[ndx1].degree; i++) { tmp1[i] = tkeyword[ndx1].nbrs[i]; tmp2[i] = authors[ndx1].weight[i]; }
		Jfree(authors[ndx1].nbrs, L"link_word_nodes_coword: authors[ndx1].nbrs"); Jfree(authors[ndx1].weight, L"link_word_nodes_coword: tkeyword[ndx1].weight");
		tkeyword[ndx1].nbrs = tmp1; tkeyword[ndx1].weight = tmp2;
	}

	// for node ndx2
	degree = tkeyword[ndx2].degree;
	tkeyword[ndx2].nbrs[degree] = ndx1;		// neighbor pointer
	tkeyword[ndx2].weight[degree] = wt;		// weighting of this particular link
	tkeyword[ndx2].degree++;					// overall degree, number of immediate neighbors 
	if (tkeyword[ndx2].degree == tkeyword[ndx2].cur_mem)	// used up the memory allocated
	{
		tmp1 = (int *)Jmalloc((tkeyword[ndx2].cur_mem + N_INCREMENT) * sizeof(int), L"link_word_nodes_coauthor: tmp1");
		tmp2 = (double *)Jmalloc((tkeyword[ndx2].cur_mem + N_INCREMENT) * sizeof(double), L"link_word_nodes_coword: tmp2");
		tkeyword[ndx2].cur_mem += N_INCREMENT;
		for (i = 0; i < tkeyword[ndx2].degree; i++) { tmp1[i] = tkeyword[ndx2].nbrs[i]; tmp2[i] = authors[ndx2].weight[i]; }
		Jfree(authors[ndx2].nbrs, L"link_word_nodes_coword: authors[ndx2].nbrs"); Jfree(authors[ndx2].weight, L"link_word_nodes_coword: tkeyword[ndx2].weight");
		tkeyword[ndx2].nbrs = tmp1; tkeyword[ndx2].weight = tmp2;
	}
#endif XXX

	return 0;
}

//
// write keyword to a bipartite paper-keyword relationship file
// this function was used before coword() function is implemented
// coword() writes coword network into a Pajek file
// prepare_coword_data() writes only a relationship list file
//
int prepare_coword_data(wchar_t *dpath, int ntkeywords, struct TKEYWORDS *tkeyword)
{
	int i, k;
	int cnt, tcnt, nkw;
	FILE *cwstream;
	wchar_t fname[FNAME_SIZE];
	struct KWBUF kbuf[MAX_TKEYWORDS];
	int prev;

	if (nkreport == 0) return 0;	// do nothing if "Keyword report.txt" file is not provided, added 2016/07/18

	swprintf(fname, FNAME_SIZE, L"%s Coword.txt", dpath);
	_wfopen_s(&cwstream, fname, L"wt, ccs=UTF-8");	// modified 2016/01/21

	for (i = 0; i < nwos; i++)
	{
		cnt = 0;
		for (k = 0; k < wos[i].ntkws; k++)
		{
			//fwprintf(logstream, L"***** %d\t%d\t%s\t%d\t%s\n", i, k, wos[i].alias, wos[i].tkws[k], tkeyword[wos[i].tkws[k]].name); fflush(logstream);
			if (is_in_keyword_report(tkeyword[wos[i].tkws[k]].name))
			{
				kbuf[cnt].id = wos[i].tkws[k];
				cnt++;
				//fwprintf(cwstream, L"%s\t%s\n", wos[i].alias, tkeyword[wos[i].tkws[k]].name); fflush(cwstream);
			}
		}
		if (cnt == 0) continue;	// no keywords that match those listed in "Keyword report.txt"
		qsort((void *)kbuf, (size_t)cnt, sizeof(struct KWBUF), compare_kwid);	
		nkw = 0; tcnt = 1; prev = -1;
		for (k = 0; k < cnt; k++)
		{
			if (kbuf[k].id == prev)
				tcnt++;
			else
			{
				if (k != 0)
				{
					fwprintf(cwstream, L"%s\t%s\t%d\n", wos[i].alias, tkeyword[prev].name, tcnt);
					tcnt = 1; nkw++;
				}
			}
			prev = kbuf[k].id;
		}
		fwprintf(cwstream, L"%s\t%s\t%d\n", wos[i].alias, tkeyword[prev].name, tcnt);
	}

	fclose(cwstream);

	return 0;
}

//
// this fucntion is to be called by qsort() only
// NOTE: order from large to small
// 
int compare_kwid(const void *n1, const void *n2)
{
	struct KWBUF *t1, *t2;
	
	t1 = (struct KWBUF *)n1;
	t2 = (struct KWBUF *)n2;
	if (t2->id < t1->id)
		return 1;
	else if (t2->id == t1->id)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_kwp(const void *n1, const void *n2)
{
	struct KWPAIR *t1, *t2;
	
	t1 = (struct KWPAIR *)n1;
	t2 = (struct KWPAIR *)n2;
	if (wcscmp(t2->names, t1->names) < 0)
		return 1;
	else if (wcscmp(t2->names, t1->names) == 0)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_dcp(const void *n1, const void *n2)
{
	struct DCPAIR *t1, *t2;
	
	t1 = (struct DCPAIR *)n1;
	t2 = (struct DCPAIR *)n2;
	if (wcscmp(t2->names, t1->names) < 0)
		return 1;
	else if (wcscmp(t2->names, t1->names) == 0)
		return 0;
	else return -1;
}
