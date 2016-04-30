#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "UnsupCls.h"
#include "sqs.h"

/*******************************************************************************
!Revision History:
Revision 1.0.0  11/2009  Yanmin Shuai

Original version - This software was developed for NASA under contract #????.

!References and Credits
Principal Investigators: Alan H. Strahler, Crystal Schaaf,
                                  Boston University
                                  alan@bu.edu, schaaf@bu.edu
                         
Developers: Yanmin Shuai
            and numerous other contributors (thanks to all)

!Design Notes

**************************************************************************/
int GetLine (FILE * fp, char *s)
{
        int i, c;

        i = 0;
        while ((c = fgetc (fp)) != EOF)
        {
                s[i] = c;
                i++;
                if (c == '\n')
                        break;
                if (i > LARGE_ALLOC)
                        break;
        }
        if (i > 0)
                s[i - 1] = '\0';
        return i;
}

/************************************************************************/

int SubString (char *str_ptr, unsigned int str_start, unsigned int str_end, char *p)
/*
!C***********************************************************************

!Description:
  SubString() extract a substring from a given string.

!Input Parameters:
char *str_ptr    source string to be extracted a substring.  
unsigned int str_start   start position of substring in source string.
unsigned int str_end    end position of substring in source string.

!Output Parameters:
 char *p  substring extracted from a given string.

!Returns:
  none

!END**************************************************************************
*/
{
        unsigned int i;
        int j;

        if ((strlen (str_ptr) < str_start) || (strlen (str_ptr) < str_end))
                return UNSCLS_FAIL;
        j = 0;
        for (i = str_start; i <= str_end; i++)
                p[j++] = str_ptr[i];

        p[j - 1] = '\0';

        return UNSCLS_SUCCESS;

}

/******************************************************************************/
int StringTrim (char *in, char *out)
/*
!C****************************************************************************

!Description:
  StringTrim() strips whitespace  from the beginning and end of a string.

!Input Parameters:
char *in      string will be stripped  whitespace

!Output Parameters:
 char *out    string without whitespace at the begining and the end. 

!Returns:
  none

!END**************************************************************************
*/
{
        int i;

        i = 0;
        while (in[i] == ' ')
                i++;
        in = in + i;
        i = strlen (in);
        while (in[i - 1] == ' ')
                i--;
        in[i] = '\0';
        strcpy (out, in);

        return UNSCLS_SUCCESS;
}

/******************************************************************************/
int GetParam (FILE * fp, PARM_T * p)
/*
!C****************************************************************************

!Description:
 Extract paramter settings from *fp and transport values to *p.

!Input Parameters:
FILE *fp      file pointer to be extracted parameters
struct PARM_T *P   A pointer to a para-structure containing pointers to accept
                               all input/out file name and parameters.

!Output Parameters:
struct PARM_T *P   take all parameter settings in FILE *fp out.  

!Returns:
  UNSCLS_SUCCESS    Normally return when function reads 'END' or 'EOF';
  UNSCLS_FAIL       when Missing 'END'  in the file read by function.
 
!END*************************************************************************
*/
{
        int len, ret;
        char key[MODERATE_ALLOC], tmpKey[MODERATE_ALLOC];
        char line[4 * MODERATE_ALLOC];
        int tmp;

        len = GetLine (fp, line);
        while (len > 0)
        {
                if (strcmp (line, "END.") == 0)
                {
                        break;
                }
                else
                {
                        if (sscanf (line, "%[^=]", tmpKey) == 0)
                        {
                                fprintf (stderr, "\n Wrong format in the input parameter file\n");
                                return UNSCLS_FAIL;
                        }
                        ret = StringTrim (tmpKey, key);
                        if (strcmp (key, "FILE_NAME") == 0)
                        {
                                sscanf (line, "%*[^=]=%s", p->filename);
                        }
                        else if (strcmp (key, "OUT_CLSMAP") == 0)
                                sscanf (line, "%*[^=]=%s", p->Clsmapname);
                        else if (strcmp (key, "DISTANCE_FILE") == 0)
                                sscanf (line, "%*[^=]=%s", p->Distfilename);
                        else if (strcmp (key, "NBANDS") == 0)
                                sscanf (line, "%*[^=]=%d", &(p->nbands));
                        /*  else if (strcmp(key,"NCOLS")==0)
                           sscanf(line, "%*[^=]=%d",&(p->nsamps));        
                           else if (strcmp(key,"NROWS")==0)
                           sscanf(line, "%*[^=]=%d",&(p->nlines)); */
                        else if (strcmp (key, "NCLASSES") == 0)
                                sscanf (line, "%*[^=]=%d", &(p->nclasses));
                        else if (strcmp (key, "THRS_RAD") == 0)
                        {
                                sscanf (line, "%*[^=]=%lf", &p->radius);
                                p->radius2=p->radius*p->radius;
                                // printf("radius=%f, %f\n", p->radius, p->radius2);
                        }
                        else if (strcmp (key, "I_CLUSTER") == 0)
                                sscanf (line, "%*[^=]=%d", &(p->nclusters));
                        /*  else if (strcmp(key,"EXCL_VAL")==0)
                           sscanf(line, "%*[^=]=%d",&(p->exclude)); */
                        /*can add other key word here */

                        len = GetLine (fp, line);
                }                                               /*end if ((strcmp(line,"END")==0) */
        }                                                       /*end for while(len>0) */


        return UNSCLS_SUCCESS;

}                                                               /*end for GetParam() function. */

int OpenLndsrFile (PARM_T * parm)
{
        int i;

        if(parm->nbands != N_SRF_BAND){
                printf("Warning: change band number to %d\n", N_SRF_BAND);
                parm->nbands = N_SRF_BAND;
        }

        if (0 != parse_sentinel_xml (parm->filename, &(parm->scene)))
        {
                printf ("Parse xml failed. XML=%s\n", parm->filename);
                return UNSCLS_FAIL;
        }

        // convert the jp2 image files to binary first
        if(0 != convert_to_binary(&(parm->scene))){
          printf("OpenLndsrFile: convert image to binary file failed.\n");
          return FAILURE;
        }

        int index;

        // open srf
        for (i = 0; i < N_SRF_BAND; i++)
        {
                index = parm->scene.srf_band_index[i];

                if (0 != open_a_band (&(parm->scene.band[index])))
                {
                        printf ("Open band %d failed.\n", i);
                        return UNSCLS_FAIL;
                }
        }

        // open qa
        index = parm->scene.qa_band_index;
        if(index < 0 || 0 != open_a_band(&(parm->scene.band[index]))){
          printf("Open band qa failed.\n");
                return UNSCLS_FAIL;
        } 

        index = parm->scene.srf_band_index[0];
        parm->nlines = parm->scene.band[index].nrow;
        parm->nsamps = parm->scene.band[index].ncol;
        parm->exclude = (int16) parm->scene.band[index].fill_value;
        parm->scalefactor = parm->scene.band[index].scale_factor;

        long sys, zone, datum;
        double pparm[15];
        double ulx, uly, res;
        if(0 != get_scene_proj(&(parm->scene), &sys, &zone, &datum, pparm, &ulx, &uly, &res)){
                printf("Get proj info failed.\n");
                return UNSCLS_FAIL;
        }

        parm->ulx = ulx;
        parm->uly = uly;
        parm->zone = zone;
        parm->res = res;

        printf("nrow=%d, ncol=%d\n",  parm->nlines, parm->nsamps);
        printf("utm zone=%d, ulx=%lf, uly=%lf\n", zone, ulx, uly);
        printf("exclude=%d, scale=%f\n", parm->exclude, parm->scalefactor);
        printf("Files opened.\n");
        return UNSCLS_SUCCESS;
}

/*************************************************************************/
int OpenLndsrFile_old (PARM_T * parm)
 /*
    !C***********************************************************************

    !Description:
    OpenLndsrFile open the LandSat surface reflectance (HDF format generated from LEDAPS),
    and read 7 bands'surface reflectance.

    !Input/Output Parameters:
    PARM_T *p  provides the lndsr file name by p.filename[];
    return all the reflectance values by p.image[][][] 

    !Returns:
    UNSCLS_FAIL     when unexpected error happens
    UNSCLS_SUCCESS  normally exit
    !END**********************************************************************
  */
{
        int32 sds_index = 0, sds_id = 0, att_id;        /*index of a dataset, and Data set identifier */
        int32 rank, dimisizes2[2] = { 0 }, SDdata_type, n_attrs;
        int ret, status, i, j;
        int16 tmp;
        char name[MODERATE_ALLOC];
        char sdsName[7][10] = { "band1", "band2", "band3", "band4", "band5", "band7", "band6" };
        double scalefct;

        parm->fp_id = SDstart (parm->filename, DFACC_READ);
        if (parm->fp_id == FAILURE)
        {
                fprintf (stderr, "\nOpenLndsrFile():fail to open the lndsr file.");
                printf ("\nLNDSR FILE:%s\n", parm->filename);
                return UNSCLS_FAIL;
        }

        /*read the grid metadata to get the dimension infor. */
        sds_index = SDnametoindex (parm->fp_id, sdsName[0]);
        if (sds_index == FAILURE)
        {
                fprintf (stderr, "\nOpenLndsrFile():fail to get the identifier for SDS:%s.", sdsName[0]);
                return UNSCLS_FAIL;
        }
        else
        {
                sds_id = SDselect (parm->fp_id, sds_index);
                if (sds_id == FAILURE)
                {
                        fprintf (stderr, "\nOpenLndsrFile():fail to select the SDS:%s.", sdsName[0]);
                        return UNSCLS_FAIL;
                }
                else
                {
                        status = SDgetinfo (sds_id, name, &rank, dimisizes2, &SDdata_type, &n_attrs);
                        if (status != FAILURE)
                        {
                                parm->nlines = dimisizes2[0];
                                parm->nsamps = dimisizes2[1];
                        }
                        else
                        {
                                fprintf (stderr, "\nCan't get the row/samples info. from SDgetinfo()");
                                return UNSCLS_FAIL;
                        }
                        /*Yanmin 2009/12 read the fillvalue from the metadata */
                        parm->exclude = -9999;
                        att_id = SDfindattr (sds_id, "_FillValue");
                        if (att_id != FAILURE)
                        {
                                status = SDreadattr (sds_id, att_id, &tmp);
                                if (status != FAILURE)
                                        parm->exclude = tmp;
                        }
                        /*Yanmin 2009/12 read the scale factor from metadata,otherwise set it to the LEDAPSE's current deafult 0.0001 */
                        parm->scalefactor = 0.0001;
                        att_id = SDfindattr (sds_id, "scale_factor");
                        if (att_id != FAILURE)
                        {
                                status = SDreadattr (sds_id, att_id, &scalefct);
                                if (status != FAILURE)
                                        parm->scalefactor = scalefct;
                        }

                }                                               /*end sds_id */
        }                                                       /*end sds_index */

        /*get the SDS identifiers */
        for (i = 0; i < parm->nbands; i++)
        {
                sds_index = SDnametoindex (parm->fp_id, sdsName[i]);
                if (sds_index == FAILURE)
                {
                        fprintf (stderr, "\nOpenLndsrFile():fail to acquire the identifier for SDS:%s.", sdsName[i]);
                        return UNSCLS_FAIL;
                }
                else
                {
                        sds_id = SDselect (parm->fp_id, sds_index);
                        if (sds_id == FAILURE)
                        {
                                fprintf (stderr, "\nOpenLndsrFile():fail to select the SDS:%s.", sdsName[i]);
                                return UNSCLS_FAIL;
                        }
                        else
                                parm->sr_id[i] = sds_id;
                }
        }                                                       /*end for */

        return UNSCLS_SUCCESS;

}

int ReadLndsr(PARM_T * parm)
{
        int i;
        int index;
        int row, col;

        uint16 *OneRowBuffer;
        alloc_1dim_contig ((void **) (&OneRowBuffer), parm->nsamps, sizeof (uint16));
        //int16 (*OneRowBuffer)[parm->nsamps] = (int16 (*)[parm->nsamps]) malloc(sizeof(int16)*parm->nsamps*parm->nlines);
        
        uint16 *OneRowBuffer_qa;
        alloc_1dim_contig ((void **) (&OneRowBuffer_qa), parm->nsamps, sizeof(uint16));
        //int16 (*OneRowBuffer_qa)[parm->nsamps] = (int16 (*)[parm->nsamps]) malloc(sizeof(int16)*parm->nsamps*parm->nlines);
                        
        /*printf("Reading qa %dx%d...\n", parm->nlines, parm->nsamps);
        index = parm->scene.cld_band_index;
        if(0 != read_a_band(&(parm->scene.band[index]), (void *)OneRowBuffer_qa)){
                printf("Read cfmask failed.\n");
                return FAILURE;
        }*/
        
        /* if(0 != convert_to_binary(&(parm->scene))){ */
        /*   printf("ReadLndsr: convert image to binary file failed.\n"); */
        /*   return FAILURE; */
        /* } */

        int index2 = parm->scene.cld_band_index;
        int index_qa = parm->scene.qa_band_index;

        for (i = 0; i < N_SRF_BAND; i++)
        {
                /*index = parm->scene.srf_band_index[i];
                printf("Reading band %d ...\n", index);
                if(0 != read_a_band(&(parm->scene.band[index]), (void *)OneRowBuffer)){
                        printf("Read failed.\n");
                        free (OneRowBuffer);
                        return FAILURE;
                }*/
                index = parm->scene.srf_band_index[i];
                printf("Reading %s ...\n", parm->scene.band[index].file_name);
                for(row=0; row<parm->nlines; row++){
                        if(0 != read_a_band_a_row(&(parm->scene.band[index]), row, (void *)OneRowBuffer)){
                                printf("Read failed.\n");
                                free (OneRowBuffer);
                                return FAILURE;
                        }
                
                        // scene classification
                        if(0 != read_a_band_a_row(&(parm->scene.band[index_qa]), row, (void *)OneRowBuffer_qa)){
                                printf("Read scene classification failed.\n");
                                return FAILURE;
                        }

                        // usgs cfmask
                        // 0: clear
                        // 1: water
                        // 2: cloud shadow
                        // 3: snow
                        // 4: cloud

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
                        
                        for(col=0; col<parm->nsamps; col++){
                                parm->image[row][col][i] = OneRowBuffer[col];
#ifdef SNOW
                                // allow snow temporarily
                                if(OneRowBuffer_qa[col] == 11){
                                        ;
                                }
                                else 
#endif
                                  if(OneRowBuffer_qa[col] < 4 || OneRowBuffer_qa[col] > 6){
                                    parm->image[row][col][i] = parm->exclude;
                                        /*printf("here, row=%d, col=%d, band=%d, cloud=%d, img=%d, exclude=%d, new=%d\n", 
                                                                                                        row, col, i, OneRowBuffer_qa[col], OneRowBuffer[col], parm->exclude,
                                                                                                        parm->image[row][col][i]);*/
                                }
                        }
                }
        }
        printf("Reading done.\n");

        free (OneRowBuffer);
        free (OneRowBuffer_qa);
        return UNSCLS_SUCCESS;
}

/*****************************************************************************/
int ReadLndsr_old (PARM_T * parm)
/*
!C****************************************************************************

!Description:
  ReadLndsrRow read the 7 bands LandSat surface reflectances
  (HDF format generated from LEDAPS),

!Input/Output Parameters:
   PARM_T *parm return all the reflectance values by parm->image[band][nsamps][nlines] 

!Returns:
   SUCCEED 1
   FAIL -1

!END**************************************************************************
*/
{
        int32 start[2], length[2];
        int iband, irow, icol;
        int flg = UNSCLS_SUCCESS;
        int16 *OneRowBuffer;

        alloc_1dim_contig ((void **) (&OneRowBuffer), parm->nsamps, sizeof (int16));

        /*read as a BSQ format */
        fprintf (stderr, "\nread Landsat data processing... ");
        for (iband = 0; iband < parm->nbands; iband++)
                for (irow = 0; irow < parm->nlines; irow++)
                {
                        if (!(irow % (parm->nlines / 700)))
                                fprintf (stderr, "%3d\b\b\b", irow / (parm->nlines / 700));
                        start[0] = irow;
                        start[1] = 0;
                        length[0] = 1;
                        length[1] = parm->nsamps;
                        if (SDreaddata (parm->sr_id[iband], start, NULL, length, OneRowBuffer) == FAILURE)
                        {
                                fprintf (stderr, "\nOpenLndsrFile():fail to read the %dth SDS.", iband);
                                flg = UNSCLS_FAIL;
                                break;
                        }
                        else
                        {
                                for (icol = 0; icol < parm->nsamps; icol++)
                                        parm->image[irow][icol][iband] = OneRowBuffer[icol];
                        }
                }

        free (OneRowBuffer);
        if (flg != UNSCLS_FAIL)
                return UNSCLS_SUCCESS;
        else
                return UNSCLS_FAIL;
}

/*****************************************************************************/

int ShortestDisCls (int row, int col, PARM_T * parm)
/*
!C***********************************************

!Description: Calculate the distances between the unkonwn pixel (row,col)
                     and clusters, return a class ID by the shortest distance rule.

!Input

!Returns:
          available class ID or -9.

*/
{
        double *dis, tmp_dis, mini_dis;
        int icluster, iband;
        int mini_icls, icls, i, j;

        alloc_1dim_contig ((void **) (&dis), parm->nclasses, sizeof (double));

        mini_dis = 1e10;
        tmp_dis = 1e10;

        for (icluster = 0; icluster < parm->nclasses; icluster++)
        {
                icls = parm->c_order[icluster];
                if (icls != -1)
                {
                        for (iband = 0; iband < parm->nbands; iband++)
                        {
                                tmp_dis = parm->image[row][col][iband] * parm->scalefactor - parm->mean_x[icls][iband];
                                dis[icluster] += tmp_dis * tmp_dis;
                        }
                        if (dis[icluster] < mini_dis)
                        {
                                mini_dis = dis[icluster];
                                mini_icls = icluster;
                        }
                }
        }

        free (dis);
        /*if (tmp_dis<=10*parm->radius2) */
        if (mini_icls != -1)
                return (mini_icls);
        else
                return (-9);

}

/**************************************************************************/
int MergeCls (PARM_T * parm)
{
        int available_icls;
        int icls, icls_a, icls_b;
        int i, iband, loc_a, loc_b;
        int ret;

        available_icls = 0;
        for (i = 0; i < parm->iclusters; i++)
        {
                icls = parm->c_order[i];
                if (icls != -1)
                        available_icls++;
        }

/*Print the mean_x values before Merge*/
/*  for(i=0;i<parm->iclusters;i++){
        icls=parm->c_order[i];
        fprintf(stderr, "\nmean for icluster=%d, icls#=%d",i,icls);
        if (icls!=-1)
          for(iband=0;iband<parm->nbands;iband++)
                fprintf(stderr, "  %f, ", parm->mean_x[icls][iband]);           
        }
*/
        while (available_icls > parm->nclasses)
        {
                ret = Find2ClsWithMiniDis (&loc_a, &loc_b, parm);
                /*merge cluster (loc_b) into cluster (loc_a) */
                if (ret == UNSCLS_SUCCESS)
                {
                        icls_a = parm->c_order[loc_a];
                        icls_b = parm->c_order[loc_b];
                        parm->c_order[loc_b] = -1;
                        parm->c_npixels[icls_a] += parm->c_npixels[icls_b];
                        for (iband = 0; iband < parm->nbands; iband++)
                        {
                                parm->sum_x[icls_a][iband] += parm->sum_x[icls_b][iband];
                                parm->mean_x[icls_a][iband] = parm->sum_x[icls_a][iband] / parm->c_npixels[icls_a];
                        }
                        available_icls--;
                }
                else
                {
                        fprintf (stderr, "\nfailed to find two clusters for merge!!");
                        return UNSCLS_FAIL;
                }
        }                                                       /*end while */

/*Print the mean_x values after Merge*/
        /*for(i=0;i<parm->iclusters;i++){
           icls=parm->c_order[i];
           fprintf(stderr, "\nmean for icluster=%d, icls#=%d",i,icls);
           if (icls!=-1)
           for(iband=0;iband<parm->nbands;iband++)
           fprintf(stderr, "  %f, ", parm->mean_x[icls][iband]);          
           }
         */
        return UNSCLS_SUCCESS;
}

/**************************************************************************/
int Find2ClsWithMiniDis (int *loc_a, int *loc_b, PARM_T * parm)
{
        int band;
        int loc1, loc2, cls1, cls2;
        double mini_dis, dis, tmp_diff;

        mini_dis = 100000.0;

        *loc_a = -1;
        *loc_b = -1;

        for (loc1 = 0; loc1 < parm->iclusters; loc1++)
                for (loc2 = loc1 + 1; loc2 < parm->iclusters; loc2++)
                {
                        cls1 = parm->c_order[loc1];
                        cls2 = parm->c_order[loc2];
                        if ((cls1 != -1) && (cls2 != -1))
                        {
                                dis = 0.0;
                                for (band = 0; band < parm->nbands; band++)
                                {
                                        tmp_diff = parm->mean_x[cls1][band] - parm->mean_x[cls2][band];
                                        dis += tmp_diff * tmp_diff;
                                }
                                if (dis < mini_dis)
                                {
                                        *loc_a = loc1;
                                        *loc_b = loc2;
                                        mini_dis = dis;
                                }
                        }                                       /*if (cls1!=-1) && */

                }                                               /*end for */

        if ((*loc_a != -1) && (*loc_b != -1))
                return UNSCLS_SUCCESS;
        else
                return UNSCLS_FAIL;
}

int CloseLndsr (PARM_T * parm)
{
        int i;
        int index;
        for (i = 0; i < N_SRF_BAND; i++)
        {
                index = parm->scene.srf_band_index[i];
                close_a_band(&(parm->scene.band[index]));
        }
        index = parm->scene.cld_band_index;
        close_a_band(&(parm->scene.band[index]));
        return UNSCLS_SUCCESS;
}

/**************************************************************************/

int CloseLndsr_old (PARM_T * parm)
/*
!C*************************************************************************

!Description:Close SDS dataset and hdf file.
             free memory.

!Input Parameters:
 PARM_T *parm provide the sds identifiers and the Lndsr file handler 

!Returns:
  SUCCEED 1
  FAIL -1
  
!END*********************************************************************** 
*/
{
        int iband;

        for (iband = 0; iband < parm->nbands; iband++)
                if ((SDendaccess (parm->sr_id[iband])) == FAILURE)
                {
                        fprintf (stderr, "\nCloseLndsr():SDendaccess for lndar band%d error!", iband);
                        return UNSCLS_FAIL;
                }
        if ((SDend (parm->fp_id)) == FAILURE)
        {
                fprintf (stderr, "\nCloseLndsr():SDend for file %s error!", parm->filename);
                return UNSCLS_FAIL;
        }

        return UNSCLS_SUCCESS;
}

void alloc_1dim_contig (void **ptr, int d1, int elsize)
{
        void *p = NULL;

        p = calloc (d1, elsize);
        if (!p)
        {
                fprintf (stderr, "\nMemory allocation error in alloc_1dim_contig");
                return;
        }
        *ptr = p;
        return;
}

void alloc_2dim_contig (void ***ptr, int d1, int d2, int elsize)
{
        void *p = NULL;
        void **pp = NULL;
        int i = 0;

        /* alloc array for data */
        alloc_1dim_contig ((void **) (&p), d1 * d2, elsize);

        /* alloc array for pointers */
        alloc_1dim_contig ((void **) (&pp), d1, sizeof (void *));

        /* Set the pointers to indicate the appropriate elements of the data array. */
        for (i = 0; i < d1; i++)
        {
                pp[i] = (char *) p + (i * d2 * elsize);
        }

        *ptr = pp;

        return;
}

void alloc_3dim_contig (void ****ptr, int d1, int d2, int d3, int elsize)
{
        void *p = NULL;
        void **pp = NULL;
        void ***ppp = NULL;
        int i = 0;

        /* allocate the data array */
        alloc_1dim_contig ((void **) (&p), d1 * d2 * d3, elsize);

        /* alloc the double pointers */
        alloc_1dim_contig ((void **) (&pp), d1 * d2, sizeof (void *));

        /* and again for the first dimensional pointers */
        alloc_1dim_contig ((void **) (&ppp), d1, sizeof (void **));

        /* first set the d1 pointers */
        for (i = 0; i < d1; i++)
        {
                ppp[i] = pp + (i * d2);
        }

        /* next set all of the d2 pointers */
        for (i = 0; i < d1 * d2; i++)
        {
                pp[i] = (char *) p + (i * d3 * elsize);
        }

        *ptr = ppp;

        return;
}

void alloc_4dim_contig (void *****ptr, int d1, int d2, int d3, int d4, int elsize)
{
        void *p = NULL;
        void **pp = NULL;
        void ***ppp = NULL;
        void ****pppp = NULL;
        int i = 0;

        /* allocate the data array */
        alloc_1dim_contig ((void **) (&p), d1 * d2 * d3 * d4, elsize);

        /* alloc the double pointers */
        alloc_1dim_contig ((void **) (&pp), d1 * d2 * d3, sizeof (void *));

        /* and again for the triple pointers */
        alloc_1dim_contig ((void **) (&ppp), d1 * d2, sizeof (void **));

        alloc_1dim_contig ((void **) (&pppp), d1, sizeof (void ***));

        for (i = 0; i < d1; i++)
        {
                pppp[i] = ppp + (i * d2);
        }

        for (i = 0; i < d1 * d2; i++)
        {
                ppp[i] = pp + (i * d3);
        }

        for (i = 0; i < d1 * d2 * d3; i++)
        {
                pp[i] = (char *) p + (i * d4 * elsize);
        }

        *ptr = pppp;

        return;
}

int AllocateEarlyVars (PARM_T * parm)
{
        /* alloc the memory  to variables */
        alloc_3dim_contig ((void ****) (&parm->image), parm->nlines, parm->nsamps, parm->nbands, sizeof (int16));

        alloc_1dim_contig ((void **) (&parm->c_npixels), parm->nclusters, sizeof (int));
        alloc_1dim_contig ((void **) (&parm->c_order), parm->nclusters, sizeof (int));
        alloc_2dim_contig ((void ***) (&parm->sum_x), parm->nclusters, parm->nbands, sizeof (long double));
        alloc_2dim_contig ((void ***) (&parm->mean_x), parm->nclusters, parm->nbands, sizeof (long double));
        alloc_3dim_contig ((void ****) (&parm->sum_xy), parm->nclusters, parm->nbands, parm->nbands, sizeof (long double));

        return UNSCLS_SUCCESS;
}

void free_2dim_contig (void **a)
{
        free (a[0]);
        free (a);
        return;
}

void free_3dim_contig (void ***a)
{
        free (a[0][0]);
        free (a[0]);
        free (a);
        return;
}

void free_4dim_contig (void ****a)
{
        free (a[0][0][0]);
        free (a[0][0]);
        free (a[0]);
        free (a);
        return;
}
