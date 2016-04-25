#!/bin/bash

ncls=10
mod_dir=/neponset/nbdata05/albedo/qsun/V6_OUT/selected_for_yanmin
top_landsat_dir=/neponset/nbdata05/albedo/qsun/LandsatDATA

for sub_d in {"CA-SF1","US-BO2","US-Bar","US-Br3","US-FPe","US-PIE","US-RO1","US-Slt","US-UMB"}; do

landsat_dir="${top_landsat_dir}/${sub_d}"

dir_run=`pwd`
cls_exe=${dir_run}/../UnSuperClassfier_UMassBoston/UnsupCls_main.exe
alb_exe=${dir_run}/lndAlbedo_main.exe
sub_exe=/home/qsun/code/mosaic_cut_modis/sub

err=./err_list.txt

for tar in ${landsat_dir}/*.tar.gz; do
	short=${tar##*/}
	base=${short%.tar.gz}

	dir=${landsat_dir}/${base}
	if [ ! -r ${dir} ]; then
		echo "Unzipping $tar ..."
		mkdir -p ${dir}
		cd $dir
		tar zxf $tar
		if [ $? -ne 0 ]; then
			echo "unzip $tar failed."
			echo "$tar failed." >> $err
			#exit 1
			continue
		fi
		cd $dir_run
	fi

	xml=`find $dir -name "*.xml"`
	if [ "$xml" == "" ] || [ ! -r "$xml" ]; then
		echo "XML not exist in $dir"
		echo "$xml failed." >> $err
		#exit 1
		continue
	fi

	log="${dir}/${base}.log.txt"

	mod_a1="${dir}/${base}.modis.a1.hdf"
	mod_a2="${dir}/${base}.modis.a2.hdf"

	if [ ! -r $mod_a1 ]; then
		echo "Subsetting mcd43a1 ..."
		$sub_exe $mod_dir "MCD43A1" $xml $mod_a1 >> $log
		#if [ $? -ne 0 ]; then
		if [ ${PIPESTATUS[0]} -ne 0 ]; then
			echo "Get mcd43a1 subset failed." 
			#exit 1
			echo "$log failed: sub mcd43a1." >> $err
			continue
		fi
	fi
	
	if [ ! -r $mod_a2 ]; then
		echo "Subsetting mcd43a2 ..."
		$sub_exe $mod_dir "MCD43A2" $xml $mod_a2 >> $log
		#if [ $? -ne 0 ]; then
		if [ ${PIPESTATUS[0]} -ne 0 ]; then
			echo "Get mcd43a2 subset failed."
			#exit 1
			echo "$log failed: sub mcd43a2." >> $err
			continue
		fi
	fi

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
	if [ ! -r $cls ]; then
		echo "Classifying ..."
		$cls_exe $cls_parm >> $log
		#if [ $? -ne 0 ]; then
		if [ ${PIPESTATUS[0]} -ne 0 ]; then
			echo "ERROR"
			#exit
			echo "$log failed: classification." >> $err
			continue
		fi
	else
		echo "Warning: classification file already exists, skip classification." >> $log
	fi

	#if [ ! -r $alb ]; then
		echo "Calculating albedo ..."
		$alb_exe $alb_parm >> $log
		#if [ $? -ne 0 ]; then
		if [ ${PIPESTATUS[0]} -ne 0 ]; then
			echo "ERROR"
			#exit
			echo "$log failed: calculating albedo." >> $err
			continue
		fi
	#fi
	
done
done

echo "SUCCEED."

