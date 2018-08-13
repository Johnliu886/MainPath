//
// statistics.cpp
//

//
// Revision History:
// 2016/06/29 Created
//

#include "stdafx.h"
#include <string.h>
#include <wchar.h>
#include <math.h>
#include "network.h"

extern FILE *logstream;

//
// calculate basic statistics of an array
//
int array_statistics(int n, double *a, struct ARRAY_STATS *stats)
{
	int i;
	double mean, sum, sumsqr;
	double min, max;

	min =  9999999999999999999.9;
	max = -9999999999999999999.9;
	sum = 0.0;
	for (i = 0; i < n; i++)
	{
		sum += a[i];
		if (a[i] > max) max = a[i];
		if (a[i] < min) min = a[i];
	}
	mean = sum / n;

	sumsqr = 0.0;
	for (i = 0; i < n; i++)
		sumsqr += (a[i] - mean) * (a[i] - mean);

	stats->mean = mean;
	stats->stdev = sqrt(sumsqr / n);
	stats->min = min;
	stats->max = max;

	return 0;
}
