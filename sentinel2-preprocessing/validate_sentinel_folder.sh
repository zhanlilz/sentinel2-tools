#!/bin/bash

read -d '' USAGE <<EOF 
validate_sentinel_folder_bnu.sh [options] SEN_DATA_DIR COR_FILE

Validate the integrity of Sentinel data files in the folder SEN_DATA_DIR and
write the list of corrupted file names to an ASCII file COR_FILE

Options:

  -c, --clean, optional: if set, remove corrupted files after checking; e.g.
	-c or --clean; default: NOT remove.

  -p, --pattern, optional: a string of file name pattern for searching files
	to validate; e.g. -p"*.zip" or --pattern"*.zip"; default file name patterns
	for searching is:     
	Pattern 1: *.zip in the given folder, NOT in subfolders.     
	Pattern 2: \$value in the given folder and all its subfolders.

EOF

VERIFIED_RECORD="checksum_verified"

CLEAN=0
OPTS=`getopt -o cp:: --long clean,pattern:: -n 'validate_sentinel_folder_bnu.sh' -- "$@"`
if [[ $? != 0 ]]; then echo "Failed parsing options." >&2 ; exit 1 ; fi
eval set -- "${OPTS}"
while true;
do
    case "${1}" in
        -c | --clean ) 
			CLEAN=1 ; shift ;;
        -p | --pattern )
            case "${2}" in
                "") shift 2 ;;
                *) PATTERN=${2} ; shift 2 ;;
            esac ;;
        -- ) shift ; break ;;
        * ) break ;;
    esac
done
# parse input arguments
MINPARAMS=2
if [[ $# -lt "$MINPARAMS" ]]; then
  echo "${USAGE}"
  exit 1
fi

if [[ -n "${1}" ]]; then
    DATADIR="${1}"
    echo "Directory to the sentinel data folders: $DATADIR"
fi 
if [[ -n "${2}" ]]; then
    CTLIST="${2}"
    echo "Output file of list of corrupted files: $CTLIST"
fi 
if [[ ${CLEAN} -eq 1 ]]; then
    echo "Corrupted files will be cleaned"
fi
echo

DLTYPE="zip"
if [[ -z ${PATTERN} ]]; then
    TMP1=($(find ${DATADIR} -maxdepth 1 -name '*.zip'))
    if [[ ${#TMP1[@]} -eq 0 ]]; then
        TMP1=($(find ${DATADIR} -name "\$value"))
        DLTYPE="value"
    fi
else
    TMP1=($(find ${DATADIR} -name "${PATTERN}"))
    DLTYPE="user"
fi
NUMFILES=${#TMP1[@]}
if [[ ${NUMFILES} -eq 0 ]]; then
    echo "No available files found in this folder. Have you given the correct pattern of file names to search?"
    echo "Default file name patterns for searching is: "
    echo "Pattern 1: *.zip in the given folder, NOT in subfolders."
    echo "Pattern 2: \$value in the given folder and all its subfolders."
    exit 1
else
    echo "Found ${NUMFILES} files to validate."
fi

if [[ -w ${CTLIST} ]]; then
        > ${CTLIST}
        echo "Cleared old corrupted file list!"
else
    > ${CTLIST}
fi

SKIP=0
if [[ -f ${DATADIR}/${VERIFIED_RECORD} ]]; then
    echo "Found a record of validated files. Will skip validating the checksum of these files to save time"
    echo
    SKIP=1
fi

# ServiceRootUri="https://scihub.copernicus.eu/dhus/odata/v1"
for (( i=0; i<${NUMFILES}; ++i ));
do
    line=${TMP1[i]}
    fname=$(readlink -f ${line})

    if [[ ${SKIP} -eq 1 ]]; then
        TMP=$(grep "${fname}" ${DATADIR}/${VERIFIED_RECORD})
        if [[ $? -eq 0 ]]; then
            echo "Skip $((i+1)) / ${NUMFILES}: ${fname}, validated"
            continue
        else
        	TMP=$(grep "${line}" ${DATADIR}/${VERIFIED_RECORD})
        	if [[ $? -eq 0 ]]; then
        		echo "Skip $((i+1)) / ${NUMFILES}: ${fname}, validated"
            	continue	
        	fi
        fi
    fi

    echo -n "Validating $((i+1)) / ${NUMFILES}: ${fname}"
    # get the checksum file
    CKSUMFILE="${fname}.md5"
    if [[ ! -f ${CKSUMFILE} ]]; then
        CKSUMFILE=${fname%".zip"}".md5"
        if [[ ! -f ${CKSUMFILE} ]]; then
            echo
            echo "Checksum file does not exist: ${fname}"
            continue
        fi
    fi
    
    O_CHECKSUM=$(cat ${CKSUMFILE})
    if [[ -z ${O_CHECKSUM} ]]; then
        echo
        echo "${CKSUMFILE} is empty! Removed it!"
        rm -rf ${CKSUMFILE}
        continue
    fi 
    
    C_CHECKSUM=$(md5sum ${TMP1[i]} | tr -s ' ' | cut -d ' ' -f 1)
    if [[ ${O_CHECKSUM^^} != ${C_CHECKSUM^^} ]]; then
        echo ${fname} >> ${CTLIST}
        echo ", corrupted"
        if [[ ${CLEAN} -eq 1 ]]; then
            echo "Remove corrupted file: ${fname}"
            rm -rf ${TMP1[i]}
            # if the download was done by wget recursive option, i.e. the file
            # is $value in a folder named with product ID.  also remove the
            # folder of product ID.
            if [[ ${DLTYPE} == "value" ]]; then
            	rm -rf ${fname%"/\$value"}
                echo "Remove the folder of the corrupted file: ${fname%"/\$value"}"
            fi
        fi
    else
        echo ", valid"
        echo ${fname} >> ${DATADIR}/${VERIFIED_RECORD}
    fi
done

echo "${NUMFILES} files validation done!"
echo `cat ${CTLIST} | wc -l` "files found corrupted"
echo "=================================="
