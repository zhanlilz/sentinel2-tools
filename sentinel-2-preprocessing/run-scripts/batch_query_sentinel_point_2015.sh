#!/bin/bash

LATS=( \
40.125615 \
40.0521 \
36.62373 \
48.30796 \
34.2551 \
40.7203 \
43.7343 \
)

LONS=( \
-105.2378 \
-88.37309 \
-116.01947 \
-105.10175 \
-89.8737 \
-77.93098 \
-96.62334 \
)

if [[ ! -d "../meta-files" ]]; then
        mkdir "../meta-files"
fi

Q_RS=( \
"../meta-files/boulder_query" \
"../meta-files/boundville_query" \
"../meta-files/desert_rock_query" \
"../meta-files/fort_peck_query" \
"../meta-files/goodwin_creek_query" \
"../meta-files/penn_state_query" \
"../meta-files/sioux_falls_query" \
)

OUT_URLS=( \
"../meta-files/boulder_s2_2015_urls" \
"../meta-files/boundville_s2_2015_urls" \
"../meta-files/desert_rock_s2_2015_urls" \
"../meta-files/fort_peck_s2_2015_urls" \
"../meta-files/goodwin_creek_s2_2015_urls" \
"../meta-files/penn_state_s2_2015_urls" \
"../meta-files/sioux_falls_s2_2015_urls" \
)

CMD="../query_sentinel_point_2015.sh"

for (( i=0; i<${#LATS[@]}; ++i )); 
do
    echo "query for $(basename ${OUT_URLS[i]})"
    ${CMD} ${LATS[i]} ${LONS[i]} ${OUT_URLS[i]} ${Q_RS[i]}
done
