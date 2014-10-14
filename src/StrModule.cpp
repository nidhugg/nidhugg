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

#include "StrModule.h"

#include <llvm/Assembly/PrintModulePass.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/PassManager.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/system_error.h>

namespace StrModule {

  llvm::Module *read_module(std::string infile){
    llvm::Module *mod;
    llvm::SMDiagnostic err;
    llvm::OwningPtr<llvm::MemoryBuffer> buf;
    if(llvm::error_code ec = llvm::MemoryBuffer::getFile(infile,buf)){
      throw std::logic_error("Failed to read file "+infile+": "+ec.message());
    }
    mod = llvm::ParseIR(buf.take(),err,llvm::getGlobalContext());
    if(!mod){
      err.print("",llvm::errs());
      throw std::logic_error("Failed to parse assembly.");
    }
    return mod;
  };

  llvm::Module *read_module_src(const std::string &src){
    llvm::Module *mod;
    llvm::SMDiagnostic err;
    llvm::MemoryBuffer *buf =
      llvm::MemoryBuffer::getMemBuffer(src,"",false);
    mod = llvm::ParseIR(buf,err,llvm::getGlobalContext());
    if(!mod){
      err.print("",llvm::errs());
      throw std::logic_error("Failed to parse assembly.");
    }
    return mod;
  };

  void write_module(llvm::Module *mod, std::string outfile){
    llvm::PassManager PM;
    std::string errs;
    llvm::raw_ostream *os = new llvm::raw_fd_ostream(outfile.c_str(),errs);
    if(errs.size()){
      delete os;
      throw std::logic_error("Failed to write transformed module to file "+outfile+": "+errs);
    }
    PM.add(llvm::createPrintModulePass(os,true));
    PM.run(*mod);
  };

  std::string write_module_str(llvm::Module *mod){
    std::string s;
    llvm::PassManager PM;
    llvm::raw_ostream *os = new llvm::raw_string_ostream(s);
    PM.add(llvm::createPrintModulePass(os,true));
    PM.run(*mod);
    return s;
  };

};

