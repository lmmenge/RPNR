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

double LMImageRepresentation::differenceFactor(
                                               uint64_t x, uint64_t y,
                                               uint64_t otherX, uint64_t otherY,
                                               uint8_t tolerance
                                               ) const
{
  if(_toleranceOverride != 0)
    tolerance = _toleranceOverride;
  double factor = 0;
  uint8_t bitsPerSample = _bitsPerPixel/_samplesPerPixel;
  if(bitsPerSample == 8)
  {
    uint8_t reference;
    uint8_t other;
    uint16_t delta;
    for(uint8_t sample=0; sample<_samplesPerPixel; sample++)
    {
      reference = _bytes[y*_bytesPerRow+x*_samplesPerPixel+sample];
      other = _bytes[otherY*_bytesPerRow+otherX*_samplesPerPixel+sample];
      delta = abs((int16_t)reference-other);
      if(delta > tolerance)
        return 0;
      factor += delta;///(float)0xFF;
    }
  }
  else if(bitsPerSample == 16)
  {
    uint16_t reference;
    uint16_t other;
    uint32_t delta;
    uint16_t* samples16 = (uint16_t*)_bytes;
    uint64_t samplesPerRow = _width*_samplesPerPixel;
    uint16_t tolerance16 = tolerance*(0xFFFF/(double)0xFF);
    for(uint8_t sample=0; sample<_samplesPerPixel; sample++)
    {
      reference = samples16[y*samplesPerRow+x*_samplesPerPixel+sample];
      other = samples16[otherY*samplesPerRow+otherX*_samplesPerPixel+sample];
      delta = abs((int32_t)reference-other);
      if(delta > tolerance16)
        return 0;
      factor += delta*(0xFF/(double)0xFFFF);///(float)0xFF;
    }
  }
  //factor /= samplesPerPixel*0xFF;
  //factor *= (float)0xFF/tolerance;
  //std::cout << "f: " << factor << "\n";
  
  //factor = 1-factor;
  //return 1-factor/(samplesPerPixel*0xFF)*((float)0xFF/tolerance);
  return 1-factor/_samplesPerPixel/tolerance;
  //factor = factor*factor*factor;
  return factor;
}

bool LMImageRepresentation::blurSurfaces(
                                         int16_t radius,
                                         const LMImageRepresentation* factor,
                                         const LMImageRepresentation** delimiters, uint8_t delimiterCount,
                                         uint8_t toleranceMin, uint8_t toleranceMax
                                         )
{
  if(radius == 0)
    return true;
  if(factor == NULL || factor->_samplesPerPixel != 1 || factor->_width != _width || factor->_height != _height)
  {
    //std::cout << "Factor image didn't match me\n";
    return false;
  }
  
  uint8_t* bytesSource = (uint8_t*)calloc(_bytesPerPlane, sizeof(uint8_t));
  memcpy(bytesSource, _bytes, _bytesPerPlane);
  
  // this method we run the image twice. doing horizontal blur, then vertical blur
  // set up for the X axis (0)
  uint8_t deltaX = 1;
  uint8_t deltaY = 0;
  uint32_t maxConvolutionOffset = _width;
  for(uint8_t axis=0; axis<2; axis++)
  {
    if(axis == 1)
    {
      // since we only read from bytesSource and write to _bytes
      // update the results when moving from axis 0 to 1
      memcpy(bytesSource, _bytes, _bytesPerPlane);
      
      // we're doing the Y axis (1) now
      deltaX = 0;
      deltaY = 1;
      maxConvolutionOffset = _height;
    }
    
#ifdef GCD
    dispatch_apply(_width, dispatch_get_global_queue(0, 0), ^(size_t i)
    {
      uint32_t x = (uint32_t)i;
#else
    for(uint32_t x=0; x<_width; x++)
    {
#endif
      // place to store the difference factors for a pixel once we reach them
      double* differenceFactors = (double*)calloc(2*radius+1, sizeof(double));
      uint8_t originalValue;
      uint32_t value;
      for(uint32_t y=0; y<_height; y++)
      {
        // calculating the factor just for this pixel
        uint8_t factorValue = factor->_bytes[factor->_bytesPerRow*y+x];
        float factorPercent = factorValue/(float)0xFF;
        //int16_t pixelRadius = factorPercent*radius; // use larger radius for more occluded areas
        int16_t pixelRadius = radius; // use fixed radius
        uint8_t tolerance = (factorPercent*factorPercent*factorPercent*factorPercent*factorPercent)*(toleranceMax-toleranceMin)+toleranceMin;
        
        // calculate the difference factors for each pixel around us
        int64_t convolutionBase = x;
        if(axis == 1)
          convolutionBase = y;
        for(int16_t convDelta=-pixelRadius; convDelta<=pixelRadius; convDelta++)
        {
          int64_t convPointReal = convolutionBase+convDelta;
          int16_t convDeltaClipped = convDelta;
          if(convPointReal < 0)
          {
            convDeltaClipped = convDelta+(0-convPointReal);
            //convPointReal = 0;
          }
          else if(convPointReal >= maxConvolutionOffset)
          {
            convDeltaClipped = convDelta-(convPointReal-(maxConvolutionOffset-1));
            //convPointReal = maxConvolutionOffset-1;
          }
          
          double factor = 0;
          if(delimiterCount == 0)
          {
            factor = 1;
          }
          else
          {
            // do it for regular noise
            for(uint8_t d=0; d<delimiterCount; d++)
            {
              double thisFactor = delimiters[d]->differenceFactor(x, y, x+convDeltaClipped*deltaX, y+convDeltaClipped*deltaY, tolerance);
              if(thisFactor == 0)
              {
                factor = 0;
                break;
              }
              factor += thisFactor;
            }
            factor /= delimiterCount;
          }
          //std::cout << std::endl << "f " << factor << std::endl;
          if(convDelta+pixelRadius < 0 || convDelta+pixelRadius >= 2*radius+1)
            std::cout << "OOB" << std::endl;
          differenceFactors[convDelta+pixelRadius] = factor;
        }
        
        // run each sample
        for(uint8_t sample=0; sample<_samplesPerPixel; sample++)
        {
          uint32_t radiusDivisorSample = 0;
          
          // delimiter value for this original sample
          originalValue = bytesSource[(uint64_t)y*_bytesPerRow+(uint64_t)x*_samplesPerPixel+sample];
          
          value = 0;
          double factor;
          // run the other samples
          for(int16_t convDelta=-pixelRadius; convDelta<=pixelRadius; convDelta++)
          {
            int64_t convPointReal = convolutionBase+convDelta;
            int16_t convDeltaClipped = convDelta;
            if(convPointReal < 0)
            {
              convDeltaClipped = convDelta+(0-convPointReal);
              //convPointReal = 0;
            }
            else if(convPointReal >= maxConvolutionOffset)
            {
              convDeltaClipped = convDelta-(convPointReal-(maxConvolutionOffset-1));
              //convPointReal = maxConvolutionOffset-1;
            }
            
            factor = differenceFactors[convDelta+pixelRadius];
            if(factor != 0)
            {
              radiusDivisorSample++;
              value += bytesSource[(uint64_t)(y+convDeltaClipped*deltaY)*_bytesPerRow+(uint64_t)(x+convDeltaClipped*deltaX)*_samplesPerPixel+sample]*factor+originalValue*(1-factor);
            }
          }
          if(radiusDivisorSample != 0)
          {
            value /= radiusDivisorSample;
            _bytes[(uint64_t)y*_bytesPerRow+(uint64_t)x*_samplesPerPixel+sample] = value;
          }
        } // for sample
      } // for y
      free(differenceFactors);
    } // for x
#ifdef GCD
    );
#endif
  }
  
  free(bytesSource);
                   
  return true;
}