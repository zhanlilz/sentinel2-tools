#include <math.h>

#include "UnsupCls.h"

void
accum (int cndx, int16 * pixel, PARM_T * parm)
{
	int band, band2;
	float p_band;
	double *c_sum_x;
	double *c_mean_x;
	double **c_sum_xy;

	++parm->c_npixels[cndx];

	for (band = 0; band < parm->nbands; band++)
		{
			p_band = (float) pixel[band] * parm->scalefactor;
			parm->sum_x[cndx][band] += p_band;
			for (band2 = 0; band2 <= band; band2++)
				{
					parm->sum_xy[cndx][band][band2] +=
						p_band * pixel[band2] * parm->scalefactor;
				}

			parm->mean_x[cndx][band] =
				parm->sum_x[cndx][band] / parm->c_npixels[cndx];
		}

/*
    c_sum_x = parm->sum_x[cndx];
    c_mean_x = parm->mean_x[cndx];
    c_sum_xy = parm->sum_xy[cndx];

    for (band = 0; band < parm->nbands; ++band) {
	 p_band =(float) pixel[band];
	double   *band_c_sum_xy = *c_sum_xy++;

	for (band2 = 0; band2 <= band; ++band2) {
	    *band_c_sum_xy++ += p_band * pixel[band2];
	}

	*c_sum_x += p_band;
	*c_mean_x++ = *c_sum_x++ / parm->c_npixels[cndx];
    }
*/
}

/**************************************************************/

int
find_cluster (int l, int s, PARM_T * parm)
{
	int i;
	int cfreq = 0;
	int band;
	int iclus;
	int freqndx;
	double dist2;
	double diff;
	double cradius2;

	cclndx = -1;
	cfreqndx = freqndx = -1;
	cradius2 = parm->radius2;

	for (i = 0; i < parm->iclusters; i++)
		{
			iclus = parm->c_order[i];
			if (parm->c_npixels[iclus] != cfreq)
				{
					cfreq = parm->c_npixels[iclus];
					freqndx = i;
				}
			dist2 = 0.0;
			for (band = 0; band < parm->nbands; band++)
				{
					diff =
						parm->mean_x[iclus][band] -
						parm->image[l][s][band] * parm->scalefactor;

					if (fabs (diff) > parm->radius)
						break;
					dist2 += diff * diff;
				}
			if (band < parm->nbands)
				continue;
			if (dist2 <= cradius2)
				{
					cradius2 = dist2;
					cclndx = i;
					cfreqndx = freqndx;
				}
		}

	if (cclndx >= 0)
		return (TRUE);
	else
		return (FALSE);

}

void
add_to_cluster (int16 * pvec, PARM_T * parm)
{
	int cclus = parm->c_order[cclndx];

	accum (cclus, pvec, parm);
	parm->c_order[cclndx] = parm->c_order[cfreqndx];
	parm->c_order[cfreqndx] = cclus;
}

void
make_new_cluster (int16 * pvec, PARM_T * parm)
{

	accum (parm->iclusters, pvec, parm);
	parm->c_order[parm->iclusters] = parm->iclusters;

	parm->iclusters++;
}

int
ipow2 (int expo)								/* exponent     */
{
	int i;
	int result;

	if (expo < 0)
		{
			fprintf (stderr, "2**%d is not an integer", expo);
			return (0);
		}

	if (expo > sizeof (int) * CHAR_BIT)
		{
			fprintf (stderr, "2**%d won't fit in an \"int\"", expo);
			return (0);
		}

	result = 1;
	for (i = 0; i < expo; ++i)
		{
			result <<= 1;
		}

	if (result < 1)
		{
			fprintf (stderr, "2**%d won't fit in an \"int\"", expo);
			return (0);
		}

	return (result);
}

void
start_pixel (int l, int s)
{
	int mdim;

	lines = l;
	samps = s;
	pow2exp = 0;
	for (mdim = MAX (lines, samps) - 1; mdim; mdim >>= 1)
		{
			pow2exp++;
		}
	myindex = 0;

	/* Yanmin: original initialization for pow2squ in IPW source code */
	pow2squ = ipow2 (2 * pow2exp);

	/* Yanmin: this was used in the first version concurrent-approach 2009-2011  */
	/*pow2squ=l*s; */

}

int
next_pixel (pline, psamp)
		 int *pline;
		 int *psamp;
{
	int line;
	int samp;
	int cur;
	int order;

	while (myindex < pow2squ)
		{
			samp = line = 0;
			cur = myindex;
			order = pow2exp;

			while (cur)
				{
					samp <<= 1;
					line <<= 1;
					samp |= cur & 01;
					cur >>= 1;
					line |= cur & 01;
					cur >>= 1;
					order--;
				}
			if (order)
				{
					samp <<= order;
					line <<= order;
				}
			myindex++;

			if (line < lines && samp < samps)
				{
					*pline = line;
					*psamp = samp;
					return UNSCLS_SUCCESS;
				}
			else
				{
					continue;
				}
		}
	return UNSCLS_FAIL;
}

void
mcov (PARM_T * parm)
{
	int iclass;										/* class #           */
	int band;											/* band #            */
	int clus;

	for (iclass = 0; iclass < parm->nclasses && iclass < parm->iclusters;
			 ++iclass)
		{
			clus = parm->c_order[iclass];
			if (parm->c_npixels[clus] <= 0)
				{
					return;
				}

			/*
			 * magic cookie
			 */
			printf ("#<stats>\n\n");

			/*
			 * # bands
			 */
			printf ("%d\n\n", parm->nbands);

			/*
			 * mean vector
			 */
			for (band = 0; band < parm->nbands; ++band)
				{
					printf ("%.*lg%s", DBL_DIG,
									parm->sum_x[clus][band] / parm->c_npixels[clus],
									band == parm->nbands - 1 ? "\n\n" : " ");
				}
/**
  * variance-covariance matrix.  Element [band1][band2] is:
  *
  *                      sum(band1) * sum(band2)
  * sum(band1 * band2) - -----------------------
  *                                 N
  * ---------------------------------------------
  *                       N
  */
			for (band = 0; band < parm->nbands; ++band)
				{
					int band2;

					for (band2 = 0; band2 <= band; ++band2)
						{
							printf ("%.*lg%s", DBL_DIG,
											(parm->sum_xy[clus][band][band2] -
											 ((parm->sum_x[clus][band] *
												 parm->sum_x[clus][band2]) /
												parm->c_npixels[clus])) / parm->c_npixels[clus],
											band2 == band ? "\n" : " ");
						}
				}

			printf ("\n");

			printf ("* %d\n\n", iclass);

			printf ("%d\n\n", parm->c_npixels[clus]);
		}

}

/**************************************************************/

/*
 * Derived from the corresponding file for the "mstats" command
 
 * ustats -- unsupervised cluster-based classification and statistics
 */

void
ustats (PARM_T * parm)
{
	int l = 0;
	int s = 0;
	int i;

	/*
	 * Since we are only considering parm.nclusters clusters, and we
	 * seed each cluster from a single pixel, how we choose those
	 * pixels is important.  Otherwise we run the risk of excluding
	 * a distinct cluster/class representatives of which we encounter
	 * after we have seeded all parm.nclusters clusters.  So
	 * use start_pixel and next_pixel to cover the image in a
	 * pseudo-random walk.
	 */

	start_pixel (parm->nlines, parm->nsamps);

	while (next_pixel (&l, &s) != UNSCLS_FAIL)
		{

			/*printf("\n line=%d, sample=%d, icluster=%d ", l, s, parm->iclusters); */
			for (i = 0; i < parm->nbands; i++)
				{
					if (parm->image[l][s][i] == parm->exclude)
						break;
				}
			if (i < parm->nbands)
				{
					/* pixel does not need to be classified */
					continue;
				}

			if (find_cluster (l, s, parm))
				{
					/* new pixel belongs to a preexisting cluster */
					add_to_cluster (parm->image[l][s], parm);

				}
			else if (parm->iclusters < parm->nclusters)
				{
					/*
					 * new pixel does not belong to a preexisting cluster, but there is
					 * room to create the new cluster
					 */
					make_new_cluster (parm->image[l][s], parm);

				}
			else
				{
					/*
					 * new pixel does not belong to a preexisting cluster, and the
					 * cluster list is full;  ignore
					 */
					continue;
				}
		}

	/*mcov(parm); */
}
