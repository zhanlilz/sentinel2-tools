#include <stdio.h>
#include <assert.h>
#include "def.h"
#include "hdf.h"
#include "mfhdf.h"
#include "HdfEosDef.h"
#include "proj.h"

int create_output (char *hdf_sample, char *hdf_out, Region * region);

int
main (int argc, char *argv[])
{
	char *hdf_sample = argv[1];
	char *hdf = argv[2];
	int r1 = atoi (argv[3]);
	int c1 = atoi (argv[4]);
	int r2 = atoi (argv[5]);
	int c2 = atoi (argv[6]);

	Region r;
	r.upper_left.row = r1;
	r.upper_left.col = c1;
	r.lower_right.row = r2;
	r.lower_right.col = c2;

	assert (0 == create_output (hdf_sample, hdf, &r));

	return 0;
}

int
copy_field (int32 dstGd, int32 tgtGd, char *fieldName, int nrow, int ncol)
{
	int32 rank;
	int32 dims[10];
	int32 numType;
	char dimList[100];
	int32 compCode;
	intn compParm[10];
	int32 tileCode;
	int32 tileRank;
	int32 tileDims[10];
	int ntSize;
	VOIDP buf;
	int row;
	int32 start[10];
	int32 edge[10];

	int32 fillv = 0;
	//printf("Copying %s...\n", fieldName);

	// copy info
	if (SUCCEED !=
			GDfieldinfo (dstGd, fieldName, &rank, dims, &numType, dimList))
		{
			printf ("Error! Get field info failed: %s, %d\n", fieldName, dstGd);
			return -1;
		}
	if (SUCCEED != GDgetfillvalue (dstGd, fieldName, (VOIDP) & fillv))
		{
			//printf("Error! Get fill value failed: %s, gridid=%d, fillv=%d\n", fieldName, dstGd, fillv);
			//return -1;
			if (numType == DFNT_INT16)
				{
					fillv = 32767;
				}
			else
				{
					fillv = 255;
				}
			printf
				("Warning: Get fill value failed for %s. Using default value: %d\n",
				 fieldName, fillv);
		}

	if (SUCCEED != GDcompinfo (dstGd, fieldName, &compCode, compParm))
		{
			printf ("Error! Inqury comp info failed.\n");
			return -1;
		}
	if (SUCCEED !=
			GDtileinfo (dstGd, fieldName, &tileCode, &tileRank, tileDims))
		{
			printf ("Error! Inqury comp info failed.\n");
			return -1;
		}

	tileDims[0] = 100;
	tileDims[1] = ncol;

	compParm[1] = 8;

	if (SUCCEED !=
			GDdeffield (tgtGd, fieldName, dimList, numType, HDFE_NOMERGE))
		{
			printf ("Error! Definie field %s failed.\n", fieldName);
			return -1;
		}
	if (SUCCEED != GDsetfillvalue (tgtGd, fieldName, &fillv))
		{
			printf ("Error! Set fill value failed: %s, %d\n", fieldName, tgtGd);
			return -1;
		}
	/*if(SUCCEED != GDsettilecomp(tgtGd, fieldName, tileRank, tileDims, compCode, compParm)){
	   printf("Error! Set comp failed.\n");
	   return -1;
	   } */

	return 0;
}

//int copy_field_attr(int32 dstId, int32 tgtId, char *fieldName)
int
copy_field_attr (int32 sdId_dst, int32 sdId_tgt, char *fieldName)
{
	// copy attr
	int i;
	int32 rank;
	int32 dims[2];
	int32 numType;
	int32 hdfFid_dst;
	int32 hdfFid_tgt;
	//int32 sdId_dst;
	//int32 sdId_tgt;
	int32 sdsId_dst;
	int32 sdsId_tgt;
	int32 idx_dst;
	int32 idx_tgt;
	char sdsName[100];
	int32 numAttrs;
	char attrName[100];
	int32 count;
	char attrBuf[MAX_ATTR_BYTES];
	hdf_varlist_t varList[2];
	int32 n_vars;

	/*if(SUCCEED != EHidinfo(dstId, &hdfFid_dst, &sdId_dst)){
	   HEprint(stdout, 0);
	   printf("Error! Get SD ids failed.\n");
	   return -1;
	   }
	   if(SUCCEED != EHidinfo(tgtId, &hdfFid_tgt, &sdId_tgt)){
	   HEprint(stdout, 0);
	   printf("Error! Get SD ids failed.\n");
	   return -1;
	   } */

	idx_dst = SDnametoindex (sdId_dst, fieldName);
	if (idx_dst == FAIL)
		{
			printf ("Error! Name to dst index failed.\n");
			return -1;
		}

	SDgetnumvars_byname (sdId_tgt, fieldName, &n_vars);
	if (n_vars == 1)
		{
			idx_tgt = SDnametoindex (sdId_tgt, fieldName);
		}
	else if (n_vars > 1)
		{
			SDnametoindices (sdId_tgt, fieldName, varList);
			idx_tgt = varList[n_vars - 1].var_index;	// always get the last index, because attr for the privious have already been copied
		}
	else
		{
			idx_tgt = FAIL;
		}

	if (idx_tgt == FAIL)
		{
			printf ("Error! Name to index failed.\n");
			return -1;
		}

	sdsId_dst = SDselect (sdId_dst, idx_dst);
	if (sdsId_dst == FAIL)
		{
			printf ("Error! Select sds failed.\n");
			return -1;
		}
	sdsId_tgt = SDselect (sdId_tgt, idx_tgt);
	if (sdsId_tgt == FAIL)
		{
			printf ("Error! Select sds failed.\n");
			return -1;
		}

	if (SUCCEED !=
			SDgetinfo (sdsId_dst, sdsName, &rank, dims, &numType, &numAttrs))
		{
			printf ("Error! Get info failed.\n");
			return -1;
		}

	for (i = 0; i < numAttrs; i++)
		{
			if (SUCCEED != SDattrinfo (sdsId_dst, i, attrName, &numType, &count))
				{
					printf ("Error! Get attr info failed.\n");
					return -1;
				}
			if (SUCCEED != SDreadattr (sdsId_dst, i, (VOIDP) attrBuf))
				{
					printf ("Error! Read attr failed.\n");
					return -1;
				}
			if (SUCCEED !=
					SDsetattr (sdsId_tgt, attrName, numType, count, (VOIDP) attrBuf))
				{
					printf ("Error! Read attr failed.\n");
					return -1;
				}
		}

	SDendaccess (sdsId_dst);
	SDendaccess (sdsId_tgt);

	return 0;
}

int
create_output (char *hdf_sample, char *hdf_out, Region * region)
{
	int nrow = region->lower_right.row - region->upper_left.row + 1;
	int ncol = region->lower_right.col - region->upper_left.col + 1;

	float64 upleft[2] = { UL_X + PIX_SIZE * region->upper_left.col,
		UL_Y - PIX_SIZE * region->upper_left.row
	};
	float64 lowright[2] =
		{ upleft[0] + ncol * PIX_SIZE, upleft[1] - PIX_SIZE * nrow };

	int32 id = GDopen (hdf_out, DFACC_CREATE);
	assert (id > 0);

	int32 gid = GDcreate (id, "MOD_Grid_BRDF", ncol, nrow, upleft, lowright);
	assert (gid != FAIL);

	assert (SUCCEED == GDdefproj (gid, PROJ_NUM, UTM_ZONE, SPHERE, PROJ_PARAM));

	int32 tmp;
	int32 hdfid;
	assert (SUCCEED == EHidinfo (id, &tmp, &hdfid));
	//add_file(hdf_out, id, gid, hdfid);

	int32 id0, gid0, hdfid0;

	//if(!is_openned(hdf_sample)){
	id0 = GDopen (hdf_sample, DFACC_READ);
	assert (id0 > 0);

	gid0 = GDattach (id0, "MOD_Grid_BRDF");
	assert (gid0 > 0);

	assert (SUCCEED == EHidinfo (id0, &tmp, &hdfid0));

	//add_file(hdf_sample, id0, gid0, hdfid0);
	//}
	//else{
	//  assert(0 == get_id(hdf_sample, &id0, &gid0, &hdfid0));  
	//}

	int32 nFields;
	char fieldList[MAX_FIELDS * 100];
	char fields[MAX_FIELDS][100];
	int32 ranks[MAX_FIELDS];
	int32 numType[MAX_FIELDS];

	int32 nDim;
	int32 dims[10];
	char dimList[500];
	char dim_name[10][50];
	int i, j, k;

	nDim = GDinqdims (gid0, dimList, dims);
	if (nDim < 1)
		{
			//printf("Error! Inq dims failed. %s\n", hdf_sample);
			//return -1;
		}

	j = 0;
	for (i = 0; i < nDim; i++)
		{
			k = 0;
			while (dimList[j] != '\0' && dimList[j] != ',')
				{
					dim_name[i][k++] = dimList[j++];
				}
			dim_name[i][k] = '\0';
			j++;
		}

	for (i = 0; i < nDim; i++)
		{
			GDdefdim (gid, dim_name[i], dims[i]);
		}

	nFields = GDinqfields (gid0, fieldList, ranks, numType);
	if (nFields <= 0)
		{
			printf ("Error! Inqury fields failed.\n");
			return -1;
		}

	j = 0;
	for (i = 0; i < nFields; i++)
		{
			k = 0;
			while (fieldList[j] != '\0' && fieldList[j] != ',')
				{
					fields[i][k++] = fieldList[j++];
				}
			fields[i][k] = '\0';
			j++;
		}

	for (i = 0; i < nFields; i++)
		{
			if (0 != copy_field (gid0, gid, fields[i], nrow, ncol))
				{
					printf ("Error! Copy %s failed. %s\n", fields[i], hdf_sample);
					return -1;
				}
			if (0 != copy_field_attr (hdfid0, hdfid, fields[i]))
				{
					printf ("Error! Copy %s attr failed.\n", fields[i]);
					return -1;
				}
		}

	assert (SUCCEED == GDdetach (gid));
	assert (SUCCEED == GDclose (id));
	assert (SUCCEED == GDdetach (gid0));
	assert (SUCCEED == GDclose (id0));
	return 0;
}
