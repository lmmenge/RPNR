//
//  LMImageRepresentation.cpp
//  rpnr
//
//  Created by Lucas Menge on 12/18/13.
//  Copyright (c) 2013 Lucas Menge. All rights reserved.
//

#include "LMImageRepresentation.h"

#include <iostream>

LMImageRepresentation::LMImageRepresentation(uint8_t* bytes, uint8_t bitsPerPixel, uint64_t bytesPerRow, uint64_t bytesPerPlane, uint8_t samplesPerPixel)
{
  _bytes = (uint8_t*)calloc(bytesPerPlane, sizeof(uint8_t));
  memcpy(_bytes, bytes, bytesPerPlane);
  _bitsPerPixel = bitsPerPixel;
  _bytesPerRow = bytesPerRow;
  _bytesPerPlane = bytesPerPlane;
  _samplesPerPixel = samplesPerPixel;
  
  _width = (uint32_t)(_bytesPerRow/(_bitsPerPixel/8));
  _height = (uint32_t)(_bytesPerPlane/_bytesPerRow);
  
  _toleranceOverride = 0;
}

LMImageRepresentation::LMImageRepresentation(LMImageRepresentation* other)
{
  _bytes = (uint8_t*)calloc(1, other->_bytesPerPlane);
  memcpy(_bytes, other->_bytes, other->_bytesPerPlane);
  _bitsPerPixel = other->_bitsPerPixel;
  _bytesPerRow = other->_bytesPerRow;
  _bytesPerPlane = other->_bytesPerPlane;
  _samplesPerPixel = other->_samplesPerPixel;
  
  _width = other->_width;
  _height = other->_height;
  
  _toleranceOverride = other->_toleranceOverride;
}

LMImageRepresentation::~LMImageRepresentation()
{
  if(_bytes != NULL)
  {
    free(_bytes);
    _bytes = NULL;
  }
}

bool LMImageRepresentation::isPixelEdgePixel(int64_t x, int64_t y) const
{
  for(uint8_t sample=0; sample<_samplesPerPixel; sample++)
  {
    uint64_t sampleIndex = y*_bytesPerRow+x*_samplesPerPixel+sample;
    for(int8_t xD=-1; xD<2; xD++)
    {
      if(((int64_t)x)+xD < 0 || ((int64_t)x)+xD >= _width)
        continue;
      for(int8_t yD=-1; yD<2; yD++)
      {
        if(((int64_t)y)+yD < 0 || ((int64_t)y)+yD >= _height)
          continue;
        
        uint64_t otherSampleIndex = (y+yD)*_bytesPerRow+(x+xD)*_samplesPerPixel+sample;
        if(abs((int16_t)(_bytes[sampleIndex]) - (int16_t)(_bytes[otherSampleIndex])) >= _toleranceOverride)
          return true;
      }
    }
  }
  return false;
}

bool LMImageRepresentation::invert()
{
  if(_bitsPerPixel == 8)
  {
    for(uint64_t i=0; i<_bytesPerPlane; i++)
    {
      _bytes[i] = 0xFF-_bytes[i];
    }
  }
  else
  {
    std::cout << "Unsupported image format" << std::endl;
    return false;
  }
  return true;
}

bool LMImageRepresentation::convertFromColorToGrayscale()
{
  // TODO: this considers alpha channel as a valid color sample. maybe ignore it?
  if(_bytes == NULL)
    return false;
  
  uint8_t* newBytes = (uint8_t*)calloc(_bytesPerPlane/_samplesPerPixel, sizeof(uint8_t));
  uint8_t bitsPerSample = _bitsPerPixel/_samplesPerPixel;
  if(bitsPerSample == 8)
  {
    for(uint64_t i=0; i<_bytesPerPlane; i+=_samplesPerPixel)
    {
      uint16_t value = 0;
      for(uint8_t sample=0; sample<_samplesPerPixel; sample++)
        value += _bytes[i+sample];
      value /= _samplesPerPixel;
      
      newBytes[i/_samplesPerPixel] = value;
    }
  }
  else if(bitsPerSample == 16)
  {
    uint16_t* samples16 = (uint16_t*)_bytes;
    uint16_t* newSamples16 = (uint16_t*)newBytes;
    uint64_t totalSamples = (uint64_t)_width*_height*_samplesPerPixel;
    for(uint64_t i=0; i<totalSamples; i+=_samplesPerPixel)
    {
      uint32_t value = 0;
      for(uint8_t sample=0; sample<_samplesPerPixel; sample++)
        value += samples16[i+sample];
      value /= _samplesPerPixel;
      
      newSamples16[i/_samplesPerPixel] = value;
      //std::cout << samples16[i] << " " << newSamples16[i/_samplesPerPixel] << "\n";
    }
  }
  else
  {
    std::cout << "Don't know how to convert this image composition\n";
    free(newBytes);
    return false;
  }
  free(_bytes);
  _bytes = newBytes;
  
  _bitsPerPixel /= _samplesPerPixel;
  _bytesPerRow /= _samplesPerPixel;
  _bytesPerPlane /= _samplesPerPixel;
  _samplesPerPixel /= _samplesPerPixel;
  return true;
}

bool LMImageRepresentation::convertFromGrayscale8ToColor24()
{
  if(_bytes == NULL || _samplesPerPixel != 1)
    return false;
  
  uint8_t* newBytes = (uint8_t*)calloc(1, _bytesPerPlane*3); // potential overflow at _bytesPerPlane*3 if image is huge
  for(uint64_t i=0; i<_bytesPerPlane; i++)
  {
    newBytes[i*3] = _bytes[i];
    newBytes[i*3+1] = _bytes[i];
    newBytes[i*3+2] = _bytes[i];
  }
  free(_bytes);
  _bytes = newBytes;
  
  // potential overflow at all these below if any of them are sufficiently large
  _bitsPerPixel *= 3;
  _bytesPerRow *= 3;
  _bytesPerPlane *= 3;
  _samplesPerPixel *= 3;
  return true;
}

void LMImageRepresentation::printDebugInfo() const
{
  std::cout << "bpp: " << _bitsPerPixel << "\nBpr: " << _bytesPerRow << "\nBpP: " << _bytesPerPlane << "\nspp: " << _samplesPerPixel << "\n";
  std::cout << "width: " << _width << "\nheight: " << _height << "\n";
  
  uint64_t pixelsToPrint = (uint64_t)_width*_height;
  if(pixelsToPrint > 10)
    pixelsToPrint = 10;
  for(uint64_t i=0; i<pixelsToPrint*_samplesPerPixel; i+=_samplesPerPixel)
  {
    for(uint8_t sample=0; sample<_samplesPerPixel; sample++)
      std::cout << _bytes[i+sample] << " ";
    std::cout << "\n";
  }
}

void LMImageRepresentation::checkData() const
{
  uint64_t ok = 0;
  uint64_t uninit = 0;
  uint64_t freed = 0;
  for(uint64_t i=0; i<_bytesPerPlane; i++)
  {
    if(_bytes[i] == 0xAA)
      uninit++;
    else if(_bytes[i] == 0x55)
      freed++;
    else
      ok++;
  }
  if(uninit != 0 || freed != 0)
  {
    std::cout << "OK " << ok << "; uninit " << uninit << "; freed " << freed << std::endl;
  }
}
