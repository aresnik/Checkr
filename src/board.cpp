/*
 * board.cpp
 *
 *      Author: Harrison
 */

#include <assert.h>
#include "board.h"
#include <cctype>
#include <list>

using std::list;

int board::timeLimit = 30;

// frees the memory allocated on the heap for each jump pointer
// avoids double freeing of memory by keeping track of the
// number of times each jump was added to a moves jump list
// first decrements each jump's numTimes
// only deletes the jump if numTimes equals 0
// this is necessary for multiple moves utilizing the same jumps,
// such as in the case of branching jumps:
//			1
//		2
//	3		3'
//		4
//  1 -> 2 would have numTimes equal to 2 since the jump would be utilized twice,
// once for each move
move::~move()
{
	for (list<jump *>::iterator it = jpoints.begin(); it != jpoints.end(); ++it)
	{
		--(*it)->numTimes;
		if ((*it)->numTimes == 0)
			delete (*it);
	}
}

// initializes everything for the checker board
board::board()
{
	reset();
}

// destructor deallocates memory for all the moves in mlist
board::~board()
{
	while (!mlist.empty())
	{
		delete mlist.front();
		mlist.pop_front();
	}
}

// copy constructor: copies over all data values except the move list
// useful for creating new boards for each move in alpha-beta search
board::board(const board &b) : color(b.color)
{
	for (int i = 0; i != 8; ++i)
		for (int j = 0; j != 4; ++j)
			arr[i][j] = b.arr[i][j];
}

// resets the board, called by printEBoard in boardPublic.cpp
// create the start board
// first three rows are filled with black pieces
// next two rows are empty
// last three rows are filled with red pieces
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

bool board::generateLegalMoves()
{
	return !terminalTest();
}

char board::getTurnPublic() const
{
	return color;
}

std::list<move *> &board::getMoveList()
{
	return mlist;
}

int board::toExpandedCol(int row, int compressedCol) const
{
	if (row % 2 == 0)
		return (2 * compressedCol + 1);
	else
		return (2 * compressedCol);
}

char board::getPieceAt8x8(int row, int col) const
{
	if (row < 0 || row >= 8 || col < 0 || col >= 8)
		return 'x';

	// Light square → not playable
	if ((row + col) % 2 == 0)
		return 'x';

	int compressedCol;
	if (row % 2 == 0)
		compressedCol = (col - 1) / 2;
	else
		compressedCol = col / 2;

	if (compressedCol < 0 || compressedCol >= 4)
		return 'x';

	return arr[row][compressedCol];
}
