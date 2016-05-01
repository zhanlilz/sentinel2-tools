#include <stdio.h>
#include <assert.h>
#include "envi.h"

int read_envi_hdr(char *hdr, ENVI_HDR *envi)
{
	if(envi == NULL){
		return -1;
	}

	FILE *fp = fopen(hdr, "r");
	if(fp == NULL){
		return -1;
	}

	char line[1024];
	char stmp[20];

	while(fgets(line, 1023, fp) != NULL){
		if(strncmp(line, "samples", 7) == 0){
			assert(1 == sscanf(line, "%*s = %d", &envi->ncol));
		}	
		if(strncmp(line, "lines", 5) == 0){
			assert(1 == sscanf(line, "%*s = %d", &envi->nrow));
		}	
		if(strncmp(line, "bands", 5) == 0){
			assert(1 == sscanf(line, "%*s = %d", &envi->nband));
		}	
		if(strncmp(line, "data type", 9) == 0){
			assert(1 == sscanf(line, "%*s %*s = %d", &envi->dtype));
		}	
		if(strncmp(line, "interleave", 10) == 0){
			assert(1 == sscanf(line, "%*s = %s", envi->interleave));
		}	
		if(strncmp(line, "map info", 8) == 0){
			assert(11 == sscanf(line, "%*s %*5s{%[^,], %lf, %lf, %lf, %lf, %lf, %lf, %d, %[^,], %[^,], %*[^=]=%[^}]}",
															envi->proj, &envi->tieX, &envi->tieY, &envi->upleftX, &envi->upleftY, 
															&envi->pixsizeX, &envi->pixsizeY, &envi->utmzone, envi->orig, envi->datum, envi->unit));
			envi->have_map = 1;
		}	
	}
	
	fclose(fp);
	return 0;
}
