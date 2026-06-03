CXX = clang++

# Path to the UIWidgets library (sibling directory)
WIDGET_DIR = ../UIWidgets

CXXFLAGS = -std=c++17 -Wall -I/opt/homebrew/include -I$(WIDGET_DIR)/include
LDFLAGS = -L/opt/homebrew/lib -lSDL3 -lSDL3_image -lSDL3_ttf

TARGET = bin/checkr

# Object groups
BOARD_OBJS = bin/board.o bin/boardJumps.o bin/boardMoves.o bin/boardPublic.o
APP_OBJS = bin/main.o bin/gameController.o

# More succinct way to find all widget sources and objects
WIDGET_SRCS = $(wildcard $(WIDGET_DIR)/include/*.cpp)
WIDGET_OBJS = $(patsubst $(WIDGET_DIR)/include/%.cpp, bin/%.o, $(WIDGET_SRCS))

$(TARGET): $(APP_OBJS) $(BOARD_OBJS) $(WIDGET_OBJS)
	$(CXX) -o $(TARGET) $(APP_OBJS) $(BOARD_OBJS) $(WIDGET_OBJS) $(LDFLAGS)

# Pattern rule for compiling UIWidgets library files
bin/%.o: $(WIDGET_DIR)/include/%.cpp $(WIDGET_DIR)/include/widgets.h
	$(CXX) $(CXXFLAGS) -c $< -o $@


bin/main.o: src/main.cpp src/board.h src/gameController.h
	$(CXX) $(CXXFLAGS) -c src/main.cpp -o bin/main.o

bin/board.o: src/board.cpp src/board.h
	$(CXX) $(CXXFLAGS) -c src/board.cpp -o bin/board.o

bin/boardJumps.o: src/boardJumps.cpp src/board.h
	$(CXX) $(CXXFLAGS) -c src/boardJumps.cpp -o bin/boardJumps.o

bin/boardMoves.o: src/boardMoves.cpp src/board.h
	$(CXX) $(CXXFLAGS) -c src/boardMoves.cpp -o bin/boardMoves.o

bin/boardPublic.o: src/boardPublic.cpp src/board.h
	$(CXX) $(CXXFLAGS) -c src/boardPublic.cpp -o bin/boardPublic.o

bin/gameController.o: src/gameController.cpp src/gameController.h
	$(CXX) $(CXXFLAGS) -c src/gameController.cpp -o bin/gameController.o

debug:
	$(CXX) -g $(CXXFLAGS) -o checkersDebug \
		src/main.cpp src/board.cpp src/boardJumps.cpp src/boardMoves.cpp \
		src/boardPublic.cpp src/gameController.cpp $(WIDGET_SRCS) \
		$(LDFLAGS)

clean:
	rm -f bin/*.o *.stackdump *~

backup:
	test -d backups || mkdir backups
	cp src/*.cpp backups
	cp src/*.h backups