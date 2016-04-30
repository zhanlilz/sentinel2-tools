#ifndef __INC_H_DEF_
#define __INC_H_DEF_

#define RADIUS (500000)
//#define RADIUS (1000000)

#define MAX_FIELDS 100
#define MAX_ATTR_BYTES 40960

// proj parameters
#define PROJ_NUM SNSOID
#define SPHERE -1
#define UL_X -20015109.354
#define UL_Y 10007554.677
#define PIX_SIZE 463.3127165277778
double PROJ_PARAM[15]  = {6371007.181,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
#define UTM_ZONE -1

#define TILE_SIZE 1111950.519666667
#define	TILE_NROW ((int)(TILE_SIZE / PIX_SIZE))
#define	TILE_NCOL ((int)(TILE_SIZE / PIX_SIZE))
#define GLOBAL_NROW (TILE_NROW * 18)
#define	GLOBAL_NCOL (TILE_NCOL * 36)

typedef struct{
	int row;
	int col;
}Point;

typedef struct{
	Point upper_left;
	Point upper_right;
	Point lower_right;	
	Point lower_left;	
}Region;

typedef struct{
	int h;
	int v;
	int row;
	int col;
}TilePoint;

#endif
