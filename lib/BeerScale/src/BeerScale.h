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

const KegSize_t HALFBARREL = {
  .emptyWeight = 33.0,
  .fullWeight = 160.0
};

const KegSize_t QUARTERBARREL = {
  .emptyWeight = 22.0,
  .fullWeight = 87.0
};

const KegSize_t KegSelections[2] = {HALFBARREL, QUARTERBARREL};

class BeerScale
{
public:
  
  void     init(uint8_t dataPin, uint8_t clockPin, int eep_add_offset, int eep_add_divider, int eep_add_kegSizeIndex);
  void     saveScaleParams();
  void     readScaleParams();
  bool     set_scale(float divider = 1.0);
  void     set_offset(long offset = 0);
  long     tare();
  float    getUnitWeight();
  float    setKnownWeight(uint16_t knownWeight);
  void     getBeerRemaining(BeerStatus_t * beerStatus);
  KegSize_t getKegSize();
  void     setKegSize(int kegsizeIndex);
private:
  HX711    _scale;
  long     _offset;
  float    _divider;
  int      _eep_add_divider;
  int      _eep_add_offset;
  int      _eep_add_kegSizeIndex;
  float    _unitWeight;
  int      _kegSelection;
  KegSize_t _kegSize;
};



#endif
