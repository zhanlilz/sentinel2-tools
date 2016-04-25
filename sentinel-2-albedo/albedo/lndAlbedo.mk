#############################################################################
# !make
#
# makefile name: makefile (for ledaps)
#
##!END
#############################################################################
CC = gcc 
CFLAGS =-m64 -D_BSD_SOURCE -ansi -Wall -g

TARGET = lndAlbedo_main.exe

INC =   -I. -I$(SRCGCTP) -I$(TIFFINC) -I$(GEOTIFF_INC) -I$(HDFINC) -I$(HDFEOS_INC)

LIB =   -L. -L$(HDFEOS_LIB) -lhdfeos \
	-L$(GEOTIFF_LIB) -lgeotiff  \
	-L$(TIFFLIB) -ltiff \
	-L$(LIBGCTP) geolib64.a \
	-L$(HDFLIB) -lmfhdf -ldf -lz -ljpeg  -lsz -lm 

OBJ = lndAlbedo_main.o Reproject_CalAlbedo.o MODIS_io.o \
	LandSat_io.o Sensor2MODIS.o CalculateANratio.o \
	CalculateAlbedo.o ParsePara_malloc.o BRDF_model.o

INC_FILES=lndsr_albedo.h cproj.h proj.h

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


