import random
import string
from scache_client import *
import optparse
import time
import datetime


async def warmup(ip, port, number, dict, length=8):
    client = await getScacheclient(ip, port)
    for i in range(number):
        randomStr = ''.join(
            random.sample(string.ascii_letters + string.digits, 8))
        dict[i] = randomStr
        await client.setKeyValue(randomStr, randomStr)
    await client.close()


async def randomAccess(ip, port, number, dict):
    client = await getScacheclient(ip, port)
    print("{}:{} start: ", end=" ")
    print(int(round(time.time() * 1000)))
    for i in range(number):
        keyIndex = random.randint(0, len(dict) - 1)
        key = dict[keyIndex]
        ops = random.randint(0, 3)
        if (ops == 0):  # get
            await client.getKeyValue(key)
            continue
        if (ops == 1):  # update
            valueLength = random.randint(24, 32)
            value = random.sample(string.ascii_letters + string.digits,
                                  valueLength)
            await client.setKeyValue(key, value)
            continue
        if (ops == 2):  # add
            key = random.sample(string.ascii_letters + string.digits, 8)
            valueLength = random.randint(24, 32)
            value = random.sample(string.ascii_letters + string.digits,
                                  valueLength)
            await client.setKeyValue(key, value)
            dict[len(dict)] = key
            continue
        if (ops == 3):  # del
            await client.delKeyValue(key)
    await client.close()
    print("{}:{} end: ", end=" ")
    print(int(round(time.time() * 1000)))


opts = optparse.OptionParser()

opts.add_option(
    "-c",
    "--clientNumber",
    action="store",
    type="int",
    help="Number of clients.")
opts.add_option(
    "-w",
    "--warmup",
    action="store",
    type="int",
    help="Number of key-value used to warmup cache.")
opts.add_option(
    "-r",
    "--requestNumber",
    action="store",
    type="int",
    help="Number of request fo every client.")
opts.add_option(
    "-i",
    "--ip",
    action="store",
    type="string",
    help="IP address of cache server.")
opts.add_option(
    "-p", "--port", action="store", type="int", help="Port of cache server.")

opt, _ = opts.parse_args()

loop = asyncio.get_event_loop()

keyDict = {}
loop.run_until_complete(warmup(opt.ip, opt.port, opt.warmup, keyDict, 16))

taskList = []

for i in range(opt.clientNumber):
    taskList.append(
        asyncio.ensure_future(
            randomAccess(opt.ip, opt.port, opt.requestNumber, keyDict)))

start = int(round(time.time() * 1000))
loop.run_until_complete(asyncio.wait(taskList))
end = int(round(time.time() * 1000))

qps = int(((opt.clientNumber * opt.requestNumber) / (end - start)) * 1000)
concurrency = opt.clientNumber

print("QPS: {} Concurrency: {}".format(qps, concurrency))
