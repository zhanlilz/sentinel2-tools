#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include "hdf.h"
#include "mfhdf.h"

int copy_hdf (char *hdf_s, int row_s, int col_s, int nrow_s, int ncol_s,
							char *hdf_t, int row_t, int col_t, int nrow_t, int ncol_t);

int
main (int argc, char *argv[])
{
	char *hdf_s = argv[1];
	char *hdf_t = argv[2];
	int row_s = atoi (argv[3]);
	int col_s = atoi (argv[4]);
	int nrow_s = atoi (argv[5]);
	int ncol_s = atoi (argv[6]);
	int row_t = atoi (argv[7]);
	int col_t = atoi (argv[8]);
	int nrow_t = atoi (argv[9]);
	int ncol_t = atoi (argv[10]);

	assert (0 == copy_hdf (hdf_s, row_s, col_s, nrow_s, ncol_s,
												 hdf_t, row_t, col_t, nrow_t, ncol_t));

	return 0;
}

int
copy_hdf (char *hdf_s, int row_s, int col_s, int nrow_s, int ncol_s,
					char *hdf_t, int row_t, int col_t, int nrow_t, int ncol_t)
{
	int32 start_s[3] = { row_s, col_s, 0 };
	int32 edge_s[3] = { nrow_s, ncol_s, 3 };

	int32 start_t[3] = { row_t, col_t, 0 };
	int32 edge_t[3] = { nrow_t, ncol_t, 3 };

	int32 id_s = SDstart (hdf_s, DFACC_READ);
	assert (id_s > 0);

	int32 id_t = SDstart (hdf_t, DFACC_RDWR);
	assert (id_t > 0);

	int i;
	int32 rank, dtype, dimsizes[10];
	int32 nsds, nattr;
	char sds_name[100];
	int32 stat;

	assert (SUCCEED == SDfileinfo (id_s, &nsds, &nattr));

	for (i = 0; i < nsds; i++)
		{
			int32 sds_s = SDselect (id_s, i);
			assert (sds_s > 0);
			assert (SUCCEED ==
							SDgetinfo (sds_s, sds_name, &rank, dimsizes, &dtype, &nattr));

			int32 index = SDnametoindex (id_t, sds_name);
			//assert(index >= 0);
			if (index < 0)
				{
					printf ("Warning: skip sds \"%s\".\n", sds_name);
					continue;
				}

			int32 sds_t = SDselect (id_t, index);
			assert (sds_t > 0);

			int32 dsize = DFKNTsize (dtype);
			//printf("sds=%d, sds_out=%d, index=%d, hdfid0=%d, hdfid=%d\n", sds, sds_out, index, hdfid0, hdfid);
			//printf("%d, %d, %d\n", dsize,edge_s[0],edge_s[1]);
			void *buf = malloc (dsize * nrow_s * ncol_s * 3);
			//char buf[dsize*edge_s[0]*edge_s[1]];
			assert (buf != NULL);

			stat = SDreaddata (sds_s, start_s, NULL, edge_s, buf);
			if (stat != SUCCEED)
				{
					HEprint (stdout, 0);
				}
			assert (SUCCEED == stat);
			stat = SDwritedata (sds_t, start_t, NULL, edge_t, buf);
			if (stat != SUCCEED)
				{
					HEprint (stdout, 0);
					printf ("Write error, start=%d, %d, edge=%d, %d, sds=%d, stat=%d\n",
									start_t[0], start_t[1], edge_t[0], edge_t[1], sds_t, stat);
				}
			assert (SUCCEED == stat);

			free (buf);

			SDendaccess (sds_s);
			SDendaccess (sds_t);
		}

	SDend (id_s);
	SDend (id_t);

	return 0;
}
