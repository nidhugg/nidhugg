/* Copyright (C) 2017 Magnus Lång
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

#include "TraceDumper.h"

#include <fstream>
#include <iostream>

namespace TraceDumper {

  static std::string dot_escape_string(const std::string &input) {
    std::string output = "";
    for (char c : input) {
      if (c == '\\') {
        output += "\\\\";
      } else if (c == '"') {
        output += "\\\"";
      } else {
        output += c;
      }
    }
    return output;
  }

  static std::string node_name(unsigned trace_index, const IID<CPid> &iid) {
    return "\"" + std::to_string(trace_index)
      + " " + dot_escape_string(iid.to_string()) + "\"";
  }

  static void print_trace_node
  (std::fstream &out, unsigned ti, const Trace &trace, unsigned ei,
   std::map<IID<CPid>,std::vector<Error*>> &errorm) {
    const IID<CPid> &iid = trace.get_iid(ei);
    out << "    " << node_name(ti, iid) << " [label=\"" << iid
        << dot_escape_string(trace.event_desc(ei));
    std::vector<Error*> errors = std::move(errorm[trace.get_iid(ei)]);
    errorm.erase(trace.get_iid(ei));
    for (const Error *error : errors) {
      out << "\n" << dot_escape_string(error->to_string());
    }
    out << "\",";
    if (errors.size()) {
      out << "color=red,penwidth=5,";
    }
    out << "]\n";
  }

  static void print_end_node(std::fstream &out, unsigned ti, const Trace &t) {
    out << "    end_" << ti << " [label=\"";
    if (t.has_errors()) {
      out << "Error\",style=filled,fillcolor=red]\n";
    } else if (t.is_blocked()) {
      out << "SSB\",style=filled,fillcolor=yellow]\n";
    } else {
      out << "Ok\",style=filled,fillcolor=green]\n";
    }
  }

  static VClock<CPid> reconstruct_error_clock
  (const IIDVCSeqTrace &t, const IID<CPid> &iid, unsigned *last_index_out) {
    VClock<CPid> result = t.get_clock(*last_index_out = 0);
    for (unsigned i = t.size()-1; i > 0; --i) {
      if (t.get_iid(i).get_pid() == iid.get_pid()) {
        result = t.get_clock(*last_index_out = i);
        break;
      }
    }
    result[iid.get_pid()] = iid.get_index();
    return result;
  }

  static bool open_file(std::fstream &out, std::string filename) {
    out.open(filename, std::fstream::out | std::fstream::trunc);
    if (!out) {
      throw std::logic_error("Failed to open output file " +filename);// << std::endl;
    }
    return !!out;
  }

  static std::map<IID<CPid>,std::vector<Error*>> sort_errors(const Trace &t) {
    std::map<IID<CPid>,std::vector<Error*>> errors;
    for (const std::unique_ptr<Error> &error : t.get_errors()) {
      errors[error->get_location()].push_back(error.get());
    }
    return errors;
  }

  static void dump_traces(const DPORDriver::Result &res,
                          const Configuration &conf) {
    if (conf.memory_model != Configuration::SC
        && conf.memory_model != Configuration::TSO
        && conf.memory_model != Configuration::PSO) {
      return;
    }
    std::fstream out;
    if (!open_file(out, conf.trace_dump_file)) return;
    out << "strict digraph {\n";
    out << "  packmode=array_tuc1\n"; // Speed up layout
    out << "  node [shape=box,fontname=Monospace]\n";
    for (unsigned ti = 0; ti < res.all_traces.size(); ++ti) {
      const IIDVCSeqTrace &t = static_cast<IIDVCSeqTrace&>(*res.all_traces[ti]);
      std::map<IID<CPid>,std::vector<Error*>> errors = sort_errors(t);
      out << "  subgraph trace_" << ti << " {\n";
      out << "    sortv = " << ti << "\n";
      out << "    start_" << ti << " [label=\"Trace " << (ti+1) << "\"]\n";
      out << "    start_" << ti << " -> " << node_name(ti, t.get_iid(0)) << "\n";
      for (unsigned ei = 0; ei < t.size(); ++ei) {
        print_trace_node(out, ti, t, ei, errors);
        std::vector<const VClock<CPid>*> frontier;
        for (unsigned pi = ei; pi > 0;) {
          --pi;
          if (t.get_clock(pi).leq(t.get_clock(ei))
              && !std::any_of(frontier.begin(), frontier.end(),
                              [&](const VClock<CPid> *fc) {
                                return t.get_clock(pi).leq(*fc);
                              })) {
            out << "    " << node_name(ti, t.get_iid(pi)) << " -> "
                << node_name(ti, t.get_iid(ei)) << "\n";
            frontier.push_back(&t.get_clock(pi));
          }
        }
      }

      std::vector<VClock<CPid>> end_front;
      for (std::pair<const IID<CPid>,std::vector<Error*>> &p : errors) {
        out << "    " << node_name(ti, p.first)
            << " [color=red,penwidth=5,label=\"";
        for (const Error *error : p.second) {
          out << dot_escape_string(error->to_string()) << "\n";
        }
        out << "\"]\n";
        out << "    " << node_name(ti, p.first) << " -> "
            << "end_" << ti << "\n";
        unsigned last_index;
        end_front.push_back(reconstruct_error_clock(t, p.first, &last_index));
        out << "    " << node_name(ti, t.get_iid(last_index)) << " -> "
            << node_name(ti, p.first) << "\n";

      }
      /* Print end node */
      print_end_node(out, ti, t);
      for (unsigned pi = t.size(); pi > 0;) {
        --pi;
        if (!std::any_of(end_front.begin(), end_front.end(),
                         [&](const VClock<CPid> &fc) {
                           return t.get_clock(pi).leq(fc);
                         })) {
          out << "    " << node_name(ti, t.get_iid(pi)) << " -> "
              << "end_" << ti << "\n";
          end_front.push_back(t.get_clock(pi));
        }
      }
      out << "  }\n";
    }

    out << "}\n";
  }

  static void dump_tree(const DPORDriver::Result &res,
                        const Configuration &conf) {
    std::fstream out;
    if (!open_file(out, conf.tree_dump_file)) return;
    out << "digraph {\n";
    out << "  graph [ranksep=0.3]\n";
    out << "  node [shape=box,width=7,fontname=Monospace]\n";
    out << "  start [label=\"Initial\"]\n";

    /* Labels of the events of the current column- */
    std::vector<std::string> labels;

    for (unsigned ti = 0; ti < res.all_traces.size(); ++ti) {
      const Trace &t = *res.all_traces[ti];
      std::map<IID<CPid>,std::vector<Error*>> errors = sort_errors(t);
      out << "  subgraph trace_" << ti << " {\n";
      std::string first_node_name = node_name(ti, t.get_iid(t.replay_point()));
      print_trace_node(out, ti, t, t.replay_point(), errors);
      /* Edge from previous trace */
      if (labels.empty()) {
        /* This is the first trace, edge from start node instead */
        out << "    start -> " << first_node_name << " [weight=1000]\n";
      } else {
        std::string previous_node_name = labels[t.replay_point()];
        std::string pre_prev_node_name =
          t.replay_point() ? labels[t.replay_point() - 1] : "start";
        out << "    " << pre_prev_node_name << " -> " << first_node_name
            << " [style=invis,weight=1]\n";
        out << "    " << previous_node_name << " -> " << first_node_name
            << " [constraint=false]\n";
        labels.resize(t.replay_point());
      }
      labels.push_back(first_node_name);
      for (unsigned ei = t.replay_point()+1; ei < t.size(); ++ei) {
        print_trace_node(out, ti, t, ei, errors);
        out << "    " << labels.back() << " -> " << node_name(ti, t.get_iid(ei))
            << " [weight=1000]\n";
        labels.push_back(node_name(ti, t.get_iid(ei)));
      }

      for (std::pair<const IID<CPid>,std::vector<Error*>> &p : errors) {
        out << "    " << node_name(ti, p.first)
            << " [color=red,penwidth=5,label=\"";
        for (const Error *error : p.second) {
          out << dot_escape_string(error->to_string()) << "\n";
        }
        out << "\"]\n";
        out << "    " << labels.back() << " -> "
            << node_name(ti, p.first) << " [weight=1000]\n";
        labels.push_back(node_name(ti, p.first));
      }
      /* Print end node */
      print_end_node(out, ti, t);
      out << "    " << labels.back() << " -> " << "end_" << ti
          << " [weight=1000]\n";
      out << "  }\n";
    }

    out << "}\n";
  }


  void dump(const DPORDriver::Result &res,
            const Configuration &conf) {
    if (!conf.debug_collect_all_traces) return;
    if (!conf.trace_dump_file.empty()) dump_traces(res, conf);
    if (!conf.tree_dump_file.empty()) dump_tree(res, conf);
  }

}
