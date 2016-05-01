#ifndef __INC_SPACE_H
#define __INC_SPACE_H

int SetupSpace(long proj_num, long zone, double *proj_param, long sphere, double ul_x, double ul_y, double pix_size);
int ToSpace(double lat, double lon, double *l, double *s);
int FromSpace(double l, double s, double *lat, double *lon);

#endif
