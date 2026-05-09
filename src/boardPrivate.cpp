/*
 * boardPrivate.cpp
 *
 *      Author: Harrison
 */

#include <assert.h>
#include "board.h"
#include <iostream>
#include <list>
#include <string>

using std::cout;
using std::endl;
using std::list;
using std::string;

// converts a position on the compressed 8x4 matrix
// to a component of a command for the expanded 8x8 matrix
// string s is the command the point is appended to
// don't need to bound check because that has
// already been done when creating moves and jumps
// called by createJumpMove in boardJumps.cpp
// called by createMove in boardMove.cpp
void board::convert(const int &x, const int &y, string &s)
{
	assert(0 <= x && x <= 7 && 0 <= y && y <= 3);
	char c1 = '0' + x;
	char c2;
	if (x % 2 == 0)
	{
		c2 = '0' + (2 * y + 1);
	}
	else
	{
		c2 = '0' + (2 * y);
	}
	s += c1;
	s += ' ';
	s += c2;
	s += ' ';
}

// used for printing out moves, converting the y coordinate in the matrix
// to the coordinate on the expanded 8x8 board
inline int board::convertY(const int &x, const int &y)
{
	if (x % 2 == 0)
		return (2 * y + 1);
	else
		return (2 * y);
}
