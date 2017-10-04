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

#include "Configuration.h"
#include "DPORDriver.h"
#include "GlobalContext.h"
#include "Transform.h"

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/ManagedStatic.h>

#include <iostream>
#include <set>
#include <stdexcept>

llvm::cl::opt<std::string>
cl_transform("transform",llvm::cl::init(""),
             llvm::cl::desc("Transform the input module and store it (as LLVM assembly) to OUTFILE."),
             llvm::cl::NotHidden,llvm::cl::value_desc("OUTFILE"));

llvm::cl::opt<std::string>
cl_input_file(llvm::cl::desc("<input bitcode or assembly>"),
              llvm::cl::Positional,
              llvm::cl::init("-"));

llvm::cl::list<std::string>
cl_program_arguments(llvm::cl::desc("[-- <program arguments>...]"),
                     llvm::cl::Positional,
                     llvm::cl::ZeroOrMore);

void print_version(){
  std::cout << PACKAGE_STRING
            << " ("
#ifdef GIT_COMMIT
            << GIT_COMMIT << ", "
#endif
#ifndef NDEBUG
            << "Debug"
#else
            << "Release"
#endif
            << ", with LLVM-" << LLVM_VERSION << ":" << LLVM_BUILDMODE << ")\n";
}

// Normal exit code
#define EXIT_OK 0
// Exit code when verification failed (an error was detected)
#define VERIFICATION_FAILURE 42

int main(int argc, char *argv[]){
  /* Command line options */
  llvm::cl::SetVersionPrinter(print_version);
  {
    /* Hide all options defined by the LLVM library, except the ones
     * approved by Configuration.
     */
    std::set<std::string> visible_options =
      {"version"};
    visible_options.insert(Configuration::commandline_opts().begin(),
                           Configuration::commandline_opts().end());
#ifdef LLVM_CL_GETREGISTEREDOPTIONS_TAKES_ARGUMENT
    llvm::StringMap<llvm::cl::Option*> opts;
    llvm::cl::getRegisteredOptions(opts);
#else
    llvm::StringMap<llvm::cl::Option*> &opts =
      llvm::cl::getRegisteredOptions();
#endif
    for(auto it = opts.begin(); it != opts.end(); ++it){
      if(visible_options.count(it->getKey()) == 0){
        it->getValue()->setHiddenFlag(llvm::cl::Hidden);
      }
    }
  }
  llvm::cl::ParseCommandLineOptions(argc, argv);

  bool errors_detected = false;
  try{
    Configuration conf;
    conf.assign_by_commandline();
    conf.check_commandline();

    if(cl_transform != ""){
      Transform::transform(cl_input_file,cl_transform,conf);
    }else{
      /* Use DPORDriver to explore the given module */
      DPORDriver *driver =
        DPORDriver::parseIRFile(cl_input_file,conf);

      DPORDriver::Result res = driver->run();
      std::cout << "Trace count: " << res.trace_count
                << " (also " << res.sleepset_blocked_trace_count
                << " sleepset blocked)" << std::endl;
      if(res.has_errors()){
        errors_detected = true;
        std::cout << "\n Error detected:\n"
                  << res.error_trace->to_string(2);
      }else{
        std::cout << "No errors were detected." << std::endl;
      }

      delete driver;
    }
    GlobalContext::destroy();
    llvm::llvm_shutdown();
  }catch(std::exception *exc){
    std::cerr << "Error: " << exc->what() << "\n";
    GlobalContext::destroy();
    llvm::llvm_shutdown();
    return 1;
  }catch(std::exception &exc){
    std::cerr << "Error: " << exc.what() << "\n";
    GlobalContext::destroy();
    llvm::llvm_shutdown();
    return 1;
  }

  return (errors_detected ? VERIFICATION_FAILURE : EXIT_OK);
}
