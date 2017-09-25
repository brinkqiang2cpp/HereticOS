# coding:utf-8
import time
import asyncio

# loop = 1


async def just_sleep(sleep_milisecs):
    return await asyncio.sleep(sleep_milisecs / 1000)


async def main(tasks_count, sleep_milisecs):
    threads_pool = 1
    print("Task num: {tasks_count} sleep period： {sleep_milisecs}(ms) pool_size: {threads_pool}".format(**locals()))
    tasks = list(range(tasks_count))
    start = time.time()
    for i in tasks:
        # print(i)
        rs = await just_sleep(sleep_milisecs)

    # end
    cost = time.time() - start
    iops = (float(tasks_count) / cost) if cost > 0 else 0
    print("iops: {iops}  time used: {cost_ms}(ms) ".format(iops=round(iops,2), cost_ms=round(cost * 1000, 2)))
    pass


if __name__ == '__main__':
    loop = asyncio.get_event_loop()
    loop.run_until_complete(main(
        tasks_count=10000,
        sleep_milisecs=10
    ))
    loop.close()
    pass
