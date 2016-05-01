#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "hdf.h"
#include "mfhdf.h"
#include "proj.h"

#include "envi.h"
#include "space.h"

int main(int argc, char *argv[])
{
        char *fenvi = argv[1];
        double lat = atof(argv[2]);
        double lon = atof(argv[3]);
        int window = atoi(argv[4]);
        int year = atoi(argv[5]);
        int doy = atoi(argv[6]);
        char *tile = argv[7];
        char *sensor = argv[8];
        char *base = argv [9];

        char hdr[1024];
        int len = strlen(fenvi);
        int i;
        strcpy(hdr, fenvi);

        for(i=len-1; i>=0; i--){
                if(hdr[i] == '.'){
                        hdr[i] = '\0';
                        strcat(hdr, ".hdr");
                        break;  
                }
        }

        ENVI_HDR envi;

        if(0 != read_envi_hdr(hdr, &envi)){
                sprintf(hdr, "%s.hdr", fenvi);
                if(0 != read_envi_hdr(hdr, &envi)){
                        printf("ENVI HEADER READ FAILED. %s\n", hdr);
                        return 1;
                }
        }

        if(!envi.have_map){
                printf("ERROR! NO MAP INFO.\n");
                return 1;
        }

        long proj_num = SNSOID;
        long sphere = -1;
        double ul_x = -20015109.354;
        double ul_y = 10007554.677;
        double pix_size = 926.6254330555556;
        //double proj_param[15] = {6371007.181,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        double proj_param[15] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        long utm_zone = -1;

        // currently only consider utm
        if(strcmp(envi.proj, "UTM") == 0){
                proj_num = UTM;
        }
        if(strcmp(envi.datum, "WGS-84") == 0){
                sphere = 12;
        }
        ul_x = envi.upleftX;
        ul_y = envi.upleftY;
        pix_size = envi.pixsizeX;
        utm_zone = envi.utmzone;

        assert(0 == SetupSpace(proj_num, utm_zone, proj_param, sphere, ul_x, ul_y, pix_size));

        double l, s;
        assert(0 == ToSpace(lat, lon, &l, &s));

        //printf("lat=%f, lon=%f, line=%f, sample=%f\n", lat, lon, l, s);

        // read & average
        int np = window / envi.pixsizeX;
        int row = (int)l;
        int col = (int)s;

        int r1 = row - np/2;
        int r2 = row + np/2;
        int c1 = col - np/2;
        int c2 = col + np/2;
        
        assert(r1>=0 && r1<envi.nrow);
        assert(r2>=0 && r2<envi.nrow);
        assert(c1>=0 && c1<envi.ncol);
        assert(c2>=0 && c2<envi.ncol);

        //bip
        if(strcmp(envi.interleave, "bip") != 0){
                printf("THIS IS FOR BIP.\n");
                return 1;
        }

        int sum[envi.nband];
        int cnt[envi.nband];
        int var[envi.nband];

        short buf[envi.ncol*envi.nband];
        int r, c, b;

        for(b=0; b<envi.nband; b++){
                sum[b] = 0;
                cnt[b] = 0;
                var[b] = 0;
        }

        FILE *fp = fopen(fenvi, "rb");
        assert(fp != NULL);

        for(r=r1; r<=r2; r++){
                //printf("row=%d, seek pos= %ld\n", r, 2*r*envi.ncol*envi.nband);
                assert(0 == fseek(fp, 2*r*envi.ncol*envi.nband, SEEK_SET));
                assert(envi.ncol*envi.nband == fread(buf, 2, envi.ncol*envi.nband, fp));
                for(c=c1; c<=c2; c++){
                        for(b=0; b<envi.nband; b++){
                                if(buf[c*envi.nband+b] != 32767){
                                        sum[b] += buf[c*envi.nband+b];
                                        cnt[b] ++;
                                }       
                                /*if(r == row && c == col){
                                        printf("ROW %d COL %d BAND %d: %d\n", r, c, b, buf[c*envi.nband+b]);
                                }*/
                        }               
                }
        }




        for(b=0; b<envi.nband; b++){
                if(cnt[b] > 0){
                        sum[b] /= cnt[b];
                }
                else{
                        sum[b] = 32767;
                }
        }

        for(r=r1; r<=r2; r++){
                //printf("row=%d, seek pos= %ld\n", r, 2*r*envi.ncol*envi.nband);
                assert(0 == fseek(fp, 2*r*envi.ncol*envi.nband, SEEK_SET));
                assert(envi.ncol*envi.nband == fread(buf, 2, envi.ncol*envi.nband, fp));
                for(c=c1; c<=c2; c++){
                        for(b=0; b<envi.nband; b++){
                                if(buf[c*envi.nband+b] != 32767){
                                        var[b] += (buf[c*envi.nband+b]-sum[b])*(buf[c*envi.nband+b]-sum[b]);
                                }       
                                /*if(r == row && c == col){
                                        printf("ROW %d COL %d BAND %d: %d\n", r, c, b, buf[c*envi.nband+b]);
                                }*/
                        }               
                }
        }



        for(b=0; b<envi.nband; b++){
                if(cnt[b] > 0){
                        var[b] = sqrt(var[b] / cnt[b]);
                }
                else{
                        var[b] = 32767;
                }
        }

        printf("%s,%d,%03d,%f,%f,%s,%s,", tile, year, doy, lat, lon, sensor, base);    
        for(b=0; b<envi.nband; b++){
                //printf("AVERAGE BAND %d: %d, COUNT: %d\n", b, sum[b], cnt[b]);
                if(b < envi.nband-1){
                        printf("%.3f,", sum[b]/10000.0);
                        printf("%.3f,", var[b]/10000.0);
                        printf("%d,", cnt[b]);
                }
                else{
                        printf("%.3f,", sum[b]/10000.0);
                        printf("%.3f,", var[b]/10000.0);
                        printf("%d\n", cnt[b]);
                }
        }

        return 0;
}
