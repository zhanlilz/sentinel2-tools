/*
!C-INC************************************************************************

!Description:
Main header for the unsupervised classification processing. Contains the typedefs that
contain all the variables, especially "parm variables".
Also main place where other headers required get included.

!Input Parameters:
none

!Output Parameters:
none

!Returns:
none

!Revision History:
Original version-UnsupervisedCls v1.0 (Earth Resources Technology Inc.) 
		developed for the NASA project "North American Forest 
		Dynamics via Integration of ASTER, NODIS, and Landsat 
		Reflectance Data". In this package, the ustats routine 
		in IPW 2.0(contributed by Boston University)was referred
		to perform the multivariate statistics and acquire the 
		mean of each cluster.  

!Team-unique Header:
This software is developed for NASA project under contract #
	Principal Investigators: Jeffrey Masek (NASA) 
				 Feng Gao (ERT Inc., NASA)
                                   
Developers: Yanmin Shuai  (ERT Inc. and NASA, Yanmin.Shuai@ertcorp.com)
            and numerous other contributors (thanks to all)

!Design Notes

!END**************************************************************************
*/
/* INCLUDE HEADERS */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <strings.h>
#include <float.h> 	/* floats */
#include <time.h>	/* C time functions */

#include <limits.h>

#include <sys/types.h>

#include "sqs.h"

#define OK		1
#define ERROR		(-2)	/* from IPW version 1.0 */

/* HDF and HDF-EOS headers for MET and HDF reading : */
#include "hdf.h"
#include "mfhdf.h"
#include "HdfEosDef.h"

/* General macro definitions */
#define MAX_STRLEN 1000
#define LARGE_ALLOC  1000
#define MODERATE_ALLOC  100
/*#define MAX_DBValue 1e+20*/

#define SUCCESS 1
#define FAILURE -1

#define UNSCLS_FAIL 255
#define UNSCLS_SUCCESS 256
#define GRID_ERRCODE -1

/*# scale factor for Land Surface Reflectance*/
#define SR_SCALE 0.0001
#define TR_SCALE 0.001
#define CLS_PIX 0.000001

/*DEBUG pixel*/
/*#define DEBUG*/
#ifdef DEBUG
#define DEB_ROW 4096
#define DEB_COL 2048
#endif

/* Functions */
#ifndef MAX
#define MAX(X,Y) ((X)>(Y)?(X):(Y))
#endif
#ifndef MIN
#define MIN(X,Y) ((X)<(Y)?(X):(Y))
#endif

/* Data structure definition */
typedef struct {
    char	filename[4*MODERATE_ALLOC]; 	/*lndsr hdf format file name*/
    char  Clsmapname[4*MODERATE_ALLOC]; /*output class map name*/
    char Distfilename[4*MODERATE_ALLOC]; /*output the distance matix in the spectral space among final unsupervised classes*/
    int32	fp_id;	/* HDF scientific data handler	 */
    int32	sr_id[7];  /* sds handler for lndsr        */
    int	nlines;	/* #lines / image		 */
    int	nsamps;	/* #samples / line	 	 */
    int	nbands;	/* #bands / sample	 	 */
    int	*c_npixels;	/* #pixels[cluster]  */
    int	*c_order;	/* cluster ordering, most populous first */
    int	nclasses;	/* # final classes   */
    int	nclusters;	/* max # intermediate clusters   */
    int	iclusters;	/* current # intermediate clusters */
    int	exclude;	/* pixel exclusion value */
  double scalefactor; /*scale fatcor for lndSR data*/
    int	exclusion;	/* use exclusion value */
    double radius;	/* cluster threshold radius */
    double radius2;/* cluster threshold radius squared */
    int16 ***image; /* [nlines][nsamps][band]input image to be clustered */
    double **sum_x;	/* sum[cluster][band]		 */
    double **mean_x;	/* mean[cluster][band]		 */
    double ***sum_xy;	/* sum[cluster][band][band]	 */
   
	  usgs_t scene;	
		long zone;
		double ulx;
		double uly;
} PARM_T;

/*Global variables used in the ustats() */
static int cfreqndx;
static int cclndx;
static int lines;
static int samps;
static int pow2squ;
static int pow2exp;
static int myindex;

/* function prototyping */
 int main(); 
 int GetLine(FILE *fp,char *s);
 int GetParam(FILE *fp, PARM_T *p );
 int SubString(char *str_ptr,unsigned int str_start,unsigned int str_end,char *p);
 int StringTrim(char *in,char *out);
 
 int OpenLndsrFile(PARM_T *parm);
 int ReadLndsr(PARM_T *parm);
 int CloseLndsr(PARM_T *parm);
 int ShortestDisCls(int row, int col, PARM_T *parm);
 int MergeCls(PARM_T *parm);
 int Find2ClsWithMiniDis( int *loc_a, int *loc_b, PARM_T *parm);
 
void alloc_1dim_contig (void **, int, int);
void alloc_2dim_contig (void ***, int, int, int);
void alloc_3dim_contig (void ****, int, int, int, int);
void alloc_4dim_contig (void *****, int, int, int, int, int);

int AllocateEarlyVars(PARM_T *parm);

void free_2dim_contig  (void **);
void free_3dim_contig  (void ***);
void free_4dim_contig  (void ****);
void  accum(int cndx, int16 *pixel, PARM_T *parm);
void  ustats(PARM_T *parm);
void  start_pixel(int l, int s);
int  next_pixel(int *pline, int *psamp);
int  find_cluster(int l, int s, PARM_T *parm);
void  add_to_cluster(int16 *pvec, PARM_T *parm);
void  make_new_cluster(int16 *pvec, PARM_T *parm);
void  mcov(PARM_T *parm);
int ipow2(int expo);
