#pragma once

#include <string>

enum CacheType { DictType, ListType, LongType, StringType };


using int64 = long long;

class CacheBase {
private:
    const CacheType m_type;
protected:
    CacheBase(CacheType t) :m_type(t) { ; }
    virtual ~CacheBase() = default;
public:
    CacheType getType();

    friend void delInstance(CacheBase* base);
};


class CacheValue : public CacheBase {
private:
    void *m_value = nullptr;

public:
    CacheValue(CacheType valueType)
        : CacheBase(valueType) { ; }
    virtual ~CacheValue();

    std::string getValue();

    void setValue(std::string value);
};

template<class T> T* getInstance(CacheType type) {
    switch (type) {
    case StringType:
        return (T*)(new CacheValue(StringType));
    case LongType:
        return (T*)(new CacheValue(LongType));
    case ListType:
        return (T*)(new CacheList<CacheValue*>());
    case DictType:
        return (T*)(new CacheDict<std::string,
            CacheValue*>());
    default:
        return nullptr;
    }
}
void delInstance(CacheBase* base);