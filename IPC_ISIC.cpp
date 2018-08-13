//
// IPC-ISIC.cpp
//
// This file contains code to map an IPC code to ISIC code
//
// 2013/07/24 Basic function works

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <conio.h>
#include "resource.h"
#include "network.h"

#define SBUF_SIZE 1024

extern FILE *logstream;

int nipc_isic;
struct IPC_ISIC *ipc_isic;

int compare_ipc2(const void *, const void *);
int compare_isic(const void *, const void *);
int compare_percent(const void *, const void *);
int ipc2_search(struct IPC_ISIC *, int, wchar_t *);

//
// read the file that contains IPC-ISIC Table
//
int read_IPC_ISIC()
{
	FILE *istream;
	int cnt;
	wchar_t line[SBUF_SIZE];
	wchar_t ps[100];

	if (_wfopen_s(&istream, L"IPC-ISIC.txt", L"r") != 0)	
		return MSG_WOSFILE_NOTFOUND;
	if(fgetws(line, SBUF_SIZE, istream) == NULL)	// the 1st line should have code label information
		return UNKNOWN_DATA;
	if (line[0] == '\n' || line[0] == '\r')
		return UNKNOWN_DATA;

	// 1st pass count the number of ipcs
	nipc_isic = 0;
	while (TRUE)
	{		
		if(fgetws(line, SBUF_SIZE, istream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r')
			continue;
		nipc_isic++;
	}

	// allocate memory for the alias table
	ipc_isic = (struct IPC_ISIC *)malloc(nipc_isic * sizeof(struct IPC_ISIC));
	if (ipc_isic == NULL) return MSG_NOT_ENOUGH_MEMORY;

	// 2nd pass, read-in the data
	rewind(istream);	// back to the begining of the file
	fgetws(line, SBUF_SIZE, istream); // skip the 1st line
	cnt = 0;
	while (TRUE)
	{		
		if(fgetws(line, SBUF_SIZE, istream) == NULL)
			break;
		if (line[0] == '\n' || line[0] == '\r')
			continue;
		swscanf(line, L"%s\t%s\t%s", &ipc_isic[cnt].isic, &ipc_isic[cnt].ipc, ps);
		ipc_isic[cnt].percent = _wtof(ps);
		//fwprintf(logstream, L"** %s\t%s\t%f\n", ipc_isic[cnt].isic, ipc_isic[cnt].ipc, ipc_isic[cnt].percent);
		cnt++;
	}

	qsort((void *)ipc_isic, (size_t)nipc_isic, sizeof(struct IPC_ISIC), compare_ipc2);	
	
	return 0;
}

//
// find the corresponding ISICs for a given patent
//
int find_ISIC(int ndx, struct USPTO *usptox)
{
	int i, j, k;
	int nisic;

	nisic = 0;
	for (k = 0; k < usptox[ndx].nipc; k++)
	{
		j = ipc2_search(ipc_isic, nipc_isic, &usptox[ndx].ipc[MAX_IPC_CODE*k]);
		if (j < 0)
		{
			fwprintf(logstream, L"WARNING: %s finds no corresponding ISIC code.\n", &usptox[ndx].ipc[MAX_IPC_CODE*k]);
			continue;
		}
		else	// found the matching IPC code in the ipc_isic table
		{
			wcscpy(&usptox[ndx].isic[nisic*MAX_ISIC_CODE], ipc_isic[j].isic);
			usptox[ndx].isic_percent[nisic] = ipc_isic[j].percent;
			//fwprintf(logstream, L"###%s [%s => %s %.2f]\n", usptox[ndx].pid, &usptox[ndx].ipc[MAX_IPC_CODE*k], ipc_isic[j].isic, ipc_isic[j].percent);
			nisic++;
			j++;
			while (wcscmp(ipc_isic[j].ipc, &usptox[ndx].ipc[MAX_IPC_CODE*k]) == 0)	// get the rest of ISIC codes for this IPC
			{
				wcscpy(&usptox[ndx].isic[nisic*MAX_ISIC_CODE], ipc_isic[j].isic);
				usptox[ndx].isic_percent[nisic] = ipc_isic[j].percent;
				//fwprintf(logstream, L"###%s [%s => %s %.2f]\n", usptox[ndx].pid, &usptox[ndx].ipc[MAX_IPC_CODE*k], ipc_isic[j].isic, ipc_isic[j].percent);
				nisic++; j++;
			}
		}
	}
	usptox[ndx].nisic = nisic;

	// next, consolidate the same ISIC codes
	wchar_t prev_isic[MAX_ISIC_CODE];
	struct IPC_ISIC tmp_isic[50];
	double percent;
	int ndup;
	for (k = 0; k < usptox[ndx].nisic; k++)	// move to a temporary array
	{
		wcscpy(tmp_isic[k].isic, &usptox[ndx].isic[k*MAX_ISIC_CODE]);
		tmp_isic[k].percent = usptox[ndx].isic_percent[k];
	}
	// sort and consolidate duplicate ISIC codes
	qsort((void *)tmp_isic, (size_t)nisic, sizeof(struct IPC_ISIC), compare_isic);
	prev_isic[0] = '\0';
	k = 0; ndup = 1; percent = tmp_isic[0].percent;
	for (i = 0; i < nisic; i++)
	{
		if (wcscmp(tmp_isic[i].isic, prev_isic) != 0)	// hit a new isic
		{
			wcscpy_s(tmp_isic[k].isic, MAX_ISIC_CODE, tmp_isic[i].isic); 
			wcscpy_s(prev_isic, MAX_ISIC_CODE, tmp_isic[i].isic); 
			if (k > 0)
			{
				tmp_isic[k-1].percent = percent / ndup;
				percent = tmp_isic[i].percent;
				ndup = 1; 
			}
			k++;
		}
		else
		{
			percent += tmp_isic[i].percent;
			ndup++;
		}
	}
	wcscpy_s(tmp_isic[k].isic, MAX_ISIC_CODE, prev_isic); 
	tmp_isic[k-1].percent = percent / ndup;
	usptox[ndx].nisic = k;

	// sort again according to percent
	qsort((void *)tmp_isic, (size_t)k, sizeof(struct IPC_ISIC), compare_percent);

	for (k = 0; k < usptox[ndx].nisic; k++)
	{
		wcscpy_s(&usptox[ndx].isic[k*MAX_ISIC_CODE], MAX_ISIC_CODE, tmp_isic[k].isic); 
		usptox[ndx].isic_percent[k] = tmp_isic[k].percent;
	}

	return 0;
}

//
// use binary search to find the proper position of an ipc code in an "IPC_ISIC" array
//
int ipc2_search(struct IPC_ISIC d[], int num, wchar_t *str)
{
	int low, high, cur;

	if (num == 0) return -1;
	if (wcscmp(str, d[0].ipc) < 0)	return -1;

	low = 0;
	high = num;
	for (;;)
	{
		cur = (low + high) / 2;
		if (wcscmp(str, d[cur].ipc) == 0)
			return cur;
		if (cur == low)
		{
			if (wcscmp(str, d[high].ipc) == 0)
				return high;
			else 
				return -1;
		}
		else if (wcscmp(str, d[cur].ipc) > 0)
			low = cur;
		else
			high = cur;
	}
}

//
// this fucntion is to be called by qsort() only
// 
int compare_ipc2(const void *n1, const void *n2)
{
	struct IPC_ISIC *t1, *t2;
	
	t1 = (struct IPC_ISIC *)n1;
	t2 = (struct IPC_ISIC *)n2;
	if (wcscmp(t2->ipc, t1->ipc) < 0)
		return 1;
	else if (wcscmp(t2->ipc, t1->ipc) == 0)
		return 0;
	else return -1;
}


//
// this fucntion is to be called by qsort() only
// 
int compare_isic(const void *n1, const void *n2)
{
	struct IPC_ISIC *t1, *t2;
	
	t1 = (struct IPC_ISIC *)n1;
	t2 = (struct IPC_ISIC *)n2;
	if (wcscmp(t2->isic, t1->isic) < 0)
		return 1;
	else if (wcscmp(t2->isic, t1->isic) == 0)
		return 0;
	else return -1;
}


//
// this fucntion is to be called by qsort() only
// 
int compare_percent(const void *n1, const void *n2)
{
	struct IPC_ISIC *t1, *t2;
	
	t1 = (struct IPC_ISIC *)n1;
	t2 = (struct IPC_ISIC *)n2;
	if (t1->percent < t2->percent)
		return 1;
	else if (t1->percent == t2->percent)
		return 0;
	else return -1;
}