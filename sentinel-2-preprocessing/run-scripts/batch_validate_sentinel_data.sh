#!/bin/bash

DATADIRS=( \
"../sentinel-2-l1c/boulder" \
"../sentinel-2-l1c/boundville" \
"../sentinel-2-l1c/desert-rock" \
"../sentinel-2-l1c/fort-peck" \
"../sentinel-2-l1c/goodwin-creek" \
"../sentinel-2-l1c/penn-state" \
"../sentinel-2-l1c/sioux-falls" \
)

CKSUMDIRS=( \
"../sentinel-2-l1c/boulder" \
"../sentinel-2-l1c/boundville" \
"../sentinel-2-l1c/desert-rock" \
"../sentinel-2-l1c/fort-peck" \
"../sentinel-2-l1c/goodwin-creek" \
"../sentinel-2-l1c/penn-state" \
"../sentinel-2-l1c/sioux-falls" \
)

CTFILES=( \
"../meta-files/boulder.corrupted" \
"../meta-files/boundville.corrupted" \
"../meta-files/desert_rock.corrupted" \
"../meta-files/fort_peck.corrupted" \
"../meta-files/goodwin_creek.corrupted" \
"../meta-files/penn_state.corrupted" \
"../meta-files/sioux_falls.corrupted" \
)

CMD="../validate_sentinel_folder.sh"

for (( i=0; i<${#DATADIRS[@]}; ++i ));
do
    echo "Verify files in the directory ${DATADIRS[i]}"
    ${CMD} -c ${DATADIRS[i]} ${CKSUMDIRS[i]} ${CTFILES[i]}
done
