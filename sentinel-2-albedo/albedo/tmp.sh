#!/bin/bash

ncls=10
mod_dir=/neponset/nbdata07/albedo/Cherski/MODIS
#top_landsat_dir=/neponset/nbdata07/albedo/Cherski/Landsat/LC81040112013191
#top_landsat_dir=/neponset/nbdata07/albedo/Cherski/Run2_segerror/LC81040132015133
#top_landsat_dir=/neponset/nbdata05/albedo/qsun/LandsatDATA/CA-SF1/LT50380222003213-SC20150915224746
top_landsat_dir=/neponset/nbdata07/albedo/Cherski/funkyones/LC81040122015181
out_dir=./output

landsat_dir="${top_landsat_dir}/"


dir_run=`pwd`
cls_exe=${dir_run}/../UnSuperClassfier_UMassBoston/UnsupCls_main.exe
alb_exe=${dir_run}/lndAlbedo_main.exe
sub_exe=/home/qsun/code/mosaic_cut_modis/sub

err=./err_list.txt

xmls=`find $landsat_dir -name "*.xml"`

#for xml in $xmls; do
	xml=/neponset/nbdata07/albedo/Sentinel/S2A_USER_PRD_MSIL2A_PDMC_20151220T205850_R011/GRANULE/S2A_USER_MSI_L2A_TL_SGS__20151220T174150_A002582_T18TWL_N02.01/S2A_USER_MTD_L2A_TL_SGS__20151220T174150_A002582_T18TWL.xml

	short=${xml##*/}
	base=${short%.xml}

	echo "base=$base"

	cls_parm="./runtime/cls_parm_${base}.txt"
	alb_parm="./runtime/alb_parm_${base}.txt"

	cls="${dir}/cls${ncls}_${ncls}_150_${base}.bin"
	dist="${dir}/cls${ncls}_${ncls}_150_${base}.dist"
	alb="${dir}/lndAlbedo_${base}.bin"

	cd $dir_run

	#generate class parameter file
	echo "PARAMETER_FILE" > $cls_parm
	echo "FILE_NAME =${xml}" >> $cls_parm
	echo "OUT_CLSMAP=${cls}" >> $cls_parm
	echo "DISTANCE_FILE=${dist}" >> $cls_parm
	echo "NBANDS = 6" >> $cls_parm
	echo "NCLASSES =${ncls}" >> $cls_parm
	echo "THRS_RAD =0.150" >> $cls_parm
	echo "I_CLUSTER =${ncls}" >> $cls_parm
	echo "END." >> $cls_parm

  #generate albedo parameter file
	echo "#define parameters for input TM class map and SR file" > $alb_parm
	echo "DM_CLSMAP=${cls}" >> $alb_parm
	echo "DM_FILE_NAME=${xml}" >> $alb_parm
	echo "DM_NBANDS=6" >> $alb_parm
	echo "DM_NCLS=${ncls}" >> $alb_parm
	echo "CLS_FILL_VALUE=-9" >> $alb_parm
	echo "" >> $alb_parm
	echo "# MODIS input file" >> $alb_parm
	echo "MODIS_BRDF_FILE=${mod_a1}" >> $alb_parm
	echo "MODIS_QA_FILE=${mod_a2}" >> $alb_parm
	echo "" >> $alb_parm
	echo "AMONGclsDIST_FILE_NAME=${dist}" >> $alb_parm
	echo "OUTPUT_ALBEDO =${alb}" >> $alb_parm
	echo "END" >> $alb_parm

  #run

	#if [ ! -r $alb ]; then
		echo "Calculating albedo ..."
		#$alb_exe $alb_parm >> $log
		$alb_exe $alb_parm 
		#if [ $? -ne 0 ]; then
		if [ ${PIPESTATUS[0]} -ne 0 ]; then
			echo "ERROR"
			#exit
			echo "$log failed: calculating albedo." >> $err
			continue
		fi
	#fi
	
#done

echo "DONE."

