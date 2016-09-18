TARGET=libdropboxapiv2.so

LIBS=-lcurl -lpthread
LIBSPATH=
INCLUDE=
INCLUDEPATH=
SRC=$(wildcard *.cpp)
OBJS=$(SRC:.cpp=.o)
CXX=g++
LDFLAGS=-g -O2 -Wall -fPIC
MAKEDIR=mkdir -p

SAMPLE_FOLDER=./sample/download
SAMPLE_DOWNLOAD=$(SAMPLE_FOLDER)/downloaddrop

all:$(TARGET)

$(TARGET):$(OBJS)
	@$(MAKEDIR) libs/include
	@$(MAKEDIR) libs/lib
	@$(CXX) -shared $^ -o $@
	@cp *.h libs/include
	@mv $@ libs/lib
%.o:%.cpp
	@$(CXX) $(LDFLAGS) -c $< $(INCLUDEPATH) $(INCLUDE)
samples:
	$(CXX) -o $(SAMPLE_DOWNLOAD) $(SAMPLE_FOLDER)/main.cpp -I./libs/include -L./libs/lib  -ldropboxapiv2 $(LIBS)
PHONY:clean
clean:
	@rm -rf $(OBJS) $(TARGET) ./libs $(SAMPLE_DOWNLOAD)