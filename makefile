CXX = clang++

# Path to the IceCream library (sibling directory)
# Note: Avoid quotes and trailing spaces in variable definitions.
ICECREAM_DIR = ../IceCream
# Path to the UIWidgets library (sibling directory)
WIDGET_DIR = ../UIWidgets

CXXFLAGS = -std=c++17 -g -Wall -O3 -I/opt/homebrew/include -I$(WIDGET_DIR)/src -I$(ICECREAM_DIR)/src
LDFLAGS = -L/opt/homebrew/lib -lSDL3 -lSDL3_image -lSDL3_ttf -lSDL3_mixer

TARGET = bin/checkr

# Object groups
APP_OBJS = bin/main.o bin/gameController.o bin/assetManager.o bin/gameScene.o bin/mainMenuScene.o bin/boardWidget.o bin/gameSetupDialog.o

# All widget sources and objects
WIDGET_SRCS = $(shell find $(WIDGET_DIR)/src -name "*.cpp") # Use shell find for paths with spaces
WIDGET_OBJS = $(patsubst $(WIDGET_DIR)/src/%.cpp, bin/%.o, $(WIDGET_SRCS))

# All IceCream sources and objects
ICECREAM_SRCS = $(shell find $(ICECREAM_DIR)/src -name "*.cpp") # Use shell find for paths with spaces
ICECREAM_OBJS = $(patsubst $(ICECREAM_DIR)/src/%.cpp, bin/%.o, $(ICECREAM_SRCS))

$(TARGET): $(APP_OBJS) $(WIDGET_OBJS) $(ICECREAM_OBJS)
	$(CXX) -o $(TARGET) $(APP_OBJS) $(WIDGET_OBJS) $(ICECREAM_OBJS) $(LDFLAGS)

# Pattern rule for compiling UIWidgets library files
bin/%.o: $(WIDGET_DIR)/src/%.cpp $(WIDGET_DIR)/src/widgets.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Pattern rule for compiling IceCream engine files
bin/%.o: $(ICECREAM_DIR)/src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

bin/assetManager.o: src/assetManager.cpp src/assetManager.h
	$(CXX) $(CXXFLAGS) -c src/assetManager.cpp -o bin/assetManager.o

bin/gameScene.o: src/gameScene.cpp src/gameScene.h src/appState.h src/scene.h $(WIDGET_DIR)/src/widgets.h $(WIDGET_DIR)
	$(CXX) $(CXXFLAGS) -c src/gameScene.cpp -o bin/gameScene.o

bin/gameSetupDialog.o: src/gameSetupDialog.cpp src/gameSetupDialog.h src/appState.h $(WIDGET_DIR)/src/widgets.h
	$(CXX) $(CXXFLAGS) -c src/gameSetupDialog.cpp -o bin/gameSetupDialog.o

bin/boardWidget.o: src/boardWidget.cpp src/boardWidget.h src/appState.h
	$(CXX) $(CXXFLAGS) -c src/boardWidget.cpp -o bin/boardWidget.o

bin/mainMenuScene.o: src/mainMenuScene.cpp src/mainMenuScene.h src/appState.h src/scene.h $(WIDGET_DIR)/src/widgets.h $(WIDGET_DIR)
	$(CXX) $(CXXFLAGS) -c src/mainMenuScene.cpp -o bin/mainMenuScene.o

bin/main.o: src/main.cpp $(ICECREAM_DIR)/src/board.h src/gameController.h src/assetManager.h src/appState.h src/gameScene.h src/mainMenuScene.h
	$(CXX) $(CXXFLAGS) -c src/main.cpp -o bin/main.o

bin/gameController.o: src/gameController.cpp src/gameController.h $(ICECREAM_DIR)/src/board.h
	$(CXX) $(CXXFLAGS) -c src/gameController.cpp -o bin/gameController.o

debug:
	$(CXX) -g $(CXXFLAGS) -o checkersDebug \
		src/main.cpp src/gameController.cpp src/assetManager.cpp src/gameScene.cpp src/mainMenuScene.cpp src/boardWidget.cpp src/gameSetupDialog.cpp \
		$(ICECREAM_SRCS) $(WIDGET_SRCS) \
		$(LDFLAGS)

clean:
	rm -f bin/*.o bin/checkr
	rm -f *.stackdump *~

backup:
	test -d backups || mkdir backups
	cp src/*.cpp backups
	cp src/*.h backups