//
//  IO.h
//  rpnr
//
//  Created by Lucas Menge on 12/27/13.
//  Copyright (c) 2013 Lucas Menge. All rights reserved.
//

#ifndef rpnr_IO_h
#define rpnr_IO_h

class LMImageRepresentation;

extern LMImageRepresentation* loadImageData(const char* workingPath, const char* imageName);
extern bool saveImageData(LMImageRepresentation* rep, const char* workingPath, const char* imageName);

#endif
