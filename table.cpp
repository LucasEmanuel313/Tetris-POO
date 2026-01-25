//
// Created by lucas on 21/11/2025.
//

#include "table.h"

int table::right_most_col_of_piece() const {
	int maxCol = 0;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			if (current_block->piece[i][j] == '#') {
				if (i > maxCol) maxCol = i;
			}
		}
	}
	return maxCol;
}

void table::clear_line(int line) {
	for (int j = line; j < (kRows - 1); ++j) {
		for (int i = 0; i < kCols; ++i) {
			positions[i][j] = positions[i][j + 1];
			fixedType[i][j] = fixedType[i][j + 1];
		}
	}
	for (int i = 0; i < kCols; ++i) {
		positions[i][kRows - 1] = ' ';
		fixedType[i][kRows - 1] = kEmptyType;
	}
}

int table::check_and_clear_lines() {
	int linesCleared = 0;

	for (int y = 0; y < kRows; ++y) {
		bool full = true;
		for (int x = 0; x < kCols; ++x) {
			if (fixedType[x][y] == kEmptyType) {
				full = false;
				break;
			}
		}
		if (full) {
			clear_line(y);
			++linesCleared;
			--y;
		}
	}
	return linesCleared;
}

void table::handle_landing() {
	// landing ocorreu (a peça não conseguiu descer mais)
	lastLandedEvent += 1;

	// A peça atual já está desenhada em positions[] (apenas parte visível) como '#'.
	// Agora marcamos os tipos nas células FIXAS.
	if (current_block) {
		int t = current_block->type();
		if (t < 0) t = 0;
		if (t > 6) t = 6;
		bool lockedAboveTop = false;
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				if (current_block->piece[i][j] == '#') {
					int x = block_x_pos + i;
					int y = block_y_pos + j;
					if (y >= kRows) {
						lockedAboveTop = true;
						continue;
					}
					if (x >= 0 && x < kCols && y >= 0 && y < kRows) {
						fixedType[x][y] = static_cast<unsigned char>(t);
					}
				}
			}
		}

		// Se a peça travou com qualquer bloco acima do topo visível, é GAME OVER.
		if (lockedAboveTop) {
			gameOver = true;
			return;
		}
	}

	int lines = check_and_clear_lines();
	if (lines < 0) lines = 0;
	if (lines > 4) lines = 4; // por jogada, o máximo em Tetris é 4
	lastClearedEvent += lines;

	switch (lines) {
		case 1: score += 40; break;
		case 2: score += 100; break;
		case 3: score += 300; break;
		case 4: score += 1200; break;
		default: break;
	}

	// Nova peça
	holdUsedThisTurn = false;
	delete current_block;
	current_block = nullptr;
	needsSpawn = true;
}

bool table::collides_here() const {
	if (!current_block) return false;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			if (current_block->piece[i][j] == '#') {
				int x = block_x_pos + i;
				int y = block_y_pos + j;

				if (x < 0 || x >= kCols) return true;
				if (y < 0) return true;
				if (y > kMaxActiveY) return true;

				// acima do topo visível, consideramos vazio (spawn buffer)
				if (y < kRows) {
					if (fixedType[x][y] != kEmptyType) return true;
				}
			}
		}
	}
	return false;
}

bool table::top_reached() const {
	for (int x = 0; x < kCols; ++x) {
		if (fixedType[x][kRows - 1] != kEmptyType) return true;
	}
	return false;
}

void table::update_table_no_overwrite() {
	if (!current_block) return;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			if (current_block->piece[i][j] == '#') {
				int x = block_x_pos + i;
				int y = block_y_pos + j;
				if (x >= 0 && x < kCols && y >= 0 && y < kRows) {
					if (positions[x][y] == ' ') positions[x][y] = '#';
				}
			}
		}
	}
}

void table::refill_bag_if_needed() {
	if (!bag.empty()) return;
	bag.clear();
	bag.reserve(7);
	for (int i = 0; i < 7; ++i) bag.push_back(i);
	std::shuffle(bag.begin(), bag.end(), rng);
}

int table::draw_from_bag() {
	refill_bag_if_needed();
	int t = bag.back();
	bag.pop_back();
	return t;
}

void table::ensure_next_queue() {
	while ((int)nextQueue.size() < kNextPreviewCount) {
		nextQueue.push_back(draw_from_bag());
	}
}

int table::pop_next_type() {
	ensure_next_queue();
	int t = nextQueue.front();
	nextQueue.pop_front();
	ensure_next_queue();
	return t;
}

std::string table::mini_line_for_type(int type, int miniRowFromTop) {
	// miniRowFromTop: 0..3 (0 = topo)
	if (type < 0 || type > 6) return "        ";
	const char (*p)[4][4] = TETROMINOES[type];
	std::string out;
	out.reserve(8);
	int y = 3 - miniRowFromTop; // converte para o sistema usado no piece (y cresce pra cima)
	for (int x = 0; x < 4; ++x) {
		out += ((*p)[x][y] == '#') ? "[]" : "  ";
	}
	return out;
}

std::string table::side_panel_line_single(int rowIndexFromTop) const {
	// rowIndexFromTop: 0..21 (0 = topo da tela)
	// Layout (22 linhas): HOLD(1+4) + blank(1) + NEXT(1 + 3*4 + 2 blanks) = 22
	if (rowIndexFromTop == 0) return "HOLD";
	if (rowIndexFromTop >= 1 && rowIndexFromTop <= 4) {
		if (holdType < 0) return (rowIndexFromTop == 1) ? "(none)" : "";
		return mini_line_for_type(holdType, rowIndexFromTop - 1);
	}
	if (rowIndexFromTop == 5) return "";
	if (rowIndexFromTop == 6) return "NEXT";

	int base = 7;
	for (int n = 0; n < kNextPreviewCount; ++n) {
		int start = base + n * 5; // 4 linhas + 1 blank entre peças
		int end = start + 3;
		if (rowIndexFromTop >= start && rowIndexFromTop <= end) {
			int t = (n < (int)nextQueue.size()) ? nextQueue[n] : -1;
			return mini_line_for_type(t, rowIndexFromTop - start);
		}
		if (rowIndexFromTop == start + 4) return "";
	}
	return "";
}

table::table() {
	for (int i = 0; i < kCols; ++i) {
		for (int j = 0; j < kRows; ++j) {
			positions[i][j] = ' ';
			fixedType[i][j] = kEmptyType;
		}
	}
	score = 0;
	lastClearedEvent = 0;
	lastLandedEvent = 0;

	std::random_device rd;
	rng.seed(rd());
	bag.clear();
	nextQueue.clear();
	ensure_next_queue();
}

table::~table() {
	delete current_block;
}

void table::print_table() {
	for (int i = kRows - 1; i >= 0; --i) {
		for (int j = 0; j < kCols; ++j) {
			std::cout << '|' << positions[j][i] << '|';
		}
		int rowIndexFromTop = (kRows - 1) - i;
		std::string panel = side_panel_line_single(rowIndexFromTop);
		if (!panel.empty()) {
			std::cout << "   " << panel;
		}
		std::cout << '\n';
	}
	std::cout << "-------------------------------\n";
	std::cout << "Score: " << score << "   C=HOLD";
	std::cout << std::string(30, ' ') << "\n";
}

int table::get_score() const { return score; }

int table::get_fixed_type(int x, int y) const {
	if (x < 0 || x >= kCols || y < 0 || y >= kRows) return -1;
	unsigned char t = fixedType[x][y];
	return (t == kEmptyType) ? -1 : (int)t;
}

const block* table::get_current_block() const { return current_block; }

int table::get_block_x_pos() const { return block_x_pos; }

int table::get_block_y_pos() const { return block_y_pos; }

char table::get_fixed_cell(int x, int y) const {
	if (x < 0 || x >= kCols || y < 0 || y >= kRows) return ' ';
	return (fixedType[x][y] == kEmptyType) ? ' ' : '#';
}

int table::get_ghost_y() const {
	if (!current_block) return block_y_pos;

	auto occupied_fixed = [&](int x, int y) -> bool {
		if (x < 0 || x >= kCols) return true;
		if (y < 0) return true;
		if (y >= kRows) return false; // acima do topo visível não tem fixos
		return fixedType[x][y] != kEmptyType;
	};

	int ghostY = block_y_pos;
	while (true) {
		// tenta descer 1
		bool can = true;
		for (int i = 0; i < 4 && can; ++i) {
			for (int j = 0; j < 4 && can; ++j) {
				if (current_block->piece[i][j] == '#') {
					int x = block_x_pos + i;
					int yNext = (ghostY - 1) + j;
					if (yNext < 0) {
						can = false;
						break;
					}
					if (occupied_fixed(x, yNext)) {
						can = false;
						break;
					}
				}
			}
		}

		if (!can) break;
		ghostY -= 1;
	}

	return ghostY;
}

int table::get_hold_type() const { return holdType; }

std::vector<int> table::get_next_types(int count) const {
	if (count < 0) count = 0;
	if (count > kNextPreviewCount) count = kNextPreviewCount;
	std::vector<int> out;
	out.reserve((size_t)count);
	for (int i = 0; i < count && i < (int)nextQueue.size(); ++i) out.push_back(nextQueue[i]);
	return out;
}

std::string table::serialize_board() const {
	int t;
	char c;
	std::string out;
	out.reserve(kCols * kRows);
	for (int y = 0; y < kRows; ++y) {
		for (int x = 0; x < kCols; ++x) {
			t = get_fixed_type(x, y);
			c = (t < 0) ? '.' : static_cast<char>('0' + t);
			std::cout << "Character sent: " << c << std::endl;
			out.push_back(positions[x][y] == ' ' ? '.' : c);
		}
	}
	return out;
}

void table::update_table() {
	if (!current_block) return;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			if (current_block->piece[i][j] == '#') {
				int x = block_x_pos + i;
				int y = block_y_pos + j;
				if (x >= 0 && x < kCols && y >= 0 && y < kRows) {
					positions[x][y] = '#';
				}
			}
		}
	}
}

void table::block_clear() {
	if (!current_block) return;
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			if (current_block->piece[i][j] == '#') {
				int x = block_x_pos + i;
				int y = block_y_pos + j;
				if (x >= 0 && x < kCols && y >= 0 && y < kRows) {
					positions[x][y] = ' ';
				}
			}
		}
	}
}

bool table::can_move(int dx, int dy) {
	if (!current_block) return false;

	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			if (current_block->piece[i][j] == '#') {
				int newX = block_x_pos + dx + i;
				int newY = block_y_pos + dy + j;

				if (newX < 0 || newX >= kCols) return false;
				if (newY < 0) return false;
				if (newY > kMaxActiveY) return false;

				// acima do topo visível: sem colisão com fixos
				if (newY < kRows) {
					if (fixedType[newX][newY] != kEmptyType) return false;
				}
			}
		}
	}

	return true;
}

void table::add_block() {
	int type = pop_next_type();
	current_block = new block(type);
	block_x_pos = kSpawnX;
	block_y_pos = kSpawnY;
	needsSpawn = false;

	// se já nasce colidindo com o topo/stack, game over imediato
	if (collides_here()) {
		gameOver = true;
		// ainda desenha a peça que tentou nascer (pra não "sumir")
		update_table_no_overwrite();
		return;
	}
	update_table();
}

void table::spawn_if_needed() {
	if (gameOver) return;
	if (!needsSpawn) return;
	if (current_block != nullptr) return;
	add_block();
}

void table::hold_block() {
	if (!current_block || gameOver) return;
	if (holdUsedThisTurn) return;

	int curType = current_block->type();

	block_clear();
	delete current_block;
	current_block = nullptr;

	if (holdType < 0) {
		holdType = curType;
		add_block();
	} else {
		int swapType = holdType;
		holdType = curType;
		current_block = new block(swapType);
		block_x_pos = kSpawnX;
		block_y_pos = kSpawnY;

		if (collides_here()) {
			gameOver = true;
			update_table_no_overwrite();
		} else {
			update_table();
		}
	}

	holdUsedThisTurn = true;
}

void table::rotate_block() {
	if (!current_block || gameOver) return;

	// remove a peça atual do grid
	block_clear();

	// backup da peça e posição
	char backup[4][4];
	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j)
			backup[i][j] = current_block->piece[i][j];

	int oldX = block_x_pos;
	int oldY = block_y_pos;

	// rotaciona
	current_block->rotate();

	// tenta "kicks" (0, empurra pra esquerda, depois direita)
	const int kicks[] = { 0, -1, -2, -3, 1, 2, 3 };
	bool ok = false;

	for (int k = 0; k < (int)(sizeof(kicks)/sizeof(kicks[0])); ++k) {
		block_x_pos = oldX + kicks[k];
		block_y_pos = oldY;

		if (!collides_here()) { // colisão/borda contra FIXOS (fixedType)
			ok = true;
			break;
		}
	}

	if (!ok) {
		// desfaz rotação + posição
		block_x_pos = oldX;
		block_y_pos = oldY;
		for (int i = 0; i < 4; ++i)
			for (int j = 0; j < 4; ++j)
				current_block->piece[i][j] = backup[i][j];
	}

	// redesenha a peça (válida ou revertida)
	update_table();
}

void table::block_descend() {
	if (!current_block || gameOver) return;
	if (can_move(0, -1)) {
		block_clear();
		block_y_pos -= 1;
		update_table();
	} else {
		handle_landing();
	}
}

void table::block_left() {
	if (!current_block || gameOver) return;
	if (can_move(-1, 0)) {
		block_clear();
		block_x_pos -= 1;
		update_table();
	}
}

void table::block_right() {
	if (!current_block || gameOver) return;
	int rightCol = right_most_col_of_piece();
	int globalRight = block_x_pos + rightCol;
	if (globalRight >= (kCols - 1)) return;

	if (can_move(1, 0)) {
		block_clear();
		block_x_pos += 1;
		update_table();
	}
}

void table::block_drop() {
	if (!current_block || gameOver) return;
	while (can_move(0, -1)) {
		block_clear();
		block_y_pos -= 1;
		update_table();
	}
	handle_landing();
}

bool table::is_game_over() const { return gameOver; }

void table::apply_garbage(int n) {
	if (n <= 0) return;

	bool hadActive = (current_block != nullptr);
	if (hadActive) block_clear();

	for (int k = 0; k < n; ++k) {
		// Se já existe algo no topo, subir mais significa estourar o teto.
		if (top_reached()) {
			gameOver = true;
			return;
		}

		std::uniform_int_distribution<int> dist(0, kCols - 1);
		int hole = dist(rng);

		// shift up: y = 21 <- 20 <- ... <- 0
		for (int y = (kRows - 1); y > 0; --y) {
			for (int x = 0; x < kCols; ++x) {
				positions[x][y] = positions[x][y - 1];
				fixedType[x][y] = fixedType[x][y - 1];
			}
		}

		// linha lixo em y=0
		for (int x = 0; x < kCols; ++x) {
			positions[x][0] = (x == hole) ? ' ' : '#';
			fixedType[x][0] = (x == hole) ? kEmptyType : kGarbageType;
		}
	}

	if (hadActive) {
		// checa colisão antes de redesenhar a peça
		// (se redesenhar primeiro, vai colidir com ela mesma)
		bool overlap = collides_here();
		if (overlap) {
			gameOver = true;
			update_table_no_overwrite();
		} else {
			update_table();
		}
	}

	// Nota: não é game over só por ocupar a linha do topo.
	// Game over aqui acontece quando o lixo tenta empurrar blocos para fora (checado acima).
}

int table::pop_cleared_lines_event() {
	int v = lastClearedEvent;
	lastClearedEvent = 0;
	return v;
}

int table::pop_landed_event() {
	int v = lastLandedEvent;
	lastLandedEvent = 0;
	return v;
}

void table::reset(bool spawn) {
	delete current_block;
	current_block = nullptr;

	for (int i = 0; i < kCols; ++i) {
		for (int j = 0; j < kRows; ++j) {
			positions[i][j] = ' ';
			fixedType[i][j] = kEmptyType;
		}
	}

	block_x_pos = 0;
	block_y_pos = 0;
	score = 0;
	lastClearedEvent = 0;
	lastLandedEvent = 0;
	gameOver = false;

	holdType = -1;
	holdUsedThisTurn = false;
	needsSpawn = false;
	bag.clear();
	nextQueue.clear();
	ensure_next_queue();

	if (spawn) add_block();
}