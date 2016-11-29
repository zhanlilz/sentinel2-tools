#!/bin/bash

# DATADIR="../scihub.copernicus.eu/dhus/odata/v1"
# DLLIST="../meta-files/2016_jan_dl_aa.txt"
# MSLIST="../meta-files/missing_file_dl_list.txt"

# parse input arguments
MINPARAMS=3
if [[ $# -lt "$MINPARAMS" ]]; then
  echo
  echo "Find files missing from the dowload link list"
  echo "Usage: "
  echo "${0} directory_to_sentinel_data_folders path_to_list_of_download_links path_to_list_of_missing_files_url"
  exit 1
fi

if [[ -n "${1}" ]]; then
	DATADIR="${1}"
	echo "Directory to the sentinel data folders: $DATADIR"
fi 

if [[ -n "${2}" ]]; then
	DLLIST="${2}"	
	echo "File of list of download links: $DLLIST"
fi 

if [[ -n "${3}" ]]; then
	MSLIST="${3}"	
	echo "File of list of missing file url: $MSLIST"
fi 

TMPPREFIX="${0}"_"$(date +%s)"

# ----------------------------------
# find missing $value, the data file
TMP1="${TMPPREFIX}_tmp_data_list.txt"
TMP2="${TMPPREFIX}_tmp_dl_list.txt"
TMP3="${TMPPREFIX}_tmp_missing_file_list.txt"

find ${DATADIR} -name "\$value" > ${TMP1}.tmp
rev ${TMP1}.tmp | cut -d'/' -f2 | rev > ${TMP1}
cat $DLLIST | cut -d' ' -f1 | tr -s '/' | rev | cut -d'/' -f2 | rev > ${TMP2}
sort ${TMP2} ${TMP1} | uniq -u > ${TMP3}

# just in case the DATADIR has some files that are not included in the download url list
sort ${TMP2} ${TMP1} | uniq -d > ${TMPPREFIX}.tmp.1 # exist files that have been downloaded
sort ${TMPPREFIX}.tmp.1 ${TMP1} | uniq -u > ${TMPPREFIX}.tmp.2 # files in the data folder but not in the download url list
sort ${TMPPREFIX}.tmp.2 ${TMP3} | uniq -u > ${TMPPREFIX}.tmp.3 # files in the download url list that have NOT been downloaded
cat ${TMPPREFIX}.tmp.3 | sed 's/^/https:\/\/scihub.copernicus.eu\/dhus\/odata\/v1\//' | sed 's/$/\/\$value/' > $MSLIST

echo
echo Number of data files missing = `cat $MSLIST | wc -l`

if [[ ! -z $(cat ${TMPPREFIX}.tmp.2) ]]; then
	echo
	echo "You have data files in ${DATADIR} that are NOT in your URL list ${DLLIST}"
	echo "Double check if your data folder is the right one for the URL list"
	cat ${TMPPREFIX}.tmp.2
fi

TMP_DATA_LIST=${TMP1}
rm -f ${TMP1}.tmp ${TMP2} ${TMP3} ${TMPPREFIX}.tmp.1 ${TMPPREFIX}.tmp.2 ${TMPPREFIX}.tmp.3

# ------------------------------------------
# find missing $value.md5, the checksum file
TMP1="${TMPPREFIX}_tmp_md5_list.txt"
TMP2="${TMPPREFIX}_tmp_dl_list.txt"
TMP3="${TMPPREFIX}_tmp_missing_file_list.txt"

find ${DATADIR} -name "\$value.md5" > ${TMP1}.tmp
rev ${TMP1}.tmp | cut -d'/' -f2 | rev > ${TMP1} # ${TMPPREFIX}.tmp.0

cat $DLLIST | cut -d' ' -f1 | tr -s '/' | rev | cut -d'/' -f2 | rev > ${TMP2}
sort ${TMP2} ${TMP1} | uniq -u > ${TMP3}

# just in case the DATADIR has some files that are not included in the download url list
sort ${TMP2} ${TMP1} | uniq -d > ${TMPPREFIX}.tmp.1 # exist files that have been downloaded
sort ${TMPPREFIX}.tmp.1 ${TMP1} | uniq -u > ${TMPPREFIX}.tmp.2 # files in the data folder but not in the download url list
sort ${TMPPREFIX}.tmp.2 ${TMP3} | uniq -u > ${TMPPREFIX}.tmp.3 # files in the download url list that have NOT been downloaded

sort ${TMPPREFIX}.tmp.3 ${TMP_DATA_LIST} | uniq -d > ${TMPPREFIX}.tmp.4

cat ${TMPPREFIX}.tmp.4 | sed 's/^/https:\/\/scihub.copernicus.eu\/dhus\/odata\/v1\//' | sed 's/$/\/\$value/' > ${TMPPREFIX}.tmp.5

# make sure these missing md5 files' links are not among the missing data files links
sort ${TMPPREFIX}.tmp.5 $MSLIST | uniq -d > ${TMPPREFIX}.tmp.6
sort ${TMPPREFIX}.tmp.5 ${TMPPREFIX}.tmp.6 | uniq -u > ${MSLIST}.md5

echo
echo Number of checksum files missing = `cat ${MSLIST}.md5 | wc -l`

if [[ ! -z $(cat ${TMPPREFIX}.tmp.2) ]]; then
	echo
	echo "You have checksum files in ${DATADIR} that are NOT in your URL list ${DLLIST}"
	echo "Double check if your data folder is the right one for the URL list"
	cat ${TMPPREFIX}.tmp.2
fi

rm -f ${TMP1} ${TMP1}.tmp ${TMP2} ${TMP3} ${TMPPREFIX}.tmp.0 ${TMPPREFIX}.tmp.1 ${TMPPREFIX}.tmp.2 ${TMPPREFIX}.tmp.3 ${TMPPREFIX}.tmp.4 ${TMPPREFIX}.tmp.5 ${TMPPREFIX}.tmp.6 ${TMP_DATA_LIST}