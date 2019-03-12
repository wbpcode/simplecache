#pragma once
#include "cache-object.h"

class CacheListNode {
  private:
    CacheListNode *m_next;
    CacheListNode *m_prev;
    CacheObject *m_value;

  public:
    void setNext(CacheListNode *next);
    CacheListNode *getNext();

    void setPrev(CacheListNode *prev);
    CacheListNode *getPrev();

    void setValue(CacheObject *value);
    CacheObject *getValue();

    CacheListNode() = default;
    CacheListNode(CacheObject *value) : m_value(value) { ; }

    void destoryValue();
};

class CacheList : public CacheObject {
  private:
    int m_size;
    CacheListNode *m_head;
    CacheListNode *m_tail;

  protected:
    CacheList(std::string key, CacheType valueType = ListType);
    virtual ~CacheList();

  public:
    virtual void addKeyValue(std::string key, void *value, CacheType valueType);
    virtual CacheObject *getKeyValue(std::string key);
    virtual void delKeyValue(std::string key);

    CacheListNode *addNode(CacheListNode *node, CacheListNode *pos = nullptr);
    CacheListNode *popNode(CacheListNode *node = nullptr);

    CacheListNode *serachByKey(std::string key);

    CacheListNode *getHead();
    CacheListNode *getTail();
    int getSize();

    friend CacheObject *getInstance(std::string key, void *value,
                                    CacheType valueType);
    friend void destoryInstance(CacheObject *o);
};