from enum import Enum
from constants import *


class PlayerType(Enum):
    human = 0
    computer = 1


class Player:

    def __init__(self, mark, turn_value, player_type, computer=None):
        self.mark = mark
        self.turn_value = turn_value
        self.player_type = player_type
        self.computer = computer

    @property
    def is_human_player(self):
        return self.player_type == PlayerType.human

    def next_turn(self, board):

        if self.is_human_player:
            return int(input())

        return self.computer.next_turn(board)

    def victory_message(self):
        return HUMAN_VICTORY_MESSAGE if self.is_human_player else COMPUTER_VICTORY_MESSAGE
