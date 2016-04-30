/*
 * !Description
 *  Retrieve one pixel or one row Landsat surface reflectance, extract Landsat metadata, 
 *              clean-up memory, calculate view-zenith-angle, get the closest class, 
 *              cloud and snow determination.
 *      
 * !Usage
 *   parseParameters (filename, ptr_LndSensor, ptr_MODIS)
 * 
 * !Developer
 *  Initial version (V1.0.0): Yanmin Shuai (Yanmin.Shuai@nasa.gov)
 *                                          and numerous other contributors
 *                                           (thanks to all)
 *
 * !Revision History
 * 
 *
 *!Team-unique header:
 * This system (research version) was developed for NASA TE project "Albedo  
 *         Trends Related to Land CoverChange and Disturbance: A Multi-sensor   
 *         Approach", and is adopted by USGS Landsat science team (2012-2017) 
 *         to produce the long-term land surface albedo dataset over North America
 *         at U-Mass Boston (PI: Crystal B. Schaaf, Co-Is:Zhongping Lee and Yanmin Shuai). 
 * 
 * !References and Credits
 * Principal Investigator: Jeffrey G. masek (Jeffrey.G.Masek@nasa.gov)
 *                                       Code 618, Biospheric Science Laboratory, NASA/GSFC.
 *
 * Please contact us with a simple description of your application, if you have any interest.
 */
#include <assert.h>
#include "lndsr_albedo.h"
#include "sqs.h"

/* read one row from input lndsr classification map */
int loadSensorClsRow(DIS_LANDSAT *sensor, int irow)
{
  int temp;
  long offset;
  //int8 mycls;

  
  offset = (long) irow * sensor->ncols*sizeof(int8); /* + 1 * sensor->nrows * sensor->ncols;*/
  temp=fseek(sensor->fpCls, offset, 0);
  if (temp!=0){
    printf("\n Can't find the position to read for row:%d",irow);
    exit(0);
  }

  /* read data array from binary file */
  temp=fread(sensor->Clsdata, sizeof(int8),sensor->ncols, sensor->fpCls);
  if (temp!=sensor->ncols){
      printf("\nCan't read the classification data for row=%d",irow);
      exit(0);
   }
  
/*for (j=0;j<sensor->ncols;j++)
  {
    temp=fread(&mycls, sizeof(int8), 1, sensor->fpCls);
    if (temp!=1){
      printf("\nCan't read the cls# for row=%d,col=%d",irow,j);
      exit(0);
    }
      sensor->Clsdata[j]=mycls;    
  }*/
  return SUCCESS;
}

int load_lndSRrow(DIS_LANDSAT *sensor, int row)
{
        int i;
        int index;

        for (i = 0; i < N_SRF_BAND; i++)
        {
                index = sensor->scene.srf_band_index[i];
                if(0 != read_a_band_a_row(&(sensor->scene.band[index]), row, (void *)sensor->OneRowlndSR[i])){
                        printf("Read failed.\n");
                        return FAILURE;
                }

        }

        index = sensor->scene.qa_band_index;
        if(0 != read_a_band_a_row(&(sensor->scene.band[index]), row, (void *)sensor->OneRowlndSRQAData)){
                printf("Read failed.\n");
                return FAILURE;
        }

        index = sensor->scene.cld_band_index;
        if(0 != read_a_band_a_row(&(sensor->scene.band[index]), row, (void *)sensor->OneRowlndSRCldData)){
                printf("Read failed.\n");
                return FAILURE;
        }
        index = sensor->scene.snw_band_index;
        if(0 != read_a_band_a_row(&(sensor->scene.band[index]), row, (void *)sensor->OneRowlndSRSnwData)){
                printf("Read failed.\n");
                return FAILURE;
        }
        

  return SUCCESS;  
}

/*read one row from lndSR*/
int load_lndSRrow_old(DIS_LANDSAT *sensor, int irow)
{

  int32 start[2],length[2];
  int iband,icol;
   int32 sds_id;  /* Data set identifier */
  int16 *data;

  
  alloc_1dim_contig((void **)(&data), sensor->ncols, sizeof(int16));

  for (iband=0; iband<SENSOR_BANDS; iband++)
   {
        start[0]=irow;
        start[1]=0;
        length[0]=1;
        length[1]=sensor->ncols;
        
       sds_id=sensor->sr_id[iband];
     
       /* if (SDreaddata(sds_id,start,NULL, length,data)==FAILURE){
                fprintf(stderr, "ERROR:  detected FAIL\n"
                                              "\t from HDF procedure SDreaddata\n"
                                              "\t while attempting to read from\n"
                                              "\t band %d.\n", iband);
        }
        
        for(icol=0;icol<sensor->ncols;icol++)
                sensor->OneRowlndSR[iband][icol]=data[icol];
      */
        
        if (SDreaddata(sds_id,start,NULL, length,sensor->OneRowlndSR[iband])==FAILURE){
                fprintf(stderr, "ERROR:  detected FAIL\n"
                                              "\t from HDF procedure SDreaddata\n"
                                              "\t while attempting to read from\n"
                                              "\t band %d.\n", iband);
                exit(1);
        }    
     
    }
 
   free(data);
   
    if (SDreaddata(sensor->lndatmo_id,start,NULL,length,sensor->OneRowlndSRAtmoData)==FAILURE){
               fprintf (stderr, "\nload_lndSRrow():fail to read LndSRAtmo at row:%d.",irow);
                 exit(1);
    }
   if (SDreaddata(sensor->lndQA_id,start,NULL,length,sensor->OneRowlndSRQAData)==FAILURE){
                 fprintf (stderr, "\nload_lndSRrow():fail to read LndSRQA at row:%d.",irow);
                 exit(1);
         }
         
 
  return SUCCESS;  

}

/* read one pixel  from sensor lndsr input */
int loadSensorSRPixel(DIS_LANDSAT *sensor, int irow, int icol)
{

  int32 start[2],length[2];
  int iband;
  int16 tmp_sr;

 for (iband=0; iband<SENSOR_BANDS; iband++)
   {
                start[0]=irow;
                start[1]=icol;
                length[0]=1;
                length[1]=1;
              if (SDreaddata(sensor->sr_id[iband],start,NULL,length,&tmp_sr)==FAILURE){
                     fprintf (stderr, "\nOpenLndsrFile():fail to read the SDS band:%d.",iband);
                     exit(1);
              }
              else
                sensor->Pix_sr[iband]=tmp_sr;
    }

  return SUCCESS;  
  
}

int openSensorFile(DIS_LANDSAT *sensor) 
{
        printf("Parsing %s...\n", sensor->fileName);

        if(0 != parse_sentinel_xml(sensor->fileName, &(sensor->scene))){
                printf("Parse xml failed. XML=%s\n", sensor->fileName);
                return FAILURE;
        }

        int i;
        int index;

        // open srf
        GDALAllRegister();

        // convert the jp2 image files to binary first
        if(0 != convert_to_binary(&(sensor->scene))){
          printf("OpenLndsrFile: convert image to binary file failed.\n");
          return FAILURE;
        }

        printf("Opening files...\n");
        for (i = 0; i < N_SRF_BAND; i++)
        {
                index = sensor->scene.srf_band_index[i];
                printf("  %s : %s\n", sensor->scene.band[index].name, sensor->scene.band[index].file_name);

                if (0 != open_a_band (&(sensor->scene.band[index])))
                {
                        printf ("Open band %d failed.\n", i);
                        return FAILURE;
                }
        }

        // open scene classification
        index = sensor->scene.qa_band_index;
        printf("  %s : %s\n", sensor->scene.band[index].name, sensor->scene.band[index].file_name);
        if(index < 0 || 0 != open_a_band(&(sensor->scene.band[index]))){
          printf("Open band qa failed.\n");
          return FAILURE;
        } 

        // open cloud & snow confidence
        index = sensor->scene.cld_band_index;
        printf("  %s : %s\n", sensor->scene.band[index].name, sensor->scene.band[index].file_name);
  if(index < 0 || 0 != open_a_band(&(sensor->scene.band[index]))){
                printf("Open band cloud confidence failed.\n");
        return FAILURE;
  } 
        index = sensor->scene.snw_band_index;
        printf("  %s : %s\n", sensor->scene.band[index].name, sensor->scene.band[index].file_name);
  if(index < 0 || 0 != open_a_band(&(sensor->scene.band[index]))){
                printf("Open band snow confidence failed.\n");
        return FAILURE;
  } 

        // get proj info from band1
        if(0 != get_scene_proj(&(sensor->scene), &sensor->insys, &sensor->inzone, &sensor->indatum,
                                                                                                        sensor->inparm, &sensor->ulx, &sensor->uly, &sensor->res)){
                printf("Get proj info failed.\n");
                return FAILURE;
        }

        //
        index = sensor->scene.srf_band_index[0];
        sensor->nrows = sensor->scene.band[index].nrow;
        sensor->ncols = sensor->scene.band[index].ncol;
        sensor->SR_fillValue = (int16)sensor->scene.band[index].fill_value;
        sensor->lndSR_ScaFct = sensor->scene.band[index].scale_factor;
        sensor->SZA = sensor->scene.szn;
        sensor->SAA = sensor->scene.san;

        printf("INSTRUMENT=%d (TM=1, ETM=2, OLI=3), SZN=%f.\n", sensor->scene.instrument, sensor->scene.szn);
        printf("NROW=%d, NCOL=%d, FILLV=%d, SCALE=%f, PROJ_SYS=%d, UTM_ZONE=%d, DATUM=%d, ULX=%f, ULY=%f, RES=%f\n",
                                        sensor->nrows, sensor->ncols, sensor->SR_fillValue, sensor->lndSR_ScaFct, sensor->insys, 
                                        sensor->inzone, sensor->indatum, sensor->ulx, sensor->uly, sensor->res);

 /*open the 30m lndsr classification map */  
 if((sensor->fpCls = fopen(sensor->clsmap, "rb"))==NULL) {
    fprintf(stderr, "\nfailed to open the input classmap file in binary mode\n");
    exit(1);
  }

 /*Yanmin 2012/06 open the txt file that saves among-classes-distance*/
 if((sensor->fpClsDist= fopen(sensor->clsDist, "r"))==NULL) {
    fprintf(stderr, "\nfailed to open  the input among-clasees-distance text file\n");
    exit(1);
  }

 sensor->fplndalbedo=fopen(sensor->lndalbedoName,"wb");
 if (sensor->fplndalbedo==NULL){
    fprintf(stderr, "\ngetSensorMetaInfo: Error in creat albedo file\n");
    exit(1);
 }

 /*Yanmin 2012/06 create a lndAlbedo QA file*/
 sensor->fplndQA=fopen(sensor->lndQAName,"wb");
 if (sensor->fplndQA==NULL){
    fprintf(stderr, "\ngetSensorMetaInfo: Error in creat lndQA file\n");
    exit(1);
 }

 /*sqs*/
 #ifdef OUT_SpectralAlbedo
 char spec_name[1024];
 sprintf(spec_name, "%s.spec.bin", sensor->lndalbedoName);
 sensor->fpSpectralAlbedo=fopen(spec_name,"wb");
 if (sensor->fpSpectralAlbedo==NULL){
    fprintf(stderr, "\ngetSensorMetaInfo: Error in creat lndSpectralAlbedo.bin file\n");
    exit(1);
  }
  #endif
        
        return SUCCESS;
}

/* allocate memory, get metadata and open specific sds for sensor data */
int openSensorFile_old(DIS_LANDSAT *sensor) 
{
  int32 sds_index=0, sds_id=0;
  int  status,i;
  char sdsName[9][20]={"band1","band2","band3","band4","band5","band7","band6","atmos_opacity","lndsr_QA"};
  int32 att_id;
  float float_att;

  double GD_upleft[2],GD_lowright[2];
  /*  int32 GD_origincode;
  int32 GD_projcode;
  int32 GD_spherecode;
  int32 GD_unit;
  int32 GD_datum;*/
  float64 GD_projparm[15];
  int ret;
  int32 gfid,gid;
  char GridName1[50]="Orthorectified_REF";
  char GridName2[50]="Grid";
  char tmpStr[100]="\0"; /*substring the lndSR file name*/
  char *p;

#ifdef DEBUG
  fprintf(stderr,"\nDM_ClsMap_NAME = %s\n", sensor->clsmap);
  fprintf(stderr,"\nDM_FILE_NAME = %s\n", sensor->fileName);
  fprintf(stderr,"DM_NCLS = %d\n", sensor->init_ncls);    
  fprintf(stderr,"DM_PROJECTION_CODE = %ld\n", sensor->insys);
  fprintf(stderr,"DM_PROJECTION_ZONE = %ld\n", sensor->inzone);
  fprintf(stderr,"DM_UPPER_LEFT_CORNER = %f, %f\n", sensor->ulx, sensor->uly);
#endif  
  
  /*read the grid metadata*/
  gfid=GDopen(sensor->fileName, DFACC_READ);
  if (gfid==GRID_ERRCODE){
    fprintf(stderr, "\nOpenSensorFile(): failed to GDopen %s\n",sensor->fileName);
    return FAILURE;
  }
  /*attach to LandSat5*/
  gid=GDattach(gfid,GridName1);
  if (gid==GRID_ERRCODE){
    /*attach to LandSat7*/
    gid=GDattach(gfid,GridName2);
    if (gid==GRID_ERRCODE){
      fprintf(stderr, "\nOpenSensorFile(): failed to GDattach the grid object %s\n",GridName1);
      return FAILURE;
    }
  }
  ret=GDprojinfo(gid,&(sensor->insys),&(sensor->inzone),&(sensor->indatum),GD_projparm);
  if(ret==GRID_ERRCODE){
    fprintf(stderr, "\nOpenSensorFile(): failed to retrieve project inform\n");
    return FAILURE;
  }

  ret=GDgridinfo(gid,&sensor->ncols,&sensor->nrows,GD_upleft,GD_lowright); 
  if(ret==GRID_ERRCODE){
    fprintf(stderr, "\nOpenSensorFile(): failed to grid info.\n");
    return FAILURE;
  }
  sensor->ulx=(double)GD_upleft[0];
  sensor->uly=(double)GD_upleft[1];

  ret=GDdetach(gid);
  if(ret==GRID_ERRCODE){
    fprintf(stderr, "\nOpenSensorFile(): failed to detach grid.\n");
    return FAILURE;
  }
  ret=GDclose(gfid);
  if(ret==GRID_ERRCODE){
    fprintf(stderr, "\nOpenSensorFile(): failed to close file\n");
    return FAILURE;
  }

  /* open sensor files (one file per band) */
  sensor->fp_id =SDstart(sensor->fileName, DFACC_READ);
  if(sensor->fp_id ==FAILURE) {
    fprintf(stderr, "\nOpenSensorFile(): failed to open input sensor hdf file\n");
    return FAILURE;
  }

   /*get the SZA value*/
   att_id=SDfindattr(sensor->fp_id,"SolarZenith");
   if (att_id!=FAIL){
           status=SDreadattr(sensor->fp_id, att_id, &float_att);
            if (status!=FAIL)
                sensor->SZA=(double)float_att;
     }
   else{
        fprintf(stderr, "\nFail to get the SZA!");
        exit(0);
    }

   /*get the pixel_size value*/
   /*   att_id=SDfindattr(sensor->fp_id,"PixelSize");
   if (att_id!=FAIL){
     status=SDreadattr(sensor->fp_id, att_id, &float_att);
     if (status!=FAIL)
       sensor->res=(double)float_att;
   }
   else{
     fprintf(stderr, "\nFail to get the pixel_size.");
     exit(0);
   }
   */

   /*get the SDS identifiers*/ 
   for (i=0;i<SENSOR_BANDS; i++){
      sds_index=SDnametoindex(sensor->fp_id, sdsName[i]);
      if (sds_index==FAILURE){
         fprintf (stderr, "\nOpenSensorFile():fail to acquire the identifier for SDS:%s.\n", sdsName[i]);
         return FAILURE;
      }  
      else{
         sds_id=SDselect(sensor->fp_id,sds_index);
         if (sds_id==FAILURE){
             fprintf (stderr, "\nOpenSensorFile():fail to select the SDS:%s.\n",sdsName[i]);
             return FAILURE;
                }
          else
             sensor->sr_id[i]=sds_id; 
       }   
    } /*end for*/

     /*get the SDS handler of the atmos_opacity band*/
        sds_index=SDnametoindex(sensor->fp_id, sdsName[7]);
        if (sds_index==FAILURE){
                 fprintf (stderr, "\nOpenInputFiles():fail to get the identifier for SDS:%s.\n", sdsName[7]);
                 return FAILURE;
        }
        else{
                sds_id=SDselect(sensor->fp_id, sds_index);
                if (sds_id==FAILURE){
                        fprintf (stderr, "\nOpenInputFiles():fail to select the SDS:%s.\n",sdsName[7]);
                        return FAILURE;
                }
                else
                        sensor->lndatmo_id=sds_id; 
       }   

        /*get the SDS handler of lndSR QA band*/
        sds_index=SDnametoindex(sensor->fp_id, sdsName[8]);
        if (sds_index==FAILURE){
                 fprintf (stderr, "\nOpenInputFiles():fail to get the identifier for SDS:%s.\n", sdsName[8]);
                 return FAILURE;
                }  
        else{
                sds_id=SDselect(sensor->fp_id,sds_index);
                if (sds_id==FAILURE){
                        fprintf (stderr, "\nOpenInputFiles():fail to select the SDS:%s.\n",sdsName[8]);
                        return FAILURE;
                }
                else
                        sensor->lndQA_id=sds_id; 
       }        
  
  /*retrieve LandSat surface reflectance fillvalue*/
  if((att_id=SDfindattr(sensor->sr_id[0],"_FillValue"))==FAILURE) {
    fprintf(stderr, "\n Failed to find LandSat SR fill value:openSensorValue" );
    return FAILURE;
  }
 
  if(SDreadattr(sensor->sr_id[0],att_id,&(sensor->SR_fillValue))==FAILURE){
      fprintf(stderr,"\nFailed to retrieve metadata-LandSat SR fill value:openSensorValue");
      return FAILURE;
  } 

  /*Get lndSR's scale factor*/
   if((att_id=SDfindattr(sensor->sr_id[0],"scale_factor"))==FAILURE) {
                fprintf(stderr, "\n OpenInputFiles(): fail to find lndSR scale factor." );
                return FAILURE;
  }
   if(SDreadattr(sensor->sr_id[0],att_id,&(sensor->lndSR_ScaFct))==FAILURE){
                fprintf(stderr,"\n OpenInputFiles(): fail to retrieve metadata-LndSR scale_factor.");
                return FAILURE;
  }

/*Get the scale factor for Landsat band6-Surface Temperature-celcius*/
   if((att_id=SDfindattr(sensor->sr_id[6],"scale_factor"))==FAILURE) {
                fprintf(stderr, "\n OpenInputFiles(): fail to find lndSR thermal bank scale factor." );
                return FAILURE;
  }
   if(SDreadattr(sensor->sr_id[6],att_id,&(sensor->lndTemp_ScaFct))==FAILURE){
                fprintf(stderr,"\n OpenInputFiles(): fail to retrieve metadata-LndTemp scale_factor.");
                return FAILURE;
  }

 /*open the 30m lndsr classification map */  
 if((sensor->fpCls = fopen(sensor->clsmap, "rb"))==NULL) {
    fprintf(stderr, "\nfailed to open the input classmap file in binary mode\n");
    exit(1);
  }

 /*Yanmin 2012/06 open the txt file that saves among-classes-distance*/
 if((sensor->fpClsDist= fopen(sensor->clsDist, "r"))==NULL) {
    fprintf(stderr, "\nfailed to open  the input among-clasees-distance text file\n");
    exit(1);
  }

 sensor->fplndalbedo=fopen(sensor->lndalbedoName,"wb");
 if (sensor->fplndalbedo==NULL){
    fprintf(stderr, "\ngetSensorMetaInfo: Error in creat albedo file\n");
    exit(1);
 }

 /*Yanmin 2012/06 create a lndAlbedo QA file*/
 sensor->fplndQA=fopen(sensor->lndQAName,"wb");
 if (sensor->fplndQA==NULL){
    fprintf(stderr, "\ngetSensorMetaInfo: Error in creat lndQA file\n");
    exit(1);
 }

 /*sqs*/
 #ifdef OUT_SpectralAlbedo
 char spec_name[1024];
 sprintf(spec_name, "%s.spec.bin", sensor->lndalbedoName);
 sensor->fpSpectralAlbedo=fopen(spec_name,"wb");
 if (sensor->fpSpectralAlbedo==NULL){
    fprintf(stderr, "\ngetSensorMetaInfo: Error in creat lndSpectralAlbedo.bin file\n");
    exit(1);
  }
  #endif
 
  return SUCCESS;
}

/* close opened Landsat sds and file, free memory */
int cleanUpSensor(DIS_LANDSAT *sensor)
{
        int i;
        int index;
        for (i = 0; i < N_SRF_BAND; i++)
        {
                index = sensor->scene.srf_band_index[i];
                close_a_band(&(sensor->scene.band[index]));
        }
        index = sensor->scene.cld_band_index;
        close_a_band(&(sensor->scene.band[index]));
        index = sensor->scene.snw_band_index;
        close_a_band(&(sensor->scene.band[index]));
        
  fclose(sensor->fpCls);
  fclose(sensor->fpClsDist);

  free(sensor->Clsdata); 
  
  return SUCCESS;
}

/**************************************************************************/
int getAmongClassesDistance(DIS_LANDSAT *LndWorkspace)
{
        char  buffer[MAX_STRLEN] = "\0";
        char  *label = NULL;
        char  *tokenptr = NULL;
        char  *seperator = "=, ";
        int i,j,myCluster,iband;
        float **mean_ClsSpec;
        int ava_cls;
        float temp_dist;
        char tmp;

        alloc_2dim_contig((void***)(&mean_ClsSpec), LndWorkspace->init_ncls, LndWorkspace->nbands, sizeof(float));
        for (i=0;i<LndWorkspace->init_ncls; i++)
                for (j=0;j<LndWorkspace->nbands; j++)
                        mean_ClsSpec[i][j]=1000;
        
        /* process line by line */
        ava_cls=0;
        while(fgets(buffer, MAX_STRLEN, LndWorkspace->fpClsDist) != NULL) {
                /*get string token*/
                tokenptr=strtok(buffer,seperator);
                label=tokenptr;
                
                /* skip comment line */
                tmp=buffer[0];
                if (tmp=='#')
                        continue;
                
              /* if(strcmp(buffer[0],"#") == 0) continue;*/
     
               sscanf(tokenptr,"%d ",&myCluster);
               
               if (myCluster!=-1){
                ava_cls++;
                for (i=0;i<LndWorkspace->nbands;i++){
                        tokenptr=strtok(NULL, seperator);
                                sscanf(tokenptr,"%f ",&temp_dist);
                                mean_ClsSpec[myCluster][i]=temp_dist;
                        } 
               }               
        }
        
        /*Yanmin 2012/06 update the nclass# */
        LndWorkspace->actu_ncls=ava_cls;
        
        for (i=0;i<LndWorkspace->init_ncls;i++)
                for (j=0;j<LndWorkspace->init_ncls;j++)
                        LndWorkspace->AmongClsDistance[i][j]=0.0;
        
        /*calculate the among-classes-distance, only save in the upper-triangle*/
        /* six spectral bands excluding the temperature band*/
        for (i=0;i<LndWorkspace->actu_ncls; i++)
                for (j=i+1;j<LndWorkspace->actu_ncls;j++){
                        temp_dist=0;
                        for (iband=0;iband<LndWorkspace->nbands;iband++)
                                temp_dist+=(mean_ClsSpec[i][iband]-mean_ClsSpec[j][iband])*(mean_ClsSpec[i][iband]-mean_ClsSpec[j][iband]);
                        if (temp_dist>=0)
                          temp_dist=sqrt(temp_dist);
                        else
                          temp_dist=0.0;

                        LndWorkspace->AmongClsDistance[i][j]=temp_dist;                 
        }

        free_2dim_contig((void **)mean_ClsSpec);
        
        return SUCCESS;  

} /*end getAmongClassesDistance() */
/**************************************************************************/
int GetEquation4VZACorrection(DIS_LANDSAT *LndWorkspace)
{
        int irow,icol;
        int16 *oneRow; /*[icol]*/
        int16 **oneBand; /*[irow][icol]*/
        int16 tmpVal,tmpRow,tmpCol;
        int32 start[2],length[2];  
        int find_UL,find_UR;
        int find_LL,find_LR;
        int find_nonFill;
        int UpperLeft[2],UpperRight[2]; /*col, row*/
        int LowerLeft[2],LowerRight[2];
        double k,b,P_SE[2],P_NE[2],dist_NE,dist_SE;
        
        alloc_1dim_contig((void **) (&oneRow), LndWorkspace->ncols, sizeof(int16)); 
        alloc_2dim_contig((void ***) (&oneBand), LndWorkspace->nrows, LndWorkspace->ncols, sizeof(int16)); 
        
        int     index = LndWorkspace->scene.srf_band_index[0];

        /*Yanmin 2015/12/16  Find out the four corners of available data region in one Landsat scene */

        /*read out one band into memory for seaking the corners*/
      for (irow=0;irow<LndWorkspace->nrows;irow++){
                start[0]=irow;
                start[1]=0;
                length[0]=1;
                length[1]=LndWorkspace->ncols;          
                if(0 != read_a_band_a_row(&(LndWorkspace->scene.band[index]), irow, (void *)oneRow)){
                        printf("Read failed.\n");
                        return FAILURE;
                }               
                for (icol=0;icol<LndWorkspace->ncols;icol++)
                        oneBand[irow][icol]=oneRow[icol];                       
        }/*end for(irow=0;...) read the whole band into memory */

        find_UL=0;
        find_UR=0;
        find_LR=0;
        find_LL=0;
        /*find the most northern corner-UpperLeft or UpperRight corner*/
        for (irow=0;irow<LndWorkspace->nrows; irow++){
                for (icol=0;icol<LndWorkspace->ncols;icol++)
                        if (oneBand[irow][icol]!=LndWorkspace->SR_fillValue){
                           if (icol < 0.5*LndWorkspace->ncols){
                                find_UL=1;
                                UpperLeft[0]=icol;
                                UpperLeft[1]=irow;
                                break;
                           }
                          else{
                                find_UR=1;
                                UpperRight[0]=icol;
                                UpperRight[1]=irow;
                                break;
                          }                       
                        }
                if ((find_UL==1)||(find_UR==1))
                        break;
        }
                                
        /*find the most southern corner-LowerLeft or LowerRight corner*/
        for (irow=LndWorkspace->nrows-1; irow>=0;irow--){
                for (icol=LndWorkspace->ncols-1;icol>=0;icol--)
                        if (oneBand[irow][icol]!=LndWorkspace->SR_fillValue){
                                if(icol > 0.5*LndWorkspace->ncols){
                                        find_LR=1;
                                        LowerRight[0]=icol;
                                        LowerRight[1]=irow;
                                        break;
                                }
                                else{
                                        find_LL=1;
                                        LowerLeft[0]=icol;
                                        LowerLeft[1]=irow;
                                        break;
                                }
                        }
                if((find_LR==1)||(find_LL==1))
                        break;
        }
                        
        /*find the most eastern corner-UpperRight or Lowerright corner*/
        find_nonFill=0;
        for (icol=LndWorkspace->ncols-1;icol>=0;icol--){
                for (irow=0; irow<LndWorkspace->nrows;irow++)
                        if (oneBand[irow][icol]!=LndWorkspace->SR_fillValue){
                                        tmpCol=icol;
                                        tmpRow=irow;
                                        find_nonFill=1;
                                        break;
                                }
                if (find_nonFill==1)
                        break;
                }
                        
        if((tmpRow<0.5*LndWorkspace->nrows) && (find_UR!=1)){
                        find_UR=1;
                        UpperRight[0]=tmpCol;
                        UpperRight[1]=tmpRow;
        }
        if((tmpRow>0.5*LndWorkspace->nrows) && (find_LR!=1)){
                        find_LR=1;
                        LowerRight[0]=tmpCol;
                        LowerRight[1]=tmpRow;
        }
                
        /*find the most western corner-UpperLeft or LowerLeft corner */
        find_nonFill=0;
        for (icol=0; icol<LndWorkspace->ncols;icol++) {
                for (irow=LndWorkspace->nrows-1; irow>=0;irow--)
                        if (oneBand[irow][icol]!=LndWorkspace->SR_fillValue){
                                tmpCol=icol;
                                tmpRow=irow;
                                find_nonFill=1;
                                break;
                        }
        if (find_nonFill==1)
                break;
        }
                        
        if((irow<0.5*LndWorkspace->nrows) && (find_UL!=1)){
                find_UL=1;
                UpperLeft[0]=tmpCol;
                UpperLeft[1]=tmpRow;
        }
        if((irow>0.5*LndWorkspace->nrows) && (find_LL!=1)){
                find_LL=1;
                LowerLeft[0]=tmpCol;
                LowerLeft[1]=tmpRow;
        }
                        
        if((find_UL!=1)||(find_UR!=1)||(find_LL!=1)||(find_LR!=1)){
                 fprintf (stderr, "\nGetFormula4VZAcorrection():fail to find four corners of available data region.\n");
                return FAILURE;
        }

        /*calculate the slop and intercection for the VZA=0 line 
                according to (x1-x2)/(x2-x1)=(y1-y2)/(y2-y1)
                and the product of two slopes (line & its perpendicular line) is -1*/
        if ((UpperRight[1]==UpperLeft[1])||(LowerRight[1]==LowerLeft[1])||(UpperRight[0]==UpperLeft[0])||(LowerRight[0]==LowerLeft[0])){
                fprintf(stderr,"\nGetEquation4VZACorrection():error when calculates the slop for VZA=0 line\n");
                return FAILURE;
                }

        P_NE[0]=0.5*(UpperRight[0]+UpperLeft[0]); /* col  */
        P_NE[1]=0.5*(UpperRight[1]+UpperLeft[1]); /* row */
        P_SE[0]=0.5*(LowerLeft[0]+LowerRight[0]); /* col  */
        P_SE[1]=0.5*(LowerLeft[1]+LowerRight[1]); /* row */

        k=(P_NE[1]-P_SE[1])/(double)(P_NE[0]-P_SE[0]);
        b=P_SE[1]-k*P_SE[0];

        LndWorkspace->slop=k;
        LndWorkspace->interception=b;
        
        /* slop2 and interception2 are for the horizontal line, i.e. the upper edge of a scene */
        LndWorkspace->slop2=-1.0/k;
        LndWorkspace->interception2=UpperLeft[1]-LndWorkspace->slop2*UpperLeft[0];

        dist_NE=sqrt(pow((UpperLeft[0]-UpperRight[0]),2)+pow((UpperLeft[1]-UpperRight[1]),2));
        dist_SE=sqrt(pow((LowerLeft[0]-LowerRight[0]),2)+pow((LowerLeft[1]-LowerRight[1]),2));
        LndWorkspace->half_dist=0.5*(dist_NE+dist_SE)*0.5;
        
        LndWorkspace->img_upleft[0] = UpperLeft[0];
        LndWorkspace->img_upleft[1] = UpperLeft[1];
        LndWorkspace->img_upright[0] = UpperRight[0];
        LndWorkspace->img_upright[1] = UpperRight[1];
        LndWorkspace->NE_center[0]=P_NE[0];
        LndWorkspace->NE_center[1]=P_NE[1];
        LndWorkspace->SE_center[0]=P_SE[0];
        LndWorkspace->SE_center[1]=P_SE[1];
        
        free_1dim_contig((void *)oneRow);
        free_2dim_contig((void **)oneBand);
        
        return SUCCESS;  
        
}

/**************************************************************************/
int GetEquation4VZACorrection_old(DIS_LANDSAT *LndWorkspace)
{
        int irow,icol;
        /*int iband;*/
        int16 *oneRow; /*[icol]*/
        int32 start[2],length[2];  
        /*int16 tmp_sr;*/
        int find_UL,find_UR;
        int find_LL,find_LR;
        int UpperLeft[2],UpperRight[2]; /*col, row*/
        int LowerLeft[2],LowerRight[2];
        int MostRightInLastRow,MostRightInCurrentRow;
        int MostLeftInLastRow, MostLeftInCurrentRow;
        double k,b,P_SE[2],P_NE[2],dist_NE,dist_SE;
        
        alloc_1dim_contig((void **)  (&oneRow), LndWorkspace->ncols, sizeof(int16)); 
        
        int     index = LndWorkspace->scene.srf_band_index[0];
        
        /*find the Upper Left (UL) and Upper Right (LR) corner of available scan-Yanmin 2015/11/10*/
        find_UL=0;
        find_UR=0;
        MostRightInCurrentRow=-1;
        MostRightInLastRow=-1;
        for (irow=0;irow<LndWorkspace->nrows;irow++){

                start[0]=irow;
                start[1]=0;
                length[0]=1;
                length[1]=LndWorkspace->ncols;
                /*if (SDreaddata(LndWorkspace->sr_id[0],start,NULL,length,oneRow)==FAILURE){
                     fprintf (stderr, "\nGetFormula4VZAcorrection():fail to read LndSR at row:%d.",irow);
                     exit(1);
                }*/
                if(0 != read_a_band_a_row(&(LndWorkspace->scene.band[index]), irow, (void *)oneRow)){
                        printf("Read failed.\n");
                        return FAILURE;
                }
        
                for (icol=0;icol<LndWorkspace->ncols;icol++){
                        if ((oneRow[icol]!=LndWorkspace->SR_fillValue)&& (find_UL==0)&&(find_UR==0)){
                                UpperLeft[0]=icol;
                                UpperLeft[1]=irow;
                                find_UL=1;

                                MostRightInCurrentRow=icol;
                        }
                        if ((oneRow[icol]!=LndWorkspace->SR_fillValue)&& (find_UL==1)&&(find_UR==0))
                          if (icol>MostRightInCurrentRow)                          
                            MostRightInCurrentRow=icol;                         
                }
        
                if ((MostRightInCurrentRow<=MostRightInLastRow)&&(find_UL==1)){
                        UpperRight[0]=MostRightInLastRow;
                        UpperRight[1]=irow-1;
                        find_UR=1;
                }
                if ((MostRightInCurrentRow>MostRightInLastRow)&&(find_UL==1))
                      MostRightInLastRow=MostRightInCurrentRow;
                
                /*break the for (irow) loop when both actural corners are found.*/
                if ((find_UR==1)&&(find_UL==1))
                        break;

                /*Yanmin 2012/05 continue to seek the upper-right point if the current UR and UL are not far enough*/
                /*if ((find_UR==1)&&(find_UL==1)&(abs(UpperRight[1]-UpperLeft[1])<0.01*LndWorkspace->ncols)){
                        find_UR=0;
                        UpperLeft[0]=UpperRight[0];
                        UpperLeft[1]=UpperRight[1]+1;                           
                }  */
        }/* end for (irow=0; irow... )  to seek the UL and UR corners */

       /*find the Lower Left (LL) and Lower Right (LR) corner of available scan-Yanmin 2015/11/10*/
        find_LL=0;
        find_LR=0;
        MostLeftInCurrentRow=LndWorkspace->ncols;
        MostLeftInLastRow=LndWorkspace->ncols;
        for (irow=LndWorkspace->nrows-1;irow>=0;irow--){
                start[0]=irow;
                start[1]=0;
                length[0]=1;
                length[1]=LndWorkspace->ncols;
                /*if (SDreaddata(LndWorkspace->sr_id[0],start,NULL,length,oneRow)==FAILURE){
                     fprintf (stderr, "\nGetFormula4VZAcorrection():fail to read LndSR at row:%d.",irow);
                     exit(1);
                }*/
                if(0 != read_a_band_a_row(&(LndWorkspace->scene.band[index]), irow, (void *)oneRow)){
                        printf("Read failed.\n");
                        return FAILURE;
                }
        
                for (icol=LndWorkspace->ncols-1;icol>=0;icol--){
                        if ((oneRow[icol]!=LndWorkspace->SR_fillValue)&& (find_LR==0)&&(find_LL==0)){
                                LowerRight[0]=icol;
                                LowerRight[1]=irow;
                                find_LR=1;

                                MostLeftInCurrentRow=icol;
                        }
                        if ((oneRow[icol]!=LndWorkspace->SR_fillValue)&& (find_LR==1)&&(find_LL==0))
                          if (icol<MostLeftInCurrentRow)                           
                            MostLeftInCurrentRow=icol;                  
                }
        
                if ((MostLeftInCurrentRow>=MostLeftInLastRow)&&(find_LR==1)&&(find_LL==0)){
                        LowerLeft[0]=MostLeftInLastRow;
                        LowerLeft[1]=irow+1;
                        find_LL=1;
                }
                if ((MostLeftInCurrentRow<MostLeftInLastRow)&&(find_LR==1)&&(find_LL==0))
                      MostLeftInLastRow=MostLeftInCurrentRow;
                
                /*break the for (irow) loop when both actural corners are found.*/
                if ((find_LR==1)&&(find_LL==1))
                        break;
                
        }/*end for (irow=0; irow... ) to seek the LL and LR corners */

        /*calculate the slop and intercection for the VZA=0 line 
                according to (x1-x2)/(x2-x1)=(y1-y2)/(y2-y1)
                and the product of two slopes (line & its perpendicular line) is -1*/
        if ((UpperRight[1]==UpperLeft[1])||(LowerRight[1]==LowerLeft[1])||(UpperRight[0]==UpperLeft[0])||(LowerRight[0]==LowerLeft[0])){
                fprintf(stderr,"\nGetEquation4VZACorrection():error when calculates the slop for VZA=0 line\n");
                exit(1);
                }

        P_NE[0]=0.5*(UpperRight[0]+UpperLeft[0]); /* col  */
        P_NE[1]=0.5*(UpperRight[1]+UpperLeft[1]); /* row */
        P_SE[0]=0.5*(LowerLeft[0]+LowerRight[0]); /* col  */
        P_SE[1]=0.5*(LowerLeft[1]+LowerRight[1]); /* row */

        k=(P_NE[1]-P_SE[1])/(double)(P_NE[0]-P_SE[0]);
        b=P_SE[1]-k*P_SE[0];

        LndWorkspace->slop=k;
        LndWorkspace->interception=b;
        
        /* slop2 and interception2 are for the horizontal line, i.e. the upper edge of a scene */
        LndWorkspace->slop2=-1.0/k;
        LndWorkspace->interception2=UpperLeft[1]-LndWorkspace->slop2*UpperLeft[0];

        dist_NE=sqrt(pow((UpperLeft[0]-UpperRight[0]),2)+pow((UpperLeft[1]-UpperRight[1]),2));
        dist_SE=sqrt(pow((LowerLeft[0]-LowerRight[0]),2)+pow((LowerLeft[1]-LowerRight[1]),2));
        LndWorkspace->half_dist=0.5*(dist_NE+dist_SE)*0.5;
        
        free_1dim_contig((void *)oneRow);
        
        LndWorkspace->img_upleft[0] = UpperLeft[0];
        LndWorkspace->img_upleft[1] = UpperLeft[1];
        LndWorkspace->img_upright[0] = UpperRight[0];
        LndWorkspace->img_upright[1] = UpperRight[1];
        LndWorkspace->NE_center[0]=P_NE[0];
        LndWorkspace->NE_center[1]=P_NE[1];
        LndWorkspace->SE_center[0]=P_SE[0];
        LndWorkspace->SE_center[1]=P_SE[1];
        
        return SUCCESS;  

}

/**************************************************************************/
double lndSRPixVZA(DIS_LANDSAT *LndWorspace, int row,int col)
{
        double half_dist;
        double pointDist;
        double k,b;
        double VZA,x_ct;

        half_dist=LndWorspace->half_dist;
        k=LndWorspace->slop;
        b=LndWorspace->interception;
        
        pointDist=abs(k*col-row+b)/sqrt(1+pow(k,2));

        VZA=7.5*pointDist/half_dist; /*VZA in degrees*/
        /*Determine the view direction: forward- or backward+*/
        x_ct=(row-b)/k;
        if (x_ct<=col) /*forward-opposite direction with sun incident*/
                VZA=-1*VZA;
        
        return VZA;
}
/*********************************************************/
double CalPixVZA(DIS_LANDSAT *LndWorkspace,int irow,int icol)
{
        double VZA,x_ct;
        double PixDist;
        double k,b;

        /*calculate the distance from the current pixel to the VZA=0 line*/
        k=LndWorkspace->slop;
        b=LndWorkspace->interception;
        
        PixDist=abs(k*icol-irow+b)/sqrt(1+pow(k,2));

        VZA=7.5*PixDist/LndWorkspace->half_dist; /*VZA in degrees*/
        /*Determine the view direction: forward- or backward+*/
        x_ct=(irow-b)/k;
        if (x_ct<=icol) /*forward-opposite direction with sun incident*/
                VZA=-1*VZA;

        //sqs
        if(VZA < 0.0){
                VZA *= -1.0;
        }

        return VZA;
}

/**********************************************************************************/
double GetVZARefCorrectionFactor(int16 *tmp_brdf,double SZA,double iVZA)
{
        double ref_nadir;
        double ref_vza;
        double k_iso,k_geo,k_vol;
         double brdf_scale=0.001;
         double VZACrFct;

         SZA=SZA*D2R;
         iVZA=iVZA*D2R;
         
         k_iso=1.0;
         k_geo=LiSparseR_kernel(0.0, SZA, 0.0);
         k_vol=RossThick_kernel(0.0, SZA, 0.0);
         ref_nadir=brdf_scale*(k_iso*tmp_brdf[0]+k_vol*tmp_brdf[1]+k_geo*tmp_brdf[2]);

         k_iso=1.0;
         k_geo=LiSparseR_kernel(iVZA, SZA, 0.0);
         k_vol=RossThick_kernel(iVZA, SZA, 0.0);
         ref_vza=brdf_scale*(k_iso*tmp_brdf[0]+k_vol*tmp_brdf[1]+k_geo*tmp_brdf[2]);

         if (ref_vza!=0){
             VZACrFct=ref_nadir/ref_vza;
             return VZACrFct;   
         }
        else{
                fprintf(stderr,"\nGetVZARefCorrectionFactor():Fail to calculate the VZA-correction-factor.\n");
                return FAILURE;
                }

}
/**************************************************/
int WriteOutHeadFile(DIS_LANDSAT *sensor)
{
  char *fname_hdr, tmp[MAX_STRLEN]={'\0'};
  FILE *fp_hdr; /* write out a head file for the lndAlbedo */
  int myStrLen,i;

  myStrLen=strlen(sensor->lndalbedoName);
  /* strncpy(tmp,sensor->lndalbedoName, myStrLen-4); */
  /* for (i=myStrLen-4;i<myStrLen;i++) */
  /*   tmp[i]='\0'; */
  strncpy(tmp,sensor->lndalbedoName, myStrLen);
  
  fname_hdr=strcat(tmp,".hdr");
  if (fname_hdr==NULL){
         fprintf(stderr, "\nWriteOutHeadFile: Failed at the strncat() for lndAlbedo!");
        return FAILURE;
        }
//  tmp[myStrLen+4]='\0';
  
 if((fp_hdr=fopen(fname_hdr,"w"))==NULL){
         fprintf(stderr,"\nWriteOutHeadFile:Can't create hdr file:%s.\n",fname_hdr);
         return FAILURE;
   }
  fprintf(fp_hdr,"ENVI\ndescription = {\n  File Imported into ENVI.}");
  fprintf(fp_hdr,"\nsamples = %d\nlines   = %d\nbands  = 2\nheader offset = 0",sensor->ncols,sensor->nrows);
  fprintf(fp_hdr,"\nfile type = ENVI Standard\ndata type = 2\ninterleave = bip\nsensor type = Landsat");
  fprintf(fp_hdr,"\nbyte order = 0\nwavelength units = Shortwave band 300-2500nm");
  fprintf(fp_hdr,"\nband names ={\n\t(%dX%d):Shortwave band black sky albedo: Band 1,",sensor->ncols,sensor->nrows);
  fprintf(fp_hdr,"\n\t(%dX%d):Shortwave band white sky albedo: Band 2}",sensor->ncols,sensor->nrows);
  fprintf(fp_hdr,"\nmap info={UTM, 1.000, 1.000, %f, %f, %f, %f, ",sensor->ulx,sensor->uly, sensor->res, sensor->res);
  fprintf(fp_hdr," %d, North, WGS-84, units=Meters}",sensor->inzone);
  fclose(fp_hdr);

  /*Yanmin 2012/06 write out a head file for the lndQA file*/
  myStrLen=strlen(sensor->lndQAName);
  /* strncpy(tmp,sensor->lndQAName,myStrLen-4); */
  /* for (i=myStrLen-4;i<myStrLen;i++) */
  /*   tmp[i]='\0'; */
  memset(tmp, 0, strlen(tmp));
  strncpy(tmp,sensor->lndQAName,myStrLen);
    
  fname_hdr=strcat(tmp,".hdr");
   if (fname_hdr==NULL){
         fprintf(stderr, "\n WriteOutHeadFile:Failed at the strncat() for lndQA!");
        return FAILURE;
        }
 if((fp_hdr=fopen(fname_hdr,"w"))==NULL){
         fprintf(stderr,"\nWriteOutHeadFile:Can't create hdr file:%s .\n",fname_hdr);
         return FAILURE;
   }
  fprintf(fp_hdr,"ENVI\ndescription = {\n  File Imported into ENVI.}");
  fprintf(fp_hdr,"\nsamples = %d\nlines   = %d\nbands  = 1\nheader offset = 0",sensor->ncols,sensor->nrows);
  fprintf(fp_hdr,"\nfile type = ENVI Standard\ndata type = 1\ninterleave = bip\nsensor type = Landsat");
  fprintf(fp_hdr,"\nbyte order = 0\nwavelength units = Shortwave band 300-2500nm");
  fprintf(fp_hdr,"\nband names ={\n\t(%dX%d):Shortwave band QA: Band 1}",sensor->ncols,sensor->nrows);
 
  fprintf(fp_hdr,"\nmap info={UTM, 1.000, 1.000, %f, %f, %f, %f, ",sensor->ulx,sensor->uly, sensor->res, sensor->res);
  fprintf(fp_hdr," %d, North, WGS-84, units=Meters}",sensor->inzone);
  fclose(fp_hdr);

 /*sqs*/
#ifdef OUT_SpectralAlbedo
 int j;
 sprintf(tmp, "%s.spec.bin.hdr", sensor->lndalbedoName);
 char alb_types[2][10] = {"BSA", "WSA"};
  
 if((fp_hdr=fopen(fname_hdr,"w"))==NULL){
         fprintf(stderr,"\nWriteOutHeadFile:Can't create hdr file:%s.\n",fname_hdr);
         return FAILURE;
   }
  fprintf(fp_hdr,"ENVI\ndescription = {\n  File Imported into ENVI.}");
  fprintf(fp_hdr,"\nsamples = %d\nlines   = %d\nbands  = 12\nheader offset = 0",sensor->ncols,sensor->nrows);
  fprintf(fp_hdr,"\nfile type = ENVI Standard\ndata type = 2\ninterleave = bip\nsensor type = Landsat");
  fprintf(fp_hdr,"\nbyte order = 0\nwavelength units = Shortwave band 300-2500nm");
  fprintf(fp_hdr,"\nband names ={\n\t");
        for(i=0; i<6; i++){
                for(j=0; j<2; j++){
                        fprintf(fp_hdr, "%s_BAND%d", alb_types[j], i+1);
                        if(!(i == 5 && j == 1)){
                                fprintf(fp_hdr, ",\n");
                        }
                }
        }
        fprintf(fp_hdr, "}");
        fprintf(fp_hdr,"\nmap info={UTM, 1.000, 1.000, %f, %f, %f, %f, ",sensor->ulx,sensor->uly, sensor->res, sensor->res);
  fprintf(fp_hdr," %d, North, WGS-84, units=Meters}",sensor->inzone);
  fclose(fp_hdr);
#endif
 
 return  SUCCESS;
}

/*************************************************/
// cloud: return 1
// snow: return 2
int CloudMask(DIS_LANDSAT *LndWorkspace,int icol)
{
        int temp;
        int bitp[32];
        
        uint16 iQA=LndWorkspace->OneRowlndSRQAData[icol];

        // usgs cfmask
        // 0: clear
        // 1: water
        // 2: cloud shadow
        // 3: snow
        // 4: cloud

        /*if(iQA == 2 || iQA == 4 || iQA == 255){
                return 1;
        }
        else if(iQA == 3){
                return 2;
        }*/

        /*
          Sentinel-2 scene classification
          0: no data
          1: saturated
          2: shadow
          3: cloud shadow
          4: vegetation
          5: bare soil, desert
          6: water
          7: cloud low proba
          8: cloud medium proba
          9: cloud high proba
          10: thin cirrus
          11: snow ice
        */
        if(iQA == 0 || iQA == 3 || iQA == 7 || iQA == 8 || iQA == 9 || iQA == 10){
          return 1;
        }
        else if(iQA == 11){
          return 2;
        }

        return 0;       
}

/**********************************************************/
// no thermal band any more
int SnowDetermination(DIS_LANDSAT *LndWorkspace, int icol)
{
        double NDSI;
        double NDVI,diff;

        double b1,b2,b3,b4,b5,b6,b7;
        int snowflag;

        snowflag=0;

        // use cfmask
        
        uint16 mask=LndWorkspace->OneRowlndSRSnwData[icol];
        if(mask > 50){
                snowflag = 1;
        }

        return snowflag;        
        
        b1=LndWorkspace->OneRowlndSR[0][icol];
        b2=LndWorkspace->OneRowlndSR[1][icol];
        b3=LndWorkspace->OneRowlndSR[2][icol];
        b4=LndWorkspace->OneRowlndSR[3][icol];
        b5=LndWorkspace->OneRowlndSR[4][icol];
        b7=LndWorkspace->OneRowlndSR[5][icol];
        //b6=LndWorkspace->OneRowlndSR[6][icol]; /* thermal band in celsius */
                
        if ((b2==0) && (b5==0)){
                /*fprintf(stderr,"\nSnowDetermination(): invalid TM_b2 and TM_b5 for the NDSI calculation.\n");*/
                snowflag=2;
                return snowflag;
        }
        NDSI=(b2-b5)/(b2+b5);

        if ((b3==0) && (b4==0)){
                /*fprintf(stderr,"\nSnowDetermination(): invalid TM_b2 and TM_b5 for the NDSI calculation.\n");*/
                snowflag=2;
                return snowflag;
        }
        NDVI=(b4-b3)/(b4+b3);

        //b6=b6*LndWorkspace->lndTemp_ScaFct+273.15; /*convert to Kelvin*/
        
        b2=b2*LndWorkspace->lndSR_ScaFct; /*apply the scale factor to reflectance*/
        b4=b4*LndWorkspace->lndSR_ScaFct;
        
        //if ((NDSI>0.4)&&(b4>0.11)&&(b2>0.10)&&(b6<277)) 
        if ((NDSI>0.4)&&(b4>0.11)&&(b2>0.10)) 
          snowflag=1;

        diff=fabs(NDVI-0.1);
        //if ((diff<0.005)&&(b6<277)) 
        if ((diff<0.005)) 
          snowflag=1; /*constraint with a forest class may need*/

        return snowflag;

}

/*************************************************/
int getClosestCls(DIS_LANDSAT *LndWorkspace, int icls,int band)
{
        int mycls;
        float min_dist;
        int i;
        int brdfNotAva;

        min_dist=100000;
        mycls=-1;
        
        for (i=0;i<LndWorkspace->actu_ncls;i++){
                brdfNotAva=0;
                if ((LndWorkspace->BRDF_paras[i][band][0]==0)&&(LndWorkspace->BRDF_paras[i][band][1]==0)&&(LndWorkspace->BRDF_paras[i][band][2]==0))
                        brdfNotAva=1;
                        
                if ((icls!=i)&&(LndWorkspace->AmongClsDistance[icls][i]<min_dist)&&(brdfNotAva==0)){
                        min_dist=LndWorkspace->AmongClsDistance[icls][i];
                        mycls=i;
                }
        }

        return mycls;
}/* end getClosestCls()*/

/**************************************************************************/

double CalPixAzimuth(DIS_LANDSAT *LndWorkspace,int irow,int icol)
/*Yanmin 2015 added this routine to calcuate pixel-based azimuth */
{
        double Azimuth;
        double k,b;
        double X_sc,Y_sc;
        double delta_row, delta_col;

        double distance, tmp_dis;
        int index;
        
        k=LndWorkspace->slop2;
        b=LndWorkspace->interception2;


        tmp_dis=k*icol-irow+b;
        if (tmp_dis<0)
                tmp_dis*=-1.0;
        
        distance = tmp_dis/sqrt(1.0+k*k);

        index = (int)(distance /LndWorkspace->oneScanDist_alongNadir);

        X_sc = LndWorkspace->scan_centers[0][index];
        Y_sc = LndWorkspace->scan_centers[1][index];

        double delt_x = X_sc - icol;
        double delt_y = irow - Y_sc;

        Azimuth = atan(delt_x / delt_y) * R2D;
        if(Azimuth < 0.0){
                Azimuth *= -1.0;
        }

        if(delt_x > 0.0 && delt_y > 0.0){
                ;
        }
        else if (delt_y < 0.0 && delt_x > 0.0){
                Azimuth = 180.0 - Azimuth;
        }
        else if (delt_y < 0.0 && delt_x < 0.0){
                Azimuth = Azimuth - 180.0;
        }
        else if (delt_y > 0.0 && delt_x < 0.0){
                Azimuth = -1.0*Azimuth;
        }

        //printf("row=%d, col=%d, azimuth=%f, index=%d, center xy=%f, %f, distance=%f\n", irow,icol, Azimuth, index, X_sc, Y_sc, distance);

        return Azimuth;
}

/**************************************************************************/
int CalcScanCenters(DIS_LANDSAT *LndWorkspace)
{
        int i;
        double delta_col,delta_row;

        delta_col=(LndWorkspace->SE_center[0]-LndWorkspace->NE_center[0])*1.0/NSCAN;
        delta_row=(LndWorkspace->SE_center[1]-LndWorkspace->NE_center[1])*1.0/NSCAN;
        if ((LndWorkspace->SE_center[0]==LndWorkspace->NE_center[0])||(LndWorkspace->SE_center[1]==LndWorkspace->NE_center[1]))
          return FAILURE;

        LndWorkspace->scan_centers[0][0]=LndWorkspace->NE_center[0]+0.5*delta_col;
        LndWorkspace->scan_centers[1][0]=LndWorkspace->NE_center[1]+0.5*delta_row;
        
        for(i=1; i<NSCAN; i++){
                LndWorkspace->scan_centers[0][i]=LndWorkspace->scan_centers[0][i-1]+delta_col;
                LndWorkspace->scan_centers[1][i]=LndWorkspace->scan_centers[1][i-1]+delta_row;  
        }
        
        LndWorkspace->oneScanDist_alongNadir=sqrt(delta_col*delta_col+delta_row*delta_row);
        
        return SUCCESS;
}
/**************************************************************************/

int CalcScanCenters_old(DIS_LANDSAT *LndWorkspace)
{
        int i;
        double r_step = (sin(atan(LndWorkspace->slop))*LINES_PER_SCAN);

        if(r_step < 0){
                r_step *= -1.0;
        }

        double scan_center[2];
        //first line center
  scan_center[0]        = (LndWorkspace->img_upright[0]+LndWorkspace->img_upleft[0])/2;
  scan_center[1]        = (LndWorkspace->img_upright[1]+LndWorkspace->img_upleft[1])/2;

        printf("r_step=%f\n", r_step);
        printf("img_upleft=%d, %d, upright=%d, %d\n",LndWorkspace->img_upleft[0],LndWorkspace->img_upleft[1],
                                                                        LndWorkspace->img_upright[0], LndWorkspace->img_upright[1]);
        printf("first line center=%f, %f\n", scan_center[0], scan_center[1]);
        printf("slop1=%f, interc=%f\n", LndWorkspace->slop, LndWorkspace->interception);
        printf("slop2=%f, interc2=%f\n", LndWorkspace->slop2, LndWorkspace->interception2);


        for(i=0; i<NSCAN; i++){
                //int p_x = scan_center[0];     
                double p_y = scan_center[1]+r_step;     
                if(i==0){
                        p_y = scan_center[1]+r_step/2.0;        
                }

                scan_center[0] = (p_y - LndWorkspace->interception)/LndWorkspace->slop;
                scan_center[1] = p_y;

                LndWorkspace->scan_centers[0][i] = scan_center[0];
                LndWorkspace->scan_centers[1][i] = scan_center[1];

                if(LndWorkspace->scan_centers[0][i] >= LndWorkspace->ncols ||
                         LndWorkspace->scan_centers[1][i] >= LndWorkspace->nrows){
                        return FAILURE;
                }

                printf("Center %d: %f, %f\n", i, scan_center[0], scan_center[1]);
        }
        
  return SUCCESS;
}
