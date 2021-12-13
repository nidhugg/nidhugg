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

#include "GlobalContext.h"
#include "nregex.h"
#include "StrModule.h"

#if defined(HAVE_LLVM_ASSEMBLY_PRINTMODULEPASS_H)
#include <llvm/Assembly/PrintModulePass.h>
#elif defined(HAVE_LLVM_IR_IRPRINTINGPASSES_H)
#include <llvm/IR/IRPrintingPasses.h>
#endif
#include <llvm/IRReader/IRReader.h>
#if defined(HAVE_LLVM_PASSMANAGER_H)
#include <llvm/PassManager.h>
#elif defined(HAVE_LLVM_IR_PASSMANAGER_H)
#include <llvm/IR/PassManager.h>
#endif
#if defined(HAVE_LLVM_IR_LEGACYPASSMANAGER_H) && defined(LLVM_PASSMANAGER_TEMPLATE)
#include <llvm/IR/LegacyPassManager.h>
#endif
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/SourceMgr.h>
#if defined(HAVE_LLVM_SUPPORT_SYSTEM_ERROR_H)
#include <llvm/Support/system_error.h>
#endif
#if defined(HAVE_LLVM_SUPPORT_ERROROR_H)
#include <llvm/Support/ErrorOr.h>
#endif

#include <stdexcept>

namespace StrModule {

  llvm::Module *read_module(std::string infile, llvm::LLVMContext &context){
    llvm::Module *mod;
    llvm::SMDiagnostic err;
#ifdef LLVM_MEMORY_BUFFER_GET_FILE_OWNINGPTR_ARG
    llvm::OwningPtr<llvm::MemoryBuffer> buf;
    if(llvm::error_code ec = llvm::MemoryBuffer::getFile(infile,buf)){
      throw std::logic_error("Failed to read file "+infile+": "+ec.message());
    }
    llvm::MemoryBuffer *mbp = buf.take();
#else
    llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer> > buf =
      llvm::MemoryBuffer::getFile(infile);
    if(std::error_code ec = buf.getError()){
      throw std::logic_error("Failed to read file "+infile+": "+ec.message());
    }
    llvm::MemoryBuffer *mbp = buf.get().release();
#endif
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
#ifdef HAVE_LLVM_SYS_FS_OPENFLAGS
    llvm::raw_ostream *os = new llvm::raw_fd_ostream(outfile.c_str(),errs,
#  ifdef LLVM_SYS_FS_OPENFLAGS_PREFIX_OF
                                                     llvm::sys::fs::OF_None);
#  else
                                                     llvm::sys::fs::F_None);
#  endif
#else
    llvm::raw_ostream *os = new llvm::raw_fd_ostream(outfile.c_str(),errs,0);
#endif
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
#ifdef LLVM_CREATE_PRINT_MODULE_PASS_PTR_ARG
    PM.add(llvm::createPrintModulePass(os,true));
#else
    PM.add(llvm::createPrintModulePass(*os));
#endif
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
#ifdef LLVM_CREATE_PRINT_MODULE_PASS_PTR_ARG
    PM.add(llvm::createPrintModulePass(os,true));
#else
    PM.add(llvm::createPrintModulePass(*os));
#endif
    PM.run(*mod);
#ifndef LLVM_CREATE_PRINT_MODULE_PASS_PTR_ARG
    delete os;
#endif
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
}

