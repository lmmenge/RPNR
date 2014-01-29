//
//  LMImageRepresentationBlur.cpp
//  rpnr
//
//  Created by Lucas Menge on 12/19/13.
//  Copyright (c) 2013 Lucas Menge. All rights reserved.
//

#include "LMImageRepresentation.h"

#include <iostream>
#include <math.h>

bool LMImageRepresentation::blur(int16_t radius)
{
  if(radius == 0)
    return true;
  
  uint8_t* bytesOriginal = (uint8_t*)calloc(_bytesPerPlane, sizeof(uint8_t));
  memcpy(bytesOriginal, _bytes, _bytesPerPlane);
  
  uint32_t radiusDivisor = abs(radius);
  radiusDivisor = 1+2*radiusDivisor;

  // this method we run the image twice. doing horizontal blur, then vertical blur
  // run the image in one axis
#ifdef GCD
  dispatch_apply(_width, dispatch_get_global_queue(0, 0), ^(size_t i)
  {
    uint32_t x = (uint32_t)i;
#else
  for(uint32_t x=0; x<_width; x++)
  {
#endif
    uint32_t value;
    for(uint32_t y=0; y<_height; y++)
    {
      // run each sample
      for(uint8_t sample=0; sample<_samplesPerPixel; sample++)
      {
        value = 0;
        // run the other samples
        for(int64_t xs=(int64_t)x-radius; xs<=(int64_t)x+radius; xs++)
        {
          int64_t xsReal = xs;
          if(xsReal < 0)
            xsReal = 0;
          else if(xsReal >= _width)
            xsReal = _width-1;
          
          value += bytesOriginal[(uint64_t)y*_bytesPerRow+(uint64_t)xsReal*_samplesPerPixel+sample];
        }
        value /= radiusDivisor;
        _bytes[(uint64_t)y*_bytesPerRow+(uint64_t)x*_samplesPerPixel+sample] = value;
      } // for sample
    } // for y
  } // for x
#ifdef GCD
  );
#endif
    
  // update the results
  memcpy(bytesOriginal, _bytes, _bytesPerPlane);
    
  // run the image in the other axis
#ifdef GCD
  dispatch_apply(_width, dispatch_get_global_queue(0, 0), ^(size_t i)
  {
    uint32_t x = (uint32_t)i;
#else
  for(uint32_t x=0; x<_width; x++)
  {
#endif
    uint32_t value;
    for(uint32_t y=0; y<_height; y++)
    {
      // run each sample
      for(uint8_t sample=0; sample<_samplesPerPixel; sample++)
      {
        value = 0;
        // run the other samples
        for(int64_t ys=(int64_t)y-radius; ys<=(int64_t)y+radius; ys++)
        {
          int64_t ysReal = ys;
          if(ysReal < 0)
            ysReal = 0;
          else if(ysReal >= _height)
            ysReal = _height-1;
          
          value += bytesOriginal[(uint64_t)ysReal*_bytesPerRow+(uint64_t)x*_samplesPerPixel+sample];
        }
        
        value /= radiusDivisor;
        _bytes[(uint64_t)y*_bytesPerRow+(uint64_t)x*_samplesPerPixel+sample] = value;
      } // for sample
    } // for y
  } // for x
#ifdef GCD
  );
#endif
  
  free(bytesOriginal);
  return true;
}

bool LMImageRepresentation::blurWithFactor(int16_t radius, LMImageRepresentation* factor)
{
  if(radius == 0)
    return true;
  if(factor == NULL || factor->_samplesPerPixel != 1 || factor->_width != _width || factor->_height != _height)
  {
    std::cout << "Factor image didn't match me\n";
    return false;
  }
  
  uint8_t* bytesOriginal = (uint8_t*)calloc(_bytesPerPlane, sizeof(uint8_t));
  memcpy(bytesOriginal, _bytes, _bytesPerPlane);
  
  uint32_t value;
  uint16_t radiusDivisor;
  // this method we run the image twice. doing horizontal blur, then vertical blur
  // run the image in one axis
  for(uint32_t x=0; x<_width; x++)
  {
    for(uint32_t y=0; y<_height; y++)
    {
      // run each sample
      for(uint8_t sample=0; sample<_samplesPerPixel; sample++)
      {
        // calculating the factor just for this pixel
        uint8_t factorValue = factor->_bytes[factor->_bytesPerRow*y+x];
        float factorPercent = factorValue/(float)0xFF;
        uint16_t pixelRadius = factorPercent*radius;
        radiusDivisor = abs(pixelRadius);
        radiusDivisor = 1+2*radiusDivisor;
        
        value = 0;
        // run the other samples
        for(int64_t xs=(int64_t)x-pixelRadius; xs<=(int64_t)x+pixelRadius; xs++)
        {
          int64_t xsReal = xs;
          if(xsReal < 0)
            xsReal = 0;
          else if(xsReal >= _width)
            xsReal = _width-1;
          
          value += bytesOriginal[(uint64_t)y*_bytesPerRow+(uint64_t)xsReal*_samplesPerPixel+sample];
        }
        value /= radiusDivisor;
        _bytes[(uint64_t)y*_bytesPerRow+(uint64_t)x*_samplesPerPixel+sample] = value;
      } // for sample
    } // for y
  } // for x
  // update the results
  memcpy(bytesOriginal, _bytes, _bytesPerPlane);
  // run the image in the other axis
  for(uint32_t x=0; x<_width; x++)
  {
    for(uint32_t y=0; y<_height; y++)
    {
      // run each sample
      for(uint8_t sample=0; sample<_samplesPerPixel; sample++)
      {
        // calculating the factor just for this pixel
        uint8_t factorValue = factor->_bytes[factor->_bytesPerRow*y+x];
        float factorPercent = factorValue/(float)0xFF;
        uint16_t pixelRadius = factorPercent*radius;
        radiusDivisor = abs(pixelRadius);
        radiusDivisor = 1+2*radiusDivisor;
        
        value = 0;
        // run the other samples
        for(int64_t ys=(int64_t)y-pixelRadius; ys<=(int64_t)y+pixelRadius; ys++)
        {
          int64_t ysReal = ys;
          if(ysReal < 0)
            ysReal = 0;
          else if(ysReal >= _height)
            ysReal = _height-1;
          
          value += bytesOriginal[(uint64_t)ysReal*_bytesPerRow+(uint64_t)x*_samplesPerPixel+sample];
        }
        
        value /= radiusDivisor;
        _bytes[(uint64_t)y*_bytesPerRow+(uint64_t)x*_samplesPerPixel+sample] = value;
      } // for sample
    } // for y
  } // for x
  
  free(bytesOriginal);
  return true;
}