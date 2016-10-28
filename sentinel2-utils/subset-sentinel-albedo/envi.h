#ifndef __INC_ENVI_H
#define __INC_ENVI_H

typedef struct{
	int nrow;
	int ncol;
	int nband;
	int dtype;
	char interleave[10];
	int have_map;
	char proj[10];
	double tieX;
	double tieY;
	double upleftX;
	double upleftY;
	double pixsizeX;
	double pixsizeY;
	int utmzone;
	char orig[10];
	char datum[10];
	char unit[10];
}ENVI_HDR;

int read_envi_hdr(char *hdr, ENVI_HDR *envi);

#endif
