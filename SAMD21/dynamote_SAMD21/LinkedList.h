#ifndef LinkedList_h
#define LinkedList_h

#include "Arduino.h"
#include "IRLibGlobals.h"

#define ARRAY_SIZE  RECV_BUF_LENGTH

class LinkedList 
{
  private:
    uint16_t linkedList[ARRAY_SIZE];
    unsigned int index = 0;

  public:
    LinkedList();
    void clear();
    void add(uint16_t value);
    uint16_t get(int indexValue);
    void removeFirst();
    int size();
    uint16_t* toArray();
};

#endif
