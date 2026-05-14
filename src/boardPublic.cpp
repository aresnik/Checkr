/*
 * boardPublic.cpp
 *
 *      Author: alex@alexanderresnik.com
 *
 * This file contains the public-facing board operations used by the
 * SDL/GUI version of the checkers game, along with the board evaluation
 * function and the AI move-selection wrapper.
 *
 * The board engine still stores only the 32 playable dark squares in an
 * 8 x 4 array. Several functions in this file translate between that compact
 * internal representation and the expanded 8 x 8 row/column coordinates used
 * by the graphical interface.
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

// Applies a fully validated move to the board.
//
// The move object uses the board's internal 8 x 4 coordinates. If the move is
// a jump or multi-jump, its jpoints list contains the captured pieces. Those
// captured pieces are removed first, then the moving checker is transferred
// from its starting square to its final square.
//
// After the move is applied, handleKinging() promotes the piece if it reached
// the opponent's back row, and changeTurn() advances the game to the next
// player.
void board::makeMove(move *m)
{
	if (!m->jpoints.empty())
	{
		list<jump *>::iterator it = m->jpoints.begin();
		for (; it != m->jpoints.end(); ++it)
			arr[(*it)->x][(*it)->y] = 'e';
	}

	// Save the moving piece, clear its starting square, and place it on the
	// final destination square.
	char c = arr[m->xi][m->yi];
	arr[m->xi][m->yi] = 'e';
	arr[m->xf][m->yf] = c;

	// Promote to king if appropriate, then give the turn to the other side.
	handleKinging(m->xf, m->yf);
	changeTurn();
}

// Reverses a move that was previously applied with makeMove().
//
// This is used heavily by the AI search. The search temporarily applies moves,
// evaluates the resulting board, and then restores the previous position before
// trying the next branch.
//
// For jump moves, each captured piece is restored from the saved jump data. The
// moving piece is then placed back on its original square using m->mP, which
// stores the piece as it existed before the move was made.
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

	// Clear the move's final square and restore the original piece to its
	// starting square.
	arr[m->xf][m->yf] = 'e';
	arr[m->xi][m->yi] = m->mP;
}

// Scores "double-corner" endgame positions.
//
// In checkers, the double corner can help a losing side survive longer because
// it limits how easily the opponent can approach. This helper gives the losing
// side a small defensive bonus for occupying one of those corner regions.
//
// The winning side receives small offsets for nearby diagonal pressure. This
// helps the evaluation prefer positions where the stronger side is positioned
// to force the weaker side out of the corner.
int board::cornerDiagonal(char losing, char winning)
{
	int c = 0;

	// Top-left double-corner region in compact board coordinates.
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

	// Bottom-right double-corner region in compact board coordinates.
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

// Evaluates the current board position for the AI.
//
// Positive scores favor black. Negative scores favor red.
//
// The score is built in weighted layers:
//
//   a1/a2: material value
//          - normal pieces are worth 2
//          - kings are worth 3
//
//   b:     advancement bonus for normal pieces
//          - black receives more value as it moves down the board
//          - red receives more value as it moves up the board
//
//   c:     raw piece-count difference
//
//   d:     endgame double-corner adjustment
//
//   e:     small pseudo-random tie breaker so the AI does not always choose
//          the same move when multiple positions evaluate equally
//
// The large multipliers make material dominate the smaller positional terms.
int board::evaluate()
{
	int a1 = 0, a2 = 0, b = 0, c = 0, d = 0;

	for (int i = 0; i != 8; ++i)
		for (int j = 0; j != 4; ++j)
		{
			if (arr[i][j] == 'b')
			{
				a1 += 2;

				// Reward black normal pieces for advancing toward king row.
				if (i == 0)
					b += 9;
				else
					b += i;

				c += 1;
			}
			else if (arr[i][j] == 'r')
			{
				a2 -= 2;

				// Reward red normal pieces for advancing toward king row.
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

	// In smaller endgames, apply the double-corner heuristic for the side that
	// is behind in material.
	if (c > 0 && a2 >= -8)
		d -= cornerDiagonal('r', 'b');
	else if (c < 0 && a1 <= 8)
		d += cornerDiagonal('b', 'r');

	// Weight the score components by importance.
	a1 *= 100000000;
	a2 *= 100000000;
	b *= 1000000;
	c *= 10000;
	d *= 100;

	// Add a tiny random term to break exact ties. Flip its sign on red's turn
	// so the random nudge remains consistent with the side to move.
	int e = rand() % 100;
	if (color == 'r')
		e = -e;

	return a1 + a2 + b + c + d + e;
}

// Initializes the board for a new game.
void board::startup()
{
	// reset the board
	reset();
}

// Adds one square to a move's GUI animation path.
//
// x and y are compact 8 x 4 board coordinates. The path stored in the move is
// converted to expanded 8 x 8 coordinates so the SDL layer can animate pieces
// without knowing about the board's compressed internal representation.
void board::addPathPoint(move *m, int x, int y)
{
	m->path8x8.push_back({x, toExpandedCol(x, y)});
}

// Returns all legal destination squares for a selected GUI square.
//
// The caller passes expanded 8 x 8 coordinates from the mouse/UI layer. This
// function regenerates the current move list through terminalTest(), then
// converts each move's compact start/end coordinates back into expanded 8 x 8
// coordinates for highlighting legal destination squares in the GUI.
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

		// Convert the compact starting column to an expanded 8 x 8 column.
		int start_col;
		if (m->xi % 2 == 0)
			start_col = 2 * m->yi + 1;
		else
			start_col = 2 * m->yi;

		if (m->xi == row && start_col == col)
		{
			// Convert the compact destination column to an expanded 8 x 8 column.
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

// Attempts to make a move selected through the GUI.
//
// The GUI works in expanded 8 x 8 board coordinates, while the engine stores
// only playable dark squares in an 8 x 4 array. This function validates the
// expanded coordinates, regenerates legal moves, finds the matching internal
// move object, and applies that move through makeMove().
bool board::tryMove8x8(int fromRow, int fromCol, int toRow, int toCol)
{
	// Basic bounds check for expanded 8 x 8 coordinates.
	if (fromRow < 0 || fromRow >= 8 || fromCol < 0 || fromCol >= 8 ||
		toRow < 0 || toRow >= 8 || toCol < 0 || toCol >= 8)
	{
		return false;
	}

	// Only dark squares are playable in checkers.
	if ((fromRow + fromCol) % 2 == 0 || (toRow + toCol) % 2 == 0)
	{
		return false;
	}

	// Regenerate legal moves for the current side to move. This is important
	// because jump availability can change after every move.
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

	// No legal move matched the requested start/end squares.
	return false;
}

// Finds the full animation path for a legal GUI move.
//
// For ordinary moves, the path usually contains the start and final square.
// For multi-jump moves, it can contain the intermediate landing squares as
// well. The SDL layer can use this returned path to animate the checker through
// each segment instead of jumping directly from the first square to the last.
std::vector<std::pair<int, int>> board::getMovePath8x8(int fromRow, int fromCol, int toRow, int toCol)
{
	std::vector<std::pair<int, int>> path;

	// Basic bounds check.
	if (fromRow < 0 || fromRow >= 8 || fromCol < 0 || fromCol >= 8 ||
		toRow < 0 || toRow >= 8 || toCol < 0 || toCol >= 8)
	{
		return path;
	}

	// Only dark squares are playable.
	if ((fromRow + fromCol) % 2 == 0 || (toRow + toCol) % 2 == 0)
	{
		return path;
	}

	// Regenerate legal moves for the current side to move. The move list is
	// the source of truth for whether the requested move is legal and what path
	// it should animate through.
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

// Searches for the AI's best move and returns it in expanded 8 x 8 coordinates.
//
// This function is designed for the GUI/controller layer. It hides the board's
// compact internal representation and gives the caller normal row/column pairs
// that can be animated and applied visually.
//
// The search uses iterative deepening. It starts at depth 1 and searches
// deeper until it reaches maxIterDepth, runs out of time, or reaches a terminal
// line. The best completed-depth move is retained so the AI can still return a
// legal move if time expires during a deeper search.
bool board::findBestMove8x8(int timeLimit, int &fromRow, int &fromCol, int &toRow, int &toCol)
{
	const int maxIterDepth = 20;

	// Use -1 values as a clear "no move found yet" state for the caller.
	fromRow = -1;
	fromCol = -1;
	toRow = -1;
	toCol = -1;

	if (timeLimit < 1)
		timeLimit = 1;

	// Generate legal moves for the current side to move.
	if (terminalTest())
		return false;

	// If there is only one legal move, there is no reason to run alpha-beta.
	if (mlist.size() == 1)
	{
		move *m = mlist.front();
		fromRow = m->xi;
		fromCol = toExpandedCol(m->xi, m->yi);
		toRow = m->xf;
		toCol = toExpandedCol(m->xf, m->yf);
		return true;
	}

	move *bestM = NULL;     // Best move from the last fully completed depth.
	move *tempBestM = NULL; // Best move found during the current depth.

	int maxdepth = 0;
	int cdepth = 0;
	bool timeUp = false;
	bool reachedEnd = false;

	std::time_t startTime = 0;
	std::time_t endTime = 0;
	std::time_t startTimeD = 0;
	std::time_t endTimeD = 0;

	// Local recursive alpha-beta function.
	//
	// It operates directly on board objects. Each branch temporarily applies a
	// move, copies the resulting board into newB, recursively searches newB,
	// then undoes the temporary move on the original board object so the next
	// branch starts from the same position.
	std::function<int(board &, int, int, int)> alphabeta =
		[&](board &b, int depth, int alpha, int beta) -> int
	{
		// If a terminal state is reached below the root, return an extreme score.
		// The side to move has no winning continuation, so the previous side has
		// effectively won.
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

		// Leaf node: score the current position.
		if (depth == 0)
			return b.evaluate();

		std::list<move *>::iterator iter = b.mlist.begin();

		int localalpha = std::numeric_limits<int>::min();
		int localbeta = std::numeric_limits<int>::max();

		// Black is the maximizing side.
		if (b.getTurn() == 'b')
		{
			for (; iter != b.mlist.end(); ++iter)
			{
				std::time(&endTime);

				// Stop searching before the allotted time expires completely.
				if (std::difftime(endTime, startTime) >= (timeLimit - 1))
				{
					timeUp = true;
					break;
				}

				// Try the move, search the resulting board, then restore b.
				b.makeMove(*iter);
				board newB(b);
				int value = alphabeta(newB, depth - 1, alpha, std::min(localbeta, beta));
				b.undoMove(*iter);
				b.changeTurn();

				if (value > alpha)
				{
					alpha = value;

					// Only the root depth records the actual move to return.
					if (depth == maxdepth)
						tempBestM = *iter;
				}

				// Alpha-beta cutoff.
				if (alpha >= beta && depth < maxdepth)
					return alpha;
			}

			if (!timeUp && depth == maxdepth)
				cdepth = depth;

			return alpha;
		}
		// Red is the minimizing side.
		else
		{
			for (; iter != b.mlist.end(); ++iter)
			{
				std::time(&endTime);

				// Stop searching before the allotted time expires completely.
				if (std::difftime(endTime, startTime) >= (timeLimit - 1))
				{
					timeUp = true;
					break;
				}

				// Try the move, search the resulting board, then restore b.
				b.makeMove(*iter);
				board newB(b);
				int value = alphabeta(newB, depth - 1, std::max(localalpha, alpha), beta);
				b.undoMove(*iter);
				b.changeTurn();

				if (value < beta)
				{
					beta = value;

					// Only the root depth records the actual move to return.
					if (depth == maxdepth)
						tempBestM = *iter;
				}

				// Alpha-beta cutoff.
				if (alpha >= beta)
					return beta;
			}

			if (!timeUp && depth == maxdepth)
				cdepth = depth;

			return beta;
		}
	};

	std::time(&startTime);

	// Iterative deepening loop. The AI keeps deepening the search while time
	// remains. bestM is only updated after a depth completes, which avoids
	// returning a partially searched move from an interrupted depth.
	for (int depth = 1; depth != maxIterDepth; ++depth)
	{
		std::time(&startTimeD);

		maxdepth = depth;
		alphabeta(*this, depth,
				  std::numeric_limits<int>::min(),
				  std::numeric_limits<int>::max());

		std::time(&endTimeD);

		// If the last completed depth consumed a large fraction of the allowed
		// time, do not risk starting another much deeper iteration.
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

	// Fallback: if the search was interrupted before recording a best move,
	// return the first legal move so the AI still plays something valid.
	if (bestM == NULL && !mlist.empty())
		bestM = mlist.front();

	if (bestM == NULL)
		return false;

	// Convert the selected internal move back into expanded 8 x 8 GUI
	// coordinates for the caller.
	fromRow = bestM->xi;
	fromCol = toExpandedCol(bestM->xi, bestM->yi);
	toRow = bestM->xf;
	toCol = toExpandedCol(bestM->xf, bestM->yf);

	return true;
}
