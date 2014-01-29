//
//  IO.OSX.c
//  rpnr
//
//  Created by Lucas Menge on 12/27/13.
//  Copyright (c) 2013 Lucas Menge. All rights reserved.
//

#if (TARGET_OS_MAC)

#include "LMImageRepresentation.h"

LMImageRepresentation* loadImageData(const char* workingPath, const char* imageName)
{
  @autoreleasepool
  {
    NSString* path = [[NSString stringWithCString:workingPath encoding:NSUTF8StringEncoding] stringByAppendingPathComponent:[NSString stringWithCString:imageName encoding:NSUTF8StringEncoding]];
    NSImage* image = [[NSImage alloc] initWithContentsOfFile:path];
    if(image == nil)
      return NULL;
    NSBitmapImageRep* imageRep = [NSBitmapImageRep imageRepWithData:[image TIFFRepresentation]];
    if(imageRep == nil)
      return NULL;
    LMImageRepresentation* rep = new LMImageRepresentation(imageRep.bitmapData, imageRep.bitsPerPixel, imageRep.bytesPerRow, imageRep.bytesPerPlane, imageRep.samplesPerPixel);
    return rep;
  }
}

bool saveImageData(LMImageRepresentation* rep, const char* workingPath, const char* imageName)
{
  @autoreleasepool
  {
    NSString* colorSpace = nil;
    if(rep->_samplesPerPixel == 1)
      colorSpace = NSDeviceWhiteColorSpace;
    else if(rep->_samplesPerPixel == 3)
      colorSpace = NSDeviceRGBColorSpace;
    else
    {
      NSLog(@"Unsupported colorSpace for saving");
      return false;
    }
    
    NSString* path = [[NSString stringWithCString:workingPath encoding:NSUTF8StringEncoding] stringByAppendingPathComponent:[NSString stringWithCString:imageName encoding:NSUTF8StringEncoding]];
    
    NSBitmapImageRep* imageRep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:&(rep->_bytes)
                                                                         pixelsWide:rep->_width
                                                                         pixelsHigh:rep->_height
                                                                      bitsPerSample:rep->_bitsPerPixel/rep->_samplesPerPixel
                                                                    samplesPerPixel:rep->_samplesPerPixel
                                                                           hasAlpha:NO
                                                                           isPlanar:NO
                                                                     colorSpaceName:colorSpace
                                                                        bytesPerRow:rep->_bytesPerRow
                                                                       bitsPerPixel:rep->_bitsPerPixel];
    if(path == nil || imageRep == nil)
      return false;
    [[imageRep representationUsingType:NSPNGFileType properties:nil] writeToFile:path atomically:NO];
    return true;
  }
}

#endif