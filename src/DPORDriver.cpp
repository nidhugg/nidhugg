/* Copyright (C) 2014 Carl Leonardsson
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

#include "DPORDriver.h"

#include "CheckModule.h"
#include "Debug.h"

#include <fstream>
#include <stdexcept>

#ifdef LLVM_INCLUDE_IR
#include <llvm/IR/LLVMContext.h>
#else
#include <llvm/IR/LLVMContext.h>
#endif
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>

DPORDriver::DPORDriver(const Configuration &C) :
  conf(C), mod(0) {
  std::string ErrorMsg;
  llvm::sys::DynamicLibrary::LoadLibraryPermanently(0,&ErrorMsg);
};

DPORDriver *DPORDriver::parseIRFile(const std::string &filename,
                                    const Configuration &C){
  DPORDriver *driver =
    new DPORDriver(C);
  read_file(filename,driver->src);
  driver->reparse();
  CheckModule::check_functions(driver->mod);
  return driver;
};

DPORDriver *DPORDriver::parseIR(const std::string &llvm_asm,
                                const Configuration &C){
  DPORDriver *driver =
    new DPORDriver(C);
  driver->src = llvm_asm;
  driver->reparse();
  CheckModule::check_functions(driver->mod);
  return driver;
};

DPORDriver::~DPORDriver(){
  delete mod;
};

void DPORDriver::read_file(const std::string &filename, std::string &tgt){
  std::ifstream is(filename);
  if(!is){
    throw std::logic_error("Failed to read assembly file.");
  }
  is.seekg(0,std::ios::end);
  tgt.resize(is.tellg());
  is.seekg(0,std::ios::beg);
  is.read(&tgt[0],tgt.size());
  is.close();
};

void DPORDriver::reparse(){
  delete mod;
  llvm::SMDiagnostic err;
  llvm::MemoryBuffer *buf =
    llvm::MemoryBuffer::getMemBuffer(src,"",false);
  mod = llvm::ParseIR(buf,err,llvm::getGlobalContext());
  if(!mod){
    err.print("",llvm::errs());
    throw std::logic_error("Failed to parse assembly.");
  }
  if(mod->getDataLayout().empty()){
    if(llvm::sys::IsLittleEndianHost){
      mod->setDataLayout("e");
    }else{
      mod->setDataLayout("E");
    }
  }
};

DPORDriver::Result DPORDriver::run(){
  Debug::warn("DPORDriver::run")
    << "WARNING: DPORDriver::run: Not implemented.\n";
  Result res;
  return res;
};
