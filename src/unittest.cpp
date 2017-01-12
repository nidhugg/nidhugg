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

#include "GlobalContext.h"

#ifdef HAVE_BOOST_UNIT_TEST_FRAMEWORK

#include <llvm/Support/ManagedStatic.h>

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE DPOR
#include <boost/test/unit_test.hpp>

struct Fixture{
  Fixture(){
    // Global initialization
  };
  ~Fixture(){
    // Global destruction
    GlobalContext::destroy();
    llvm::llvm_shutdown();
  };
};

BOOST_GLOBAL_FIXTURE(Fixture);

BOOST_AUTO_TEST_CASE(Testing_testing){
  BOOST_CHECK_MESSAGE(true,"Testing testing.");
}

/* Solves a problem with compiling with clang++ against certain
 * versions of the boost library.
 */
namespace boost{ namespace unit_test { namespace ut_detail{
      std::string normalize_test_case_name(const_string name) {
        return name[0] == '&' ? std::string(name.begin()+1, name.size()-1) : std::string(name.begin(), name.size());
      }
    }
  }
}

#else

#include <iostream>

int main(int argc, char *argv[]){
  std::cerr << "\n\nERROR: Compiled without BOOST unit test.\n\n\n";
  return 1;
}

#endif
