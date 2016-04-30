#!/bin/bash

exe=./sub_test
data_dir=/neponset/nbdata05/albedo/qsun/V6_OUT
data_dir=/neponset/nbdata05/albedo/qsun/V6_OUT/selected_for_yanmin
data_dir=/neponset/nbdata05/albedo/qsun/LandsatDATA/test/modis

#data_dir=/neponset/nbdata07/albedo/Cherski/MODIS

#lat=44.997917
#lon=-77.775972
#lat=44.997917
#lon=-83.079080



xml=/neponset/nbdata03/qsun/Landsat8_SR/official/LC80120312014221LGN00/LC80120312014221LGN00.xml
xml=/neponset/nbdata03/qsun/Landsat8_SR/official/LC80120312014237LGN00/LC80120312014237LGN00.xml
xml=/neponset/nbdata07/albedo/Cherski/Landsat/p104r11/Less80/LC81040112014210/LC81040112014210LGN00.xml
xml=/neponset/nbdata05/albedo/qsun/LandsatDATA/test/LT50290302011236PAC03.xml


#for scene in {"p29r29","p29r31"}; do
scene=p29r30

dir=/neponset/nbdata05/albedo/qsun/LandsatDATA/test/${scene}
xml=`ls ${dir}/*.xml`

for head in {"MCD43A1","MCD43A2"}; do

out=${dir}/${head}_${scene}.hdf

#echo "$exe $data_dir $head $lat $lon $year $doy $out"
#$exe $data_dir $head $lat $lon $year $doy $out
$exe $data_dir $head $xml $out

done
#done
