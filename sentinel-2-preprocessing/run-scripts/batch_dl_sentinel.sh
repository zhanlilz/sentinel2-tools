#!/bin/bash

IN_URLS=( \
"../meta-files/boulder_s2_2015_urls_1.txt" \
"../meta-files/boundville_s2_2015_urls_1.txt" \
"../meta-files/desert_rock_s2_2015_urls_1.txt" \
"../meta-files/fort_peck_s2_2015_urls_1.txt" \
"../meta-files/goodwin_creek_s2_2015_urls_1.txt" \
"../meta-files/penn_state_s2_2015_urls_1.txt" \
"../meta-files/sioux_falls_s2_2015_urls_1.txt" \
)
OUTDIRS=( \
"../sentinel-2-l1c/boulder" \
"../sentinel-2-l1c/boundville" \
"../sentinel-2-l1c/desert-rock" \
"../sentinel-2-l1c/fort-peck" \
"../sentinel-2-l1c/goodwin-creek" \
"../sentinel-2-l1c/penn-state" \
"../sentinel-2-l1c/sioux-falls" \
)

CMD="./download_sentinel.sh"

# read_dom ()
# {
#     local IFS=\>
#     read -d \< ENTITY CONTENT
# }

# dl_sentinel ()
# {
#     local IN_URL_FILE=${1}
#     local OUTDIR=${2}

#     USER=zhan.li
#     PSW=1986721615

#     while IFS= read -r url
#     do
#         local qurl=${url%/\$value}

#         tmp=$(wget -q -O - --wait=15 -t 0 --waitretry=30 --no-check-certificate --user=${USER} --password=${PSW} ${qurl})
#         echo "Metadata query returns:"
#         echo "${tmp}"
        
#         while read_dom;
#         do
#             if [[ ${ENTITY} = "d:Name" ]]; then
#                 local outname=${CONTENT}
#                 break
#             fi
#         done < <(echo ${tmp})

#         echo "Downloading ${outname} from ${url}"
#         wget -q -c -O ${OUTDIR}/${outname}.zip --wait=15 -t 0 --waitretry=30 --no-check-certificate --user=${USER} --password=${PSW} ${url}

#     done < "${IN_URL_FILE}"    
# }

for (( i=0; i<${#IN_URLS[@]}; ++i )); 
do
    if [[ ! -d ${OUTDIRS[i]} ]]; then
        mkdir -p ${OUTDIRS[i]}
    fi
    echo "Download files in the list: ${IN_URLS[i]}"
    # dl_sentinel ${IN_URLS[i]} ${OUTDIRS[i]}
    ${CMD} -c ${IN_URLS[i]} ${OUTDIRS[i]}
    echo 
done
