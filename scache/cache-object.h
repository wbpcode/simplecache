#pragma once

#include <string>

enum CacheType { DictType, ListType, LongType, StringType, LinkType };

class CacheObject {
  private:
    const std::string m_key;
    CacheType m_valueType;

  protected:
    CacheObject(std::string key, CacheType valueType)
        : m_key(key), m_valueType(valueType) {
        ;
    }
    virtual ~CacheObject() { ; }

  public:
    std::string getKey();
    CacheType getValueType();

    friend void destoryInstance(CacheObject *o);
};

class CacheValue : public CacheObject {
  private:
    void *m_value = nullptr;

  protected:
    CacheValue(std::string key, CacheType valueType)
        : CacheObject(key, valueType) {
        ;
    }
    virtual ~CacheValue();

  public:
    template <class T> T &getValue();

    template <class T> void setValue(T value);

    friend CacheObject *getInstance(std::string key, CacheType valueType);
    friend void destoryInstance(CacheObject *o);
};

template <class T> T &CacheValue::getValue() {
    if (!m_value) {
        m_value = new T();
    }
    T &value = *(T *)m_value;
    return value;
}

template <class T> void CacheValue::setValue(T value) {
    if (!m_value)
        m_value = new T(std::move(value));
    else
        *(T *)m_value = value;
}

class CacheContainer : public CacheObject {
  protected:
    CacheContainer(std::string key, CacheType valueType)
        : CacheObject(key, valueType) {
        ;
    }
    virtual ~CacheContainer() { ; }

  public:
    virtual void addKeyValue(CacheObject *o) = 0;
    virtual CacheObject *getKeyValue(std::string key) = 0;
    virtual void delKeyValue(std::string key) = 0;

    friend void destoryInstance(CacheObject *o);
};

CacheObject *getInstance(std::string key, CacheType valueType);
void destoryInstance(CacheObject *o);
