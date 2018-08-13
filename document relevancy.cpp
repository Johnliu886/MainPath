//
// document relevancy.cpp
//

//
// Revision History:
// 2015/10    Basic function works
// 2016/05/xx Added function Jaccard_distance_generlized()
// 2016/10/13 Added codes to support WEBPAT3 data	
// 2017/01/13 Corrected a relevancy calculation problem, in the function find_relevancy()
// 

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "resource.h"
#include "network.h"
#include "document relevancy.h"

int find_relevancy(int, int, struct PN *);
int compare_ipcv(const void *, const void *);

extern FILE *logstream;
extern int full_record_type;
extern int nnodes;
extern struct PN *nw;
extern int nwos;
extern struct WOS *wos;
extern int nuspto;
extern struct USPTO *uspto;
extern struct TKEYWORDS *tkeyword;	// array for title keywords (may also contain keywords in the abstract)
extern int ntkeywords;
extern struct TKEYWORDS *tword;	// array for keywords listed in the keyword report file
extern int nkreport;

extern double Jaccard_distance(int, PSTRING *, PSTRING *);
extern double Jaccard_distance_generalized(int, PSTRING *, PSTRING *);
extern int replace_IPC_codes(int, int, struct WOS *, struct USPTO *);
extern int replace_CPC4_codes(int, struct WOS *, struct USPTO *);

extern int compare_tkeyword(const void *, const void *);
extern int compare_wordcount(const void *, const void *);
extern int compare_tmpid(const void *, const void *);

extern double overlapping_distance(int, PSTRING *, PSTRING *);

extern bool is_in_keyword_report(wchar_t *);
extern int compare_indegree(const void *, const void *);
extern int compare_outdegree(const void *, const void *);

#ifdef XXX
//
// pool outward neighbors of the 2 given papers together and remove duplicates
//
int pool2_out_nbrs(int p1, int p2, int nnodes, struct PN *nw, int *n_outnbrs, struct NEIGHBOR *out_nbrs)
{
	int i, k;
	int ntmp;
	int cnt;
	struct NEIGHBOR *tmp;
	int prev;

	ntmp = 0;
	for (k = 0; k < nw[p1].out_deg; k++)	// paper 1
		tmp[ntmp++].id = nw[p1].out_nbrs[k];
	for (k = 0; k < nw[p2].out_deg; k++)	// paper 2
		tmp[ntmp++].id = nw[p2].out_nbrs[k];

	qsort((void *)tmp, (size_t)ntmp, sizeof(struct NEIGHBOR), compare_tmpid);
	// consolidate duplicate ids
	prev = -1;
	k = 0; cnt = 1;
	for (i = 0; i < ntmp; i++)
	{
		if (tmp[i].id != prev)	// hit a new id
		{
			if (k > 0) tmp[k-1].cnt = cnt;
			tmp[k++].id = tmp[i].id;
			prev = tmp[i].id;
			cnt = 1;
		}
		else
			cnt++;
	}
	tmp[k-1].cnt = cnt;

	for (i = 0; i < k; i++)	// move to out_nbrs[] 
		out_nbrs[i] = tmp[i];

	*n_outnbrs = k;

	return 0;
}

//
// pool inward neighbors of the 2 given papers together and remove duplicates
//
int pool2_in_nbrs(int p1, int p2, int nnodes, struct PN *nw, int *n_innbrs, struct NEIGHBOR *in_nbrs)
{
	int i, k;
	int ntmp;
	int cnt;
	struct NEIGHBOR *tmp;
	int prev;

	ntmp = 0;
	for (k = 0; k < nw[p1].in_deg; k++)	// paper 1
		tmp[ntmp++].id = nw[p1].in_nbrs[k];
	for (k = 0; k < nw[p2].in_deg; k++)	// paper 2
		tmp[ntmp++].id = nw[p2].in_nbrs[k];

	qsort((void *)tmp, (size_t)ntmp, sizeof(struct NEIGHBOR), compare_tmpid);
	// consolidate duplicate ids
	prev = -1;
	k = 0; cnt = 1;
	for (i = 0; i < ntmp; i++)
	{
		if (tmp[i].id != prev)	// hit a new id
		{
			if (k > 0) tmp[k-1].cnt = cnt;
			tmp[k++].id = tmp[i].id;
			prev = tmp[i].id;
			cnt = 1;
		}
		else
			cnt++;
	}
	tmp[k-1].cnt = cnt;

	for (i = 0; i < k; i++)	// move to in_nbrs[] 
		in_nbrs[i] = tmp[i];

	*n_innbrs = k;

	return 0;
}
#endif XXX

static int nipcv;
static int nnbrs;
static int ntk;
static struct IPC_VECTOR *ipc_v;
static struct PSTRING *pstring1;
static struct PSTRING *pstring2;
static struct NEIGHBOR *in_nbrs;
static struct NEIGHBOR *out_nbrs;
static struct NEIGHBOR *tnbrs;
// 
// initialize IPC_VECTOR array
//
int init_relevancy()
{
	ipc_v = (struct IPC_VECTOR *)Jmalloc(LENGTH_IPC_VECTOR * sizeof(struct IPC_VECTOR), L"init_relevancy: ipc_v");
	if (ipc_v == NULL) return MSG_NOT_ENOUGH_MEMORY;

	pstring1 = (struct PSTRING *)Jmalloc(LENGTH_IPC_VECTOR * sizeof(struct PSTRING), L"init_relevancy: pstring1");
	if (pstring1 == NULL) return MSG_NOT_ENOUGH_MEMORY;
	pstring2 = (struct PSTRING *)Jmalloc(LENGTH_IPC_VECTOR * sizeof(struct PSTRING), L"init_relevancy: pstring2");
	if (pstring2 == NULL) return MSG_NOT_ENOUGH_MEMORY;

	in_nbrs = (struct NEIGHBOR *)Jmalloc(N_ATTRIBUTES * sizeof(struct NEIGHBOR), L"init_relevancy: in_nbrs");
	if (in_nbrs == NULL) return MSG_NOT_ENOUGH_MEMORY;
	out_nbrs = (struct NEIGHBOR *)Jmalloc(N_ATTRIBUTES * sizeof(struct NEIGHBOR), L"init_relevancy: out_nbrs");
	if (out_nbrs == NULL) return MSG_NOT_ENOUGH_MEMORY;
	tnbrs = (struct NEIGHBOR *)Jmalloc(N_ATTRIBUTES * sizeof(struct NEIGHBOR), L"init_relevancy: tnbrs");
	if (tnbrs == NULL) return MSG_NOT_ENOUGH_MEMORY;

	return 0;
}

//
// clean up IPC_VECTOR array
//
int cleanup_relevancy()
{
	Jfree(ipc_v, L"cleanup_relevancy: ipc_v");
	Jfree(pstring1, L"cleanup_relevancy: pstring1");
	Jfree(pstring2, L"cleanup_relevancy: pstring2");
	Jfree(in_nbrs, L"cleanup_relevancy: in_nbrs");
	Jfree(out_nbrs, L"cleanup_relevancy: out_nbrs");
	Jfree(tnbrs, L"cleanup_relevancy: tmp");
	return 0;
}

//
// pool IPCs of the two given patents together and remove duplicates
// input: index for patent 1 and 2
// output: set the value of nipcv and pstring1, pstring2
//
int pool_IPCs(int ndx1, int ndx2)
{
	int i, k;
	int wndx1, wndx2;
	int ntipcv;
	int cnt;
	int cnt2;
	struct IPC_VECTOR *tipcv;
	wchar_t prev_name[MAX_IPC_CODE];

	wndx1 = nw[ndx1].ndx2wos;
	wndx2 = nw[ndx2].ndx2wos;
	tipcv = ipc_v;
	ntipcv = 0;
	for (k = 0; k < wos[wndx1].nipc; k++)	// patent 1
	{
		tipcv[ntipcv].ndx1 = k;	// index to the patent 1 IPC array
		tipcv[ntipcv].ndx2 = -1;	
		tipcv[ntipcv].cnt = wos[wndx1].ipc_cnt[k];
		wcscpy(tipcv[ntipcv++].ipc, &wos[wndx1].ipc[k*MAX_IPC_CODE]);
	}
	for (k = 0; k < wos[wndx2].nipc; k++)	// patent 2
	{
		tipcv[ntipcv].ndx1 = -1;	
		tipcv[ntipcv].ndx2 = k;	// index to the patent 2 IPC array
		tipcv[ntipcv].cnt = wos[wndx2].ipc_cnt[k];
		wcscpy(tipcv[ntipcv++].ipc, &wos[wndx2].ipc[k*MAX_IPC_CODE]);
	}
	qsort((void *)tipcv, (size_t)ntipcv, sizeof(struct IPC_VECTOR), compare_ipcv);

	// consolidate duplicate IPC codes
	prev_name[0] = '\0';
	for (i = 0; i < ntipcv; i++) pstring1[i].cnt = pstring2[i].cnt = 0;
	k = 0; cnt = tipcv[0].cnt;
	for (i = 0; i < ntipcv; i++)
	{
		if (wcscmp(tipcv[i].ipc, prev_name) != 0)	// hit a new IPC code
		{
			if (k > 0) tipcv[k-1].cnt = cnt;
			wcscpy_s(tipcv[k++].ipc, MAX_IPC_CODE, tipcv[i].ipc); 
			wcscpy_s(prev_name, MAX_IPC_CODE, tipcv[i].ipc); 
			cnt = tipcv[i].cnt;;
		}
		else
			cnt += tipcv[i].cnt;
		if (tipcv[i].ndx1 >= 0) pstring1[k-1].cnt += tipcv[i].cnt;
		if (tipcv[i].ndx2 >= 0) pstring2[k-1].cnt += tipcv[i].cnt;
	}
	tipcv[k-1].cnt = cnt;
	nipcv = k;

	// normalize pstring[]
	for (i = 0; i < nipcv; i++)
	{
		pstring1[i].prob = (double)pstring1[i].cnt / nipcv;
		pstring2[i].prob = (double)pstring2[i].cnt / nipcv;
	}

#ifdef DEBUG
	for (i = 0; i < nipcv; i++)
		fwprintf(logstream, L"@@@@@ (%d)%s=>(%d)%s [%s] %d [%d %d]\n", ndx2, wos[wndx2].alias, ndx1, wos[wndx1].alias, tipcv[i].ipc, tipcv[i].cnt, pstring1[i].cnt, pstring2[i].cnt);
	fwprintf(logstream, L"\n");
#endif DEBUG

	return 0;
}

//
// pool all neighbors of the two given documents together and remove duplicates
// input: index for document 1 and 2
// output: set the value of nnbrs and pstring1, pstring2
//
int pool_neighbors(int ndx1, int ndx2)
{
	int i, k;
	int cnt;
	int cnt2;
	int prev;

	nnbrs = 0;
	for (k = 0; k < nw[ndx1].degree; k++)	// document 1
	{
		tnbrs[nnbrs].ndx1 = k;	// index to the document 1 nbrs array
		tnbrs[nnbrs].ndx2 = -1;	
		tnbrs[nnbrs++].id = nw[ndx1].nbrs[k];
	}
	for (k = 0; k < nw[ndx2].degree; k++)	// document 2
	{
		tnbrs[nnbrs].ndx1 = -1;	
		tnbrs[nnbrs].ndx2 = k;	// index to the patent 2 IPC array
		tnbrs[nnbrs++].id = nw[ndx2].nbrs[k];
	}

	qsort((void *)tnbrs, (size_t)nnbrs, sizeof(struct NEIGHBOR), compare_tmpid);

	// consolidate duplicate ids
	prev = -1;
	for (i = 0; i < nnbrs; i++) pstring1[i].cnt = pstring2[i].cnt = 0;
	k = 0; cnt = 1;
	for (i = 0; i < nnbrs; i++)
	{
		if (tnbrs[i].id != prev)	// hit a new id
		{
			if (k > 0) tnbrs[k-1].cnt = cnt;
			tnbrs[k++].id = tnbrs[i].id;
			prev = tnbrs[i].id;
			cnt = 1;
		}
		else
			cnt++;
		if (tnbrs[i].ndx1 >= 0) pstring1[k-1].cnt += 1;
		if (tnbrs[i].ndx2 >= 0) pstring2[k-1].cnt += 1;
	}
	tnbrs[k-1].cnt = cnt;
	nnbrs = k;

	// normalize pstring[]
	for (i = 0; i < nnbrs; i++)
	{
		pstring1[i].prob = (double)pstring1[i].cnt / nnbrs;
		pstring2[i].prob = (double)pstring2[i].cnt / nnbrs;
	}

#ifdef DEBUG
	for (i = 0; i < nnbrs; i++)
		fwprintf(logstream, L"@@@@@ (%d)%s=>(%d)%s [%d] %d [%d %d]\n", ndx2, wos[nw[ndx2].ndx2wos].alias, ndx1, wos[nw[ndx1].ndx2wos].alias, tnbrs[i].id, tnbrs[i].cnt, pstring1[i].cnt, pstring2[i].cnt);
	fwprintf(logstream, L"\n");
#endif DEBUG

	return 0;
}

//
// pool keywords of the two given documents and remove duplicates
// pooled keywords are stored in tk[], and ntk indicates the number of pooled keywords
// NOTE: this function also filters out keywords that are not listed in the "Keyword report.txt" file
// added 2017/01/11
//
int pool_keywords(int p1, int p2, struct WOS *wos, struct TKEYWORDS *tkeyword)
{
	int i, k;
	int nttk;
	int cnt;
	int cnt2;
	struct TKEYWORDS *ttk;
	wchar_t prev_name[MAX_TKEYWORDS];

	ttk = (struct TKEYWORDS *)Jmalloc(MAX_KEYWORDS * 30 * sizeof(struct TKEYWORDS), L"pool_keywords: ttk");
	if (ttk == NULL) return MSG_NOT_ENOUGH_MEMORY;

	//fwprintf(logstream, L"***** %s=>%s\n", wos[p1].alias, wos[p2].alias);

	// NOTE: wos[].tkws[] include keywords in both the title and abstract
	nttk = 0;
	for (k = 0; k < wos[p1].ntkws; k++)	// paper 1
	{
		if (is_in_keyword_report(tkeyword[wos[p1].tkws[k]].name))	// take only the keyword listed in the "Keyword report.txt" file
		{
			ttk[nttk].ndx1 = k;	// index to the document 1 keyword array
			ttk[nttk].ndx2 = -1;	
			ttk[nttk].cnt = 0;
			wcscpy(ttk[nttk++].name, tkeyword[wos[p1].tkws[k]].name);
			//fwprintf(logstream, L"P1 %s ", tkeyword[wos[p1].tkws[k]].name);
		}
	}
	for (k = 0; k < wos[p2].ntkws; k++)	// paper 2
	{
		if (is_in_keyword_report(tkeyword[wos[p2].tkws[k]].name))	// take only the keyword listed in the "Keyword report.txt" file
		{
			ttk[nttk].ndx1 = -1;	
			ttk[nttk].ndx2 = k;	// index to the document 2 keyword array
			ttk[nttk].cnt = 0;
			wcscpy(ttk[nttk++].name, tkeyword[wos[p2].tkws[k]].name);
			//fwprintf(logstream, L"P2 %s ", tkeyword[wos[p2].tkws[k]].name);
		}
	}

	qsort((void *)ttk, (size_t)nttk, sizeof(struct TKEYWORDS), compare_tkeyword);
	// consolidate duplicate title keywords names
	prev_name[0] = '\0';
	for (i = 0; i < nttk; i++) pstring1[i].cnt = pstring2[i].cnt = 0;
	k = 0;  cnt = 1;
	for (i = 0; i < nttk; i++)
	{
		if (wcscmp(ttk[i].name, prev_name) != 0)	// hit a new name
		{
			if (k > 0) ttk[k-1].cnt = cnt;
			//fwprintf(logstream, L"&&&&& %d=>%d %d %d %d\n", p1, p2, i, k, cnt);
			wcscpy_s(ttk[k++].name, MAX_TKEYWORDS, ttk[i].name); 
			wcscpy_s(prev_name, MAX_TKEYWORDS, ttk[i].name); 
			cnt = 1;
		}
		else
			cnt++;
		if (ttk[i].ndx1 >= 0) pstring1[k-1].cnt += 1;
		if (ttk[i].ndx2 >= 0) pstring2[k-1].cnt += 1;
	}
	ttk[k-1].cnt = cnt;
	ntk = k;

	// normalize pstring[]
	for (i = 0; i < ntk; i++)
	{
		pstring1[i].prob = (double)pstring1[i].cnt / ntk;
		pstring2[i].prob = (double)pstring2[i].cnt / ntk;
	}

#ifdef DEBUG
	fwprintf(logstream, L"***** ntk=%d\n", ntk);
	for (i = 0; i < ntk; i++)
		fwprintf(logstream, L"%03d %02d [%s]\n", i, ttk[i].cnt, ttk[i].name);
	fflush(logstream);
#endif DEBUG

	Jfree(ttk, L"pool_keywords: ttk");

	return 0;
}

//
// calculate the difference between the two given connected nodes
//
double relevancy(int relevancystrategy, int node_to, int node_from)
{
	int k;
	double rlv;
	double rlv_IPC, rlv_kwds;
	double rlv_nbrs;
	struct P_IPC ipc_to, ipc_from;
	int nipc_to, nipc_from;
	int wndx_to, wndx_from;

	if (full_record_type == USPTO_DATA || full_record_type == THOMSON_INNOVATION_DATA || full_record_type == WEBPAT2_DATA || full_record_type == WEBPAT3_DATA) 
	{	
		if (relevancystrategy == RELEVANCY_CPC_JACCARD)
		{
			pool_IPCs(node_to, node_from);
			//rlv = 1.0 - Jaccard_distance_generalized(nipcv, pstring1, pstring2);
			rlv = 1.0 - Jaccard_distance(nipcv, pstring1, pstring2);
		}
		else if (relevancystrategy == RELEVANCY_CPC3_JACCARD)
		{
			wndx_to = nw[node_to].ndx2wos;
			wndx_from = nw[node_from].ndx2wos;
			// save IPC info in wos[]
			ipc_to.nipc = wos[wndx_to].nipc;
			for (k = 0; k < wos[wndx_to].nipc; k++)	
				wcscpy_s(&ipc_to.ipc[MAX_IPC_CODE*k], MAX_IPC_CODE, &wos[wndx_to].ipc[MAX_IPC_CODE*k]);
			ipc_from.nipc = wos[wndx_from].nipc;
			for (k = 0; k < wos[wndx_from].nipc; k++)	
				wcscpy_s(&ipc_from.ipc[MAX_IPC_CODE*k], MAX_IPC_CODE, &wos[wndx_from].ipc[MAX_IPC_CODE*k]);
			// reduce IPC (CPC) codes to 4 characters
			nipc_to = replace_IPC_codes(wndx_to, 4, wos, uspto);
			nipc_from = replace_IPC_codes(wndx_from, 4, wos, uspto);
			pool_IPCs(node_to, node_from);
			//rlv = 1.0 - Jaccard_distance_generalized(nipcv, pstring1, pstring2);
			rlv = 1.0 - Jaccard_distance(nipcv, pstring1, pstring2);
			// restore IPC info back to wos[]
			wos[wndx_to].nipc = ipc_to.nipc;
			for (k = 0; k < ipc_to.nipc; k++)
				wcscpy_s(&wos[wndx_to].ipc[MAX_IPC_CODE*k], MAX_IPC_CODE, &ipc_to.ipc[MAX_IPC_CODE*k]);
			wos[wndx_from].nipc = ipc_from.nipc;
			for (k = 0; k < ipc_from.nipc; k++)
				wcscpy_s(&wos[wndx_from].ipc[MAX_IPC_CODE*k], MAX_IPC_CODE, &ipc_from.ipc[MAX_IPC_CODE*k]);
		}
		else if (relevancystrategy == RELEVANCY_CPC4_JACCARD)
		{
			wndx_to = nw[node_to].ndx2wos;
			wndx_from = nw[node_from].ndx2wos;
			// save IPC info in wos[]
			ipc_to.nipc = wos[wndx_to].nipc;
			for (k = 0; k < wos[wndx_to].nipc; k++)	
				wcscpy_s(&ipc_to.ipc[MAX_IPC_CODE*k], MAX_IPC_CODE, &wos[wndx_to].ipc[MAX_IPC_CODE*k]);
			ipc_from.nipc = wos[wndx_from].nipc;
			for (k = 0; k < wos[wndx_from].nipc; k++)	
				wcscpy_s(&ipc_from.ipc[MAX_IPC_CODE*k], MAX_IPC_CODE, &wos[wndx_from].ipc[MAX_IPC_CODE*k]);
			// reduce CPC codes up to '/'
			nipc_to = replace_CPC4_codes(wndx_to, wos, uspto);
			nipc_from = replace_CPC4_codes(wndx_from, wos, uspto);
			pool_IPCs(node_to, node_from);
			//rlv = 1.0 - Jaccard_distance_generalized(nipcv, pstring1, pstring2);
			rlv = 1.0 - Jaccard_distance(nipcv, pstring1, pstring2);
			// restore IPC info back to wos[]
			wos[wndx_to].nipc = ipc_to.nipc;
			for (k = 0; k < ipc_to.nipc; k++)
				wcscpy_s(&wos[wndx_to].ipc[MAX_IPC_CODE*k], MAX_IPC_CODE, &ipc_to.ipc[MAX_IPC_CODE*k]);
			wos[wndx_from].nipc = ipc_from.nipc;
			for (k = 0; k < ipc_from.nipc; k++)
				wcscpy_s(&wos[wndx_from].ipc[MAX_IPC_CODE*k], MAX_IPC_CODE, &ipc_from.ipc[MAX_IPC_CODE*k]);
		}
		else if (relevancystrategy == RELEVANCY_CITATION_JACCARD)
		{
			pool_neighbors(node_to, node_from);
			//rlv = 1.0 - Jaccard_distance_generalized(nnbrs, pstring1, pstring2);
			rlv = 1.0 - Jaccard_distance(nnbrs, pstring1, pstring2);
		}
		else if (relevancystrategy == RELEVANCY_CPC3_CITATION_JACCARD)
		{
			static struct P_IPC ipc_to, ipc_from;
			int nipc_to, nipc_from;
			int wndx_to, wndx_from;
			wndx_to = nw[node_to].ndx2wos;
			wndx_from = nw[node_from].ndx2wos;
			// 1st step, find relevancy using IPC4
			// save IPC info in wos[]
			ipc_to.nipc = wos[wndx_to].nipc;
			for (k = 0; k < wos[wndx_to].nipc; k++)	
				wcscpy_s(&ipc_to.ipc[MAX_IPC_CODE*k], MAX_IPC_CODE, &wos[wndx_to].ipc[MAX_IPC_CODE*k]);
			ipc_from.nipc = wos[wndx_from].nipc;
			for (k = 0; k < wos[wndx_from].nipc; k++)	
				wcscpy_s(&ipc_from.ipc[MAX_IPC_CODE*k], MAX_IPC_CODE, &wos[wndx_from].ipc[MAX_IPC_CODE*k]);
			// reduce IPC codes to 4 characters
			nipc_to = replace_IPC_codes(wndx_to, 4, wos, uspto);
			nipc_from = replace_IPC_codes(wndx_from, 4, wos, uspto);
			pool_IPCs(node_to, node_from);
			//rlv_IPC = 1.0 - Jaccard_distance_generalized(nipcv, pstring1, pstring2);
			rlv_IPC = 1.0 - Jaccard_distance(nipcv, pstring1, pstring2);
			// restore IPC info back to wos[]
			wos[wndx_to].nipc = ipc_to.nipc;
			for (k = 0; k < ipc_to.nipc; k++)
				wcscpy_s(&wos[wndx_to].ipc[MAX_IPC_CODE*k], MAX_IPC_CODE, &ipc_to.ipc[MAX_IPC_CODE*k]);
			wos[wndx_from].nipc = ipc_from.nipc;
			for (k = 0; k < ipc_from.nipc; k++)
				wcscpy_s(&wos[wndx_from].ipc[MAX_IPC_CODE*k], MAX_IPC_CODE, &ipc_from.ipc[MAX_IPC_CODE*k]);
			// 2nd step, find relevancy using citations
			pool_neighbors(node_to, node_from);
			rlv_nbrs = 1.0 - Jaccard_distance_generalized(nnbrs, pstring1, pstring2);
			rlv = 0.5 *rlv_IPC + 0.5 * rlv_nbrs;
		}
		else if (relevancystrategy == RELEVANCY_CPC4_CITATION_JACCARD)
		{
			static struct P_IPC ipc_to, ipc_from;
			int nipc_to, nipc_from;
			int wndx_to, wndx_from;
			wndx_to = nw[node_to].ndx2wos;
			wndx_from = nw[node_from].ndx2wos;
			// 1st step, find relevancy using IPC4
			// save IPC info in wos[]
			ipc_to.nipc = wos[wndx_to].nipc;
			for (k = 0; k < wos[wndx_to].nipc; k++)	
				wcscpy_s(&ipc_to.ipc[MAX_IPC_CODE*k], MAX_IPC_CODE, &wos[wndx_to].ipc[MAX_IPC_CODE*k]);
			ipc_from.nipc = wos[wndx_from].nipc;
			for (k = 0; k < wos[wndx_from].nipc; k++)	
				wcscpy_s(&ipc_from.ipc[MAX_IPC_CODE*k], MAX_IPC_CODE, &wos[wndx_from].ipc[MAX_IPC_CODE*k]);
			// reduce CPC codes up to '/'
			nipc_to = replace_CPC4_codes(wndx_to, wos, uspto);
			nipc_from = replace_CPC4_codes(wndx_from, wos, uspto);
			pool_IPCs(node_to, node_from);
			//rlv_IPC = 1.0 - Jaccard_distance_generalized(nipcv, pstring1, pstring2);
			rlv_IPC = 1.0 - Jaccard_distance(nipcv, pstring1, pstring2);
			// restore IPC info back to wos[]
			wos[wndx_to].nipc = ipc_to.nipc;
			for (k = 0; k < ipc_to.nipc; k++)
				wcscpy_s(&wos[wndx_to].ipc[MAX_IPC_CODE*k], MAX_IPC_CODE, &ipc_to.ipc[MAX_IPC_CODE*k]);
			wos[wndx_from].nipc = ipc_from.nipc;
			for (k = 0; k < ipc_from.nipc; k++)
				wcscpy_s(&wos[wndx_from].ipc[MAX_IPC_CODE*k], MAX_IPC_CODE, &ipc_from.ipc[MAX_IPC_CODE*k]);
			// 2nd step, find relevancy using citations
			pool_neighbors(node_to, node_from);
			rlv_nbrs = 1.0 - Jaccard_distance_generalized(nnbrs, pstring1, pstring2);
			rlv = 0.5 *rlv_IPC + 0.5 * rlv_nbrs;
		}
		else
			rlv = 1.0;
	}
	else if (full_record_type == WOS_DATA || full_record_type == SCOPUS_DATA) 
	{	
		int iia, iib;	// index to the wos[] array
		int j;
		struct TKEYWORDS *tk;
		tk = (struct TKEYWORDS *)Jmalloc(MAX_KEYWORDS * 30 * sizeof(struct TKEYWORDS), L"critical_transition: tk");
		if (tk == NULL) return MSG_NOT_ENOUGH_MEMORY;
		if (relevancystrategy == RELEVANCY_D_KEYWORD_JACCARD)
		{
			iia = nw[node_from].ndx2wos; iib = nw[node_to].ndx2wos;
#ifdef DEBUG
			fwprintf(logstream, L"@@@@@ %d\t", iia);
			for (k = 0; k < wos[iia].ntkws; k++)
				fwprintf(logstream, L"%s\t", tkeyword[wos[iia].tkws[k]].name);
			fwprintf(logstream, L"\n");
			fwprintf(logstream, L"##### %d\t", iib);
			for (k = 0; k < wos[iib].ntkws; k++)
				fwprintf(logstream, L"%s\t", tkeyword[wos[iib].tkws[k]].name);
			fwprintf(logstream, L"\n");
#endif DEBUG
			for (j = 0; j < ntkeywords; j++) tkeyword[j].ndx = -1;
			pool_keywords(iia, iib, wos, tkeyword);
			rlv = 1.0 - Jaccard_distance_generalized(ntk, pstring1, pstring2);
		}
		else if (relevancystrategy == RELEVANCY_D_CITATION_JACCARD)
		{
			pool_neighbors(node_to, node_from);
			rlv = 1.0 - Jaccard_distance_generalized(nnbrs, pstring1, pstring2);	// generalized or not, the results are the same
		}
		else if (relevancystrategy == RELEVANCY_D_KEYWORD_CITATION_JACCARD)
		{
			iia = nw[node_from].ndx2wos; iib = nw[node_to].ndx2wos;
			for (j = 0; j < ntkeywords; j++) tkeyword[j].ndx = -1;
			pool_keywords(iia, iib, wos, tkeyword);
			rlv_kwds = 1.0 - Jaccard_distance_generalized(ntk, pstring1, pstring2);
			pool_neighbors(node_to, node_from);
			rlv_nbrs = 1.0 - Jaccard_distance_generalized(nnbrs, pstring1, pstring2);
			rlv = 0.5 *rlv_kwds + 0.5 * rlv_nbrs;
		}
		else
			rlv = 1.0;
		Jfree(tk, L"relevancy: tk");
	}
	else
		rlv = 1.0;

	return rlv; 
}

#ifdef XXX
//
// Given a set of paths, find whether there are critical transitions on the path
// NOTE: the sequence is a->b->c
//
int relevancy(int npaths, struct SPATH *paths, int mptype, int nnodes, struct PN *nw, int nwos, struct WOS *wos, int ntkeywords, struct TKEYWORDS *tkeyword)
{
	int i, j, k, r, m;
	int ia, ib, ic;	// index in the nw[] array
	int iia, iib, iic;	// index in the wos[] array
	int ka, kb, kc, kin, kout;
	int in_cnt, out_cnt;
	int ntk, n_innbrs, n_outnbrs;
	int nattr;
	int ct_cnt;
	int ranking;
	PSTRING *a, *b, *c;	// probability distribution array
	double Dac, Dab, Dbc;
	struct TKEYWORDS *tk;
	struct NEIGHBOR *in_nbrs;
	struct NEIGHBOR *out_nbrs;
	int ndx2tk;
	//int indx[N_INDEGREE_ATTR];	// index (for nw[]) of the top in-degree nodes
	//int ondx[N_OUTDEGREE_ATTR];	// index (for nw[]) of the top out-degree nodes

	tk = (struct TKEYWORDS *)Jmalloc(MAX_KEYWORDS * 30 * sizeof(struct TKEYWORDS), L"critical_transition: tk");
	if (tk == NULL) return MSG_NOT_ENOUGH_MEMORY;
	in_nbrs = (struct NEIGHBOR *)Jmalloc(N_ATTRIBUTES * sizeof(struct NEIGHBOR), L"critical_transition: in_nbrs");
	if (in_nbrs == NULL) return MSG_NOT_ENOUGH_MEMORY;
	out_nbrs = (struct NEIGHBOR *)Jmalloc(N_ATTRIBUTES * sizeof(struct NEIGHBOR), L"critical_transition: out_nbrs");
	if (out_nbrs == NULL) return MSG_NOT_ENOUGH_MEMORY;

	//qsort((void *)nw, (size_t)nnodes, sizeof(struct PN), compare_indegree);	// sort according to a node's degree
	//for (i = 0; i < N_INDEGREE_ATTR; i++)
	//	indx[i] = nw[i].ndx;	// indx[0] contains the index that points to the node that has the largest in-degree, etc.
	//qsort((void *)nw, (size_t)nnodes, sizeof(struct PN), compare_outdegree);	// sort according to a node's degree
	//for (i = 0; i < N_OUTDEGREE_ATTR; i++)
	//	ondx[i] = nw[i].ndx;	// ondx[0] contains the index that points to the node that has the largest out-degree, etc.
	//for (i = 0; i < nnodes; i++)
	//	fwprintf(logstream, L"%d\t%s\t%s\t%d\t%d\n", nw[i].degree, nw[i].name, nw[i].alias, nw[i].ndx2wos, nw[i].ndx);
	//qsort((void *)nw, (size_t)nnodes, sizeof(struct PN), compare_nwname);	// sort back to the original order
	//for (i = 0; i < N_CITATION_ATTR; i++)
	//	fwprintf(logstream, L"***%d %d\n", indx[i], nw[dndx[i]].degree);

	a = (PSTRING *)Jmalloc(N_ATTRIBUTES * sizeof(PSTRING), L"critical_transition: a");
	b = (PSTRING *)Jmalloc(N_ATTRIBUTES * sizeof(PSTRING), L"critical_transition: b");
	c = (PSTRING *)Jmalloc(N_ATTRIBUTES * sizeof(PSTRING), L"critical_transition: c");

	fwprintf(logstream, L"\nCritical transitions:\n");
#ifdef OVERLAPPING_DISTANCE
	fwprintf(logstream, L"Apply Overlapping Distance\n");
#endif OVERLAPPING_DISTANCE

	for (i = 0; i < npaths; i++)
	{ 
		if (mptype == P_LOCAL_F && (paths[i].tcnt == 0 || paths[i].hit_a_sink == FALSE)) continue;
		if (mptype == P_LOCAL_B && (paths[i].tcnt == 0 || paths[i].hit_a_source == FALSE)) continue;
		fwprintf(logstream, L"paths=%d\n", i);
		if (paths[i].len < 3) continue;	// only when there are more than 3 nodes on the path
		for (k = 0; k < (paths[i].len - 2); k++)
		{
			ka = paths[i].seq[k]; kb = paths[i].seq[k+1]; kc = paths[i].seq[k+2];
			fwprintf(logstream, L"\t\t[%s->%s->%s]\t", nw[ka].alias, nw[kb].alias, nw[kc].alias);
			ct_cnt = in_cnt = 0;
			for (kin = 0; kin < nw[kb].in_deg; kin++)	// check on all the doglegs centered on 'ib'
			{
				in_cnt++; out_cnt = 0;
				for (kout = 0; kout < nw[kb].out_deg; kout++)
				{
					out_cnt++;
					//fwprintf(logstream, L"\t\t%d %d\n", kin, kout);
					ia = nw[kb].in_nbrs[kin]; ib = kb; ic = nw[kb].out_nbrs[kout];
					// establish the probability distribution array for a, b, and c
					for (j = 0; j < N_ATTRIBUTES; j++) {a[j].cnt = b[j].cnt = c[j].cnt = 0; }
					iia = nw[ia].ndx2wos; iib = nw[ib].ndx2wos; iic = nw[ic].ndx2wos;
					for (j = 0; j < ntkeywords; j++) tkeyword[j].ndx = -1;  // added 2015/03/04
					pool2_keywords(iia, iib, wos, tkeyword, &ntk, tk);	// pooled keywords are stored in tk[]
					// NOTE: after pooing keywords, tk[].acnt=0, tk[].cnt=total count of the keyword in both the title and abstract
					//fwprintf(logstream, L"##### ");
					//for (r = 0; r <  ntk; r++) fwprintf(logstream, L"%03d %s: %d %d %d %d\n", r, tk[r].name, tk[r].acnt, tk[r].cnt, tk[r].ranking, tk[r].ndx);
					//fflush(logstream);

					pool2_in_nbrs(ia, ib, nnodes, nw, &n_innbrs, in_nbrs);
					pool2_out_nbrs(ia, ib, nnodes, nw, &n_outnbrs, out_nbrs);

					//n_innbrs = n_outnbrs = 0;	// for debugging purpose
					//ntk = 0; n_innbrs = 0;			// for debugging purpose

					nattr = ntk + n_innbrs + n_outnbrs;

					// Note: the sequence is a->b->c
					// distribution for a
					for (r = 0; r < wos[iia].ntkws; r++)	// titlle keyword distribution for a
					{
						ndx2tk = tkeyword[wos[iia].tkws[r]].ndx;
						if (ndx2tk == -1)	// not in the current keyword pool
							continue;
						ranking = tk[ndx2tk].ranking;
						if (ranking < ntk) a[ranking].cnt++;
					}
					for (r = 0; r < nw[ia].in_deg; r++)	// indegree citation distribution for a
					{
						for (j = 0; j < n_innbrs; j++)
						{
							if (in_nbrs[j].id == nw[ia].in_nbrs[r]) a[ntk+j].cnt++;
						}
					}
					for (r = 0; r < nw[ia].out_deg; r++)	// outdegree citation distribution for a
					{
						for (j = 0; j < n_outnbrs; j++)
						{
							if (out_nbrs[j].id == nw[ia].out_nbrs[r]) a[ntk+n_innbrs+j].cnt++;
						}
					}
					// distribution for b
					for (r = 0; r < wos[iib].ntkws; r++)	// titlle keyword distribution for b
					{
						ndx2tk = tkeyword[wos[iib].tkws[r]].ndx;
						if (ndx2tk == -1)	// not in the current keyword pool
							continue;
						ranking = tk[ndx2tk].ranking;
						if (ranking < ntk) b[ranking].cnt++;
					}
					for (r = 0; r < nw[ib].in_deg; r++)	// indegree citation distribution for b
					{
						for (j = 0; j < n_innbrs; j++)
						{
							if (in_nbrs[j].id == nw[ib].in_nbrs[r]) b[ntk+j].cnt++;
						}
					}
					for (r = 0; r < nw[ib].out_deg; r++)	// outdegree citation distribution for b
					{
						for (j = 0; j < n_outnbrs; j++)
						{
							if (out_nbrs[j].id == nw[ib].out_nbrs[r]) b[ntk+n_innbrs+j].cnt++;
						}
					}
					// distribution for c
					for (r = 0; r < wos[iic].ntkws; r++)	// titlle keyword distribution for c
					{
						ndx2tk = tkeyword[wos[iic].tkws[r]].ndx;
						if (ndx2tk == -1)	// not in the current keyword pool
							continue;
						ranking = tk[ndx2tk].ranking;
						if (ranking < ntk) c[ranking].cnt++;
					}			
					for (r = 0; r < nw[ic].in_deg; r++)	// indegree citation distribution for c
					{
						for (j = 0; j < n_innbrs; j++)
						{
							if (in_nbrs[j].id == nw[ic].in_nbrs[r]) c[ntk+j].cnt++;
						}
					}
					for (r = 0; r < nw[ic].out_deg; r++)	// outdegree citation distribution for c
					{
						for (j = 0; j < n_outnbrs; j++)
						{
							if (out_nbrs[j].id == nw[ic].out_nbrs[r]) c[ntk+n_innbrs+j].cnt++;
						}
					}

		#ifdef DEBUG	// display probability distribution
					fwprintf(logstream, L"\n%s->%s->%s (ntk=%d, n_innbrs=%d, n_outnbrs=%d)\n", nw[ia].alias, nw[ib].alias, nw[ic].alias, ntk, n_innbrs, n_outnbrs);
					fwprintf(logstream, L"a: [");
					for (j = 0; j < nattr; j++) fwprintf(logstream, L"%f ", a[j].prob);
					fwprintf(logstream, L"]\nb: [");
					for (j = 0; j < nattr; j++) fwprintf(logstream, L"%f ", b[j].prob);
					fwprintf(logstream, L"]\nc: ["); fflush(logstream);
					for (j = 0; j < nattr; j++) fwprintf(logstream, L"%f ", c[j].prob);
					fwprintf(logstream, L"]\n"); fflush(logstream);
		#endif DEBUG
		#ifdef DEBUG	// display the contents that determine probability distribution
					fwprintf(logstream, L"\n%s->%s->%s (ntk=%d, n_innbrs=%d, n_outnbrs=%d)\n", nw[ia].alias, nw[ib].alias, nw[ic].alias, ntk, n_innbrs, n_outnbrs);
					fwprintf(logstream, L"a: ");
					for (j = 0; j < ntk; j++) fwprintf(logstream, L"%s[%d] ", tk[j].name, a[j].cnt);
					for (j = 0; j < n_innbrs; j++) fwprintf(logstream, L"%d(%d) ", in_nbrs[j].id, a[ntk+j].cnt);
					for (j = 0; j < n_outnbrs; j++) fwprintf(logstream, L"%d{%d} ", out_nbrs[j].id, a[ntk+n_innbrs+j].cnt);
					fwprintf(logstream, L"\nb: ");
					for (j = 0; j < ntk; j++) fwprintf(logstream, L"%s[%d] ", tk[j].name, b[j].cnt);
					for (j = 0; j < n_innbrs; j++) fwprintf(logstream, L"%d(%d) ", in_nbrs[j].id, b[ntk+j].cnt);
					for (j = 0; j < n_outnbrs; j++) fwprintf(logstream, L"%d{%d} ", out_nbrs[j].id, b[ntk+n_innbrs+j].cnt);
					fwprintf(logstream, L"\nc: "); fflush(logstream);
					for (j = 0; j < ntk; j++) fwprintf(logstream, L"%s[%d] ", tk[j].name, c[j].cnt);
					for (j = 0; j < n_innbrs; j++) fwprintf(logstream, L"%d(%d) ", in_nbrs[j].id, c[ntk+j].cnt);
					for (j = 0; j < n_outnbrs; j++) fwprintf(logstream, L"%d{%d} ", out_nbrs[j].id, c[ntk+n_innbrs+j].cnt);
					fwprintf(logstream, L"\n"); fflush(logstream);
		#endif DEBUG

#ifdef OVERLAPPING_DISTANCE
					Dab = overlapping_distance(nattr, b, a);
#endif OVERLAPPING_DISTANCE

		#ifdef DEBUG
					fwprintf(logstream, L"[%f, %f, %f] ==> %f\t", Dac, Dab, Dbc, Dac-(Dab+Dbc)); fflush(logstream);
					fwprintf(logstream, L"[%s->%s->%s]\n", nw[ia].alias, nw[ib].alias, nw[ic].alias);
		#endif DEBUG
					if (Dac-(Dab+Dbc) > 0)
					{
						ct_cnt++;
						//fwprintf(logstream, L"\t\t[%s->%s->%s]  ", nw[ia].alias, nw[ib].alias, nw[ic].alias);
						//fwprintf(logstream, L"\t\t\t[%f, %f, %f] ==> %f\n", Dac, Dab, Dbc, Dac-(Dab+Dbc)); fflush(logstream);
					}
				}
			}
			fwprintf(logstream, L"%d/%d=\t%f\n", ct_cnt, in_cnt*out_cnt, (double)ct_cnt/(in_cnt*out_cnt));
		}
	}

	Jfree(a, L"critical_transition: a");
	Jfree(b, L"critical_transition: b");
	Jfree(c, L"critical_transition: c");

	return 0;
}
#endif XXX

//
// find document relevancy for every citation links
// use the depth first algorithm
//
static int level;
int find_document_relevancy(int relevancystrategy, int nnodes, struct PN *nw)
{
	int i, j, k;
	int count;

	// initialization, necessary even for the RELEVANCY_FLAT case
	for (i = 0; i < nnodes; i++) 
	{
		for (k = 0; k < nw[i].in_deg; k++)
			nw[i].in_relevancy[k] = 1.0;
		for (k = 0; k < nw[i].out_deg; k++)
			nw[i].out_relevancy[k] = 1.0;
		nw[i].was_here = 0;
	}

	if (relevancystrategy == RELEVANCY_FLAT)
		return 0;

	//if (full_record_type == USPTO_DATA || full_record_type == THOMSON_INNOVATION_DATA) 
	init_relevancy();

	// begin (depth-first algorithm) 
	level = 0;
	for (i = 0; i < nnodes; i++)
	{
		if (nw[i].was_here == 0)	// work only on those nodes that are not checked
		{
			find_relevancy(relevancystrategy, i, nw);	
		}
	}
#ifdef DEBUG	
	for (i = 0; i < nnodes; i++) 
	{
		for (k = 0; k < nw[i].in_deg; k++)
			fwprintf(logstream, L"##### %s<==%s dist=%.6f\n", wos[nw[i].ndx2wos].alias, wos[nw[nw[i].in_nbrs[k]].ndx2wos].alias, nw[i].in_relevancy[k]);
	}
#endif DEBUG
	
	//if (full_record_type == USPTO_DATA || full_record_type == THOMSON_INNOVATION_DATA) 
	cleanup_relevancy();

	return 0;
}


//
// this function is recurrsive, that is, it calls itself
// Note: two static variables are used to xxxt
// IMPORTANT: for large database, it may need to increase the STACK size (system default is only 1MB)
//            To increase stack size, use linker option "/stack:0x200000" (2MB)
//
int find_relevancy(int relevancystrategy, int curn, struct PN *nw)
{
	int i, j, k, m;
	int nbhr;
	double rlv;

	if (nw[curn].was_here == 1)			
		return 0;					// this node is checked before, no need to duplicate the effort

	level++;
	for (j = 0; j < nw[curn].in_deg; j++)
	{
		nw[curn].was_here = 1;
		//nbhr = nw[curn].nbrs[j];	// before 2017/01/13, was wrong
		nbhr = nw[curn].in_nbrs[j];	// after 2017/01/13, is now correct
		rlv = relevancy(relevancystrategy, curn, nw[curn].in_nbrs[j]);
		//fwprintf(logstream, L"*****%.6f\n", rlv);
		nw[curn].in_relevancy[j] = rlv;	// set the value for in_relevancy
		i = nw[curn].in_nbrs[j];
		for (k = 0; k < nw[i].out_deg; k++)	// also set the value for out_relevancy by searching backward
		{
			if (nw[i].out_nbrs[k] == curn)
			{
				nw[i].out_relevancy[k] = rlv;
				break;
			}
		}
		find_relevancy(relevancystrategy, nbhr, nw);
	}
	level--;

	return 0;
}

//
// apply relevancy strategy, i.e., multiply the SPX with the relevancy value
//
int apply_relevancy_strategy(int relevancystrategy)
{
	int i, k;

	if (relevancystrategy == RELEVANCY_FLAT)	// for RELEVANCY_FLAT, do nothing
		return 0;

	for (i = 0; i < nnodes; i++)
	{
		for (k = 0; k < nw[i].in_deg; k++)
			nw[i].in_spx[k] = nw[i].in_spx[k] * nw[i].in_relevancy[k];
		for (k = 0; k < nw[i].out_deg; k++)
			nw[i].out_spx[k] = nw[i].out_spx[k] * nw[i].out_relevancy[k];
	}
#ifdef DEBUG
	for (i = 0; i < nnodes; i++)
	{
		fwprintf(logstream, L"@@@@@[in_spx] %d %s\n", i, nw[i].alias);
		for (k = 0; k < nw[i].in_deg; k++)
			fwprintf(logstream, L"%f\t", nw[i].in_spx[k]);
		fwprintf(logstream, L"\n&&&&&[out_spx] %d %s\n ", i, nw[i].alias);
		for (k = 0; k < nw[i].out_deg; k++)
			fwprintf(logstream, L"%f\t", nw[i].out_spx[k]);
		fwprintf(logstream, L"\n");
	}
#endif DEBUG

	return 0;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_ipcv(const void *n1, const void *n2)
{
	struct IPC_VECTOR *t1, *t2;
	
	t1 = (struct IPC_VECTOR *)n1;
	t2 = (struct IPC_VECTOR *)n2;
	if (wcscmp(t2->ipc, t1->ipc) < 0)
		return 1;
	else if (wcscmp(t2->ipc, t1->ipc) == 0)
		return 0;
	else return -1;
}