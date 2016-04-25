/* 
 * !Description
 * Ross-Thick-Li-Sparse Receiprocal BRDF model 
 *	
 * !Usage
 *   parseParameters (filename, ptr_LndSensor, ptr_MODIS)
 * 
 * !Developer
 *  Initial version (V1.0.0) Yanmin Shuai (Yanmin.Shuai@nasa.gov)
 *						and numerous other contributors
 *						(thanks to all)
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
 * Please contact us with a simple description of your application, if you have interests.
 */
#include "lndsr_albedo.h"

/*****************************************************************/
void
GetPhaseAngle (
		 double cos1,
		 double cos2,
		 double sin1,
		 double sin2,
		 double cos3,
		 double *cosres,
		 double *res,
		 double *sinres)
/*
!C****************************************************************************

!Description:
This function computes the phase angle between viewing and
illumination direction.

!Input Parameters:
cos1       Cosine of the view zenith angle.
cos2       Cosine of the sun zenith angle.
sin1       Sine of the view zenith angle.
sin2       Sine of the sun zenith angle.
cos3       Cosince of the relative azimuth angle.

!Output Parameters:
cosres     Pointer to the cosine of the phase angle.
res        Pointer to the phase angle.
sinres     Pointer to the sine of the phase angle.

!Returns:
The function returns an int status qualifying success.

!END**************************************************************************
*/
{
   *cosres = cos1 * cos2 + sin1 * sin2 * cos3;
   *res = acos (MAX (-1., MIN (1., *cosres)));
   *sinres = sin (*res);

   return;
}

/*--------------------------------------------------------------------*/

void
GetDistance (
	       double tan1,
	       double tan2,
	       double cos3,
	       double *res)
/*
!C****************************************************************************

!Description:
This functions calculates the distance parameter used in
many geometric-optical models; this parameter is the dimensionless
distance factor between the centers of the ground-projected
illumination and viewing shadow centers.

!Input Parameters:
tan1       Tangens of the angle corresponding to view zenith.
tan2       Tangens of the angle corresponding to sun zenith.
cos3       Cosine of the relative azimuth angle.


!Output Parameters:
res        Distance parameter for use in geometric-optial models.

!Returns:
The function returns an int status qualifying success.

!Design Notes
!END**************************************************************************
*/
{
   double temp = 0.;

   temp = tan1 * tan1 + tan2 * tan2 - 2. * tan1 * tan2 * cos3;
   *res = sqrt (MAX (0., temp));

   return;
}

/*--------------------------------------------------------------------*/

void
GetpAngles (
	      double brratio,
	      double tan1,
	      double *sinp,
	      double *cosp,
	      double *tanp)
/*
!C****************************************************************************

!Description:
This function transforms real zenith angles to the zenith angles
that would be needed to create the same amount of shadowing on
the ground if the spheroids representing the crowns were spheres
(primed zeniths).

!Input Parameters:
brratio    Ratio of vertical to horizontal crown radius.
tan1       Tangens of the zenith angle in question.

!Output Parameters:
sinp       Pointer to the sine of the transformed (primed) zenith angle.
cosp       Pointer to the cosine of the transformed (primed) zenith angle.
tanp       Pointer to the tangens of the transformed (primed) zenith angle.

!Returns:
The function returns an int status qualifying success.

!END************************************************************************ */
{
   double angp = 0.;

   *tanp = brratio * tan1;
   if (*tanp < 0)
      *tanp = 0.;
   angp = atan (*tanp);
   *sinp = sin (angp);
   *cosp = cos (angp);

   return;
}

/*--------------------------------------------------------------------*/

void
GetOverlap (
	      double hbratio,
	      double distance,
	      double cos1,
	      double cos2,
	      double tan1,
	      double tan2,
	      double sin3,
	      double *overlap,
	      double *temp)
/*
!C****************************************************************************

!Description:
This function calculates the average overlap area between
ground-projected illumination and viewing shadows for use
in the Li kernels.

!Input Parameters:
hbratio    Ratio of height-to-center of crown to horizontal crown radius.
distance   Distance parameter.
cos1       Cosine of angle correpsonding to view zenith.
cos2       Cosine of angle correpsonding to sun zenith.
tan1       Tangens of angle correpsonding to view zenith.
tan2       Tangens of angle correpsonding to sun zenith.
sin3       Sine of relative azimuth angle.

!Output Parameters:
overlap    Pointer to the overlap area of the shadows.
temp       Pointer to a geometric term needed later.

!Returns:
The function returns an int status qualifying success.

!END**************************************************************************
*/
{
   double cost = 0., sint = 0., tvar = 0.;
   double pi=3.1415926535;
   
   *temp = 1. / cos1 + 1. / cos2;
   cost = hbratio * sqrt (distance * distance + tan1 * tan1 * tan2 * tan2 * sin3 * sin3) /
      *temp;
   cost = MAX (-1., MIN (1., cost));
   tvar = acos (cost);
   sint = sin (tvar);
   *overlap = 1. / pi * (tvar - sint * cost) * (*temp);
   *overlap = MAX (0., *overlap);

   return;
}

double RossThick_kernel(double tv, double ti, double phi)
/*
units of three input angles: radiance 

*/
{
   double costv = -999.;
   double sintv = -999.;
   double costi = -999.;
   double sinti = -999.;
   double cosphi = 0.;
   double sinphi = 0.;
   
   double phaang = 0.;
   double cosphaang = 0.;
   double sinphaang = 0.;
   double rosselement = 0.;
   
  double kernelV;
  double pi=3.1415926535;
	
   cosphi=cos(phi);
   sinphi=sin(phi);
   costv=cos(tv);
   sintv=sin(tv);
   costi=cos(ti);
   sinti=sin(ti);

  GetPhaseAngle(costv, costi,sintv,sinti,cosphi,&cosphaang,&phaang, &sinphaang);

   rosselement = (pi/ 2. - phaang) * cosphaang + sinphaang;

   kernelV=rosselement / (costi + costv) - pi / 4.;

   return kernelV;

}


double LiSparseR_kernel(double tv, double ti, double phi)
{
   float brratio=1.0,hbratio=2.0;
   double tantv = 0.,tanti = 0.;
   double cosphi=0.,sinphi=0.;

   double sintvp = 0., costvp = 0., tantvp = 0., sintip = 0., costip = 0., tantip = 0.;
   double phaangp = 0., cosphaangp = 0., sinphaangp = 0., distancep = 0.;
   double overlap = 0., temp = 0.;

   double kernelV;

   cosphi=cos(phi);
   sinphi=sin(phi);
   
    if(cos(tv)!=0)
     		tantv = sin(tv)/cos(tv);
    
     if(cos(ti)!=0)
     		tanti = sin(ti)/cos(ti);

   GetpAngles (brratio, tantv, &sintvp, &costvp, &tantvp);
   GetpAngles (brratio, tanti, &sintip, &costip, &tantip);
   GetPhaseAngle (costvp, costip, sintvp, sintip, cosphi, &cosphaangp, &phaangp, &sinphaangp);
   GetDistance (tantvp, tantip, cosphi, &distancep);
   GetOverlap (hbratio, distancep, costvp, costip, tantvp, tantip, sinphi, &overlap, &temp);

   kernelV=overlap - temp + 1. / 2. * (1. + cosphaangp) / costvp / costip;

   return kernelV;

}
