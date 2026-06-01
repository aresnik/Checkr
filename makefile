CXX = clang++
CXXFLAGS = -std=c++17 -Wall -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib -lSDL3 -lSDL3_image -lSDL3_ttf

TARGET = bin/checkr

$(TARGET): bin/main.o bin/board.o bin/boardJumps.o bin/boardMoves.o bin/boardPublic.o bin/gameController.o bin/uiWidgets.o
	$(CXX) -o $(TARGET) bin/main.o bin/board.o bin/boardJumps.o bin/boardMoves.o bin/boardPublic.o bin/gameController.o bin/uiWidgets.o $(LDFLAGS)

bin/uiWidgets.o: src/uiWidgets.cpp src/uiWidgets.h
	$(CXX) $(CXXFLAGS) -c src/uiWidgets.cpp -o bin/uiWidgets.o

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
		src/boardPublic.cpp src/gameController.cpp \
		$(LDFLAGS)

clean:
	rm -f bin/*.o *.stackdump *~

backup:
	test -d backups || mkdir backups
	cp src/*.cpp backups
	cp src/*.h backups