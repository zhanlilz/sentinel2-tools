#!/bin/bash

# query_result_xml="/home/zhanli/scratch/temp/test.xml"
# query_result_meta_csv="/home/zhanli/scratch/temp/test.xml.meta.csv"
# csv_head=1

query_result_xml=${1}
query_result_meta_csv=${2}
csv_head=${3}

get_entry_item ()
{
    tag=${1}
    name=${2}
    if [[ ${#} -eq 1 ]]; then
        xmllint --xpath "//*[local-name()='feed']/*[local-name()='entry']/*[local-name()='${tag}']" ${query_result_xml} 2>/dev/null \
            | sed "s/<${tag}>//g" | sed "s/<\/${tag}>/\\n/g"
    else
        xmllint --xpath "//*[local-name()='feed']/*[local-name()='entry']/*[local-name()='${tag}'][@name='${name}']" ${query_result_xml} 2>/dev/null \
            | sed "s/<${tag} name=\"${name}\">//g" | sed "s/<\/${tag}>/\\n/g"
    fi
}

put_entry_item ()
{
    tag=${1}
    name=${2}

    cp ${query_result_meta_csv} ${query_result_meta_csv}.tmp
    if [[ ${tag} == "str" ]]; then
        paste -d "," ${query_result_meta_csv}.tmp <(get_entry_item ${tag} ${name} | sed "s/^/\"/g" | sed "s/$/\"/g") > ${query_result_meta_csv}
    else
        paste -d "," ${query_result_meta_csv}.tmp <(get_entry_item ${tag} ${name}) > ${query_result_meta_csv}
    fi
}

tag_list=("title" "id" "date" "date" "date" "date" \
    "int" "int" "double" "str" \
    "str" "str" "str" "str" "str" \
    "str" "str" "str" "str" "str" \
    "str" "str" "str" "str" \
    "str")

name_list=("" "" "datatakesensingstart" "beginposition" "endposition" "ingestiondate" \
    "orbitnumber" "relativeorbitnumber" "cloudcoverpercentage" "sensoroperationalmode" \
    "footprint" "tileid" "processingbaseline" "platformname" "instrumentname" \
    "instrumentshortname" "size" "s2datatakeid" "producttype" "platformidentifier" \
    "orbitdirection" "platformserialidentifier" "processinglevel" "identifier"\
    "uuid")

> ${query_result_meta_csv}

get_entry_item ${tag_list[0]} ${name_list[0]} > ${query_result_meta_csv}

for ((i=1; i<${#tag_list[@]}; i++));
do
    put_entry_item ${tag_list[i]} ${name_list[i]}
done

# extra items of links in the xml attribute
cp ${query_result_meta_csv} ${query_result_meta_csv}.tmp
paste -d "," ${query_result_meta_csv}.tmp <(xmllint --xpath "//*[local-name()='feed']/*[local-name()='entry']/*[local-name()='link'][@rel='icon']/@href" ${query_result_xml} 2>/dev/null | sed "s/href=/\\n/g" | tail -n +2) > ${query_result_meta_csv}
cp ${query_result_meta_csv} ${query_result_meta_csv}.tmp
paste -d "," ${query_result_meta_csv}.tmp <(xmllint --xpath "//*[local-name()='feed']/*[local-name()='entry']/*[local-name()='link'][not(@rel)]/@href" ${query_result_xml} 2>/dev/null | sed "s/href=/\\n/g" | tail -n +2) > ${query_result_meta_csv}

if [[ ${csv_head} -eq 1 ]]; then
    cp ${query_result_meta_csv} ${query_result_meta_csv}.tmp
    > ${query_result_meta_csv}
    for ((i=0; i<${#name_list[@]}; i++));
    do
        if [[ -z ${name_list[i]} ]]; then
            echo -n "${tag_list[i]}" >> ${query_result_meta_csv}
        else
            echo -n "${name_list[i]}" >> ${query_result_meta_csv}
        fi
        echo -n "," >> ${query_result_meta_csv}
    done
    echo -n "quicklooklink," >> ${query_result_meta_csv}
    echo "downloadlink" >> ${query_result_meta_csv}
    cat ${query_result_meta_csv}.tmp >> ${query_result_meta_csv}
fi

rm -f ${query_result_meta_csv}.tmp
