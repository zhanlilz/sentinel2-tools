#############################################################################
# !make
#
# makefile name: makefile (for ledaps)
#
##!END
#############################################################################
CC = gcc 
CFLAGS =-m64 -D_BSD_SOURCE -Wall -g

TARGET = lndAlbedo_main.exe

INC =   -I. -I$(SRCGCTP) -I$(TIFFINC) -I$(GEOTIFF_INC) -I$(HDFINC) -I$(HDFEOS_INC) -I$(GCTPINC) -I$(GDAL_INC) -I../common

LIB =   -L. -L$(HDFEOS_LIB) -lhdfeos \
	-L$(GEOTIFF_LIB) -lgeotiff  \
	-L$(TIFFLIB) -ltiff \
	-L$(LIBGCTP) \
	-L$(HDFLIB) -lmfhdf -ldf -lz -ljpeg  -lsz -lm -lGctp \
	-L$(GDAL_LIB) -lgdal

OBJ = lndAlbedo_main.o Reproject_CalAlbedo.o MODIS_io.o \
	LandSat_io.o Sensor2MODIS.o CalculateANratio.o \
	CalculateAlbedo.o ParsePara_malloc.o BRDF_model.o space.o ../common/sqs.o

INC_FILES=lndsr_albedo.h

all: $(TARGET)

$(OBJ) : $(INC_FILES)

# Make the process
$(TARGET) : $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LIB)  -o $(TARGET)

#
# Rules
#
.c.o: $(INC_FILES)
	$(CC) $(CFLAGS) $(ADD_CFLAGS) $(INC) -c $< -o $@

#******************* End of make file *******************************


