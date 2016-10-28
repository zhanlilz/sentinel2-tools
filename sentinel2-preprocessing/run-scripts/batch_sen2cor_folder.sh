#!/bin/bash

L1CDIRS=( \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l1c/boulder/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l1c/boundville/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l1c/desert-rock/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l1c/fort-peck/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l1c/goodwin-creek/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l1c/penn-state/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l1c/sioux-falls/" \
)

OUTDIRS=( \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l2a/boulder/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l2a/boundville/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l2a/desert-rock/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l2a/fort-peck/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l2a/goodwin-creek/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l2a/penn-state/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l2a/sioux-falls/" \
)

RESOLUTIONS=( \
20 20 20 20 20 20 20 \
)

GRANULES=( \
"13TDE" \
"16TCK" \
"11SNA" \
"13UDP" \
"15SYT,15SYU,16SBC,16SBD" \
"18TTL" \
"14TPP" \
)

CMD="../sen2cor_folder.sh"

for (( i=0; i<${#L1CDIRS[@]}; ++i )); 
do
    echo "Run atmoshperic correction on ${L1CDIRS[i]}"
    ${CMD} --resolution=${RESOLUTIONS[i]} -g"${GRANULES[i]}" -o"${OUTDIRS[i]}" ${L1CDIRS[i]}
done