#############################################################################
# !make
#   Revision 1.0.0  11/2009  Yanmin Shuai
#############################################################################
#
# Executable name
CC=gcc -m64
CFLAGS=  -Wall -g

TARGET = UnsupCls_main.exe

#CC     = gcc
#CFLAGS = -g -Dlynx

# Gather include directories: 
INC   = -I. -I./my_stdinc -I/usr/include -I$(HDFINC) -I$(HDFEOS_INC) -I$(HDF5INC) -I$(GDAL_INC) -I../common

LIB =   -L. -L$(HDFEOS_LIB) -lhdfeos  \
        -L$(HDFLIB) -lmfhdf -ldf -lz -ljpeg -lsz -lm -L$(GDAL_LIB) -lgdal
      

# Gather searchable libraries:
########## USE THE FOLLOWING LINE AT SDST ###########################
# Files
OBJ = UnsupCls_main.o UnsupCls_funcs.o Unsup_stats.o ../common/sqs.o

INC_FILES = UnsupCls.h 

# make
$(TARGET) : $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LIB) -o $(TARGET)

.c.o: $(INC_FILES) 
	$(CC) $(CFLAGS) $(INC) -c $< -o $@



