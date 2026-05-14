/*
 * boardPublic.cpp
 *
 *      Author: alex@alexanderresnik.com
 */

#include <assert.h>
#include "board.h"
#include <cctype>
#include <cstdlib>
#include <list>
#include <string>
#include <vector>
#include <ctime>
#include <functional>
#include <sstream>

using std::cout;
using std::endl;
using std::ifstream;
using std::list;
using std::string;
using std::tolower;

// makes a move
// if there's any jumps, they are implemented
// pieces are erased and subtracted from the total count if necessary
// moves the piece from one position to another
void board::makeMove(move *m)
{
	if (!m->jpoints.empty())
	{
		list<jump *>::iterator it = m->jpoints.begin();
		for (; it != m->jpoints.end(); ++it)
			arr[(*it)->x][(*it)->y] = 'e';
	}

	// save the piece
	// replace the start position with an empty space
	// add back in the saved piece at the end point
	char c = arr[m->xi][m->yi];
	arr[m->xi][m->yi] = 'e';
	arr[m->xf][m->yf] = c;

	// check if the piece should be changed to a king and change player's turn
	handleKinging(m->xf, m->yf);
	changeTurn();
}

// undoes a move
// replaces the starting jump point only if the starting jump hasn't already been replaced
// iterate through its list of jumps
// add back all the characters that were temporarily deleted
// add the jumping piece in the start position of the move
void board::undoMove(move *m)
{
	if (!m->jpoints.empty())
	{
		for (list<jump *>::iterator it = m->jpoints.begin(); it != m->jpoints.end(); ++it)
		{
			arr[(*it)->xs][(*it)->ys] = 'e';
			arr[(*it)->x][(*it)->y] = (*it)->c;
			arr[(*it)->xend][(*it)->yend] = 'e';
		}
	}
	arr[m->xf][m->yf] = 'e';
	arr[m->xi][m->yi] = m->mP;
}

// gives a small bonus to losing player for being in a double corner
// gives a smaller bonus to winning player for being on a diagonal close to the losing piece's corner
// will help winning player force losing player out of a corner
int board::cornerDiagonal(char losing, char winning)
{
	int c = 0;
	if (tolower(arr[0][0]) == losing || tolower(arr[1][0]) == losing)
	{
		c += 9;
		if (tolower(arr[0][0]) == winning)
			c -= 3;
		if (tolower(arr[1][0]) == winning)
			c -= 3;
		if (tolower(arr[1][1]) == winning)
			c -= 1;
		if (tolower(arr[2][0]) == winning)
			c -= 1;
		if (tolower(arr[2][1]) == winning)
			c -= 1;
		if (tolower(arr[3][1]) == winning)
			c -= 1;
	}
	if (tolower(arr[6][3]) == losing || tolower(arr[7][3]) == losing)
	{
		c += 9;
		if (tolower(arr[4][2]) == winning)
			c -= 1;
		if (tolower(arr[5][2]) == winning)
			c -= 1;
		if (tolower(arr[5][3]) == winning)
			c -= 1;
		if (tolower(arr[6][2]) == winning)
			c -= 1;
		if (tolower(arr[6][3]) == winning)
			c -= 3;
		if (tolower(arr[7][3]) == winning)
			c -= 3;
	}
	return c;
}

// black is more positive
// red is more negative
// aabbccddee
// aa is a count of all pieces: king is worth a total of 3, regular pieces are 2 (black - red)
// bb is measuring how close a normal piece is to becoming a king, give last row a bonus
// cc is a piece count difference
// dd is a measurement near end game, double corners add a bonus for losing player
// ee is pseudo-random in the case that multiple moves tie for best
int board::evaluate()
{
	int a1 = 0, a2 = 0, b = 0, c = 0, d = 0;
	for (int i = 0; i != 8; ++i)
		for (int j = 0; j != 4; ++j)
		{
			if (arr[i][j] == 'b')
			{
				a1 += 2;
				if (i == 0)
					b += 9;
				else
					b += i;
				c += 1;
			}
			else if (arr[i][j] == 'r')
			{
				a2 -= 2;
				if (i == 7)
					b -= 9;
				else
					b -= (7 - i);
				c -= 1;
			}
			else if (arr[i][j] == 'B')
			{
				a1 += 3;
				c += 1;
			}
			else if (arr[i][j] == 'R')
			{
				a2 -= 3;
				c -= 1;
			}
		}
	if (c > 0 && a2 >= -8)
		d -= cornerDiagonal('r', 'b');
	else if (c < 0 && a1 <= 8)
		d += cornerDiagonal('b', 'r');
	a1 *= 100000000;
	a2 *= 100000000;
	b *= 1000000;
	c *= 10000;
	d *= 100;
	int e = rand() % 100;
	if (color == 'r')
		e = -e;
	return a1 + a2 + b + c + d + e;
}

void board::startup()
{
	// reset the board
	reset();
}

void board::addPathPoint(move *m, int x, int y)
{
	// New structured path
	m->path8x8.push_back({x, toExpandedCol(x, y)});
}

std::vector<std::pair<int, int>> board::getLegalDestinationsForSquare(int row, int col)
{
	std::vector<std::pair<int, int>> result;

	// Regenerate legal moves for the current side to move.
	// If terminalTest() returns true, the game is over and there are no moves.
	if (terminalTest())
		return result;

	for (std::list<move *>::iterator it = mlist.begin(); it != mlist.end(); ++it)
	{
		move *m = *it;

		int start_col;
		if (m->xi % 2 == 0)
			start_col = 2 * m->yi + 1;
		else
			start_col = 2 * m->yi;

		if (m->xi == row && start_col == col)
		{
			int end_col;
			if (m->xf % 2 == 0)
				end_col = 2 * m->yf + 1;
			else
				end_col = 2 * m->yf;

			result.push_back(std::make_pair(m->xf, end_col));
		}
	}

	return result;
}

bool board::tryMove8x8(int fromRow, int fromCol, int toRow, int toCol)
{
	// basic bounds check
	if (fromRow < 0 || fromRow >= 8 || fromCol < 0 || fromCol >= 8 ||
		toRow < 0 || toRow >= 8 || toCol < 0 || toCol >= 8)
	{
		return false;
	}

	// playable squares in your expanded 8x8 are dark squares only
	if ((fromRow + fromCol) % 2 == 0 || (toRow + toCol) % 2 == 0)
	{
		return false;
	}

	// regenerate legal moves for the current side to move
	if (terminalTest())
	{
		return false;
	}

	for (std::list<move *>::iterator it = mlist.begin(); it != mlist.end(); ++it)
	{
		move *m = *it;

		int start_col = toExpandedCol(m->xi, m->yi);
		int end_col = toExpandedCol(m->xf, m->yf);

		if (m->xi == fromRow && start_col == fromCol &&
			m->xf == toRow && end_col == toCol)
		{
			makeMove(m);
			return true;
		}
	}

	return false;
}

std::vector<std::pair<int, int>> board::getMovePath8x8(int fromRow, int fromCol, int toRow, int toCol)
{
	std::vector<std::pair<int, int>> path;

	// Basic bounds check
	if (fromRow < 0 || fromRow >= 8 || fromCol < 0 || fromCol >= 8 ||
		toRow < 0 || toRow >= 8 || toCol < 0 || toCol >= 8)
	{
		return path;
	}

	// Only dark squares are playable
	if ((fromRow + fromCol) % 2 == 0 || (toRow + toCol) % 2 == 0)
	{
		return path;
	}

	// Regenerate legal moves for the current side to move
	if (terminalTest())
	{
		return path;
	}

	for (move *m : mlist)
	{
		int startCol = toExpandedCol(m->xi, m->yi);
		int endCol = toExpandedCol(m->xf, m->yf);

		if (m->xi == fromRow && startCol == fromCol &&
			m->xf == toRow && endCol == toCol)
		{
			for (const Square &sq : m->path8x8)
				path.push_back({sq.row, sq.col});

			return path;
		}
	}

	return path;
}

bool board::findBestMove8x8(int timeLimit, int &fromRow, int &fromCol, int &toRow, int &toCol)
{
	const int maxIterDepth = 20;

	fromRow = -1;
	fromCol = -1;
	toRow = -1;
	toCol = -1;

	if (timeLimit < 1)
		timeLimit = 1;

	// Generate legal moves for the current side to move.
	if (terminalTest())
		return false;

	// If there is only one legal move, return it immediately.
	if (mlist.size() == 1)
	{
		move *m = mlist.front();
		fromRow = m->xi;
		fromCol = toExpandedCol(m->xi, m->yi);
		toRow = m->xf;
		toCol = toExpandedCol(m->xf, m->yf);
		return true;
	}

	move *bestM = NULL;
	move *tempBestM = NULL;

	int maxdepth = 0;
	int cdepth = 0;
	bool timeUp = false;
	bool reachedEnd = false;

	std::time_t startTime = 0;
	std::time_t endTime = 0;
	std::time_t startTimeD = 0;
	std::time_t endTimeD = 0;

	std::function<int(board &, int, int, int)> alphabeta =
		[&](board &b, int depth, int alpha, int beta) -> int
	{
		if (depth != maxdepth && b.terminalTest())
		{
			reachedEnd = true;
			cdepth = maxdepth;

			if (b.getTurn() == 'r')
				return std::numeric_limits<int>::max();
			else
				return std::numeric_limits<int>::min();
		}

		reachedEnd = false;

		if (depth == 0)
			return b.evaluate();

		std::list<move *>::iterator iter = b.mlist.begin();

		int localalpha = std::numeric_limits<int>::min();
		int localbeta = std::numeric_limits<int>::max();

		// Black = maximizing side
		if (b.getTurn() == 'b')
		{
			for (; iter != b.mlist.end(); ++iter)
			{
				std::time(&endTime);

				if (std::difftime(endTime, startTime) >= (timeLimit - 1))
				{
					timeUp = true;
					break;
				}

				b.makeMove(*iter);
				board newB(b);
				int value = alphabeta(newB, depth - 1, alpha, std::min(localbeta, beta));
				b.undoMove(*iter);
				b.changeTurn();

				if (value > alpha)
				{
					alpha = value;
					if (depth == maxdepth)
						tempBestM = *iter;
				}

				if (alpha >= beta && depth < maxdepth)
					return alpha;
			}

			if (!timeUp && depth == maxdepth)
				cdepth = depth;

			return alpha;
		}
		// Red = minimizing side
		else
		{
			for (; iter != b.mlist.end(); ++iter)
			{
				std::time(&endTime);

				if (std::difftime(endTime, startTime) >= (timeLimit - 1))
				{
					timeUp = true;
					break;
				}

				b.makeMove(*iter);
				board newB(b);
				int value = alphabeta(newB, depth - 1, std::max(localalpha, alpha), beta);
				b.undoMove(*iter);
				b.changeTurn();

				if (value < beta)
				{
					beta = value;
					if (depth == maxdepth)
						tempBestM = *iter;
				}

				if (alpha >= beta)
					return beta;
			}

			if (!timeUp && depth == maxdepth)
				cdepth = depth;

			return beta;
		}
	};

	std::time(&startTime);

	for (int depth = 1; depth != maxIterDepth; ++depth)
	{
		std::time(&startTimeD);

		maxdepth = depth;
		alphabeta(*this, depth,
				  std::numeric_limits<int>::min(),
				  std::numeric_limits<int>::max());

		std::time(&endTimeD);

		if (std::difftime(endTimeD, startTimeD) >= (timeLimit / 2))
		{
			std::time(&endTime);
			timeUp = true;
			break;
		}

		if (timeUp)
			break;
		else
			bestM = tempBestM;

		if (reachedEnd)
			break;
	}

	if (bestM == NULL && !mlist.empty())
		bestM = mlist.front();

	if (bestM == NULL)
		return false;

	fromRow = bestM->xi;
	fromCol = toExpandedCol(bestM->xi, bestM->yi);
	toRow = bestM->xf;
	toCol = toExpandedCol(bestM->xf, bestM->yf);

	return true;
}
