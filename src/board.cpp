/*
 * board.cpp
 *
 *      Author: alex@alexanderresnik.com
 *
 * This file contains the core constructors/destructors and a small set of
 * public helper functions for the board class.
 *
 * Important design note:
 * The engine stores only the playable dark squares of the checkerboard.
 * Instead of an 8 x 8 array, it uses arr[8][4], where each row has only
 * four playable columns. The GUI, however, works in normal 8 x 8 screen
 * coordinates, so several helper functions below translate between the
 * compressed engine representation and expanded 8 x 8 board coordinates.
 */

#include <assert.h>
#include "board.h"
#include <cctype>
#include <list>

using std::list;

// Default AI thinking time, in seconds.
// This is a static board member, so it is shared across board instances.
int board::timeLimit = 30;

/*
 * move destructor
 *
 * A move can contain one or more jump pointers in jpoints. Some jump objects
 * may be shared by multiple generated moves when the jump tree branches.
 *
 * Example:
 *
 *          1
 *       2
 *    3     3'
 *       4
 *
 * In a branching multi-jump search, the same early jump can be reused by
 * more than one final move path. To prevent deleting the same jump object
 * twice, each jump tracks how many move lists reference it through numTimes.
 *
 * This destructor decrements each jump's reference count and deletes the jump
 * only when no remaining move refers to it.
 */
move::~move()
{
	for (list<jump *>::iterator it = jpoints.begin(); it != jpoints.end(); ++it)
	{
		--(*it)->numTimes;
		if ((*it)->numTimes == 0)
			delete (*it);
	}
}

/*
 * board constructor
 *
 * Creates a new board and initializes it to the standard starting position.
 */
board::board()
{
	reset();
}

/*
 * board destructor
 *
 * The board owns the move pointers stored in mlist, so it must delete them
 * before the board object is destroyed.
 */
board::~board()
{
	while (!mlist.empty())
	{
		delete mlist.front();
		mlist.pop_front();
	}
}

/*
 * board copy constructor
 *
 * Copies the logical board position and side-to-move color.
 *
 * The move list is intentionally not copied. Legal moves are generated from
 * the copied position when needed. This is especially useful for AI search,
 * where many temporary board objects are created while exploring possible
 * future game states.
 */
board::board(const board &b) : color(b.color)
{
	for (int i = 0; i != 8; ++i)
		for (int j = 0; j != 4; ++j)
			arr[i][j] = b.arr[i][j];
}

/*
 * reset
 *
 * Restores the board to the standard checkers starting position.
 *
 * Internal representation:
 *   'b' = black regular piece
 *   'r' = red regular piece
 *   'e' = empty playable square
 *
 * The first three rows contain black pieces, the middle two rows are empty,
 * and the last three rows contain red pieces.
 *
 * The turn is reset to red. If you later want black to move first, this is
 * one of the places where that starting-turn behavior would be changed.
 */
void board::reset()
{
	color = 'r';
	for (int i = 0; i != 3; ++i)
		for (int j = 0; j != 4; ++j)
			arr[i][j] = 'b';
	for (int i = 3; i != 5; ++i)
		for (int j = 0; j != 4; ++j)
			arr[i][j] = 'e';
	for (int i = 5; i != 8; ++i)
		for (int j = 0; j != 4; ++j)
			arr[i][j] = 'r';
}

/*
 * generateLegalMoves
 *
 * Public wrapper used by outside code that needs the board's legal move list
 * refreshed without directly calling lower-level engine internals.
 *
 * terminalTest() both checks for game-over conditions and regenerates mlist
 * as part of the engine's existing move-generation flow. Returning the inverse
 * means:
 *
 *   true  = at least one legal move/game can continue
 *   false = terminal state/no legal moves
 */
bool board::generateLegalMoves()
{
	return !terminalTest();
}

/*
 * getTurnPublic
 *
 * Returns the current side to move.
 *
 *   'r' = red to move
 *   'b' = black to move
 *
 * This is a const-safe public accessor for GUI/controller code.
 */
char board::getTurnPublic() const
{
	return color;
}

/*
 * getMoveList
 *
 * Returns the current list of legal moves.
 *
 * This exposes the engine's move list to controller/UI code that needs to
 * inspect legal moves. The caller should make sure move generation has already
 * been refreshed, typically by calling generateLegalMoves() or by using an
 * engine function that calls terminalTest().
 */
std::list<move *> &board::getMoveList()
{
	return mlist;
}

/*
 * toExpandedCol
 *
 * Converts an internal compressed column index into a normal 8 x 8 board
 * column index.
 *
 * The engine stores only playable dark squares:
 *
 *   arr[row][0..3]
 *
 * The GUI uses full board coordinates:
 *
 *   row 0..7, col 0..7
 *
 * On even rows, playable squares are at columns 1, 3, 5, 7.
 * On odd rows, playable squares are at columns 0, 2, 4, 6.
 */
int board::toExpandedCol(int row, int compressedCol) const
{
	if (row % 2 == 0)
		return (2 * compressedCol + 1);
	else
		return (2 * compressedCol);
}

/*
 * getPieceAt8x8
 *
 * Returns the piece located at an expanded 8 x 8 GUI coordinate.
 *
 * Valid return values from the board:
 *   'b' = black regular piece
 *   'B' = black king
 *   'r' = red regular piece
 *   'R' = red king
 *   'e' = empty playable square
 *
 * This function returns 'e' when the requested square is invalid or not
 * playable. That lets the SDL/GUI layer safely ask about any screen square
 * without needing direct knowledge of the compressed arr[8][4] layout.
 */
char board::getPieceAt8x8(int row, int col) const
{
	if (row < 0 || row >= 8 || col < 0 || col >= 8)
		return 'e';

	// Light squares are not playable in checkers and are not stored in arr.
	if ((row + col) % 2 == 0)
		return 'e';

	int compressedCol;
	if (row % 2 == 0)
		compressedCol = (col - 1) / 2;
	else
		compressedCol = col / 2;

	if (compressedCol < 0 || compressedCol >= 4)
		return 'e';

	return arr[row][compressedCol];
}
