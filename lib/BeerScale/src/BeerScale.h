#ifndef BEER_SCALE_H
#define BEER_SCALE_H

#define OZ_PER_LB 16
#define OZ_PER_UNIT 12

#include <HX711.h>

// Structure to store status values
typedef struct {
  float weight_lbs;
  float level_percent;
  float units_remain;
} BeerStatus_t;

typedef struct {
  float emptyWeight;
  float fullWeight;
} KegSize_t;

class BeerScale
{
public:
  
  void     init(uint8_t dataPin, uint8_t clockPin);
  bool     set_scale(float divider = 1.0);
  void     set_offset(long offset = 0);
  long     tare();
  float    getUnitWeight();
  float    setKnownWeight(uint16_t knownWeight);
  void     getBeerRemaining(BeerStatus_t * beerStatus, KegSize_t * activeKegSize);
private:
  HX711    _scale;
  long     _offset;
  float    _divider;
  float    _unitWeight;
};



#endif
