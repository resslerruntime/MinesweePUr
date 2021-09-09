#include <SDL.h>
#include <random>
#include <vector>
#include <array>
#include <utility>
#include "Game.hpp"
#include "Constants.hpp"
#include "Cell.hpp"

std::random_device rds;
std::mt19937_64 rd(rds());

Game::Game() {
	initBoard();
	generateBoard();
	lost_ = false;
}

void Game::initBoard() {
	//premature optimisation is the root of all fun
	//code below heavily assumes an 8x8 board
	cells_.resize(8);
	for (int i = 0; i < 8; ++i) {
		cells_[i].reserve(8);
	}

	for (int i = 0; i < 8; ++i) {
		for (int j = 0; j < 8; ++j) {
			cells_[i].emplace_back(Cell({CENTER_X + 64.0f * CELL_SCALE * j, CENTER_Y + 64.0f * CELL_SCALE * i}, {0, 0, 64, 64}, {0, 0, 64, 64}, bg, fg));
		}
	}
}

void Game::generateBoard() {
	//mines
	for (int i = 0; i < NUM_MINES; ++i) {
		for (;;) {
			int row = rd() % 8;
			int col = rd() % 8;
			if (cells_[row][col].getValue() == fg_value::MINE) {
				continue;
			}
			cells_[row][col].setCellValue(fg_value::MINE);
			printf("%d, %d\n", row, col);
			break;
		}
	}

	//numbers
	for (int row = 0; row < 8; ++row) {
		for (int col = 0; col < 8; ++col) {
			fg_value neighbour_mines = determineCellValue(row, col);
			cells_[row][col].setCellValue(neighbour_mines);
		}
	}
}

bool Game::cellRefPossible(int row, int col) {
	if (col < 0 || col > 7 || row < 0 || row > 7) {
		return false;
	} else {
		return true;
	}
}

fg_value Game::determineCellValue(int row, int col) {
	if (cells_[row][col].getValue() == fg_value::MINE) {
		return fg_value::MINE;
	}
	int neighbour_mines = checkNeighbours(row, col, fg_value::MINE);
	if (neighbour_mines == 0) {
		return fg_value::NONE;
	}
	//we subtract one from neighbour_mines because the enum doesn't cast perfectly
	return static_cast<fg_value>(neighbour_mines - 1);
}

int Game::checkNeighbours(int row, int col, fg_value val) {
	int neighbours = 0;
	//left
	if (cellRefPossible(row, col - 1)) {
		if (cells_[row][col - 1].getValue() == val) {
			++neighbours;
		}
	}
	//right
	if (cellRefPossible(row, col + 1)) {
		if (cells_[row][col + 1].getValue() == val) {
			++neighbours;
		}
	}
	//up
	if (cellRefPossible(row - 1, col)) {
		for (int i = -1; i < 2; ++i) {
			if (!cellRefPossible(row, col + i)) {
				continue;
			}
			if (cells_[row - 1][col + i].getValue() == val) {
				++neighbours;
			}
		}
	}
	//down
	if (cellRefPossible(row + 1, col)) {
		for (int i = -1; i < 2; ++i) {
			if (!cellRefPossible(row, col + i)) {
				continue;
			}
			if (cells_[row + 1][col + i].getValue() == val) {
				++neighbours;
			}
		}
	}
	return neighbours;
}

void Game::revealNeighbours(int row, int col) {
						printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	std::vector<std::pair<int, int>> check_queue;
	check_queue.push_back(std::pair<int, int>(row, col));
	while (!check_queue.empty()) {
		int check_row = check_queue.back().first;
		int check_col = check_queue.back().second;
		if (cells_[check_row][check_col].getValue() == fg_value::MINE) {
			check_queue.pop_back();
			continue;
		}
		bool popped_currently_checking = false;
		for (int i = -1; i < 2; ++i) {
			for (int j = -1; j < 2; ++j) {
				//continue if we would go out of bounds
				if ((i == 0 && j == 0) || !cellRefPossible(check_row + i, check_col + j)) {
					continue;
				}
				//only queue cells that aren't mines and aren't already clicked
				if (cells_[check_row + i][check_col + j].getValue() != fg_value::MINE && !cells_[check_row + i][check_col + j].fgShown()) {
					if (!popped_currently_checking) {
						check_queue.pop_back();
						printf("Popping %d, %d\n", check_row, check_col);
						popped_currently_checking = true;
					}
					//don't queue tiles with numbers in them
					if (cells_[check_row + i][check_col + j].getValue() == fg_value::NONE) {
						check_queue.push_back(std::pair<int, int>(check_row + i, check_col + j));
					}
					cells_[check_row + i][check_col + j].leftClick();
					printf("Revealing %d, %d\n", check_row + i, check_col + j);
					continue;
				}
			}
		}
		if (!popped_currently_checking) {
		check_queue.pop_back();
		}
	}
}

void Game::checkLose(int row, int col) {
	if (cells_[row][col].getValue() == fg_value::MINE) {
		lose();
	}
}

void Game::lose() {
	printf("you LOSEEEEEE :((");
	lost_ = true;
}

void Game::checkWin() {
	int remaining = 0;
	for (int i = 0; i < 8; ++i) {
		for (int j = 0; j < 8; ++j) {
			if (!cells_[i][j].clicked()) {
				if (++remaining > NUM_MINES) {
					goto the_end;
				}
			}
		}
	}
	the_end:
	if (remaining == NUM_MINES && !lost_) {
		win();
	}
}

void Game::win() {
	printf("YOU WINNNNNNNNNNNNNNNNNNNNNN");
}

void Game::checkCellClick(Sint32 x, Sint32 y, bool right_mouse) {
	for (int row = 0; row < 8; ++row){
		for (int col = 0; col < 8; ++col){
			if (cells_[row][col].getPos().x < x && cells_[row][col].getPos().x + 64.0f * CELL_SCALE > x) {
				if (cells_[row][col].getPos().y < y && cells_[row][col].getPos().y + 64.0f * CELL_SCALE > y) {
					if (right_mouse) {
						cells_[row][col].rightClick();
					} else if (!cells_[row][col].fgShown()) {
						cells_[row][col].leftClick();
						revealNeighbours(row, col);
						checkLose(row, col);
						checkWin();
					}
				}
			}
		}
	}
}

Cell& Game::cell(int i, int j) {
	return cells_[i][j];
}