#include "LinkedList.h"

/*
 * I would prefer to use a linked list library rather than this class.
 * Some libraries exist but they don't yet seem compatible with the Nano 33 iot
 */

LinkedList::LinkedList() {}

void LinkedList::clear() 
{
  index = 0;
  linkedList[0] = 0;
}

void LinkedList::add(uint16_t value) 
{
  linkedList[index] = value;
  index++;
}

uint16_t LinkedList::get(int indexValue) 
{
  return linkedList[indexValue];
}

void LinkedList::removeFirst() 
{
  for (int x = 0; x < index-1; x++) {
    linkedList[x] = linkedList[x+1];
  }
  index--;
}

int LinkedList::size()
{
  return index;
}

uint16_t* LinkedList::toArray() 
{
  uint16_t linkedListArray[index];
  for (int x = 0; x < index; x++) {
    linkedListArray[x] = linkedList[x];
  }
}
