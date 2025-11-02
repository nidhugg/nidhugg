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

#include "StrModule.h"

#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IRReader/IRReader.h>
#if defined(HAVE_LLVM_IR_LEGACYPASSMANAGER_H) && defined(LLVM_PASSMANAGER_TEMPLATE)
#include <llvm/IR/LegacyPassManager.h>
#endif
#include <llvm/Support/ErrorOr.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/SourceMgr.h>
#if defined(HAVE_LLVM_SUPPORT_SYSTEM_ERROR_H)
#include <llvm/Support/system_error.h>
#endif
#include <llvm/Pass.h>

#include <memory>
#include <stdexcept>

#include "GlobalContext.h"
#include "nregex.h"

namespace StrModule {

  llvm::Module *read_module(std::string infile, llvm::LLVMContext &context){
    llvm::Module *mod;
    llvm::SMDiagnostic err;
    llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer> > buf =
      llvm::MemoryBuffer::getFile(infile);
    if(std::error_code ec = buf.getError()){
      throw std::logic_error("Failed to read file "+infile+": "+ec.message());
    }
    llvm::MemoryBuffer *mbp = buf.get().release();
#ifdef LLVM_PARSE_IR_MEMBUF_PTR
    mod = llvm::ParseIR(mbp,err,context);
#else
    mod = llvm::parseIR(mbp->getMemBufferRef(),err,context).release();
#endif
#ifndef LLVM_PARSE_IR_TAKES_OWNERSHIP
    delete mbp;
#endif
    if(!mod){
      err.print("",llvm::errs());
      throw std::logic_error("Failed to parse assembly.");
    }
    return mod;
  }

  llvm::Module *read_module_src(const std::string &src, llvm::LLVMContext &context){
    llvm::Module *mod;
    llvm::SMDiagnostic err;
    llvm::MemoryBuffer *buf =
#ifdef LLVM_GETMEMBUFFER_RET_PTR
      llvm::MemoryBuffer::getMemBuffer(src,"",false);
#else
      llvm::MemoryBuffer::getMemBuffer(src,"",false).release();
#endif
#ifdef LLVM_PARSE_IR_MEMBUF_PTR
    mod = llvm::ParseIR(buf,err,context);
#else
    mod = llvm::parseIR(buf->getMemBufferRef(),err,context).release();
#endif
#ifndef LLVM_PARSE_IR_TAKES_OWNERSHIP
    delete buf;
#endif
    if(!mod){
      err.print("",llvm::errs());
      throw std::logic_error("Failed to parse assembly.");
    }
    return mod;
  }

  void write_module(llvm::Module *mod, std::string outfile){
#ifdef LLVM_PASSMANAGER_TEMPLATE
    llvm::legacy::PassManager PM;
#else
    llvm::PassManager PM;
#endif
#ifdef LLVM_RAW_FD_OSTREAM_ERR_STR
    std::string errs;
#else
    std::error_code errs;
#endif
    llvm::raw_ostream *os = new llvm::raw_fd_ostream(outfile.c_str(), errs,
                                                     llvm::sys::fs::OF_None);
#ifdef LLVM_RAW_FD_OSTREAM_ERR_STR
    if(errs.size()){
      delete os;
      throw std::logic_error("Failed to write transformed module to file "+outfile+": "+errs);
    }
#else
    if(errs){
      delete os;
      throw std::logic_error("Failed to write transformed module to file "+outfile+": "+errs.message());
    }
#endif
    PM.add(llvm::createPrintModulePass(*os));
    PM.run(*mod);
  }

  std::string write_module_str(llvm::Module *mod){
    std::string s;
#ifdef LLVM_PASSMANAGER_TEMPLATE
    llvm::legacy::PassManager PM;
#else
    llvm::PassManager PM;
#endif
    llvm::raw_ostream *os = new llvm::raw_string_ostream(s);
    PM.add(llvm::createPrintModulePass(*os));
    PM.run(*mod);
    delete os;
    return s;
  }

  std::string portasm(std::string s){
#ifndef LLVM_ASM_LOAD_EXPLICIT_TYPE
    {
      /* Remove explicit return types for loads: Replace loads of the
       * form
       *
       * load rettype, rettype* addr
       *
       * with loads of the form
       *
       * load rettype* addr
       */
      s = nregex::regex_replace(s,"load *((atomic)?) [^,]*,","load $1 ");
    }
    {
      /* Remove explicit return types for getelementptr.*/
      s = nregex::regex_replace(s,"getelementptr *((inbounds)?) *[^,(]*,","getelementptr $1 ");
      s = nregex::regex_replace(s,"getelementptr *\\([^,]*,","getelementptr (");
    }
#endif
    return s;
  }
}  // namespace StrModule

