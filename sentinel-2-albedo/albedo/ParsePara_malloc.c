/* 
 * !Description
 *  Extract input parameters from input text file 
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
int parseParameters(char *ifile, DIS_LANDSAT *sensor, GRID_MODIS *modis)
{
  FILE *in;
  int i;

  char  buffer[MAX_STRLEN] = "\0";
  char  *label = NULL;
  char  *tokenptr = NULL;
  char  *seperator = "= ,",*p1=".MajCls",*p2=".Majhis",*p3="_albedo.bin", *p4="_QA.bin";
  char tmp[MAX_STRLEN], oldlndalbedoName[MAX_STRLEN];
  char *tmp_fname;
  
  if((in=fopen(ifile,"r"))==NULL) {
    fprintf(stderr, "\nCan't open input %s: parseParameters\n", ifile);
    return FAILURE;
  }

  /* process line by line */
  while(fgets(buffer, MAX_STRLEN, in) != NULL) {
 
    /* get string token */
    tokenptr = strtok(buffer, seperator);
    label=tokenptr;

    /* skip comment line */
    if(strcmp(label,"#") == 0) continue;

    while(tokenptr != NULL) {
  
      tokenptr = strtok(NULL, seperator);

      /* get file names */
      if(strcasecmp(label, "DM_CLSMAP")==0)
      	sscanf(tokenptr, "%s ", sensor->clsmap);
      else if(strcasecmp(label, "DM_FILE_NAME") == 0)
	sscanf(tokenptr, "%s ", sensor->fileName);
      else if(strcasecmp(label, "AMONGclsDIST_FILE_NAME") == 0)
	sscanf(tokenptr, "%s ", sensor->clsDist); /* Yanmin 2012/06 add the among-class-distance */
      else if(strcasecmp(label, "OUTPUT_ALBEDO") == 0)
				{
	sscanf(tokenptr, "%s ", sensor->lndalbedoName);
        memset(oldlndalbedoName,0,sizeof(oldlndalbedoName));
        strncpy(oldlndalbedoName, sensor->lndalbedoName,strlen(sensor->lndalbedoName));
#ifdef SNOW
        sprintf(sensor->lndalbedoName, "%s_snow", sensor->lndalbedoName);
#endif
                                }
      else if(strcasecmp(label, "OUTPUT_HIS") == 0)
	sscanf(tokenptr, "%s ", modis->his_name);
      else if(strcasecmp(label, "RATIOMAP") == 0)
        sscanf(tokenptr, "%s ", modis->ratioMap);
      if(strcasecmp(label, "DM_NBANDS") == 0) 
	sscanf(tokenptr, "%d", &(sensor->nbands)); 
      if(strcasecmp(label, "DM_NCLS") == 0) 
	sscanf(tokenptr, "%d", &(sensor->init_ncls)); 
      else if(strcasecmp(label, "DM_PIXEL_SIZE") == 0)
	sscanf(tokenptr, "%lf", &(sensor->res));
      else if(strcasecmp(label, "CLS_FILL_VALUE") == 0)
	sscanf(tokenptr, "%d", &(sensor->Cls_fillValue));
      else if(strcasecmp(label, "DM_PROJECTION_CODE") == 0)
      sscanf(tokenptr, "%d", &(sensor->insys));
      else if(strcasecmp(label, "DM_PROJECTION_PARAMETERS") == 0)
	for(i=0; i<15; i++) {
	  sscanf(tokenptr, "%lf ", &(sensor->inparm[i]));
	  tokenptr = strtok(NULL, seperator);
	}
      else if(strcasecmp(label, "DM_PROJECTION_ZONE") == 0)
	sscanf(tokenptr, "%d", &(sensor->inzone));
      else if(strcasecmp(label, "DM_PROJECTION_UNIT") == 0)
	sscanf(tokenptr, "%d", &(sensor->inunit));
      else if(strcasecmp(label, "DM_PROJECTION_DATUM") == 0)
	sscanf(tokenptr, "%d", &(sensor->indatum));
      
      else if(strcasecmp(label, "MODIS_BRDF_FILE") == 0)
	sscanf(tokenptr, "%s", modis->srName); 
      else if(strcasecmp(label, "MODIS_QA_FILE") == 0) 
	sscanf(tokenptr, "%s", modis->qaName); 
      else if(strcasecmp(label, "HISTOGRAM_FILE") == 0) 
	sscanf(tokenptr, "%s", modis->his_name); 
      else if(strcasecmp(label, "MIN_ACCEPTABLE_PERCENT") == 0) 
	sscanf(tokenptr, "%d", &(modis->min_percent)); 
      else if(strcasecmp(label, "MODIS_YEAR") == 0) 
	sscanf(tokenptr, "%d", &(modis->year)); 
      else if(strcasecmp(label, "MODIS_JDAY") == 0) 
	sscanf(tokenptr, "%d", &(modis->jday)); 
  
      /* in case label (key words) is not the first word in a line */
      label = tokenptr;

    }  /* while token */
  } /* while line */

 memset(tmp,0,sizeof(tmp));
  strncpy(tmp, sensor->clsmap,strlen(sensor->clsmap)-4);
  tmp_fname=strcat(tmp,p1);
  strcpy(modis->aggFileName,tmp_fname);
 
 memset(tmp,0,sizeof(tmp));
 strncpy(tmp, sensor->clsmap,strlen(sensor->clsmap)-4);
  tmp_fname=strcat(tmp,p2);
  strcpy(modis->his_name,tmp_fname);

  memset(tmp,0,sizeof(tmp));  
  strncpy(tmp,sensor->clsmap,strlen(sensor->clsmap)-4);
   tmp_fname=strcat(tmp,p3);
	/* sqs */
	/* strcpy(sensor->lndalbedoName,tmp_fname);  */

  memset(tmp,0,sizeof(tmp));
  //strncpy(tmp,sensor->clsmap,strlen(sensor->clsmap)-4);
  strncpy(tmp,oldlndalbedoName,strlen(oldlndalbedoName)-4);
  tmp_fname=strcat(tmp,p4);
  strcpy(sensor->lndQAName,tmp_fname);

  if (sensor->init_ncls>MAX_cls){
  	printf("\nWarning::initial unsupervised classes %d is greater than the MAX_cls:%d!!\nPlease reset the MAX_cls in the Lndsr_albedo.h\n",
  		sensor->init_ncls, MAX_cls);
  	return FAILURE;
  	}
  
  return SUCCESS;
}

int substr(char *str,char*s, int begin,int end)
{
  char *p;
  int i;
  
  i=strlen(str);
  if (begin<=0 || begin>end ||end>i){
     fprintf(stderr,"\n substr():failed to get the substring\n");
     return FAIL;
  }

  p=str+begin-1;
  memcpy(s,str,(end-begin+1));
  *(s+end-begin+1)='\0';
  return SUCCESS;
}

/**********************************************************************/
void alloc_1dim_contig (
		     void **ptr,
		     int d1,
		     int elsize)
{
   void *p = NULL;

   p = calloc (d1, elsize);
   if (!p) {
      fprintf (stderr, "Memory allocation error in alloc_1dim_contig");
     exit(1);
   }
   *ptr = p;

   return;
}

/*------------------------------------------------------------------------*/
void alloc_2dim_contig (
		     void ***ptr,
		     int d1,
		     int d2,
		     int elsize)
{
   void *p = NULL;
   void **pp = NULL;
   int i = 0;

   /* alloc array for data */
   alloc_1dim_contig ((void **) (&p), d1 * d2, elsize);

   /* alloc array for pointers */
   alloc_1dim_contig ((void **) (&pp), d1, sizeof (void *));

   /*
    * Set the pointers to indicate the appropriate elements of the data
    * array.
    */
   for (i = 0; i < d1; i++) {
      pp[i] = (char *) p + (i * d2 * elsize);
   }

   *ptr = pp;

   return;
}

/*------------------------------------------------------------------------*/
void alloc_3dim_contig (
		     void ****ptr,
		     int d1,
		     int d2,
		     int d3,
		     int elsize)
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
   for (i = 0; i < d1; i++) {
      ppp[i] = pp + (i * d2);
   }

   /* next set all of the d2 pointers */
   for (i = 0; i < d1 * d2; i++) {
      pp[i] = (char *) p + (i * d3 * elsize);
   }

   *ptr = ppp;

   return;
}

/*------------------------------------------------------------------------*/

void alloc_4dim_contig (
		     void *****ptr,
		     int d1, int d2,
		     int d3,
		     int d4,
		     int elsize)
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

   for (i = 0; i < d1; i++) {
      pppp[i] = ppp + (i * d2);
   }

   for (i = 0; i < d1 * d2; i++) {
      ppp[i] = pp + (i * d3);
   }

   for (i = 0; i < d1 * d2 * d3; i++) {
      pp[i] = (char *) p + (i * d4 * elsize);
   }

   *ptr = pppp;

   return;
}

/*------------------------------------------------------------------------*/

void free_1dim_contig (void *a)
{
   free (a);
   return;
}

/*------------------------------------------------------------------------*/

void free_2dim_contig (void **a)
{
   free (a[0]);
   free (a);
   return;
}

/*------------------------------------------------------------------------*/

void free_3dim_contig (void ***a)
{
   free (a[0][0]);
   free (a[0]);
   free (a);
   return;
}

/*------------------------------------------------------------------------*/
void free_4dim_contig (void ****a)
{
   free (a[0][0][0]);
   free (a[0][0]);
   free (a[0]);
   free (a);
   return;
}

/* convert decimal number to binary number */
/* sqs, 12/6/2013 */
/*void dec2bin(unsigned short int num, int *bitpattern, int limit)*/
void dec2bin(unsigned int num, int *bitpattern, int limit)
{
 
  register int i=0;       
 
  for(i=0; i<limit; i++)
    bitpattern[i]=0;
 
  for(i=0; num>0; i++)
    {
      bitpattern[i] = (int)num & 1;
      num >>= 1;
    }
}
