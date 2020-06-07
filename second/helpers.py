from anytree import Node, RenderTree
from copy import deepcopy
from assignment import Assignment
from constants import *

counter = 0


class Helper:

    @staticmethod
    def next_node_name_from(assignment_id):

        if assignment_id is not None:
            return assignment_id

        global counter
        counter += 1
        return counter

    @staticmethod
    def print_tree(root):
        for pre, fill, node in RenderTree(root):
            if node.assignment is not None and node.assignment.is_winning_move:
                print("%s%s result:%s player:%s opposite_player:%s is_winning_move:%s" % (
                pre, node.assignment.assignment_id, node.assignment.result, node.assignment.player_mark,
                node.assignment.opposite_player_mark, node.assignment.is_winning_move), flush=True)
                node.assignment.board.print_board(pre)
                continue
            elif node.assignment is not None:
                print("%s%s result: %s" % (
                pre, node.assignment.assignment_id, node.assignment.result), flush=True)
                continue

            print("%s%s no board" % (pre, node.name), flush=True)

    @staticmethod
    def generate_tree_level(root, board=None):

        for root_tmp in root.leaves:

            # generate first level assignments
            if root.assignment is None and root_tmp == root:
                Helper.generate_column_drops(root, COMPUTER_PLAYER_MARK, HUMAN_PLAYER_MARK, board)
                break

            # generate deeper level assignments
            Helper.generate_column_drops(root_tmp, root_tmp.assignment.opposite_player_mark,
                                         root_tmp.assignment.player_mark, root_tmp.assignment.board)

    @staticmethod
    def generate_column_drops(root, player_mark, opposite_player_mark, board):

        temp_board = deepcopy(board)

        if root.assignment is not None:
            if root.assignment.is_winning_move:
                return

        for column in range(COLUMN_COUNT):

            # column has already been filled
            if temp_board.is_invalid_move(column):
                continue

            board = deepcopy(temp_board)

            row = board.next_free_row_for_column(column)

            board.drop_coin(column, player_mark)

            assignment_id = Helper.next_node_name_from(None)

            assignment = \
                Assignment(
                    assignment_id=Helper.next_node_name_from(assignment_id),
                    board=board,
                    row=row,
                    col=column,
                    player_mark=player_mark,
                    opposite_player_mark=opposite_player_mark,
                    is_winning_move=board.is_winning_move(row, column, player_mark)
                )

            node = Node(assignment_id, root, assignment=assignment)
