//
//    FILE: BeerScale.cpp
//  AUTHOR: Cory Klopotic
// VERSION: 0.0.1


#include "BeerScale.h"
#include <HX711.h>

void BeerScale::init(uint8_t dataPin, uint8_t clockPin)
{
  _scale.begin(dataPin, clockPin);
}

bool BeerScale::set_scale(float divider){
  return _scale.set_scale(divider);
};
void BeerScale::set_offset(long offset){
  _scale.set_offset(offset);
};

long BeerScale::tare(){
  _offset = 0;   
  _scale.tare(20);
  _offset = _scale.get_offset();
  return _offset;
};

float BeerScale::setKnownWeight(uint16_t knownWeight){
  _divider = 0;
  _scale.calibrate_scale(knownWeight, 20);
  _divider = _scale.get_scale();
  return _divider;
};

float BeerScale::getUnitWeight(){
  _unitWeight = _scale.get_units(10);
  return _unitWeight;
}

void BeerScale::getBeerRemaining(BeerStatus_t * pBeerStatus, KegSize_t * pActiveKegSize){
  if (_scale.wait_ready_timeout(1000)) {
    getUnitWeight();
    pBeerStatus->weight_lbs = _unitWeight - pActiveKegSize->emptyWeight; //subtract off the weight of the keg to get the weight of the beer
    pBeerStatus->level_percent = (pBeerStatus->weight_lbs / (pActiveKegSize->fullWeight - pActiveKegSize->emptyWeight)) * 100;
    pBeerStatus->units_remain = (pBeerStatus->weight_lbs * OZ_PER_LB / OZ_PER_UNIT);
  } else {
      printf("HX711 not found.\n");
  }
};