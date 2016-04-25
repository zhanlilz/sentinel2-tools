#!/bin/bash
# read a query of Sentinel data search and output all the download links to a file for the input file to wget
# Zhan Li, zhan.li@umb.edu
# Created: Tue Mar  8 17:44:57 EST 2016

LAT=${1}
LON=${2}
OUTPREFIX=${3}
QUERY_RESULT=${4}
# LAT=40.125615
# LON=-105.2378
# OUTPREFIX="../meta-files/south_china_sea_20140701_20160315_s2"
# QUERY_RESULT="../meta-files/query_result"

USER="zhan.li"
PSW="1986721615"
DISKSPACE=$((4*1024*1024*1024*1024)) # 4T

QUERY_STR="https://scihub.copernicus.eu/dhus/search?q=( footprint:\"Intersects(${LAT}, ${LON})\" ) AND ( beginPosition:[2015-06-23T00:00:00.000Z TO 2015-12-31T23:59:59.999Z] AND endPosition:[2015-06-23T00:00:00.000Z TO 2015-12-31T23:59:59.999Z] ) AND (platformname:Sentinel-2)&rows=20000&start=0"

# # ask user to confirm the query
# cat << EOF
# Query string to send to Sentinel data hub:
#   ${QUERY_STR}

# Raw output from query will be saved in: 
#   ${QUERY_RESULT}

# Prefix of output file of list of found data: 
#   ${OUTPREFIX}

# User name: 
#   ${USER}

# Password: 
#   ${PSW}

# All correct to go? (Y/n)
# EOF
# read YN
# if [[ "${YN}" == "Y" ]]; then
#         # ready to go
#         echo ""
# else
#         exit 1
# fi


echo "Start querying ..."

wget --no-check-certificate --user=zhan.li --password=1986721615 --output-file=../meta-files/.log_query.log -O $QUERY_RESULT "${QUERY_STR}"

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

echo "Total size of data files in the #${LISTIND} list = $(((${ACCSIZE}+${BYTE2GB})/${BYTE2GB})) GB"

rm -f $OUTPREFIX