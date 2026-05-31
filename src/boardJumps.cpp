/*
 * boardJumps.cpp
 *
 *      Author: alex@alexanderresnik.com
 */

#include <assert.h>
#include "board.h"
#include <iostream>
#include <cctype>
#include <list>
#include <memory>

using std::list;

// create a "code" for each jump, used to prevent repetitions
inline int board::createkey(int xs, int ys, int xj, int yj, int xe, int ye)
{
	return ye + xe * 10 + yj * 100 + xj * 1000 + ys * 10000 + xs * 100000;
}

// reverses a jump's key
// so 233444 will be broken up into 23 34 44 and reorganized to become 444332
// this is to prevent any repeated jumps
int board::reverse(int i)
{
	int num = 0;
	while (i / 100 > 0)
	{
		num += i % 100;
		num *= 100;
		i /= 100;
	}
	num += i;
	return num;
}

// creates a new jump given the start position (xs, ys),
// jumped over position (xj, yj), and end position (xe, ye)
// also takes a jump pointer jp which represents the previous jump
// it will be added in when the jumps are connected into a move
// instead the character is passed
// recursively call jumpAvailable, using the new end position
// keep track of what piece is currently making the jumps by passing it as parameter
void board::createJump(list<std::shared_ptr<jump>> &jlist, char c, int xs, int ys, int xj, int yj, int xe, int ye, std::shared_ptr<jump> jp)
{
	arr[xs][ys] = 'e';
	//+1 to each because 0 will mess up the keys and reverse for an edge case such as xs=0 ys =0
	int key = createkey(xs + 1, ys + 1, xj + 1, yj + 1, xe + 1, ye + 1);
	std::shared_ptr<jump> jcheck = jp;
	while (jcheck != nullptr)
	{
		if (key == jcheck->key || key == reverse(jcheck->key))
			return;
		jcheck = jcheck->prev;
	}
	auto j = std::make_shared<jump>(c, arr[xj][yj], xs, ys, xj, yj, xe, ye, jp, key);
	if (jp != nullptr)
		jp->noNext = false;
	jlist.push_front(j);
	jumpAvailable(jlist, c, xe, ye, j);
}

// iterate through the list of jumps
// create new moves only when encountering the last jump
// add all the appropriate jumps to the move's list of jumps via backtracking
// increment the jump's numTimes
// repeat the loop until the first jump was added to the move's jump list
// add the start point to the move's start position
// undoes each jump, so the starting character is where it began before getting erased
// replaces the 'e's with original characters
void board::createJumpMove(list<std::shared_ptr<jump>> &jlist)
{
	if (!jlist.empty())
	{
		for (auto it = jlist.begin(); it != jlist.end(); ++it)
		{
			if ((*it)->noNext)
			{
				auto m = std::make_unique<move>((*it)->jumpingPiece, -1, -1, -1, -1);
				std::shared_ptr<jump> jp = (*it);
				while (jp != nullptr)
				{
					m->jpoints.push_front(jp);
					jp = jp->prev;
				}
				m->xi = m->jpoints.front()->xs;
				m->yi = m->jpoints.front()->ys;

				addPathPoint(m, m->jpoints.front()->xs, m->jpoints.front()->ys);

				for (auto it_j = m->jpoints.begin(); it_j != m->jpoints.end(); ++it_j)
				{
					addPathPoint(m, (*it_j)->xend, (*it_j)->yend);

					if ((*it_j)->noNext)
					{
						m->xf = (*it_j)->xend;
						m->yf = (*it_j)->yend;
					}
				}

				undoMove(m);
				mlist.push_back(std::move(m));
			}
		}
	}
}

// checking for jumping in all four directions
//(x,y) is the start point
void board::jumpAvailable(list<std::shared_ptr<jump>> &jlist, char c, int x, int y, std::shared_ptr<jump> jp)
{
	if (tolower(c) == 'b' || c == 'R')
	{
		if (x % 2 == 0) // even x
		{
			if (jumpConditions(x + 1, y, x + 2, y - 1)) // checks left down jump
				createJump(jlist, c, x, y, x + 1, y, x + 2, y - 1, jp);
			if (jumpConditions(x + 1, y + 1, x + 2, y + 1)) // checks right down jump
				createJump(jlist, c, x, y, x + 1, y + 1, x + 2, y + 1, jp);
		}
		else // odd x
		{
			if (jumpConditions(x + 1, y - 1, x + 2, y - 1)) // checks left down jump
				createJump(jlist, c, x, y, x + 1, y - 1, x + 2, y - 1, jp);
			if (jumpConditions(x + 1, y, x + 2, y + 1)) // checks right down jump
				createJump(jlist, c, x, y, x + 1, y, x + 2, y + 1, jp);
		}
	}
	if (tolower(c) == 'r' || c == 'B')
	{
		if (x % 2 == 0) // even x
		{
			if (jumpConditions(x - 1, y + 1, x - 2, y + 1)) // checks right up jump
				createJump(jlist, c, x, y, x - 1, y + 1, x - 2, y + 1, jp);
			if (jumpConditions(x - 1, y, x - 2, y - 1)) // checks left up jump
				createJump(jlist, c, x, y, x - 1, y, x - 2, y - 1, jp);
		}
		else // odd x
		{
			if (jumpConditions(x - 1, y - 1, x - 2, y - 1)) // checks left up jump
				createJump(jlist, c, x, y, x - 1, y - 1, x - 2, y - 1, jp);
			if (jumpConditions(x - 1, y, x - 2, y + 1)) // checks right up jump
				createJump(jlist, c, x, y, x - 1, y, x - 2, y + 1, jp);
		}
	}
}

// called by movesAvailable in board.h
// clears all moves
// if the piece belongs to the current turn's color
// check it for jumps
// then create jumping moves once the search is finished
bool board::jumpsAvailable()
{
	mlist.clear();
	for (int i = 0; i != 8; ++i)
	{
		for (int j = 0; j != 4; ++j)
		{
			if (arr[i][j] == color || arr[i][j] == toupper(color))
			{
				list<std::shared_ptr<jump>> jlist;
				jumpAvailable(jlist, arr[i][j], i, j, nullptr);
				createJumpMove(jlist);
			}
		}
	}

	// if no jumping moves were added, return false, else return true
	if (mlist.empty())
		return false;
	return true;
}

// checks for jumping conditions
// checks if the jumped point is valid and has the enemy's color
// checks if the end point is valid and empty
// returns true if conditions are satisfied
bool board::jumpConditions(int xj, int yj, int xe, int ye)
{
	if (isValidPos(xj, yj) && isValidPos(xe, ye) && arr[xj][yj] != 'e' &&
		arr[xj][yj] != color && arr[xe][ye] == 'e' && arr[xj][yj] != std::toupper(color))
		return true;
	return false;
}
