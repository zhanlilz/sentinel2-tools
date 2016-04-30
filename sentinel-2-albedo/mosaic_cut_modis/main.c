#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <assert.h>
#include "proj.h"
#include "hdf.h"
#include "space.h"
#include "def.h"
#include "sqs.h"

typedef struct{
        char fname_s[1024];
        char fname_t[1024];
        int32 start_s[2];
        int32 edge_s[2];
        int32 start_t[2];
        int32 edge_t[2];
}Job;

#define MAX_JOB 9
int N_JOB = 0;
Job JOBS[MAX_JOB];

const char *Data_dir = NULL;
const char *Data_head = NULL;
char path[1024];

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KYEL  "\x1B[33m"
char msg[1024];

#define WARN(msg) {printf("%s%s%s",KYEL,msg,KNRM);}
#define ERRO(msg) {printf("%s%s%s",KRED,msg,KNRM);}

void make_region(Point *center, Region *region);
int create_output(char *hdf_sample, char *hdf_out, Region *region);
int mosaic_cut(char *hdf_out, double lat, double lon, int year, int doy);
int mosaic_cut_new(char *hdf_out, double lat, double lon, int year, int doy);
int _get_hdf_data(int h, int v, int year, int doy, char *result);
int new_job(char *hdf_source, int32 start_source[], int32 edge_source[], 
                                                                 char *hdf_target, int32 start_target[], int32 edge_target[]);
int find_modis_region(char *xml, Region *rModis, int *year, int *doy);
int mosaic_cut_new2(char *hdf_out, Region *rModis, int year, int doy);
void geo2grid(double lat, double lon, Point *point);
void add_job(Job *job);
int submit_jobs();

int fill_flag = 0;

int main(int argc, char *argv[])
{
        char *me = argv[0];
        Data_dir = argv[1];
        Data_head = argv[2];
        char *hdf_out = argv[3];
        int nXml = atoi(argv[4]);
        int i;

        char *usage = "usage: exe MODIS_dir MCD43A1/MCD43A2 hdf_out nXml xml1 xml2 ... xmln";
        if(argc < 6 || argc < nXml+5){
                printf("%s\n", usage);
                return -1;
        }

        if(argc > nXml+5){
                if(strcmp(argv[nXml+5],"-f") != 0){
                        printf("%s\n", usage);
                        return -1;
                }
                fill_flag = 1;
        }
        
        sprintf(path, me);
        int len = strlen(path);
        for(i=len-1; i>=0; i--){
                if(path[i] == '/'){
                        path[i] = '\0';
                        break;
                }
        }
        
        Region rModis;
        Region rTmp;
        int year, doy;

        for(i=0; i<nXml; i++){
                char *xml = argv[5+i];

                if(i==0){
                        find_modis_region(xml, &rModis, &year, &doy);
                        printf("xml%d: MODIS REGION: ROWS %d-%d, COLS %d-%d\n",
                                                 i+1, rModis.upper_left.row, rModis.lower_right.row,
                                                 rModis.upper_left.col, rModis.lower_right.col);
                }
                else{
                        find_modis_region(xml, &rTmp, &year, &doy);
                        if(rTmp.upper_left.row < rModis.upper_left.row){
                                rModis.upper_left.row = rTmp.upper_left.row;
                        }
                        if(rTmp.upper_left.col < rModis.upper_left.col){
                                rModis.upper_left.col = rTmp.upper_left.col;
                        }
                        if(rTmp.lower_right.row > rModis.lower_right.row){
                                rModis.lower_right.row = rTmp.lower_right.row;
                        }
                        if(rTmp.lower_right.col > rModis.lower_right.col){
                                rModis.lower_right.col = rTmp.lower_right.col;
                        }
                        
                        printf("xml%d: MODIS REGION: ROWS %d-%d, COLS %d-%d\n",
                                                 i+1, rModis.upper_left.row, rModis.lower_right.row,
                                                 rModis.upper_left.col, rModis.lower_right.col);
                }
        }

        assert(0 == mosaic_cut_new2(hdf_out, &rModis, year, doy));

        return 0;
}

int main1(int argc, char *argv[])
{
        char *me = argv[0];
        char *data_dir = argv[1];
        char *data_head = argv[2];
        //double lat = atof(argv[3]);
        //double lon = atof(argv[4]);
        //int year = atoi(argv[5]);
        //int doy = atoi(argv[6]);
        char *xml = argv[3];
        char *hdf_out = argv[4];

        if(argc == 6){
                if(strcmp(argv[5], "-f") == 0){
                        fill_flag = 1;
                }
        }
        
        usgs_t scene;
        long sys, zone, datum;
        double parm[15], ulx, uly, res;
        double lat, lon;
        int irow, icol;
        int year, doy;

        if(0 != parse_usgs_xml(xml, &scene)){
                if(0 != parse_sentinel_xml(xml, &scene)){
                        printf("Parse xml failed. XML=%s\n", xml);
                        return -1;
                }
        }
        
        int index = scene.srf_band_index[0];
        if(0 != open_a_band(&(scene.band[index]))){
                printf("Open band %s failed.\n", scene.band[index].file_name);
        }

        if(0 != get_scene_proj(&scene, &sys, &zone, &datum, parm, &ulx, &uly, &res)){
                printf("Get proj info failed.\n");
          return -1;
        }

        close_a_band(&(scene.band[index]));

        // scene center
        irow = scene.band[index].nrow / 2;
        icol = scene.band[index].ncol / 2;
        year = scene.year;
        doy = scene.doy;

        if(0 != SetupSpace(sys, zone, parm, datum, ulx, uly, res)){
                printf("Setup space failed.\n");
                return -1;
        }
        if(0 != FromSpace((double)irow, (double)icol, &lat, &lon)){
                printf("from space failed.\n");
                return -1;
        }

        // print
        printf("xml=%s\n", xml);
        printf("year=%d, doy=%d, nrow=%d, ncol=%d, band 0 index=%d\n", year, doy, scene.band[0].nrow, scene.band[0].ncol, index);
        printf("proj sys=%d, zone=%d, datum=%d, ulx=%lf, uly=%lf, res=%lf\n", sys, zone, datum, ulx, uly, res);
        printf("center row=%d, col=%d, lat=%lf, lon=%lf\n", irow, icol, lat, lon);

        Data_dir = data_dir;
        Data_head = data_head;
        sprintf(path, me);
        int i;
        int len = strlen(path);
        for(i=len-1; i>=0; i--){
                if(path[i] == '/'){
                        path[i] = '\0';
                        break;
                }
        }

        assert(0 == SetupSpace(PROJ_NUM, UTM_ZONE, PROJ_PARAM, SPHERE, UL_X, UL_Y, PIX_SIZE));

        assert(0 == mosaic_cut_new(hdf_out, lat, lon, year, doy));

        printf("SUCCEED. %s\n\n", hdf_out);

        return 0;
}

int main2(int argc, char *argv[])
{
        char *me = argv[0];
        char *data_dir = argv[1];
        char *data_head = argv[2];
        double lat = atof(argv[3]);
        double lon = atof(argv[4]);
        int year = atoi(argv[5]);
        int doy = atoi(argv[6]);
        char *hdf_out = argv[7];

        Data_dir = data_dir;
        Data_head = data_head;
        sprintf(path, me);
        int i;
        int len = strlen(path);
        for(i=len-1; i>=0; i--){
                if(path[i] == '/'){
                        path[i] = '\0';
                        break;
                }
        }

        assert(0 == SetupSpace(PROJ_NUM, UTM_ZONE, PROJ_PARAM, SPHERE, UL_X, UL_Y, PIX_SIZE));

        assert(0 == mosaic_cut(hdf_out, lat, lon, year, doy));

        return 0;
}

int find_modis_region(char *xml, Region *rModis, int *year, int *doy)
{
        usgs_t scene;
        long sys, zone, datum;
        double parm[15], ulx, uly, res;

        if(0 != parse_usgs_xml(xml, &scene)){
                if(0 != parse_sentinel_xml(xml, &scene)){
                        printf("Parse xml failed. XML=%s\n", xml);
                        return -1;
                }
        }
        
        int index = scene.srf_band_index[0];

        /* if(0 != open_a_band(&(scene.band[index]))){ */
        /*         printf("Open band %s failed.\n", scene.band[index].file_name); */
        /* } */

        if(0 != get_scene_proj(&scene, &sys, &zone, &datum, parm, &ulx, &uly, &res)){
                printf("Get proj info failed.\n");
          return -1;
        }

        // close_a_band(&(scene.band[index]));
        
        *year = scene.year;
        *doy = scene.doy;

        double lat1, lon1;
        double lat2, lon2;
        double lat3, lon3;
        double lat4, lon4;
        int irow1, icol1;
        int irow2, icol2;
        int irow3, icol3;
        int irow4, icol4;

        irow1 = 0;
        icol1 = 0;

        irow2 = 0;
        icol2 = scene.band[index].ncol-1;

        irow3 = scene.band[index].nrow-1;
        icol3 = scene.band[index].ncol-1;

        irow4 = scene.band[index].nrow-1;
        icol4 = 0;

        // landsat proj
        assert(0 == SetupSpace(sys, zone, parm, datum, ulx, uly, res));
        assert(0 == FromSpace((double)irow1, (double)icol1, &lat1, &lon1));
        assert(0 == FromSpace((double)irow2, (double)icol2, &lat2, &lon2));
        assert(0 == FromSpace((double)irow3, (double)icol3, &lat3, &lon3));
        assert(0 == FromSpace((double)irow4, (double)icol4, &lat4, &lon4));

        // modis proj
        Point pTmp;
        int east, west, north, south;

        assert(0 == SetupSpace(PROJ_NUM, UTM_ZONE, PROJ_PARAM, SPHERE, UL_X, UL_Y, PIX_SIZE));

        geo2grid(lat1, lon1, &pTmp);
        east = pTmp.col;
        west = pTmp.col;
        north = pTmp.row;
        south = pTmp.row;

        geo2grid(lat2, lon2, &pTmp);
        if(pTmp.col > east){
                east = pTmp.col;
        }
        if(pTmp.col < west){
                west = pTmp.col;
        }
        if(pTmp.row > south){
                south = pTmp.row;
        }
        if(pTmp.row < north){
                north = pTmp.row;
        }

        geo2grid(lat3, lon3, &pTmp);
        if(pTmp.col > east){
                east = pTmp.col;
        }
        if(pTmp.col < west){
                west = pTmp.col;
        }
        if(pTmp.row > south){
                south = pTmp.row;
        }
        if(pTmp.row < north){
                north = pTmp.row;
        }

        geo2grid(lat4, lon4, &pTmp);
        if(pTmp.col > east){
                east = pTmp.col;
        }
        if(pTmp.col < west){
                west = pTmp.col;
        }
        if(pTmp.row > south){
                south = pTmp.row;
        }
        if(pTmp.row < north){
                north = pTmp.row;
        }

        int buffer = 5;

        rModis->upper_left.row = north-buffer;
        rModis->upper_left.col = west-buffer;
        rModis->lower_right.row = south+buffer;
        rModis->lower_right.col = east+buffer;

        return 0;
}

void global2tile(Point *gpoint, TilePoint *tpoint)
{
        int h = gpoint->col / TILE_NCOL;
        int v = gpoint->row / TILE_NROW;
        int trow = gpoint->row % TILE_NROW;     
        int tcol = gpoint->col % TILE_NCOL;     

        tpoint->h = h;
        tpoint->v = v;
        tpoint->row = trow;
        tpoint->col = tcol;
}

void geo2grid(double lat, double lon, Point *point)
{
        double l, s;
        assert(0 == ToSpace(lat, lon, &l, &s));
        
        point->row = (int)l;    
        point->col = (int)s;    
}

int get_hdf_data(Point *point, int year, int doy, char *result)
{
        if(Data_dir == NULL || Data_head == NULL){
                printf("dir or header is null.\n");
                return -1;
        }

        TilePoint tp;
        global2tile(point, &tp);

        return _get_hdf_data(tp.h, tp.v, year, doy, result);
}

int _get_hdf_data(int h, int v, int year, int doy, char *result)
{
        char cmd[2048];
        char tile[10];
        sprintf(tile, "h%02dv%02d", h, v);
        sprintf(cmd, "find %s -name \"%s.A%d%03d.%s.*hdf\" > /tmp/tmpfile_sqs_1231232323", Data_dir, Data_head, year, doy, tile);
        system(cmd);
        FILE *fp = fopen("/tmp/tmpfile_sqs_1231232323", "r");
        if(fp == NULL){
                sprintf(msg, "can't open the temp file.\n");
                ERRO(msg);
                return -1;
        }       

        //static char result[1024];
        if(fgets(result, 1023, fp) == NULL){
                sprintf(msg, "Not found %s %d-%03d %s %s\n", tile, year, doy, Data_dir, Data_head);
                WARN(msg);
                fclose(fp);
                return -1;
        }

        // why closing here can lead to failure?
        //fclose(fp);

        if(strlen(result) <= strlen(Data_dir)){
                sprintf(msg, "Not found %s %d-%03d %s %s\n", tile, year, doy, Data_dir, Data_head);
                WARN(msg);
                return -1;
        }

        if(fgets(result, 1023, fp) != NULL){
                sprintf(msg, "Error, find multiple files for %s %d %03d.\n", tile, year, doy);
                ERRO(msg);
                return -1;
        }

        int n = strlen(result);
        if(result[n-1] == '\n'){
                result[n-1] = '\0';
        }
        return 0;
}

int sametile(TilePoint *tp1, TilePoint *tp2)
{
        if(tp1->h == tp2->h && tp1->v == tp2->v){
                return 1;
        }

        return 0;
}

int create_output(char *hdf_sample, char *hdf_out, Region *region)
{
        char cmd[1024];
        sprintf(cmd, "%s/create %s %s %d %d %d %d", path, hdf_sample, hdf_out,
                                                                region->upper_left.row,region->upper_left.col,
                                                                region->lower_right.row,region->lower_right.col);
        assert(0 == system(cmd));
        return 0;
}

int new_job(char *hdf_source, int32 start_source[], int32 edge_source[], 
                                                                 char *hdf_target, int32 start_target[], int32 edge_target[])
{
        Job job;
        sprintf(job.fname_s, hdf_source);
        sprintf(job.fname_t, hdf_target);
        job.start_s[0] = start_source[0];
        job.start_s[1] = start_source[1];
        job.edge_s[0] = edge_source[0];
        job.edge_s[1] = edge_source[1];
        job.start_t[0] = start_target[0];
        job.start_t[1] = start_target[1];
        job.edge_t[0] = edge_target[0];
        job.edge_t[1] = edge_target[1];

        add_job(&job);

        return 0;
}

int do_copy_sub_hdf(char *hdf_source, int32 start_source[], int32 edge_source[], 
                                                                 char *hdf_target, int32 start_target[], int32 edge_target[])
{
        char cmd[1024];
        sprintf(cmd, "%s/copy %s %s %d %d %d %d %d %d %d %d", path, hdf_source, hdf_target,
                                                                        start_source[0],start_source[1],edge_source[0],edge_source[1],
                                                                        start_target[0],start_target[1],edge_target[0],edge_target[1]);
        assert(0 == system(cmd));
        return 0;
}

int mosaic_cut_new(char *hdf_out, double lat, double lon, int year, int doy)
{
        Point center;   
        geo2grid(lat, lon, &center);

        Region region;
        make_region(&center, &region);

        int32 start[2];
        int32 edge[2];
        int32 start_out[2];
        int32 edge_out[2];

        int row, col;
        int h, v;

        Point *p1 = &(region.upper_left);
        Point *p2 = &(region.upper_right);
        Point *p3 = &(region.lower_right);
        Point *p4 = &(region.lower_left);
                
        TilePoint tp1, tp2, tp3, tp4;
        global2tile(p1, &tp1);
        global2tile(p2, &tp2);
        global2tile(p3, &tp3);
        global2tile(p4, &tp4);

        printf("Tiles: h%02d-%02d, v%02d-%02d.\n", tp1.h, tp2.h, tp1.v, tp4.v);

        //copy sds
        char hdf_in[1024];
        char hdf_sample[1024]; 
        assert(0 == get_hdf_data(&center, year, doy, hdf_sample));
        assert(0 == create_output(hdf_sample, hdf_out, &region));
        
        for(h=tp1.h; h<=tp2.h; h++){
                for(v=tp1.v; v<=tp4.v; v++){
                        // source
                        // start row
                        if(v==tp1.v){
                                start[0] = tp1.row;
                        }       
                        else{
                                start[0] = 0;
                        }
                        // edge row
                        if(v==tp4.v){
                                edge[0] = tp4.row - start[0] + 1;
                        }
                        else{
                                edge[0] = TILE_NROW - start[0];
                        }

                        //start col
                        if(h==tp1.h){
                                start[1] = tp1.col;
                        }       
                        else{
                                start[1] = 0;
                        }
                        // edge col
                        if(h==tp2.h){
                                edge[1] = tp2.col - start[1] + 1;
                        }
                        else{
                                edge[1] = TILE_NCOL - start[1];
                        }

                        //target
                        edge_out[0] = edge[0];          
                        edge_out[1] = edge[1];          

                        if(v==tp1.v){
                                start_out[0] = 0;
                        }
                        else{
                                start_out[0] = (TILE_NROW-tp1.row)+(v-tp1.v-1)*TILE_NROW;
                        }

                        if(h==tp1.h){
                                start_out[1] = 0;
                        }
                        else{
                                start_out[1] = (TILE_NCOL-tp1.col)+(h-tp1.h-1)*TILE_NCOL;
                        }
                        
                        // submit       
                        /*printf("adding job: h%02dv%02d row [%5d,%5d] col [%5d,%5d] ---> row [%5d,%5d] col [%5d,%5d].\n", 
                                                                h,v,start[0],start[0]+edge[0]-1,start[1],start[1]+edge[1]-1,start_out[0],start_out[0]+edge_out[0]-1,
                                                                start_out[1],start_out[1]+edge_out[1]-1);*/
                        if(0 == _get_hdf_data(h, v, year, doy, hdf_in)){
                                assert(0 == new_job(hdf_in, start, edge, hdf_out, start_out, edge_out));
                        }
                        else if(!fill_flag){
                                sprintf(msg, "Error: h%02dv%02d is missing.\n", h, v);
                                ERRO(msg);
                                return -1;
                        }
                        else{
                                sprintf(msg, "Warning: h%02dv%02d is missing. flag -f detected.\n", h, v);
                                WARN(msg);
                        }
                }
        }

        assert(0 == submit_jobs());
        return 0;
}

int mosaic_cut_new2(char *hdf_out, Region *rModis, int year, int doy)
{
        int32 start[2];
        int32 edge[2];
        int32 start_out[2];
        int32 edge_out[2];

        int row, col;
        int h, v;

        Point *p1 = &(rModis->upper_left);
        Point *p3 = &(rModis->lower_right);
                
        TilePoint tp1, tp3;
        global2tile(p1, &tp1);
        global2tile(p3, &tp3);

        printf("Tiles: h%02d-%02d, v%02d-%02d.\n", tp1.h, tp3.h, tp1.v, tp3.v);

        //copy sds
        char hdf_in[1024];
        char hdf_sample[1024]; 
        assert(0 == get_hdf_data(p1, year, doy, hdf_sample));
        assert(0 == create_output(hdf_sample, hdf_out, rModis));
        
        for(h=tp1.h; h<=tp3.h; h++){
                for(v=tp1.v; v<=tp3.v; v++){
                        // source
                        // start row
                        if(v==tp1.v){
                                start[0] = tp1.row;
                        }       
                        else{
                                start[0] = 0;
                        }
                        // edge row
                        if(v==tp3.v){
                                edge[0] = tp3.row - start[0] + 1;
                        }
                        else{
                                edge[0] = TILE_NROW - start[0];
                        }

                        //start col
                        if(h==tp1.h){
                                start[1] = tp1.col;
                        }       
                        else{
                                start[1] = 0;
                        }
                        // edge col
                        if(h==tp3.h){
                                edge[1] = tp3.col - start[1] + 1;
                        }
                        else{
                                edge[1] = TILE_NCOL - start[1];
                        }

                        //target
                        edge_out[0] = edge[0];          
                        edge_out[1] = edge[1];          

                        if(v==tp1.v){
                                start_out[0] = 0;
                        }
                        else{
                                start_out[0] = (TILE_NROW-tp1.row)+(v-tp1.v-1)*TILE_NROW;
                        }

                        if(h==tp1.h){
                                start_out[1] = 0;
                        }
                        else{
                                start_out[1] = (TILE_NCOL-tp1.col)+(h-tp1.h-1)*TILE_NCOL;
                        }
                        
                        // submit       
                        /*printf("adding job: h%02dv%02d row [%5d,%5d] col [%5d,%5d] ---> row [%5d,%5d] col [%5d,%5d].\n", 
                                                                h,v,start[0],start[0]+edge[0]-1,start[1],start[1]+edge[1]-1,start_out[0],start_out[0]+edge_out[0]-1,
                                                                start_out[1],start_out[1]+edge_out[1]-1);*/
                        if(0 == _get_hdf_data(h, v, year, doy, hdf_in)){
                                assert(0 == new_job(hdf_in, start, edge, hdf_out, start_out, edge_out));
                        }
                        else if(!fill_flag){
                                sprintf(msg, "Error: h%02dv%02d is missing.\n", h, v);
                                ERRO(msg);
                                return -1;
                        }
                        else{
                                sprintf(msg, "Warning: h%02dv%02d is missing. flag -f detected.\n", h, v);
                                WARN(msg);
                        }
                }
        }

        assert(0 == submit_jobs());
        return 0;
}

int mosaic_cut(char *hdf_out, double lat, double lon, int year, int doy)
{
        Point center;   
        geo2grid(lat, lon, &center);

        Region region;
        make_region(&center, &region);

        TilePoint tp;
        global2tile(&center, &tp);
        printf("CENTER: LAT %f LON %f GLOBAL ROW  = %d GLOBAL COL = %d, TILE=h%02dv%02d, %d, %d\n", 
                                                                        lat, lon, center.row, center.col, tp.h, tp.v, tp.row, tp.col);
        global2tile(&(region.upper_left), &tp);
        printf("Upper left: h%02dv%02d, %d, %d\n", tp.h, tp.v, tp.row, tp.col);
        global2tile(&(region.lower_right), &tp);
        printf("Lower right: h%02dv%02d, %d, %d\n", tp.h, tp.v, tp.row, tp.col);
        //printf("REGION: UPLEFT: %d, %d, LOWRIGHT: %d, %d\n", region.upper_left.row,region.upper_left.col,
        //                                                              region.lower_right.row, region.lower_right.col);

        //copy sds
        char hdf_in[1024];
        char hdf_sample[1024]; 
        assert(0 == get_hdf_data(&center, year, doy, hdf_sample));
        assert(0 == create_output(hdf_sample, hdf_out, &region));

        int r, c;
        printf("Calculating...\n");
        Point *p1 = &(region.upper_left);
        Point *p2 = &(region.upper_right);
        Point *p3 = &(region.lower_right);
        Point *p4 = &(region.lower_left);
                
        TilePoint tp1, tp2, tp3, tp4;
        global2tile(p1, &tp1);
        global2tile(p2, &tp2);
        global2tile(p3, &tp3);
        global2tile(p4, &tp4);

        int32 start[2];
        int32 edge[2];
        int32 start_out[2];
        int32 edge_out[2];

        if(sametile(&tp1, &tp3)){       // single tile
                start[0] = tp1.row;
                start[1] = tp1.col;
                edge[0] = tp3.row - tp1.row + 1;
                edge[1] = tp3.col - tp1.col + 1;

                start_out[0] = 0;
                start_out[1] = 0;
                edge_out[0] = edge[0];
                edge_out[1] = edge[1]; 

                if(0 == get_hdf_data(p1, year, doy, hdf_in)){
                        assert(0 == new_job(hdf_in, start, edge, hdf_out, start_out, edge_out));
                }
                else if(!fill_flag){
                        return -1;
                }
                else{
                        sprintf(msg, "Warning: h%02dv%02d is missing.\n", tp1.h, tp1.v);
                        WARN(msg);
                }
        }
        else if(sametile(&tp1, &tp2)){  // two tiles, up & down
                //top
                start[0] = tp1.row;
                start[1] = tp1.col;
                edge[0] = TILE_NROW - tp1.row;
                edge[1] = tp2.col - tp1.col + 1;

                start_out[0] = 0;
                start_out[1] = 0;
                edge_out[0] = edge[0];
                edge_out[1] = edge[1]; 

                if(0 == get_hdf_data(p1, year, doy, hdf_in)){
                        assert(0 == new_job(hdf_in, start, edge, hdf_out, start_out, edge_out));
                }
                else if(!fill_flag){
                        return -1;
                }
                else{
                        sprintf(msg, "Warning: h%02dv%02d is missing.\n", tp1.h, tp1.v);
                        WARN(msg);
                }

                //bottom
                start[0] = 0; 
                start[1] = tp1.col;
                edge[0] = tp4.row + 1;
                edge[1] = tp2.col - tp1.col + 1;

                start_out[0] = TILE_NROW - tp1.row;
                start_out[1] = 0;
                edge_out[0] = edge[0];
                edge_out[1] = edge[1]; 

                if(0 == get_hdf_data(p4, year, doy, hdf_in)){
                        assert(0 == new_job(hdf_in, start, edge, hdf_out, start_out, edge_out));
                }
                else if(!fill_flag){
                        return -1;
                }
                else{
                        sprintf(msg, "Warning: h%02dv%02d is missing.\n", tp4.h, tp4.v);
                        WARN(msg);
                }
        }
        else if(sametile(&tp1, &tp4)){  // two tiles, left & right
                //left
                start[0] = tp1.row;
                start[1] = tp1.col;
                edge[0] = tp4.row - tp1.row + 1;
                edge[1] = TILE_NCOL - tp1.col;

                start_out[0] = 0;
                start_out[1] = 0;
                edge_out[0] = edge[0];
                edge_out[1] = edge[1]; 

                if(0 == get_hdf_data(p1, year, doy, hdf_in)){
                        assert(0 == new_job(hdf_in, start, edge, hdf_out, start_out, edge_out));
                }
                else if(!fill_flag){
                        return -1;
                }
                else{
                        sprintf(msg, "Warning: h%02dv%02d is missing.\n", tp1.h, tp1.v);
                        WARN(msg);
                }

                //right
                start[0] = tp2.row; 
                start[1] = 0;
                edge[0] = tp4.row - tp1.row + 1;
                edge[1] = tp2.col + 1;

                start_out[0] = 0;
                start_out[1] = TILE_NCOL - tp1.col;
                edge_out[0] = edge[0];
                edge_out[1] = edge[1]; 

                if(0 == get_hdf_data(p2, year, doy, hdf_in)){
                        assert(0 == new_job(hdf_in, start, edge, hdf_out, start_out, edge_out));
                }
                else if(!fill_flag){
                        return -1;
                }
                else{
                        sprintf(msg, "Warning: h%02dv%02d is missing.\n", tp2.h, tp2.v);
                        WARN(msg);
                }
        }
        else{   // four tiles
                //upleft
                start[0] = tp1.row;
                start[1] = tp1.col;
                edge[0] = TILE_NROW - tp1.row;
                edge[1] = TILE_NCOL - tp1.col;

                start_out[0] = 0;
                start_out[1] = 0;
                edge_out[0] = edge[0];
                edge_out[1] = edge[1]; 

                if(0 == get_hdf_data(p1, year, doy, hdf_in)){
                        assert(0 == new_job(hdf_in, start, edge, hdf_out, start_out, edge_out));
                }
                else if(!fill_flag){
                        return -1;
                }
                else{
                        sprintf(msg, "Warning: h%02dv%02d is missing.\n", tp1.h, tp1.v);
                        WARN(msg);
                }

                //lowleft
                start[0] = 0; 
                start[1] = tp1.col;
                edge[0] = tp4.row + 1;
                edge[1] = TILE_NCOL - tp1.col;

                start_out[0] = TILE_NROW - tp1.row;
                start_out[1] = 0;
                edge_out[0] = edge[0];
                edge_out[1] = edge[1]; 

                if(0 == get_hdf_data(p4, year, doy, hdf_in)){
                        assert(0 == new_job(hdf_in, start, edge, hdf_out, start_out, edge_out));
                }
                else if(!fill_flag){
                        return -1;
                }
                else{
                        sprintf(msg, "Warning: h%02dv%02d is missing.\n", tp4.h, tp4.v);
                        WARN(msg);
                }

                //upright
                start[0] = tp2.row;
                start[1] = 0;
                edge[0] = TILE_NROW - tp2.row;
                edge[1] = tp2.col + 1;

                start_out[0] = 0;
                start_out[1] = TILE_NCOL - tp1.col;
                edge_out[0] = edge[0];
                edge_out[1] = edge[1]; 

                if(0 == get_hdf_data(p2, year, doy, hdf_in)){
                        assert(0 == new_job(hdf_in, start, edge, hdf_out, start_out, edge_out));
                }
                else if(!fill_flag){
                        return -1;
                }
                else{
                        sprintf(msg, "Warning: h%02dv%02d is missing.\n", tp2.h, tp2.v);
                        WARN(msg);
                }

                //lowright
                start[0] = 0;
                start[1] = 0;
                edge[0] = tp3.row + 1;
                edge[1] = tp3.col + 1;

                start_out[0] = TILE_NROW - tp1.row;
                start_out[1] = TILE_NCOL - tp1.col;
                edge_out[0] = edge[0];
                edge_out[1] = edge[1]; 

                if(0 == get_hdf_data(p3, year, doy, hdf_in)){
                        assert(0 == new_job(hdf_in, start, edge, hdf_out, start_out, edge_out));
                }
                else if(!fill_flag){
                        return -1;
                }
                else{
                        sprintf(msg, "Warning: h%02dv%02d is missing.\n", tp3.h, tp3.v);
                        WARN(msg);
                }
        }

        assert(0 == submit_jobs());
        return 0;
}

void make_region(Point *center, Region *region)
{
        int r1 = center->row - RADIUS / PIX_SIZE;
        int r2 = center->row + RADIUS / PIX_SIZE;
        int c1 = center->col - RADIUS / PIX_SIZE;
        int c2 = center->col + RADIUS / PIX_SIZE;

        if(r1 < 0) r1 = 0;
        if(r2 < 0) r2 = 0;
        if(c1 < 0) c1 = 0;
        if(c2 < 0) c2 = 0;

        if(r1 >= GLOBAL_NROW) r1 = GLOBAL_NROW-1;
        if(r2 >= GLOBAL_NROW) r2 = GLOBAL_NROW-1;
        if(c1 >= GLOBAL_NCOL) c1 = GLOBAL_NCOL-1;
        if(c2 >= GLOBAL_NCOL) c2 = GLOBAL_NCOL-1;

        region->upper_left.row = r1;
        region->upper_left.col = c1;
        region->upper_right.row = r1;
        region->upper_right.col = c2;
        region->lower_right.row = r2;
        region->lower_right.col = c2;
        region->lower_left.row = r2;
        region->lower_left.col = c1;
}

void add_job(Job *job)
{
        int i;
        for(i=0; i<N_JOB; i++){
                if(strcmp(JOBS[i].fname_s, job->fname_s) == 0 &&
                         strcmp(JOBS[i].fname_t, job->fname_t) == 0){
                        assert(job->start_s[0] == JOBS[i].start_s[0]+JOBS[i].edge_s[0]);
                        assert(job->start_s[1] == JOBS[i].start_s[1]);
                        assert(job->edge_s[1] == JOBS[i].edge_s[1]);

                        assert(job->start_t[0] == JOBS[i].start_t[0]+JOBS[i].edge_t[0]);
                        assert(job->start_t[1] == JOBS[i].start_t[1]);
                        assert(job->edge_t[1] == JOBS[i].edge_t[1]);
                        // exists, merge
                        JOBS[i].edge_s[0] += job->edge_s[0];
                        JOBS[i].edge_t[0] += job->edge_t[0];
                        return;
                }
        }
        // new job
        assert(N_JOB < MAX_JOB);
        memcpy(&(JOBS[N_JOB]), job, sizeof(Job));
        N_JOB ++;
}

int submit_jobs()
{
        int i;
        printf("Total subsets: %d\n", N_JOB);
        for(i=0; i<N_JOB; i++){
                printf("Copy %s:\n  col %4d - %4d\n  row %4d - %4d\n",
                                                                                JOBS[i].fname_s, JOBS[i].start_s[1], JOBS[i].start_s[1] + JOBS[i].edge_s[1]-1,
                                                                                JOBS[i].start_s[0], JOBS[i].start_s[0] + JOBS[i].edge_s[0]-1);
                printf("To   %s:\n  col %4d - %4d\n  row %4d - %4d\n",
                                                                                JOBS[i].fname_t, JOBS[i].start_t[1], JOBS[i].start_t[1] + JOBS[i].edge_t[1]-1,
                                                                                JOBS[i].start_t[0], JOBS[i].start_t[0] + JOBS[i].edge_t[0]-1);
                assert(0 == do_copy_sub_hdf(JOBS[i].fname_s, JOBS[i].start_s, JOBS[i].edge_s, 
                                                                                                                 JOBS[i].fname_t, JOBS[i].start_t, JOBS[i].edge_t));
        }

        return 0;
}
