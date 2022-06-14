/* Copyright (C) 2018 Magnus Lång and Tuan Phong Ngo
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

#include "SExpr.h"

#include <cstdint>


typedef std::logic_error parse_error;

static bool endoftok(int c) {
    return c == EOF || std::isspace(c) || c == ')' || c == '(';
}

std::istream &operator>>(std::istream &is, SExpr &sexp) {
    std::ws(is);
    int c = is.get();
    assert(!(std::isspace(c)));
    if (c == EOF) throw parse_error("Got EOF");
    if (c == ')') throw parse_error("Unmatched closing paren");
    if (c == '(') {
        std::vector<SExpr> elems;
        while (std::ws(is).peek() != ')') {
            elems.emplace_back();
            is >> elems.back();
        }
        c = is.get();
        assert(c == ')');
        sexp = SExpr(std::move(elems));
    } else {
        std::string str;
        for( ; !endoftok(c); c = is.get()) str.push_back(c);
        is.putback(c);
        char *end;
        int num = std::strtol(str.c_str(), &end, 10);
        if ((end - str.c_str()) != int_fast64_t(str.size())) {
            sexp = SExpr(str);
        } else {
            sexp = SExpr(num);
        }
    }
    return is;
}

std::ostream &operator<<(std::ostream &os, const SExpr &sexp) {
    switch(sexp.kind()) {
    case SExpr::TOKEN:
        return os << sexp.token().name;
    case SExpr::INT:
        return os << sexp.num().value;
    case SExpr::LIST: {
        os << "(";
        const auto &l = sexp.list().elems;
        auto i = l.begin();
        if (i != l.end()) {
            while(true) {
                os << *i;
                ++i;
                if (i == l.end()) break;
                os << " ";
            }
        }
        return os << ")";
    }
    default:
        abort();
    }
}
