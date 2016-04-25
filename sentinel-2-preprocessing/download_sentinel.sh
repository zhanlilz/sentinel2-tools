#!/bin/bash

read -d '' USAGE <<EOF
Usage: download_sentinel.sh [options] URL_LIST_FILE DIRECTORY

Download sentinel data to the directory, DIRECTORY given the list of
download URLs in the file, URL_LIST_FILE, one URL per line

Options:

  -c, --checksum, download checksum files

  -C, --checksum-only, ONLY download checksum files

EOF

CKSUM=0
CKSUM_ONLY=0
OPTS=`getopt -o cC --long checksum,checksum-only -n 'download_sentinel.sh' -- "$@"`
if [[ $? != 0 ]]; then echo "Failed parsing options." >&2 ; exit 1 ; fi
eval set -- "${OPTS}"
while true;
do
    case "${1}" in
        -c | --checksum )
            case "${2}" in
                "") shift 2 ;;
                *) CKSUM=1 ; shift 2 ;;
            esac ;;
        -C | --checksum-only )
            case "${2}" in
                "") shift 2 ;;
                *) CKSUM_ONLY=1 ; shift 2 ;;
            esac ;;
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

USER=zhan.li
PSW=1986721615
LOG="./.download_sentinel.log"

read_dom ()
{
    local IFS=\>
    read -d \< ENTITY CONTENT
}

> ${LOG}
while IFS= read -r url
do
    url=$(echo ${url} | cut -f1 -d ' ')
    qurl=${url%\/\$value}
    tmp=$(wget -a ${LOG} -O - --wait=15 -t 0 --waitretry=30 --no-check-certificate --user=${USER} --password=${PSW} ${qurl})
    echo
    echo "Metadata query returns:"
    echo "${tmp}"
    if [[ -z ${tmp} ]]; then
        echo
        echo "No return from query the meta data of this download URL: ${qurl}"
        continue
    fi

    while read_dom;
    do
        if [[ ${ENTITY} = "d:Name" ]]; then
            outname=${CONTENT}
            break
        fi
    done < <(echo ${tmp})

    if [[ ${CKSUM_ONLY} -eq 0 ]]; then
        echo
        echo "Downloading ${outname} from ${url} to ${OUTDIR}"
        wget -a ${LOG} -c -O ${OUTDIR}/${outname}.zip --wait=15 -t 0 --waitretry=30 --no-check-certificate --user=${USER} --password=${PSW} ${url}
    fi
    if [[ (${CKSUM_ONLY} -eq 1) || (${CKSUM} -eq 1) ]]; then
        echo
        echo "Downloading checksum for ${outname} to ${OUTDIR}"
        wget -a ${LOG} -c -O ${OUTDIR}/${outname}.md5 --wait=15 -t 0 --waitretry=30 --no-check-certificate --user=${USER} --password=${PSW} "${qurl}/Checksum/Value/\$value"
    fi

done < "${IN_URL_FILE}"    
