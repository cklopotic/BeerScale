//
//    FILE: BeerScale.cpp
//  AUTHOR: Cory Klopotic
// VERSION: 0.0.1


#include "BeerScale.h"
#include <HX711.h>
#include <EEPROM.h>
#include <Preferences.h>

Preferences scalePreferences;

void BeerScale::init(uint8_t dataPin, uint8_t clockPin, int eep_add_offset, int eep_add_divider, int eep_add_kegSizeIndex){
  _scale.begin(dataPin, clockPin);
  _eep_add_divider = eep_add_divider;
  _eep_add_offset = eep_add_offset;
  _eep_add_kegSizeIndex = eep_add_kegSizeIndex;
  readScaleParams();
  set_offset(_offset);
  set_scale(_divider);
  getKegSize();
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

void BeerScale::getBeerRemaining(BeerStatus_t * pBeerStatus){
  if (_scale.wait_ready_timeout(1000)) {
    getUnitWeight();
    pBeerStatus->weight_lbs = _unitWeight - _kegSize.emptyWeight; //subtract off the weight of the keg to get the weight of the beer
    float calculated_percent = (pBeerStatus->weight_lbs / (_kegSize.fullWeight - _kegSize.emptyWeight)) * 100;
    pBeerStatus->level_percent = constrain(calculated_percent, 0.0, 100.0);
    pBeerStatus->units_remain = (pBeerStatus->weight_lbs * OZ_PER_LB / OZ_PER_UNIT);
    Serial.print("KegSize MT weight:");
    Serial.println(_kegSize.emptyWeight);
  } else {
      printf("HX711 not found.\n");
  }
}

void BeerScale::saveScaleParams(){
  EEPROM.put(_eep_add_offset, _offset);
  EEPROM.put(_eep_add_divider, _divider);
  if(EEPROM.commit()){
    Serial.print(">>>> Scale Values saved. Offset: ");
    Serial.print(_offset);
    Serial.print(", Divider: ");
    Serial.println(_divider);
  } else {
    Serial.println("ERROR: Values not saved");
  }

  scalePreferences.begin("BeerScale", false);
  scalePreferences.putLong("offset", _offset);
  scalePreferences.putFloat("divider", _divider);
  scalePreferences.end();
  
}

void BeerScale::readScaleParams(){
  EEPROM.get(_eep_add_offset, _offset);
  EEPROM.get(_eep_add_divider,_divider);
  EEPROM.get(_eep_add_kegSizeIndex,_kegSelection); //selectionIndex



  scalePreferences.begin("BeerScale", false);
  /*
  _offset = scalePreferences.getLong("offset", 0);
  _divider = scalePreferences.getFloat("divider", 1.0);
  _kegSelection = scalePreferences.getInt("kegSelection", 0);
  */
  //one time run to transfer EEPROM to preferences values
  scalePreferences.putLong("offset", _offset);
  scalePreferences.putFloat("divider", _divider);
  scalePreferences.putInt("kegSelection", _kegSelection);
  //end one time transfer
  scalePreferences.end();

  _kegSelection = constrain(_kegSelection,0,9);
  //_kegSelection = 0;
  _kegSize = KegSelections[_kegSelection];  //set the KegSize using Index

  Serial.print("EEPROM Read>> Scale Offset: ");
  Serial.print(_offset);
  Serial.print(", Divider: ");
  Serial.print(_divider);
  Serial.print(", KegSize Index: ");
  Serial.println(_kegSelection);
}  

KegSize_t BeerScale::getKegSize(){
  return _kegSize;
}

void BeerScale::setKegSize(int kegsizeIndex){
  _kegSelection = kegsizeIndex;
  _kegSize = KegSelections[_kegSelection];
  EEPROM.put(_eep_add_kegSizeIndex,_kegSelection);
  
  scalePreferences.begin("BeerScale", false);
  scalePreferences.putInt("kegSelection", _kegSelection);
  scalePreferences.end();
}

;