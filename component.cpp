//
// component.cpp
//

#include "stdafx.h"
#include <malloc.h>
#include <wchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "resource.h"
#include "network.h"

int n_components;
struct COMP *cp;
int comp_largest;
int nd_count;

extern FILE *logstream;

int mark_components(int, int, int, struct PN *);

//
// find components
//
int find_components(int nnodes, struct PN *nw)
{
	int i, j;
	int count;
	int level;

	// allocate memory for component information
	cp = (struct COMP *)Jmalloc(MAX_COMP * sizeof(struct COMP), L"find_components: cp");
	if (cp == NULL) return MSG_NOT_ENOUGH_MEMORY;

	// comp = 0 : not assigned, comp = 1 is most likely the largest component
	// initialize component structure
	for (i = 0; i < MAX_COMP; i++) cp[i].nnodes = 0;
	for (i = 0; i < nnodes; i++) nw[i].comp = 0;

	// marking component (depth-first algorithm) 
	n_components = 0;
	nd_count = 0;
	for (i = 0; i < nnodes; i++)
	{
		if (nw[i].comp == 0)	/* work only on those boards that are not checked */
		{
			level = 0;
			mark_components(n_components, level, i, nw);	
			cp[n_components].nnodes = nd_count;
			nd_count = 0;
			n_components++;
		}
	}
	comp_largest = 0;
	for (i = 0; i < n_components; i++)
	{
		if (cp[i].nnodes > cp[comp_largest].nnodes)
			comp_largest = i;
	}

	return 0;
}


//
// mark components
// components are nodes that linked together
// this function is recurrsive, that is, it calls itself
// Note: two global variables are used to accumulate node count for each component
// IMPORTANT: for large database, it may need to increase the STACK size (system default is only 1MB)
//            To increase stack size, use linker option "/stack:0x200000" (2MB)
//
int mark_components(int cm, int level, int curn, struct PN *nw)
{
	int j, k;
	int lvl;
	int nbhr;

	if (nw[curn].comp == 0)			// first time to check this node
	{
		nw[curn].comp = cm + 1;		// mark component, id is counted from 1 to ... 
		nd_count++;					// this is a global variable 
	}
	else
		return 0;					// this node is checked before, no need to duplicate the effort

	lvl = level;
	for (j = 0; j < nw[curn].degree; j++)
	{
		nbhr = nw[curn].nbrs[j];
		//if (nw[nbhr].comp == 0)		// first time to check this director
		//{
		//	nw[nbhr].comp = cm + 1;	// mark component
		//	nd_count++;				// this is a global variable
		//}
		//else
		//	continue;	// this director is checked before, no need to duplicate the effort
		lvl++;
		mark_components(cm, lvl, nbhr, nw);
	}

	return 0;
}
