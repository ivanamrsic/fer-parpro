from board import Board
from constants import *


class Game:

    def __init__(self, player1, player2):
        self.board = Board()
        self.current_turn = 0
        self.total_move_count = 0
        self.human_player = player1
        self.computer_player = player2

    @property
    def current_player(self):
        return self.human_player if self.current_turn == 0 else self.computer_player

    @property
    def is_board_filled(self):
        return self.total_move_count == ROW_COUNT * COLUMN_COUNT

    def _toggle_turn(self):
        self.current_turn += 1
        self.current_turn %= 2
        self.total_move_count += 1

    def _check_state(self, col):
        row = self.board.drop_coin(col, self.current_player.mark)
        return self.board.is_winning_move(row, col, self.current_player.mark)

    def start(self):

        while True:

            column = self.current_player.next_turn(self.board)

            while self.board.is_invalid_move(column):
                if self.current_player is self.human_player:
                    column = int(input("Potez nije validan, probaj ponovo: "))
                else:
                    self.current_player.next_turn(self.board)

            state = self._check_state(column)

            if self.current_player.mark == COMPUTER_PLAYER_MARK:
                self.board.print_board()

            if state or self.is_board_filled:
                break

            self._toggle_turn()
