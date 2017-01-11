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

#ifdef HAVE_BOOST_UNIT_TEST_FRAMEWORK
#include <boost/test/unit_test.hpp>

#include "VClock.h"
#include "CPid.h"

BOOST_AUTO_TEST_SUITE(VClock_CPid_test)

BOOST_AUTO_TEST_CASE(Init_default){
  BOOST_CHECK_EQUAL(VClock<CPid>()[CPid()],0);
  BOOST_CHECK_EQUAL(VClock<CPid>()[CPid().spawn(3)],0);
}

BOOST_AUTO_TEST_CASE(Init_map){
  std::map<CPid,int> m;
  m[CPid()] = 1;
  m[CPid({0,1})] = 2;
  m[CPid({0,2},0)] = 3;
  m[CPid({0,1,1})] = 0;
  m[CPid({0,1,1,1})] = 2;
  VClock<CPid> vc(m);
  BOOST_CHECK_EQUAL(vc[CPid()],1);
  BOOST_CHECK_EQUAL(vc[CPid({0,1})],2);
  BOOST_CHECK_EQUAL(vc[CPid({0,2},0)],3);
  BOOST_CHECK_EQUAL(vc[CPid({0,1,1})],0);
  BOOST_CHECK_EQUAL(vc[CPid({0,1,1,1})],2);
  BOOST_CHECK_EQUAL(vc[CPid({0,1,2})],0);
  BOOST_CHECK_EQUAL(vc[CPid({0,1,2,2,3,4})],0);
}

BOOST_AUTO_TEST_CASE(Init_initializer_list){
  VClock<CPid> vc = {{CPid({0,1}),3},{CPid({0,1,1}),2},{CPid({0,1},0),1}};
  BOOST_CHECK_EQUAL(vc[CPid({0,1})],3);
  BOOST_CHECK_EQUAL(vc[CPid({0,1,1})],2);
  BOOST_CHECK_EQUAL(vc[CPid({0,1},0)],1);
  BOOST_CHECK_EQUAL(vc[CPid()],0);
}

BOOST_AUTO_TEST_CASE(Assignment_1){
  VClock<CPid> vc1 = {{CPid({0,1}),1},{CPid({0,1,2}),2},{CPid(),3}};
  VClock<CPid> vc2 = {{CPid({0,1}),4},{CPid({0,1,2}),5},{CPid(),6}};
  vc1 = vc2;
  BOOST_CHECK_EQUAL(vc1[CPid({0,1})],4);
  BOOST_CHECK_EQUAL(vc1[CPid({0,1,2})],5);
  BOOST_CHECK_EQUAL(vc1[CPid()],6);
  BOOST_CHECK_EQUAL(vc1[CPid({0,1},0)],0);
}

BOOST_AUTO_TEST_CASE(Assignment_2){
  VClock<CPid> vc1 = {{CPid({0,1}),1},{CPid({0,1,2}),3}};
  VClock<CPid> vc2 = {{CPid({0,1}),4},{CPid({0,2}),5},{CPid({0,3}),6}};
  vc1 = vc2;
  BOOST_CHECK_EQUAL(vc1[CPid({0,1})],4);
  BOOST_CHECK_EQUAL(vc1[CPid({0,2})],5);
  BOOST_CHECK_EQUAL(vc1[CPid({0,3})],6);
  BOOST_CHECK_EQUAL(vc1[CPid({0,1,2})],0);
  BOOST_CHECK_EQUAL(vc1[CPid({0,1,2},0)],0);
}

BOOST_AUTO_TEST_CASE(Assignment_3){
  VClock<CPid> vc1 = {{CPid({0,1}),1},{CPid({0,2}),2},{CPid({0,3}),3}};
  VClock<CPid> vc2 = {{CPid({0,1}),4},{CPid({0,1,4}),5}};
  vc1 = vc2;
  BOOST_CHECK_EQUAL(vc1[CPid({0,1})],4);
  BOOST_CHECK_EQUAL(vc1[CPid({0,2})],0);
  BOOST_CHECK_EQUAL(vc1[CPid({0,3})],0);
  BOOST_CHECK_EQUAL(vc1[CPid({0,1,4})],5);
}

BOOST_AUTO_TEST_CASE(Equality){
  BOOST_CHECK(VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),2},{CPid({0,3}),3}}) ==
              VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),2},{CPid({0,3}),3}}));
  BOOST_CHECK(VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),2},{CPid({0,3}),3}}) ==
              VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),2},{CPid({0,3}),3},{CPid({0,4}),0},{CPid({0,5}),0}}));
  BOOST_CHECK(VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),2},{CPid({0,3}),3}}) !=
              VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),1},{CPid({0,3}),3}}));
  BOOST_CHECK(VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),2},{CPid({0,3}),3}}) !=
              VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),2}}));
  BOOST_CHECK(VClock<CPid>({{CPid({0,1}),0},{CPid({0,2}),0},{CPid({0,3}),0}}) == VClock<CPid>());
  BOOST_CHECK(VClock<CPid>({{CPid({0,1}),1}}) != VClock<CPid>());
}

BOOST_AUTO_TEST_CASE(Addition){
  BOOST_CHECK_EQUAL(VClock<CPid>() + VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),2},{CPid({0,3}),3}}),
                    VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),2},{CPid({0,3}),3}}));
  BOOST_CHECK_EQUAL(VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),2},{CPid({0,3}),3}}) +
                    VClock<CPid>({{CPid({0,1}),3},{CPid({0,2}),2},{CPid({0,3}),1}}),
                    VClock<CPid>({{CPid({0,1}),3},{CPid({0,2}),2},{CPid({0,3}),3}}));
  BOOST_CHECK_EQUAL(VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),0},{CPid({0,3}),0},{CPid({0,4}),0},{CPid({0,5}),3}})+
                    VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),2},{CPid({0,3}),0},{CPid({0,4}),0},{CPid({0,5}),2},{CPid({0,6}),4}}),
                    VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),2},{CPid({0,3}),0},{CPid({0,4}),0},{CPid({0,5}),3},{CPid({0,6}),4}}));
}

BOOST_AUTO_TEST_CASE(Less){
  BOOST_CHECK(VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),2},{CPid({0,3}),3}})
              .lt(VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),3},{CPid({0,3}),3}})));
  BOOST_CHECK(VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),3},{CPid({0,3}),3}})
              .gt(VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),2},{CPid({0,3}),3}})));
  BOOST_CHECK(!(VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),2},{CPid({0,3}),3}})
                .lt(VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),2},{CPid({0,3}),3}}))));
  BOOST_CHECK(VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),2},{CPid({0,3}),3}})
              .leq(VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),2},{CPid({0,3}),3}})));
  BOOST_CHECK(VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),2},{CPid({0,3}),3}})
              .geq(VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),2},{CPid({0,3}),3}})));
  BOOST_CHECK(!(VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),2},{CPid({0,3}),3}})
                .gt(VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),2},{CPid({0,3}),3}}))));
  BOOST_CHECK(VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),2}})
              .lt(VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),2},{CPid({0,3}),0},{CPid({0,4}),1}})));
  BOOST_CHECK(!(VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),2},{CPid({0,3}),0},{CPid({0,4}),1}})
                .lt(VClock<CPid>({{CPid({0,1}),1},{CPid({0,2}),2}}))));
  BOOST_CHECK(VClock<CPid>().leq(VClock<CPid>({{CPid({0,1}),0},{CPid({0,2}),0}})));
  BOOST_CHECK(!(VClock<CPid>().lt(VClock<CPid>({{CPid({0,1}),0},{CPid({0,2}),0}}))));
}

BOOST_AUTO_TEST_CASE(Addition_2){
  VClock<CPid> vc;
  VClock<CPid> vc2 = {{CPid({0,1}),1},{CPid({0,2}),2},{CPid({0,3}),3}};
  vc += vc2;
  BOOST_CHECK_EQUAL(vc,vc2);
  vc = {{CPid({0,1}),4},{CPid({0,2}),0},{CPid({0,3}),1}};
  vc += vc2;
  BOOST_CHECK_EQUAL(vc,VClock<CPid>({{CPid({0,1}),4},{CPid({0,2}),2},{CPid({0,3}),3}}));
}

BOOST_AUTO_TEST_SUITE_END()

#endif
