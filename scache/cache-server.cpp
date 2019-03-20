#include "request-buffer.h"
#include "cache-server.h"
#include "cache-dict.h"
#include "cache-list.h"
#include "cache-base.h"
#include "cache-tool.h"
#include "cache-session.h"
#include <map>
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
const std::string KEY_VALUE_IS_EXPIRED = "error key-value is expired";
const std::string CONTAINER_IS_EMPTY = "error container is empty";

// 标志过期时间任务
const std::string EXPIRE_TASK = "expireTask";

SimpleCache::SimpleCache() {
    m_expireTable = new ExpireTable();
    m_clientLockTable = new ClientLockTable();
    m_linkedList = new LinkedList();
    m_cacheTable = new CacheTable();
    m_globalConfig = getGlobalConfig();
}

SimpleCache::~SimpleCache() {
    delete m_clientLockTable;
    delete m_expireTable;
    // 销毁缓存中剩余一级对象
    std::function<void(const NodeType*)> func=
        [](const NodeType* x) {
        delInstance(x->getValue());
    };
    m_linkedList->walk(func);
    delete m_cacheTable;
}

// 更新或者插入对象，过期时间自动销毁，节点移动到链表首部
void SimpleCache::set(std::string& key, CacheBase* value) {
    try {
        // key已经存在：更新
        PairType& pair = m_cacheTable->get(key);
        // 销毁存储旧对象
        delInstance(pair.m_two.getValue());
        pair.m_two.setValue(value);
        // 销毁失效过期时间
        m_expireTable->del(key);
        // 将节点移动到链表首部
        m_linkedList->popNode(&pair.m_two);
        m_linkedList->addNode(&pair.m_two);
    }
    catch(std::string e){
        // key不存在：插入新pair
        m_cacheTable->set({key, CacheListNode<CacheBase*>(value)});
        // 获取实际存储的pair的引用并提取其中value部分(ListNode部分)的指针
        // 将新Node插入到链表中
        PairType& pair = m_cacheTable->get(key);
        m_linkedList->addNode(&pair.m_two);
    }
}

// 返回对象，节点移动到链表首部
CacheBase* SimpleCache::get(std::string& key) {
    try {
        auto& pair = m_cacheTable->get(key);
        m_linkedList->popNode(&pair.m_two);
        m_linkedList->addNode(&pair.m_two);
        return pair.m_two.getValue();
    }
    catch (std::string e) {
        return nullptr;
    }
}

// 删除对象，过期时间自动销毁，节点从链表移除，客户端锁自动销毁
void SimpleCache::del(std::string& key) {
    try {
        auto& pair = m_cacheTable->get(key);
        m_linkedList->popNode(&pair.m_two);
        delInstance(pair.m_two.getValue());
        m_cacheTable->del(key);
        m_clientLockTable->del(key);
        m_expireTable->del(key);
    }
    catch (std::string e) {
        return;
    }
}

bool SimpleCache::has(std::string& key) {
    return m_cacheTable->has(key);
}

int64 SimpleCache::getSize() {
    return m_cacheTable->getSize();
}

SimpleCache::NodeType* SimpleCache::getHead() {
    return m_linkedList->getHead();
}

SimpleCache::NodeType* SimpleCache::getTail() {
    return m_linkedList->getTail();
}

int64 SimpleCache::walk(std::function<void(const NodeType*)> func, 
    int64 maxSize = LLONG_MAX) {
    m_linkedList->walk(func, maxSize, false);
}

void SimpleCache::setClientLock(std::string& key, std::string& name) {
    int64 time = getCurrentTime() + m_globalConfig->lockDuration;
    m_clientLockTable->set({ key,{name,time} });
}

void SimpleCache::delClientLock(std::string& key) {
    m_clientLockTable->del(key);
}

// 如果锁不存在：返回false；如果锁过期：销毁锁，返回false；
// 如果锁未过期：判断客户端是否对应，是则返回false；否者返回true。
bool SimpleCache::getClientLock(std::string& key, std::string& name) {
    try {
        auto& pair = m_clientLockTable->get(key);
        if (getCurrentTime() >= pair.m_two.m_expireTime) {
            delClientLock(key);
            return false;
        }
        if (name == pair.m_two.m_name) {
            return false;
        }
        return true;
    }
    catch (std::string e) {
        return false;
    }
}

void SimpleCache::setExpire(std::string& key, int64 time) {
    time += getCurrentTime();
    m_expireTable->set({ key,time });
}

void SimpleCache::delExpire(std::string& key) {
    m_expireTable->del(key);
}

// 如果未设置过期时间，则数据未过期；如果设置过期时间则检查是否超时；
// 如果超时则销毁对应key(从链表移除，删除对象，删除客户端锁，删除过期时间)
bool SimpleCache::getExpire(std::string& key) {
    try {
        auto& pair = m_expireTable->get(key);
        if (getCurrentTime() >= pair.m_two) {
            del(key); return true;
        }
        return false;
    }
    catch (std::string e) {
        return false;
    }
}


std::string setKeyValueHandler(Request &rq) {
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
        if (!isNumber(rq.cmd[4])) {
            return WRONG_REQUEST_FORMAT;
        }
        expireTime = std::stoll(rq.cmd[4]);
        if (expireTime <= 0) {
            return WRONG_REQUEST_FORMAT;
        }
    }
    auto cache = getSimpleCache();
    std::string key = rq.cmd[1], value = rq.cmd[2];
    auto type = isNumber(rq.cmd[2]) ? LongType : StringType;
    if (cache->getClientLock(key, rq.m_name)) {
        return KEY_VALUE_IS_LOCKED;
    }
    auto object = getInstance<CacheValue>(type);
    object->setValue(value);

    cache->set(key, object);

    // 设置新过期时间
    if (expireTime > 0) {
        cache->setExpire(rq.cmd[1], expireTime);
    }
    return "ok";
}

std::string getKeyValueHandler(Request &rq) {
    if (rq.cmd.size() != 2) {
        return WRONG_REQUEST_FORMAT;
    }
    std::string key = rq.cmd[1];
    auto cache = getSimpleCache();
    if (cache->getClientLock(key, rq.m_name)) {
        return KEY_VALUE_IS_LOCKED;
    }
    if (cache->getExpire(key)) {
        return KEY_VALUE_IS_EXPIRED;
    }
    auto object = cache->get(key);
    if (!object) {
        return KEY_VALUE_NOT_EXIST;
    }
    std::string result = "ok ";
    switch (object->getType()) {
    case LongType:
        result += 
            dynamic_cast<CacheValue*>(object)->getValue();
        break;
    case StringType:
        result += 
            dynamic_cast<CacheValue*>(object)->getValue();
        break;
    case ListType:
        result += key + " (list)";
        break;
    case DictType:
        result += key + " (dict)";
        break;
    }
    return result;
}

std::string expireKeyValueHandler(Request &rq) {
    if (rq.cmd.size() != 3) {
        return WRONG_REQUEST_FORMAT;
    }
    if (!isNumber(rq.cmd[2])) {
        return WRONG_REQUEST_FORMAT;
    }
    std::string key = rq.cmd[1];
    auto cache = getSimpleCache();
    if (cache->getClientLock(key, rq.m_name)) {
        return KEY_VALUE_IS_LOCKED;
    }
    if (cache->getExpire(key)) {
        return KEY_VALUE_IS_EXPIRED;
    }
    if (!cache->has(key)) {
        return KEY_VALUE_NOT_EXIST;
    }
    int64 time = std::stoll(rq.cmd[2]);
    cache->setExpire(key, time);
    return "ok";
}

std::string delKeyValueHandler(Request &rq) {
    if (rq.cmd.size() != 2) {
        return WRONG_REQUEST_FORMAT;
    }
    std::string key = rq.cmd[1];
    auto cache = getSimpleCache();
    if (cache->getClientLock(key, rq.m_name)) {
        return KEY_VALUE_IS_LOCKED;
    }
    if (!cache->has(key)) {
        return KEY_VALUE_NOT_EXIST;
    }
    cache->del(key);
    return "ok";
}

std::string dictSetKeyValueHandler(Request &rq) {
    if (rq.cmd.size() != 4) {
        return WRONG_REQUEST_FORMAT;
    }
    std::string mainKey = rq.cmd[1];
    std::string viceKey = rq.cmd[2];
    std::string value = rq.cmd[3];
    CacheType type = isNumber(value) ? LongType : StringType;
    auto cache = getSimpleCache();
    if (cache->getClientLock(mainKey, rq.m_name)) {
        return KEY_VALUE_IS_LOCKED;
    }
    auto object = cache->get(mainKey);
    // 对应词典不存在则创建词典，对应对象不是词典类型则返回
    // 不支持的操作
    if (!object) {
        object = getInstance<CacheBase>(DictType);
        cache->set(mainKey, object);
    }      
    if (object->getType() != DictType)
        return UNSUPPORTED_OPERATION;
    
    auto dict = dynamic_cast<CacheDict<std::string, 
        CacheValue*>*>(object);

    auto temp = getInstance<CacheValue>(type);
    temp->setValue(value);
    try {
        auto& pair = dict->get(viceKey);
        delInstance(pair.m_two);
        pair.m_two = temp;
    }
    catch(std::string e){
        dict->set({ viceKey,temp });
    }

    return "ok";
}

std::string dictGetKeyValueHandler(Request &rq) {
    if (rq.cmd.size() != 3) {
        return WRONG_REQUEST_FORMAT;
    }
    std::string mainKey = rq.cmd[1];
    std::string viceKey = rq.cmd[2];
    auto cache = getSimpleCache();
    if (cache->getClientLock(mainKey, rq.m_name)) {
        return KEY_VALUE_IS_LOCKED;
    }
    if (cache->getExpire(mainKey)) {
        return KEY_VALUE_IS_EXPIRED;
    }

    auto object = cache->get(mainKey);
    if (!object)
        return KEY_VALUE_NOT_EXIST;
    if (object->getType() != DictType)
        return UNSUPPORTED_OPERATION;

    auto dict = dynamic_cast<CacheDict<std::string,
        CacheValue*>*>(object);

    std::string result = "ok ";
    try {
        auto& pair = dict->get(viceKey);
        return result + pair.m_two->getValue();
    }
    catch (std::string e) {
        return KEY_VALUE_NOT_EXIST;
    }
}

std::string dictDelKeyValueHandler(Request &rq) {
    if (rq.cmd.size() != 3) {
        return WRONG_REQUEST_FORMAT;
    }
    std::string mainKey = rq.cmd[1];
    std::string viceKey = rq.cmd[2];
    auto cache = getSimpleCache();
    if (cache->getClientLock(mainKey, rq.m_name)) {
        return KEY_VALUE_IS_LOCKED;
    }

    auto object = cache->get(mainKey);
    if (!object)
        return KEY_VALUE_NOT_EXIST;

    if (object->getType() != DictType)
        return UNSUPPORTED_OPERATION;

    auto dict = dynamic_cast<CacheDict<std::string,
        CacheValue*>*>(object);

    try {
        auto& pair = dict->get(viceKey);
        delInstance(pair.m_two);
        dict->del(viceKey);
        return "ok";
    }
    catch (std::string e) {
        return KEY_VALUE_NOT_EXIST;
    }
}

std::string listAddKeyValueHandler(Request &rq) {
    if (rq.cmd.size() < 3) {
        return WRONG_REQUEST_FORMAT;
    }
    std::string key = rq.cmd[1];

    auto cache = getSimpleCache();
    if (cache->getClientLock(key, rq.m_name)) {
        return KEY_VALUE_IS_LOCKED;
    }

    auto object = cache->get(key);

    if (!object) {
        object = getInstance<CacheBase>(ListType);
        cache->set(key, object);
    }
    if (object->getType() != ListType)
        return UNSUPPORTED_OPERATION;
    auto list = dynamic_cast<CacheList<CacheValue*>*>
        (object);

    for (int64 i = 2; i < rq.cmd.size(); i++) {
        std::string value = rq.cmd[i];
        auto type = isNumber(value) ? LongType : StringType;
        auto temp = getInstance<CacheValue>(type);
        temp->setValue(value);
        list->add(temp);
    }
    return "ok";
}

std::string listPopKeyValueHandler(Request &rq) {
    if (rq.cmd.size() != 2) {
        return WRONG_REQUEST_FORMAT;
    }
    std::string key = rq.cmd[1];

    auto cache = getSimpleCache();
    if (cache->getClientLock(key, rq.m_name)) {
        return KEY_VALUE_IS_LOCKED;
    }
    auto object = cache->get(key);
    if (!object)
        return KEY_VALUE_NOT_EXIST;
    if (object->getType() != ListType) {
        return UNSUPPORTED_OPERATION;
    }

    auto list = dynamic_cast<CacheList<CacheValue*>*>
        (object);

    try {
        auto temp = list->pop();
        std::string result = "ok " + temp->getValue();
        delInstance(temp);
    }
    catch(std::string e){
        return CONTAINER_IS_EMPTY;
    }
}


std::string listGetKeyValueHandler(Request &rq) {
    if (rq.cmd.size() != 2) {
        return WRONG_REQUEST_FORMAT;
    }
    std::string key = rq.cmd[1];

    auto cache = getSimpleCache();
    if (cache->getClientLock(key, rq.m_name)) {
        return KEY_VALUE_IS_LOCKED;
    }
    auto object = cache->get(key);
    if (!object)
        return KEY_VALUE_NOT_EXIST;
    if (object->getType() != ListType) {
        return UNSUPPORTED_OPERATION;
    }

    auto list = dynamic_cast<CacheList<CacheValue*>*>
        (object);

    if (list->getSize() <= 0) {
        return CONTAINER_IS_EMPTY;
    }

    auto temp = list->getHead()->getNext()->getValue();

    return "ok " + temp->getValue();
}

std::string listAllKeyValueHandler(Request &rq) {
    if (rq.cmd.size() != 2) {
        return WRONG_REQUEST_FORMAT;
    }
    std::string key = rq.cmd[1];

    auto cache = getSimpleCache();
    if (cache->getClientLock(key, rq.m_name)) {
        return KEY_VALUE_IS_LOCKED;
    }

    auto object = cache->get(key);
    if (!object) 
        return KEY_VALUE_NOT_EXIST;
    if (object->getType() != ListType) {
        return UNSUPPORTED_OPERATION;
    }

    auto list = dynamic_cast<CacheList<CacheValue*>*>
        (object);
   
    auto node = list->getHead();
    std::string result = "ok ";
    while (node = node->getNext()) {
        result += node->getValue()->getValue();
        result += "\r\n";
    }
    return result;
}

std::string lockKeyValueHandler(Request& rq) {
    if (rq.cmd.size() != 2) {
        return WRONG_REQUEST_FORMAT;
    }
    auto key = rq.cmd[1];
    auto cache = getSimpleCache();
    if (cache->getClientLock(key, rq.m_name)) {
        return KEY_VALUE_IS_LOCKED;
    }
    cache->setClientLock(key, rq.m_name);
    return "ok";
}

std::string unlockKeyValueHandler(Request& rq) {
    if (rq.cmd.size() != 2) {
        return WRONG_REQUEST_FORMAT;
    }
    auto key = rq.cmd[1];
    auto cache = getSimpleCache();
    if (cache->getClientLock(key, rq.m_name)) {
        return KEY_VALUE_IS_LOCKED;
    }
    cache->delClientLock(rq.cmd[1]);
    return "ok";
}

SimpleCache* getSimpleCache() {
    static SimpleCache* cache = new SimpleCache();
    return cache;
}

void delSimpleCache() {
    delete getSimpleCache();
}

void expireTaskHandler(){
    auto cache = getSimpleCache();

    int64 count = 0, maxCount = getGlobalConfig()->expireCount;

    auto tail = cache->getTail(), head = cache->getHead();
    if (!tail) return;

    auto temp = tail, prev = temp->getPrev();
    
    while (temp != head && count < maxCount) {
        SimpleCache::PairType* pair_p =
            getHeadPointer(temp, SimpleCache::PairType, m_two);
        std::string key = pair_p->m_one;
        // 锁过期自动销毁，第二个参数没有作用
        cache->getClientLock(key, key);
        // 数据过期自动销毁，包括链表关系，客户端锁，过期时间
        cache->getExpire(key);
        temp = prev; prev = temp->getPrev();
        count++;
    }
}



void startServer() {
    std::map<std::string, std::string(*)(Request &)> funcs = {
        {SET_COMMAND, setKeyValueHandler},        
        {GET_COMMAND, getKeyValueHandler},
        {EXPIRE_COMMAND, expireKeyValueHandler},  
        {DEL_COMMAND, delKeyValueHandler},
        {DSET_COMMAND, dictSetKeyValueHandler},   
        {DGET_COMMAND, dictGetKeyValueHandler},
        {DDEL_COMMAND, dictDelKeyValueHandler},   
        {LADD_COMMAND, listAddKeyValueHandler},
        {LPOP_COMMAND, listPopKeyValueHandler},   
        {LGET_COMMAND, listGetKeyValueHandler},   
        {LALL_COMMAND, listAllKeyValueHandler},   
        {LOCK_COMMAND, lockKeyValueHandler},      
        {UNLOCK_COMMAND, unlockKeyValueHandler}};

    auto buffer = getRequestBuffer();
    auto cache = getSimpleCache();
    auto session = getSessionManager();

    std::cout << "Server task is started." << std::endl;
    while (true) {
        Request rq = buffer->getRequest();
        if (rq.m_name == EXPIRE_TASK) {
            expireTaskHandler();
            continue;
        }
        if (funcs.find(rq.cmd[0]) == funcs.end()) {
            session->async_send(rq.m_name, 
                WRONG_REQUEST_COMMAND);
            continue;
        }
        std::string (*func)(Request &) = funcs[rq.cmd[0]];
        auto cache = getSimpleCache();
        auto result = func(rq);
        session->async_send(rq.m_name, result);
    }
    std::cout << "Server task is closed." << std::endl;
}

void startExpire() {
    auto buffer = getRequestBuffer();
    auto config = getGlobalConfig();
    std::cout << "Expire task is started." << std::endl;
    while (true) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(config->expireCycle));
        Request rq = Request(); rq.m_name = EXPIRE_TASK;
        buffer->addRequest(rq);
    }
    std::cout << "Expire task is closed." << std::endl;
}
