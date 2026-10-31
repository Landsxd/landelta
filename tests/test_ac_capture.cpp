#include "ac_capture.hpp"
#include <cassert>
#include <iostream>
using namespace acstudio;
int main(){
 Physics p{};Graphics g{};g.status=2;p.gas=.5;assert(valid(p,g));p.brake=NAN;assert(!valid(p,g));p.brake=0;g.status=1;assert(!valid(p,g));
 std::vector<Point> points;for(int i=0;i<=1000;i++)points.push_back({i/1000.,i*100.,uint64_t(1+i*100)});
 auto s=microsectors(points,50);assert(s.size()==50);for(auto v:s)assert(std::abs(v-2000)<.001);
 points.erase(points.begin()+200,points.begin()+240);s=microsectors(points,50);assert(std::isnan(s[10]));
 assert(microsectors({},50).size()==50);assert(std::isnan(microsectors({},50)[0]));
 auto partial=microsectors({{.4,1000,1},{.401,1100,101}},50);assert(std::isnan(partial[0]));
 std::cout<<"PASS: AC ABI prefixes, finite live samples, interpolated microsectors, gaps and partial laps\n";
}
