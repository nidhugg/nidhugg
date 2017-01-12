/* Copyright (C) 2014-2017 Carl Leonardsson
 *
 * This file is part of Nidhugg.
 *
 * Nidhugg is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nidhugg is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "MRef.h"

#include <cstdint>
#include <cstdlib>

MBlock::MBlock(const MRef &r, int alloc_size) : ref(r) {
  block = malloc(alloc_size);
  ptr_counter = new int(1);
  *ptr_counter = 1;
}

MBlock::MBlock(const MRef &r) : ref(r), ptr_counter(0) {
  block = r.ref;
}

MBlock::MBlock(const MBlock &B)
  : ref(B.ref), block(B.block), ptr_counter(B.ptr_counter){
  if(ptr_counter){
    ++*ptr_counter;
  }
}

MBlock &MBlock::operator=(const MBlock &B){
  if(this != &B){
    if(B.ptr_counter) ++*B.ptr_counter;
    if(ptr_counter){
      --*ptr_counter;
      if(*ptr_counter == 0){
        free(block);
        delete ptr_counter;
      }
    }
    ref = B.ref;
    block = B.block;
    ptr_counter = B.ptr_counter;
  }
  return *this;
}

MBlock::~MBlock(){
  if(ptr_counter){
    --*ptr_counter;
    if(*ptr_counter == 0){
      free(block);
      delete ptr_counter;
    }
  }
}

bool MRef::subsetof(const MRef &mr) const{
  return mr.ref <= ref && (uint8_t*)ref+size <= (uint8_t*)mr.ref+mr.size;
}

bool MRef::subsetof(const ConstMRef &mr) const{
  return mr.ref <= ref && (uint8_t*)ref+size <= (uint8_t const *)mr.ref+mr.size;
}

bool MRef::overlaps(const MRef &mr) const{
  uint8_t *a = (uint8_t*)ref, *b = (uint8_t*)mr.ref;
  return a < b+mr.size && b < a+size;
}

bool MRef::overlaps(const ConstMRef &mr) const{
  uint8_t const *a = (uint8_t const *)ref, *b = (uint8_t const *)mr.ref;
  return a < b+mr.size && b < a+size;
}

bool ConstMRef::subsetof(const MRef &mr) const{
  return mr.ref <= ref && (uint8_t const *)ref+size <= (uint8_t*)mr.ref+mr.size;
}

bool ConstMRef::subsetof(const ConstMRef &mr) const{
  return mr.ref <= ref && (uint8_t const *)ref+size <= (uint8_t const *)mr.ref+mr.size;
}

bool ConstMRef::overlaps(const MRef &mr) const{
  uint8_t const *a = (uint8_t const *)ref, *b = (uint8_t const *)mr.ref;
  return a < b+mr.size && b < a+size;
}

bool ConstMRef::overlaps(const ConstMRef &mr) const{
  uint8_t const *a = (uint8_t const *)ref, *b = (uint8_t const *)mr.ref;
  return a < b+mr.size && b < a+size;
}
