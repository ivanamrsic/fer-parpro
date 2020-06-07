from enum import Enum


class MessageType(Enum):
    ask_for_work = 0
    return_results = 1

    send_work = 2
    receive_work = 4

    new_work_created = 5

    @property
    def value(self):
        if self == MessageType.ask_for_work:
            return 0
        elif self == MessageType.return_results:
            return 1
        elif self == MessageType.send_work:
            return 2
        elif self == MessageType.receive_work:
            return 2
        elif self == MessageType.new_work_created:
            return 5

        return -1

    def description(self, rank, assignment=None):
        if self == MessageType.ask_for_work:
            return str(rank) + ". asking for work."
        elif self == MessageType.return_results:
            return str(rank) + ". returns results. result = " + str(assignment)
        elif self == MessageType.send_work:
            return str(rank) + ". sends assignment."
        elif self == MessageType.receive_work:
            return str(rank) + ". received assignment no:" + str(assignment.assignment_id) \
                if assignment is not None \
                else str(rank) + ". has no more assignments."
        elif self == MessageType.new_work_created:
            return str(rank) + ". new work created."

        return -1
