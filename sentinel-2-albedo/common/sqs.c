#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include "sqs.h"
#include "hdf.h"
#include "mfhdf.h"
#include "HdfEosDef.h"

int b_init = 0;

void
init_gdal ()
{
	if (!b_init)
		{
			GDALAllRegister ();
		}
	b_init = 1;
}

int
read_one_row_gdal (GDALDatasetH hDataset, int iBand, int iRow, void *buf)
{
	if (buf == NULL)
		{
			return -1;
		}

	int nband = GDALGetRasterCount (hDataset);
	if (nband < 1)
		{
			return -2;
		}

	if (iBand < 0 || iBand >= nband)
		{
			printf ("Illegal band number: %d\n", iBand);
			return -3;
		}

	GDALRasterBandH hBand = GDALGetRasterBand (hDataset, iBand + 1);

	int nrow = GDALGetRasterBandYSize (hBand);
	int ncol = GDALGetRasterBandXSize (hBand);

	if (iRow < 0 || iRow >= nrow)
		{
			printf ("Illegal row number: %d, %d rows\n", iRow, nrow);
			return -4;
		}

	GDALDataType dtype = GDALGetRasterDataType (hBand);

	if (CE_None !=
			GDALRasterIO (hBand, GF_Read, 0, iRow, ncol, 1, buf, ncol, 1, dtype, 0,
										0))
		{
			printf ("Read band %d row %d failed.\n", iBand, iRow);
			return -5;
		}

	return 0;
}

int
read_one_band_gdal (GDALDatasetH hDataset, int iBand, void *buf)
{
	if (buf == NULL)
		{
			return -1;
		}

	int nband = GDALGetRasterCount (hDataset);
	if (nband < 1)
		{
			return -2;
		}

	if (iBand < 0 || iBand >= nband)
		{
			printf ("Illegal band number: %d\n", iBand);
			return -3;
		}

	GDALRasterBandH hBand = GDALGetRasterBand (hDataset, iBand + 1);

	int nrow = GDALGetRasterBandYSize (hBand);
	int ncol = GDALGetRasterBandXSize (hBand);

	GDALDataType dtype = GDALGetRasterDataType (hBand);

	if (CE_None !=
			GDALRasterIO (hBand, GF_Read, 0, 0, ncol, nrow, buf, ncol, nrow, dtype,
										0, 0))
		{
			printf ("Read band %d failed.\n", iBand);
			return -5;
		}

	return 0;
}

int
get_geo_info_gdal (GDALDatasetH hDataset, long *sys, long *zone, long *datum,
									 double parm[], double *ulx, double *uly, double *res)
{
	// proj info
	char *strProj = GDALGetProjectionRef (hDataset);
	if (strProj == NULL)
		{
			printf ("No proj. info found.\n");
			return -1;
		}

	OGRSpatialReferenceH hSRS = OSRNewSpatialReference (strProj);
	if (OSRImportFromWkt (hSRS, &strProj) != CE_None)
		{
			printf ("Import projection failed.\n");
			return -2;
		}

	// note: must let GDAL to allocate the *par
	double *_parm;
	if (OSRExportToUSGS (hSRS, sys, zone, &_parm, datum) != CE_None)
		{
			printf ("Export proj failed.\n");
			return -3;
		}

	int i;
	for (i = 0; i < 15; i++)
		{
			parm[i] = _parm[i];
		}

	free (_parm);

	double adfGeoTransform[6];
	if (GDALGetGeoTransform (hDataset, adfGeoTransform) != CE_None)
		{
			printf ("Get origin failed.\n");
			return -4;
		}

	*ulx = adfGeoTransform[0];
	*uly = adfGeoTransform[3];
	*res = adfGeoTransform[1];

	OSRDestroySpatialReference (hSRS);

	return 0;
}

int
get_geo_info_hdf (char *fname, long *sys, long *zone, long *datum,
									double parm[], double *ulx, double *uly, double *res)
{
	int32 gfid = GDopen (fname, DFACC_READ);
	if (gfid <= 0)
		{
			printf ("Open %s failed.\n", fname);
			return -1;
		}

	int32 gid = GDattach (gfid, "Grid");
	if (gid <= 0)
		{
			printf ("Attach %s failed.\n", fname);
			return -2;
		}

	float64 GD_projparm[15];

	int32 _sys, _zone, _datum;
	int32 ret = GDprojinfo (gid, &_sys, &_zone, &_datum, GD_projparm);
	if (ret != SUCCEED)
		{
			return -3;
		}
	*sys = _sys;
	*zone = _zone;
	*datum = _datum;

	int i;
	for (i = 0; i < 15; i++)
		{
			parm[i] = GD_projparm[i];
		}

	int32 nrow, ncol;
	float64 GD_upleft[2];
	float64 GD_lowright[2];

	ret = GDgridinfo (gid, &ncol, &nrow, GD_upleft, GD_lowright);
	if (ret != SUCCEED)
		{
			return -4;
		}
	*ulx = (double) GD_upleft[0];
	*uly = (double) GD_upleft[1];

	*res = (GD_lowright[0] - GD_upleft[0]) / (double) ncol;

	GDdetach (gid);
	GDclose (gfid);
	return 0;
}

int
find_next_element (char *strXml, char *element)
{
	int len = strlen (strXml);
	char strTmp1[1024];
	sprintf (strTmp1, "<%s>", element);
	int len1 = strlen (strTmp1);

	char strTmp2[1024];
	sprintf (strTmp2, "<%s ", element);
	int len2 = strlen (strTmp2);

	int i;
	for (i = 0; i < len; i++)
		{
			if (strncmp (&(strXml[i]), strTmp1, len1) == 0)
				{
					return i + len1;
				}
			if (strncmp (&(strXml[i]), strTmp2, len2) == 0)
				{
					return i + len2;
				}
		}

	return -1;
}

int
get_element_value (char *strXml, char *element, char *property, char *value)
{
	int i, j;
	int p = 0;
	int len = strlen (strXml);
	char strTmp[1024];
	sprintf (strTmp, "%s=", property);
	int len2 = strlen (strTmp);

	int a = find_next_element (strXml, element);
	if (a < 0)
		{
			return -1;
		}

	if (property == NULL || strlen (property) < 1)
		{
			for (i = a; i < len; i++)
				{
					if (strXml[i] == '<')
						{
							value[p] = '\0';
							return 0;
						}
					value[p++] = strXml[i];
				}
		}
	else
		{
			for (i = a; i < len; i++)
				{
					if (strncmp (&(strXml[i]), strTmp, len2) == 0)
						{
							for (j = i + len2 + 1; j < len; j++)
								{
									if (strXml[j] == '"')
										{
											value[p] = '\0';
											return 0;
										}
									value[p++] = strXml[j];
								}
						}
				}
		}

	return -2;
}

int
get_srf_band_index (int instrument, char *bname)
{
	char str_srf_bands_tm[6][20] =
		{ "sr_band1", "sr_band2", "sr_band3", "sr_band4", "sr_band5",
		"sr_band7"
	};
	char str_srf_bands_oli[6][20] =
		{ "sr_band2", "sr_band3", "sr_band4", "sr_band5", "sr_band6",
		"sr_band7"
	};

	int i;
	for (i = 0; i < N_SRF_BAND; i++)
		{
			if (instrument == _OLI_)
				{
					if (strcmp (str_srf_bands_oli[i], bname) == 0)
						{
							return i;
						}
				}
			else
				{
					if (strcmp (str_srf_bands_tm[i], bname) == 0)
						{
							return i;
						}
				}
		}

	return -1;
}

int
parse_usgs_xml (char *fname, usgs_t * scene)
{
	int i;
	int index;
	long max_xml_size = 1024 * 1024 * 10;
	FILE *fp = fopen (fname, "r");
	if (fp == NULL)
		{
			printf ("Open %s failed.\n", fname);
			return -1;
		}

	init_gdal ();

	strcpy (scene->xml_file, fname);

	char *strXml = (char *) malloc (sizeof (char) * max_xml_size);

	int n = fread (strXml, 1, max_xml_size, fp);
	strXml[n] = '\0';

	char strTmp[1024];
	char dir[1024];

	strcpy (dir, fname);
	for (i = strlen (dir) - 1; i > 0; i--)
		{
			if (dir[i] == '/')
				{
					dir[i] = '\0';
					break;
				}
		}

	//
	int a;
	a = find_next_element (strXml, "global_metadata");
	if (a < 0)
		{
			free (strXml);
			return -2;
		}

	if (0 !=
			get_element_value (&(strXml[a]), "satellite", NULL, scene->satellite))
		{
			free (strXml);
			return -3;
		}

	if (0 != get_element_value (&(strXml[a]), "instrument", NULL, strTmp))
		{
			free (strXml);
			return -3;
		}
	if (strcmp (strTmp, "TM") == 0)
		{
			scene->instrument = _TM_;
		}
	else if (strcmp (strTmp, "ETM") == 0)
		{
			scene->instrument = _ETM_;
		}
	else if (strcmp (strTmp, "OLI_TIRS") == 0)
		{
			scene->instrument = _OLI_;
		}
	else
		{
			printf ("What's the instrument? %s\n", strTmp);
			free (strXml);
			return -5;
		}

	if (0 != get_element_value (&(strXml[a]), "solar_angles", "zenith", strTmp))
		{
			free (strXml);
			return -3;
		}
	scene->szn = atof (strTmp);

	if (0 !=
			get_element_value (&(strXml[a]), "solar_angles", "azimuth", strTmp))
		{
			free (strXml);
			return -3;
		}
	scene->san = atof (strTmp);

	// first corner_point is ul     
	if (0 != get_element_value (&(strXml[a]), "corner_point", "x", strTmp))
		{
			free (strXml);
			return -3;
		}
	scene->ulx = atof (strTmp);
	if (0 != get_element_value (&(strXml[a]), "corner_point", "y", strTmp))
		{
			free (strXml);
			return -3;
		}
	scene->uly = atof (strTmp);

	if (0 !=
			get_element_value (&(strXml[a]), "projection_information", "projection",
												 scene->projection))
		{
			free (strXml);
			return -3;
		}

	if (0 !=
			get_element_value (&(strXml[a]), "projection_information", "datum",
												 scene->datum))
		{
			free (strXml);
			return -3;
		}

	if (0 !=
			get_element_value (&(strXml[a]), "projection_information", "units",
												 scene->units))
		{
			free (strXml);
			return -3;
		}

	if (0 != get_element_value (&(strXml[a]), "zone_code", NULL, strTmp))
		{
			printf ("Warning: no zone_code found in the xml!\n");
			// free(strXml);
			// return -3;
		}
	else
		{
			scene->zone = atoi (strTmp);
		}

	int iBand = 0;
	a += find_next_element (&(strXml[a]), "bands");

	while (find_next_element (&(strXml[a]), "band") >= 0)
		{
			if (0 !=
					get_element_value (&(strXml[a]), "band", "name",
														 scene->band[iBand].name))
				{
					free (strXml);
					return -3;
				}

			index = get_srf_band_index (scene->instrument, scene->band[iBand].name);
			if (index >= 0)
				{
					scene->srf_band_index[index] = iBand;
				}
			else if (strcmp (scene->band[iBand].name, QA_BAND_NAME) == 0)
				{
					scene->qa_band_index = iBand;
				}
			else
				{
					a += find_next_element (&(strXml[a]), "/band");
					continue;
				}

			if (0 !=
					get_element_value (&(strXml[a]), "band", "data_type",
														 scene->band[iBand].data_type))
				{
					free (strXml);
					return -3;
				}
			if (strcmp (scene->band[iBand].data_type, "INT16") == 0 ||
					strcmp (scene->band[iBand].data_type, "UINT16") == 0)
				{
					scene->band[iBand].nbyte_per_pix = 2;
				}
			else if (strcmp (scene->band[iBand].data_type, "INT8") == 0 ||
							 strcmp (scene->band[iBand].data_type, "UINT8") == 0)
				{
					scene->band[iBand].nbyte_per_pix = 1;
				}
			else
				{
					printf ("Not support data type %s\n", scene->band[iBand].data_type);
					return -8;
				}

			if (0 != get_element_value (&(strXml[a]), "band", "nlines", strTmp))
				{
					free (strXml);
					return -3;
				}
			scene->band[iBand].nrow = atoi (strTmp);

			if (0 != get_element_value (&(strXml[a]), "band", "nsamps", strTmp))
				{
					free (strXml);
					return -3;
				}
			scene->band[iBand].ncol = atoi (strTmp);

			// optional
			scene->band[iBand].fill_value = 0.0;
			scene->band[iBand].scale_factor = 0.0;
			if (0 == get_element_value (&(strXml[a]), "band", "fill_value", strTmp))
				{
					scene->band[iBand].fill_value = atof (strTmp);
				}
			if (0 ==
					get_element_value (&(strXml[a]), "band", "scale_factor", strTmp))
				{
					scene->band[iBand].scale_factor = atof (strTmp);
				}

			// file name
			if (0 != get_element_value (&(strXml[a]), "file_name", NULL, strTmp))
				{
					free (strXml);
					return -3;
				}
			sprintf (scene->band[iBand].file_name, "%s/%s", dir, strTmp);

			for (i = strlen (scene->band[iBand].file_name) - 1; i >= 0; i--)
				{
					if (scene->band[iBand].file_name[i] == '.')
						{
							break;
						}
				}

			if (i < 0)
				{
					printf ("What's the format? %s\n", scene->band[iBand].file_name);
					return -4;
				}

			if (strncmp (&(scene->band[iBand].file_name[i]), ".TIF", 4) == 0 ||
					strncmp (&(scene->band[iBand].file_name[i]), ".tif", 4) == 0)
				{
					scene->band[iBand].file_format = TIF;
				}
			else if (strncmp (&(scene->band[iBand].file_name[i]), ".IMG", 4) == 0 ||
							 strncmp (&(scene->band[iBand].file_name[i]), ".img", 4) == 0)
				{
					scene->band[iBand].file_format = BIN;
				}
			else
				{
					printf ("Currently only support *.TIF or *.img!\n");
					return -5;
				}

			// date
			if (iBand == 0)
				{
					char syear[5];
					char sdoy[4];
					for (i = 0; i < 4; i++)
						{
							syear[i] = strTmp[9 + i];
						}
					syear[i] = '\0';
					for (i = 0; i < 3; i++)
						{
							sdoy[i] = strTmp[13 + i];
						}
					sdoy[i] = '\0';
					scene->year = atoi (syear);
					scene->doy = atoi (sdoy);
				}

			if (0 != get_element_value (&(strXml[a]), "pixel_size", "x", strTmp))
				{
					free (strXml);
					return -3;
				}
			scene->band[iBand].pixel_size = atof (strTmp);

			iBand++;
			if (iBand >= MAX_TOT_BAND)
				{
					printf ("WARNING! TOO MANY BANDS!!\n");
				}

			a += find_next_element (&(strXml[a]), "/band");
		}

	scene->nband = iBand;

	fclose (fp);
	return 0;
}

void
get_year_doy (char *str, int *year, int *doy)
{
	char cmd[1024];
	sprintf (cmd, "date -d '%s' '+%%j' >/tmp/sqs_get_doy.txt", str);
	system (cmd);
	FILE *fp = fopen ("/tmp/sqs_get_doy.txt", "r");
	if (fp != NULL)
		{
			char st[10];
			if (NULL != fgets (st, 9, fp))
				{
					*doy = atoi (st);
					strncpy (st, str, 4);
					st[4] = '\0';
					*year = atoi (st);
				}
			fclose (fp);
		}
}

int
parse_sentinel_xml (char *fname, usgs_t * scene)
{
	int i;
	long max_xml_size = 1024 * 1024 * 10;
	FILE *fp = fopen (fname, "r");
	if (fp == NULL)
		{
			printf ("Open %s failed.\n", fname);
			return -1;
		}

	init_gdal ();

	strcpy (scene->xml_file, fname);

	char *strXml = (char *) malloc (sizeof (char) * max_xml_size);
	int n = fread (strXml, 1, max_xml_size, fp);
	strXml[n] = '\0';

	char strTmp[1024];
	char dir[1024];

	strcpy (dir, fname);
	for (i = strlen (dir) - 1; i > 0; i--)
		{
			if (dir[i] == '/')
				{
					dir[i] = '\0';
					break;
				}
		}

	int nrow, ncol;

	char str_id[100], str_id3[100];
	int j;
	for (i = strlen (fname) - 1; i > 0; i--)
		{
			if (fname[i] == '_' && fname[i - 1] == '_')
				{
					for (j = 0; j < 23; j++)
						{
							str_id[j] = fname[i + 1 + j];
						}
					str_id[j] = '\0';

					str_id3[3] = '\0';
					for (j = 3; j > 0; j--)
						{
							str_id3[3 - j] = fname[i - 1 - j];
						}
					break;
				}
		}

	char str_id2[100];
	for (i = 0; i < 6; i++)
		{
			str_id2[i] = fname[strlen (fname) - 10 + i];
		}

	//
	int a;
	//a = find_next_element(strXml, "global_metadata");
	a = 0;
	if (a < 0)
		{
			free (strXml);
			return -2;
		}

	/*if(0 != get_element_value(&(strXml[a]), "satellite", NULL, scene->satellite)){
	   free(strXml);
	   return -3;
	   } */
	strcpy (scene->satellite, "SENTINEL-2");

	/*if(0 != get_element_value(&(strXml[a]), "instrument", NULL, strTmp)){
	   free(strXml);
	   return -3;
	   }
	   if(strcmp(strTmp, "TM") == 0){
	   scene->instrument = _TM_;
	   }
	   else if(strcmp(strTmp, "ETM") == 0){
	   scene->instrument = _ETM_;
	   }
	   else if(strcmp(strTmp, "OLI_TIRS") == 0){
	   scene->instrument = _OLI_;
	   }
	   else{
	   printf("What's the instrument? %s\n", strTmp);
	   free(strXml);
	   return -5;
	   } */
	scene->instrument = MSI;

	if (0 !=
			get_element_value (&(strXml[a]),
												 "SENSING_TIME metadataLevel=\"Standard\"", NULL,
												 strTmp))
		{
			free (strXml);
			return -3;
		}

	get_year_doy (strTmp, &(scene->year), &(scene->doy));
	printf ("year=%d, doy=%d\n", scene->year, scene->doy);

	if (0 !=
			get_element_value (&(strXml[a]), "HORIZONTAL_CS_NAME", NULL, strTmp))
		{
			free (strXml);
			return -3;
		}

	char sss[100];
	sscanf (strTmp, "%s / %s zone %s", scene->datum, scene->projection, sss);

	int len = strlen (sss);
	sss[len - 1] = '\0';
	scene->zone = atoi (sss);

	a = find_next_element (strXml, "Size resolution=\"20\"");
	if (0 != get_element_value (&(strXml[a]), "NROWS", NULL, strTmp))
		{
			free (strXml);
			return -3;
		}
	nrow = atof (strTmp);
	if (0 != get_element_value (&(strXml[a]), "NCOLS", NULL, strTmp))
		{
			free (strXml);
			return -3;
		}
	ncol = atof (strTmp);

	// first corner_point is ul     
	a = find_next_element (strXml, "Geoposition resolution=\"20\"");
	if (0 != get_element_value (&(strXml[a]), "ULX", NULL, strTmp))
		{
			free (strXml);
			return -3;
		}
	scene->ulx = atof (strTmp);
	if (0 != get_element_value (&(strXml[a]), "ULY", NULL, strTmp))
		{
			free (strXml);
			return -3;
		}
	scene->uly = atof (strTmp);

	/*if(0 != get_element_value(&(strXml[a]), "projection_information", "datum", scene->datum)){
	   free(strXml);
	   return -3;
	   }

	   if(0 != get_element_value(&(strXml[a]), "projection_information", "units", scene->units)){
	   free(strXml);
	   return -3;
	   }

	   if(0 != get_element_value(&(strXml[a]), "zone_code", NULL, strTmp)){
	   free(strXml);
	   return -3;
	   }
	   scene->zone = atoi(strTmp); */

	//angles
	a = find_next_element (strXml, "Mean_Sun_Angle");
	if (0 !=
			get_element_value (&(strXml[a]), "ZENITH_ANGLE unit=\"deg\"", NULL,
												 strTmp))
		{
			free (strXml);
			return -3;
		}
	scene->szn = atof (strTmp);

	if (0 !=
			get_element_value (&(strXml[a]), "AZIMUTH_ANGLE unit=\"deg\"", NULL,
												 strTmp))
		{
			free (strXml);
			return -3;
		}
	scene->san = atof (strTmp);

	a = find_next_element (strXml, "Mean_Viewing_Incidence_Angle bandId=\"1\"");
	if (0 !=
			get_element_value (&(strXml[a]), "ZENITH_ANGLE unit=\"deg\"", NULL,
												 strTmp))
		{
			free (strXml);
			return -3;
		}
	scene->vzn = atof (strTmp);

	if (0 !=
			get_element_value (&(strXml[a]), "AZIMUTH_ANGLE unit=\"deg\"", NULL,
												 strTmp))
		{
			free (strXml);
			return -3;
		}
	scene->van = atof (strTmp);

	char bands[6][4] = { "B02", "B03", "B04", "B8A", "B11", "B12" };
	for (i = 0; i < 6; i++)
		{
			strcpy (scene->band[i].name, bands[i]);
			scene->srf_band_index[i] = i;
			strcpy (scene->band[i].data_type, "UINT16");
			scene->band[i].nbyte_per_pix = 2;
			scene->band[i].fill_value = 0.0;
			scene->band[i].scale_factor = 0.0001;	// todo: read this from SAFE's xml
			sprintf (scene->band[i].file_name,
							 "%s/IMG_DATA/R20m/S2A_USER_MSI_L2A_TL_%s__%s_%s_%s_20m.jp2",
							 dir, str_id3, str_id, str_id2, bands[i]);
			scene->band[i].file_format = TIF;
			scene->band[i].nrow = nrow;
			scene->band[i].ncol = ncol;
			scene->band[i].pixel_size = 20;
		}

	i = 6;
	strcpy (scene->band[i].name, "SCL");
	sprintf (scene->band[i].file_name,
					 "%s/IMG_DATA/S2A_USER_SCL_L2A_TL_%s__%s_%s_20m.jp2", dir, str_id3,
					 str_id, str_id2);
	scene->qa_band_index = i;;
	strcpy (scene->band[i].data_type, "UINT16");
	scene->band[i].nbyte_per_pix = 2;
	scene->band[i].fill_value = 0.0;
	scene->band[i].scale_factor = 1.0;
	scene->band[i].file_format = TIF;
	scene->band[i].nrow = nrow;
	scene->band[i].ncol = ncol;
	scene->band[i].pixel_size = 20;

	i = 7;
	strcpy (scene->band[i].name, "CLD");
	sprintf (scene->band[i].file_name,
					 "%s/QI_DATA/S2A_USER_CLD_L2A_TL_%s__%s_%s_20m.jp2", dir, str_id3,
					 str_id, str_id2);
	scene->cld_band_index = i;;
	strcpy (scene->band[i].data_type, "UINT16");
	scene->band[i].nbyte_per_pix = 2;
	scene->band[i].fill_value = 0.0;
	scene->band[i].scale_factor = 1.0;
	scene->band[i].file_format = TIF;
	scene->band[i].nrow = nrow;
	scene->band[i].ncol = ncol;
	scene->band[i].pixel_size = 20;

	i = 8;
	strcpy (scene->band[i].name, "SNW");
	sprintf (scene->band[i].file_name,
					 "%s/QI_DATA/S2A_USER_SNW_L2A_TL_%s__%s_%s_20m.jp2", dir, str_id3,
					 str_id, str_id2);
	scene->snw_band_index = i;
	strcpy (scene->band[i].data_type, "UINT16");
	scene->band[i].nbyte_per_pix = 2;
	scene->band[i].fill_value = 0.0;
	scene->band[i].scale_factor = 1.0;
	scene->band[i].file_format = TIF;
	scene->band[i].nrow = nrow;
	scene->band[i].ncol = ncol;
	scene->band[i].pixel_size = 20;

	scene->nband = 9;

	/*int iBand = 0;
	   a += find_next_element(&(strXml[a]), "bands");

	   while(find_next_element(&(strXml[a]), "band") >= 0){
	   if(0 != get_element_value(&(strXml[a]), "band", "name", scene->band[iBand].name)){
	   free(strXml);
	   return -3;
	   }

	   index = get_srf_band_index(scene->instrument, scene->band[iBand].name);
	   if(index >= 0){
	   scene->srf_band_index[index] = iBand;
	   }               
	   else if (strcmp(scene->band[iBand].name, QA_BAND_NAME) == 0){
	   scene->qa_band_index = iBand;
	   }

	   if(0 != get_element_value(&(strXml[a]), "band", "data_type", scene->band[iBand].data_type)){
	   free(strXml);
	   return -3;
	   }
	   if(strcmp(scene->band[iBand].data_type, "INT16") == 0 ||
	   strcmp(scene->band[iBand].data_type, "UINT16") == 0){
	   scene->band[iBand].nbyte_per_pix = 2;
	   }       
	   else if(strcmp(scene->band[iBand].data_type, "INT8") == 0 ||
	   strcmp(scene->band[iBand].data_type, "UINT8") == 0){
	   scene->band[iBand].nbyte_per_pix = 1;
	   }       
	   else{
	   printf("Not support data type %s\n", scene->band[iBand].data_type);
	   return -8;
	   }

	   if(0 != get_element_value(&(strXml[a]), "band", "nlines", strTmp)){
	   free(strXml);
	   return -3;
	   }
	   scene->band[iBand].nrow = atoi(strTmp);

	   if(0 != get_element_value(&(strXml[a]), "band", "nsamps", strTmp)){
	   free(strXml);
	   return -3;
	   }
	   scene->band[iBand].ncol = atoi(strTmp);

	   // optional
	   scene->band[iBand].fill_value = 0.0;
	   scene->band[iBand].scale_factor = 0.0;
	   if(0 == get_element_value(&(strXml[a]), "band", "fill_value", strTmp)){
	   scene->band[iBand].fill_value = atof(strTmp);
	   }
	   if(0 == get_element_value(&(strXml[a]), "band", "scale_factor", strTmp)){
	   scene->band[iBand].scale_factor = atof(strTmp);
	   }

	   // file name
	   if(0 != get_element_value(&(strXml[a]), "file_name", NULL, strTmp)){
	   free(strXml);
	   return -3;
	   }
	   sprintf(scene->band[iBand].file_name, "%s/%s", dir, strTmp);

	   for(i=strlen(scene->band[iBand].file_name)-1; i>=0; i--){
	   if(scene->band[iBand].file_name[i] == '.'){
	   break;
	   }
	   }

	   if(i<0){
	   printf("What's the format? %s\n", scene->band[iBand].file_name);
	   return -4;
	   }

	   if(strncmp(&(scene->band[iBand].file_name[i]), ".TIF", 4) == 0 ||
	   strncmp(&(scene->band[iBand].file_name[i]), ".tif", 4) == 0){
	   scene->band[iBand].file_format = TIF;
	   }
	   else if(strncmp(&(scene->band[iBand].file_name[i]), ".IMG", 4) == 0 ||
	   strncmp(&(scene->band[iBand].file_name[i]), ".img", 4) == 0){
	   scene->band[iBand].file_format = BIN;
	   }
	   else{
	   printf("Currently only support *.TIF or *.img!\n");
	   return -5;
	   }

	   // date
	   if(iBand == 0){
	   char syear[5];
	   char sdoy[4];
	   for(i=0; i<4; i++){
	   syear[i] = strTmp[9+i];
	   }
	   syear[i] = '\0';
	   for(i=0; i<3; i++){
	   sdoy[i] = strTmp[13+i];
	   }
	   sdoy[i] = '\0';
	   scene->year = atoi(syear);
	   scene->doy = atoi(sdoy);
	   }

	   if(0 != get_element_value(&(strXml[a]), "pixel_size", "x", strTmp)){
	   free(strXml);
	   return -3;
	   }
	   scene->band[iBand].pixel_size = atof(strTmp);

	   iBand ++;
	   if(iBand >= MAX_TOT_BAND){
	   printf("WARNING! TOO MANY BANDS!!\n");
	   }

	   a += find_next_element(&(strXml[a]), "/band");
	   }

	   scene->nband = iBand; */

	fclose (fp);
	return 0;
}

int
convert_to_binary (usgs_t * scene)
{
	char cmd[1024];
	int i;
	for (i = 0; i < scene->nband; i++)
		{
			sprintf (cmd,
							 "if [ ! -r %s.bin ]; then \ngdal_translate -of ENVI %s %s.bin \nfi",
							 scene->band[i].file_name, scene->band[i].file_name,
							 scene->band[i].file_name);
			if (0 != system (cmd))
				{
					return -1;
				}
			strcat (scene->band[i].file_name, ".bin");
			scene->band[i].file_format = BIN;
			if (0 != open_a_band (&(scene->band[i])))
				{
					return -1;
				}
			printf ("new format: %s\n", scene->band[i].file_name);
		}

	return 0;
}

int
open_a_band (band_t * band)
{
	if (band->file_format == BIN)
		{
			band->fp = fopen (band->file_name, "rb");
			if (band->fp == NULL)
				{
					printf ("Open %s failed.\n", band->file_name);
					return -1;
				}
		}
	else if (band->file_format == TIF)
		{
			band->hDataset = GDALOpen (band->file_name, GA_ReadOnly);
			if (band->hDataset == NULL)
				{
					printf ("Open %s failed.\n", band->file_name);
					return -2;
				}
		}

	return 0;
}

void
close_a_band (band_t * band)
{
	if (band->file_format == BIN)
		{
			fclose (band->fp);
		}
	else if (band->file_format == TIF)
		{
			GDALClose (band->hDataset);
		}
}

int
read_a_band_a_row (band_t * band, int row, void *buf)
{
	if (band->file_format == TIF)
		{
			if (0 != read_one_row_gdal (band->hDataset, 0, row, buf))
				{
					printf ("read band %s row %d failed.\n", band->file_name, row);
					return -2;
				}
		}
	else if (band->file_format == BIN)
		{
			long iseek = row * band->ncol * band->nbyte_per_pix;
			if (0 != fseek (band->fp, iseek, SEEK_SET))
				{
					return -10;
				}

			if (band->ncol !=
					fread (buf, band->nbyte_per_pix, band->ncol, band->fp))
				{
					printf ("read band %s row %d failed.\n", band->file_name, row);
					return -2;
				}
			//Note: binary file from netwrok is big-endian
			/*if(band->nbyte_per_pix == 2){
			   short *pbuf = (short *)buf;
			   int i;
			   for(i=0; i<band->ncol; i++){
			   pbuf[i] = ntohs(pbuf[i]);
			   }
			   } */

		}
	else
		{
			return -9;
		}

	return 0;
}

int
read_a_band (band_t * band, void *buf)
{
	if (band->file_format == TIF)
		{
			if (0 != read_one_band_gdal (band->hDataset, 0, buf))
				{
					printf ("read band %s failed.\n", band->file_name);
					return -2;
				}
		}
	else if (band->file_format == BIN)
		{
			int row = 0;
			long iseek = row * band->ncol * band->nbyte_per_pix;
			if (0 != fseek (band->fp, iseek, SEEK_SET))
				{
					return -10;
				}

			if (band->ncol * band->nrow !=
					fread (buf, band->nbyte_per_pix, band->ncol * band->nrow, band->fp))
				{
					printf ("read band %s row %d failed.\n", band->file_name, row);
					return -2;
				}
			//Note: binary file from netwrok is big-endian
			/*if(band->nbyte_per_pix == 2){
			   short *pbuf = (short *)buf;
			   int i;
			   for(i=0; i<band->ncol*band->nrow; i++){
			   pbuf[i] = ntohs(pbuf[i]);
			   }
			   } */

		}
	else
		{
			return -9;
		}

	return 0;
}

long
usgs_gctp_datum (char *name)
{
	if (strcmp (name, "CLARKE1866") == 0)
		{
			return 0L;
		}
	else if (strcmp (name, "CLARKE1880") == 0)
		{
			return 1L;
		}
	else if (strcmp (name, "BESSEL") == 0)
		{
			return 2L;
		}
	else if (strcmp (name, "INTERNATIONAL1967") == 0)
		{
			return 3L;
		}
	else if (strcmp (name, "INTERNATIONAL1909") == 0)
		{
			return 4L;
		}
	else if (strcmp (name, "WGS72") == 0)
		{
			return 5L;
		}
	else if (strcmp (name, "EVEREST") == 0)
		{
			return 6L;
		}
	else if (strcmp (name, "WGS66") == 0)
		{
			return 7L;
		}
	else if (strcmp (name, "GRS1980") == 0)
		{
			return 8L;
		}
	else if (strcmp (name, "AIRY") == 0)
		{
			return 9L;
		}
	else if (strcmp (name, "MODIFIED_EVEREST") == 0)
		{
			return 10L;
		}
	else if (strcmp (name, "MODIFIED_AIRY") == 0)
		{
			return 11L;
		}
	else if (strcmp (name, "WGS84") == 0)
		{
			return 12L;
		}
	else if (strcmp (name, "SOUTHEAST_ASIA") == 0)
		{
			return 13L;
		}
	else if (strcmp (name, "AUSTRALIAN_NATIONAL") == 0)
		{
			return 14L;
		}
	else if (strcmp (name, "KRASSOVSKY") == 0)
		{
			return 15L;
		}
	else if (strcmp (name, "HOUGH") == 0)
		{
			return 16L;
		}
	else if (strcmp (name, "MERCURY1960") == 0)
		{
			return 17L;
		}
	else if (strcmp (name, "MODIFIED_MERCURY") == 0)
		{
			return 18L;
		}
	else if (strcmp (name, "SPHERE") == 0)
		{
			return 19L;
		}
	else
		{
			return -1;
		}
}

int
get_scene_proj (usgs_t * scene, long *sys, long *zone, long *datum,
								double parm[], double *ulx, double *uly, double *res)
{
	int i;
	int index = scene->srf_band_index[0];

	printf ("%d\n", index);
	printf ("%s\n", scene->band[index].file_name);
	printf ("%s\n", scene->band[index].name);

	if (scene->band[index].file_format == TIF)
		{
			if (0 != open_a_band (&(scene->band[index])))
				{
					printf ("get_scene_proj: Open band %s failed.\n",
									scene->band[index].file_name);
					close_a_band (&(scene->band[index]));
					return -1;
				}
			if (0 !=
					get_geo_info_gdal (scene->band[index].hDataset, sys, zone, datum,
														 parm, ulx, uly, res))
				{
					close_a_band (&(scene->band[index]));
					return -1;
				}
			close_a_band (&(scene->band[index]));
			return 0;
		}
	else if (scene->band[index].file_format == HDF)
		{
			if (0 !=
					get_geo_info_hdf (scene->band[index].file_name, sys, zone, datum,
														parm, ulx, uly, res))
				{
					return -2;
				}
			return 0;
		}
	else if (scene->band[index].file_format == BIN)
		{
			// try to find the hdf
			char fhdf[1024];
			strcpy (fhdf, scene->xml_file);
			for (i = strlen (fhdf) - 1; i >= 0; i--)
				{
					if (fhdf[i] == '.')
						{
							fhdf[i] = '\0';
							break;
						}
				}
			strcat (fhdf, ".hdf");
			if (0 == get_geo_info_hdf (fhdf, sys, zone, datum, parm, ulx, uly, res))
				{
					return 0;
				}

			// from xml
			if (strcmp (scene->projection, "UTM") == 0)
				{
					*sys = GCTP_UTM;
				}
			else if (strcmp (scene->projection, "PS") == 0)
				{
					*sys = GCTP_PS;
					// assign zone code here for non-UTM projection
					scene->zone = 0;
				}
			else
				{
					return -3;
				}

			char *datumname, *p;
			datumname = strdup (scene->datum);
			p = datumname;
			while (toupper (*p))
				{
					*p++;
				}
			*datum = usgs_gctp_datum (datumname);
			if (*datum == -1)
				{
					return -3;
				}

			*zone = scene->zone;
			*ulx = scene->ulx;
			*uly = scene->uly;
			*res = scene->band[0].pixel_size;
			int i;
			for (i = 0; i < 15; i++)
				{
					parm[i] = 0.0;
				}

			return 0;
		}

	return -9;
}
