#!/bin/bash
# read a query of Sentinel data search and output all the download links to a file for the input file to wget
# Zhan Li, zhan.li@umb.edu
# Created: Tue Mar  8 17:44:57 EST 2016

# -------------------------------------
### user inputs, change as you need ###
# the string for opensearch on Sentinel Data Hub. See instructions at https://scihub.copernicus.eu/twiki/do/view/SciHubUserGuide/5APIsAndBatchScripting#Open_Search
# QUERY_STR="https://scihub.copernicus.eu/dhus/search?q=( footprint:\"Intersects(POLYGON((-180 58.973437549923915,0 58.973437549923915,0 85.71249419049184,-180 85.71249419049184,-180 58.973437549923915)))\" OR footprint:\"Intersects(POLYGON((0 58.973437549923915,180 58.973437549923915,180 85.71249419049184,0 85.71249419049184,0 58.973437549923915)))\" ) AND ( beginPosition:[2016-01-01T00:00:00.000Z TO 2016-01-31T23:59:59.999Z] AND endPosition:[2016-01-01T00:00:00.000Z TO 2016-01-31T23:59:59.999Z] ) AND (platformname:Sentinel-1) AND (producttype:SLC OR producttype:GRD OR producttype:OCN)&rows=10000&start=0"
QUERY_STR="https://scihub.copernicus.eu/dhus/search?q=( footprint:\"Intersects(POLYGON((-180 58.973437549923915,0 58.973437549923915,0 85.71249419049184,-180 85.71249419049184,-180 58.973437549923915)))\" OR footprint:\"Intersects(POLYGON((0 58.973437549923915,180 58.973437549923915,180 85.71249419049184,0 85.71249419049184,0 58.973437549923915)))\" ) AND ( beginPosition:[2016-01-01T00:00:00.000Z TO 2016-01-31T23:59:59.999Z] AND endPosition:[2016-01-01T00:00:00.000Z TO 2016-01-31T23:59:59.999Z] ) AND (platformname:Sentinel-2)&rows=10000&start=0"
# the prefix of output list of found files, including the file path
OUTPREFIX="../test-meta/test" # in the Disk D, directory "sentinel" -> "test-meta", output file will start with prefix "test"
USER="zhan.li" # your user name on Sentinel Data Hub
PSW="your_pass_word" # your password on Sentinel Data Hub
DISKSPACE=$((16*1024*1024*1024*1024)) # 4T disk space, the disk space you have to hold the downloaded data before uncompressing them, in unit of bytes.
### end of user inputs ###
# -------------------------------------

# additional files to write for query
QUERY_RESULT="${OUTPREFIX}.query.raw"
LOG_FILE="${OUTPREFIX}.query.log"

# ask user to confirm the query
cat << EOF
Query string to send to Sentinel data hub:
  ${QUERY_STR}

Prefix of output file of list of found data: 
  ${OUTPREFIX}

User name: 
  ${USER}

Password: 
  ${PSW}

Free diskspace: 
  ${DISKSPACE}

---------------------------------------------

Additional output files from the query will be in the same folder as your ${OUTPREFIX}
1. log file of wget query: ${LOG_FILE} 
2. raw outputs from the query: ${QUERY_RESULT}

All correct to go? (Y/n)
EOF

read YN
if [[ "${YN}" == "Y" ]]; then
    # ready to go
    echo "Start querying ..."
else
    exit 1
fi

# set up output folder if it does not exist yet
OUTDIR=$(echo ${OUTPREFIX} | rev | cut -d '/' -f2- | rev)
if [[ ! -d ${OUTDIR} ]]; then
	mkdir -p ${OUTDIR}
fi

wget --no-check-certificate --user=${USER} --password=${PSW} --output-file="${LOG_FILE}" -O "${QUERY_RESULT}" "${QUERY_STR}"

echo "Parsing query results ..."
# find the start line of the found entries
TMP=$(grep -n '<entry>' ${QUERY_RESULT} | cut -d':' -f1 | head -n 1)
tail -n +"${TMP}" ${QUERY_RESULT} > "${OUTPREFIX}.tmp"
# echo ${ENTRIES}
# output the download link to a list
cat "${OUTPREFIX}.tmp" | grep '<link href=' | cut -f2 -d'"' > "${OUTPREFIX}.tmp1"
cat "${OUTPREFIX}.tmp" | grep '<title' | cut -f2 -d'>' | cut -d'<' -f1 > "${OUTPREFIX}.tmp2"
paste "${OUTPREFIX}.tmp1" "${OUTPREFIX}.tmp2" > ${OUTPREFIX}
rm -rf "${OUTPREFIX}.tmp" "${OUTPREFIX}.tmp1" "${OUTPREFIX}.tmp2"
# get number of file
N=`cat $OUTPREFIX | wc -l`
echo $N files found

BYTE2GB=$((1024*1024*1024))

ACCSIZE=0
LISTIND=1
FIND=0
if [[ -w ${OUTPREFIX}_${LISTIND}.txt ]]; then
    rm -f ${OUTPREFIX}_${LISTIND}.txt
fi
while true; do
# PS4='+ $(date "+%s.%N")\011 '
# exec 3>&2 2> ./profiling.$$.log
# set -x
    read -r data_url <&4 || break
    FIND=$((FIND+1))
    echo -ne "Sorting # ${FIND} / ${N} file ... \r"
    # the following way to get ContentLength is slow and the bottleneck... Haven't found a better way to do this in pure shell script
    qurl=$(echo ${data_url} | cut -f1 -d ' ')
    qurl=${qurl%\/\$value}
    tmp_size=$(wget --no-check-certificate --user ${USER} --password ${PSW} -q -O - "${qurl}" | tr 'd:' '\n' | grep 'ContentLength' | cut -d '>' -f 2 | cut -d '<' -f 1)
    ACCSIZE=$((ACCSIZE+tmp_size))
    if [[ "${ACCSIZE}" -lt "${DISKSPACE}" ]]; then
            echo ${data_url} >> ${OUTPREFIX}_${LISTIND}.txt
    else
            echo "Total size of data files in the #${LISTIND} list = $(((${ACCSIZE}+${BYTE2GB})/${BYTE2GB})) GB"
            LISTIND=$((LISTIND+1))
            ACCSIZE=0
            if [[ -w ${OUTPREFIX}_${LISTIND}.txt ]]; then
                    rm -f ${OUTPREFIX}_${LISTIND}.txt
            fi
            echo ${data_url} >> ${OUTPREFIX}_${LISTIND}.txt
    fi
    # if [[ "${FIND}" -gt 20 ]]; then
    #       break
    # fi 
# set +x
# exec 2>&3 3>&-
done 4<$OUTPREFIX

echo "Total size of data to be downloaded in the #${LISTIND} list = $(((${ACCSIZE}+${BYTE2GB})/${BYTE2GB})) GB"

rm -f $OUTPREFIX