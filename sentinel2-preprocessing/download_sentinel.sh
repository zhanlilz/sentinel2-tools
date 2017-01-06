#!/bin/bash

read -d '' USAGE <<EOF
Usage: download_sentinel_bnu.sh [options] URL_LIST_FILE DIRECTORY

Download sentinel data to the directory, DIRECTORY given the list of
download URLs in the file, URL_LIST_FILE, one URL per line

Options:

  --user="your_user_name", required: specify your username for Sentinel Data Hub; 

  --password="your_password", required: specify your password for Sentinel Data Hub;

  -l, --log="log_file", optional: specify a log file to record the downloaded links (100%
        finish). All newly downloaded links (100% finish) will be appended to this log
        file.

  -I, --ignore_dl="ignored_dl_list", optional, specify a file of a list of
        data records to be ignored in the downloading. These are usually the data
        records in the given URL_LIST_FILE that have been downloaded and validated.

  -c, --checksum, optional but strongly recommended: download checksum files

  -C, --checksum_only, optional: ONLY download checksum files

  -r, --recursive, optional but should used by current BNU data downloading
        protocol: download files using wget "--recursive" option

  -v, --verbose, turn on verbose mode

EOF

CKSUM=0 # download checksum to a file for each Sentinel data file
CKSUM_ONLY=0 # ONLY download checksum, no data files
RECURSIVE=0 # download files using wget --recursive option
VERBOSE=0 # verbose mode, output more detailed information to the screen
DLLOG=""
IGNORELOG=""
OPTS=`getopt -o cCrvl:I: --long checksum,checksum_only,recursive,user:,password:,verbose,log:,ignore_dl -n 'download_sentinel.sh' -- "$@"`
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
        -C | --checksum_only )
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
        -- ) shift ; break ;;
        * ) break ;;
    esac
done
MINPARAMS=2
if [[ ${#} < ${MINPARAMS} ]]; then
    echo "Missing argument!"
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
MIN_SLEEPTIME=10
MAX_SLEEPTIME=30
> ${LOG}
NUM_URL=$(cat "${IN_URL_FILE}" | wc -l)
URL_IND=0
TRIES=10

ALLDONE=0

SERVER_ERR_CNT=0

while IFS= read -r url
do
    url=$(echo ${url} | cut -f1 -d ' ')
    
    qurl=${url%\/\$value}
    
    URL_IND=$((URL_IND+1))
    
    if [[ ${RECURSIVE} -eq 0 ]]; then        
        sleep $((RANDOM%(MAX_SLEEPTIME-MIN_SLEEPTIME)+MIN_SLEEPTIME))
        tmp=$(wget -a ${LOG} -O - --wait=15 -t ${TRIES} --waitretry=${WAITRETRY} --random-wait --no-check-certificate --user=${USER} --password=${PSW} ${qurl})
        echo
        if [[ ${VERBOSE} -eq 1 ]]; then
            echo "Metadata query returns:"
            echo "${tmp}"
        fi
        if [[ -z ${tmp} ]]; then
            echo "No return from query the meta data of this download URL: ${qurl}"
            ALLDONE=1

            # Check the reason of no return, is it because the server is down again?
            wget -a ${LOG} -O /dev/null --wait=15 -t ${TRIES} --waitretry=${WAITRETRY} --random-wait --no-check-certificate --user=${USER} --password=${PSW} ${qurl}
            WGET_EXIT=$?
            if [[ ${WGET_EXIT} -ge 8 ]]; then
                SERVER_ERR_CNT=$((SERVER_ERR_CNT+1))
                echo "Server issued error, waiting $((SERVER_ERR_CNT*60)) seconds for server to recover ..."
                sleep $((SERVER_ERR_CNT*60))                
            fi
            
            continue
        fi

        while read_dom;
        do
            if [[ ${ENTITY} = "d:Name" ]]; then
                outname=${CONTENT}
                break
            fi
        done < <(echo ${tmp})

        if [[ (${CKSUM_ONLY} -ne 1) ]] && [[ -r ${IGNORELOG} ]]; then
            PRODUCT_ID=${outname}
            grep -q "${PRODUCT_ID}" ${IGNORELOG}
            if [[ $? -eq 0 ]]; then
                echo "Downloaded and validated ${URL_IND} / ${NUM_URL}: ${url}, skip"
                continue
            fi
        fi
        
        if [[ ${CKSUM_ONLY} -eq 0 ]]; then
            GOT_DATA=0
        else
            GOT_DATA=1
        fi
        if [[ ${CKSUM_ONLY} -eq 0 ]]; then
            echo -n "Downloading ${URL_IND} / ${NUM_URL}: ${outname} from ${url} to ${OUTDIR}"
            sleep $((RANDOM%(MAX_SLEEPTIME-MIN_SLEEPTIME)+MIN_SLEEPTIME))
            wget -a ${LOG} -c -O ${OUTDIR}/${outname}.zip --wait=15 -t 0 --waitretry=${WAITRETRY} --random-wait --no-check-certificate --user=${USER} --password=${PSW} ${url}
            WGET_EXIT=$?
            if [[ ${WGET_EXIT} -eq 0 ]]; then
                echo ", succeeded"
                GOT_DATA=1
                if [[ ${CKSUM} -eq 0 ]]; then
                    # no need to download checksum, we are done with this record
                    echo ${outname} >> ${DLLOG}
                fi
                SERVER_ERR_CNT=0
            else
                echo ", failed, wget exit ${WGET_EXIT}"
                ALLDONE=2
                if [[ ${WGET_EXIT} -ge 8 ]]; then
                    SERVER_ERR_CNT=$((SERVER_ERR_CNT+1))
                    echo "Server issued error, waiting $((SERVER_ERR_CNT*60)) seconds for server to recover ..."
                    sleep $((SERVER_ERR_CNT*60))
                fi
            fi
        fi
        if [[ (${CKSUM_ONLY} -eq 1) || (${CKSUM} -eq 1) ]]; then
            echo -n "Downloading ${URL_IND} / ${NUM_URL}: checksum for ${outname} to ${OUTDIR}"
            sleep $((RANDOM%(MAX_SLEEPTIME-MIN_SLEEPTIME)+MIN_SLEEPTIME))
            wget -a ${LOG} -c -O ${OUTDIR}/${outname}.md5 --wait=15 -t 0 --waitretry=${WAITRETRY} --random-wait --no-check-certificate --user=${USER} --password=${PSW} "${qurl}/Checksum/Value/\$value"
            WGET_EXIT=$?
            if [[ ${WGET_EXIT} -eq 0 ]]; then
                echo ", succeeded"
                if [[ ${GOT_DATA} -eq 1 ]]; then
                    echo ${outname} >> ${DLLOG}     
                fi
            else
                echo ", failed, wget exit ${WGET_EXIT}"
                ALLDONE=3
            fi          
        fi
    else
        echo

        if [[ (${CKSUM_ONLY} -ne 1) ]] && [[ -r ${IGNORELOG} ]]; then
            PRODUCT_ID=$(echo ${url} | rev | cut -f 2 -d "/" | rev | cut -f 2 -d "'")        
            grep -q "${PRODUCT_ID}" ${IGNORELOG}
            if [[ $? -eq 0 ]]; then
                echo "Downloaded and validated ${URL_IND} / ${NUM_URL}: ${url}, skip"
                continue
            fi
        fi
        
        if [[ ${CKSUM_ONLY} -eq 0 ]]; then
            GOT_DATA=0
        else
            GOT_DATA=1
        fi              
        if [[ ${CKSUM_ONLY} -eq 0 ]]; then
            echo -n "Downloading ${URL_IND} / ${NUM_URL}: data from ${url} to ${OUTDIR}"
            sleep $((RANDOM%(MAX_SLEEPTIME-MIN_SLEEPTIME)+MIN_SLEEPTIME))
            wget --directory-prefix "${OUTDIR}" -a ${LOG} -rc --wait=15 -t ${TRIES} --waitretry=${WAITRETRY} --random-wait --no-check-certificate --user=${USER} --password=${PSW} ${url}
            WGET_EXIT=$?
            if [[ ${WGET_EXIT} -eq 0 ]]; then
                echo ", succeeded"
                GOT_DATA=1
                if [[ ${CKSUM} -eq 0 ]]; then
                    echo ${url} >> ${DLLOG}
                fi
                SERVER_ERR_CNT=0
            else
                echo ", failed, wget exit ${WGET_EXIT}"
                ALLDONE=2
                if [[ ${WGET_EXIT} -ge 8 ]]; then
                    SERVER_ERR_CNT=$((SERVER_ERR_CNT+1))
                    echo "Server issued error, waiting $((SERVER_ERR_CNT*60)) seconds for server to recover ..."
                    sleep $((SERVER_ERR_CNT*60))
                fi
            fi
        fi
        if [[ (${CKSUM_ONLY} -eq 1) || (${CKSUM} -eq 1) ]]; then
            tmpprefix="${OUTDIR}/${url#"https://"}"
            tmpdir=${tmpprefix/\$value}
            if [[ ! -d ${tmpdir} ]]; then
                mkdir -p ${tmpdir}
            fi
            echo -n "Downloading ${URL_IND} / ${NUM_URL}: checksum to ${tmpprefix}.md5"
            sleep $((RANDOM%(MAX_SLEEPTIME-MIN_SLEEPTIME)+MIN_SLEEPTIME))
            wget -a ${LOG} -c -O "${tmpprefix}.md5" --wait=15 -t ${TRIES} --waitretry=${WAITRETRY} --random-wait --no-check-certificate --user=${USER} --password=${PSW} "${qurl}/Checksum/Value/\$value"
            WGET_EXIT=$?
            if [[ ${WGET_EXIT} -eq 0 ]]; then
                echo ", succeeded"
                if [[ ${GOT_DATA} -eq 1 ]]; then
                    echo ${url} >> ${DLLOG}
                fi
                SERVER_ERR_CNT=0
            else
                echo ", failed, wget exit ${WGET_EXIT}"
                ALLDONE=3
            fi          
        fi
    fi
done < "${IN_URL_FILE}"    

exit ${ALLDONE}
