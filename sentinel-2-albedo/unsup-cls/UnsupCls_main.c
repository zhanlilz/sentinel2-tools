/**********************************************************

!Description:
Main function for the unsupervised classification processing. 

!Input Parameters:
command line parameter file input required

	PARAMETER_FILE
	#define parameters for the image to be classified
	FILE_NAME =
	NBANDS =
	NCOLS =
	NROWS =
	
	#define parameters for the unsupervised classification
	NCLASSES =
	THRS_RAD =
	I_CLUSTER =
	EXCL_VAL = 
	
	END.

!Output Parameters:
none

!Returns:
none

!Revision History:
Original version-UnsupervisedCls v1.0 (Earth Resources Technology Inc.) 
		developed for the NASA project "North American Forest 
		Dynamics via Integration of ASTER, MODIS, and Landsat 
		Reflectance Data". In this package, the ustats routine 
		in IPW 2.0(contributed by Boston University)was referred
		to perform the multivariate statistics and acquire the 
		mean of each cluster.  
		
Revision: 2012/05  Yanmin (ERT Inc. and NASA, Yanmin.Shuai@nasa.gov)
		save the spectral-space-distence between each pair of unsupervised classes into a .txt file

!Team-unique Header:
This software is developed for NASA project under contract #
	Principal Investigators: Jeffrey G. Masek (NASA) 
                                   
Developers: Yanmin Shuai  (ERT Inc. and NASA, Yanmin.Shuai@ertcorp.com)
            and numerous other contributors (thanks to all)

!Design Notes

*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "UnsupCls.h"

/*------------------------------------------------*/
int main (int argc, const char **argv)
{
	/*define local variables */
	int ret;
	int row, col, iband, icls, i, j, mycluster;
	int Cls_No, available_cls;
	int ClsPix_thrshold, max_cls;
	int *finalCls, *finalCls_npix;

	PARM_T parm;
	char *file_name, *p2 = ".hdr";
	char *fname_hdr, tmp[4 * MODERATE_ALLOC];
	FILE *fp, *fp_outmap, *fp_dist, *fp_hdr;

	int8 *OneRow_Clsmap;

	/*Open the parameter file provided in the command line */
	if (argc != 2)
	{
		fprintf (stderr, "\nUsage: %s <input_parameter_file>\n", argv[0]);
		exit (1);
	}

	/*fprintf(stderr, "\nReading input parameters...\n"); */
	file_name = (char *) argv[1];
	if ((fp = fopen (file_name, "r")) == NULL)
	{
		fprintf (stderr, "Can't open parameter file %s in main().\n", file_name);
	}

	/* read parameters in the command line txt and parse it */
	ret = GetParam (fp, &parm);
	if (ret != UNSCLS_SUCCESS)
	{
		fprintf (stderr, "Error in GetParam().");
		exit (1);
	}

	/* open the hdf img, */
	if (OpenLndsrFile (&parm) != UNSCLS_SUCCESS)
	{
		fprintf (stderr, "\nmain:OpenLndsrFile() failed!!");
		exit (1);
	}

	if ((fp_outmap = fopen (parm.Clsmapname, "wb")) == NULL)
	{
		fprintf (stderr, "Can't create output classmap file %s in main().\n", parm.Clsmapname);
	}

	/*Yanmin 2012/05 create a .hdr file for classfication map */
	strncpy (tmp, parm.Clsmapname, strlen (parm.Clsmapname) - 4);
	tmp[strlen (parm.Clsmapname) - 4] = '\0';
	fname_hdr = strcat (tmp, p2);
	if (fname_hdr == NULL)
	{
		fprintf (stderr, "\n Failed at the strncat()!");
		exit (1);
	}
	if ((fp_hdr = fopen (fname_hdr, "w")) == NULL)
	{
		fprintf (stderr, "Can't create hdr file:%s in main().\n", fname_hdr);
	}

	if ((fp_dist = fopen (parm.Distfilename, "w")) == NULL)
	{
		fprintf (stderr, "Can't create output among-class-distance file %s in main().\n", parm.Distfilename);
	}

	if (AllocateEarlyVars (&parm) != UNSCLS_SUCCESS)
	{
		fprintf (stderr, "\n main:AllocateEarlyVars() failed!!");
		exit (1);
	}

	/* initial variables to zero */
	parm.iclusters = 0;
	for (icls = 0; icls < parm.nclusters; icls++)
	{
		parm.c_npixels[icls] = 0;
		for (iband = 0; iband < parm.nbands; iband++)
		{
			parm.mean_x[icls][iband] = 0.0;
			parm.sum_x[icls][iband] = 0.0;
		}
		for (i = 0; i < parm.nbands; i++)
			for (j = 0; j < parm.nbands; j++)
				parm.sum_xy[icls][i][j] = 0.0;
	}

	for (icls = 0; icls < parm.nclasses; icls++)
		parm.c_order[icls] = 0;

        
	/* read the whole img data into the assigned memory */
	ret = ReadLndsr (&parm);
	if (ret == FAILURE)
	{
                fprintf(stderr, "\n main:ReadLndsr() failied!\n"); // Zhan
		exit (1);
	}

	printf ("zhan Classifying...\n");
	/* call the ustats() to acquire the mean vector for each intermedia cluster */
	ustats (&parm);
	/*printf("\n Intermediate iclusters acquired: %d\n\n", parm.iclusters); */

	/* find the clusters with at least 0.001% pixels */
	ClsPix_thrshold = (int) (CLS_PIX * parm.nlines * parm.nsamps);

	available_cls = 0;

	for (i = 0; i < parm.iclusters; i++)
	{
		icls = parm.c_order[i];
		printf("  icls=%d, n=%d\n", icls, parm.c_npixels[icls]);
		if (parm.c_npixels[icls] > ClsPix_thrshold)
			available_cls++;
		else
			parm.c_order[i] = -1;	/*set to -1 when the cluster is dropped */
	}

	if (available_cls <= parm.nclasses)
		parm.nclasses = available_cls;
	else if (MergeCls (&parm) != UNSCLS_SUCCESS)
		fprintf (stderr, "\nMerge Cluster failed!!");

	alloc_1dim_contig ((void **) (&OneRow_Clsmap), parm.nsamps, sizeof (int8));

	/* loop on the whole image to assigne the classID to each pixle */
	for (row = 0; row < parm.nlines; row++)
	{
		for (col = 0; col < parm.nsamps; col++)
		{
			/*initialize the cls# */
			OneRow_Clsmap[col] = -9;

			for (iband = 0; iband < parm.nbands; iband++)
				if (parm.image[row][col][iband] == parm.exclude)
				{
					OneRow_Clsmap[col] = -9;
					break;
				}
			if (iband == parm.nbands)
				OneRow_Clsmap[col] = ShortestDisCls (row, col, &parm);
		}

		/*write out the classmap */
		if (fwrite (OneRow_Clsmap, sizeof (int8), parm.nsamps, fp_outmap) != parm.nsamps)
			printf ("\n Write the %d row failed ", row);
	}

	/*write out the mean values of each cluster */
	fprintf (fp_dist, "#unsupervised classID spectral mean value of each band in TM/ETM+ band series band1-5,7,6.\n ");
	for (mycluster = 0; mycluster < parm.nclasses; mycluster++)
	{
		icls = parm.c_order[mycluster];
		if (icls != -1)
		{
			fprintf (fp_dist, "%d,", mycluster);
			/*printf("\nics=%d,mycluster=%d,npixels=%d",icls, mycluster, parm.c_npixels[icls]); */

			for (iband = 0; iband < parm.nbands; iband++)
				fprintf (fp_dist, "%f,", parm.mean_x[icls][iband]);
			fprintf (fp_dist, "\n");
		}
	}

	/*Yanmin 2012/05 write out a head file (.hdr) in ENVI standard */
	fprintf (fp_hdr, "ENVI\ndescription = {\n  File Imported into ENVI.}");
	fprintf (fp_hdr, "\nsamples = %d\nlines   = %d\nbands   = 1\nheader offset = 0", parm.nsamps, parm.nlines);
	fprintf (fp_hdr,
		"\nfile type = ENVI Standard\ndata type = 1\ninterleave = bsq\nsensor type = Unknown\nbyte order = 0\nwavelength units = Unknown");
  fprintf(fp_hdr,"\nmap info={UTM, 1.000, 1.000, %f, %f, 30.0, 30.0, ",parm.ulx, parm.uly);
  fprintf(fp_hdr," %d, North, WGS-84, units=Meters}",parm.zone);

	printf ("Classification Done.\n");

	/*close input/output files */
	fclose (fp);
	fclose (fp_outmap);
	fclose (fp_dist);
	fclose (fp_hdr);
	ret = CloseLndsr (&parm);

	/* free memory */
	free_3dim_contig ((void ***) parm.image);
	free_3dim_contig ((void ***) parm.sum_xy);
	free_2dim_contig ((void **) parm.sum_x);
	free_2dim_contig ((void **) parm.mean_x);
	free (OneRow_Clsmap);
	free (parm.c_npixels);
	free (parm.c_order);

	return 0;
}
