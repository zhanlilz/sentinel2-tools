
#lib_dir = /home/qsun/code/Landsat_albedo_project/Code_forUMassBoston_USGS/common
#lib_dir = /home/qsun/code/Landsat_albedo_project/Code_forUMassBoston_Sentinel2/common
lib_dir = ../common

TARGET = sub2
TARGET2 = create
TARGET3 = copy

CFLAGS = -fPIC -DHAVE_NETCDF -DLINUX -O3 -ggdb
LINK = -L$(HDFEOS_LIB) -lhdfeos -L$(HDFLIB) -lmfhdf -ldf -lGctp -lz -lm -L$(JPEGLIB) -ljpeg -L$(SZIPLIB) -lsz -L$(GDAL_LIB) -lgdal
OBJ = main.o space.o $(lib_dir)/sqs.o
OBJ2 = hdfeos.o
OBJ3 = hdf.o
INC = -I. -I$(HDFEOSINC) -I$(HDFINC) -I$(GCTPINC) -I$(lib_dir) -I$(GDAL_INC)

ALL: $(TARGET) $(TARGET2) $(TARGET3)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LINK) -o $(TARGET) 
$(TARGET2): $(OBJ2)
	$(CC) $(CFLAGS) $(OBJ2) $(LINK) -o $(TARGET2) 
$(TARGET3): $(OBJ3)
	$(CC) $(CFLAGS) $(OBJ3) $(LINK) -o $(TARGET3) 

.c.o:
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

clean:
	rm $(OBJ)
	rm $(TARGET)
