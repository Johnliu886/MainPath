// 
// IPCstatistics.cpp
//
// Author: John Liu
// 
// this file contains functions that summarize and print IPC data 
//
//
// Revision History:
// 2012/03/31 1.00	Basic function works
//

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include <math.h>
#include "resource.h"
#include "network.h"

int ipc_year_stat(int, struct USPTO *, int, struct IPC_STAT *, FILE *);
int compare_ipc(const void *, const void *);
int compare_count(const void *, const void *);

extern FILE *logstream;

#define AVERAGE_IPC 10	// average number of IPCs for each patent
struct IPC_STAT
{	
	wchar_t ipc[MAX_IPC_CODE];
	int count;
};

//
// summarize and print IPC data to a given file stream
//
int ipc_statistics(struct USPTO *pat, int nuspto)
{
	int i, k;
	int byear, eyear;
	struct IPC_STAT *ipc_stat;
	wchar_t ofname[FNAME_SIZE];
	FILE *ostream;

	swprintf(ofname, L"IPC statistics.txt");
	_wfopen_s(&ostream, ofname, L"w");

	ipc_stat = (struct IPC_STAT *)Jmalloc(nuspto * AVERAGE_IPC * sizeof(struct IPC_STAT), L"ipc_statistics: ipc_stat");
	if (ipc_stat == NULL) return MSG_NOT_ENOUGH_MEMORY;

	byear = 9999; eyear = 0;
	for (i = 0; i < nuspto; i++)
	{
		if (byear > pat[i].year) byear = pat[i].year;
		if (eyear < pat[i].year) eyear = pat[i].year;
	}

	fwprintf(ostream, L"Year\tTCount\tNTypes\tEntropy\tConcentration\n");
	for (i = byear; i <= eyear; i++)
		ipc_year_stat(i, pat, nuspto, ipc_stat, ostream);

#ifdef DEBUG
	for (i = 0; i < nuspto; i++)
	{
		fwprintf(ostream, L"%s: ", pat[i].pid);
		for (k = 0; k < pat[i].nipc; k++)
		{
			fwprintf(ostream, L"%s |", &pat[i].ipc[MAX_IPC_CODE*k]);
		}
		fwprintf(ostream, L"\n");
	}
#endif DEBUG

	fclose(ostream);

	return 0;
}

// 
// summarize IPC statistics for a given year
//
int ipc_year_stat(int year, struct USPTO *pat, int nuspto, struct IPC_STAT *ipc_stat, FILE *ostream)
{
	int i, j, k;
	int tnipcs, nipcs;
	int count;
	wchar_t prev_ipc[MAX_IPC_CODE];

	j = 0;
	for (i = 0; i < nuspto; i++)
	{
		if (pat[i].year == year)
		{
			for (k = 0; k < pat[i].nipc; k++)
			{
				wcscpy_s(ipc_stat[j++].ipc, MAX_IPC_CODE*sizeof(wchar_t), &pat[i].ipc[MAX_IPC_CODE*k]);				
			}
		}
	}
	tnipcs = nipcs = j;

	qsort((void *)ipc_stat, (size_t)nipcs, sizeof(struct IPC_STAT), compare_ipc);

	// consolidate duplicate IPCs
	prev_ipc[0] = '\0';
	k = 0;
	count = 1;
	for (i = 0; i < nipcs; i++)
	{
		if (wcscmp(ipc_stat[i].ipc, prev_ipc) != 0)	// hit new ipc
		{
			wcscpy_s(ipc_stat[k].ipc, MAX_IPC_CODE, ipc_stat[i].ipc); 
			wcscpy_s(prev_ipc, MAX_IPC_CODE, ipc_stat[i].ipc); 
			if (k != 0) { ipc_stat[k-1].count = count; count = 1; }
			k++;
		}
		else
			count++;
	}
	ipc_stat[k-1].count = count;
	nipcs = k;

	// sort again according to the count
	qsort((void *)ipc_stat, (size_t)nipcs, sizeof(struct IPC_STAT), compare_count);

	// calculate entropy
	double entropy = 0.0; double concentration = 0.0;
	double prob; int sum = 0;
	for (j = 0; j < nipcs; j++)
	{
		if (tnipcs != 0.0)
		{
			prob = (double)ipc_stat[j].count / tnipcs;
			entropy += -1.0 * (prob * log(prob));
			sum += ipc_stat[j].count * ipc_stat[j].count;
		}
	}
	if (tnipcs != 0.0)
		concentration = (double)sum / (tnipcs * tnipcs);
	
	// the print is in the order: year, count, entropy, concentration 
	fwprintf(ostream, L"%d\t%d\t%d\t%.6f\t%.6f\n", year, tnipcs, nipcs, entropy, concentration);
	fwprintf(logstream, L"%d\n", year);
	for (j = 0; j < nipcs; j++)
		fwprintf(logstream, L"%d:%s(%d) ", j+1, ipc_stat[j].ipc, ipc_stat[j].count);
	fwprintf(logstream, L"\n");

	return 0;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_ipc(const void *n1, const void *n2)
{
	struct IPC_STAT *t1, *t2;
	
	t1 = (struct IPC_STAT *)n1;
	t2 = (struct IPC_STAT *)n2;
	if (wcscmp(t2->ipc, t1->ipc) < 0)
		return 1;
	else if (wcscmp(t2->ipc, t1->ipc) == 0)
		return 0;
	else return -1;
}

//
// this fucntion is to be called by qsort() only
// 
int compare_count(const void *n1, const void *n2)
{
	struct IPC_STAT *t1, *t2;
	
	t1 = (struct IPC_STAT *)n1;
	t2 = (struct IPC_STAT *)n2;
	if (t2->count > t1->count)	// larger number first
		return 1;
	else if (t2->count == t1->count)
		return 0;
	else return -1;
}
