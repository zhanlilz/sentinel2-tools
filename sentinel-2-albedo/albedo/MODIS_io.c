/*
 * !Description
 *  Retrieve one pixel or one row MODIS value, extract MODIS metadata, 
 *		and clean-up MODIS
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
 * Please contact us with a simple description of your application, if you have interests.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "lndsr_albedo.h"

/*read one pixel from MODIS BRDF and QA input*/
int loadModisPixel(GRID_MODIS *modis, int irow, int icol)
{
  int  iband;
  int32 start[3];
  int32 length[3];
  int16 val_temp[3];

  start[0] = irow;
  start[1] = icol;
  start[2] = 0;
  length[0] = 1;
  length[1] = 1;
  length[2] = 3;

  /* read MODIS surface reflectance */
  for(iband=0; iband<MODIS_NBANDS; iband++){
    if(SDreaddata(modis->brdf_id[iband], start, NULL, length, val_temp) == FAILURE) {
        fprintf(stderr, "\nloadModisRow: reading reflectance error\n");
        return FAILURE;
    }
   modis->Pix_brdf[iband][0]=val_temp[0];
   modis->Pix_brdf[iband][1]=val_temp[1];
   modis->Pix_brdf[iband][2]=val_temp[2];
 }
  /* read state QA data */
  if(modis->qa_id != FAILURE && modis->qa_id != SUCCESS){
    if(SDreaddata(modis->qa_id, start, NULL, length, &modis->Pix_qa) == FAILURE) {
      fprintf(stderr, "\nloadModisRow: reading BRDF band QA error\n");
      return FAILURE;
    }
	}
	/* sqs */
	else if ( modis->qa_id == SUCCESS){
		uint8 tt;
		uint32 tmp = 0;
		modis->Pix_qa = 0;
  	for(iband=0; iband<MODIS_NBANDS; iband++){
   	 	if(SDreaddata(modis->qa_ids[iband], start, NULL, length, &tt) == FAILURE) {
    	  fprintf(stderr, "\nloadModisRow: reading BRDF band QA error\n");
      	return FAILURE;
    	}
			tmp = tt;
			tmp = tmp << (4*iband);
		 	modis->Pix_qa += tmp;	
		}
	}
	else{
		return FAILURE;
	}
  
  return SUCCESS;
}
 
/* read one row from MODIS SR input */
int loadModisRow(GRID_MODIS *modis, int irow)
{
  int  iband;
  int32 start[2];
  int32 length[2];

  start[0] = irow;
  start[1] = 0;
  length[0] = 1;
  length[1] = modis->ncols;

  /* read MODIS surface reflectance */
  for(iband=0; iband<MODIS_NBANDS; iband++) 
    if(SDreaddata(modis->brdf_id[iband], start, NULL, length, modis->sr[iband]) == FAILURE) {
      fprintf(stderr, "\nloadModisRow: reading reflectance error\n");
      return FAILURE;
    }

  /* read state QA data */
  if(modis->qa_id != FAILURE) 
    if(SDreaddata(modis->qa_id, start, NULL, length, modis->qa) == FAILURE) {
      fprintf(stderr, "\nloadModisRow: reading albedo band QA error\n");
      return FAILURE;
    }

  if(modis->snow_id != FAILURE) 
    if(SDreaddata(modis->snow_id, start, NULL, length, modis->snow) == FAILURE) {
      fprintf(stderr, "\nloadModisRow: reading snow QA error\n");
      return FAILURE;
    }
      
  return SUCCESS;
}

/* allocate memory, get metadata and open specific sds for Modis surface reflectance */
int getModisMetaInfo(GRID_MODIS *modis) {
  char name[100];
  int i, index, ret;
  char GD_gridlist[100];
  int32 gfid=0, ngrid=0, gid=0;
  int32 bufsize=100;
  int32 att_id;
  char sdsName[7][100] = {"BRDF_Albedo_Parameters_Band1", "BRDF_Albedo_Parameters_Band2", "BRDF_Albedo_Parameters_Band3", "BRDF_Albedo_Parameters_Band4", "BRDF_Albedo_Parameters_Band5", "BRDF_Albedo_Parameters_Band6", "BRDF_Albedo_Parameters_Band7"};

   /* open a hdf file */
  gfid = GDopen(modis->srName, DFACC_READ);
  if(gfid==FAILURE){
      fprintf(stderr, "\nNot successful in retrieving MODIS grid file ID/open from %s: getModisMetaInfo\n", modis->srName);
      return FAILURE;
  }

  /* find out about grid type */
  ngrid=GDinqgrid(modis->srName, GD_gridlist, &bufsize);
  if(ngrid==FAILURE){
      fprintf(stderr, "\nNot successful in retrieving MODIS grid name list from %s: getModisMetaInfo\n", modis->srName);
      return FAILURE;
  }

  /* attach grid */
  gid = GDattach(gfid, GD_gridlist);
  if(gid==FAILURE){
    fprintf(stderr, "\nNot successful in attaching MODIS grid from %s: getModisMetaInfo\n", modis->srName);
    return FAILURE;
  }

  /* get grid info */
  ret = GDgridinfo(gid, &(modis->ncols), &(modis->nrows), modis->GD_upleft, modis->GD_lowright);
  if(ret==FAILURE){
      fprintf(stderr, "\nFailed to read grid info from %s: getModisMetaInfo\n", modis->srName);
      return FAILURE;
  }
  modis->ulx = modis->GD_upleft[0];
  modis->uly = modis->GD_upleft[1];
  modis->res = (modis->GD_lowright[0]-modis->GD_upleft[0])/modis->ncols;

  /* get projection parameters */
  ret = GDprojinfo(gid, &modis->GD_projcode, &modis->GD_zonecode, &modis->GD_spherecode, modis->GD_projparm);
  if(ret==FAILURE){
    fprintf(stderr, "\nNot successful in reading grid projection info. from %s: getModisMetaInfo\n", modis->srName);
    return FAILURE;
  }
  modis->GD_unit = 2; /* meters */
  modis->GD_datum = 12; /* WGS84 datum */

  /* get grid origin */
  ret = GDorigininfo(gid, &modis->GD_origincode);
  if(ret==FAILURE){
    fprintf(stderr, "\nFailed to read grid origin info from %s: getModisMetaInfo\n", modis->srName);
    return FAILURE;
  }

  /* detach grid */
  ret = GDdetach(gid);
  if(ret==FAILURE){
      fprintf(stderr, "\nFailed to detach grid for %s: getModisMetaInfo", modis->srName);
      return FAILURE;
  }

  /* close for grid access */
  ret = GDclose(gfid);
  if(ret==FAILURE){
      fprintf(stderr, "\nGD-file close failed for %s: getModisMetaInfo", modis->srName);
      return FAILURE;
  }

  /* open hdf file and get sds_id from given sds_name */  
  if ((modis->SD_ID = SDstart(modis->srName, DFACC_READ))<0) {
    fprintf(stderr, "\nCan't open file %s: getModisMetaInfo\n", modis->srName);
    return FAILURE;
  }

  /* open all seven bands of Modis BRDF */
  for (i=0; i<MODIS_NBANDS; i++)  {
    if ((index=SDnametoindex(modis->SD_ID, sdsName[i]))<0) {
      fprintf(stderr, "\nNot successful in convert SR sdsName to index: getModisMetaInfo\n");
      return FAILURE;
    }
    modis->brdf_id[i] = SDselect(modis->SD_ID, index);
  }
  
  /* retrieve BRDF Fill Value */
  if ((att_id = SDfindattr(modis->brdf_id[0], "_FillValue")) == FAILURE) {
    fprintf(stderr, "\nCan't retrieve SR fill value from SR SDS attr: getModisMetaInfo\n");
    return FAILURE;
  }
  if (SDreadattr(modis->brdf_id[0], att_id, &(modis->fillValue)) == FAILURE) {
    fprintf(stderr, "\nCan't retrieve fill value from SR SDS attr: getModisMetaInfo\n");
    return FAILURE;
  }

  /* retrieve SR valid_range */
  if ((att_id = SDfindattr(modis->brdf_id[0], "valid_range")) == FAILURE) {
    fprintf(stderr, "\nCan't retrieve SR valid_range from SR SDS attr: getModisMetaInfo\n");
    return FAILURE;
  }
  if (SDreadattr(modis->brdf_id[0], att_id, modis->range) == FAILURE) {
    fprintf(stderr, "\nCan't retrieve valid_range from SR SDS attr: getModisMetaInfo\n");
    return FAILURE;
  }

  /* retrieve SR scale factor */
  if ((att_id = SDfindattr(modis->brdf_id[0], "scale_factor")) == FAILURE) {
    fprintf(stderr, "\nCan't retrieve SR scale_factor from SR SDS attr: getModisMetaInfo\n");
    return FAILURE;
  }
  if (SDreadattr(modis->brdf_id[0], att_id, &modis->scale_factor) == FAILURE) {
    fprintf(stderr, "\nCan't retrieve scale_factor from SR SDS attr.: getModisMetaInfo\n");
    return FAILURE;
  }

  /* open QA hdf file and get sds_id from given sds_name */  
  if ((modis->QA_ID = SDstart(modis->qaName, DFACC_READ))<0) {
    fprintf(stderr, "\nCan't open QA file, QA won't be used in the computing: %s getModisMetaInfo\n", modis->qaName);
  }

  /* open QA layer */
  strcpy(name, "BRDF_Albedo_Band_Quality");
  if ((index=SDnametoindex(modis->QA_ID, name))<0) {
		/*sqs
    fprintf(stderr, "\nNot successful in convert sdsName to index, won't use MODIS QA: getModisMetaInfo\n");
    modis->qa_id = FAILURE;*/
    //fprintf(stderr, "\nNot successful in convert sdsName to index, try V006...\n");
    modis->qa_id = SUCCESS;
  	for (i=0; i<MODIS_NBANDS; i++)  {
			sprintf(name, "BRDF_Albedo_Band_Quality_Band%d", i+1);
  		if ((index=SDnametoindex(modis->QA_ID, name))<0) {
    		//fprintf(stderr, "\nNot successful in convert sdsName to index, try V006...\n");
    		modis->qa_id = FAILURE;
				break;
			}
			else{
				printf("Using V006 MODIS BRDF.\n");
				modis->qa_ids[i] = SDselect(modis->QA_ID, index);
			}
		}
  }
  else
    modis->qa_id = SDselect(modis->QA_ID, index);

  /* open QA snow layer */
  strcpy(name, "Snow_BRDF_Albedo");
  if ((index=SDnametoindex(modis->QA_ID, name))<0) {
    fprintf(stderr, "\nNot successful in convert sdsName to index, won't use MODIS QA: getModisMetaInfo\n");
    modis->snow_id = FAILURE;
  }
  else
    modis->snow_id = SDselect(modis->QA_ID, index);

  /* allocate memory to load surface reflectance and qa*/
  alloc_2dim_contig((void ***)(&modis->sr),  MODIS_NBANDS, modis->ncols, sizeof(int16));
  alloc_1dim_contig((void **) (&modis->qa),  modis->ncols, sizeof(uint32));
  alloc_1dim_contig((void **) (&modis->snow),  modis->ncols, sizeof(int8));

  alloc_2dim_contig((void ***)(&modis->major_cls), modis->nrows, modis->ncols, sizeof(uint8));
  alloc_2dim_contig((void ***)(&modis->percent_cls), modis->nrows, modis->ncols, sizeof(uint8));

#ifdef DEBUG
  fprintf(stderr,"\nFile: %s",modis->srName);
  fprintf(stderr,"\nnrows:%ld, ncols:%ld",modis->nrows, modis->ncols);
  fprintf(stderr,"\nulx:%8.1f, uly:%8.1f", modis->ulx, modis->uly);
  fprintf(stderr,"\nres:%5.1f", modis->res);
  fprintf(stderr,"\nfillV:%d", modis->fillValue); 
  fprintf(stderr,"\nproj=%ld\n", modis->GD_projcode);
#endif

  return SUCCESS;
}

/* close opened Modis sds and file, free memory */
int cleanUpModis(GRID_MODIS *modis)
{
  int iband;

  for(iband=0; iband<MODIS_NBANDS; iband++) 
    if((SDendaccess(modis->brdf_id[iband])) == FAILURE) {
      fprintf(stderr, "\nSDendaccess for %s reflectance (b%d) error!: cleanUpModis\n", modis->srName, iband);
      return FAILURE;
    }

  if((SDend(modis->SD_ID)) == FAILURE) {
    fprintf(stderr, "\nSDend for %s error!: cleanUpModis\n", modis->srName);
    return FAILURE;
  }

  if(modis->qa_id != FAILURE) {
		/* sqs for v6 */
  	if(modis->qa_id == SUCCESS) {
  		for(iband=0; iband<MODIS_NBANDS; iband++) 
			{
					SDendaccess(modis->qa_ids[iband]);
			}
		}
		else{	
    if((SDendaccess(modis->qa_id)) == FAILURE) {
      fprintf(stderr, "\nSDendaccess for %s qa error!: cleanUpModis\n", modis->qaName);
      return FAILURE;
    }
		}

    if((SDendaccess(modis->snow_id)) == FAILURE) {
      fprintf(stderr, "\nSDendaccess for %s snow qa error!: cleanUpModis\n", modis->qaName);
      return FAILURE;
    }

    if((SDend(modis->QA_ID)) == FAILURE) {
      fprintf(stderr, "\nSDend for %s error!: cleanUpModis\n", modis->qaName);
      return FAILURE;
		}
  }
  
  free_2dim_contig((void **)modis->sr); 
  free(modis->qa);
  free(modis->snow);
 /* free_2dim_contig((void **)modis->major_cls); 
     free_2dim_contig((void **)modis->percent_cls); */ /* freed in Reproject_CalAlbedo.c*/
  /*  fclose(modis->agg_fp);closed in Reproject.c right after the majority process*/

  return SUCCESS;
}
