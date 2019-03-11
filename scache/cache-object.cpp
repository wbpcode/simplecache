#include"cache-object.h"
#include"cache-list.h"
#include"cache-dict.h"

std::string CacheObject::getKey() {
    return m_key;
}

CacheType CacheObject::getValueType() {
    return m_valueType;
}

// 基类CacheObject相关接口不做任何工作
void* CacheObject::getValue() {
    return nullptr;
}
void CacheObject::setValue(void* value) {
    return;
}

void CacheObject::addKeyValue(std::string key, void *value, CacheType valueType) {
    return;
}

CacheObject* CacheObject::getKeyValue(std::string key) {
    return nullptr;
}

void CacheObject::delKeyValue(std::string key) {
    return;
}

void CacheLong::setValue(void* value) {
    m_value = *(long long*)value;
}

void* CacheLong::getValue() {
    return &m_value;
}

void CacheString::setValue(void* value) {
    m_value = *(std::string*)value;
}

void* CacheString::getValue() {
    return &m_value;
}

CacheObject* getInstance(std::string key, void *value, CacheType valueType) {
    switch (valueType) {
    case LongType:
        return new CacheLong(key, value, valueType);
    case StringType:
        return new CacheString(key, value, valueType);
    case ListType:
        return new CacheList(key, valueType);
    case DictType:
        return new CacheDict(key, valueType);
    case LinkType:
        return new LinkedDict(key, valueType);
    default:
        break;
    }
    return nullptr;
}

void destoryInstance(CacheObject* o) {
    delete o;
}