#include "background.hpp"
#include <cassert>
#include <iostream>
using namespace acc;
int main(){
 SessionVisibility s;
 assert(!s.update(100,false,false,false,false));assert(!s.show(false,false,true));
 assert(!s.update(1100,true,false,false,false)); // process started, still loading
 assert(!s.update(1200,true,true,true,true)); // menu always wins over cached data
 assert(s.update(2000,true,false,true,false)); // shared memory, no UDP yet
 assert(s.show(false,false,true));assert(!s.show(false,true,true));assert(!s.show(false,false,false));
 assert(s.update(2500,true,false,false,false));assert(s.update(4999,true,false,false,false)); // brief dropout
 assert(!s.update(5000,true,false,false,false));
 assert(s.update(6000,true,false,false,true)); // UDP-only session
 assert(!s.update(6100,false,false,true,true)); // close ACC immediately hides
 assert(s.update(7000,true,false,true,false));assert(!s.update(7100,true,true,false,true)); // menu
 assert(s.update(8000,true,false,true,false)); // restart into another race
 assert(s.update(300000,true,false,true,false)); // pause with valid shared snapshot
 assert(!s.update(299999,true,false,false,false)); // reject backwards clock
 assert(s.show(true,false,true));assert(!s.show(true,true,true)); // deliberate demo / F11
 assert(backgroundPollInterval(false,false)==1000&&backgroundPollInterval(true,false)==100&&backgroundPollInterval(false,true)==100);
 std::cout<<"PASS: boot, loading, menu, shared/UDP session, dropouts, pause, restart, game close, hidden/disabled panels, demo and idle polling\n";
}
