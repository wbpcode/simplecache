#pragma once

#include<string>

enum CacheType {
    DictType,
    ListType,
    LongType,
    StringType,
    LinkType
};

class CacheObject {
private:
    const std::string m_key;
    CacheType m_valueType;

protected:
    CacheObject(std::string key, CacheType valueType) 
        :m_key(key), m_valueType(valueType) { ; }
    virtual ~CacheObject() { ; }
public:
    std::string getKey();
    CacheType getValueType();

    // 基本类型接口
    virtual void* getValue();
    virtual void setValue(void* value);
    // 容器类型接口
    virtual void addKeyValue(std::string key, void *value, CacheType valueType);
    virtual CacheObject* getKeyValue(std::string key);
    virtual void delKeyValue(std::string key);

    friend void destoryInstance(CacheObject* o);
};

class CacheLong :public CacheObject {
private:
    long long m_value;

protected:
    CacheLong(std::string key, void* value, CacheType valueType = LongType) :
        CacheObject(key, valueType), m_value(*(long long*)value) {
        ;
    }
    virtual ~CacheLong() { ; }

public:
    virtual void setValue(void* value);
    virtual void* getValue();

    friend CacheObject* getInstance(std::string key, void *value, CacheType valueType);
    friend void destoryInstance(CacheObject* o);
};

class CacheString :public CacheObject {
private:
    std::string m_value;

protected:
    CacheString(std::string key, void* value, CacheType valueType = StringType) :
        CacheObject(key, valueType), m_value(*(std::string*)value) {
        ;
    }
    virtual ~CacheString() { ; }

public:
    virtual void setValue(void* value);
    virtual void* getValue();

    friend CacheObject* getInstance(std::string key, void *value, CacheType valueType);
    friend void destoryInstance(CacheObject* o);
};


CacheObject* getInstance(std::string key, void *value, CacheType valueType);

void destoryInstance(CacheObject* o);
