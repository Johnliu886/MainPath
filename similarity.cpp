//
// simlarity.cpp
//
// Author: John Liu
// 

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "resource.h"
#include "network.h"

static int *in_vec1 = NULL;
static int *in_vec2 = NULL;
static int *out_vec1 = NULL;
static int *out_vec2 = NULL;

//
// find the dissimilarity between two given nodes
// the definition used is the same as d5 defined in Pajek, with the "p" set to 1
//
int dissimilarity(int nnodes, struct PN *nw, int nd1, int nd2, double *dissim)
{
	int i, k;
	double sumsqr_i, sumsqr_o, sumsqr_c;

	if (in_vec1 == NULL)
	{
		in_vec1 = (int *)malloc(nnodes * sizeof(int));
		if (in_vec1 == NULL) return MSG_NOT_ENOUGH_MEMORY;
		in_vec2 = (int *)malloc(nnodes * sizeof(int));
		if (in_vec2 == NULL) return MSG_NOT_ENOUGH_MEMORY;
		out_vec1 = (int *)malloc(nnodes * sizeof(int));
		if (out_vec1 == NULL) return MSG_NOT_ENOUGH_MEMORY;
		out_vec2 = (int *)malloc(nnodes * sizeof(int));
		if (out_vec1 == NULL) return MSG_NOT_ENOUGH_MEMORY;
	}

	for (i = 0; i < nnodes; i++) { in_vec1[i] = in_vec2[i] = out_vec1[i] = out_vec2[i] = 0; }

	for (k = 0; k < nw[nd1].in_deg; k++)
		in_vec1[nw[nd1].in_nbrs[k]] = 1;
	for (k = 0; k < nw[nd2].in_deg; k++)
		in_vec2[nw[nd2].in_nbrs[k]] = 1;

	for (k = 0; k < nw[nd1].out_deg; k++)
		out_vec1[nw[nd1].out_nbrs[k]] = 1;
	for (k = 0; k < nw[nd2].out_deg; k++)
		out_vec2[nw[nd2].out_nbrs[k]] = 1;

	sumsqr_i = sumsqr_o = sumsqr_c = 0.0;
	for (i = 0; i < nnodes; i++)
	{
		if (i == nd1 || i == nd2)
		{
			sumsqr_i += 0.0;
			sumsqr_o += 0.0;
		}
		else
		{
			sumsqr_i += (in_vec1[i] - in_vec2[i]) * (in_vec1[i] - in_vec2[i]);
			sumsqr_o += (out_vec1[i] - out_vec2[i]) * (out_vec1[i] - out_vec2[i]);
		}
	}
	sumsqr_c += (in_vec1[nd1] - in_vec2[nd2]) * (in_vec1[nd1] - in_vec2[nd2]);	// q(u,u)-q(v,v)
	sumsqr_c += (in_vec1[nd2] - in_vec2[nd1]) * (in_vec1[nd2] - in_vec2[nd1]);	// q(u,v)-q(v,u)
	//*dissim = sqrt(sumsqr_i + sumsqr_o + sumsqr_c);
	// normalize the results, assuming the importance weighting for inward, outward and cross-reference are 0.45, 0.45 and 0.1, respectively (these can be subject to modification!!!)
	*dissim = 0.45 * sqrt(sumsqr_i / (nw[nd1].in_deg + nw[nd2].in_deg)) + 0.45 * sqrt(sumsqr_o / (nw[nd1].out_deg + nw[nd2].out_deg) + 0.1 * sumsqr_c);	// normalize

	return 0;
}
