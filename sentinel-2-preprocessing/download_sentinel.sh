#!/bin/bash

read -d '' USAGE <<EOF
Usage: download_sentinel_bnu.sh [options] URL_LIST_FILE DIRECTORY

Download sentinel data to the directory, DIRECTORY given the list of
download URLs in the file, URL_LIST_FILE, one URL per line

Options:

  --user, required: specify your username for Sentinel Data Hub; e.g. --user
        "your_user_name"

  --password, required: specify your password for Sentinel Data Hub; e.g.
        --password "your_password"

  -c, --checksum, optional but strongly recommended: download checksum files

  -C, --checksum-only, optional: ONLY download checksum files

  -r, --recursive, optional: download files using wget "--recursive" option

EOF

CKSUM=0 # download checksum to a file for each Sentinel data file
CKSUM_ONLY=0 # ONLY download checksum, no data files
RECURSIVE=0 # download files using wget --recursive option
VERBOSE=0 # verbose mode, output more detailed information to the screen
OPTS=`getopt -o cCrv --long checksum,checksum-only,recursive,user:,password:,verbose -n 'download_sentinel.sh' -- "$@"`
if [[ $? != 0 ]]; then echo "Failed parsing options." >&2 ; exit 1 ; fi
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
        -c | --checksum )
                        CKSUM=1 ; shift ;;
        -C | --checksum-only )
                        CKSUM_ONLY=1 ; shift ;;
        -r | --recursive )
                        RECURSIVE=1 ; shift ;;
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

if [[ -z ${USER} || -z ${PSW} || -z ${IN_URL_FILE} || -z ${OUTDIR} ]]; then
        echo "Missing argument!"
        echo
        echo "${USAGE}"
        exit 1
fi 

if [[ ! -f ${IN_URL_FILE} ]]; then
        echo "Input URL file ${IN_URL_FILE} does not exist or cannot be read!"
        exit 1
fi

if [[ ! -d ${OUTDIR} ]]; then
        mkdir -p "${OUTDIR}"
fi

LOG="${OUTDIR}/.download_sentinel.log"

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
    
    if [[ ${RECURSIVE} -eq 0 ]]; then
            tmp=$(wget -a ${LOG} -O - --wait=15 -t 0 --waitretry=30 --no-check-certificate --user=${USER} --password=${PSW} ${qurl})
            echo
            if [[ ${VERBOSE} -eq 1 ]]; then
                echo "Metadata query returns:"
                echo "${tmp}"
            fi
            if [[ -z ${tmp} ]]; then
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
                echo "Downloading ${outname} from ${url} to ${OUTDIR}"
                wget -a ${LOG} -c -O ${OUTDIR}/${outname}.zip --wait=15 -t 0 --waitretry=30 --no-check-certificate --user=${USER} --password=${PSW} ${url}
            fi
            if [[ (${CKSUM_ONLY} -eq 1) || (${CKSUM} -eq 1) ]]; then
                echo "Downloading checksum for ${outname} to ${OUTDIR}"
                wget -a ${LOG} -c -O ${OUTDIR}/${outname}.md5 --wait=15 -t 0 --waitretry=30 --no-check-certificate --user=${USER} --password=${PSW} "${qurl}/Checksum/Value/\$value"
            fi
    else
                echo
                if [[ ${CKSUM_ONLY} -eq 0 ]]; then
                echo "Downloading data from ${url} to ${OUTDIR}"
                wget --directory-prefix "${OUTDIR}" -a ${LOG} -rc --wait=15 -t 0 --waitretry=30 --no-check-certificate --user=${USER} --password=${PSW} ${url}
                fi
            if [[ (${CKSUM_ONLY} -eq 1) || (${CKSUM} -eq 1) ]]; then
                tmpprefix="${OUTDIR}/${url#"https://"}"
                tmpdir=${tmpprefix/\$value}
                if [[ ! -d ${tmpdir} ]]; then
                        mkdir -p ${tmpdir}
                fi
                echo "Downloading checksum to ${tmpprefix}.md5"
                wget -a ${LOG} -c -O "${tmpprefix}.md5" --wait=15 -t 0 --waitretry=30 --no-check-certificate --user=${USER} --password=${PSW} "${qurl}/Checksum/Value/\$value"
            fi
    fi
done < "${IN_URL_FILE}"    
