/*
 * board.h
 *
 *      Author: alex@alexanderresnik.com
 *
 * This header declares the core checkers engine types.
 *
 * The engine still uses the original compact 8 x 4 board representation
 * internally: each row stores only the four playable dark squares. The GUI,
 * however, works with normal 8 x 8 board coordinates. Public helper functions
 * near the top of the board class translate between those two coordinate
 * systems so the SDL/window layer does not need to know about the compressed
 * storage format.
 */

#ifndef BOARD_H_
#define BOARD_H_

#include <iostream>
#include <list>
#include <string>
#include <vector>

// A simple row/column coordinate used by the GUI-facing move path code.
// These coordinates are expanded 8 x 8 board coordinates, not compressed
// arr[row][compressedCol] coordinates.
struct Square
{
	int row;
	int col;
};

class jump
{
	// Previous jump in a multi-jump chain.
	// This lets the engine reconstruct full jump sequences recursively.
	jump *prev;

	// The checker that is performing this jump.
	// Lowercase pieces are normal checkers; uppercase pieces are kings.
	char jumpingPiece;

	// True when this jump node currently has no continuation jumps.
	// The jump-generation code uses this while building multi-jump paths.
	bool noNext;

	// Reference count for shared jump objects.
	//
	// A single jump node may be reused by more than one generated move when
	// jump paths branch. move::~move() decrements this count and only deletes
	// the jump when no remaining move references it.
	int numTimes;

	// The piece that was jumped over.
	// This is stored so undoMove() can restore captured pieces during AI search.
	char c;

	// Starting square of this jump in compressed board coordinates.
	int xs;
	int ys;

	// Square containing the captured piece, in compressed board coordinates.
	int x;
	int y;

	// Landing square of this jump in compressed board coordinates.
	int xend;
	int yend;

	// Encoded identifier used to prevent repeated captures of the same square
	// within one multi-jump sequence.
	int key;

	// Creates one jump node.
	//
	// jpingp = moving piece
	// piece  = captured piece
	// xs,ys  = start square
	// xc,yc  = captured square
	// xe,ye  = landing square
	// p      = previous jump in the chain
	// k      = repeat-detection key
	jump(char jpingp, char piece, int xs, int ys, int xc, int yc, int xe, int ye, jump *p, int k)
		: prev(p), jumpingPiece(jpingp), noNext(true), numTimes(0), c(piece), xs(xs), ys(ys),
		  x(xc), y(yc), xend(xe), yend(ye), key(k) {}

	//---------------------------------------------------------------------------------
	// friend classes:
	//---------------------------------------------------------------------------------

	// move owns/references jump pointers and releases them in its destructor.
	friend class move;

	// board creates and inspects jump nodes while generating legal moves.
	friend class board;
};

class move
{
	// The checker being moved.
	// This is preserved so undoMove() can restore the piece exactly as it was,
	// including king status.
	char mP;

	// Start square in compressed board coordinates.
	int xi;
	int yi;

	// End square in compressed board coordinates.
	int xf;
	int yf;

	// Captures that occur as part of this move.
	// Empty for a normal non-jump move; one or more entries for jumps.
	std::list<jump *> jpoints;

	// Expanded 8 x 8 path used by the GUI animation code.
	// For a multi-jump, this contains the intermediate landing squares so the
	// SDL layer can animate the checker through each jump rather than only from
	// the first square to the final square.
	std::vector<Square> path8x8;

	// Creates a move from one compressed board square to another.
	move(char c, int xs, int ys, int xe, int ye) : mP(c), xi(xs), yi(ys), xf(xe), yf(ye) {}

	// Releases any jump nodes referenced by this move.
	//
	// The implementation lives in board.cpp. Because jump nodes may be shared
	// between generated moves, the destructor uses jump::numTimes to avoid
	// deleting the same jump twice.
	~move();

	//---------------------------------------------------------------------------------
	// friend classes:
	//---------------------------------------------------------------------------------

	// board is responsible for creating, applying, undoing, and inspecting moves.
	friend class board;

	// game/gameController code may need direct access while coordinating AI moves.
	friend class game;
};

class board
{
	// Compact board representation.
	//
	// arr[row][compressedCol] stores only playable dark squares:
	// - row is 0..7
	// - compressedCol is 0..3
	//
	// This avoids storing the unused light squares from a normal 8 x 8 board.
	// Public helper functions convert this representation to 8 x 8 coordinates
	// for rendering and mouse input.
	char arr[8][4];

	// Current side to move:
	// 'b' = black
	// 'r' = red
	char color;

public:
	// Creates a new board in the standard starting position.
	board();

	// Deletes all generated moves currently stored in mlist.
	~board();

	// Copies the board position and side-to-move, but intentionally does not copy
	// the move list. AI search creates many temporary board positions, and each
	// copy should generate its own legal moves as needed.
	board(const board &);

	// Regenerates legal moves for the current side and returns true if at least
	// one legal move exists. This is a public wrapper used by the controller/UI.
	bool generateLegalMoves();

	// Read-only public accessor for the current turn.
	char getTurnPublic() const;

	// Exposes the current move list to code that still needs direct move access.
	// Prefer the 8 x 8 helper functions below for GUI-facing code when possible.
	std::list<move *> &getMoveList();

	// Converts an internal compressed column index to a visible 8 x 8 column.
	//
	// On even rows, playable squares are columns 1, 3, 5, 7.
	// On odd rows, playable squares are columns 0, 2, 4, 6.
	int toExpandedCol(int row, int compressedCol) const;

	// Returns the piece at an expanded 8 x 8 coordinate.
	//
	// Returns:
	// - 'b' or 'r' for normal pieces
	// - 'B' or 'R' for kings
	// - 'e' for an empty playable square
	// - 'x' for out-of-bounds or non-playable light squares
	char getPieceAt8x8(int row, int col) const;

	// Returns all legal destination squares, in expanded 8 x 8 coordinates, for
	// the selected expanded 8 x 8 source square.
	std::vector<std::pair<int, int>> getLegalDestinationsForSquare(int row, int col);

	// Attempts to make a legal move selected through expanded 8 x 8 coordinates.
	// Returns true only if the requested move matches one of the engine-generated
	// legal moves for the current side.
	bool tryMove8x8(int fromRow, int fromCol, int toRow, int toCol);

	// Finds the AI's best move and returns it in expanded 8 x 8 coordinates.
	//
	// The implementation uses iterative deepening plus alpha-beta search. The GUI
	// can call this from an async worker so the main SDL event loop remains
	// responsive while the AI is thinking.
	bool findBestMove8x8(int timeLimit, int &fromRow, int &fromCol, int &toRow, int &toCol);

	// Returns the expanded 8 x 8 path for a legal move.
	//
	// This is mainly used by animation code. For a normal move the path is simple;
	// for a multi-jump it includes intermediate landing squares.
	std::vector<std::pair<int, int>> getMovePath8x8(int fromRow, int fromCol, int toRow, int toCol);

	// Initializes or resets the board for a new game.
	void startup();

private:
	// Switches the current side to move after a move is applied.
	void changeTurn()
	{
		if (color == 'r')
			color = 'b';
		else
			color = 'r';
	}

	// Returns true when the current player has no legal moves.
	//
	// This also regenerates mlist through movesAvailable(), so callers should be
	// aware that checking for a terminal state refreshes the legal move list.
	bool terminalTest()
	{
		if (!movesAvailable())
			return true;
		return false;
	}

	//---------------------------------------------------------------------------------
	// General move generation helpers
	//---------------------------------------------------------------------------------

	// Populates mlist with legal moves for the current side.
	//
	// Checkers requires captures when available, so jumps are generated first.
	// Only if no jumps are available does the engine generate ordinary moves.
	bool movesAvailable()
	{
		if (jumpsAvailable())
			return true;
		if (listMoves())
			return true;
		return false;
	}

	// Promotes a normal checker to a king when it reaches the opposite end.
	//
	// Red moves toward row 0 and becomes 'R'.
	// Black moves toward row 7 and becomes 'B'.
	void handleKinging(const int &x, const int &y)
	{
		if (x == 0 && arr[x][y] == 'r')
			arr[x][y] = 'R';
		if (x == 7 && arr[x][y] == 'b')
			arr[x][y] = 'B';
	}

	// Returns true if the compressed board coordinate is inside arr[8][4].
	bool isValidPos(int i, int j)
	{
		if (i >= 0 && i < 8 && j >= 0 && j < 4)
			return true;
		else
			return false;
	}

	// Restores the standard starting position.
	// Called by startup() in boardPublic.cpp.
	void reset();

	//---------------------------------------------------------------------------------
	// Jump generation functions, implemented in boardJumps.cpp
	//---------------------------------------------------------------------------------

	int createkey(int, int, int, int, int, int);
	int reverse(int);
	void createJump(std::list<jump *> &, char, int, int, int, int, int, int, jump *);
	void createJumpMove(std::list<jump *> &);
	void jumpAvailable(std::list<jump *> &, char c, int, int, jump *);
	bool jumpsAvailable();
	bool jumpConditions(int, int, int, int);

	//---------------------------------------------------------------------------------
	// Regular move generation functions, implemented in boardMoves.cpp
	//---------------------------------------------------------------------------------

	void checkNeighbors(int &, int &);
	void createMove(const int &, const int &, int, int);
	bool listMoves();

	//---------------------------------------------------------------------------------
	// Public-board helpers implemented in boardPublic.cpp
	//---------------------------------------------------------------------------------

	// Adds one expanded 8 x 8 point to a move's animation path.
	void addPathPoint(move *m, int x, int y);

	//-------------------------------------------------------------------------------------
	// AI/search state and move list
	//-------------------------------------------------------------------------------------

	// Time budget, in seconds, for computer search.
	static int timeLimit;

	// Legal moves for the current board position.
	//
	// This list is regenerated frequently by terminalTest()/movesAvailable().
	// The board object owns these move pointers and deletes them in the destructor.
	std::list<move *> mlist;

	//---------------------------------------------------------------------------------
	// Engine functions implemented in boardPublic.cpp
	//---------------------------------------------------------------------------------

	// Applies a move to the board, removes captured pieces, handles kinging, and
	// changes the side to move.
	void makeMove(move *);

	// Reverses a move after AI search simulation.
	//
	// This restores captured pieces and returns the moving piece to its original
	// square. Search code calls changeTurn() separately after undoing because
	// makeMove() already changed the turn when the simulated move was applied.
	void undoMove(move *);

	// Endgame heuristic helper used by evaluate().
	//
	// Gives the losing side some value for occupying a double corner and gives
	// the winning side value for controlling diagonals near that corner.
	int cornerDiagonal(char, char);

	// Static board evaluation used by alpha-beta search.
	//
	// Positive scores favor black. Negative scores favor red.
	int evaluate();

	// Internal accessor used by search code.
	char getTurn()
	{
		return color;
	}
};

#endif /* BOARD_H_ */
