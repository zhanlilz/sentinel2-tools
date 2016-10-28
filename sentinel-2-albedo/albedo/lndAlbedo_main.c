/*
 * !Description
 *   Landsat albedo retrieval from lndSR and the concurrent high quality MODIS 500m BRDF
 *
 * !Usage
 *   lndAlbedo_main.exe <input_parameter_file>
 * 
 *   A sample input parameter file looks like:

 PARAMETER_FILE
 
 # define parameters for input mid-resolution DN image
 DM_CLSMAP=
 DM_FILE_NAME =
 DM_NBANDS =
 DM_NCLS = 
 DM_PIXEL_SIZE =
 CLS_FILL_VALUE =

# in gctp projection number 
 DM_PROJECTION_PARAMETERS=
 DM_PROJECTION_UNIT = 
 
 # MODIS input file
 MODIS_BRDF_FILE = 
 MODIS_QA_FILE = 
 AMONGclsDIST_FILE_NAME =

 #lndalbedo output file
 OUTPUT_ALBEDO=
 
 END
 *
 * !Developer
 *  Initial version (V1.0.0) Yanmin Shuai (Yanmin.Shuai@nasa.gov)
 *						and numerous other contributors
 *						(thanks to all)
 *
 * !Revision History
 * 
 * Oct/2012 Yanmin Shuai: Revised for the threshold to determine the "pure" pixel.
 *
 * Sep/2012 Yanmin Shuai: Add the new  N2B coefficients updated by Tao He 
 *                         in September 2012, using ~250 spectrum libarary borrowed 
 *	                     from USGS & ASTER spectrum libararies, to produce visible-,   
 *				  NIR-, and shortwave-broadband albedos.
 *
 * Sep/2012 Yanmin Shuai: Distingish the forward and backward direction before 
 *			starting View Zenith Angle correction. And the instantaneous        
 *		      directional reflectance used in the calculation of Albedo-to-directional 
 *			Ref ratio.
 *
 * May/2012 Yanmin Shuai: Revised for the minor unsupervised classes whose
 *			 highest majority_pct is lower than the given threshold, for these 
 *			 classes, BRDFs from the most close class in the spectral space is      
 *			  borrowed in the A/N ratio calculation. 
 *
 * Dec/18/2011, Yanmin Shuai: Revised for the view zenith angle effect 
 *                       		    (+/-7.5degree within Landsat scene) 
 *
 * 2009/Oct, by Yanmin Shuai: Revised for the LandSat 30m albedo generation 
 *
 *
 *!Team-unique header:
 * This system (research version) was developed for NASA TE project "Albedo  
 *	   Trends Related to Land CoverChange and Disturbance: A Multi-sensor   
 *	   Approach", and is adopted by USGS Landsat science team (2012-2017) 
 *	   to produce the long-term land surface albedo dataset over North America
 *	   at U-Mass Boston (PI: Crystal B. Schaaf, Co-Is:Zhongping Lee and Yanmin Shuai). 
 * 
 * !References and Credits
 * Principal Investigator: Jeffrey G. masek (Jeffrey.G.Masek@nasa.gov)
 *					 Code 618, Biospheric Science Laboratory, NASA/GSFC.
 *
 * Please contact us with a simple description of your application, if you have any interest.
 *		  
 * Reference paper: Shuai, Y., et al., An algorithm for the retrieval of 30-m snow-free albedo 
 *				from Landsat surface reflectance and MODIS BRDF,
 *				Remote Sensing of Environment, 115, 2204-2216, 2011.	
 *                           
 */
#include <assert.h>
#include "lndsr_albedo.h"
#include "space.h"

int
main (int argc, char *argv[])
{
	DIS_LANDSAT *sensor;
	GRID_MODIS *modis;

	/* allocate memory for variables */
	sensor = malloc (sizeof (DIS_LANDSAT));
	modis = malloc (sizeof (GRID_MODIS));

	//fprintf(stderr,"\n- parsing parameters and extract sensor and MODIS metadata\n");
	if (argc == 2)
		/* get input parameters from input file */
		parseParameters (argv[1], sensor, modis);
	else
		{
			fprintf (stderr, "\nUsage: %s <in_parameters_file>\n", argv[0]);
			exit (1);
		}

	/* get metadata from inputs */
	if (openSensorFile (sensor) == FAILURE)
		{
			fprintf (stderr, "\nmain: Retrieve Sensor metadata error\n");
			exit (1);
		}

	/*temporally output view azimuth map */
	/*sensor->fpVAA=fopen("test_lndView_azimuth.bin","wb");
	   if (sensor->fpVAA==NULL){
	   fprintf(stderr, "\ngetSensorMetaInfo: Error in creat test_lndView_azimuth file\n");
	   exit(1);
	   } */

	//assert(0 == subset_modis(sensor, modis));

	// retrieve meta data from MODIS surface reflectance
	if (getModisMetaInfo (modis) == FAILURE)
		{
			fprintf (stderr, "\nmain: Retrieve MODIS metadata error\n");
			exit (1);
		}

	/*Distance matrix among Classes */
	alloc_2dim_contig ((void ***) (&(sensor->AmongClsDistance)),
										 sensor->init_ncls + 1, sensor->init_ncls + 1,
										 sizeof (double));

	/* Alloc memory for averaged high-quality MODIS BRDF parameters */
	alloc_3dim_contig ((void ****) (&(sensor->BRDF_paras)),
										 sensor->init_ncls + 1, MODIS_NBANDS, 3, sizeof (double));

	/* Allocate memory for one Row output land SW-albedo, QA, and spectral albedo */
	alloc_1dim_contig ((void **) (&(sensor->OneRowlndSWalbedo)),
										 sensor->ncols * 2, sizeof (int16));
	alloc_1dim_contig ((void **) (&(sensor->OneRowlndQA)), sensor->ncols,
										 sizeof (int8));
	alloc_2dim_contig ((void ***) (&(sensor->OneRowlndSpectralAlbedo)), 6,
										 sensor->ncols * 2, sizeof (int16));

	/* Allocate memory to load sensor data and cloud mask */
	alloc_1dim_contig ((void **) (&sensor->Clsdata), sensor->ncols,
										 sizeof (int8));

	/* Allocate memory for one Row input lndSR, atmo,lndSRQA */
	alloc_2dim_contig ((void ***) (&(sensor->OneRowlndSR)), SENSOR_BANDS,
										 sensor->ncols, sizeof (uint16));
	alloc_1dim_contig ((void **) (&(sensor->OneRowlndSRQAData)), sensor->ncols,
										 sizeof (uint16));
	alloc_1dim_contig ((void **) (&(sensor->OneRowlndSRAtmoData)),
										 sensor->ncols, sizeof (int16));
	alloc_1dim_contig ((void **) (&(sensor->OneRowlndSRCldData)), sensor->ncols,
										 sizeof (uint16));
	alloc_1dim_contig ((void **) (&(sensor->OneRowlndSRSnwData)), sensor->ncols,
										 sizeof (uint16));

/*Yanmin 2012/06 read out the spectral mean values of each band, and calculate the among-classes-distance */
	if (getAmongClassesDistance (sensor) == FAILURE)
		{
			fprintf (stderr,
							 "\nmain: Calculate the among-classes-distance error\n");
			exit (1);
		}

/*Get the formula */
/*if(GetEquation4VZACorrection(sensor)!=SUCCESS){
   fprintf(stderr, "\nmain(): fail to GetEquation4VZACorrection()\n");
   exit(1);
}

if(CalcScanCenters(sensor)!=SUCCESS){
   fprintf(stderr, "\nmain(): fail to CalcScanCenters()\n");
   exit(1);
}*/

	/* process samples */
	//fprintf(stderr,"\n- compute albedo for the large disturbance areas\n");
	if (ReprojectLnd2MODIS (sensor, modis) == FAILURE)
		{
			fprintf (stderr, "\nmain: fail to ReprojectLnd2MODIS()\n");
			exit (1);
		}

	if (PrepareCalculation (sensor) == FAILURE)
		{
			fprintf (stderr, "\nmain: fail to ReprojectLnd2MODIS()\n");
			exit (1);
		}

	/*Yanmin 2012/05 write out a head file for the lndAlbedo */
	if (WriteOutHeadFile (sensor) == FAILURE)
		{
			fprintf (stderr,
							 "\nmain: Failed to write out the head files for lndAlbedo.bin and lndQA.bin\n");
			exit (1);
		}

	/****** free memory and file handlers ******/
	if ((cleanUpModis (modis)) == FAILURE)
		{
			fprintf (stderr, "\nmain: Cleanup Modis file error\n");
			exit (1);
		}

	if ((cleanUpSensor (sensor)) == FAILURE)
		{
			fprintf (stderr, "\nmain: Cleanup sensor file error\n");
			exit (1);
		}

	//fclose (sensor->fpVAA);
	free_2dim_contig ((void **) sensor->AmongClsDistance);
	free_2dim_contig ((void **) sensor->OneRowlndSR);
	free_2dim_contig ((void **) sensor->OneRowlndSpectralAlbedo);
	free_3dim_contig ((void ***) sensor->BRDF_paras);
	free_1dim_contig ((void *) sensor->OneRowlndQA);
	free_1dim_contig ((void *) sensor->OneRowlndSWalbedo);
	free_1dim_contig ((void *) sensor->OneRowlndSRAtmoData);
	free_1dim_contig ((void *) sensor->OneRowlndSRQAData);
	free_1dim_contig ((void *) sensor->OneRowlndSRCldData);
	free_1dim_contig ((void *) sensor->OneRowlndSRSnwData);

	free (modis);
	free (sensor);

	fprintf (stderr, "\n Program ends successfully for %s\n", argv[1]);
	return 0;

}

int
subset_modis (DIS_LANDSAT * sensor, GRID_MODIS * modis)
{
	if (0 !=
			SetupSpace (sensor->insys, sensor->inzone, sensor->inparm,
									sensor->indatum, sensor->ulx, sensor->uly,
									(double) sensor->res))
		{
			printf
				("set up sensor space failed. sys=%d, zone=%d, datum=%d, ulx=%f, uly=%f, res=%f\n",
				 sensor->insys, sensor->inzone, sensor->indatum, sensor->ulx,
				 sensor->uly, sensor->res);
			return FAILURE;
		}
	int irow = sensor->nrows / 2;
	int icol = sensor->ncols / 2;
	double lat, lon;
	if (0 != FromSpace ((double) irow, (double) icol, &lat, &lon))
		{
			printf ("from space failed.\n");
			return FAILURE;
		}
	printf ("Landsat nrows=%d, ncols=%d\n", sensor->nrows, sensor->ncols);
	printf ("Landsat scene center: lat=%f, lon=%f, Date: %d-%03d\n", lat, lon,
					sensor->scene.year, sensor->scene.doy);
	char cmd[1024];
	char *dir = "/neponset/nbdata05/albedo/qsun/V6_OUT";
	char *exe = "/home/qsun/code/mosaic_cut_modis/sub";
	char out[1024];
	char out_qa[1024];
	sprintf (out, "./runtime/mcd43a1_sub_A%d%03d_lat%f_lon%f.hdf",
					 sensor->scene.year, sensor->scene.doy, lat, lon);
	sprintf (out_qa, "./runtime/mcd43a2_sub_A%d%03d_lat%f_lon%f.hdf",
					 sensor->scene.year, sensor->scene.doy, lat, lon);
	sprintf (cmd, "%s %s MCD43A1 %f %f %d %d %s", exe, dir, lat, lon,
					 sensor->scene.year, sensor->scene.doy, out);
	assert (0 == system (cmd));
	sprintf (cmd, "%s %s MCD43A2 %f %f %d %d %s", exe, dir, lat, lon,
					 sensor->scene.year, sensor->scene.doy, out_qa);
	assert (0 == system (cmd));

	sprintf (modis->srName, out);
	sprintf (modis->qaName, out_qa);

	return 0;
}
