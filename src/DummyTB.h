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

#include <config.h>

#ifndef __DUMMYTB_H__
#define __DUMMYTB_H__

#include "Configuration.h"
#include "MRef.h"
#include "Trace.h"
#include "TraceBuilder.h"

#include <string>
#include <vector>

class DummyTB : public TraceBuilder{
public:
  DummyTB(const Configuration &conf = Configuration::default_conf);
  virtual ~DummyTB() {};
  virtual bool schedule(int *proc, int *aux, int *alt, bool *dryrun);
  virtual void refuse_schedule();
  virtual void mark_available(int proc, int aux = -1){
    if(aux < 0) available[proc] = true;
  };
  virtual void mark_unavailable(int proc, int aux = -1){
    if(aux < 0) available[proc] = false;
  };
  virtual void metadata(const llvm::MDNode *md){};
  virtual bool sleepset_is_empty() const{ return true; };
  virtual bool check_for_cycles(){ return false; };
  virtual bool has_error() const { return errors.size(); };
  virtual Trace get_trace() const;
  virtual bool reset(){ return false; };

  virtual void debug_print() const {};

  virtual void spawn();
  virtual void store(const ConstMRef &ml){};
  virtual void atomic_store(const ConstMRef &ml){};
  virtual void load(const ConstMRef &ml){};
  virtual void full_memory_conflict(){};
  virtual void fence(){};
  virtual void join(int tgt_proc){};
  virtual void mutex_lock(const ConstMRef &ml){};
  virtual void mutex_lock_fail(const ConstMRef &ml){};
  virtual void mutex_unlock(const ConstMRef &ml){};
  virtual void mutex_init(const ConstMRef &ml){};
  virtual void mutex_destroy(const ConstMRef &ml){};
  virtual void register_alternatives(int alt_count){};
  virtual void dealloc(const ConstMRef &ml){};
  virtual void assertion_error(std::string cond);
  virtual void pthreads_error(std::string msg);
  virtual void segmentation_fault_error();
protected:
  std::vector<IID<CPid> > cmp;
  std::vector<CPid> cpids;
  CPidSystem CPS;
  std::vector<bool> available;
  std::vector<int> pcs;
  int last_proc;
};

#endif
