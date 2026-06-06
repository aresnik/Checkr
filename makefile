CXX = clang++

# Path to the IceCream library (sibling directory)
ICECREAM_DIR = ../IceCream
# Path to the UIWidgets library (sibling directory)
WIDGET_DIR = ../UIWidgets

CXXFLAGS = -std=c++17 -Wall -I/opt/homebrew/include -I$(WIDGET_DIR)/src -I$(ICECREAM_DIR)/src
LDFLAGS = -L/opt/homebrew/lib -lSDL3 -lSDL3_image -lSDL3_ttf -lSDL3_mixer

TARGET = bin/checkr

# Object groups
APP_OBJS = bin/main.o bin/gameController.o

# All widget sources and objects
WIDGET_SRCS = $(wildcard $(WIDGET_DIR)/src/*.cpp)
WIDGET_OBJS = $(patsubst $(WIDGET_DIR)/src/%.cpp, bin/%.o, $(WIDGET_SRCS))

# All IceCream sources and objects
ICECREAM_SRCS = $(wildcard $(ICECREAM_DIR)/src/*.cpp)
ICECREAM_OBJS = $(patsubst $(ICECREAM_DIR)/src/%.cpp, bin/%.o, $(ICECREAM_SRCS))

$(TARGET): $(APP_OBJS) $(WIDGET_OBJS) $(ICECREAM_OBJS)
	$(CXX) -o $(TARGET) $(APP_OBJS) $(WIDGET_OBJS) $(ICECREAM_OBJS) $(LDFLAGS)

# Pattern rule for compiling UIWidgets library files
bin/%.o: $(WIDGET_DIR)/src/%.cpp $(WIDGET_DIR)/src/widgets.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Pattern rule for compiling IceCream engine files
bin/%.o: $(ICECREAM_DIR)/src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

bin/main.o: src/main.cpp $(ICECREAM_DIR)/src/board.h src/gameController.h
	$(CXX) $(CXXFLAGS) -c src/main.cpp -o bin/main.o

bin/gameController.o: src/gameController.cpp src/gameController.h $(ICECREAM_DIR)/src/board.h
	$(CXX) $(CXXFLAGS) -c src/gameController.cpp -o bin/gameController.o

debug:
	$(CXX) -g $(CXXFLAGS) -o checkersDebug \
		src/main.cpp src/gameController.cpp $(ICECREAM_SRCS) $(WIDGET_SRCS) \
		$(LDFLAGS)

clean:
	rm -f bin/*.o bin/checkr
	rm -f *.stackdump *~

backup:
	test -d backups || mkdir backups
	cp src/*.cpp backups
	cp src/*.h backups