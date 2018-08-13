// 
// SPXstatistics.cpp
//
// Author: John Liu
// 
// this file contains functions that summarize and print SPX data 
//
//
// Revision History:
// 2012/06/26 Basic function works
// 2012/07/05 Fixed a problem in taking log10 of 0
// 2013/01/31 Fixed a problem in spx_year_stat() when there is no data in the given year
//

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include <math.h>
#include "resource.h"
#include "network.h"

int spx_year_stat(int, struct SPX_STAT *, FILE *);
int compare_sspx(const void *, const void *);
int compare_sspxr(const void *, const void *);
int compare_scount(const void *, const void *);

extern int nwos;
extern struct WOS *wos;
extern int nnodes;
extern struct PN *nw;

extern FILE *logstream;

struct SPX_STAT
{	
	double spx;
	int spxr;
	int ndx2nw;
	int count;
};

//
// summarize and print SPX data to a given file stream
//
int SPX_statistics(FILE *ostream)
{
	int i, j, m, k;
	int byear, eyear;
	int nspxs;
	struct SPX_STAT *spx_stat;

	spx_stat = (struct SPX_STAT *)Jmalloc(nnodes * sizeof(struct SPX_STAT), L"SPX_statistics: spx_stat");
	if (spx_stat == NULL) return MSG_NOT_ENOUGH_MEMORY;

	byear = 9999; eyear = 0;
	for (i = 0; i < nnodes; i++)
	{
		m = nw[i].ndx2wos;
		if (byear > wos[m].year) byear = wos[m].year;
		if (eyear < wos[m].year) eyear = wos[m].year;
	}

	// display the top 200 nodes (overall)
	fwprintf(ostream, L"\nTop 200 nodes:\n");
	j = 0;
	for (i = 0; i < nnodes; i++)
	{
		spx_stat[j].ndx2nw = i;
		spx_stat[j++].spx = nw[i].total_out_spx;
	}
	nspxs = j;	
	qsort((void *)spx_stat, (size_t)nspxs, sizeof(struct SPX_STAT), compare_sspx);
	int topn;
	if (nnodes > 200) topn = 200; else topn = nnodes; 
	for (i = 0; i < topn; i++)
		fwprintf(ostream, L"%d %.0f\t%s\n", i+1, spx_stat[i].spx, nw[spx_stat[i].ndx2nw].alias); 

	fwprintf(ostream, L"\nYearly variations for node's SPX\n");
	fwprintf(ostream, L"Year\tTCount\tNTypes\tEntropy\tConcentration\n");
	for (i = byear; i <= eyear; i++)
		spx_year_stat(i, spx_stat, ostream);

	free(spx_stat);

	return 0;
}

// 
// summarize IPC statistics for a given year
//
int spx_year_stat(int year, struct SPX_STAT *spx_stat, FILE *ostream)
{
	int i, j, k, m;
	int tnspxs, nspxs;
	int count;
	int prev_spx;

	// find the entropy and concentration on the log10 reduced spx (yearly)
	j = 0;
	for (i = 0; i < nnodes; i++)
	{
		m = nw[i].ndx2wos;
		if (wos[m].year == year)
		{
			if (nw[i].total_out_spx == 0.0)
				spx_stat[j++].spxr = 0.0;	// added 2012/07/05
			else
				spx_stat[j++].spxr = (int)log10(nw[i].total_out_spx);	// take log() and then cast it to an integer
			//fwprintf(ostream, L"%d %d %f\n", year, j, spx_stat[j-1].spxr);
		}
	}
	tnspxs = nspxs = j;

	double entropy = 0.0; double concentration = 0.0;
	if (nspxs == 0)	// will cause problem if go further
	{
		// the print is in the order: year, count, entropy, concentration 
		fwprintf(ostream, L"%d\t%d\t%d\t%.6f\t%.6f\n", year, tnspxs, nspxs, entropy, concentration);
		return 0;
	}
	
	qsort((void *)spx_stat, (size_t)nspxs, sizeof(struct SPX_STAT), compare_sspxr);

	// consolidate duplicate SPXs
	prev_spx = -1.0;
	k = 0;
	count = 1;
	for (i = 0; i < nspxs; i++)
	{
		if (spx_stat[i].spxr != prev_spx) // hit a new ipc
		{
			spx_stat[k].spxr = spx_stat[i].spxr; 
			prev_spx = spx_stat[i].spxr; 
			if (k != 0) { spx_stat[k-1].count = count; count = 1; }
			k++;
		}
		else
			count++;
	}
	spx_stat[k-1].count = count;	// NOTE: this will cause problem if k=0 (no data in that year), but this is avoided in the previous code
	nspxs = k;

	// sort again according to the count
	qsort((void *)spx_stat, (size_t)nspxs, sizeof(struct SPX_STAT), compare_scount);

	// calculate entropy
	double prob; int sum = 0;
	for (j = 0; j < nspxs; j++)
	{
		if (tnspxs != 0.0)
		{
			prob = (double)spx_stat[j].count / tnspxs;
			entropy += -1.0 * (prob * log(prob));
			sum += spx_stat[j].count * spx_stat[j].count;
		}
	}
	if (tnspxs != 0.0)
		concentration = (double)sum / (tnspxs * tnspxs);
	
	// the print is in the order: year, count, entropy, concentration 
	fwprintf(ostream, L"%d\t%d\t%d\t%.6f\t%.6f\n", year, tnspxs, nspxs, entropy, concentration);

	fwprintf(logstream, L"%d\n", year);
	for (j = 0; j < nspxs; j++)
		fwprintf(logstream, L"%d:%d(%d) ", j+1, spx_stat[j].spxr, spx_stat[j].count);
	fwprintf(logstream, L"\n");

	return 0;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_sspx(const void *n1, const void *n2)
{
	struct SPX_STAT *t1, *t2;
	
	t1 = (struct SPX_STAT *)n1;
	t2 = (struct SPX_STAT *)n2;
	if (t2->spx > t1->spx)
		return 1;
	else if (t2->spxr == t1->spx) 
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_sspxr(const void *n1, const void *n2)
{
	struct SPX_STAT *t1, *t2;
	
	t1 = (struct SPX_STAT *)n1;
	t2 = (struct SPX_STAT *)n2;
	if (t2->spxr > t1->spxr)
		return 1;
	else if (t2->spxr == t1->spxr) 
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_scount(const void *n1, const void *n2)
{
	struct SPX_STAT *t1, *t2;
	
	t1 = (struct SPX_STAT *)n1;
	t2 = (struct SPX_STAT *)n2;
	if (t2->count > t1->count)	// larger number first
		return 1;
	else if (t2->count == t1->count)
		return 0;
	else return -1;
}
