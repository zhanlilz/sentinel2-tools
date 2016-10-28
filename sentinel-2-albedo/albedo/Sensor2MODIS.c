/* 
 * !Description
 *  Get the location of one pixel (row, col) in Landsat scene in MODIS SIN tile via gctp. 
 *	
 * !Usage
 *   parseParameters (filename, ptr_LndSensor, ptr_MODIS)
 * 
 * !Developer
 *  Initial version (V1.0.0) Yanmin Shuai (Yanmin.Shuai@nasa.gov)
 *						and numerous other contributors
 *						(thanks to all)
 *
 * !Revision History
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
 */
#include "lndsr_albedo.h"
#include "space.h"

#define TRUE 1
#define FALSE 0

int g_print = 0;

int
Sensor2MODIS (DIS_LANDSAT * sensor, int irow, int icol, GRID_MODIS * modis,
							int *mrow, int *mcol)
{
	double incoor[2];
	double outcoor[2];
	long ipr = 0;
	long jpr = 999;
	long flg = 1;
	double lon = 0.0, lat = 0.0, x, y;

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
	if (0 != FromSpace ((double) irow, (double) icol, &lat, &lon))
		{
			printf ("from space failed.\n");
			return FAILURE;
		}

	if (0 !=
			SetupSpace (modis->GD_projcode, modis->GD_zonecode, modis->GD_projparm,
									modis->GD_datum, modis->ulx, modis->uly,
									(double) modis->res))
		{
			printf ("set up modis space failed.\n");
			return FAILURE;
		}
	if (0 != ToSpace (lat, lon, &y, &x))
		{
			printf ("to space failed.\n");
			return FAILURE;
		}

	*mrow = (int) y;
	*mcol = (int) x;

	if (*mrow < 0 || *mrow >= modis->nrows || *mcol < 0
			|| *mcol >= modis->ncols)
		{
			if (g_print == 0)
				{
					printf
						("\nWARNING! lrow=%d, lcol=%d, mrow=%d, mcol=%d, mrows=%d, mcols=%d\n",
						 irow, icol, *mrow, *mcol, modis->nrows, modis->ncols);
					printf ("  lat=%f, lon=%f\n", lat, lon);
					printf ("  ULX=%f, ULY=%f, INSYS=%d, INZONE=%d, INDATUM=%d, PARMS=",
									sensor->ulx, sensor->uly, sensor->insys, sensor->inzone,
									sensor->indatum);
					int i;
					for (i = 0; i < 15; i++)
						{
							printf ("%f,", sensor->inparm[i]);
						}
					printf ("\n");
					g_print = 1;
				}
			return FAILURE;
		}
	else
		return SUCCESS;

}
