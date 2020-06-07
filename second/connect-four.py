from mpi4py import MPI
from master import Master
from worker import Worker


def main():

    comm = MPI.COMM_WORLD
    rank = comm.Get_rank()
    size = comm.Get_size()

    create_str = "Kreiran procesor " + str(rank) + " ukupno: " + str(size)
    print(create_str, flush=True)

    comm.barrier()

    if rank == 0:
        processor = Master(rank, size, comm)
        game = processor.create_game()
        Master.start_working(game)

    elif rank is not 0:
        processor = Worker(rank, size, comm)
        processor.do_the_work()


if __name__ == "__main__":
    main()