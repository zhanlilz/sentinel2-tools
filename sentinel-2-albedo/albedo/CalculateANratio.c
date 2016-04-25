/*
 * !Description
 * Calculate the spectral Albedo-to-directional reflectance ratios. 
 *		
 *	
 * !Usage
 *   parseParameters (filename, ptr_LndSensor, ptr_MODIS)
 * 
 * !Developer
 *  Initial version (V1.0.0): Yanmin Shuai (Yanmin.Shuai@nasa.gov)
 *					    and numerous other contributors
 *					     (thanks to all)
 *
 * !Revision History
 * 
 *
 *!Team-unique header:
 * This system (research version) was developed for NASA TE project "Albedo  
 *	   Trends Related to Land CoverChange and Disturbance: A Multi-sensor   
 *	   Approach", and is adopted by USGS Landsat science team (2012-2017) 
 *	   to produce the long-term land surface albedo dataset over North America
 *	   at U-Mass Boston (PI: Crystal B. Schaaf, Co-Is:Zhongping Lee and Yanmin Shuai). 
 * 
 * !References and Credits
 * Principal Investigator: Jeffrey G. masek (Jeffrey.G.Masek@nasa.gov)
 *					 Code 618, Biospheric Science Laboratory, NASA/GSFC.
 *
 * Please contact us with a simple description of your application, if you have any interest.
 */
 
#include "lndsr_albedo.h"

/*******************************************************/
int CalculateANratio(int16 *brdf, double SZA,double SAA,double VZA, double VAA, double *ANR_wsa, double *ANR_bsa)
{
	 double wsa,wsa_weight[3]={1.0,0.189184,-1.377622};
	 double bsa;
	 double bsk_RossThick[91]={ -0.021079, -0.021026, -0.020866, -0.020598, -0.020223, -0.019740, -0.019148, -0.018447, -0.017636, -0.016713, 
	 	                                       -0.015678, -0.014529, -0.013265, -0.011883, -0.010383, -0.008762, -0.007018, -0.005149, -0.003152, -0.001024,
	 	                                         0.001236,   0.003633, 0.006170,   0.008850,   0.011676,  0.014653,   0.017785,  0.021076,   0.024531,   0.028154, 
	 	                                         0.031952,   0.035929, 0.040091,   0.044444,   0.048996,  0.053752,   0.058720,  0.063908,   0.069324,   0.074976,
	 	                                         0.080873,   0.087026, 0.093443,   0.100136,   0.107117,  0.114396,   0.121987,  0.129904,   0.138161,   0.146772,
	 	                                         0.155755,   0.165127, 0.174906,   0.185112,   0.195766,  0.206890,   0.218509,  0.230649,   0.243337,   0.256603,
	 	                                         0.270480,   0.285003, 0.300209,   0.316139,   0.332839, 0.350356,    0.368744,  0.388061,   0.408372,   0.429747,
	 	                                         0.452264,   0.476012, 0.501088,   0.527602,   0.555677, 0.585457,    0.617102,  0.650800,   0.686769,   0.725269,
	 	                                         0.766607,   0.811157, 0.859381,   0.911864,   0.969367, 1.032915,    1.103966,  1.184742,   1.279047,   1.394945,
	 	                                         1.568252 };
	 double bsk_LiSparseR[91]={-1.287889, -1.287944, -1.288058, -1.288243, -1.288516, -1.288874, -1.289333, -1.289877, -1.290501, -1.291205, 
	 						   -1.291986, -1.292855, -1.293812, -1.294861, -1.295982, -1.297172, -1.298447, -1.299805, -1.301228, -1.302730,
	 						   -1.304319, -1.306023, -1.307829, -1.309703, -1.311615, -1.313592, -1.315674, -1.317847, -1.320090, -1.322396,
	 						   -1.324763, -1.327212, -1.329779, -1.332436, -1.335150, -1.337899, -1.340710, -1.343621, -1.346625, -1.349686,
	 						   -1.352780, -1.355925, -1.359123, -1.362429, -1.365825, -1.369290, -1.372774, -1.376264, -1.379825, -1.383465,
	 						   -1.387144, -1.390834, -1.394543, -1.398269, -1.402071, -1.405931, -1.409818, -1.413691, -1.417533, -1.421378,
	 						   -1.425284, -1.429204, -1.433052, -1.436887, -1.440693, -1.444474, -1.448251, -1.452013, -1.455691, -1.459185,
	 						   -1.462709, -1.466247, -1.469651, -1.472869, -1.476028, -1.479219, -1.482235, -1.485117, -1.487871, -1.490654,
	 						   -1.493254, -1.495838, -1.498365, -1.500982, -1.503707, -1.506758, -1.510596, -1.516135, -1.526204, -1.555139,
	 						   -37286.245624 };
	 double ref_dir;
	 double k_iso,k_geo,k_vol;
	 double brdf_scale=0.001;
	 int thisang;
	 double angdis;
	 double bs_LSRKer, bs_RTKer;
	 
	thisang=(int)(NUMBER_BS_KERNELALBEDOS-1)/90.*SZA	;
	angdis=(SZA-thisang)/((NUMBER_BS_KERNELALBEDOS-1)/90);
	bs_LSRKer=bsk_LiSparseR[thisang-1]+(bsk_LiSparseR[thisang]-bsk_LiSparseR[thisang-1])*angdis;
	bs_RTKer=bsk_RossThick[thisang-1]+(bsk_RossThick[thisang]-bsk_RossThick[thisang-1])*angdis;
 	
	double phi = VAA - SAA;
	if(phi < 0.0){
		phi += 360.0;
	}
	 if(phi > 360.0){
		phi -= 360.0;
	}
	if(phi > 180.0){
		phi = 360.0 - phi;
	}
	phi *= D2R;

	 SZA=SZA*D2R;
	 VZA=VZA*D2R;
	 wsa=brdf_scale*(wsa_weight[0]*brdf[0]+wsa_weight[1]*brdf[1]+wsa_weight[2]*brdf[2]);
	 bsa=brdf_scale*(1.0*brdf[0]+bs_RTKer*brdf[1]+ bs_LSRKer*brdf[2]);

	 k_iso=1.0;
	 k_geo=LiSparseR_kernel(VZA, SZA, phi);
	 k_vol=RossThick_kernel(VZA, SZA, phi);
	 ref_dir=brdf_scale*(k_iso*brdf[0]+k_vol*brdf[1]+k_geo*brdf[2]);

	 double _ANR_bsa = bsa/ref_dir;
	 double _ANR_wsa=wsa/ref_dir;
	 
	 /*sqs debug */
	 if(_ANR_bsa>4|| _ANR_wsa>4){
		 printf("ANR_BSA=%f,ANR_WSA=%f,SZA=%f,VZA=%f,WSA=%f,BSA=%f,NBAR=%f,BRDF=%d,%d,%d,KER=%f,%f,%f\n",
						_ANR_bsa,_ANR_wsa, SZA*R2D, VZA*R2D, wsa, bsa, ref_dir,brdf[0],brdf[1],brdf[2],k_iso,k_geo,k_vol);
	 }

	*ANR_bsa=_ANR_bsa;
	*ANR_wsa=_ANR_wsa;

	 return SUCCESS;
}
