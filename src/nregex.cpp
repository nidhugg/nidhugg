/* Copyright (C) 2016-2017 Carl Leonardsson
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

#include <sstream>
#include <stdexcept>
#include <regex.h>

#include "nregex.h"

namespace nregex{

  /* Helper for regex_replace.
   *
   * Returns format with all format strings ($n) replaced properly based
   * on pmatch, nmatch, source and source_offset.
   */
  static std::string regex_replace_format(std::string format, regmatch_t *pmatch, int nmatch,
                                          std::string source, int source_offset){
    std::string res = "";
    bool substituted = false;
    std::size_t i = 0;
    std::size_t start = 0;
    while((i = format.find("$",i)) != std::string::npos){
      int j = 1;
      while(i+j < format.size() && '0' <= format[i+j] && format[i+j] <= '9') ++j;
      if(1 < j){
        std::stringstream ss(format.substr(i+1,j-1));
        int n;
        ss >> n;
        if(n < nmatch && 0 <= pmatch[n].rm_so){
          res += format.substr(start,i-start);
          res += source.substr(source_offset+pmatch[n].rm_so,pmatch[n].rm_eo-pmatch[n].rm_so);
          start = i+j;
          substituted = true;
        }
      }
      ++i;
    }
    if(substituted){
      return res+format.substr(start);
    }else{
      return format;
    }
  }

  std::string regex_replace(std::string tgt, std::string regex, std::string format){
    regex_t preg;
    int errcode;
    if((errcode = regcomp(&preg,regex.c_str(),REG_EXTENDED)) != 0){
      char errbuf[512];
      regerror(errcode,&preg,&errbuf[0],512);
      throw std::logic_error(std::string("regex_replace: Failed to compile regex: ")+errbuf);
    }
    int nmatch = 256;
    regmatch_t *pmatch = new regmatch_t[nmatch];
    unsigned start_off = 0;
    std::string result = "";
    bool ok_match_empty = true;
    while(start_off <= tgt.size()){
      if(regexec(&preg,tgt.c_str()+start_off,nmatch,pmatch,0) == 0){ // Match
        if(pmatch[0].rm_eo == 0 && !ok_match_empty){ // Matched the empty string
          // Make sure we move forward and only match the same empty string once.
          if(start_off < tgt.size()) result += tgt[start_off];
          ++start_off;
          ok_match_empty = true; // OK to match the empty string on the next position
        }else{
          result += tgt.substr(start_off,pmatch[0].rm_so) + regex_replace_format(format,&pmatch[0],nmatch,tgt,start_off);
          start_off += pmatch[0].rm_eo;
          ok_match_empty = false; // Don't match the empty string immediately after another match
        }
      }else{ // No match
        result += tgt.substr(start_off);
        start_off = tgt.size()+1;
      }
    }
    delete[] pmatch;
    regfree(&preg);
    return result;
  }
}
