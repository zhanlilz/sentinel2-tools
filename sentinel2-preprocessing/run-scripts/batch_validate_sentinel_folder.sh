#!/bin/bash

DATADIRS=( \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l1c/boulder/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l1c/boundville/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l1c/desert-rock/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l1c/fort-peck/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l1c/goodwin-creek/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l1c/penn-state/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l1c/sioux-falls/" \
)

CKSUMDIRS=( \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l1c/boulder/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l1c/boundville/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l1c/desert-rock/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l1c/fort-peck/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l1c/goodwin-creek/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l1c/penn-state/" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/sentinel-2-l1c/sioux-falls/" \
)

CTFILES=( \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/meta-files/boulder.corrupted" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/meta-files/boundville.corrupted" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/meta-files/desert_rock.corrupted" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/meta-files/fort_peck.corrupted" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/meta-files/goodwin_creek.corrupted" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/meta-files/penn_state.corrupted" \
"/neponset/nbdata08/albedo/zhan.li/projects/lps-2016/meta-files/sioux_falls.corrupted" \
)

CMD="../validate_sentinel_folder.sh"

for (( i=0; i<${#DATADIRS[@]}; ++i ));
do
    echo "Verify files in the directory ${DATADIRS[i]}"
    ${CMD} ${DATADIRS[i]} ${CKSUMDIRS[i]} ${CTFILES[i]}
done
