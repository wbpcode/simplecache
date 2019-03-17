#include "cache-server.h"
#include "cache-dict.h"
#include "cache-list.h"
#include "cache-object.h"
#include "cache-session.h"
#include "request-buffer.h"
#include <map>
#include <sstream>
#include <string>
#include <vector>

// Command
const std::string SET_COMMAND = "set";
const std::string GET_COMMAND = "get";
const std::string EXPIRE_COMMAND = "expire";
const std::string DEL_COMMAND = "del";
const std::string DSET_COMMAND = "dset";
const std::string DGET_COMMAND = "dget";
const std::string DDEL_COMMAND = "ddel";
const std::string LADD_COMMAND = "ladd";
const std::string LADDM_COMMAND = "laddm";
const std::string LPOP_COMMAND = "lpop";
const std::string LGET_COMMAND = "lget";
const std::string LALL_COMMAND = "lall";
const std::string LOCK_COMMAND = "lock";
const std::string UNLOCK_COMMAND = "unlock";

// Error message
const std::string WRONG_REQUEST_FORMAT = "error wrong request format";
const std::string WRONG_REQUEST_COMMAND = "error wrong request command";
const std::string KEY_VALUE_NOT_EXIST = "error key-value not exist";
const std::string UNSUPPORTED_OPERATION = "error unsupported operation";
const std::string KEY_VALUE_IS_LOCKED = "error key-value is locked";

// 标志过期时间任务
const std::string EXPIRE_TASK = "expireTask";

void CacheExpireTable::set(std::string &key, int64 time) {
    m_map[key] = getCurrentTime() + time;
}
void CacheExpireTable::del(std::string &key) {
    if (m_map.find(key) == m_map.end()) {
        return;
    }
    m_map.erase(key);
}

bool CacheExpireTable::checkExpire(std::string &key) {
    if (m_map.find(key) == m_map.end()) {
        return false;
    }
    return getCurrentTime() >= m_map[key];
}

CacheLock::CacheLock(std::string &name, int64 time) :
    m_name(name), m_expireTime(time) {
    ;
}

void CacheLockTable::set(std::string &key, std::string &name, int64 time) {
    m_map[key] = CacheLock(name, getCurrentTime() + time);
}

void CacheLockTable::del(std::string &key) {
    if (m_map.find(key) == m_map.end()) {
        return;
    }
    m_map.erase(key);
}

bool CacheLockTable::checkLock(std::string &key, std::string &name) {
    if (m_map.find(key) == m_map.end()) {
        return false;
    }
    auto lock = m_map[key];
    if (getCurrentTime() >= lock.m_expireTime) {
        del(key);
        return false;
    }
    if (name != lock.m_name) {
        return true;
    }
    return false;
}


SimpleCache::~SimpleCache() { delete m_expireTable; delete m_lockTable; }

SimpleCache::SimpleCache() : LinkedDict("mySimpleCache",LinkType) {
    m_expireTable = new CacheExpireTable();
    m_lockTable = new CacheLockTable();
    m_globalConfig = getGlobalConfig();
}

void SimpleCache::addKeyValue(CacheObject *o) {
    // 注意区分m_list的成员函数popNode和继承自
    // LinkedDict的popNode，前者仅仅修改链表，
    // 而后者修改链表以及m_map
    CacheListNode *node = nullptr;
    std::string key = o->getKey();
    if (m_map.find(key) != m_map.end()) {
        node = m_map[key];
        node->destoryValue();
        node->setValue(o);
        m_list->popNode(node);
    } else {
        node = new CacheListNode(o);
        m_map[key] = node;
    }
    m_list->addNode(node);
    // 处理过期时间以及超出上限的key-value对
    m_expireTable->del(key);
    if (getSize() > m_globalConfig->maxCacheSize) {
        auto v = getTail()->getValue()->getKey();
        delKeyValue(v);
    }
}

CacheObject *SimpleCache::getKeyValue(std::string key) {
    if (m_map.find(key) == m_map.end()) {
        return nullptr;
    }
    if (m_expireTable->checkExpire(key)) {
        delKeyValue(key);
        return nullptr;
    }
    m_expireTable->del(key);
    auto node = m_map[key];
    m_list->popNode(node);
    m_list->addNode(node);
    return node->getValue();
}

void SimpleCache::delKeyValue(std::string key) {
    auto node = popNode(key);
    node->destoryValue();
    delete node;
    m_expireTable->del(key);
}

void SimpleCache::setExpire(std::string &key, int64 time) {
    m_expireTable->set(key, time);
}
bool SimpleCache::checkExpire(std::string &key) {
    return m_expireTable->checkExpire(key);
}
void SimpleCache::delExpire(std::string &key) { 
    m_expireTable->del(key); 
}

void SimpleCache::setLock(std::string &key, std::string &name) {
    m_lockTable->set(key, name, m_globalConfig->lockDuration);
}
bool SimpleCache::checkLock(std::string &key,std::string &name) {
    return m_lockTable->checkLock(key, name);
}
void SimpleCache::delLock(std::string &key) {
    m_lockTable->del(key);
}

bool isNum(std::string str) {
    std::stringstream sin(str);
    long long n;
    char p;
    if (!(sin >> n)) {
        return false;
    }
    if (sin >> p) {
        return false;
    } else {
        return true;
    }
}

std::string setKeyValue(Request &rq) {
    // 检查指令格式，指令长度为3或5，至少为3
    if (rq.cmd.size() != 3 && rq.cmd.size() != 5) {
        return WRONG_REQUEST_FORMAT;
    }
    // 检查是否有设置过期时间及过期时间设置格式是否正确
    int64 expireTime = 0;
    if (rq.cmd.size() == 5) {
        if (rq.cmd[3] != EXPIRE_COMMAND) {
            return WRONG_REQUEST_COMMAND;
        }
        if (!isNum(rq.cmd[4])) {
            return WRONG_REQUEST_FORMAT;
        }
        expireTime = std::stoll(rq.cmd[4]);
        if (expireTime <= 0) {
            return WRONG_REQUEST_FORMAT;
        }
    }
    auto cache = getSimpleCache();
    std::string key = rq.cmd[1], value = rq.cmd[2];
    auto type = isNum(rq.cmd[2]) ? LongType : StringType;
    if (cache->checkLock(key, rq.m_name)) {
        return KEY_VALUE_IS_LOCKED;
    }
    auto object = cache->getKeyValue(key);
    if (!object || object->getValueType() != type) {
        object = getInstance(key, type);
        cache->addKeyValue(object);
    }
    auto cacheValue = dynamic_cast<CacheValue*>(object);
    if (type == LongType) {
        cacheValue->setValue(std::stoll(value));
    } else {
        cacheValue->setValue(value);
    }
    // 设置新过期时间
    if (expireTime > 0) {
        cache->setExpire(rq.cmd[1], expireTime);
    }
    return "ok";
}

std::string getKeyValue(Request &rq) {
    if (rq.cmd.size() != 2) {
        return WRONG_REQUEST_FORMAT;
    }
    std::string key = rq.cmd[1];
    auto cache = getSimpleCache();
    if (cache->checkLock(key, rq.m_name)) {
        return KEY_VALUE_IS_LOCKED;
    }
    auto object = cache->getKeyValue(key);
    if (!object) {
        return KEY_VALUE_NOT_EXIST;
    }
    std::string result = "ok ";
    auto type = object->getValueType();
    switch (type) {
    case LongType:
        result += std::to_string(
            dynamic_cast<CacheValue*>(object)->getValue<int64>());
        break;
    case StringType:
        result += dynamic_cast<CacheValue*>(object)->getValue<std::string>();
        break;
    case ListType:
        result += object->getKey() + " (list)";
        break;
    case DictType:
        result += object->getKey() + " (dict)";
        break;
    }
    return result;
}

std::string expireKeyValue(Request &rq) {
    if (rq.cmd.size() != 3) {
        return WRONG_REQUEST_FORMAT;
    }
    if (!isNum(rq.cmd[2])) {
        return WRONG_REQUEST_FORMAT;
    }
    std::string key = rq.cmd[1];
    auto cache = getSimpleCache();
    if (cache->checkLock(key, rq.m_name)) {
        return KEY_VALUE_IS_LOCKED;
    }
    int64 time = std::stoll(rq.cmd[2]);
    if (!cache->existKeyValue(key)) {
        return KEY_VALUE_NOT_EXIST;
    }
    cache->setExpire(key, time);
    return "ok";
}

std::string delKeyValue(Request &rq) {
    if (rq.cmd.size() != 2) {
        return WRONG_REQUEST_FORMAT;
    }
    std::string key = rq.cmd[1];
    auto cache = getSimpleCache();
    if (cache->checkLock(key, rq.m_name)) {
        return KEY_VALUE_IS_LOCKED;
    }
    if (!cache->existKeyValue(key)) {
        return KEY_VALUE_NOT_EXIST;
    }
    cache->delKeyValue(key);
    return "ok";
}

std::string dictSetKeyValue(Request &rq) {
    if (rq.cmd.size() != 4) {
        return WRONG_REQUEST_FORMAT;
    }
    std::string mainKey = rq.cmd[1];
    std::string viceKey = rq.cmd[2];
    std::string value = rq.cmd[3];
    CacheType type = isNum(value) ? LongType : StringType;
    auto cache = getSimpleCache();
    if (cache->checkLock(mainKey, rq.m_name)) {
        return KEY_VALUE_IS_LOCKED;
    }
    auto object = cache->getKeyValue(mainKey);
    // 对应词典不存在则创建词典，对应对象不是词典类型则返回
    // 不支持的操作
    if (!object)
        cache->addKeyValue(getInstance(mainKey, DictType));
    else if (object->getValueType() != DictType)
        return UNSUPPORTED_OPERATION;

    object = cache->getKeyValue(mainKey);
    auto dict = dynamic_cast<CacheDict *>(object);
    object = dict->getKeyValue(viceKey);
    if (!object || object->getValueType() != type) {
        object = getInstance(viceKey, type);
        dict->addKeyValue(object);
    }
    auto cacheValue = dynamic_cast<CacheValue *>(object);
    if (type == LongType) {
        cacheValue->setValue(std::stoll(value));
    } else {
        cacheValue->setValue(value);
    }
    return "ok";
}

std::string dictGetKeyValue(Request &rq) {
    if (rq.cmd.size() != 3) {
        return WRONG_REQUEST_FORMAT;
    }
    std::string mainKey = rq.cmd[1];
    std::string viceKey = rq.cmd[2];
    auto cache = getSimpleCache();
    if (cache->checkLock(mainKey, rq.m_name)) {
        return KEY_VALUE_IS_LOCKED;
    }
    auto object = cache->getKeyValue(mainKey);
    if (!object)
        return KEY_VALUE_NOT_EXIST;
    if (object->getValueType() != DictType)
        return UNSUPPORTED_OPERATION;
    auto dict = dynamic_cast<CacheDict *>(object);
    object = dict->getKeyValue(viceKey);
    if (!object)
        return KEY_VALUE_NOT_EXIST;
    CacheType type = object->getValueType();

    std::string result = "ok ";
    switch (type) {
    case LongType:
        result += std::to_string(
            dynamic_cast<CacheValue *>(object)->getValue<int64>());
        break;
    case StringType:
        result += dynamic_cast<CacheValue*>(object)->getValue<std::string>();
        break;
    case ListType:
        result += object->getKey() + " (list)";
        break;
    case DictType:
        result += object->getKey() + " (dict)";
        break;
    }
    return result;
}

std::string dictDelKeyValue(Request &rq) {
    if (rq.cmd.size() != 3) {
        return WRONG_REQUEST_FORMAT;
    }
    std::string mainKey = rq.cmd[1];
    std::string viceKey = rq.cmd[2];
    auto cache = getSimpleCache();
    if (cache->checkLock(mainKey, rq.m_name)) {
        return KEY_VALUE_IS_LOCKED;
    }

    auto object = cache->getKeyValue(mainKey);
    if (!object)
        return KEY_VALUE_NOT_EXIST;
    if (object->getValueType() != DictType)
        return UNSUPPORTED_OPERATION;
    auto dict = dynamic_cast<CacheDict *>(object);
    if (dict->getKeyValue(viceKey))
        dict->delKeyValue(viceKey);
    else
        return KEY_VALUE_NOT_EXIST;
    return "ok";
}

// list中key不保证唯一性，可重复，可缺省，仅作为可选标记
std::string listAddKeyValue(Request &rq) {
    if (rq.cmd.size() != 3 && rq.cmd.size() != 4) {
        return WRONG_REQUEST_FORMAT;
    }
    std::string mainKey = rq.cmd[1];
    std::string viceKey, value;
    if (rq.cmd.size() == 4) {
        viceKey = rq.cmd[2];
        value = rq.cmd[3];
    } else {
        value = rq.cmd[2];
    }
    CacheType type = isNum(value) ? LongType : StringType;

    auto cache = getSimpleCache();
    if (cache->checkLock(mainKey, rq.m_name)) {
        return KEY_VALUE_IS_LOCKED;
    }

    // 检查对象是否存在以及类型是否匹配
    auto object = cache->getKeyValue(mainKey);
    if (!object)
        cache->addKeyValue(getInstance(mainKey, ListType));
    else if (object->getValueType() != ListType)
        return UNSUPPORTED_OPERATION;

    object = cache->getKeyValue(mainKey);
    auto list = dynamic_cast<CacheList *>(object);

    object = getInstance(viceKey, type);
    auto cacheValue = dynamic_cast<CacheValue *>(object);
    if (type == LongType) {
        cacheValue->setValue(std::stoll(value));
    } else {
        cacheValue->setValue(value);
    }
    list->addKeyValue(object);

    return "ok";
}

std::string listAddmKeyValue(Request &rq) {
    if (rq.cmd.size() < 3) {
        return WRONG_REQUEST_FORMAT;
    }
    std::string mainKey = rq.cmd[1];

    auto cache = getSimpleCache();
    if (cache->checkLock(mainKey, rq.m_name)) {
        return KEY_VALUE_IS_LOCKED;
    }
    auto object = cache->getKeyValue(mainKey);
    if (!object)
        cache->addKeyValue(getInstance(mainKey, ListType));
    else if (object->getValueType() != ListType)
        return UNSUPPORTED_OPERATION;
    object = cache->getKeyValue(mainKey);
    auto list = dynamic_cast<CacheList *>(object);
    for (long long i = 2; i < rq.cmd.size(); i++) {
        std::string value = rq.cmd[i];
        auto type = isNum(value) ? LongType : StringType;
        auto subObject = getInstance(std::string(), type);
        auto cacheValue = dynamic_cast<CacheValue *>(subObject);
        if (type == LongType)
            cacheValue->setValue(std::stoll(value));
        else
            cacheValue->setValue(value);
        list->addKeyValue(subObject);
    }
    return "ok";
}

std::string listPopKeyValue(Request &rq) {
    if (rq.cmd.size() != 2 && rq.cmd.size() != 3) {
        return WRONG_REQUEST_FORMAT;
    }
    std::string mainKey = rq.cmd[1];
    std::string viceKey;
    if (rq.cmd.size() == 3) {
        viceKey = rq.cmd[2];
    }
    auto cache = getSimpleCache();
    if (cache->checkLock(mainKey, rq.m_name)) {
        return KEY_VALUE_IS_LOCKED;
    }

    auto object = cache->getKeyValue(mainKey);
    if (!object) {
        return KEY_VALUE_NOT_EXIST;
    } else if (object->getValueType() != ListType) {
        return UNSUPPORTED_OPERATION;
    }

    CacheListNode *node = nullptr;
    auto list = dynamic_cast<CacheList *>(object);
    if (viceKey == std::string()) {
        node = list->popNode();
    } else {
        node = list->getNode(viceKey);
        if (node)
            list->popNode(node);
    }
    if (!node)
        return KEY_VALUE_NOT_EXIST;

    object = node->getValue();

    std::string result = "ok ";
    auto type = object->getValueType();
    auto cacheValue = dynamic_cast<CacheValue *>(object);
    switch (type) {
    case LongType:
        result += std::to_string(cacheValue->getValue<long long>());
        break;
    case StringType:
        result += cacheValue->getValue<std::string>();
        break;
    }

    node->destoryValue();
    delete node;

    return result;
}

std::string listGetKeyValue(Request &rq) {
    if (rq.cmd.size() != 2 && rq.cmd.size() != 3) {
        return WRONG_REQUEST_FORMAT;
    }
    std::string mainKey = rq.cmd[1];
    std::string viceKey;
    if (rq.cmd.size() == 3) {
        viceKey = rq.cmd[2];
    }
    auto cache = getSimpleCache();
    if (cache->checkLock(mainKey, rq.m_name)) {
        return KEY_VALUE_IS_LOCKED;
    }
    auto object = cache->getKeyValue(mainKey);
    if (!object) {
        return KEY_VALUE_NOT_EXIST;
    } else if (object->getValueType() != ListType) {
        return UNSUPPORTED_OPERATION;
    }
    CacheListNode *node = nullptr;
    auto list = dynamic_cast<CacheList *>(object);

    if (viceKey == std::string()) {
        node = list->getHead()->getNext();
    } else {
        node = list->getNode(viceKey);
    }
    if (!node)
        return KEY_VALUE_NOT_EXIST;

    object = node->getValue();

    std::string result = "ok ";
    auto type = object->getValueType();
    auto cacheValue = dynamic_cast<CacheValue *>(object);
    switch (type) {
    case LongType:
        result += std::to_string(cacheValue->getValue<long long>());
        break;
    case StringType:
        result += cacheValue->getValue<std::string>();
        break;
    }
    return result;
}

std::string listAllKeyValue(Request &rq) {
    if (rq.cmd.size() != 2) {
        return WRONG_REQUEST_FORMAT;
    }
    std::string key = rq.cmd[1];

    auto cache = getSimpleCache();
    if (cache->checkLock(key, rq.m_name)) {
        return KEY_VALUE_IS_LOCKED;
    }

    auto object = cache->getKeyValue(key);
    if (!object) {
        return KEY_VALUE_NOT_EXIST;
    } else if (object->getValueType() != ListType) {
        return UNSUPPORTED_OPERATION;
    }
    auto list = dynamic_cast<CacheList *>(object);
    CacheListNode *node = list->getHead();
    std::string result = "ok ";
    while (node = node->getNext()) {
        object = node->getValue();
        auto type = object->getValueType();
        if (object->getKey() == std::string())
            result += "(empty) ";
        else
            result += object->getKey() + " ";
        auto cacheValue = dynamic_cast<CacheValue *>(object);
        switch (type) {
        case LongType:
            result += std::to_string(cacheValue->getValue<long long>());
            break;
        case StringType:
            result += cacheValue->getValue<std::string>();
            break;
        }
        result += "\r\n";
    }
    return result;
}

std::string lockKeyValue(Request& rq) {
    if (rq.cmd.size() != 2) {
        return WRONG_REQUEST_FORMAT;
    }
    auto key = rq.cmd[1];
    auto cache = getSimpleCache();
    if (cache->checkLock(key, rq.m_name)) {
        return KEY_VALUE_IS_LOCKED;
    }
    cache->setLock(key, rq.m_name);
    return "ok";
}

std::string unlockKeyValue(Request& rq) {
    if (rq.cmd.size() != 2) {
        return WRONG_REQUEST_FORMAT;
    }
    auto key = rq.cmd[1];
    auto cache = getSimpleCache();
    if (cache->checkLock(key, rq.m_name)) {
        return KEY_VALUE_IS_LOCKED;
    }
    cache->delLock(rq.cmd[1]);
    return "ok";
}

SimpleCache* getSimpleCache() {
    static SimpleCache* cache = new SimpleCache();
    return cache;
}

void delSimpleCache() {
    delete getSimpleCache();
}

void startServer() {
    std::map<std::string, std::string(*)(Request &)> funcs = {
        {SET_COMMAND, setKeyValue},        {GET_COMMAND, getKeyValue},
        {EXPIRE_COMMAND, expireKeyValue},  {DEL_COMMAND, delKeyValue},
        {DSET_COMMAND, dictSetKeyValue},   {DGET_COMMAND, dictGetKeyValue},
        {DDEL_COMMAND, dictDelKeyValue},   {LADD_COMMAND, listAddKeyValue},
        {LADDM_COMMAND, listAddmKeyValue}, {LPOP_COMMAND, listPopKeyValue},
        {LGET_COMMAND, listGetKeyValue},   {LALL_COMMAND, listAllKeyValue},
        {LOCK_COMMAND, lockKeyValue},      {UNLOCK_COMMAND, unlockKeyValue}};

    auto requestBuffer = getRequestBuffer();
    auto simpleCache = getSimpleCache();
    auto sessionManager = getSessionManager();
    std::cout << "Server task is started." << std::endl;
    while (true) {
        Request rq = requestBuffer->getRequest();
        // 确保待删除的key-value确实过期
        if (rq.m_name == EXPIRE_TASK) {
            if (!simpleCache->checkExpire(rq.cmd[1])) {
                continue;
            }
        }
        if (funcs.find(rq.cmd[0]) == funcs.end()) {
            sessionManager->async_send(rq.m_name, WRONG_REQUEST_COMMAND);
            continue;
        }
        std::string (*func)(Request &) = funcs[rq.cmd[0]];
        auto result = func(rq);
        sessionManager->async_send(rq.m_name, result);
    }
    std::cout << "Server task is closed." << std::endl;
}

void startExpire() {
    long long checkNumber = 10000;
    auto simpleCache = getSimpleCache();
    auto globalConfig = getGlobalConfig();
    checkNumber = std::max(checkNumber, simpleCache->getSize());
    auto listHead = simpleCache->getHead();
    std::cout << "Expire task is started." << std::endl;
    while (true) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(globalConfig->expireCycle));

        auto tempNode = simpleCache->getTail();
        auto tempNumber = checkNumber;
        while (tempNumber > 0 && tempNode && tempNode != listHead) {
            auto expireKey = tempNode->getValue()->getKey();
            if (simpleCache->checkExpire(expireKey)) {
                Request rq;
                rq.m_name = EXPIRE_TASK;
                rq.cmd.push_back("del " + expireKey);
            }

            tempNumber--;
            tempNode = tempNode->getPrev();
        }
    }
    std::cout << "Expire task is closed." << std::endl;
}
