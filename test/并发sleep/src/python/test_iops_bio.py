# coding:utf-8
import time
from concurrent.futures import ThreadPoolExecutor

tasks_count = 10000
threads_pool = 2000
sleep_milisecs = 10

tasks = list(range(tasks_count))


def just_sleep(*args, **kwargs):
    time.sleep(sleep_milisecs / 1000)
    pass


def main(tasks_count=10000, threads_pool=2000, sleep_milisecs=10):
    print("Task num: {tasks_count} sleep period： {sleep_milisecs}(ms) pool_size: {threads_pool}".format(**locals()))
    with ThreadPoolExecutor(max_workers=threads_pool) as executor:
        start = time.time()
        executor.map(just_sleep, tasks)
        executor.shutdown()

        # end
        cost = time.time() - start
        iops = (float(tasks_count) / cost) if cost > 0 else 0
        print("iops: {iops}  time used: {cost_ms}(ms) ".format(iops=round(iops,2), cost_ms=round(cost * 1000, 2)))
    pass


if __name__ == '__main__':
    main(
        tasks_count=10000,
        threads_pool=2000,
        sleep_milisecs=10
    )
    pass
