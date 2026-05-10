/*
 * board.h
 *
 *      Author: Harrison
 */

#ifndef BOARD_H_
#define BOARD_H_

#include <iostream>
#include <list>
#include <string>
#include <vector>

class jump
{
	// previous jump
	jump *prev;

	// the piece jumping
	char jumpingPiece;

	// when there are no next moves, noNext is true
	bool noNext;

	// used to keep track of how many times jump was inserted into a move,
	// increment every time jump is concatenated to a move
	// prevents double freeing of memory
	int numTimes;

	// character jumped over
	char c;

	// start point's x and y coordinate
	int xs;
	int ys;

	// jumped character's point coordinates
	int x;
	int y;

	// end point of the jump
	int xend;
	int yend;

	int key;

	// constructor for each data value
	jump(char jpingp, char piece, int xs, int ys, int xc, int yc, int xe, int ye, jump *p, int k)
		: prev(p), jumpingPiece(jpingp), noNext(true), numTimes(0), c(piece), xs(xs), ys(ys),
		  x(xc), y(yc), xend(xe), yend(ye), key(k) {}

	//---------------------------------------------------------------------------------
	// friend classes:
	//---------------------------------------------------------------------------------
	// pointers to jump are deleted in move's destructor
	friend class move;
	// board's members create jumps
	friend class board;
};

class move
{
	// moving piece
	char mP;

	// start point's coordinates
	int xi;
	int yi;

	// end point's coordinates
	int xf;
	int yf;

	// command used to map a string to the move
	// used for when the player enters a string for a move
	std::string command;

	// list storing all the jumps made by the move
	std::list<jump *> jpoints;

	// constructor for move
	move(char c, int xs, int ys, int xe, int ye) : mP(c), xi(xs), yi(ys), xf(xe), yf(ye) {}

	// destructor for move, found in board.cpp
	// frees all the heap allocated memory for the jumps in jpoints
	~move();

	//---------------------------------------------------------------------------------
	// friend classes:
	//---------------------------------------------------------------------------------
	// board creates moves and it accesses many of move's members
	friend class board;
	// move's member: command is accessed in game::outputMessage()
	friend class game;
};

class board
{
	// first coordinate is x, second is y
	// x is vertical down, y is horizontal
	// don't need an 8x8 array since only half the spaces are legal positions for pieces
	char arr[8][4];

	//'b' for black, 'r' for red
	// indicates who's turn it is
	char color;

	//[0] for black, [1] for red
	// default initialized to false since it's a static array
	// static bool isComputer[2];

	//---------------------------------------------------------------------------------
	// functions for board creation, found in board.cpp:
	//---------------------------------------------------------------------------------
	// 1: constructor for initializing an initial board
	// 2: destructor deallocates memory for all the moves in mlist
	// 3: copy constructor
	//   copies over all data values except the move list
	//   useful for creating new boards for each move in alpha-beta search
	// 4: changeTurn(), called after a move is made
	//   called in game.cpp by alphabeta
	//   called by makeMove, which is found in boardPublic.cpp
	//   is inlined
	// 5: converts a command stored in the form 2 3 3 2 -1 to (2,3) -> (3, 2)
	//   called in inputCommand in boardPublic.cpp
	// 6: create a list of moves by calling this
	//   is called each time a new board gets created after a move is made
	//   called by evaluate, in boardPublic.cpp
	//   is inlined

public:
	board();
	~board();
	board(const board &);

	bool generateLegalMoves();
	char getTurnPublic() const;
	std::list<move *> &getMoveList();

	int toExpandedCol(int row, int compressedCol) const;
	char getPieceAt8x8(int row, int col) const;

	std::vector<std::pair<int, int>> getLegalDestinationsForSquare(int row, int col);

	bool tryMove8x8(int fromRow, int fromCol, int toRow, int toCol);
	bool findBestMove8x8(int timeLimit, int &fromRow, int &fromCol, int &toRow, int &toCol);
	std::vector<std::pair<int, int>> getMovePath8x8(int fromRow, int fromCol, int toRow, int toCol);

private:
	void changeTurn()
	{
		if (color == 'r')
			color = 'b';
		else
			color = 'r';
	}

	bool terminalTest()
	{
		if (!movesAvailable())
			return true;
		return false;
	}

	//---------------------------------------------------------------------------------
	// general functions used for moves and jumps, found in board.cpp
	//---------------------------------------------------------------------------------
	// called in the terminalTest function
	// if there are any jumps, they are added to move list
	// else if there are any moves, they are added to the move list
	// if there are no jumps or moves available, it returns false
	// is inlined
	bool movesAvailable()
	{
		if (jumpsAvailable())
			return true;
		if (listMoves())
			return true;
		return false;
	}

	// if a red piece 'r' reaches the red side's end, it becomes a red king 'R'
	// if a black piece 'b' reaches the black side's end, it becomes a black king 'B'
	// called at the end of the makeMove function, which is found in boardPublic.cpp
	// is inlined
	void handleKinging(const int &x, const int &y)
	{
		if (x == 0 && arr[x][y] == 'r')
			arr[x][y] = 'R';
		if (x == 7 && arr[x][y] == 'b')
			arr[x][y] = 'B';
	}

	// returns true if the position arr[i][j] is a valid position on the checker board
	// called by jumpConditions, which is found in boardJumps.cpp
	// called by createMove, which is found in boardMoves.cpp
	// is inlined
	bool isValidPos(int i, int j)
	{
		if (i >= 0 && i < 8 && j >= 0 && j < 4)
			return true;
		else
			return false;
	}

	// reset the board after game over
	// called by startup, which is in boardPublic.cpp
	void reset();

	//---------------------------------------------------------------------------------
	// functions for jumps: found in boardJumps.cpp
	//---------------------------------------------------------------------------------
	// createkey is inlined
	int createkey(int, int, int, int, int, int);
	int reverse(int);
	void createJump(std::list<jump *> &, char, int, int, int, int, int, int, jump *);
	void createJumpMove(std::list<jump *> &);
	void jumpAvailable(std::list<jump *> &, char c, int, int, jump *);
	bool jumpsAvailable();
	bool jumpConditions(int, int, int, int);

	//---------------------------------------------------------------------------------
	// functions for regular moves, found in boardMoves.cpp
	//---------------------------------------------------------------------------------
	void checkNeighbors(int &, int &);
	void createMove(const int &, const int &, int, int);
	bool listMoves();

	//---------------------------------------------------------------------------------
	// functions for printing, found in boardPublic.cpp
	//---------------------------------------------------------------------------------
	// converts a point to string form and appends it to command list for a move
	// called by createJumpMove in boardJumps.cpp
	// called by createMove in boardMoves.cpp
	void convert(const int &, const int &, std::string &);

	//-------------------------------------------------------------------------------------
	// FUNCTIONS AND MEMBERS UTILIZED DIRECTLY IN GAME.H:
	//-------------------------------------------------------------------------------------
	// timer for the computer
	static int timeLimit;

	// list of moves for the board:
	std::list<move *> mlist;

	//---------------------------------------------------------------------------------
	// functions found in boardPublic.cpp, functions called in gameController.cpp
	//---------------------------------------------------------------------------------

	// makes the move
	// should be used on a copy of a board when alpha-beta searching
	void makeMove(move *);

	// reverses a move
	void undoMove(move *);

	// checks double corners and diagonals near end game
	// gives points for occupying a double corner for losing player
	// winning player gets points for occupying a diagonal near losing player's corner
	// called by evaluate() in boardPublic.cpp
	int cornerDiagonal(char, char);

	// Evaluation function, need to do
	// will be implemented in alpha-beta search
	int evaluate();

	// determines whether or not players will be a computer calls modifyBoard
	void startup();

	// gets the current color's turn
	// is inlined
	char getTurn()
	{
		return color;
	}
};

#endif /* BOARD_H_ */
