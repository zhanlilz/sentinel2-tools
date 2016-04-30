#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "proj.h"
#include "cproj.h"

#define MAX_PROJ (31)  /* Maximum map projection number */

double DE2RA = 0.01745329252;
double RA2DE = 57.2957795129;

long (*my_for_trans)() = NULL;
long (*my_inv_trans)() = NULL;

static void for_init(long proj_num, long zone, double *proj_param, long sphere,
              char *file27, char *file83, long *iflag, 
	      long (*for_trans[MAX_PROJ + 1])());
static void inv_init(long proj_num, long zone, double *proj_param, long sphere,
              char *file27, char *file83, long *iflag, 
	      long (*inv_trans[MAX_PROJ + 1])());

double ul_corner_x;
double ul_corner_y;
double pixel_size;

int SetupSpace(long proj_num, long zone, double *proj_param, long sphere, double ul_x, double ul_y, double pix_size)
{
  char file27[1024];          /* name of NAD 1927 parameter file */
  char file83[1024];          /* name of NAD 1983 parameter file */
	char *libgctp;
  
	long (*for_trans[MAX_PROJ + 1])();
  long (*inv_trans[MAX_PROJ + 1])();
  
	libgctp = (char *)getenv("LIBGCTP");
  sprintf(file27, "%s/nad27sp", libgctp);
  sprintf(file83, "%s/nad83sp", libgctp);

	long iflag = 0;

	for_init(proj_num, zone, proj_param, sphere, file27, file83, &iflag, for_trans);
	if(iflag != 0){
		printf("for_init failed. iflag=%d\n", iflag);
		return -1;
	}
  my_for_trans = for_trans[proj_num];
	
	inv_init(proj_num, zone, proj_param, sphere, file27, file83, &iflag, inv_trans);
	if(iflag != 0){
		printf("inv_init failed. iflag=%d\n", iflag);
		return -1;
	}
  my_inv_trans = inv_trans[proj_num];

	pixel_size = pix_size;
	ul_corner_x = ul_x;
	ul_corner_y = ul_y;

	return 0;
}

int ToSpace(double lat, double lon, double *l, double *s)
{
	double x, y;

	lat *= DE2RA;
	lon *= DE2RA;

  if(my_for_trans == NULL){
		return -1;
	}
  if (my_for_trans(lon, lat, &x, &y) != 0) {
		return -1;
	}

	*l = (ul_corner_y - y) / pixel_size;
  *s = (x - ul_corner_x) / pixel_size;

  return 0;
}

int FromSpace(double l, double s, double *lat, double *lon)
{

	double x, y;

  y = ul_corner_y - (l * pixel_size);
  x = ul_corner_x + (s * pixel_size);

  if(my_inv_trans == NULL){
		return -1;
	}
 
	if(my_inv_trans(x, y, lon, lat) != 0) 
		return -1;

	*lat *= RA2DE;
	*lon *= RA2DE;

  return 0;
}
