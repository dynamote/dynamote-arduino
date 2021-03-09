/******************************************************************************
 * Copyright (C) 2021 Darcy Huisman
 * This program is free software: you can redistribute it and/or modify it 
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along 
 * with this program. If not, see <https://www.gnu.org/licenses/>.
 *****************************************************************************/

#ifndef LinkedList_h
#define LinkedList_h

#include "Arduino.h"
#include "IRLibGlobals.h"

#define ARRAY_SIZE  RECV_BUF_LENGTH

class DynamoteLinkedList 
{
  private:
    uint16_t linkedList[ARRAY_SIZE];
    unsigned int index = 0;

  public:
    DynamoteLinkedList();
    void clear();
    void add(uint16_t value);
    uint16_t get(int indexValue);
    void removeFirst();
    int size();
    uint16_t* toArray();
};

#endif
