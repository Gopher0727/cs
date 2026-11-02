# processes & threads
import multiprocessing as mp
import threading

box = []


def add():
    box.append("hello")


print(mp.get_start_method())


if __name__ == "__main__":
    t = threading.Thread(target=add)
    t.start()
    t.join()
    print(box)
    box.clear()

    p = mp.Process(target=add)
    p.start()
    p.join()
    print(box)
    box.clear()
