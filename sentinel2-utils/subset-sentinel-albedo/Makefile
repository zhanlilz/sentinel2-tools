#
TARGET = sub

# Files
OBJ = envi.o space.o main.o

##########################################
ADD_CFLAGS= -O3 -DLYNX  -ffloat-store -std=c99 -pedantic -DDEBUG -g

INC = -I$(API_INC) -I$(PGSINC) -I$(HDFINC) -I$(HDFEOS_INC) -I$(GCTPINC) -I. 

LIB = -L$(HDFEOS_LIB) -lhdfeos -lGctp -L$(HDFLIB) -lmfhdf -ldf -lz -lm -ljpeg -L${SZIPLIB} -lsz

ALL : $(TARGET) 

# make
$(TARGET) : $(OBJ)
	$(CC) $(CFLAGS) $(ADD_CFLAGS) $(OBJ) $(LIB) -o $(TARGET)


.c.o: 
	$(CC) $(CFLAGS) $(ADD_CFLAGS) $(INC) -c $< -o $@

#delete object files:
clean:
	rm *.o

