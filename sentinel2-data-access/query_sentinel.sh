#!/bin/bash
# read a query of Sentinel data search and output all the download links to a file for the input file to wget
# Zhan Li, zhan.li@umb.edu
# Created: Tue Mar  8 17:44:57 EST 2016

read -d '' USAGE <<EOF

query_sentinel.sh [options] REQUEST_STR

Query Sentinel data from ESA Sentinel Data Hub through OpenSearch,
given *REQUEST_STR*, the string of query conditions. *REQUEST_STR* can
be directly copied from the section "Request Done:" on the page
https://scihub.copernicus.eu/dhus/ after you interactively search the
data of your interest.

#*********************************************************************
#* IMPORTANT: put your *REQUEST_STR* in SINGLE QUOTE such that you   *
#* do NOT have to escape any special characters in the string of     *
#* query conditions, such as double quots, '$', '&' and etc..        *
#*********************************************************************

Options

  -u, --user, required
    Username of ESA Science Data Hub account

  -p, --password, required
    Password of ESA Science Data Hub account

  -o, --outprefix, required
    Prefix of output file of list of found images, CANNOT be a
    directory

  -d, --disk
    Disk space (unit, byte) to hold part of the image data, used to
    split the list of images into several files of image lists, each
    of which list contains images of size no larger than the given
    disk space. A string of mathematic expression is acceptable to
    represent the disk space, e.g. "4*1024*1024*1024" in unit of byte
    equals to 4 GB. If no disk space is provided, all found image
    files will be listed in a single file of the given outprefix
    without being sorted or split.

  -M, --meta, optional
    Turn on the output of metadata of found image records to a CSV file.

  -q, --quiet, optional
    Disable confirmation of inputs by the user and run query quietly.

Examples of *REQUEST_STR*

  * Sentinel-1, polygon, from given beginning time to given ending time
'( footprint:"Intersects(POLYGON((-180 58.973437549923915,0 58.973437549923915,0 85.71249419049184,-180 85.71249419049184,-180 58.973437549923915)))" OR footprint:"Intersects(POLYGON((0 58.973437549923915,180 58.973437549923915,180 85.71249419049184,0 85.71249419049184,0 58.973437549923915)))" ) AND ( beginPosition:[2016-02-01T00:00:00.000Z TO 2016-02-29T23:59:59.999Z] AND endPosition:[2016-02-01T00:00:00.000Z TO 2016-02-29T23:59:59.999Z] ) AND (platformname:Sentinel-1) AND (producttype:SLC OR producttype:GRD OR producttype:OCN)'

  * Sentinel-1, point, all available images from given beginning time until now
'( footprint:"Intersects(48.307960, -105.101750)" ) AND ( beginPosition:[2015-06-23T00:00:00.000Z TO NOW] AND endPosition:[2015-06-23T00:00:00.000Z TO NOW] ) AND (platformname:Sentinel-1) AND (producttype:SLC OR producttype:GRD OR producttype:OCN)'

  * Sentinel-2, polygon, from given beginning time to given ending time
'( footprint:"Intersects(POLYGON((-180 58.973437549923915,0 58.973437549923915,0 85.71249419049184,-180 85.71249419049184,-180 58.973437549923915)))" OR footprint:"Intersects(POLYGON((0 58.973437549923915,180 58.973437549923915,180 85.71249419049184,0 85.71249419049184,0 58.973437549923915)))" ) AND ( beginPosition:[2016-02-01T00:00:00.000Z TO 2016-02-29T23:59:59.999Z] AND endPosition:[2016-02-01T00:00:00.000Z TO 2016-02-29T23:59:59.999Z] ) AND (platformname:Sentinel-2)'

  * Sentinel-2, point, all available images from given beginning time until now
'( footprint:"Intersects(48.307960, -105.101750)" ) AND ( beginPosition:[2015-06-23T00:00:00.000Z TO NOW] AND endPosition:[2015-06-23T00:00:00.000Z TO NOW] ) AND (platformname:Sentinel-2)'

  * Sentinel-2, cloud cover within a given range
'( footprint:"Intersects(48.307960, -105.101750)" ) AND ( beginPosition:[2015-06-23T00:00:00.000Z TO NOW] AND endPosition:[2015-06-23T00:00:00.000Z TO NOW] ) AND (platformname:Sentinel-2 AND cloudcoverpercentage:[0 TO 30])'


Example to query data

  query_sentinel.sh -u YOUR_USERNAME -p YOUR_PASSWORD -o PREFIX_TO_YOUR_OUTPUT_LIST '( footprint:"Intersects(48.307960, -105.101750)" ) AND ( beginPosition:[2015-06-23T00:00:00.000Z TO NOW] AND endPosition:[2015-06-23T00:00:00.000Z TO NOW] ) AND (platformname:Sentinel-2 AND cloudcoverpercentage:[0 TO 30])'

EOF

DISKSPACE=-1
META=0
QUIET=0
OPTS=`getopt -o u:p:o:d:M:q --long user:,password:,outprefix:,quiet,disk:,meta -n 'query_sentinel.sh' -- "$@"`
if [[ $? != 0 ]]; then echo "Failed parsing options." >&2 ; echo "${USAGE}" ; exit 1 ; fi
substr_exist ()
{
    # substr_exist substr str
    # local substr=$(echo ${1} | sed 's/-/\\-/g')
    echo "${2}" | grep -q "${1}"
    if [[ $? -eq 0 ]]; then
        echo 1
        return 1
    else
        echo 0
        return 0
    fi
}

if [[ $(substr_exist "\-\-user" "${OPTS}") -eq 0 && $(substr_exist "\-\u" "${OPTS}") -eq 0 ]]; then echo "No -u or --user" ; echo "${USAGE}" ; exit 1 ; fi
if [[ $(substr_exist "\-\-password" "${OPTS}") -eq 0 && $(substr_exist "\-\p" "${OPTS}") -eq 0 ]]; then echo "No --password" ; echo "${USAGE}" ; exit 1 ; fi
if [[ $(substr_exist "\-\-outprefix" "${OPTS}") -eq 0 && $(substr_exist "\-\o" "${OPTS}") -eq 0  ]]; then echo "No --outprefix" ; echo "${USAGE}" ; exit 1 ; fi

eval set -- "${OPTS}"
while true;
do
    case "${1}" in
        -u | --user )
            case "${2}" in
                "") shift 2 ;;
                *) USER=${2} ; shift 2 ;;
            esac ;;
        -p | --password )
            case "${2}" in
                "") shift 2 ;;
                *) PSW=${2} ; shift 2 ;;
            esac ;;
        -o | --outprefix )
            case "${2}" in
                "") shift 2 ;;
                *) OUTPREFIX=${2} ; shift 2 ;;
            esac ;;
        -d | --disk )
            case "${2}" in
                "") shift 2 ;;
                *) DISKSPACE=${2} ; shift 2 ;;
            esac ;;
	-M | --meta )
	    META=1 ; shift ;;
        -q | --quiet )
            QUIET=1 ; shift ;;
        -- ) shift ; break ;;
        * ) break ;;
    esac
done
MINPARAMS=1
if [[ ${#} < ${MINPARAMS} ]]; then
    echo "${USAGE}"
    exit 1
fi
REQUEST_STR=${1}

CMD_DIR=$(dirname ${0})

QUERY_PREFIX='https://scihub.copernicus.eu/dhus/search?q='
QUERY_SUFFIX=''
QUERY_REC_LIMIT=100

# check if the outprefix is a directory, which is NOT allowed!
if [[ -d ${OUTPREFIX} ]]; then
    echo "${OUTPREFIX} is a directory, NOT a valid prefix for output file of found record!"
    exit 1
fi

# additional files to write for query
QUERY_RESULT="${OUTPREFIX}.query.raw"
LOG_FILE="${OUTPREFIX}.query.log"
META_CSV="${OUTPREFIX}_meta.csv"

# calculate disk space
DISKSPACE=$(echo ${DISKSPACE} | bc)
if [[ ${DISKSPACE} -eq -1 ]]; then
    DISKSPACE_INFO="No space is given and all found images listed in one file."
else
    DISKSPACE_INFO="${DISKSPACE} bytes to hold each split of the found images."
fi

cat << EOF
Query string to send to Sentinel data hub:
  ${REQUEST_STR}

Prefix of output file of list of found data: 
  ${OUTPREFIX}

User name: 
  ${USER}

Password: 
  ${PSW}

Free diskspace: 
  ${DISKSPACE_INFO}

---------------------------------------------

Additional output files from the query will be in the same folder as your ${OUTPREFIX}
1. log file of wget query: ${LOG_FILE} 
2. raw outputs from the query: ${QUERY_RESULT}
3. metadata of found image records from the query if choosing to output: ${META_CSV}

EOF

if [[ ${QUIET} -eq 0 ]]; then
    # ask user to confirm the query
    echo "All correct to go? (Y/n)"

    read YN
    if [[ "${YN}" == "Y" ]]; then
        # ready to go
        echo "Start querying ..."
    else
        exit 1
    fi
fi

# set up output folder if it does not exist yet
OUTDIR=$(echo ${OUTPREFIX} | rev | cut -d '/' -f2- | rev)
if [[ ! -d ${OUTDIR} ]]; then
    mkdir -p ${OUTDIR}
fi

GOT_ALL=0
START_REC=0
QUERY_CNT=0
QUERY_MAX=10000
# Query from the server
> ${QUERY_RESULT}
if [[ ${META} -eq 1 ]]; then
	> ${META_CSV}
fi

while [[ ${GOT_ALL} -eq 0 ]]; do
    QUERY_CNT=$((QUERY_CNT+1))
    QUERY_STR="${QUERY_PREFIX}${REQUEST_STR}${QUERY_SUFFIX}&rows=${QUERY_REC_LIMIT}&start=${START_REC}"
    echo -n "Start querying records at ${START_REC} ... "
    wget --no-check-certificate --user=${USER} --password=${PSW} --output-file="${LOG_FILE}" -O "${QUERY_RESULT}.tmp" "${QUERY_STR}"
    WGET_EXIT=$?
    if [[ ${WGET_EXIT} -ne 0 ]]; then
        if [[ ${QUERY_CNT} -lt ${QUERY_MAX} ]]; then
            TMP=$((60*(QUERY_CNT)))
            echo
            echo "wget error, server may be temporarily down. Restart querying from record ${START_REC} after waiting ${TMP} seconds"
            sleep ${TMP}
            continue
        else
            echo
            echo "wget error and retry times has reached ${QUERY_MAX}. Check the server and try it later"
            rm -f "${QUERY_RESULT}.tmp"
            exit 2  
        fi
    fi
    if [[ ${QUERY_CNT} -eq 1 ]]; then
        NREC_TOT=$(grep "<opensearch:totalResults>" "${QUERY_RESULT}.tmp" | cut -f 2 -d'>' | cut -f 1 -d'<')
    fi

    if [[ ${META} -eq 1 ]]; then
	    csv_head=0
	    if [[ ${START_REC} -eq 0 ]]; then
		    csv_head=1
	    fi
	    ${CMD_DIR}/parse_query_result_xml.sh ${QUERY_RESULT}.tmp ${META_CSV}.${START_REC} ${csv_head}
	    cat ${META_CSV}.${START_REC} >> ${META_CSV}

# 	    if [[ ${START_REC} -eq 700 ]]; then exit; fi

	    rm -f ${META_CSV}.${START_REC}
    fi

    cat "${QUERY_RESULT}.tmp" >> "${QUERY_RESULT}"
    echo "done"
    START_REC=$((${START_REC}+${QUERY_REC_LIMIT}))
    if [[ ${START_REC} -ge ${NREC_TOT} ]]; then
        GOT_ALL=1
    fi
    # wait for a bit randomly to avoid server from blocking us
    sleep $((RANDOM%30))
done
rm -f "${QUERY_RESULT}.tmp"

echo "Parsing query results ..."
# find the start line of the found entries
TMP=$(grep -m 1 -n '<entry>' ${QUERY_RESULT} | cut -d':' -f1 | head -n 1)
tail -n +"${TMP}" ${QUERY_RESULT} > "${OUTPREFIX}.tmp"
# echo ${ENTRIES}
# output the download link to a list
cat "${OUTPREFIX}.tmp" | grep '<link href=' | cut -f2 -d'"' > "${OUTPREFIX}.tmp1"
cat "${OUTPREFIX}.tmp" | grep '<title' | grep -v '<title>Sentinels' | cut -f2 -d'>' | cut -d'<' -f1 > "${OUTPREFIX}.tmp2"
paste -d ' ' "${OUTPREFIX}.tmp1" "${OUTPREFIX}.tmp2" > ${OUTPREFIX}
rm -rf "${OUTPREFIX}.tmp" "${OUTPREFIX}.tmp1" "${OUTPREFIX}.tmp2"
# get number of file
N=`cat $OUTPREFIX | wc -l`
echo $N files found

if [[ ${DISKSPACE} -eq -1 ]]; then
    mv ${OUTPREFIX} ${OUTPREFIX}.txt
    echo "No disk space specified; All found in a single list file"
    echo "    ${OUTPREFIX}.txt"
    exit
fi

BYTE2GB=$((1024*1024*1024))

ACCSIZE=0
LISTIND=1
FIND=0
# if [[ -w ${OUTPREFIX}_${LISTIND}.txt ]]; then
#     rm -f ${OUTPREFIX}_${LISTIND}.txt
# fi
while true; do
    # PS4='+ $(date "+%s.%N")\011 '
    # exec 3>&2 2> ./profiling.$$.log
    # set -x
    read -r data_url <&4 || break
    FIND=$((FIND+1))
    echo -ne "Sorting # ${FIND} / ${N} file ... "
    if [[ -r ${OUTPREFIX}_${LISTIND}.txt ]]; then
        TMPSTR=$(echo "${data_url}" | cut -f2 -d ' ')
        TMPSTR=$(grep ${TMPSTR} ${OUTPREFIX}_${LISTIND}.txt)
        if [[ ! -z ${TMPSTR} ]]; then
            CONTLEN=$(echo ${TMPSTR} | cut -f3 -d ' ')
            ACCSIZE=$((ACCSIZE+CONTLEN))
            echo "sorted, skip"
            continue
        fi
    fi
    # the following way to get ContentLength is slow and the bottleneck... Haven't found a better way to do this in pure shell script
    qurl=$(echo ${data_url} | cut -f1 -d ' ')
    qurl=${qurl%\/\$value}
    
    for ((i=0; i<${QUERY_MAX}; i++)); do
        wget --no-check-certificate --user ${USER} --password ${PSW} -q -O "${OUTPREFIX}.tmp" "${qurl}"
        WGET_EXIT=$?
        if [[ ${WGET_EXIT} -ne 0 ]]; then
            TMP=$((60*(i+1)))
            echo "wget error when querying meta info, retry after waiting ${TMP} seconds"
            sleep ${TMP}
            echo -ne "Sorting # ${FIND} / ${N} file ... "
        else
            break
        fi
    done
    if [[ ${WGET_EXIT} -ne 0 ]]; then
        echo
        echo "wget error when querying meta info for sorting ${FIND} file. "
        echo "Retry times has reached ${QUERY_MAX}. Check the server and try it later"
        rm -f "${OUTPREFIX}.tmp"
        exit 3
    fi

    CONTLEN=$(cat "${OUTPREFIX}.tmp" | tr 'd:' '\n' | grep 'ContentLength' | cut -d '>' -f 2 | cut -d '<' -f 1)
    ACCSIZE=$((ACCSIZE+CONTLEN))
    if [[ $(echo "${ACCSIZE} < ${DISKSPACE}" | bc) -eq 1 ]]; then
        echo ${data_url}" "${CONTLEN} >> ${OUTPREFIX}_${LISTIND}.txt
        echo "done"
    else
        TOTSIZE=$(((${ACCSIZE}+${BYTE2GB})/${BYTE2GB}))
        LISTIND=$((LISTIND+1))
        ACCSIZE=0
        # if [[ -w ${OUTPREFIX}_${LISTIND}.txt ]]; then
        #         rm -f ${OUTPREFIX}_${LISTIND}.txt
        # fi
        if [[ -r ${OUTPREFIX}_${LISTIND}.txt ]]; then
            TMPSTR=$(echo "${data_url}" | cut -f2 -d ' ')
            TMPSTR=$(grep ${TMPSTR} ${OUTPREFIX}_${LISTIND}.txt)
            if [[ ! -z ${TMPSTR} ]]; then
                CONTLEN=$(echo ${TMPSTR} | cut -f3 -d ' ')
                ACCSIZE=$((ACCSIZE+CONTLEN))
                echo "sorted, skip"
                echo "Total size of data files in the #$((LISTIND-1)) list = ${TOTSIZE} GB"
                continue
            fi
        fi
        echo ${data_url}" "${CONTLEN} >> ${OUTPREFIX}_${LISTIND}.txt
        echo "done"
        echo "Total size of data files in the #$((LISTIND-1)) list = ${TOTSIZE} GB"
    fi
    sleep $((RANDOM%5))
    # if [[ "${FIND}" -gt 20 ]]; then
    #       break
    # fi 
    # set +x
    # exec 2>&3 3>&-
done 4<$OUTPREFIX
rm -f "${OUTPREFIX}.tmp"

echo "Total size of data to be downloaded in the #${LISTIND} list = $(((${ACCSIZE}+${BYTE2GB})/${BYTE2GB})) GB"

rm -f $OUTPREFIX
