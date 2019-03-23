# coding:utf-8
import time
import asyncio


# 测试用客户端
async def getScacheclient(ip, port):
    reader, writer = await asyncio.open_connection(ip, port)
    return Scacheclient(reader, writer)


class Scacheclient:
    def __init__(self, reader, writer):
        self.__reader = reader
        self.__writer = writer

    async def __send(self, meg):
        self.__writer.write(meg.encode())
        await self.__writer.drain()

    async def __recv(self):
        result = await self.__reader.read(1024)
        return result.decode('utf-8')

    async def close(self):
        self.__writer.close()

    async def setKeyValue(self, key, value, expire=-1):
        if (expire == -1):
            cmd = "set {} {}".format(key, value)
        else:
            cmd = "set {} {} expire {}".format(key, value, expire)
        await self.__send(cmd)
        return await self.__recv()

    async def getKeyValue(self, key):
        cmd = "get {}".format(key)
        await self.__send(cmd)
        return await self.__recv()

    async def expireKeyValue(self, key, expire):
        cmd = "expire {} {}".format(key, expire)
        await self.__send(cmd)
        return await self.__recv()

    async def delKeyValue(self, key):
        cmd = "del {}".format(key)
        await self.__send(cmd)
        return await self.__recv()

    async def dictSetKeyValue(self, mainKey, viceKey, value):
        cmd = "dset {} {} {}".format(mainKey, viceKey, value)
        await self.__send(cmd)
        return await self.__recv()

    async def dictGetKeyValue(self, mainKey, viceKey):
        cmd = "dget {} {}".format(mainKey, viceKey)
        await self.__send(cmd)
        return await self.__recv()

    async def dictDelKeyValue(self, mainKey, viceKey):
        cmd = "ddel {} {}".format(mainKey, viceKey)
        await self.__send(cmd)
        return await self.__recv()

    async def listAddKeyValue(self, mainKey, *value):
        cmd = "ladd {}".format(mainKey)
        for v in value:
            cmd += (" " + str(v))
        await self.__send(cmd)
        return await self.__recv()

    async def listPopKeyValue(self, mainKey):
        cmd = "lpop {}".format(mainKey)
        await self.__send(cmd)
        return await self.__recv()

    async def listGetKeyValue(self, mainKey):
        cmd = "lget {}".format(mainKey)
        await self.__send(cmd)
        return await self.__recv()

    async def listAllKeyValue(self, mainKey):
        cmd = "lall {}".format(mainKey)
        await self.__send(cmd)
        return await self.__recv()

    async def lockKeyValue(self, key):
        cmd = "lock {}".format(key)
        await self.__send(cmd)
        return await self.__recv()

    async def unlockKeyValue(self, key):
        cmd = "unlock {}".format(key)
        await self.__send(cmd)
        return await self.__recv()


# 基本功能测试
async def setKeyValueTest():
    print("Test setKeyValue")
    c = await getScacheclient("127.0.0.1", 2333)
    
    print("Set value...")
    print(await c.setKeyValue("wangbaiping", "wangbaiping"))
    print("Set value with expire...")
    print(await c.setKeyValue("wangbaiping2", "wangbaiping2", 10000))
    print("Get value...")
    print(await c.getKeyValue("wangbaiping"))
    print("Get value with expire...")
    print(await c.getKeyValue("wangbaiping2"))

    print("Sleep 20 seconds...")
    await asyncio.sleep(20)

    print("Get value...")
    print(await c.getKeyValue("wangbaiping"))
    print("Get value with expire....")
    print(await c.getKeyValue("wangbaiping2"))


async def getKeyValueTest():
    print("Test getKeyValue")
    c = await getScacheclient("127.0.0.1", 2333)
    print("Set value...")
    print(await c.setKeyValue("wangbaiping", "wangbaiping"))
    print("Get value...")
    print(await c.getKeyValue("wangbaiping"))

    print("Get value not exist...")
    print(await c.getKeyValue("wangbaiping3"))


async def delKeyValueTest():
    print("Test delKeyValue")
    c = await getScacheclient("127.0.0.1", 2333)
    print("Set value...")
    print(await c.setKeyValue("wangbaiping", "wangbaiping"))
    print("Get value...")
    print(await c.getKeyValue("wangbaiping"))
    print("Del value...")
    print(await c.delKeyValue("wangbaiping"))
    print("Get value again...")
    print(await c.getKeyValue("wangbaiping"))


async def expireKeyValueTest():
    print("Test expireKeyValue")
    c = await getScacheclient("127.0.0.1", 2333)
    print("Set value...")
    print(await c.setKeyValue("wangbaiping", "wangbaiping"))
    print("Get value...")
    print(await c.getKeyValue("wangbaiping"))

    print("Set expire...15s")
    print(await c.expireKeyValue("wangbaiping", 15000))
    print("Get value...")
    print(await c.getKeyValue("wangbaiping"))

    print("Sleep 20 seconds...")
    await asyncio.sleep(20)

    print("Get value...")
    print(await c.getKeyValue("wangbaiping"))


async def dictSetKeyValueTest():
    print("Test dictSetKeyValue")
    c = await getScacheclient("127.0.0.1", 2333)
    print("Dict set value...")
    print(await c.dictSetKeyValue("mydict", "mynumber", 12345))
    print("Get value in dict...")
    print(await c.dictGetKeyValue("mydict", "mynumber"))
    print("Get dict...")
    print(await c.getKeyValue("mydict"))

    await c.delKeyValue("mydict")


async def dictGetKeyValueTest():
    print("Test dictgetKeyValue")
    c = await getScacheclient("127.0.0.1", 2333)
    print("Dict set value...")
    print(await c.dictSetKeyValue("mydict", "mynumber", 12345))
    print("Get value in dict...")
    print(await c.dictGetKeyValue("mydict", "mynumber"))
    print("Get dict...")
    print(await c.getKeyValue("mydict"))

    await c.delKeyValue("mydict")


async def dictDelKeyValueTest():
    print("Test dictDelKeyValue")
    c = await getScacheclient("127.0.0.1", 2333)
    print("Dict set value...")
    print(await c.dictSetKeyValue("mydict", "mynumber", 12345))
    print("Get value in dict...")
    print(await c.dictGetKeyValue("mydict", "mynumber"))
    print("Get dict...")
    print(await c.getKeyValue("mydict"))
    print("Del value in dict...")
    print(await c.dictDelKeyValue("mydict", "mynumber"))
    print("Get value in dict...")
    print(await c.dictGetKeyValue("mydict", "mynumber"))


async def listAddKeyValueTest():
    print("Test listAddKeyValue")
    c = await getScacheclient("127.0.0.1", 2333)
    print("List add value...")
    print(await c.listAddKeyValue("mylist", "nihao", 12345))
    print("List get value...")
    print(await c.listGetKeyValue("mylist"))

    await c.delKeyValue("mylist")


async def listGetKeyValueTest():
    print("Test listGetKeyValue")
    c = await getScacheclient("127.0.0.1", 2333)
    print("List add value...")
    print(await c.listAddKeyValue("mylist", "nihao", 12345))
    print("List get value...")
    print(await c.listGetKeyValue("mylist"))

    await c.delKeyValue("mylist")


async def listPopKeyValueTest():
    print("Test listPopKeyValue")
    c = await getScacheclient("127.0.0.1", 2333)
    print("List add value...")
    print(await c.listAddKeyValue("mylist", "nihao", 12345))
    print("List get value...")
    print(await c.listGetKeyValue("mylist"))

    print("Pop list...")
    print(await c.listPopKeyValue("mylist"))
    print("Pop list...")
    print(await c.listPopKeyValue("mylist"))
    print("Pop list...")
    print(await c.listPopKeyValue("mylist"))

    await c.delKeyValue("mylist")


async def listAllKeyValueTest():
    print("Test listAllKeyValue")
    c = await getScacheclient("127.0.0.1", 2333)
    print("List add value...")
    print(await c.listAddKeyValue("mylist", "nihao", 12345))
    print("List get all value...")
    print(await c.listAllKeyValue("mylist"))


async def lockAndUnlockTest():
    print("Test lock and unlock")
    print("Create two sessions...")
    c1 = await getScacheclient("127.0.0.1", 2333)
    c2 = await getScacheclient("127.0.0.1", 2333)
    print("c1 set a key-value...")
    print(await c1.setKeyValue("wangbaiping", "wangbaiping"))
    print("c1 lock it...")
    print(await c1.lockKeyValue("wangbaiping"))
    print("c1 can still access it...")
    print(await c1.getKeyValue("wangbaiping"))
    print("c2 cannot access it...")
    print(await c2.getKeyValue("wangbaiping"))
    print("c1 unlock this key-value...")
    print(await c1.unlockKeyValue("wangbaiping"))
    print("c2 can access it now...")
    print(await c2.getKeyValue("wangbaiping"))
    print("c2 lock this key-value...")
    print(await c2.lockKeyValue("wangbaiping"))
    print("c1 cannot access it now...")
    print(await c1.getKeyValue("wangbaiping"))

    print("Sleep 10 seconds...")
    await asyncio.sleep(10)

    print("c1 can access this key-value again...")
    print(await c1.getKeyValue("wangbaiping"))


async def functionTest():
    print("---------------------------")
    await setKeyValueTest()
    print("---------------------------")
    await getKeyValueTest()
    print("---------------------------")
    await delKeyValueTest()
    print("---------------------------")
    await expireKeyValueTest()
    print("---------------------------")
    await dictSetKeyValueTest()
    print("---------------------------")
    await dictGetKeyValueTest()
    print("---------------------------")
    await dictDelKeyValueTest()
    print("---------------------------")

    await listAddKeyValueTest()
    print("---------------------------")
    await listGetKeyValueTest()
    print("---------------------------")
    await listPopKeyValueTest()
    print("---------------------------")
    await listAllKeyValueTest()
    print("---------------------------")
    await lockAndUnlockTest()
    print("---------------------------")


if __name__ == "__main__":
    loop=asyncio.get_event_loop()
    loop.run_until_complete(functionTest())