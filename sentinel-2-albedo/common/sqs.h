#ifndef __H_SQS_
#define __H_SQS_

#include "gdal.h"
#include "ogr_srs_api.h"

#define TIF   	(1)
#define BIN  	  (2)

#define _TM_ 			(1)
#define _ETM_ 		(2)
#define _OLI_ 		(3)
#define MSI 		  (4)

#define MAX_TOT_BAND (20)
#define N_SRF_BAND (6)

#define QA_BAND_NAME "cfmask"

typedef struct
{
	int file_format;
	int nrow;
	int ncol;
	int nbyte_per_pix;
	FILE *fp;
	GDALDatasetH hDataset;
	double fill_value;
	double scale_factor;
	double pixel_size;
	char name[50];
	char data_type[20];
	char file_name[1024];

} band_t;

typedef struct
{
	int nband;
	int zone;
	int instrument;

	double szn;
	double san;
	double vzn;
	double van;
	double ulx;
	double uly;
	double lrx;
	double lry;

	int srf_band_index[N_SRF_BAND];
	int qa_band_index;
	int cld_band_index;
	int snw_band_index;
	char datum[20];
	char units[20];
	char projection[20];
	char satellite[20];
	char xml_file[1024];
	band_t band[MAX_TOT_BAND];

	int year;
	int doy;
} usgs_t;

void init_gdal ();
int read_one_row_gdal (GDALDatasetH hDataset, int iBand, int iRow, void *buf);
int get_geo_info_gdal (GDALDatasetH hDataset, long *sys, long *zone,
											 long *datum, double parm[], double *ulx, double *uly,
											 double *res);
int get_geo_info_hdf (char *fname, long *sys, long *zone, long *datum,
											double parm[], double *ulx, double *uly, double *res);
int parse_usgs_xml (char *fname, usgs_t * scene);
int parse_sentinel_xml (char *fname, usgs_t * scene);
int open_a_band (band_t * band);
void close_a_band (band_t * band);
int read_a_band_a_row (band_t * band, int row, void *buf);
int read_a_band (band_t * band, void *buf);

long usgs_gctp_datum (char *name);
int get_scene_proj (usgs_t * scene, long *sys, long *zone, long *datum,
										double parm[], double *ulx, double *uly, double *res);
int convert_to_binary (usgs_t * scene);

#endif
