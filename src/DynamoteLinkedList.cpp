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

#include "DynamoteLinkedList.h"

DynamoteLinkedList::DynamoteLinkedList() {}

void DynamoteLinkedList::clear() 
{
  index = 0;
  linkedList[0] = 0;
}

void DynamoteLinkedList::add(uint16_t value) 
{
  linkedList[index] = value;
  index++;
}

uint16_t DynamoteLinkedList::get(int indexValue) 
{
  return linkedList[indexValue];
}

void DynamoteLinkedList::removeFirst() 
{
  for (int x = 0; x < index-1; x++) {
    linkedList[x] = linkedList[x+1];
  }
  index--;
}

int DynamoteLinkedList::size()
{
  return index;
}

uint16_t* DynamoteLinkedList::toArray() 
{
  uint16_t linkedListArray[index];
  for (int x = 0; x < index; x++) {
    linkedListArray[x] = linkedList[x];
  }
}
