

#ifndef __H_SPACE
#define __H_SPACE

int ToSpace (double lat, double lon, double *l, double *s);
int FromSpace (double l, double s, double *lat, double *lon);
int SetupSpace (long proj_num, long zone, double *proj_param, long sphere,
								double ul_x, double ul_y, double pix_size);

#endif
