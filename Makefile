CC=g++

CFLAGS=-O3
TCFLAGS=-O0 -g -D TEST
LIBS=-lopencv_core -lopencv_imgproc -lopencv_videoio
TLIBS=-lopencv_core -lopencv_imgproc -lopencv_videoio -lopencv_highgui -lopencv_viz
ALIBS=`pkg-config --libs opencv4`

SOURCES=detect.cpp
HEADERS=

SRCS=$(HEADERS) $(SOURCES)
APP=$(SOURCES:.cpp=)
TEST=$(APP)_test

all:	$(APP) $(TEST)

clean:
	-rm -f *.o
	-rm -f $(APP) $(TEST)
	-rm -f OUT*.mp4

$(APP): $(APP).o
	$(CC) -o $@ $^ $(LIBS)

$(TEST): $(TEST).o
	$(CC) -o $@ $^ $(TLIBS)

$(APP).o: $(SRCS)
	$(CC) $(CFLAGS) -c $^

$(TEST).o: $(SRCS)
	$(CC) $(TCFLAGS) -o $@ -c $^