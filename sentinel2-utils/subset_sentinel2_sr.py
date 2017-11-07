#!/usr/bin/env python

# Subset Sentinel-2 surface reflectance given the diameter of a circle
# and output mean and sd values.
#
# Zhan Li, zhan.li@umb.edu
# Created: Thu Sep 28 10:38:35 EDT 2017

import sys
import os
import argparse
import types
import os.path

import numpy as np
from osgeo import gdal, gdal_array, osr

gdal.AllRegister()

def getCmdArgs():
    p = argparse.ArgumentParser(description="Subset Sentinel-2 surface reflectance and output mean and standard deviation.")
    
    p.add_argument("-i", "--input", dest="input_image", required=True, nargs="+", default=None, metavar="INPUT_IMAGE", help="The input image/s to be subset. Either one or multiple input files of single or multiple bands. The mean and sd values of subsets of all the bands from all the input images will be output.")
    p.add_argument("--scl", dest="scl", required=True, default=None, metavar="S2L2A_SCL_IMAGE", help="The Sentinel-2 L2A SCL image for quality screening of pixels.")

    p.add_argument("--broad", dest="broad_sr", required=False, action="store_true", default=False, help="If set, calculate and output broaband surface reflectance using narrow-to-broadband conversion coefficients. Input images must be in total six bands corresponding to blue(02), green(03), red(04), NIR(8A), SWIR1(11) and SWIR2(12).")
    p.add_argument("--band_index", dest="band_index", required=False, nargs="+", default=None, metavar="BAND_INDEX", help="The six band indexes of all the bands from the input images in sequential order for the calculation of broadband surface reflectance, band index names: 02, 03, 04, 8A, 11 and 12 for blue, green, red, NIR, SWIR1 and SWIR2.")

    p.add_argument("--nodata", dest="nodata_user", required=False, type=float, default=None, help="Nodata value for all the bands in all the input images designated by user. Overwrite image's own nodata value even if it has a defined one.")
    p.add_argument("--lat", dest="lat", required=True, type=float, default=None, metavar="LATITUDE", help="Latitude of the center of the subset.")
    p.add_argument("--lon", dest="lon", required=True, type=float, default=None, metavar="LONGITUDE", help="Longitude of the center of the subset.")

    p.add_argument("-D", "--diameter", dest="circle_diams", required=False, type=float, nargs="+", default=None, metavar="CIRCLE_DIAMETERS", help="One or more diameters of the circular subsets, in the unit of the same as the input image resolution unit.")

    cmdargs = p.parse_args()

    if cmdargs.broad_sr:
        if cmdargs.band_index is None:
            p.print_help()
            raise RuntimeError("To calculate broadband surface reflectance, you must designate the band indexes of the bands from your input images!")
        if len(cmdargs.band_index) != 6:
            p.print_help()
            raise RuntimeError("To calculate broadband surface reflectance, you must give six band indexes for your input bands, representing blue, green, red, NIR, SWIR1 and SWIR2.")

    return cmdargs

def geo2Proj(filename_or_spatialref, lon, lat):
    """
    Convert geographic coordinates to projected coordinates 
    given the input raster file
    """
    if isinstance(filename_or_spatialref, types.StringTypes):
        filename = filename_or_spatialref
        ds = gdal.Open(filename, gdal.GA_ReadOnly)
        out_sr = osr.SpatialReference()
        out_sr.ImportFromWkt(ds.GetProjectionRef())
    else:
        out_sr = filename_or_spatialref
    in_sr = out_sr.CloneGeogCS()
    coord_trans = osr.CoordinateTransformation(in_sr, out_sr)
    return tuple(coord_trans.TransformPoint(lon, lat)[0:2])

def proj2Geo(filename, x, y):
    """
    Convert projected coordinates to geographic coordinates
    given the input raster file    
    """
    ds = gdal.Open(filename, gdal.GA_ReadOnly)
    in_sr = osr.SpatialReference()
    in_sr.ImportFromWkt(ds.GetProjectionRef())
    out_sr = in_sr.CloneGeogCS()
    coord_trans = osr.CoordinateTransformation(in_sr, out_sr)
    return tuple(coord_trans.TransformPoint(x, y)[0:2])

def proj2Pixel(geoMatrix, x, y, ret_int=True):
    """
    Uses a gdal geomatrix (gdal.GetGeoTransform()) to calculate
    the pixel location, with 0 being the first pixel, of 
    a geospatial coordinate in the projection system.
    """
    ulX = geoMatrix[0]
    ulY = geoMatrix[3]
    xDist = geoMatrix[1]
    yDist = geoMatrix[5]
    rtnX = geoMatrix[2]
    rtnY = geoMatrix[4]
    if ret_int:
        sample = int((x - ulX) / xDist)
        line = int((y - ulY) / yDist)
    else:
        sample = (x - ulX) / xDist
        line = (y - ulY) / yDist
    return (sample, line)

def geo2Pixel(filename, lon, lat, ret_int=True):
    """
    Convert geographic coordinates to image coordinates 
    given the input raster file
    """
    ds = gdal.Open(filename, gdal.GA_ReadOnly)
    proj_coord = geo2Proj(filename, lon, lat)
    return proj2Pixel(ds.GetGeoTransform(), proj_coord[0], proj_coord[1], ret_int)

def pixel2Proj(geoMatrix, sample, line):
    """
    Covnert pixel location (sample, line), 
    with 0 being the first pixel, to the 
    geospatial coordinates in the projection system, 
    with (x, y) being the UL corner of the pixel.
    """
    ulX = geoMatrix[0]
    ulY = geoMatrix[3]
    xDist = geoMatrix[1]
    yDist = geoMatrix[5]
    rtnX = geoMatrix[2]
    rtnY = geoMatrix[4]
    x = sample * xDist + ulX
    y = line * yDist + ulY
    return (x, y)

def pixel2Geo(filename, sample, line):
    """
    Convert the image coordinates to geographic coordinates given the input raster file.
    """
    ds = gdal.Open(filename, gdal.GA_ReadOnly)
    proj_coord = pixel2Proj(ds.GetGeoTransform(), sample, line)
    return proj2Geo(filename, proj_coord[0], proj_coord[1])

def readRasterSubset(imgf, lon, lat, out_imgsize, bands=None):
    csample, cline = geo2Pixel(imgf, lon, lat, ret_int=True)
    cx, cy = geo2Proj(imgf, lon, lat)
    ds = gdal.Open(imgf, gdal.GA_ReadOnly)
    geo_trans = ds.GetGeoTransform()
    out_nsample = int(out_imgsize / geo_trans[1]) + 1
    out_nline = int(-1 * out_imgsize / geo_trans[5]) + 1
    min_sample = csample - int(out_nsample*0.5)
    min_line = cline - int(out_nline*0.5)
    max_sample = min_sample + out_nsample
    max_line = min_line + out_nline
    min_sample = min_sample if min_sample > 0 else 0
    min_line = min_line if min_line > 0 else 0
    max_sample = max_sample if max_sample < ds.RasterXSize else ds.RasterXSize
    max_line = max_line if max_line < ds.RasterYSize else ds.RasterYSize

    if bands is None:
        bands = range(1, ds.RasterCount+1) # band index starts from 0 as the first band.
    out_img = np.zeros((max_line-min_line, max_sample-min_sample, len(bands)), 
                        dtype=gdal_array.GDALTypeCodeToNumericTypeCode(ds.GetRasterBand(1).DataType))
    for i, ib in enumerate(bands):
        band = ds.GetRasterBand(ib)
        img = band.ReadAsArray()
        out_img[:,:,i] = img[min_line:max_line, min_sample:max_sample]

    return out_img # nrow, ncol, nband

def parseSCL(scl_img):
    # Parse and interpret Sentinel-2 SCL image and return three simple
    # categories: nodata/bad data (-1), clear snow free (0), cloud-contaminated (1), and snow (2).
    out_img = np.zeros_like(scl_img, dtype=int)
    # nodata / bad data
    mask = scl_img == 0
    out_img[mask] = -1
    # cloud-contaminated
    mask = reduce(np.logical_and, [scl_img==3, scl_img==7, scl_img==8, scl_img==9, scl_img==10])
    out_img[mask] = 1
    # snow
    mask = scl_img == 11
    out_img[mask] = 2
    return out_img

def getN2B(band_index, snow_flag, broad_name="sw"):
    if broad_name not in ["sw", "vis", "nir"]:
        raise RuntimeError("Illegal broaband category name {0:s}".format(broad_name))
    if (broad_name=="sw" and len(band_index) !=6) or ((broad_name=="vis" or broad_name=="nir") and len(band_index) != 3):
        raise RuntimeError("Number of bands for N2B coef. can only be 6 for SW or 3 for VIS/NIR!")
    legal_band_index = ["02", "03", "04", "8A", "11", "12"]
    for bidx in band_index:
        if bidx not in legal_band_index:
            raise RuntimeError("Illegal band index names {0:s}\nLegal names: {1:s}".format(bidx, ", ".join(legdal_band_index)))
    n2b_coef = {"snow_free":{"sw":{"02":0.2687617,
                                   "03":0.0361839,
                                   "04":0.1501418, 
                                   "8A":0.3044542, 
                                   "11":0.164433, 
                                   "12":0.0356021, 
                                   "const":-0.0048673}, 
                             "vis":None, 
                             "nir":None}, 
                "snow":{"sw":{"02":-0.1992158, 
                              "03":2.300191, 
                              "04":-0.1912122, 
                              "8A":0.6714989, 
                              "11":-2.272847, 
                              "12":1.934139, 
                              "const":-0.0001144}, 
                        "vis":None, 
                        "nir":None}}
    snow_label = "snow" if snow_flag else "snow_free"
    out_coef = [n2b_coef[snow_label][broad_name][bidx] for bidx in band_index] + [n2b_coef[snow_label][broad_name]["const"]]
    return np.array(out_coef)

def main(cmdargs):
    input_image = cmdargs.input_image
    scl_image = cmdargs.scl

    do_broad = cmdargs.broad_sr
    band_index = cmdargs.band_index

    nodata_user = cmdargs.nodata_user
    lat = cmdargs.lat
    lon = cmdargs.lon
    circle_diams = cmdargs.circle_diams

    nodata_def = -9999

    # If broadband calculation, check the total number of bands from the input images.
    band_cnt = np.array([gdal.Open(imgf, gdal.GA_ReadOnly).RasterCount for imgf in input_image])
    if do_broad and np.sum(band_cnt) != 6:
        raise RuntimeError("Total number of bands from all the input images must be 6 for broadband surface reflectance calculation.")

    out_data = [{diam:None for diam in circle_diams} for imgf in input_image]
    out_mask_nonsnow = [{diam:None for diam in circle_diams} for imgf in input_image]
    out_mask_snow = [{diam:None for diam in circle_diams} for imgf in input_image]

    sub_sizes = circle_diams
    # read the subset of SCL image
    scl_data = dict()
    for out_imgsize, diam in zip(sub_sizes, circle_diams):
        scl_data[diam] = readRasterSubset(scl_image, lon, lat, out_imgsize)[:,:,0]

    scl_parse = {diam:parseSCL(tmp) for diam, tmp in scl_data.iteritems()}

    for fidx, imgf in enumerate(input_image):
        ds = gdal.Open(imgf, gdal.GA_ReadOnly)
        ds_dtype = gdal_array.GDALTypeCodeToNumericTypeCode(ds.GetRasterBand(1).DataType)
        ds_nodata = np.zeros(ds.RasterCount, dtype=ds_dtype)
        for i, ib in enumerate(range(ds.RasterCount)):
            band = ds.GetRasterBand(ib+1)
            nodata = band.GetNoDataValue()
            if nodata is None:
                nodata = nodata_def
            if nodata_user is not None:
                nodata = nodata_user
            ds_nodata[i] = nodata

        for out_imgsize, diam in zip(sub_sizes, circle_diams):
            sub_data = readRasterSubset(imgf, lon, lat, out_imgsize)
            if scl_parse[diam].shape[0]!=sub_data.shape[0] or scl_parse[diam].shape[1]!=sub_data.shape[1]:
                raise RuntimeError("Surface reflectance image and SCl image at diameter subset {0:.3f} does not return the same raster data dimension!\nSR: {1:s}\nSCL: {2:s}".format(diam, imgf, scl_image))
            scl_mask = scl_parse[diam]
            sub_mask_nonsnow = np.dstack([np.logical_and(scl_mask==0, sub_data[:,:,ib]!=ds_nodata[ib]) for ib in range(ds.RasterCount)])
            sub_mask_snow = np.dstack([np.logical_and(scl_mask==2, sub_data[:,:,ib]!=ds_nodata[ib]) for ib in range(ds.RasterCount)])

            out_data[fidx][diam] = sub_data
            out_mask_nonsnow[fidx][diam] = sub_mask_nonsnow
            out_mask_snow[fidx][diam] = sub_mask_snow

        ds = None

    if do_broad:
        out_fname = ["sw"]
        out_stats = [{diam:None for diam in circle_diams} for imgf in out_fname]
        for outfidx, outf in enumerate(out_fname):
            snow_n2b_coef = getN2B(band_index, True, broad_name=outf)
            nonsnow_n2b_coef = getN2B(band_index, False, broad_name=outf)
            for diam in circle_diams:
                tmp_data = np.dstack([out_data[fidx][diam] for fidx in range(len(input_image))])
                tmp_mask_nonsnow = np.all(np.dstack([out_mask_nonsnow[fidx][diam] for fidx in range(len(input_image))]), axis=2)
                tmp_mask_snow = np.all(np.dstack([out_mask_snow[fidx][diam] for fidx in range(len(input_image))]), axis=2)
                
                tmp_coef_img = np.zeros((tmp_data.shape[0], tmp_data.shape[1], tmp_data.shape[2]+1), dtype=float) + np.nan
                tmp = np.dstack([tmp_mask_nonsnow for i in range(len(band_index)+1)])
                tmp_coef_img[tmp] = np.tile(nonsnow_n2b_coef, tmp_mask_nonsnow.shape+(1,))[tmp]
                tmp = np.dstack([tmp_mask_snow for i in range(len(band_index)+1)])
                tmp_coef_img[tmp] = np.tile(snow_n2b_coef, tmp_mask_nonsnow.shape+(1,))[tmp]
                
                tmp_bb = np.sum(tmp_data * tmp_coef_img[:,:,0:-1], axis=2) + tmp_coef_img[:,:,-1]
                out_stats[outfidx][diam] = np.array([[np.nanmean(tmp_bb), np.nanstd(tmp_bb), np.sum(np.isfinite(tmp_bb))]])
    else:
        out_fname = input_image
        out_stats = [{diam:None for diam in circle_diams} for imgf in input_image]
        for fidx in range(len(input_image)):
            for diam in circle_diams:
                tmp_mask = np.logical_or(out_mask_nonsnow[fidx][diam], out_mask_snow[fidx][diam])
                tmp_data = out_data[fidx][diam].copy().astype(float)
                tmp_data[np.logical_not(tmp_mask)] = np.nan
                tmp_cnt = np.sum(np.isfinite(tmp_data), axis=(0,1))
                out_stats[fidx][diam] = np.array([np.nanmean(tmp_data, axis=(0,1)) if np.sum(tmp_cnt)>0 else np.zeros_like(tmp_cnt,dtype=float)+np.nan, 
                                                  np.nanstd(tmp_data, axis=(0, 1)) if np.sum(tmp_cnt)>0 else np.zeros_like(tmp_cnt,dtype=float)+np.nan, 
                                                  tmp_cnt]).T

    sys.stdout.write("image,subset_diameter,band_index,mean,sd,count\n")
    for fidx, outf in enumerate(out_fname):
        for diam in circle_diams:
            for irow, row in enumerate(out_stats[fidx][diam]):
                sys.stdout.write("{0:s},{1:.3f},{2:d},{3[0]:.3f},{3[1]:.3f},{3[2]:.0f}\n".format(outf, diam, irow+1, row))

if __name__ == "__main__":
    cmdargs = getCmdArgs()
    main(cmdargs)
