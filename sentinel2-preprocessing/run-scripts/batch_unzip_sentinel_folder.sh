#!/bin/bash

# Unzip multiple folders of Sentinel-2 zip files

L1CDIRS=( \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l1c/boulder/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l1c/boundville/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l1c/desert-rock/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l1c/fort-peck/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l1c/goodwin-creek/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l1c/penn-state/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l1c/sioux-falls/" \
)

CMD="../unzip_sentinel_folder.sh"

for (( i=0; i<${#L1CDIRS[@]}; ++i ));
do
    echo "Unzipping folder: ${L1CDIRS[i]}"
    ${CMD} ${L1CDIRS[i]}
done