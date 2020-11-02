#!/bin/bash

# Given a file of list of Sentinel-2 data download links, repeat the five
# steps to ensure all data are downloaded correctly, 1) download, 2) validate
# checksum, 3) remove corrupted files, 4) check missing files, and 5) repeat!
#
# Zhan Li, zhan.li@umb.edu
# Created: Fri May 13 13:39:33 EDT 2016

read -d '' USAGE <<EOF
all_in_one_dl_sentinel.sh [options] DL_LIST DIR

Given DL_LIST, the file of list of Sentinel-2 data download links,
repeat the five steps to ensure all data are downloaded correctly to
the folder DIR, 1) download, 2) validate checksum, 3) remove corrupted
files, 4) check missing files, and 5) repeat!

Options

  --user

  --password

  -r, --recursive

  -v, --verbose

EOF

_script="$(readlink -f ${0})"
_mydir="$(dirname ${_script})"
echo "Will search and use commands in ${_mydir}"

RECURSIVE=0 # download files using wget --recursive option
VERBOSE=0 # verbose mode, output more detailed information to the screen
MAXNUMRUN=15 # maximum number of tries of running the five steps

OPTS=`getopt -o rv --long user:,password:,recursive,verbose -n 'all_in_one_dl_sentinel.sh' -- "$@"`
if [[ $? != 0 ]]; then echo "Failed parsing options." >&2 ; echo "${USAGE}" ; exit 1 ; fi
substr_exist ()
{
    # substr_exist substr str
    # local substr=$(echo ${1} | sed 's/-/\\-/g')
    echo "${2}" | grep -q "${1}"
    if [[ $? -eq 0 ]]; then
        return 1
    else
        return 0
    fi
}
substr_exist "\-\-user" "${OPTS}"
if [[ $? -eq 0 ]]; then echo "No --user" ; echo "${USAGE}" ; exit 1 ; fi
substr_exist "\-\-password" "${OPTS}"
if [[ $? -eq 0 ]]; then echo "No --password" ; echo "${USAGE}" ; exit 1 ; fi

eval set -- "${OPTS}"
while true;
do
    case "${1}" in 
        --user )
            case "${2}" in
                "") shift 2 ;;
                *) USER=${2} ; shift 2 ;;
            esac ;;
        --password )
            case "${2}" in
                "") shift 2 ;;
                *) PSW=${2} ; shift 2 ;;
            esac ;;
        -v | --verbose )
            VERBOSE=1 ; shift ;;
        -r | --recursive )
            RECURSIVE=1 ; shift ;;
        -- ) shift ; break ;;
        * ) break ;;
    esac
done
MINPARAMS=2
if [[ ${#} < ${MINPARAMS} ]]; then
    echo "${USAGE}"
    exit 1
fi
IN_URL_FILE=${1}
OUTDIR=${2}

# some commands
DL_CMD="${_mydir}/download_sentinel.sh" # download data
VAL_CMD="${_mydir}/validate_sentinel_folder.sh" # validate checksum
MS_CMD="${_mydir}/verify_file_list.sh" # look for missing files

# some files to write
COR_FILE="${IN_URL_FILE}.corrupted"
MS_FILE="${IN_URL_FILE}.missing"

RUN_NUM=0
options="-c"
if [[ ${VERBOSE} -eq 1 ]]; then
    options=${options}"v"
elif [[ ${RECURSIVE} -eq 1 ]]; then
    options=${options}"r"
else
    options=${options}
fi

MS_NUM=1

DL_CMD_TRIES=100
DLLOG="${IN_URL_FILE}.dllog"
IGNORELOG=${OUTDIR}/"checksum_verified"

while [[ ${MS_NUM} -gt 0 ]];
do
    if [[ ${RUN_NUM} -ge ${MAXNUMRUN} ]]; then
        echo "Reached the maximum number of running tries, ${MAXNUMRUN}."
        echo "Still have ${MS_NUM} files unsuccessfully downloaded. They are listed in the file ${MS_FILE}.${RUN_NUM}."
        exit 2
    fi

    RUN_NUM=$((RUN_NUM+1))    
    echo "Start running round #${RUN_NUM}"

    if [[ -r ${IGNORELOG} ]]; then
        cat ${IGNORELOG} > ${DLLOG}
    fi
    for (( i=0; i<${DL_CMD_TRIES}; i++ ))
    do
        echo 
        echo "Downloading Try #$((i+1)) in running round #${RUN_NUM} ..."
        if [[ ${RECURSIVE} -eq 1 ]]; then
            ${DL_CMD} --user="${USER}" --password="${PSW}" --log="${DLLOG}" -rc -I "${DLLOG}" "${IN_URL_FILE}" "${OUTDIR}"
        else
            ${DL_CMD} --user="${USER}" --password="${PSW}" --log="${DLLOG}" -c -I "${DLLOG}" "${IN_URL_FILE}" "${OUTDIR}"
        fi
        if [[ $? -eq 0 ]]; then
            break
        fi
        # wait a little longer every time when we have one more downloading
        # try to make sure the server will not block us.
        sleep $((60*$((i+1))))
    done
    
    echo
    echo "Validating ..."
    ${VAL_CMD} -c "${OUTDIR}" "${COR_FILE}.${RUN_NUM}"

    # find missing data files and checksum files
    if [[ ${RECURSIVE} -eq 1 ]]; then
        ${MS_CMD} --pattern="\$value" "${OUTDIR}" "${IN_URL_FILE}" "${MS_FILE}.${RUN_NUM}"
    else
        ${MS_CMD} --pattern="*.zip" "${OUTDIR}" "${IN_URL_FILE}" "${MS_FILE}.${RUN_NUM}"
    fi
    
    MS_NUM=$(cat "${MS_FILE}.${RUN_NUM}" | wc -l)

    echo "Finish running round #${RUN_NUM}"
    echo "********************************"
    echo
done
echo "Finish downloading all!"
