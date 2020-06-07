from mpi4py import MPI
from anytree import Node, PostOrderIter
from functools import reduce
import time
from constants import *
from player import Player, PlayerType
from game import Game
from helpers import Helper
from message import MessageType


class Master:

    def __init__(self, rank, size, comm=MPI.COMM_WORLD):
        self.rank = rank
        self.size = size
        self.comm = comm
        self.human_player = Player(HUMAN_PLAYER_MARK, HUMAN_PLAYER_TURN, PlayerType.human)
        self.computer_player = Player(COMPUTER_PLAYER_MARK, COMPUTER_PLAYER_TURN, PlayerType.computer, computer=self)
        self.assignments_sent = 0
        self.assignments_completed = 0
        self.root = None

    @property
    def assignments(self):
        return self.root.leaves if self.root is not None else None

    def create_game(self):
        return Game(self.human_player, self.computer_player)

    @staticmethod
    def start_working(game):
        game.start()

    def next_turn(self, board):

        # tic = time.perf_counter() # Start time tracking

        self._create_assignments(board)

        self._announce_new_work()

        while self.assignments_completed != len(self.root.leaves):
            self._receive_request()

        decision = self._make_decision()

        # toc = time.perf_counter() # End time tracking
        # print("workers: ", self.size - 1, " total time: ", toc-tic, flush=True)

        return decision

    def _announce_new_work(self):
        self.comm.bcast("NEW WORK CREATED", root=MASTER_RANK)

    def _receive_request(self):
        status = MPI.Status()
        assignment = self.comm.recv(source=MPI.ANY_SOURCE, tag=MPI.ANY_TAG, status=status)
        self._handle_request_with_status(status, assignment)

    def _handle_request_with_status(self, status, assignment=None):

        if status.tag == MessageType.ask_for_work.value:
            self._send_assignment(status.source)
        elif status.tag == MessageType.return_results.value:
            self._receive_assignment_result(assignment)

    def _send_assignment(self, destination):

        message = MessageType.send_work

        # All assignments have been sent
        if self.assignments_sent == len(self.assignments):
            self.comm.send(None, dest=destination, tag=message.value)
            return

        assignment = self.assignments[self.assignments_sent].assignment
        self.assignments_sent += 1
        self.comm.send(assignment, dest=destination, tag=message.value)

    def _receive_assignment_result(self, assignment):

        if assignment is None:
            return

        for node in self.root.leaves:

            if node.assignment is None:
                continue

            if node.assignment.assignment_id == assignment[0]:
                node.assignment.result = assignment[1]
                break

        self.assignments_completed += 1

    def _create_assignments(self, board):

        self.assignments_completed = 0
        self.assignments_sent = 0

        self.root = Node(ROOT_NODE_NAME, assignment=None)

        Helper.generate_tree_level(self.root, board)
        Helper.generate_tree_level(self.root)
        Helper.generate_tree_level(self.root)

    def _make_decision(self):

        for node in PostOrderIter(self.root):

            if node.parent is None:
                continue

            assignment = node.assignment

            node_result = 0

            # node is a leaf
            if len(node.children) == 0 and node.assignment.result != 0.0:
                if assignment.is_winning_move:
                    node_result = 1 if assignment.player_mark == COMPUTER_PLAYER_MARK else -1
            # node is not a leaf
            elif len(node.children) > 0:
                children_results = []
                # Helper.print_tree(self.root)
                for child in node.children:
                    children_results.append(child.assignment.result)

                node_result = reduce(lambda a, b: a + b, children_results) / len(node.children)

            if node_result != 0.0:
                assignment.result = node_result

        return self.print_children_results_and_get_max()

    def print_children_results_and_get_max(self):

        max_result = -2
        max_child_col = -1

        for child in self.root.children:
            print("%.3f" % round(child.assignment.result, 3), end=" ", flush=True)
            if child.assignment.result > max_result:
                max_result = child.assignment.result
                max_child_col = child.assignment.col

        print()

        return max_child_col
