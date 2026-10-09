#include "../src/relatives.hpp"
#include <cassert>
#include <iostream>
using namespace acc;
int main(){State s;Car me;me.id=9;me.spline=.98f;me.laps=4;me.location=1;me.seen=10000;s.cars[9]=me;
 Car a=me;a.id=2;a.spline=.01f;a.laps=5;s.cars[2]=a;Car b=me;b.id=3;b.spline=.95f;s.cars[3]=b;
 auto r=relativeRows(s,me,11000);assert(r.ahead.size()==1&&r.ahead[0].car.id==2&&!r.ahead[0].stale);assert(r.behind.size()==1&&r.behind[0].car.id==3);
 // A dropout ahead does not clear the valid rows behind; keep it visibly stale.
 s.cars[3].seen=15000;r=relativeRows(s,me,15000);assert(r.ahead.size()==1&&r.ahead[0].stale);assert(!r.behind[0].stale);
 r=relativeRows(s,me,18000);assert(r.ahead.empty()&&r.behind.size()==1);
 s.cars[2].seen=18100;r=relativeRows(s,me,18200);assert(r.ahead.size()==1&&!r.ahead[0].stale);
 s.cars[2].location=2;r=relativeRows(s,me,18200);assert(r.ahead.empty());s.cars[2].location=1;
 r=relativeRows(s,me,999999,true);assert(r.ahead.size()==1&&!r.ahead[0].stale);
 // World order is independent of race position and lap deficit.
 s.cars[2].laps=3;s.cars[2].position=30;r=relativeRows(s,me,18200);assert(r.ahead[0].car.id==2);
 // Fresh samples take priority over stale samples on the same side.
 Car c=a;c.id=4;c.spline=.05f;c.seen=22000;s.cars[4]=c;r=relativeRows(s,me,23000);assert(r.ahead.size()==2&&r.ahead[0].car.id==4&&r.ahead[1].stale);
 s.clearSession();r=relativeRows(s,me,23000);assert(r.ahead.empty()&&r.behind.empty());
 std::cout<<"PASS: finish-line wrap, independent sides, short dropout/stale state, expiry/recovery, pits, pause, lapped cars, fresh priority and session reset\n";
}
