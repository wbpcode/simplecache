#include "cache-object.h"
#include "cache-dict.h"
#include "cache-list.h"

std::string CacheObject::getKey() { return m_key; }

CacheType CacheObject::getValueType() { return m_valueType; }

CacheValue::~CacheValue() {
    switch (getValueType()) {
    case StringType:
        delete (std::string *)m_value;
        return;
    case LongType:
        delete (long long *)m_value;
        return;
    }
}

CacheObject *getInstance(std::string key, CacheType valueType) {
    switch (valueType) {
    case LongType:
        return new CacheValue(key,valueType);
    case StringType:
        return new CacheValue(key,valueType);
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

void delInstance(CacheObject *o) {
    if (o)
        delete o;
}