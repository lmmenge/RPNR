//
//  LMImageRepresentationBlur2.cpp
//  rpnr
//
//  Created by Lucas Menge on 1/15/14.
//  Copyright (c) 2014 Lucas Menge. All rights reserved.
//

#include "LMImageRepresentation.h"

#include <iostream>
#include <math.h>

bool LMImageRepresentation::blurEdges(float edgeBlurWeight,
                                      LMImageRepresentation* originalCombined,
                                      LMImageRepresentation* originalCombinedBlurred,
                                      LMImageRepresentation* diffuseColor,
                                      LMImageRepresentation* normal,
                                      LMImageRepresentation* z,
                                      LMImageRepresentation* emission
                                      )
{
  if(edgeBlurWeight == 0)
    return true;

  if(originalCombined == NULL || originalCombinedBlurred == NULL)
    return false;
  
  uint8_t toleranceOverride = 2;
  if(normal != NULL)
    normal->_toleranceOverride = toleranceOverride;
  if(z != NULL)
    z->_toleranceOverride = toleranceOverride;
  
  uint8_t tolerance = 1;
  
#ifdef GCD
  dispatch_apply(_width, dispatch_get_global_queue(0, 0), ^(size_t i)
  {
    uint32_t x = (uint32_t)i;
#else
  for(uint32_t x=0; x<_width; x++)
  {
#endif
    for(uint32_t y=0; y<_height; y++)
    {
      uint8_t diff = 0;
      for(uint8_t sample=0; sample<_samplesPerPixel; sample++)
      {
        uint64_t pixelIndex = (uint64_t)y*_bytesPerRow+(uint64_t)x*_samplesPerPixel+sample;
        if(abs((int16_t)_bytes[pixelIndex] - (int16_t)originalCombined->_bytes[pixelIndex]) >= tolerance)
        {
          diff = 0xFF;
          break;
        }
      }
      if(diff < tolerance
         && normal->isPixelEdgePixel(x, y) == false
         && z->isPixelEdgePixel(x, y) == false)
        diff = 0xFF;
      for(uint8_t sample=0; sample<_samplesPerPixel; sample++)
      {
        uint64_t pixelIndex = (uint64_t)y*_bytesPerRow+(uint64_t)x*_samplesPerPixel+sample;
        if(diff < tolerance)
        {
          _bytes[pixelIndex] = _bytes[pixelIndex]*(1-edgeBlurWeight)+originalCombinedBlurred->_bytes[pixelIndex]*edgeBlurWeight;
          //_bytes[pixelIndex] = 0xFF;
        }
        //else
          //_bytes[pixelIndex] = 0;
      }
    }
  }
#ifdef GCD
    );
#endif
  return true;
}