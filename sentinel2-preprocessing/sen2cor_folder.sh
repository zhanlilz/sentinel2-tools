#!/bin/bash

# sen2cor_forlder.sh
#
# Run sen2cor, the atmospheric correction program of Sentinel-2 on all
# Sentinel-2 SAFEs/scenes in a given folder.
#
# Zhan Li, zhan.li@umb.edu
# Created: Mon Apr 25 11:50:20 EDT 2016

read -d '' USAGE <<EOF
sen2cor_folder.sh [options] L1C_DIR

Usage

  Run atmospheric correction on all Sentinel-2 SAFEs/scenes in the folder L1C_DIR

Options

  -r, --resolution, required, target resolution, must be 10, 20 or 60
   [m], e.g. -r 10, or --resolution=20

  -o, --output-directory, optional, if given a directory, output L2A
   SAFEs here. Otherwise, leave L2A SAFEs in the same directory as the
   L1C SAFEs. Default: no output directory. e.g.,
   -o"my/out/directory", or --output-directory="my/out/directory"

  -g, --granule, optional, a list of granule ID (UTM MGRS Grid)
   delimitered by comma ',', trick L2A_Process to only process these
   granules rather than a whole scene by changing the granule folder
   name of non-selected granules; e.g., -g"15SYT,15SYU", or,
   --granule="18TTL"

  --clean-l1c, optional, if this option set, the program will remove
    L1C SAFE folder after successfully finishing sen2cor and
    generating L2A SAFE folder.

  --cmd, optional, full path to the L2A_Process command. Default is
    simply L2A_Process.

EOF

CLEAN_L1C=0
CMD="L2A_Process"
OPTS=`getopt -o r:o:g: --long resolution:,output-directory:,granule:,clean-l1c -n 'sen2cor_folder.sh' -- "$@"`
if [[ ! $? -eq 0 ]]; then echo "Failed parsing options" >&2 ; echo "${USAGE}" ; exit 1 ; fi
eval set -- "${OPTS}"
while true; 
do
    case "${1}" in 
        -r | --resolution )
            case "${2}" in
                "") shift 2 ;;
                *) RES=${2} ; shift 2 ;;
            esac ;;
        -o | --output-directory )
            case "${2}" in
                "") shift 2 ;;
                *) OUTDIR=${2} ; shift 2 ;;
            esac ;;
        -g | --granule )
            case "${2}" in
                "") shift 2 ;;
                *) GRANULES=${2} ; shift 2 ;;
            esac ;;
        --clean-l1c )
            CLEAN_L1C=1 ; shift ;;
        --cmd )
            case "${2}" in
                "") shift 2 ;;
                *) CMD=$${2} ; shift 2 ;;
            esac ;;
        -- ) shift ; break ;;
        * ) break ;;
    esac
done
if [[ -z ${RES} ]]; then
    echo "Option --resolution is required!"
    echo "${USAGE}"
    exit 1
fi

MINPARAMS=1
if [[ ${#} < ${MINPARAMS} ]]; then
    echo "Missing positional arguments"
    echo "${USAGE}"
    exit 1
fi
L1C_DIR=${1}

# Some extra config here to make sure we have the GDAL_DATA env
# variable set to the sen2cor version.
if [[ ! -z ${SEN2COR_HOME} ]]; then
    OLD_GDAL_DATA=${GDAL_DATA}
    source ${SEN2COR_HOME}/L2A_Bashrc
fi

L1CS=($(find ${L1C_DIR} -maxdepth 1 -type d -name "S*L1C*.SAFE"))

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

if [[ ! -z ${GRANULES} ]]; then
    echo "Select Granules: ${GRANULES}"
    IGNORE_DIR="ignore-granule"
    while IFS= read -r -d ',' tmp;
    do
        # GR_LIST=("${GR_LIST[@]}" "$tmp")
        GR_LIST+=("${tmp}")
    done < <(echo "${GRANULES},")

    # change the folder names of non-selected granuules temporarily to
    # trick L2A_Process not to process them.
    for (( i=0; i<${#L1CS[@]}; ++i ));
    do
        if [[ -d "${L1CS[i]}/GRANULE/${IGNORE_DIR}" ]]; then
            # restore the ignore status of granules for new
            # selection             
            find "${L1CS[i]}/GRANULE/${IGNORE_DIR}" -type d -name "S*L1C*" -print0 | xargs -0 -I dirs mv dirs ${L1CS[i]}/GRANULE/
            # rm -rf "${L1CS[i]}/GRANULE/${IGNORE_DIR}"
        fi

        ALL_GRS=($(find "${L1CS[i]}/GRANULE/" -maxdepth 1 -type d -name "S*L1C*"))
        for (( j=0; j<${#ALL_GRS[@]}; ++j ));
        do
            gname=${ALL_GRS[j]}
            id=$(basename ${gname} | rev | cut -d '_' -f2 | rev)
            # cut the "T" at the beginning
            id=${id:1}
            In_Array "${id}" "$(echo ${GR_LIST[@]})"
            if [[ $? != 1 ]]; then
                if [[ ! -d "${L1CS[i]}/GRANULE/${IGNORE_DIR}" ]]; then
                    mkdir -p "${L1CS[i]}/GRANULE/${IGNORE_DIR}"
                fi
                mv "${gname}" "${L1CS[i]}/GRANULE/${IGNORE_DIR}/"
            fi
        done
    done
fi

if [[ ! -z ${OUTDIR} ]]; then
    if [[ ! -d ${OUTDIR} ]]; then
        mkdir -p ${OUTDIR}
    fi
fi

NUMFILES=${#L1CS[@]}
for (( i=0; i<${NUMFILES}; ++i ));
do
    echo "L2A <---- L1C $((i+1)) / ${NUMFILES}: $(basename ${L1CS[i]})"
    ${CMD} --resolution ${RES} ${L1CS[i]}
    SEN2COR_EXIT=$?
    if [[ ! -z ${OUTDIR} ]]; then
        TMP=${L1CS[i]}
        TMP=${TMP/"OPER"/"USER"}
        TMP=${TMP/"L1C"/"L2A"}
        mv ${TMP} ${OUTDIR}
    fi
    if [[ ${SEN2COR_EXIT} -eq 0 && ${CLEAN_L1C} -eq 1 ]]; then
        echo "Clean L1C input ${L1CS[i]}"
        rm -rf ${L1CS[i]}
    fi
    echo
done

if [[ ! -z ${SEN2COR_HOME} ]]; then
    # restore old GDAL_DATA after processing with sen2cor
    export GDAL_DATA=${OLD_GDAL_DATA}
fi
