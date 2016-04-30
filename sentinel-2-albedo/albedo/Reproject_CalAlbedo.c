/* 
 * !Description
 * Reproject Landsat UTM to MODIS SIN projection, and prepare to calcuate albedo 
 *      
 * !Usage
 *   parseParameters (filename, ptr_LndSensor, ptr_MODIS)
 * 
 * !Developer
 *  Initial version (V1.0.0) Yanmin Shuai (Yanmin.Shuai@nasa.gov)
 *                                              and numerous other contributors
 *                                              (thanks to all)
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

#include "assert.h"
#include "lndsr_albedo.h"


int ReprojectLnd2MODIS (DIS_LANDSAT * sensor, GRID_MODIS * modis)
{
        int i, j, k, band;
        /*int agg_exist; */
        int min_count;
        int16 icls;
        /*int sensor_good_flag; */
        int mi = 0, mj = 0;
        int bitp[32], tmp;
        int16 max_percent;
        int16 maj_cls;
        int16 **count;                          /*[irow][icol] */
        int16 ***mcls;                          /* stores percentage of cluster type in MODIS pixels in [irow][icol][icls] */

        /*long int  **his_ClsPix; *//* store the histogram of class percent  for each class */
        long int his_ClsPix[MAX_cls + 1][100 / HIS_BIN + 2];

        long int **AvaPix_cnt;          /*[icls][iband]; store the TOP_Pct pixel# for each band */
        /*long int AvaPix_cnt[MAX_cls+1][MODIS_NBANDS]; */

        int16 tmpRatio[MODIS_NBANDS * 2];
        int HisBininPos;
        long int tmphis;

        FILE *fp_his;                           /* print out the histogrms of majority percentage for each class */
        /* FILE *fp_ratioMap; output the 2400*2400 WSA & BSA ratio map for each MODIS band */

        alloc_3dim_contig ((void ****) (&mcls), modis->nrows, modis->ncols, sensor->init_ncls + 1, sizeof (int16));
        alloc_2dim_contig ((void ***) (&count), modis->nrows, modis->ncols, sizeof (int16));
        alloc_2dim_contig ((void ***) (&AvaPix_cnt), sensor->init_ncls + 1, MODIS_NBANDS, sizeof (long int));
        /*alloc_2dim_contig((void ***)(&his_ClsPix), sensor->init_ncls+1, 100/HIS_BIN+2, sizeof(long int)); */

        /*only six bands (band1-5 & 7) used in the albedo calculation wheather the thermal band is included in the lndSr or not */
        sensor->nbands = 6;

        if ((fp_his = fopen (modis->his_name, "w")) == NULL)
        {
                printf ("\n Can't creat the histogram output file");
                exit (1);
        }

        /*if((fp_ratioMap=fopen(modis->ratioMap,"wb"))==NULL){
           printf("\n Can't creat the ratioMap output file");
           exit(1);
           } */

        /*initialize the statistics counters */
        /*for(k=0; k<sensor->init_ncls+1; k++){
           for (i=0;i<MODIS_NBANDS;i++){
           sensor->BRDF_paras[k][i][0]=0.0;
           AvaPix_cnt[k][i]=0;
           }
           } */

        modis->agg_fp = fopen (modis->aggFileName, "wb");
        if (modis->agg_fp == NULL)
        {
                printf ("\nCan't creat the output majority&percentage file");
                exit (1);
        }

        //fprintf (stderr, "\taggregating sensor data to MODIS resolution ...\n");
        for (i = 0; i < modis->nrows; i++)
                for (j = 0; j < modis->ncols; j++)
                {
                        count[i][j] = 0;
                        for (k = 0; k < sensor->init_ncls + 1; k++)
                                mcls[i][j][k] = 0;
                }

        /* minimum count must satisfy at least 80% of all pixels */
        min_count = (int) ((modis->res * modis->res) / (sensor->res * sensor->res) * 0.8);

        if (Sensor2MODIS (sensor, sensor->nrows/2, sensor->ncols/2, modis, &mi, &mj) == SUCCESS)
        {
                printf("CENTER: MODIS %d %d\n", mi, mj);
        }

        /* Assign the sensor-scale class# into a MODIS resolution bin -Yanmin 2009/10 */
        for (i = 0; i < sensor->nrows; i++)
        {
                //fprintf (stderr, "%3d\b\b\b", (int) (i + 1) * 100 / sensor->nrows);
                if (loadSensorClsRow (sensor, i) == FAILURE)
                {
                        fprintf (stderr, "searchSamples: load data from sensor error");
                        return FAILURE;
                }
                for (j = 0; j < sensor->ncols; j++)
                {
                        icls = sensor->Clsdata[j];
                        if ((icls > sensor->actu_ncls + 1) || ((icls < 0) && (icls != sensor->Cls_fillValue)))
                        {
                                fprintf (stderr, "\nClass#%d is out of scope at (row:%d,col:%d)!", icls, i, j);
                                return FAILURE;
                        }
                        if (icls != sensor->Cls_fillValue)
                                //if (Sensor2MODIS (sensor, i, j, modis, &mi, &mj) == SUCCESS)
                                //force
                                assert(Sensor2MODIS (sensor, i, j, modis, &mi, &mj) == SUCCESS);
                                {
                                        count[mi][mj]++;
                                        mcls[mi][mj][icls]++;
                                }
                }                                               /* end of j (row) */
        }                                                       /* end of i (row) */

        /*initialize the histogram array */
        for (icls = 0; icls < sensor->actu_ncls + 1; icls++)
        {
                sensor->pure_thrsh[icls] = -1;
                for (i = 0; i <= 100 / HIS_BIN + 1; i++)
                        his_ClsPix[icls][i] = 0;
        }

        /* compute the percentage of each majority class */
        for (i = 0; i < modis->nrows; i++)
                for (j = 0; j < modis->ncols; j++)
                {
                        /* find the  majority # falling in the target MODIS pixel */
                        max_percent = -1;
                        maj_cls = sensor->Cls_fillValue;
                        for (k = 0; k < sensor->actu_ncls + 1; k++)
                                if (mcls[i][j][k] > max_percent)
                                {
                                        max_percent = mcls[i][j][k];
                                        maj_cls = k;
                                }

                        if ((max_percent != -1) && (count[i][j] >= min_count)
                                && (maj_cls != sensor->Cls_fillValue) /*&&(maj_cls<sensor->actu_ncls+1) */ )
                        {
                                /*make sure enough pixels >80% fallend in the MODIS bin */
                                modis->major_cls[i][j] = (int8) maj_cls;
                                modis->percent_cls[i][j] = (uint8) (max_percent * 100.0 / count[i][j] + 0.5);
                                HisBininPos = modis->percent_cls[i][j] / HIS_BIN;
                                his_ClsPix[maj_cls][HisBininPos]++;
                                his_ClsPix[maj_cls][21]++;      /*get the total MODIS pixel# for each majority class */
                        }
                        else
                        {
                                modis->major_cls[i][j] = sensor->Cls_fillValue;
                        }
                }                                               /*enf for i; for j */

        /* Yanmin 2012/06 find the thresholds (>=60% majority-pct) in the histogram to screen pure pixels for each class */
        for (icls = 0; icls < sensor->actu_ncls + 1; icls++)
        {
                tmphis = 0;
                for (j = 100 / HIS_BIN; j >= PUREPIX_thredhold; j--)
                {
                        tmphis = tmphis + his_ClsPix[icls][j];

                        /* Yanmin 2012/06 enough nominal pure pixels are found. */
                        if (tmphis > his_ClsPix[icls][21] * TOP_Pct * 0.01)
                        {
                                sensor->pure_thrsh[icls] = j * HIS_BIN;
                                break;
                        }
                }
                /* Yanmin 2012/06 fill the threshold if not enough nominal pure pixels,
                   but still have available pixels (with >=PUREPIX_thredhold*HIS_BIN majority percentage) are  found.  */
                if ((j < PUREPIX_thredhold) && (tmphis > 0))
                        sensor->pure_thrsh[icls] = PUREPIX_thredhold * HIS_BIN;

                /* Yanmin 2012/06 fill-value if not nominal pure pixels (over PUREPIX_thredhold/20*100) are found.  */
                if ((j < PUREPIX_thredhold) && (tmphis = 0))
                        sensor->pure_thrsh[icls] = -1;

        }

        /*write out MODIS scale majority classes & the percentage in BIP model */
        for (i = 0; i < modis->nrows; i++)
                for (j = 0; j < modis->ncols; j++)
                {
                        fwrite (&(modis->major_cls[i][j]), sizeof (int8), 1, modis->agg_fp);
                        fwrite (&(modis->percent_cls[i][j]), sizeof (uint8), 1, modis->agg_fp);
                }

        fprintf (fp_his, "\nCls#, PurePix_threshold,0-4, 5-9,10-14,...95-100, total_Pix#");
        for (i = 0; i < sensor->actu_ncls + 1; i++)
        {
                fprintf (fp_his, "\n%d, %d,", i, sensor->pure_thrsh[i]);
                /* if (pure_thrsh[i]<1)
                   printf("\nFailed to get pure-BRDF for class:%d!!",i); */
                for (j = 0; j <= 100 / HIS_BIN + 1; j++)
                        fprintf (fp_his, "%ld,", his_ClsPix[i][j]);
        }

        fclose (modis->agg_fp);
        fclose (fp_his);

        /*### Extract MODIS BRDF and QA ### */
        //fprintf (stderr, "\n\tloading and extracting high quality MODIS albedo\n");
        /*initialize variables ration_AN and AvaPix[icls][band][0=wsa & 1=bsa] */
        for (icls = 0; icls < sensor->init_ncls + 1; icls++)
                for (band = 0; band < MODIS_NBANDS; band++)
                {
                        AvaPix_cnt[icls][band] = 0;
                        sensor->BRDF_paras[icls][band][0] = 0.0;
                        sensor->BRDF_paras[icls][band][1] = 0.0;
                        sensor->BRDF_paras[icls][band][2] = 0.0;
                }

        for (i = 0; i < modis->nrows; i++)
                for (j = 0; j < modis->ncols; j++)
                {
                        /*initialize the ANratio array for writing out a ratiomap */
                        for (k = 0; k < 18; k++)
                                tmpRatio[k] = 32767;    /*32767 is the fillvalue */

                        icls = modis->major_cls[i][j];

                        /* Yanmin 2012/06 add the constrain of  pure_thrsh[icls]!=-1 */
                        if ((icls != sensor->Cls_fillValue)     /*&&(icls>0)&&(icls<=sensor->actu_ncls) */
                                && (sensor->pure_thrsh[icls] != -1) && (modis->percent_cls[i][j] >= sensor->pure_thrsh[icls]))
                        {
                                loadModisPixel (modis, i, j);   /*read the BRDF & QA for the related "pure" modis pixel */

                                /* check Band dependent state QA */
                                if (modis->qa_id != FAILURE)
                                {
                                        dec2bin (modis->Pix_qa, bitp, 32);
                                        /*00-03  Band 1 Quality
                                           0 = best quality, full inversion (WoDs, RMSE majority good)
                                           1 = good quality, full inversion                           
                                           2 = Magnitude inversion (numobs >=7)               
                                           3 = Magnitude inversion (numobs >=3&<7)                  
                                           4 = Fill value */

                                        for (band = 0; band < MODIS_NBANDS; band++)
                                        {
                                                tmp = 0;
                                                for (k = 0 + band * 4; k < 4 + band * 4; k++){
                                                        tmp += bitp[k];
                                                }

                                                /*Yanmin 2012/06 relax the quality constrain for BRDF, only depend on MODIS QA, to ingest more BRDFs for each class */
                                                /*if (tmp<1) */

                                                /* sqs 12/6/2013 */
                                                /* sqs 9/25/14: accept qa=0 or 1 */
                                                tmp = modis->Pix_qa >> (4*band);
                                                tmp = tmp & 15; /* 15=1111 */

                                                        /*if (tmp<1 && modis->Pix_brdf[band][0] <= 1000 && modis->Pix_brdf[band][1] <= 1000 && */
                                                        if (tmp<=1 && modis->Pix_brdf[band][0] <= 1000 && modis->Pix_brdf[band][1] <= 1000 && 
                                                           modis->Pix_brdf[band][2] <= 1000 && modis->Pix_brdf[band][0] >= 0 && 
                                                           modis->Pix_brdf[band][1] >= 0 && modis->Pix_brdf[band][2] >= 0) 
                                                        /*if ((tmp<1)&& (!((modis->Pix_brdf[band][1]==0)&&(modis->Pix_brdf[band][2]==0)))) */
                                                {
                                                        AvaPix_cnt[icls][band]++;
                                                        sensor->BRDF_paras[icls][band][0] += modis->Pix_brdf[band][0];
                                                        if (modis->Pix_brdf[band][0] > 1000)
                                                        {
                                                                printf ("XXXXXXXXXXX ISO=%d, i=%d, j=%d, band=%d, bits=%d,%d,%d,%d, tmp=%d,qa=%d, bits=",
                                                                        modis->Pix_brdf[band][0], i, j, band, bitp[k-4], bitp[k-3], bitp[k-2], bitp[k-1], tmp, modis->Pix_qa);
                                                                int aaa;
                                                                for(aaa=0; aaa<32; aaa++){
                                                                        printf("%d,", bitp[aaa]);
                                                                }
                                                                printf("\n");
                                                        }
                                                        sensor->BRDF_paras[icls][band][1] += modis->Pix_brdf[band][1];
                                                        sensor->BRDF_paras[icls][band][2] += modis->Pix_brdf[band][2];
                                                }
                                        }                       /*end for (band=0... */
                                }                               /* if (modis->qa_id!=FAILURE... */
                        }                                       /*end  if (pure_thrsh[icls]!=-1) && (modis->major_cls[i][j]==icls) */
                }                                               /* end for (i, j) to get the mean of each ANratio */

        /*calculate the mean values */
        for (icls = 0; icls < sensor->actu_ncls + 1; icls++)
                for (band = 0; band < MODIS_NBANDS; band++)
                        if (AvaPix_cnt[icls][band] != 0)
                        {
                                sensor->BRDF_paras[icls][band][0] = sensor->BRDF_paras[icls][band][0] / (float) AvaPix_cnt[icls][band];
                                sensor->BRDF_paras[icls][band][1] = sensor->BRDF_paras[icls][band][1] / (float) AvaPix_cnt[icls][band];
                                sensor->BRDF_paras[icls][band][2] = sensor->BRDF_paras[icls][band][2] / (float) AvaPix_cnt[icls][band];
                        }
        /*for debug to print out the BRDF_paras */
        /* for (icls=0;icls<sensor->actu_ncls+1; icls++){
           printf("\nicls#%d,",icls);
           for(band=0;band<MODIS_NBANDS; band++)
           printf(" %f,%f,%f, ",BRDF_paras[icls][band][0],BRDF_paras[icls][band][1],BRDF_paras[icls][band][2]);
           }
         */
        for (band = 0; band < MODIS_NBANDS; band++)
        {
                for (icls = 0; icls < sensor->actu_ncls; icls++)
                {
                        //printf("band=%d/%d, icls=%d/%d\n", band, MODIS_NBANDS, icls, sensor->actu_ncls);
                        printf("MODIS BAND=%d, CLS=%d, CNT=%d, BRDF=%d, %d, %d\n", 
                                                        band, icls, AvaPix_cnt[icls][band], 
                                                        (int)(sensor->BRDF_paras[icls][band][0]+0.5),
                                                        (int)(sensor->BRDF_paras[icls][band][1]+0.5), 
                                                        (int)(sensor->BRDF_paras[icls][band][2]+0.5));
                }
                printf("\n");
        }

        /*release memory used in BRDF average */
        free_3dim_contig ((void ***) mcls);
        free_2dim_contig ((void **) count);

        free_2dim_contig ((void **) AvaPix_cnt);
        /*  free_2dim_contig((void **)his_ClsPix); */

        free_2dim_contig ((void **) modis->major_cls);  /*release earlier */
        free_2dim_contig ((void **) modis->percent_cls);

        return SUCCESS;
}

/******************************************************************************/
int PrepareCalculation (DIS_LANDSAT * sensor)
{
        int i, j, k;
        int8 Pix_QA;

        int16 lndPixAlbedo[SENSOR_BANDS * 2 + 2 * 2], tmp;
        double tmplndPixAlbedo[SENSOR_BANDS * 2 + 2 * 2];       /*store spectral BSA & WSA for 6 Landsat bands except the thermal band,
                                                                                                                   SW band BSA & WSA from TM coefficents.  */
        int fw_num;
        int CloudFlag, SnowFlag;

        /* calculate Landsat albedo */
        //fprintf (stderr, "\tCalculating landsat 30m albedo from lndSr  ... ");

        /* if(0 != convert_to_binary(&(sensor->scene))){ */
        /*         return FAILURE; */
        /* } */

        for (i = 0; i < sensor->nrows; i++)
        {
                /* fprintf(stdout, "%3d\b\b\b", (int)(i+1)*100/sensor->nrows); */
                if (loadSensorClsRow (sensor, i) == FAILURE)
                {
                        fprintf (stderr, "searchSamples: load data from sensor error");
                        return FAILURE;
                }

                if (load_lndSRrow (sensor, i) == FAILURE)
                {
                        fprintf (stderr, "searchSamples: load data from sensor error");
                        return FAILURE;
                }

                for (j = 0; j < sensor->ncols; j++)
                {
                        //if ((i==5524)&&(j==203))
                        //      printf("Debug!");
                        
                        //sensor->pix_VZA= CalPixVZA (sensor, i, j);
                        //sensor->pix_VAA= CalPixAzimuth(sensor,i, j);
                        sensor->pix_VZA= sensor->scene.vzn;
                        sensor->pix_VAA= sensor->scene.van;
                        //printf("solar: %f, %f, view: %f, %f\n", sensor->SZA, sensor->SAA, sensor->pix_VZA, sensor->pix_VAA);
                        
                        /******************************************************/
                        /*if  (sensor->OneRowlndSR[0][j] == sensor->SR_fillValue){
                                double tmp_fill=-32767;
                                 fwrite (&(tmp_fill), sizeof (double), 1, sensor->fpVAA);
                                 fwrite (&(tmp_fill), sizeof (double), 1, sensor->fpVAA);
                        }
                        if (sensor->OneRowlndSR[0][j] != sensor->SR_fillValue){
                                fw_num = fwrite (&(sensor->pix_VZA), sizeof (double), 1, sensor->fpVAA);
                                if (fw_num != 1)
                                {
                                        fprintf (stderr, "\n Failed to write out VZA at row=%d,col=%d.", i,j);
                                        return FAILURE;
                                }
                                
                                fw_num = fwrite (&(sensor->pix_VAA), sizeof (double), 1, sensor->fpVAA);
                                if (fw_num != 1)
                                {
                                        fprintf (stderr, "\n Failed to write out VAA at row=%d,col=%d.", i,j);
                                        return FAILURE;
                                }
                        }*/
                        /*******************************************************/
                        /*initilize result to fillvalues */
                        for (k = 0; k < 6; k++)
                        {
                                sensor->OneRowlndSpectralAlbedo[k][2 * j] = lndAlbedoFILLV;
                                sensor->OneRowlndSpectralAlbedo[k][2 * j + 1] = lndAlbedoFILLV;
                        }

                        sensor->OneRowlndSWalbedo[2 * j] = lndAlbedoFILLV;
                        sensor->OneRowlndSWalbedo[2 * j + 1] = lndAlbedoFILLV;
                        sensor->OneRowlndQA[j] = -1;

                        /*if ((sensor->OneRowlndSR[0][j] == sensor->SR_fillValue)
                                || (sensor->OneRowlndSR[6][j] == sensor->SR_fillValue))
                                continue;*/

#ifdef DEBUG
        if (i == DEBUG_irow && j == DEBUG_icol)
        {
          printf("qa band file name = %s\n", sensor->scene.band[sensor->scene.qa_band_index].file_name);
          printf("i=%d, j=%d, unsup class value = %d\n", i, j, sensor->OneRowlndSRQAData[j]);
          int zl;
          for (zl=0; zl<N_SRF_BAND; zl++){
            printf("sr band file name = %s\n", sensor->scene.band[sensor->scene.srf_band_index[zl]].file_name);
            printf("sr, band %d, i=%d, j=%d, val=%d\n", zl, i, j, sensor->OneRowlndSR[zl][j]);
          }
          
        }
        else{
                continue;
        }
#endif
                        /*snow and cloud determination to get clear snow-free pixel */
                        CloudFlag = CloudMask (sensor, j);      /*cloud, cloud shadow, internal cloud,or invalid data */
                        // SnowFlag = SnowDetermination (sensor, j);
                        SnowFlag = CloudMask(sensor, j);

#ifdef DEBUG
        if (i == DEBUG_irow && j == DEBUG_icol)
        {
                printf("CLOUD FLAG=%d, SNOW FLAG = %d\n", CloudFlag, SnowFlag);
        }
        else{
                continue;
        }
#endif


                        /* sqs 12/2/14, allow snow */
                        /* CloudFlag == 2 : snow;  CloudFlag == 1: cloud */
                        /* some times could be CloudFlag == 1 and SnowFlag == 1, in this case trust the snowflag */
                        /*if ((CloudFlag != 0) || (SnowFlag == 1))*/
                        //if ((CloudFlag == 1 && SnowFlag == 0))
                        if ((CloudFlag == 1))
                        {
                                sensor->OneRowlndQA[j] = 10;
                                sensor->OneRowlndSWalbedo[2 * j] = lndAlbedoFILLV;      /*shortwave Black-sky-albedo */
                                sensor->OneRowlndSWalbedo[2 * j + 1] = lndAlbedoFILLV;  /*shortwave White-sky-albedo */
                                continue;
                        }
                        if ((SnowFlag == 1) || (CloudFlag == 2))
                                sensor->OneRowlndQA[j] = 11;

                        if ((CalculateAlbedo (sensor, tmplndPixAlbedo, i, j, &Pix_QA, SnowFlag)) == SUCCESS)
                        {
                                for (k = 0; k < 18; k++)
                                {
                                        lndPixAlbedo[k] = (int16) ((10000.0 * tmplndPixAlbedo[k] + 0.5) / 1.0);
                                }
                                for (k = 0; k < 6; k++)
                                {
                                        sensor->OneRowlndSpectralAlbedo[k][2 * j] = lndPixAlbedo[k * 2];
                                        sensor->OneRowlndSpectralAlbedo[k][2 * j + 1] = lndPixAlbedo[k * 2 + 1];
                                }
                        }
                        else
                        {                                       /*fillvalue */
                                for (k = 0; k < 18; k++)
                                        lndPixAlbedo[k] = lndAlbedoFILLV;
                                Pix_QA = -1;
                                for (k = 0; k < 6; k++)
                                {
                                        sensor->OneRowlndSpectralAlbedo[k][2 * j] = lndAlbedoFILLV;
                                        sensor->OneRowlndSpectralAlbedo[k][2 * j + 1] = lndAlbedoFILLV;
                                }
                        }

                        sensor->OneRowlndSWalbedo[2 * j] = lndPixAlbedo[12];    /*shortwave Black-sky-albedo */
                        sensor->OneRowlndSWalbedo[2 * j + 1] = lndPixAlbedo[13];        /*shortwave White-sky-albedo */
                        sensor->OneRowlndQA[j] = Pix_QA;

                }

                /*write out one row SW-albedo (BSA & WSA) result */
                fw_num = fwrite (sensor->OneRowlndSWalbedo, sizeof (int16), sensor->ncols * 2, sensor->fplndalbedo);
                if (fw_num != sensor->ncols * 2)
                {
                        fprintf (stderr, "\n Failed to write out lndAlbedo for row:%d.", i);
                        return FAILURE;
                }

                /* write out one row QA result */
                fw_num = fwrite (sensor->OneRowlndQA, sizeof (int8), sensor->ncols, sensor->fplndQA);
                if (fw_num != sensor->ncols)
                {
                        fprintf (stderr, "\n Failed to write out lndQA for row:%d.", i);
                        return FAILURE;
                }

                /* write out one row spectral albedo */
#ifdef OUT_SpectralAlbedo
                for (j = 0; j < sensor->ncols; j++)
                        for (k = 0; k < 6; k++)
                        {
                                tmp = sensor->OneRowlndSpectralAlbedo[k][j * 2];
                                fw_num = fwrite (&tmp, sizeof (int16), 1, sensor->fpSpectralAlbedo);
                                if (fw_num != 1)
                                {
                                        fprintf (stderr, "\n Failed to write out lndSpectralAlbedo for row:%d,col:%d.", i, j);
                                        return FAILURE;
                                }
                                tmp = sensor->OneRowlndSpectralAlbedo[k][j * 2 + 1];
                                fw_num = fwrite (&tmp, sizeof (int16), 1, sensor->fpSpectralAlbedo);
                                if (fw_num != 1)
                                {
                                        fprintf (stderr, "\n Failed to write out lndSpectralAlbedo for row:%d,col:%d.", i, j);
                                        return FAILURE;
                                }
                        }
#endif

        }                                                       /*end for (i=0... */

        /*free memory and close the file handlers */
        /* fclose(fp_ratioMap); */
        fclose (sensor->fplndalbedo);
        fclose (sensor->fplndQA);
#ifdef OUT_SpectralAlbedo
        fclose (sensor->fpSpectralAlbedo);
#endif

        return SUCCESS;
}
