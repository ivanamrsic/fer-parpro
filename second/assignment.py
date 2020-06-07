class Assignment:

    def __init__(self, assignment_id, board, row, col, player_mark, opposite_player_mark, result=0, is_winning_move=False):
        self.assignment_id = assignment_id
        self.board = board
        self.row = row
        self.col = col
        self.player_mark = player_mark
        self.opposite_player_mark = opposite_player_mark
        self.result = result
        self.is_winning_move = is_winning_move
        self.checked_children_result = is_winning_move
