from constants import *


class Board:

    def __init__(self):
        self.board = Board._create_board()

    @staticmethod
    def _create_board():
        board = []
        for _ in range(ROW_COUNT):
            board_row = []
            for _ in range(COLUMN_COUNT):
                board_row.append(EMPTY_SPACE_MARK)
            board.append(board_row)
        return board

    def print_board(self, pre=None):
        for i in range(ROW_COUNT - 1, -1, -1):
            for j in range(COLUMN_COUNT):
                if j == 0 and pre is not None:
                    print("%s" % pre, " ", self.board[i][j], " ", end='', flush=True)
                    continue

                if pre is not None:
                    print(" ", self.board[i][j], " ", end='', flush=True)
                    continue

                print(" ", end='', flush=True)
                print(self.board[i][j], end='', flush=True)
                print(" ", end='', flush=True)
            print()
        print()

    def drop_coin(self, col, mark):
        row = self.next_free_row_for_column(col)
        self.board[row][col] = mark
        return row

    def next_free_row_for_column(self, col):
        for i in range(ROW_COUNT):
            if self.board[i][col] == EMPTY_SPACE_MARK:
                return i

        return -1

    def is_invalid_move(self, column):
        is_column_in_bounds = COLUMN_COUNT > column >= 0
        return self.board[ROW_COUNT - 1][column] != EMPTY_SPACE_MARK if is_column_in_bounds else True

    def is_winning_move(self, row, col, mark):

        vertical_win = self._check_vertical_win(row, col, mark)
        horizontal_win = self._check_horizontal_win(row, col, mark)
        diagonal_win = self._check_diagonal_win(row, col, mark)

        return vertical_win or horizontal_win or diagonal_win

    def _check_vertical_win(self, row, col, mark):

        counter = 1

        for i in range(row, -1, -1):

            if i == row:
                continue

            if self.board[i][col] == mark:
                counter += 1
            else:
                break

        return counter >= 4

    def _check_horizontal_win(self, row, col, mark):

        counter = 1

        for i in range(col, -1, -1):

            if i == col:
                continue

            if self.board[row][i] == mark:
                counter += 1
            else:
                break

        for i in range(col + 1, COLUMN_COUNT):
            if self.board[row][i] == mark:
                counter += 1
            else:
                break

        return counter >= 4

    def _check_main_diagonal_win(self, row, col, mark):

        counter = 1

        # up + left
        row_index = row
        col_index = col

        while col_index >= 0 and row_index >= 0:

            if col_index == col and row_index == row:
                col_index -= 1
                row_index -= 1
                continue

            if self.board[row_index][col_index] == mark:
                counter += 1
            else:
                break
            col_index -= 1
            row_index -= 1

        # down + right
        row_index = row + 1
        col_index = col + 1

        while col_index < COLUMN_COUNT and row_index < ROW_COUNT:

            if self.board[row_index][col_index] == mark:
                counter += 1
            else:
                break
            col_index += 1
            row_index += 1

        return counter >= 4

    def _check_supporting_diagonal_win(self, row, col, mark):

        counter = 0

        # up + right
        row_index = row
        col_index = col

        while col_index >= 0 and row_index < ROW_COUNT:

            if col_index == col and row_index == row:
                col_index -= 1
                row_index += 1
                continue

            if self.board[row_index][col_index] == mark:
                counter += 1
            else:
                break
            col_index -= 1
            row_index += 1

        # down + left
        row_index = row - 1
        col_index = col + 1

        while col_index < COLUMN_COUNT and row_index >= 0:

            if self.board[row_index][col_index] == mark:
                counter += 1
            else:
                break
            col_index += 1
            row_index -= 1

        return counter >= 4

    def _check_diagonal_win(self, row, col, mark):

        main_diagonal_win = self._check_main_diagonal_win(row, col, mark)
        support_diagonal_win = self._check_supporting_diagonal_win(row, col, mark)

        return main_diagonal_win or support_diagonal_win
