CC=g++

CFLAGS=-O3
LIBS=-lopencv_core -lopencv_imgproc -lopencv_videoio -lopencv_highgui -lopencv_viz
ALIBS=`pkg-config --libs opencv4`

SOURCES=detect.cpp
HEADERS=

SRCS=$(HEADERS) $(SOURCES)
APP=$(SOURCES:.cpp=)

all:	$(APP)

clean:
	-rm -f *.o
	-rm -f $(APP)

$(APP): $(APP).o
	$(CC) -o $@ $^ $(LIBS)

$(APP).o: $(SRCS)
	$(CC) $(CFLAGS) -c $^
