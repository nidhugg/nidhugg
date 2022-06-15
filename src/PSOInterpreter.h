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

#include <config.h>
#ifndef __PSO_INTERPRETER_H__
#define __PSO_INTERPRETER_H__

#include "Interpreter.h"
#include "PSOTraceBuilder.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

/* A PSOInterpreter is an interpreter running under the PSO
 * semantics. The execution should be guided by scheduling from a
 * PSOTraceBuilder.
 */
class PSOInterpreter : public llvm::Interpreter{
public:
  explicit PSOInterpreter(llvm::Module *M, PSOTraceBuilder &TB,
                          const Configuration &conf = Configuration::default_conf);
  virtual ~PSOInterpreter();

  static std::unique_ptr<PSOInterpreter>
  create(llvm::Module *M, PSOTraceBuilder &TB,
         const Configuration &conf = Configuration::default_conf,
         std::string *ErrorStr = 0);

  virtual void visitLoadInst(llvm::LoadInst &I);
  virtual void visitStoreInst(llvm::StoreInst &I);
  virtual void visitFenceInst(llvm::FenceInst &I);
  virtual void visitAtomicCmpXchgInst(llvm::AtomicCmpXchgInst &I);
  virtual void visitAtomicRMWInst(llvm::AtomicRMWInst &I);
  virtual void visitInlineAsm(llvm::CallInst &CI, const std::string &asmstr);

protected:
  /* The auxiliary threads under PSO are numbered 0, 1, ... . Each
   * auxiliary thread correspond to the store buffer for memory
   * locations starting with one certain byte in memory. The auxiliary
   * threads are numbered in the order in which the first store to
   * each such memory location occurs for that real thread.
   */
  virtual void runAux(int proc, int aux);
  virtual int newThread(const CPid &cpid);
  virtual bool isFence(llvm::Instruction &I);
  virtual bool checkRefuse(llvm::Instruction &I);
  virtual void terminate(llvm::Type *RetTy, llvm::GenericValue Result);

  /* A PendingStoreByte represents one byte in a pending entry in a
   * store buffer.
   */
  class PendingStoreByte{
  public:
    PendingStoreByte(const SymAddrSize &ml, uint8_t val) : ml(ml), val(val) {}
    /* ml is the complete memory location of the pending store of
     * which this byte is part.
     */
    SymAddrSize ml;
    /* val is the value of this byte in the pending store. */
    uint8_t val;
  };

  /* Additional information for a thread, supplementing the
   * information in Interpreter::Thread.
   */
  class PSOThread{
  public:
    PSOThread() : awaiting_buffer_flush(BFL_NO), buffer_flush_ml({SymMBlock::Global(0),0},1) {}
    /* aux_to_byte and byte_to_aux provide the mapping between
     * auxiliary thread indices and the first byte in the memory
     * locations for which that auxiliary thread is responsible.
     */
    std::vector<SymAddr> aux_to_byte;
    std::vector<uint8_t*> aux_to_addr;
    std::map<SymAddr,int> byte_to_aux;
    /* Each pending store s is split into PendingStoreBytes (see
     * above) and ordered into store buffers. For each byte b in
     * memory, store_buffers[b] contains precisely all
     * PendingStoreBytes of pending stores to that byte. The entries
     * in store_buffers[b] are ordered such that newer entries are
     * further to the back.
     */
    std::map<SymAddr,std::vector<PendingStoreByte> > store_buffers;
    /* awaiting_buffer_flush indicates whether this thread is blocked
     * waiting for some store buffer to flush.
     */
    enum BufferFlush {
      /* The thread does not wait for any buffer flush. */
      BFL_NO,
      /* The thread waits for a complete flush of all buffers. */
      BFL_FULL,
      /* The thread waits for a (partial) flush of buffers such that a
       * load of the memory location buffer_flush_ml is possible
       * (either from memory or by ROWE). I.e., for each byte b in
       * buffer_flush_ml it should be the case that either
       * store_buffers[b] is empty, or the latest entry in
       * store_buffers[b] is a store to precisely the memory location
       * buffer_flush_ml.
       */
      BFL_PARTIAL
    } awaiting_buffer_flush;
    /* Indicates which buffers need to be flushed. (See BFL_PARTIAL
     * above.) If awaiting_buffer_flush != BFL_PARTIAL, then the value
     * of buffer_flush_ml is undefined.
     */
    SymAddrSize buffer_flush_ml;

    /* Check if ml is readable, as described above for BFL_PARTIAL. */
    bool readable(const SymAddrSize &ml) const;
    bool all_buffers_empty() const{
#ifndef NDEBUG
      /* Empty buffers should be removed from store_buffers. */
      for(auto it = store_buffers.begin(); it != store_buffers.end(); ++it){
        assert(it->second.size());
      }
#endif
      return store_buffers.empty();
    }
  };
  /* All threads that are or have been running during this execution
   * have an entry in Threads, in the order in which they were
   * created.
   */
  std::vector<PSOThread> pso_threads;
};

#endif

