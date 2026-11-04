#include "telemetry.hpp"
#include "car_brand.hpp"
#include <cassert>
#include <string>
#include <iostream>
int main(){
 acc::Car pending;assert(pending.model==-1);assert(std::string(acc::carBrand(pending.model)).empty());
 for(int id:{5,22,35,59})assert(std::string(acc::carBrand(id))=="mclaren");
 for(int id:{0,9,23,28,34,61,85,86})assert(std::string(acc::carBrand(id))=="porsche");
 for(int id:{-1,37,49,54,62,79,81,87,255})assert(std::string(acc::carBrand(id)).empty());
 for(int id=0;id<=36;id++)assert(!std::string(acc::carBrand(id)).empty());
 for(int id:{50,51,52,53,55,56,57,58,59,60,61,80,82,83,84,85,86})assert(!std::string(acc::carBrand(id)).empty());
 std::cout<<"PASS: ACC brand IDs, McLaren variants and unknown-before-metadata fallback\n";
}
