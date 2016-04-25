#!/bin/bash

# unzip_sentinel_data.sh
#
# Unzip sentinel zip files in a folder.
#
# Zhan Li, zhan.li@umb.edu
# Created: Mon Apr 25 08:43:55 EDT 2016

read -d '' USAGE <<EOF
Usage: unzip_sentinel_data.sh [options] DATA_DIR

Unzip the sentinel zip files in a directory DATA_DIR.

Options:

  -m, --move, move zip file into the folder from unzipping.

EOF


MOVE=0
OPTS=`getopt -o m --long move -n 'unzip_sentinel_data.sh' -- "$@"`
if [[ $? != 0 ]]; then echo "Failed parsing options" >&2 ; exit 1 ; fi
eval set -- "${OPTS}"
while true;
do
    case "${1}" in
        -m | --move )
            case "${2}" in
                "") shift 2 ;;
                *) MOVE=1 ; shift 2 ;;
            esac ;;
        -- ) shift ; break ;;
        * ) break ;;
    esac
done
MINPARAMS=1
if [[ ${#} < ${MINPARAMS} ]]; then
    echo "${USAGE}"
    exit 1
fi
DATADIR=${1}

ZIPFILES=($(find ${DATADIR} -maxdepth 1 -name "*.zip"))
NUMFILES=${#ZIPFILES[@]}
for (( i=0; i<${NUMFILES}; ++i )); 
do
    fname=$(basename ${ZIPFILES[i]})
    echo "<----- $((i+1)) / ${NUMFILES}: ${fname}"

    mkdir -p "${DATADIR}/temp"
    unzip -q ${ZIPFILES[i]} -d "${DATADIR}/temp"
    SAFE=$(find "${DATADIR}/temp" -maxdepth 1 -name "*.SAFE")
    if [[ -z ${SAFE} ]]; then
        echo "No SAFE folder is found after zipping:"
        ls "${DATADIR}/temp"
        echo "Check the sentinel zip file: ${fname}"
        rm -rf "${DATADIR}/temp/*"
        continue
    fi
    if [[ ${MOVE} -eq 1 ]]; then
        mv "${ZIPFILES[i]}" "${SAFE}"
    fi
    mv "${SAFE}" "${DATADIR}"

done

rm -rf "${DATADIR}/temp"

echo "Unzipping sentinel done, ${DATADIR}!"
