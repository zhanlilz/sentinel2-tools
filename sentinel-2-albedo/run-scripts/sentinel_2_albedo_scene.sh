#!/usr/bin/env bash
#
# sentinel_2_albedo_scene.sh
#
# Generate land surface albedo for selected granules of one Sentinel 2
# scene (L2A product) using Yanmin Shuai's Landsat albedo
# algorithm. This script is changed based on Qingsong's original
# script.
#
# Zhan Li, zhan.li@umb.edu
# Created: Tue Apr 26 21:01:51 EDT 2016

read -d '' USAGE <<EOF
sentinel_2_albedo_scene.sh [options] SENTINEL_SAFE

Generate land surface albedo for selected granules of the Sentinel
scene in the SAFE data folder of L2A product, SENTINEL_SAFE.

Options

  -m, --mcd43, required, directory of MODIS MCD43 product files.

  -o, --output-directory, required, output albedo data in the given
   directory; it CANNOT be the same directory as where the input SAFE
   data is.

  -g, --granule, optional, selected granule IDs (UTM MGRS grid)
   delimited by comma ','; e.g., -g"13TDE,12SBC", or,
   --granule="13TDE,12SBC"; if not provided, process all granules in
   this scene.

  -s, --snow, optional, turn on processing for snow pixel; default is
   to only process snow-free pixels.

EOF

# number of clusters in the unsupervised classification of land covers.
ncls=10
# where we call this script
dir_run=`pwd`
# where are the executables
cmd_dir="/home/zhan.li/Workspace/src-programs/sentinel-2-tools/sentinel-2-albedo/albedo"
# where are the executable for subset modis data
# sub_exe="/home/qsun/code/mosaic_cut_modis/sub2"
sub_exe="/home/zhan.li/Workspace/src-programs/sentinel-2-tools/sentinel-2-albedo/mosaic_cut_modis/sub2"
# err log file
err=${dir_run}/err_list.log

SNOW=0
OPTS=`getopt -o m:o:g::s --long mcd43:,output-directory:,granule::,snow -n 'sentinel_2_albedo_scene.sh' -- "$@"`
if [[ $? != 0 ]]; then echo "Failed parsing options" >&2 ; echo "${USAGE}" ; exit 1 ; fi
eval set -- "${OPTS}"
while true;
do
    case "${1}" in
        -m | --mcd43 )
            case "${2}" in
                "") shift 2 ;;
                *) mod_dir=${2} ; shift 2 ;;
            esac ;;
        -o | --output-directory )
            case "${2}" in
                "") shift 2 ;;
                *) out_dir_top=${2} ; shift 2 ;;
            esac ;;
        -g | --granule )
            case "${2}" in
                "") shift 2 ;;
                *) granules=${2} ; shift 2 ;;
            esac ;;
        -s | --snow )
            case "${2}" in
                "") shift 2 ;;
                *) SNOW=1 ; shift ;;
            esac ;;
        -- ) shift ; break ;;
        * ) break ;;
    esac
done
MINPARAMS=1
if [[ ${#} < ${MINPARAMS} ]]; then
    echo "Missing positional arguments"
    echo "${USAGE}"
    exit 1
fi
L2ADIR=${1}

if [[ ${SNOW} -eq 0 ]]; then
    cls_exe="${cmd_dir}/../unsup-cls/UnsupCls_main_snow_free.exe"
    alb_exe="${cmd_dir}/lndAlbedo_main_snow_free.exe"
    echo "Snow-free albedo by running program"
else
    cls_exe="${cmd_dir}/../unsup-cls/UnsupCls_main_snow_incl.exe"
    alb_exe="${cmd_dir}/lndAlbedo_main_snow_incl.exe"
    echo "Snow-included albedo by running program"
fi
echo "Classification: ${cls_exe}"
echo "Albedo Calulation: ${alb_exe}"

# # directory of MCD43A1*.hdf (QA flags) and MCD43A2*.hdf (BRDF data)
# mod_dir=/neponset/nbdata06/albedo/zhan.li/modis/mcd43-v6
# # directory of output data
# out_dir_top=/neponset/nbdata06/albedo/zhan.li/sentinel-2/albedo

In_Array ()
{
    local id=${1}
    local list=($(echo "${2}"))
    # echo ${id}
    local i
    for (( i=0; i<${#list[@]}; ++i ));
    do
        if [[ ${list[i]^^} == ${id} ]]; then
            return 1
        fi
    done
    return 0
}

if [[ ! -z ${granules} ]]; then
    echo "Selected granules: ${granules}"
    while IFS= read -r -d ',' tmp;
    do
        gr_list+=("$(echo ${tmp} | tr -d ' ')")
    done < <(echo "${granules},")

    ALL_GRS=($(find "${L2ADIR}/GRANULE/" -maxdepth 1 -type d -name "S*L2A*"))
    for (( i=0; i<${#ALL_GRS[@]}; ++i )); 
    do
        gname=${ALL_GRS[i]}
        id=$(basename ${gname} | rev | cut -d '_' -f2 | rev)
        # cut the "T" at the beginning
        id=${id:1}
        In_Array "${id}" "$(echo ${gr_list[@]})"
        if [[ $? == 1 ]]; then
            grdirs_todo+=(${gname})
        fi
    done
else
    grdirs_todo=($(find "${L2ADIR}/GRANULE/" -maxdepth 1 -type d -name "S*L2A*"))
fi

num_grdirs=${#grdirs_todo[@]}
if [[ ${num_grdirs} -le 0 ]]; then
    echo "No granules available. Check your granule IDs and GRANULE subdirectory of the input SAFE folder"
    exit 1
fi

if [[ ! -d ./runtime ]]; then
    mkdir -p ./runtime
fi

errlog="./runtime/sentinel_2_albedo_scene.err.log"

find_mcd43 ()
{
# find_mcd43 out_mod_a1_name out_mod_a2_name n_xml xml_1 xml_2 ... xml_n

    # find modis
    mod_a1=${1}
    mod_a2=${2}
    shift 2
    xml_args=${@}

    if [ ! -r $mod_a1 ]; then
        echo "Mosaic MCD43A1 ..."
        echo $sub_exe $mod_dir MCD43A1 $mod_a1 ${xml_args}
        $sub_exe $mod_dir MCD43A1 $mod_a1 ${xml_args}
        if [ $? -ne 0 ]; then
            echo "mosaic MCD43A1 failed"
            rm -f $mod_a1
            echo "MOSAIC1_FAIL ${mod_a1}" >> ${errlog}
            return 1
        fi
    fi
    if [ ! -r $mod_a2 ]; then
        echo "Mosaic MCD43A2 ..."
        echo $sub_exe $mod_dir MCD43A2 $mod_a2 ${xml_args}
        $sub_exe $mod_dir MCD43A2 $mod_a2 ${xml_args}
        if [ $? -ne 0 ]; then
            echo "mosaic MCD43A2 failed"
            rm -f $mod_a2
            echo "MOSAIC2_FAIL ${mod_a2}" >> ${errlog}
            return 1
        fi
    fi
    return 0
}

l2a_name=$(basename ${L2ADIR})
for (( i=0; i<${num_grdirs}; ++i ));
do
    granule_name=$(basename ${grdirs_todo[i]})
    echo "Albedo <---- L2A, granule $((i+1)) / ${num_grdirs} in ${L2ADIR}, ${granule_name}"
    xml=($(find "${grdirs_todo[i]}" -maxdepth 1 -name "S*L2A*.xml"))
    if [[ ${#xml[@]} -gt 1 ]]; then
        echo "More than one L2A granule XML file are found. Ambiguous! Skip ${grdirs_todo[i]}"
        continue
    fi
    xml=${xml[0]}

    out_dir="${out_dir_top}/${l2a_name}/${granule_name}"
    if [[ ! -d ${out_dir} ]]; then
        mkdir -p "${out_dir}"
    fi

    echo "${out_dir}"

    # start generating albedo

    # base=${xml##*/}
    # base=${base%.*}
    base=$(basename ${xml} '.xml')

    cls_parm=./runtime/cls_parm_${base}.txt
    alb_parm=./runtime/alb_parm_${base}.txt

    # #select MODIS AOD data for blue sky albedo
    # year=${base:9:4}
    # if [ ${base:13:1} == "0" ]; then
    #         doy=${base:14:2}
    # else
    #         doy=${base:13:3}
    # fi
    # doy=${doy##0}
    # doy=${doy##0}
    # doy=`printf %03d $doy`

    # echo "year=$year doy=$doy"

    # day8=${doy##0}
    # day8=${day8##0}
    # day8=$((${day8}/8*8+1))
    # day8=`printf %03d $day8`

    # daym=${doy##0}
    # if [ $daym -ge 335 ]; then
    #         daym=335
    # elif [ $daym -ge 305 ]; then
    #         daym=305
    # elif [ $daym -ge 274 ]; then
    #         daym=274
    # elif [ $daym -ge 244 ]; then
    #         daym=244
    # elif [ $daym -ge 213 ]; then
    #         daym=213
    # elif [ $daym -ge 182 ]; then
    #         daym=182
    # elif [ $daym -ge 152 ]; then
    #         daym=152
    # elif [ $daym -ge 121 ]; then
    #         daym=121
    # elif [ $daym -ge 91 ]; then
    #         daym=091
    # elif [ $daym -ge 60 ]; then
    #         daym=060
    # elif [ $daym -ge 32 ]; then
    #         daym=032
    # else
    #         daym=001
    # fi

    # echo "doy=$doy 8-day=$day8 monthly=$daym"

    # aod1=`find ${aod_dir}/MOD08_D3/${year}/ -name "MOD08_D3.A${year}${doy}.051.*.hdf"`
    # if [ -z "$aod1" ] || [ "$aod1" == "" ] || [ ! -r "$aod1" ]; then
    #         echo "aod1 not found: $aod1"
    # fi
    # aod2=`find ${aod_dir}/MYD08_D3/${year}/ -name "MYD08_D3.A${year}${doy}.051.*.hdf"`
    # if [ -z "$aod2" ] || [ "$aod2" == "" ] || [ ! -r "$aod2" ]; then
    #         echo "aod2 not found: $aod2"
    # fi
    # aod3=`find ${aod_dir}/MOD08_E3/${year}/ -name "MOD08_E3.A${year}${day8}.051.*.hdf"`
    # if [ -z "$aod3" ] || [ "$aod3" == "" ] || [ ! -r "$aod3" ]; then
    #         echo "aod3 not found: $aod3"
    # fi
    # aod4=`find ${aod_dir}/MYD08_E3/${year}/ -name "MYD08_E3.A${year}${day8}.051.*.hdf"`
    # if [ -z "$aod4" ] || [ "$aod4" == "" ] || [ ! -r "$aod4" ]; then
    #         echo "aod4 not found: $aod4"
    # fi
    # aod5=`find ${aod_dir}/MOD08_M3/${year}/ -name "MOD08_M3.A${year}${daym}.051.*.hdf"`
    # if [ -z "$aod5" ] || [ "$aod5" == "" ] || [ ! -r "$aod5" ]; then
    #         echo "aod5 not found: $aod5"
    # fi
    # aod6=`find ${aod_dir}/MYD08_M3/${year}/ -name "MYD08_M3.A${year}${daym}.051.*.hdf"`
    # if [ -z "$aod6" ] || [ "$aod6" == "" ] || [ ! -r "$aod6" ]; then
    #         echo "aod6 not found: $aod6"
    # fi

    # mosaic MCD43 for this Landsat scene
    mod43a1=${out_dir}/MCD43A1_FOR_${base}.hdf
    mod43a2=${out_dir}/MCD43A2_FOR_${base}.hdf

    find_mcd43 "${mod43a1}" "${mod43a2}" 1 "${xml}"
    if [[ $? -ne 0 ]]; then
        echo "Mosaicing MCD43 for this scene ${base} failed!"
        continue
    fi

    echo "Using $mod43a1"
    echo "Using $mod43a2"

    cls=${out_dir}/cls${ncls}_${ncls}_150_${base}.bin
    dist=${out_dir}/cls${ncls}_${ncls}_150_${base}.dist
    alb=${out_dir}/lndAlbedo_${base}_n2b.bin

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
    echo "MODIS_BRDF_FILE=${mod43a1}" >> $alb_parm
    echo "MODIS_QA_FILE=${mod43a2}" >> $alb_parm
    echo "" >> $alb_parm
    echo "AMONGclsDIST_FILE_NAME=${dist}" >> $alb_parm
    echo "OUTPUT_ALBEDO =${alb}" >> $alb_parm
    if [ -r "$aod1" ]; then
            echo "MOD08_D3 =${aod1}" >> $alb_parm
    fi
    if [ -r "$aod2" ]; then
            echo "MOD08_D3 =${aod2}" >> $alb_parm
    fi
    if [ -r "$aod3" ]; then
            echo "MOD08_D3 =${aod3}" >> $alb_parm
    fi
    if [ -r "$aod4" ]; then
            echo "MOD08_D3 =${aod4}" >> $alb_parm
    fi
    if [ -r "$aod5" ]; then
            echo "MOD08_D3 =${aod5}" >> $alb_parm
    fi
    if [ -r "$aod6" ]; then
            echo "MOD08_D3 =${aod6}" >> $alb_parm
    fi
    echo "END" >> $alb_parm

    #run
#        if [ ! -r $cls ]; then
            echo $cls_exe $cls_parm
            $cls_exe $cls_parm
            if [ $? -ne 0 ]; then
                    echo "CLASSIFICATION ERROR"
                    # clean bad classification files
                    rm -rf ${out_dir}/cls*
                    # clean up temporary MCD43 mosaic
                    rm -rf ${mod43a1} ${mod43a2}
                    continue
            fi
        # else
        #         echo "Warning: classification file already exists, skip classification."
        # fi

    #if [ ! -r $alb ]; then
            echo $alb_exe $alb_parm
            $alb_exe $alb_parm
    #else
    #   echo "ALREADY EXIST: $alb, SKIP"
    #fi
    if [ $? -ne 0 ]; then
            echo "LNDALBEDO ERROR"
            # clean up temporary MCD43 mosaic
#            rm -rf ${mod43a1} ${mod43a2}
            continue
    fi

    # clean up temporary MCD43 mosaic
    rm -rf ${mod43a1} ${mod43a2}
    
done


# #xmls=`find $landsat_dir -name "*.xml"`

# #for xml in $xmls; do
#         #xml=/neponset/nbdata07/albedo/Sentinel/S2A_USER_PRD_MSIL2A_PDMC_20151220T205850_R011/GRANULE/S2A_USER_MSI_L2A_TL_SGS__20151220T174150_A002582_T18TWL_N02.01/S2A_USER_MTD_L2A_TL_SGS__20151220T174150_A002582_T18TWL.xml

# # xml file of the sentinel-2 tile (100 km by 100 km, NOT the scene) to be processed.
# # e.g.: xml=[data_dir]/S2A_USER_PRD_MSIL2A_PDMC_20150818T101216_R065_V20150806T102902_20150806T102902.SAFE/GRANULE/S2A_USER_MSI_L2A_TL_MTI__20150812T174654_A000634_T33UUB_N01.03/S2A_USER_MTD_L2A_TL_MTI__20150812T174654_A000634_T33UUB.xml
#         xml=/neponset/nbdata06/albedo/zhan.li/sentinel-2/l2a/S2A_USER_PRD_MSIL2A_PDMC_20160107T014224_R111_V20160106T154314_20160106T154314.SAFE/GRANULE/S2A_USER_MSI_L2A_TL_MTI__20160106T203715_A002825_T19TCH_N02.01/S2A_USER_MTD_L2A_TL_MTI__20160106T203715_A002825_T19TCH.xml

#         short=${xml##*/}
#         base=${short%.xml}

#         echo "base=$base"

#         dir="${out_dir}/${base}"
#         if [ ! -r $dir ]; then
#                 mkdir -p $dir
#         fi

#         log="${dir}/${base}.log.txt"

#         mod_a1="${dir}/${base}.modis.a1.hdf"
#         mod_a2="${dir}/${base}.modis.a2.hdf"

#         if [ ! -r $mod_a1 ]; then
#                 echo "Subsetting mcd43a1 ..."
#                 $sub_exe $mod_dir "MCD43A1" $xml $mod_a1 -f
#                 if [ $? -ne 0 ]; then
#                         echo "Get mcd43a1 subset failed." 
#                         exit 1
#                 fi
#         fi
        
#         if [ ! -r $mod_a2 ]; then
#                 echo "Subsetting mcd43a2 ..."
#                 $sub_exe $mod_dir "MCD43A2" $xml $mod_a2 -f
#                 if [ $? -ne 0 ]; then
#                         echo "Get mcd43a2 subset failed."
#                         exit 1
#                 fi
#         fi

#         if [ ! -r "./runtime" ]; then
#             mkdir -p "./runtime"
#         fi
#         cls_parm="./runtime/cls_parm_${base}.txt"
#         alb_parm="./runtime/alb_parm_${base}.txt"

#         cls="${dir}/cls${ncls}_${ncls}_150_${base}.bin"
#         dist="${dir}/cls${ncls}_${ncls}_150_${base}.dist"
#         alb="${dir}/lndAlbedo_${base}.bin"

#         cd $dir_run

#         #generate class parameter file
#         echo "PARAMETER_FILE" > $cls_parm
#         echo "FILE_NAME =${xml}" >> $cls_parm
#         echo "OUT_CLSMAP=${cls}" >> $cls_parm
#         echo "DISTANCE_FILE=${dist}" >> $cls_parm
#         echo "NBANDS = 6" >> $cls_parm
#         echo "NCLASSES =${ncls}" >> $cls_parm
#         echo "THRS_RAD =0.150" >> $cls_parm
#         echo "I_CLUSTER =${ncls}" >> $cls_parm
#         echo "END." >> $cls_parm

#   #generate albedo parameter file
#         echo "#define parameters for input TM class map and SR file" > $alb_parm
#         echo "DM_CLSMAP=${cls}" >> $alb_parm
#         echo "DM_FILE_NAME=${xml}" >> $alb_parm
#         echo "DM_NBANDS=6" >> $alb_parm
#         echo "DM_NCLS=${ncls}" >> $alb_parm
#         echo "CLS_FILL_VALUE=-9" >> $alb_parm
#         echo "" >> $alb_parm
#         echo "# MODIS input file" >> $alb_parm
#         echo "MODIS_BRDF_FILE=${mod_a1}" >> $alb_parm
#         echo "MODIS_QA_FILE=${mod_a2}" >> $alb_parm
#         echo "" >> $alb_parm
#         echo "AMONGclsDIST_FILE_NAME=${dist}" >> $alb_parm
#         echo "OUTPUT_ALBEDO =${alb}" >> $alb_parm
#         echo "END" >> $alb_parm

#   #run
#         #if [ ! -r $cls ]; then
#                 echo "Classifying ..."
#                 #$cls_exe $cls_parm >> $log
#                 $cls_exe $cls_parm 
#                 if [ $? -ne 0 ]; then
#                         echo "ERROR"
#                         #exit
#                         echo "$log failed: classification." >> $err
# #                       continue
#                 fi
#         #else
#         #       echo "Warning: classification file already exists, skip classification." >> $log
#         #fi

#         #if [ ! -r $alb ]; then
#                 echo "Calculating albedo ..."
#                 #$alb_exe $alb_parm >> $log
#                 $alb_exe $alb_parm 
#                 if [ $? -ne 0 ]; then
#                         echo "ERROR"
#                         #exit
#                         echo "$log failed: calculating albedo." >> $err
# #                       continue
#                 fi
#         #fi
        
# #done

echo "DONE, sentinel_2_albedo_scene.sh"
