# Sentinel-2 Tools #

Collection of tools to search, download and process Sentinel-2 data. 

### Categories of tools ###

* Preprocessing, in the folder _sentinel-2-preprocessing_
    * Search Sentinel-2 data via ESA Sentinel Data Hub API. 
    * Download found Sentinel-2 data in batch. 
    * Validate the integrity of downloaded ddata. 
    * Unzip Sentinel-2 zip files.
    * Atmosphere correction using ESA's sen2cor. 
* Generate land surface albedo from Sentinel L2A data, in the folder _sentinel-2-albedo_
    * Adapt albedo algorithm for Landsat images by Yanming Shuai et al., 2011 to process Sentinel-2 data to land surface albedo.
* Post-processing utilities, in the folder _sentinel-2-utils_
    * Subset Sentinel-2 albedo images and output statistical summaries within the subset footprint. 

### Contacts ###

* Crystal Schaaf, PI
* Yanmin Shuai, Co-PI
* Qingsong Sun
* Angela Erb
* Zhan Li, current custodian of this repo, zhan.li@umb.edu

### References ###
* Shuai, Y., Masek, J. G., Gao, F., & Schaaf, C. B. (2011). An algorithm for the retrieval of 30-m snow-free albedo from Landsat surface reflectance and MODIS BRDF. _Remote Sensing of Environment_, 115(9), 2204–2216. 
* Shuai, Y., Masek, J. G., Gao, F., Schaaf, C. B., & He, T. (2014). An approach for the long-term 30-m land surface snow-free albedo retrieval from historic Landsat surface reflectance and MODIS-based a priori anisotropy knowledge. _Remote Sensing of Environment_, 152, 467–479. 