TARGET = CDE

CXXFLAGS = -c -g -O2 -Wno-c++11-extensions
CXX = clang++

INCLUDE_DIR = -I/usr/local/include/
LIB_DIR = -L/usr/local/lib
LIBS = -lopencv_core -lopencv_highgui -lopencv_imgproc

SOURCES = main.cpp CDE.cpp GraphUtils.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LIB_DIR) $(LIBS) $^ -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIR) $< -o $@


clean:
	rm -f $(TARGET) $(OBJECTS)
