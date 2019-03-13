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

SimpleCache *g_simpleCache;
extern RequestBuffer *g_requestBuffer;
extern SessionManager *g_sessionManager;

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

// Error message
const std::string WRONG_REQUEST_FORMAT = "error wrong request format";
const std::string WRONG_REQUEST_COMMAND = "error wrong request command";
const std::string KEY_VALUE_NOT_EXIST = "error key-value not exist";
const std::string UNSUPPORTED_OPERATION = "error unsupported operation";

// 标志过期时间任务
const std::string EXPIRE_TASK = "expireTask";

void startServer() {
    if (!g_simpleCache || !g_requestBuffer || !g_sessionManager) {
        std::cout
            << "No simple cache or no request buffer or no session manager."
            << std::endl;
        exit(0);
    }
    std::map<std::string, std::string (*)(Request &)> funcs = {
        {SET_COMMAND, setKeyValue},        {GET_COMMAND, getKeyValue},
        {EXPIRE_COMMAND, expireKeyValue},  {DEL_COMMAND, delKeyValue},
        {DSET_COMMAND, dictSetKeyValue},   {DGET_COMMAND, dictGetKeyValue},
        {DDEL_COMMAND, dictDelKeyValue},   {LADD_COMMAND, listAddKeyValue},
        {LADDM_COMMAND, listAddmKeyValue}, {LPOP_COMMAND, listPopKeyValue},
        {LGET_COMMAND, listGetKeyValue},   {LALL_COMMAND, listAllKeyValue}};
    while (true) {
        Request rq = g_requestBuffer->getRequest();
        // 确保待删除的key-value确实过期
        if (rq.m_name == EXPIRE_TASK) {
            if (!g_simpleCache->checkExpire(rq.cmd[1])) {
                continue;
            }
        }
        if (funcs.find(rq.cmd[0]) == funcs.end()) {
            g_sessionManager->async_send(rq.m_name, WRONG_REQUEST_COMMAND);
            continue;
        }
        std::string (*func)(Request &) = funcs[rq.cmd[0]];
        auto result = func(rq);
        g_sessionManager->async_send(rq.m_name, result);
    }
}

void startExpire() {
    if (!g_simpleCache || !g_requestBuffer || !g_sessionManager) {
        std::cout
            << "No simple cache or no request buffer or no session manager."
            << std::endl;
        exit(0);
    }
    int checkNumber = 10000;
    checkNumber = std::max(checkNumber, g_simpleCache->getCacheSize());
    CacheList *list = g_simpleCache->getList();
    while (true) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(g_simpleCache->getExpireCycle()));

        auto tempNode = list->getTail();
        auto tempNumber = checkNumber;
        while (tempNumber > 0 && tempNode) {
            auto expireKey = tempNode->getValue()->getKey();
            if (g_simpleCache->checkExpire(expireKey)) {
                Request rq;
                rq.m_name = EXPIRE_TASK;
                rq.cmd.push_back("del " + expireKey);
            }

            tempNumber--;
            tempNode = tempNode->getPrev();
        }
    }
}

void ExpireTable::set(std::string key, std::chrono::milliseconds time) {
    auto expireTime = std::chrono::time_point_cast<std::chrono::milliseconds>(
                          std::chrono::system_clock::now()) +
                      time;
    m_map[key] = expireTime;
}
void ExpireTable::del(std::string key) {
    if (m_map.find(key) == m_map.end()) {
        return;
    }
    m_map.erase(key);
}

bool ExpireTable::checkExpire(std::string key) {
    if (m_map.find(key) == m_map.end()) {
        return false;
    }
    auto now = std::chrono::time_point_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now());
    return now >= m_map[key];
}

SimpleCache::~SimpleCache() {
    destoryInstance(g_dict);
    delete g_expire;
}

SimpleCache::SimpleCache(int maxCacheSize, int expireCycle)
    : LinkedDict("myCache", LinkType), m_maxCacheSize(maxCacheSize),
      m_expireCycle(expireCycle) {
    g_dict =
        dynamic_cast<LinkedDict *>(getInstance("myCache", nullptr, LinkType));
    g_expire = new ExpireTable();
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
    g_expire->del(key);
    if (getSize() > m_maxCacheSize) {
        auto v = getTail()->getValue()->getKey();
        delKeyValue(v);
    }
}

CacheObject *SimpleCache::getKeyValue(std::string key) {
    if (m_map.find(key) == m_map.end()) {
        return nullptr;
    }
    if (g_expire->checkExpire(key)) {
        delKeyValue(key);
        return nullptr;
    }
    g_expire->del(key);
    auto node = m_map[key];
    m_list->popNode(node);
    m_list->addNode(node);
    return node->getValue();
}

void SimpleCache::delKeyValue(std::string key) {
    auto node = popNode(key);
    node->destoryValue();
    delete node;
    g_expire->del(key);
}

void SimpleCache::setExpire(std::string key, long long time) {
    g_expire->set(key, std::chrono::milliseconds(time));
}
bool SimpleCache::checkExpire(std::string key) {
    return g_expire->checkExpire(key);
}
void SimpleCache::delExpire(std::string key) { g_expire->del(key); }

int SimpleCache::getMaxCacheSize() { return m_maxCacheSize; }
int SimpleCache::getExpireCycle() { return m_expireCycle; }

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
    long long expireTime = 0;
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
    std::string key = rq.cmd[1], value = rq.cmd[2];
    auto type = isNum(rq.cmd[2]) ? LongType : StringType;

    auto object = g_simpleCache->getKeyValue(key);

    if (!object || object->getValueType() != type) {
        object = getInstance(key, type);
        g_simpleCache->addKeyValue(object);
    }
    if (type == LongType) {
        object->setValue(std::stoll(value))
    } else {
        object->setValue(value);
    }
    // 设置新过期时间
    if (expireTime > 0) {
        g_simpleCache->setExpire(rq.cmd[1], expireTime);
    }
    return "ok";
}

std::string getKeyValue(Request &rq) {
    if (rq.cmd.size() != 2) {
        return WRONG_REQUEST_FORMAT;
    }
    std::string key = rq.cmd[1];
    auto object = g_simpleCache->getKeyValue(key);
    if (!object) {
        return KEY_VALUE_NOT_EXIST;
    }

    std::string result = "ok ";
    auto valueType = object->getValueType();
    switch (valueType) {
    case LongType:
        result += std::to_string(object->getValue<long long>());
        break;
    case StringType:
        result += object->getValue<std::string>();
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
    long long time = std::stoll(rq.cmd[2]);
    if (!g_simpleCache->existKeyValue(key)) {
        return KEY_VALUE_NOT_EXIST;
    }
    g_simpleCache->setExpire(key, time);
    return "ok";
}

std::string delKeyValue(Request &rq) {
    if (rq.cmd.size() != 2) {
        return WRONG_REQUEST_FORMAT;
    }
    std::string key = rq.cmd[1];
    if (!g_simpleCache->existKeyValue(key)) {
        return KEY_VALUE_NOT_EXIST;
    }
    g_simpleCache->delKeyValue(key);
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

    auto object = g_simpleCache->getKeyValue(mainKey);
    // 对应词典不存在则创建词典，对应对象不是词典类型则返回
    // 不支持的操作
    if (!object)
        g_simpleCache->addKeyValue(getInstance(mainKey, DictType));
    else if (object->getValueType() != DictType)
        return UNSUPPORTED_OPERATION;

    object = g_simpleCache->getKeyValue(mainKey);
    auto dict = dynamic_cast<CacheDict *>(object);

    object = dict->getKeyValue(viceKey);
    if (!object || object->getValueType() != type) {
        object = getInstance(viceKey, type);
        dict->addKeyValue(object);
    }
    if (type == LongType) {
        object->setValue(std::stoll(value))
    } else {
        object->setValue(value);
    }
    return "ok";
}

std::string dictGetKeyValue(Request &rq) {
    if (rq.cmd.size() != 3) {
        return WRONG_REQUEST_FORMAT;
    }
    std::string mainKey = rq.cmd[1];
    std::string viceKey = rq.cmd[2];
    auto object = g_simpleCache->getKeyValue(mainKey);
    if (!object)
        return KEY_VALUE_NOT_EXIST;
    if (object->getValueType() != DictType)
        return UNSUPPORTED_OPERATION;

    object = object->getKeyValue(viceKey);
    if (!object)
        return KEY_VALUE_NOT_EXIST;
    CacheType type = object->getValueType();

    std::string result = "ok ";

    switch (type) {
    case LongType:
        result += std::to_string(object->getValue<long long>());
        break;
    case StringType:
        result += object->getValue<std::string>();
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
    auto object = g_simpleCache->getKeyValue(mainKey);
    if (!object)
        return KEY_VALUE_NOT_EXIST;
    if (object->getValueType() != DictType)
        return UNSUPPORTED_OPERATION;

    if (object->getKeyValue(viceKey))
        object->delKeyValue(viceKey);
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
    CachtType type = isNum(value) ? LongType : StringType;
    // 检查对象是否存在以及类型是否匹配
    auto object = g_simpleCache->getKeyValue(mainKey);
    if (!object)
        g_simpleCache->addKeyValue(getInstance(mainKey, ListType));
    else if (object->getValueType() != ListType)
        return UNSUPPORTED_OPERATION;

    object = g_simpleCache->getKeyValue(mainKey);
    auto list = dynamic_cast<CacheList *>(object);

    object = getInstance(viceKey, type);
    if (type == LongType) {
        object->setValue(std::stoll(value))
    } else {
        object->setValue(value);
    }
    list->addKeyValue(object);

    return "ok";
}

std::string listAddmKeyValue(Request &rq) {
    if (rq.cmd.size() < 3) {
        return WRONG_REQUEST_FORMAT;
    }
    std::string mainKey = rq.cmd[1];
    auto object = g_simpleCache->getKeyValue(mainKey);
    if (!object)
        g_simpleCache->addKeyValue(getInstance(mainKey, ListType));
    else if (object->getValueType() != ListType)
        return UNSUPPORTED_OPERATION;
    object = g_simpleCache->getKeyValue(mainKey);

    for (int i = 2; i < rq.cmd.size(); i++) {
        std::string value = rq.cmd[i];
        auto type = isNum(value) ? LongType : StringType;
        subObject = getInstance(std::string(), type);
        if (type == LongType)
            subObject->setValue(std::stoll(value));
        else
            subObject->setValue(value);
        object->addKeyValue(subObject);
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
    auto object = g_simpleCache->getKeyValue(mainKey);
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
        node = list->serachByKey(viceKey);
        if (node)
            list->popNode(node);
    }
    if (!node)
        return KEY_VALUE_NOT_EXIST;

    object = node->getValue();

    std::string result = "ok ";
    auto type = object->getValueType();

    switch (type) {
    case LongType:
        result += std::to_string(object->getValue<long long>());
        break;
    case StringType:
        result += object->getValue<std::string>();
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

    auto object = g_simpleCache->getKeyValue(mainKey);
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
        node = list->serachByKey(viceKey);
    }
    if (!node)
        return KEY_VALUE_NOT_EXIST;

    object = node->getValue();

    std::string result = "ok ";
    auto type = object->getValueType();

    switch (type) {
    case LongType:
        result += std::to_string(object->getValue<long long>());
        break;
    case StringType:
        result += object->getValue<std::string>();
        break;
    }
    return result;
}

std::string listAllKeyValue(Request &rq) {
    if (rq.cmd.size() != 2) {
        return WRONG_REQUEST_FORMAT;
    }
    std::string key = rq.cmd[1];
    auto object = g_simpleCache->getKeyValue(key);
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
        switch (type) {
        case LongType:
            result += std::to_string(object->getValue<long long>());
            break;
        case StringType:
            result += object->getValue<std::string>();
            break;
        }
        result += "\r\n";
    }
    return result;
}