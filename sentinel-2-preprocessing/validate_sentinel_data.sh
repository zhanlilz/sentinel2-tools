#!/bin/bash

# verify data integrity via md5 checksum
# usage:
# verify_data_integrity.sh directory_to_sentinel_data_files directory_to_sentinel_checksum_files path_to_list_of_corrupted_files

USER="zhan.li"
PSW="1986721615"

VERIFIED_RECORD="checksum_verified"

CLEAN=0
OPTS=`getopt -o c --long clean -n 'verify_data_integrity.sh' -- "$@"`
if [[ $? != 0 ]]; then echo "Failed parsing options." >&2 ; exit 1 ; fi
eval set -- "${OPTS}"
while true;
do
    case "${1}" in
        -c | --clean )
            case "${2}" in
                "") shift 2 ;;
                *) CLEAN=1 ; shift 2 ;;
            esac ;;
        -- ) shift ; break ;;
        * ) break ;;
    esac
done
# parse input arguments
MINPARAMS=3
if [[ $# -lt "$MINPARAMS" ]]; then
  echo
  echo "Verify data integrity via md5 checksum"
  echo "Usage: "
  echo "${0} [options] directory_to_sentinel_data_files directory_to_sentinel_checksum_files path_to_list_of_corrupted_files"
  exit 1
fi

if [[ -n "${1}" ]]; then
    DATADIR="${1}"
    echo "Directory to the sentinel data folders: $DATADIR"
fi 
if [[ -n "${2}" ]]; then
    CKSUMDIR="${2}"
    echo "Directory to the sentinel checksum files: ${CKSUMDIR}"
fi
if [[ -n "${3}" ]]; then
    CTLIST="${3}"
    echo "Output file of list of corrupted files: $CTLIST"
fi 
if [[ ${CLEAN} -eq 1 ]]; then
    echo "Corrupted files will be cleaned"
fi
echo

TMP1=($(find ${DATADIR} -maxdepth 1 -name '*.zip'))
NUMFILES=${#TMP1[@]}

if [[ -w ${CTLIST} ]]; then
        rm -f ${CTLIST}
        echo "Removed old corrupted file list!"
fi

SKIP=0
if [[ -f ${DATADIR}/${VERIFIED_RECORD} ]]; then
    echo "Found a record of verified files. Will skip verifying the checksum of these files to save time"
    echo
    SKIP=1
fi

# ServiceRootUri="https://scihub.copernicus.eu/dhus/odata/v1"
for (( i=0; i<${NUMFILES}; ++i ));
do
        line=${TMP1[i]}
        fname=$(basename ${line})
        bname=$(basename ${fname} '.zip')

        if [[ ${SKIP} -eq 1 ]]; then
            TMP=$(grep "${bname}" ${DATADIR}/${VERIFIED_RECORD})
            if [[ $? -eq 0 ]]; then
                echo "Skip ${fname}"
                continue
            fi
        fi

        echo "Verifying ${fname}"
        # get the checksum file
        CKSUMFILE="${bname}.md5"
        if [[ ! -f ${CKSUMDIR}/${CKSUMFILE} ]]; then
            echo "Checksum file does not exist: ${fname}"
            continue
        fi
        
        O_CHECKSUM=$(cat ${CKSUMDIR}/${CKSUMFILE})
        
        C_CHECKSUM=$(md5sum ${TMP1[i]} | tr -s ' ' | cut -d ' ' -f 1)
        if [[ ${O_CHECKSUM^^} != ${C_CHECKSUM^^} ]]; then
            echo ${fname} >> $CTLIST
            if [[ ${CLEAN} -eq 1 ]]; then
                echo "Remove corrupted file: ${fname}"
                rm -rf ${TMP1[i]}
            fi
        else
            echo ${fname} >> ${DATADIR}/${VERIFIED_RECORD}
        fi
done

echo "Verification done!"
echo "=================="