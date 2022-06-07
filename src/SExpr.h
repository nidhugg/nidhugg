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

#include <config.h>
#ifndef __SEXPR_H__
#define __SEXPR_H__

#include <istream>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include <boost/variant.hpp>

class SExpr final {
public:
    SExpr() : SExpr(List{{}}) {}
    SExpr(int i) : SExpr(Int{i}) {}
    SExpr(const char *s) : SExpr(Token{s}) {}
    SExpr(std::string s) : SExpr(Token{std::move(s)}) {}
    SExpr(std::vector<SExpr> l) : SExpr(List{std::move(l)}) {}
    SExpr(std::initializer_list<SExpr> l) : SExpr(List{std::move(l)}) {}

    enum Kind : int {
        TOKEN,
        INT,
        LIST,
    };

    class Token {
    public:
        std::string name;
    };

    class Int {
    public:
        int value;
    };

    class List {
    public:
        std::vector<SExpr> elems;
    };

    Kind kind() const { return Kind(variant.which()); }
    const Token &token() const { return boost::get<Token>(variant); }
    const Int &num() const { return boost::get<Int>(variant); }
          List &list()       { return boost::get<List>(variant); }
    const List &list() const { return boost::get<List>(variant); }

private:
    typedef boost::variant<Token,Int,List> vartype;
    SExpr(vartype variant) : variant(std::move(variant)) {}

    vartype variant;
};

std::istream &operator>>(std::istream &is, SExpr &sexp);
std::ostream &operator<<(std::ostream &os, const SExpr &sexp);

#endif
