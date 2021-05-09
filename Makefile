CC=g++

CFLAGS=-O3
LIBS=-lopencv_core -lopencv_imgproc -lopencv_videoio -lopencv_highgui -lopencv_viz
ALIBS=`pkg-config --libs opencv4`

MAINSRC=detect.cpp
SOURCES=VideoInfo.cpp FrameBuffer.cpp
HEADERS=VideoInfo.h FrameBuffer.h

SRCS=$(MAINSRC) $(HEADERS) $(SOURCES)
OBJS=$(SOURCES:.cpp=)
APP=$(MAINSRC:.cpp=)

all:	$(APP)

clean:
	-rm -f *.o
	-rm -f $(APP)

$(APP): $(APP).o VideoInfo.o FrameBuffer.o
	$(CC) -o $@ $^ $(LIBS)

$(APP).o: $(MAINSRC)
	$(CC) $(CFLAGS) -c $^

VideoInfo.o: VideoInfo.cpp VideoInfo.h
	$(CC) $(CFLAGS) -c $^ -lopencv_core

FrameBuffer.o: FrameBuffer.cpp FrameBuffer.h
	$(CC) $(CFLAGS) -c $^ -lopencv_videoio
