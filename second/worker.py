from mpi4py import MPI
import time
from functools import reduce
from anytree import Node, PostOrderIter
from constants import *
from message import MessageType
from helpers import Helper


class Worker:

    def __init__(self, rank, size, comm=MPI.COMM_WORLD):
        self.rank = rank
        self.size = size
        self.comm = comm
        self.assignment = None
        self.root = None
        self.is_currently_processing = False
        self.no_more_work = False

    @property
    def is_free(self):
        return self.assignment is None

    @property
    def start_board(self):
        return self.assignment.board if self.assignment is not None else None

    def do_the_work(self):

        while True:

            if self.is_free:
                self.ask_for_work()
                self.wait_for_assignment()

            if self.no_more_work:
                self.wait_for_next_computer_move()

            if not self.is_currently_processing and self.assignment is not None:
                self.is_currently_processing = True
                self.process_assignment()
                self.report_result_to_master()

    def wait_for_next_computer_move(self):
        while not self.comm.Iprobe(source=MASTER_RANK):
            time.sleep(1)
        work_created = self.comm.recv(source=MASTER_RANK)
        self.no_more_work = work_created == "NEW WORK CREATED"

    def ask_for_work(self):
        message = MessageType.ask_for_work
        self.comm.send(None, dest=MASTER_RANK, tag=message.value)

    def wait_for_assignment(self):
        message = MessageType.receive_work
        self.assignment = self.comm.recv(source=MASTER_RANK, tag=message.value)
        if self.assignment is None:
            self.no_more_work = True

    def report_result_to_master(self):
        message = MessageType.return_results
        result = (self.assignment.assignment_id, self.root.assignment.result)
        self.comm.send(result, dest=MASTER_RANK, tag=message.value)
        self.assignment = None
        self.is_currently_processing = False

    def process_assignment(self):
        self._generate_decision_tree()
        self._make_decision()

    def _generate_decision_tree(self):

        self.root = Node(ROOT_NODE_NAME, assignment=self.assignment)

        if self.start_board is None:
            return

        Helper.generate_tree_level(self.root)
        Helper.generate_tree_level(self.root)
        Helper.generate_tree_level(self.root)
        Helper.generate_tree_level(self.root)

    def _make_decision(self):

        for node in PostOrderIter(self.root):

            assignment = node.assignment

            node_result = 0

            # node is a leaf
            if len(node.children) == 0:
                if assignment.is_winning_move:
                    node_result = 1 if assignment.player_mark == COMPUTER_PLAYER_MARK else -1
            # node is not a leaf
            else:
                children_results = []
                for child in node.children:
                    children_results.append(child.assignment.result)
                node_result = reduce(lambda a, b: a + b, children_results) / len(node.children)

            assignment.result = node_result
