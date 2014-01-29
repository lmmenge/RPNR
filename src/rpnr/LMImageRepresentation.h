//
//  LMImageRepresentation.h
//  rpnr
//
//  Created by Lucas Menge on 12/18/13.
//  Copyright (c) 2013 Lucas Menge. All rights reserved.
//

#ifndef __rpnr__LMImageRepresentation__
#define __rpnr__LMImageRepresentation__

#include <iostream>

class LMImageRepresentation
{
public:
  uint8_t* _bytes;
  uint8_t _bitsPerPixel;
  uint64_t _bytesPerRow;
  uint64_t _bytesPerPlane;
  uint8_t _samplesPerPixel;
  
  uint32_t _width;
  uint32_t _height;
  
  uint8_t _toleranceOverride;
  
  LMImageRepresentation(uint8_t* bytes, uint8_t bitsPerPixel, uint64_t bytesPerRow, uint64_t bytesPerPlane, uint8_t samplesPerPixel);
  LMImageRepresentation(LMImageRepresentation* other);
  ~LMImageRepresentation();
  
  bool isPixelEdgePixel(int64_t x, int64_t y) const;
  double differenceFactor(
                          uint64_t x, uint64_t y,
                          uint64_t otherX, uint64_t otherY,
                          uint8_t tolerance
                          ) const;
  
  bool blur(int16_t radius);
  bool blurWithFactor(int16_t radius, LMImageRepresentation* factor);
  bool blurSurfaces(
                    int16_t radius,
                    const LMImageRepresentation* factor,
                    const LMImageRepresentation** delimiters, uint8_t delimiterCount,
                    uint8_t toleranceMin, uint8_t toleranceMax
                    );
  bool blurEdges(float edgeBlurWeight,
                 LMImageRepresentation* originalCombined,
                 LMImageRepresentation* originalCombinedBlurred,
                 LMImageRepresentation* diffuseColor,
                 LMImageRepresentation* normal,
                 LMImageRepresentation* z,
                 LMImageRepresentation* emission
                 );
  bool invert();
  bool convertFromColorToGrayscale();
  bool convertFromGrayscale8ToColor24();
  
  void printDebugInfo() const;
  void checkData() const;
};

#endif /* defined(__rpnr__LMImageRepresentation__) */
