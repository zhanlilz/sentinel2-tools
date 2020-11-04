#!/bin/bash

bold=$(tput bold)
normal=$(tput sgr0)
read -d '' USAGE <<EOF
Usage: download_sentinel_gs.sh [options] URL_LIST_FILE DIRECTORY

Download sentinel data from ${bold}Google Cloud Storage${normal} to the
directory DIRECTORY, given the list of download URLs in the file URL_LIST_FILE
in which one arbitrary string (e.g., custome label), one product name separated
by space per line, for example, 

my_label S2A_MSIL2A_20200829T102031_N0214_R065_T33UUV_20200829T131252
or_just_placeholder S2A_MSIL2A_20200829T102031_N0214_R065_T32UQE_20200829T131252

Options:
  -i, --index="path/to/gs/index.csv", ${bold}required${normal}
    Give the path to the index.csv file from the Goolge Storage. It is usually
    called index.csv on Google Cloud Storage. See
    https://cloud.google.com/storage/docs/public-datasets for more details.

  -l, --log="log_file", optional
    Specify a log file to record the downloaded links (100% finish). All newly
    downloaded links (100% finish) will be appended to this log file.

  -I, --ignore_dl="ignored_dl_list", optional
    Specify a file of a list of data records to be ignored in the downloading.
    These are usually the data records in the given URL_LIST_FILE that have been
    downloaded and validated.

  -R, --resolution=RES, optional
    Select a resolution RES from accepted values ${bold}10, 20, and
    60${normal}.  If set, only images at the given resolution will be
    downloaded. ${bold}Multiple -R/--resolution options can be used to select
    multiple resolutions to download${normal}.

  -B, --band=BAND_SUFFIX, optional
    Select a band by giving a band suffix BAND_SUFFIX from accepted values
    ${bold}B01, B02, B03, B04, B05, B06, B07, B08, B8A, B09, B11, B12, AOT,
    SCL, TCI, WVP${normal}. If set, only images with the given band suffxi will
    be downloaded. ${bold}Multiple -B/--band options can be used to select
    images with multiple band suffixes to download${normal}. For more details
    on what these suffixes mean, see ESA's website
    https://sentinel.esa.int/web/sentinel/user-guides/sentinel-2-msi/processing-levels/level-2,
    or read ESA's document "L2A Input Output Data Definitions (IODD)".

  -v, --verbose, optional
    Turn on verbose mode

EOF

GS_PUBURL_PREFIX='https://storage.googleapis.com/'

GSINDEX=""
VERBOSE=0 # verbose mode, output more detailed information to the screen
DLLOG=""
IGNORELOG=""
RESOLUTION_ARR=()
BAND_ARR=()
OPTS=`getopt -o vl:I:i:R:B: --long verbose,log:,ignore_dl:,index:,resolution:,band: -n 'download_sentinel_gs.sh' -- "$@"`
if [[ $? != 0 ]]; then echo "Failed parsing options."; echo "${USAGE}"; exit 1 ; fi
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

eval set -- "${OPTS}"
while true;
do
    case "${1}" in
        -i | --index )
            case "${2}" in
                "") shift 2 ;;
                *) GSINDEX=${2} ; shift 2 ;;
            esac ;;
        -l | --log )
            case "${2}" in
                "") shift 2 ;;
                *) DLLOG=${2} ; shift 2 ;;
            esac ;;
        -I | --ignore_dl )
            case "${2}" in
                "") shift 2 ;;
                *) IGNORELOG=${2} ; shift 2 ;;
            esac ;;                     
        -v | --verbose )
            VERBOSE=1 ; shift ;;
        -R | --resolution )
            case "${2}" in
                "") shift 2 ;;
                *) RESOLUTION_ARR=(${RESOLUTION_ARR[@]} ${2}) ; shift 2 ;;
            esac ;;
        -B | --band )
            case "${2}" in
                "") shift 2 ;;
                *) BAND_ARR=(${BAND_ARR[@]} ${2}) ; shift 2 ;;
            esac ;;
        -- ) shift ; break ;;
        * ) break ;;
    esac
done

if [[ -z ${GSINDEX} ]]; then
    echo "-i/--index is required!"
    echo 
    echo "${USAGE}"
    exit 1
fi

MINPARAMS=2
if [[ ${#} < ${MINPARAMS} ]]; then
    echo "Missing argument!"
    echo "${USAGE}"
    exit 1
fi
IN_URL_FILE=${1}
OUTDIR=${2}

if [[ -z ${IN_URL_FILE} || -z ${OUTDIR} ]]; then
    echo "Missing argument!"
    echo
    echo "${USAGE}"
    exit 1
fi 

if [[ ! -r ${IN_URL_FILE} ]]; then
    echo "Input URL file ${IN_URL_FILE} does not exist or cannot be read!"
    exit 1
fi

if [[ ! -d ${OUTDIR} ]]; then
    mkdir -p "${OUTDIR}"
fi

if [[ -z ${DLLOG} ]]; then
    DLLOG="/dev/null"
    echo "NOT to log the record of successfully downloaded data files"
fi
if [[ ! -r ${DLLOG} ]]; then
    touch ${DLLOG}
fi

LOG="${OUTDIR}/.download_sentinel.log"

read_dom ()
{
    local IFS=\>
    read -d \< ENTITY CONTENT
}

WAITRETRY=60
MIN_SLEEPTIME=2
MAX_SLEEPTIME=10
> ${LOG}
NUM_URL=$(cat "${IN_URL_FILE}" | wc -l)
URL_IND=0
TRIES=10

ALLDONE=0

SERVER_ERR_CNT=0

while IFS= read -r url
do
    URL_IND=$((URL_IND+1))
    outname=$(echo ${url} | cut -f2 -d ' ')
   
    if [[ -r ${IGNORELOG} ]]; then
        PRODUCT_ID=${outname}
        grep -q "${PRODUCT_ID}" ${IGNORELOG}
        if [[ $? -eq 0 ]]; then
            echo "Downloaded and validated ${URL_IND} / ${NUM_URL}: ${url}, skip"
            continue
        fi
    fi

    gs_rec=$(grep ${outname} ${GSINDEX})
    if [[ -z ${gs_rec} ]]; then
        echo "${outname} NOT found in ${GSINDEX}!"
        continue
    fi
    granule_id=$(echo ${gs_rec} | cut -d',' -f1)
    base_url=$(echo ${gs_rec} | rev | cut -d',' -f1 | rev)
    
    echo "Downloading ${URL_IND} / ${NUM_URL}: ${outname} from ${base_url} to ${OUTDIR}"
    
    sleep $((RANDOM%(MAX_SLEEPTIME-MIN_SLEEPTIME)+MIN_SLEEPTIME))
    if [[ ${#RESOLUTION_ARR[@]} -eq 0 && ${#BAND_ARR[@]} -eq 0 ]]; then
        gsutil -m cp -r ${base_url} ${OUTDIR}
        continue
    fi

    if [[ ${#RESOLUTION_ARR[@]} -eq 0 ]]; then
        RESOLUTION_ARR=(10 20 60)
    fi
    for ((i=0; i<${#RESOLUTION_ARR[@]}; i++)); do
        res=${RESOLUTION_ARR[$i]}
        tmp_dir="${OUTDIR}/${outname}.SAFE/GRANULE/${granule_id}/IMG_DATA/R${res}m"
        if [[ ! -d ${tmp_dir} ]]; then 
            mkdir -p ${tmp_dir}
        fi
        if [[ ${#BAND_ARR[@]} -eq 0 ]]; then
            gsutil -m cp -r ${base_url}/GRANULE/*/IMG_DATA/R${res}m/* ${tmp_dir}/
        else
            for ((j=0; j<${#BAND_ARR[@]}; j++)); do
                gsutil -m cp -r ${base_url}/GRANULE/*/IMG_DATA/R${res}m/*${BAND_ARR[$j]}* ${tmp_dir}/
            done
        fi
    done
done < "${IN_URL_FILE}"    

exit ${ALLDONE}
