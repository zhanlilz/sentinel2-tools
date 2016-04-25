#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "proj.h"
#include "cproj.h"

#include "hdf.h"
#include "mfhdf.h"
#include "HdfEosDef.h"

#include "sqs.h"

#define MAX_STRLEN   400
#define SUCCESS 1
#define FAILURE -1
#define GRID_ERRCODE -1 /*HDF-EOS error code*/

#define lndAlbedoFILLV 32767
#define FILLV   -9999   
#define MODIS_NBANDS 7
#define SENSOR_BANDS 7

#define MAX_cls 20

#define HIS_BIN 5
#define TOP_Pct 15
#define PUREPIX_thredhold 12 /*12=60/100*20 means 60%*/

#define NUMBER_BS_KERNELALBEDOS 91	/* number of black-sky kernel albedos
					   stored in albedo table file */

/*#define D2R 3.1415926535/180.0*/
#define  LndScaleFct 0.0001

/* Model internal paramerters definitions */
#define MODISLISPARSEBR 1.0
#define MODISLISPARSEHB 2.0

/*#define DEBUG */
#ifdef DEBUG
#define DEBUG_irow 1653
#define DEBUG_icol 4541
#endif

#define NSCAN 375
#define LINES_PER_SCAN 16

#define OUT_SpectralAlbedo

/**************************************************/
typedef struct {
  char clsmap[MAX_STRLEN];
  char clsDist[MAX_STRLEN];
  char  fileName[MAX_STRLEN];
  char lndalbedoName[MAX_STRLEN];
  char lndQAName[MAX_STRLEN];
  int    nrows; 
  int    ncols;
  int    nbands;
  int    actu_ncls;
  int init_ncls;
  int  Cls_fillValue;
  int16  SR_fillValue;
  double lndSR_ScaFct;
  double lndTemp_ScaFct;
  int    range[2];

  double ulx;              /* x value of up-left corner */
  double uly;              /* y value of up-left corner */
  double res;              /* spatial resolution */

  double SZA;     /*solar zenith angle for each scene*/
  double SAA;     /*solar zenith angle for each scene*/
  double pix_VZA;
  double pix_VAA;
  
  long   insys;
  long   inzone;
  double inparm[15];
  long   inunit;
  long   indatum;

  int32 fp_id;
  int32 sr_id[SENSOR_BANDS];  
   int32 lndQA_id;
  int32 lndatmo_id;
  
  FILE  *fpCls;
  FILE *fpClsDist;
  FILE  *fplndalbedo;
  FILE *fplndQA;
  FILE *fpSpectralAlbedo;
  FILE *fpVAA; /* pixel based-view azimuth*/
  int8  *Clsdata;         /* in [icol] */
  int16 Pix_sr[SENSOR_BANDS];
  int16 pure_thrsh[MAX_cls];

  uint16 **OneRowlndSR;
  uint8 *OneRowlndSRQAData;
  int16 *OneRowlndSRAtmoData;
	uint16 *OneRowlndSRCldData;
	uint16 *OneRowlndSRSnwData;

 /* nadir-view line */
  double slop;
  double interception;
  double half_dist;
	
  /* horizontal direction line, normal to the nadir-view line */
  double slop2;
  double interception2;

  int img_upleft[2];	// col, row
  int img_upright[2];	// col, row
  int img_lowleft[2];
  int img_lowright[2];
  double NE_center[2];
  double SE_center[2];
  double oneScanDist_alongNadir;
  
 double scan_centers[2][NSCAN];	// col, row

 /* double BRDF_paras[MAX_cls][MODIS_NBANDS][3]; *//*[icls][MOD_band][ipara], save the averaged high-quality BRDFs*/
 double ***BRDF_paras;
 double **AmongClsDistance;
 int16 *OneRowlndSWalbedo; /*BSA and WSA*/
 int16 **OneRowlndSpectralAlbedo; /*6 bands of spectral BSA & WSA*/
 int8 *OneRowlndQA;

 usgs_t scene;
  
} DIS_LANDSAT;   

typedef struct {
  char srName[MAX_STRLEN];       /* albedo hdf file name */
  char qaName[MAX_STRLEN];       /* QA hdf file name if there is */
  char his_name[MAX_STRLEN];  /* output the percentage histogram of each majority class */
  char ratioMap[MAX_STRLEN];  /* 2400*2400 WSA & BSA ratio Map for each MODIS band */

  int32 nrows;           
  int32 ncols;
  float ulx;              /* x value of up-left corner */
  float uly;              /* y value of up-left corner */
  int16 fillValue;
  int16 range[2];
  float64 scale_factor;
  float64 res;

  /* image metadata */
  float64 GD_upleft[2];
  float64 GD_lowright[2];
  int32   GD_projcode;
  int32   GD_zonecode;
  int32   GD_spherecode;
  float64 GD_projparm[16];
  int32   GD_origincode;
  int32   GD_unit;
  int32   GD_datum;

  int32 SD_ID;               /* HDF scientific data grid ID for SR */
  int32 QA_ID;               /* HDF scientific data grid ID for QA */
  int32 sr_id[MODIS_NBANDS]; /* sds handler for albedo */
  int32 brdf_id[MODIS_NBANDS];           /*sds handler for BRDFs 3 layers in MCD43A1-Yanmin 2009/10*/
  int32 qa_id;               /* sds handler for quality */
	/*sqs*/
  int32 qa_ids[MODIS_NBANDS];               /* sds handler for quality for V006 */
  int32 snow_id;
  int32 igbp_id;

  int16  **sr;            /* albedo (one row) in [iband][icol] */
  int16 Pix_brdf[MODIS_NBANDS][3];  /*brdf shape for one pixel*/
  uint32 Pix_qa;        /*QA for one pixel*/
  uint32 *qa;             /* reflectance quality (one row) */
  int8   *snow;
  uint8  *igbp;

  int8  **major_cls;     /* majority cluster from fine resolution data */
  uint8  **percent_cls;   /* percentage of majority cluster */

  

  int min_percent;        /* minimum acceptable percentage of major class */
  int year;
  int jday;

  char aggFileName[MAX_STRLEN]; 
  FILE *agg_fp;

} GRID_MODIS;    /* structure to store albedo info */

int parseParameters(char *ifile, DIS_LANDSAT *sensor, GRID_MODIS *modis);
int  getModisMetaInfo(GRID_MODIS *sr);
int  openSensorFile(DIS_LANDSAT *sensor);
int  loadSensorClsRow(DIS_LANDSAT *sensor, int irow);
int  loadSensorSRPixel(DIS_LANDSAT *sensor, int irow, int icol);
int load_lndSRrow(DIS_LANDSAT *sensor, int irow);
int  loadModisRow(GRID_MODIS *modis, int irow);
int  loadModisPixel(GRID_MODIS *modis, int irow, int icol);
int  Sensor2MODIS(DIS_LANDSAT *sensor, int irow, int icol, GRID_MODIS *modis, int *mrow, int *mcol);
int CloudMask(DIS_LANDSAT *LndWorkspace,int icol);
int SnowDetermination(DIS_LANDSAT *LndWorkspace, int icol);
int PrepareCalculation(DIS_LANDSAT * sensor);
int CalculateANratio(int16 *brdf, double SZA,double SAA,double VZA, double VAA, double *ANR_wsa, double *ANR_bsa);
int CalculateAlbedo (DIS_LANDSAT * sensor, double *lndPixAlbedo, int row, int col, int8 * QA, int snow_flag);
int  ReprojectLnd2MODIS(DIS_LANDSAT *sensor, GRID_MODIS *modis);
int getAmongClassesDistance(DIS_LANDSAT *sensor); /* Yanmin 2012/06 */
int getClosestCls(DIS_LANDSAT *LndWorkspace, int icls, int band); /* Yanmin 2012/06 */
int GetEquation4VZACorrection(DIS_LANDSAT *);
double CalPixVZA(DIS_LANDSAT *sensor,int irow,int icol);
double CalPixAzimuth(DIS_LANDSAT *LndWorkspace,int irow,int icol);
double lndSRPixVZA(DIS_LANDSAT *LndWorkspace,int row, int col);
double GetVZARefCorrectionFactor(int16 *pix_brdf,double SZA, double iVZA);
double CalBRDFMagFct(double ***BRDFlut,DIS_LANDSAT *sensor,int iband);
int WriteOutHeadFile(DIS_LANDSAT *sensor);
int  cleanUpModis(GRID_MODIS *sr);
int  cleanUpSensor(DIS_LANDSAT *sensor);
void alloc_1dim_contig (void **, int, int);
void alloc_2dim_contig (void ***, int, int, int);
void alloc_3dim_contig (void ****, int, int, int, int);
void alloc_4dim_contig (void *****, int, int, int, int, int);
void free_1dim_contig(void *);
void free_2dim_contig  (void **);
void free_3dim_contig  (void ***);
void free_4dim_contig  (void ****);
int substr(char *str,char*s, int begin,int end);
/*void dec2bin(unsigned short int num, int *bitpattern, int limit);*/
void dec2bin(unsigned int num, int *bitpattern, int limit);
void GetPhaseAngle (double cos1, double cos2, double sin1, double sin2, double cos3, double *cosres, double *res, double *sinres);
void GetDistance (double tan1, double tan2, double cos3, double *res);
void GetpAngles (double brratio,double tan1,double *sinp, double *cosp, double *tanp);
void GetOverlap (double hbratio,
	      double distance,
	      double cos1,
	      double cos2,
	      double tan1,
	      double tan2,
	      double sin3,
	      double *overlap,
	      double *temp);
double RossThick_kernel(double tv, double ti, double phi);
double LiSparseR_kernel(double tv,double ti,double phi);

int CalcScanCenters(DIS_LANDSAT *LndWorkspace);
