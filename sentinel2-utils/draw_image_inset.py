#!/usr/bin/env python

# Cut a square out of a given Landsat image to draw an inset figure
# centered at a given location with circles of given diameters, and
# several squares of given side sizes.
# 
# Zhan Li, zhan.li@umb.edu
# Created: Mon Mar 27 21:34:09 EDT 2017

import sys
import os
import argparse
import types

import numpy as np
from osgeo import gdal, gdal_array, osr

import matplotlib as mpl
mpl.use('Agg')
import matplotlib.pyplot as plt
from mpl_toolkits.basemap import Basemap

gdal.AllRegister()

def getCmdArgs():
    p = argparse.ArgumentParser(description="Cut a square out of a given image and draw circles and squares on it.")

    p.add_argument("-i", "--input", dest="input_image", required=True, nargs="+", default=None, metavar="INPUT_IMAGE", help="The input image/s to be cut. Either one or three input files. If three input files, they will be composited first as RGB in order for the output.")
    p.add_argument("-o", "--output", dest="output_image", required=True, default=None, metavar="OUTPUT_IMAGE", help="The output image.")
    p.add_argument("-S", "--size", dest="figsize", required=False, type=float, nargs=2, default=(3, 3), metavar="FIGSIZE", help="The size of output figure. Default: (3 in, 3 in)")

    p.add_argument("--stretch", dest="stretch_range", required=True, type=float, nargs=2, default=None, metavar="STRETCH_RANGE", help="The low and high boundary of input image pixel values to stretch for the output.")

    p.add_argument("--lat", dest="lat", required=True, type=float, default=None, metavar="LATITUDE", help="Latitude of the center of the output inset.")
    p.add_argument("--lon", dest="lon", required=True, type=float, default=None, metavar="LONGITUDE", help="Longitude of the center of the output inset.")

    p.add_argument("-D", "--diameter", dest="circle_diams", required=False, type=float, nargs="+", default=None, metavar="CIRCLE_DIAMETERS", help="One or more diameters of the circles to be drawn, in the unit of the same as the input image resolution unit.")
    p.add_argument("-L", "--length", dest="square_lens", required=True, type=float, nargs="+", default=None, metavar="SQUARE_LENGTHS", help="One or more lengths of the squares to be drawn, in the unit of the same as the input image resolution unit. The largest length will be the size of output image.")
    p.add_argument("-C", "--color", dest="line_color", required=False, type=int, nargs=3, default=(255, 255, 255), metavar="LINE_COLOR", help="Line color to draw circles and squares in three numbers of RGB with each from 0-255. Default: (255,255,255).")

    cmdargs = p.parse_args()

    if len(cmdargs.input_image) != 1 and len(cmdargs.input_image) != 3:
        raise RuntimeError("Number of input image/s can only be one file (either single band or RGB bands) or three files in the order of RGB composite.")

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

def main(cmdargs):
    input_image = cmdargs.input_image
    output_image = cmdargs.output_image
    figsize = cmdargs.figsize
    line_color = cmdargs.line_color
    stretch_range = cmdargs.stretch_range
    lat = cmdargs.lat
    lon = cmdargs.lon
    circle_diams = cmdargs.circle_diams
    square_lens = cmdargs.square_lens

    nodata_def = -9999
    line_color = np.array(line_color) / 255.

    inset_data_list = list()
    inset_mask_list = list()
    out_imgsize = np.max(square_lens)
    for imgf in input_image:
        csample, cline = geo2Pixel(imgf, lon, lat, ret_int=True)
        cx, cy = geo2Proj(imgf, lon, lat)

        ds = gdal.Open(imgf, gdal.GA_ReadOnly)
        geo_trans = ds.GetGeoTransform()
        proj_ref = ds.GetProjectionRef()

        out_nsample = int(out_imgsize / geo_trans[1])
        out_nline = int(-1 * out_imgsize / geo_trans[5])
        min_sample = csample - int(out_nsample*0.5)
        min_line = cline - int(out_nline*0.5)
        llgeo = pixel2Geo(imgf, min_sample, min_line+out_nline)
        urgeo = pixel2Geo(imgf, min_sample+out_nsample, min_line)
        
        for ib in range(ds.RasterCount):
            band = ds.GetRasterBand(ib+1)
            nodata = band.GetNoDataValue()
            if nodata is None:
                nodata = nodata_def
            img = band.ReadAsArray()
            img = img[min_line:min_line+out_nline, min_sample:min_sample+out_nsample]
            img = (img - stretch_range[0]) / (stretch_range[1] - stretch_range[0])
            img[img<0] = 0
            img[img>1] = 1
            mask = img != nodata
            inset_data_list.append(img)
            inset_mask_list.append(mask)
        ds = None

    if len(inset_data_list) == 1:
        inset_mask_list = inset_mask_list * 3
        inset_data_list = inset_data_list * 3

    if len(inset_data_list) != 3:
        raise RuntimeError("Total number of bands from the input image/s can only be one or three!")
        
    out_mask = reduce(np.logical_and, inset_mask_list)
    for dat in inset_data_list:
        dat[np.logical_not(out_mask)] = 0
    out_image = np.dstack(inset_data_list)

    utm_zone = int((lon+180)/6) + 1
    ctr_meridian = (utm_zone - 1 + utm_zone) * 6 / 2 - 180
    bmobj = Basemap(projection="tmerc", ellps="WGS84", lat_0=0, lon_0=ctr_meridian, k_0=0.9996, 
                    llcrnrlon=llgeo[0], llcrnrlat=llgeo[1], urcrnrlon=urgeo[0], urcrnrlat=urgeo[1], 
                    resolution="h", suppress_ticks=True)
    fig, ax = plt.subplots(figsize=figsize)
    imobj = bmobj.imshow(np.flipud(out_image))

    for slen in square_lens:
        if slen == out_imgsize:
            continue
        tmpx = np.array([cx-0.5*slen, cx+0.5*slen, cx+0.5*slen, cx-0.5*slen, cx-0.5*slen])
        tmpy = np.array([cy+0.5*slen, cy+0.5*slen, cy-0.5*slen, cy-0.5*slen, cy+0.5*slen])
        tmpgeo = [proj2Geo(input_image[0], x, y) for x, y in zip(tmpx, tmpy )]
        tmplon, tmplat = zip(*tmpgeo)
        tmpx, tmpy = bmobj(tmplon, tmplat)
        bmobj.plot(tmpx, tmpy, latlon=False, linestyle="-", color=line_color)

    if circle_diams is not None:
        for cdiam in circle_diams:
            tmpres = 1/cdiam
            tmpang = np.arange(0, np.pi*2, tmpres)
            tmpx = cdiam*0.5*np.sin(tmpang) + cx
            tmpy = cdiam*0.5*np.cos(tmpang) + cy
            tmpgeo = [proj2Geo(input_image[0], x, y) for x, y in zip(tmpx, tmpy )]
            tmplon, tmplat = zip(*tmpgeo)
            tmpx, tmpy = bmobj(tmplon, tmplat)
            bmobj.plot(tmpx, tmpy, latlon=False, linestyle="-", color=line_color)

    plt.savefig(output_image, dpi=300, bbox_inches="tight", pad_inches=0.)


if __name__ == "__main__":
    cmdargs = getCmdArgs()
    main(cmdargs)
