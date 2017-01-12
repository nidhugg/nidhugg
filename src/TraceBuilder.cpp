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

#include "TraceBuilder.h"

TraceBuilder::TraceBuilder(const Configuration &C) : conf(C) {
}

TraceBuilder::~TraceBuilder(){
  for(unsigned i = 0; i < errors.size(); ++i){
    delete errors[i];
  }
}

void TraceBuilder::assertion_error(std::string cond, const IID<CPid> &loc){
  if(loc.is_null()){
    errors.push_back(new AssertionError(get_iid(),cond));
  }else{
    errors.push_back(new AssertionError(loc,cond));
  }
  if(conf.debug_print_on_error) debug_print();
}

void TraceBuilder::pthreads_error(std::string msg, const IID<CPid> &loc){
  if(loc.is_null()){
    errors.push_back(new PthreadsError(get_iid(),msg));
  }else{
    errors.push_back(new PthreadsError(loc,msg));
  }
  if(conf.debug_print_on_error) debug_print();
}

void TraceBuilder::segmentation_fault_error(const IID<CPid> &loc){
  if(loc.is_null()){
    errors.push_back(new SegmentationFaultError(get_iid()));
  }else{
    errors.push_back(new SegmentationFaultError(loc));
  }
  if(conf.debug_print_on_error) debug_print();
}

void TraceBuilder::memory_error(std::string msg, const IID<CPid> &loc){
  if(loc.is_null()){
    errors.push_back(new MemoryError(get_iid(),msg));
  }else{
    errors.push_back(new MemoryError(loc,msg));
  }
  if(conf.debug_print_on_error) debug_print();
}
