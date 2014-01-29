//
//  main.m
//  rpnr
//
//  Created by Lucas Menge on 12/18/13.
//  Copyright (c) 2013 Lucas Menge. All rights reserved.
//

#include "LMImageRepresentation.h"
#include "IO.h"

void LMPrintHelp()
{
  std::cout
  << "RPNR: Render Pass Noise Reduction" << std::endl << std::endl
  
  << "Usage: rpnr -i folder [-e ending] [[-fradius | -fbnoisy | -fbao | -tolmin | -tolmax | -tolz | -toldiffuse | -tolglossy | toltranslucent] number] [-saveaux]" << std::endl << std::endl

  << "-i: The input path" << std::endl << "\tThis should point to a folder with the separate render passes." << std::endl

  << "-e: The input files terminators" << std::endl << "\tDefines the terminators of the file names. Default value is \".png\"." << std::endl

  << "-fradius: The main algorithm radius" << std::endl << "\tThe higher the value, the less detail within a surface." << std::endl
  << "\tBlender-specific value suggestions below:" << std::endl
  << "\t2700p image @ 100 samples:  25 (~1 minute)" << std::endl
  << "\t1080p image @ 100 samples:  15 (~6 seconds)" << std::endl
  << "\t356p image @ 100 samples:   10" << std::endl
  << "\t540p image @ 225 samples:   10" << std::endl
  << "\t356p image @ 400 samples:    7" << std::endl

  << "-fbnoisy: The initial blur radius for noisy delimiters" << std::endl
  << "\tAt the moment, just the combined pass is re-used as a noisy delimiter." << std::endl

  << "-fbao: The blur radius for the Ambient Occlusion delimiter" << std::endl
  << "\tThe AO delimiter is used as noise tolerance regulator." << std::endl
  << "\tMore blur gives smoother, but potentially blurrier results." << std::endl

  << "-tolmin: The minimum tolerance to noise" << std::endl
  << "\tMinimum color deviation value that the noise tolerance will range." << std::endl
  << "\tThe algorithm will accept deltas of 0-tolmin or 0-tolmax as noise." << std::endl
  << "\tThe more samples you have, the lower this can be." << std::endl
  << "\tValues below 4 can cause non-occluded areas to nave no noise reduction." << std::endl

  << "-tolmax: The maximum tolerance to noise" << std::endl
  << "\tMaximum color deviation value that the noise tolerance will range." << std::endl
  << "\tThe algorithm will accept deltas of 0-tolmin or 0-tolmax as noise." << std::endl
  << "\tThe more samples you have, the lower this can be." << std::endl
  << "\tValues below 20 might clip the algorithm causing noise." << std::endl

  << "-tolz: The Z color difference at which we break a surface" << std::endl
  << "\tThe higher this value, the easier we'll consider different surfaces on" << std::endl
  << "\tthe Z delimiter as a single surface." << std::endl
  << "\t4 is good enough for most topologies. Anything below might cause banding" << std::endl
  << "\ton very inclined surfaces" << std::endl

  << "-toldiffuse: The diffuse color difference at which we break a surface" << std::endl
  << "\tThe higher, the blurrier each texture. 4 is a good value for most." << std::endl
  
  << "-tolnormal: The normal color difference at which we break a surface" << std::endl
  << "\tThe higher, the blurrier each surface. 4 is a good value for most." << std::endl

  << "-tolglossy: The glossy color difference at which we break a surface" << std::endl
  << "\tSince this input is mostly white and noisy, tolerance should be high." << std::endl

  << "-toltranslucent: The translucent color differece at which we break a surface" << std::endl
  << "\tSince this is noisy, tolerance should be high for non-noisy refractions." << std::endl
  
  << "-edgeblurweight: The weight of the blur on the surface edges (0-100)" << std::endl
  << "\tThe higher, the blurrier surface edges will be, but less noisy." << std::endl

  << "-saveaux: Tells the program to save any intermediate image generated" << std::endl;
}

typedef struct _LMDelimiter
{
  const char* fileName;
  bool isNoisy;
  bool isSurfaceSource;
  LMImageRepresentation* image;
} LMDelimiter;

void LMFreeDelimiters(LMDelimiter* delimiters, int count)
{
  for(int i=0; i<count; i++)
  {
    if(delimiters[i].image != NULL)
    {
      delete delimiters[i].image;
      delimiters[i].image = NULL;
    }
  }
}

int main(int argc, const char * argv[])
{
  const char* workingPath = "";
  const char* fileEndings = ".png";
  
  // blurs the combined input for use as delimiter
  // 5 is a good value. larger values make the image blurrier overall. lower cause more noise
  int blurFilterNoisyDelimiter = 5;
  // blurs the original AO input to attempt to reduce noise on it
  // it's lo-fi because we only use to regulate tolerance
  // 7 is a pretty good value
  int blurAODelimiter = 7;
  // the more you blur, the less detail within a surface you get
  // 2700p image @ 100 samples, 25 is good (1 minute)
  // 1080p image @ 100 samples, 15 is good (6 seconds)
  // 33% of 1080p @ 100 samples, 10 is good
  // 50% of 1080p @ 225 samples, 10 is good
  // 33% of 1080p @ 400 samples, 7 is good
  int blurFilterRadius = 10;
  // the higher the tolerance, the higher amplitude the noise will be gone, but we lose detail
  // toleranceMin tweaks noise in low occlusion areas
  // toleranceMax tweaks noise in highly occluded areas
  // 100 samples, 16-28 is good. 200 samples, 8-28 is good. 400 samples, 4-28 is good
  int toleranceMin = 16;
  int toleranceMax = 28;
  // the higher the tolerance, the less Z-fighting on inclined surfaces with high blurCombined
  // but the higher, the more likely to confuse parallel surfaces with different Z coordinates
  // 4 is usually good for the defaults above. if you go with larger blurCombined values, you'll need to raise this
  int zTolerance = 4;
  // the higher the tolerance, the more noise will be taken out of textured surfaces.
  // textures will become blurrier, too. 4 is the default
  int diffuseTolerance = 4;
  // the higher the tolerance, the more noise will be taken out of irregular surfaces.
  // surface irregularities will become blurrier, too. 16 is the default
  int normalTolerance = 16;
  // the higher the tolerance, the more noise will be taken out of the glossy reflections.
  // glossy reflections will be blurrier, too. 96 is the default
  int glossyTolerance = 96;
  // the higher the tolerance, the more noise will be taken out of the refractions.
  // refractions will be blurrier, too. 96 is the default
  int translucentTolerance = 96;
  // the higher the edge blur weight, the more blurry surface edges will be (0-100).
  int edgeBlurWeight = 50;
  // saves each file that was processed to the disk so you see what was done
  bool saveAuxiliaryFiles = false;
  
  // parsing command line arguments
  for(int i=0; i<argc; i++)
  {
    // not a parameter
    if(argv[i][0] != '-')
      continue;
    // define if we might have a value afterwards
    bool mightHaveValue = false;
    if(i <= argc-1)
      mightHaveValue = true;
    // check for the parameters
    if(mightHaveValue == true)
    {
      if(strcmp(argv[i], "-fbnoisy") == 0)
      {
        blurFilterNoisyDelimiter = atoi(argv[i+1]);
        i++;
        continue;
      }
      else if(strcmp(argv[i], "-fbao") == 0)
      {
        blurAODelimiter = atoi(argv[i+1]);
        i++;
        continue;
      }
      else if(strcmp(argv[i], "-fradius") == 0)
      {
        blurFilterRadius = atoi(argv[i+1]);
        i++;
        continue;
      }
      else if(strcmp(argv[i], "-tolmin") == 0)
      {
        toleranceMin = atoi(argv[i+1]);
        i++;
        continue;
      }
      else if(strcmp(argv[i], "-tolmax") == 0)
      {
        toleranceMax = atoi(argv[i+1]);
        i++;
        continue;
      }
      else if(strcmp(argv[i], "-tolz") == 0)
      {
        zTolerance = atoi(argv[i+1]);
        i++;
        continue;
      }
      else if(strcmp(argv[i], "-toldiffuse") == 0)
      {
        diffuseTolerance = atoi(argv[i+1]);
        i++;
        continue;
      }
      else if(strcmp(argv[i], "-tolnormal") == 0)
      {
        normalTolerance = atoi(argv[i+1]);
        i++;
        continue;
      }
      else if(strcmp(argv[i], "-tolglossy") == 0)
      {
        glossyTolerance = atoi(argv[i+1]);
        i++;
        continue;
      }
      else if(strcmp(argv[i], "-toltranslucent") == 0)
      {
        translucentTolerance = atoi(argv[i+1]);
        i++;
        continue;
      }
      else if(strcmp(argv[i], "-edgeblurweight") == 0)
      {
        edgeBlurWeight = atoi(argv[i+1]);
        if(edgeBlurWeight < 0)
          edgeBlurWeight = 0;
        else if(edgeBlurWeight > 100)
          edgeBlurWeight = 100;
        i++;
        continue;
      }
      else if(strcmp(argv[i], "-i") == 0)
      {
        workingPath = argv[i+1];
        i++;
        continue;
      }
      else if(strcmp(argv[i], "-e") == 0)
      {
        fileEndings = argv[i+1];
        i++;
        continue;
      }
    }
    if(strcmp(argv[i], "-saveaux") == 0)
    {
      saveAuxiliaryFiles = true;
      continue;
    }
    if(strcmp(argv[i], "-h") == 0
       || strcmp(argv[i], "-help") == 0
       || strcmp(argv[i], "--help") == 0)
    {
      LMPrintHelp();
      exit(0);
    }
  }
  
  if(workingPath == NULL)
  {
    LMPrintHelp();
    exit(0);
  }
  
  // confirming commands
  std::cout << "RPNR: Render Pass Noise Reduction" << std::endl << std::endl;
  std::cout << "Input Path (-i): \"" << workingPath << "\"" << std::endl;
  std::cout << "Input File endings (-e): \"" << fileEndings << "\"" << std::endl;
  std::cout << "Output radius (-fradius): " << blurFilterRadius << std::endl;
  std::cout << "Noisy delimiter radius (-fbnoisy): " << blurFilterNoisyDelimiter << std::endl;
  std::cout << "Ambient Occlusion delimiter radius (-fbao): " << blurAODelimiter << std::endl;
  std::cout << "Minimum delimiter noise tolerance (-tolmin): " << toleranceMin << std::endl;
  std::cout << "Maximum delimiter noise tolerance (-tolmax): " << toleranceMax << std::endl;
  std::cout << "Z delimiter surface tolerance (-tolz): " << zTolerance << std::endl;
  std::cout << "Diffuse delimiter tolerance (-toldiffuse): " << diffuseTolerance << std::endl;
  std::cout << "Normal delimiter tolerance (-tolnormal): " << normalTolerance << std::endl;
  std::cout << "Glossy delimiters tolerance (-tolglossy): " << glossyTolerance << std::endl;
  std::cout << "Translucent delimiter tolerance (-toltranslucent): " << translucentTolerance << std::endl;
  std::cout << "Edge blur weight (-edgeblurweight): " << edgeBlurWeight << std::endl;
  std::cout << "Save aux files (-saveaux): " << (saveAuxiliaryFiles?"YES":"NO") << std::endl << std::endl;
  
  /*
   Layers to get as input. The more, the more complete.
   Only required right now are Combined and Ambient Occlusion (which can be replaced by a white image)
   
   Combined               - helps differentiate shades
   Ambient Occlusion      - helps know where there is higher amplitude noise
   Normal                 - helps differentiate surfaces
   Z                      - helps differentiate surfaces
   Diffuse Color          - helps differentiate surfaces/textures
   Glossy Direct          - don’t blur glossy highlights
   Glossy Indirect        - reflections
   Transmission Indirect  - see through glass
   Emission               - if emissors are behind objects
  */
  LMDelimiter delimiters[] = {
    // {"filename", isNoisy, isSurfaceSource, image}
    {"combined",             true,  false, NULL},
    {"ao",                   false, false, NULL},
    {"diffusecolor",         false, false, NULL},
    {"normal",               false, false, NULL},
    {"z",                    false, true,  NULL},
    {"glossydirect",         false, false, NULL},
    {"glossyindirect",       false, false, NULL},
    {"transmissionindirect", false, false, NULL},
    {"emission",             false, false, NULL}
  };
  int maximumDelimiterCount = 9;
  
  int validDelimiterCount = 0;
  int surfaceDelimiterCount = 0;
  for(int i=0; i<maximumDelimiterCount; i++)
  {
    char* fileName = (char*)calloc(strlen(delimiters[i].fileName)+strlen(fileEndings)+1, sizeof(char));
    strcat(fileName, delimiters[i].fileName);
    strcat(fileName, fileEndings);
    std::cout << "Loading " << delimiters[i].fileName << " (" << fileName << ")" << "... " << std::endl;
    delimiters[i].image = loadImageData(workingPath, fileName);
    if(delimiters[i].image != NULL)
    {
      // loaded
      validDelimiterCount++;
      if(delimiters[i].isSurfaceSource == true)
        surfaceDelimiterCount++;
      if(i == 0)
        std::cout << "OK." <<  std::endl;
      else
      {
        // make sure the images match size
        if(delimiters[i].image->_width != delimiters[0].image->_width
           || delimiters[i].image->_height != delimiters[0].image->_height)
        {
          std::cout << "FAIL! All delimiters must have the same dimensions." << std::endl;
          
          free(fileName);
          LMFreeDelimiters(delimiters, maximumDelimiterCount);
          exit(3);
        }
        else
          std::cout << "OK." <<  std::endl;
      }
      
      bool wasProcessed = false;
      // generic noisy processing
      if(delimiters[i].isNoisy == true)
      {
        std::cout << "Processing " << delimiters[i].fileName << "... " << std::endl;
        delimiters[i].image->blur(blurFilterNoisyDelimiter);
        wasProcessed = true;
        std::cout << "OK." << std::endl;
      }
      
      // layer-specific processing
      if(i == 1)
      {
        // ao delimiter
        std::cout << "Processing " << delimiters[i].fileName << "... " << std::endl;
        delimiters[i].image->convertFromColorToGrayscale();
        delimiters[i].image->blur(blurAODelimiter);
        delimiters[i].image->invert();
        wasProcessed = true;
        std::cout << "OK." << std::endl;
      }
      else if(i == 2)
        // diffuse delimiter
        delimiters[i].image->_toleranceOverride = diffuseTolerance;
      else if(i == 3)
        // normal delimiter
        delimiters[i].image->_toleranceOverride = normalTolerance;
      else if(i == 4)
      {
        // z delimiter
        delimiters[i].image->_toleranceOverride = zTolerance;
        delimiters[i].image->convertFromColorToGrayscale();
      }
      else if(i == 5 || i == 6)
        // glossy delimiters
        delimiters[i].image->_toleranceOverride = glossyTolerance;
      else if(i == 7)
        // translucent indirect delimiters
        delimiters[i].image->_toleranceOverride = translucentTolerance;
      else if(i == 8)
        // emission tolerance
        delimiters[i].image->_toleranceOverride = diffuseTolerance;
      
      // saving when processed
      if(wasProcessed == true && saveAuxiliaryFiles == true)
      {
        char* altFileName = (char*)calloc(strlen(delimiters[i].fileName)+strlen(fileEndings)+2, sizeof(char));
        if(altFileName != NULL)
        {
          strcat(altFileName, "_");
          strcat(altFileName, delimiters[i].fileName);
          strcat(altFileName, fileEndings);
          std::cout << "Saving " << altFileName << "... " << std::endl;
          saveImageData(delimiters[i].image, workingPath, altFileName);
          std::cout << "OK." << std::endl;
          free(altFileName);
        }
      }
    }
    else
    {
      // failed to load
      if(i == 0 || i == 1)
      {
        // this file was necessary
        std::cout << "FAILED to load " << fileName << "!" << std::endl;
        std::cout << "The " << delimiters[0].fileName << " and " << delimiters[1].fileName << " files are necessary for processing. Please make sure both are in the specified path." << std::endl;

        free(fileName);
        LMFreeDelimiters(delimiters, maximumDelimiterCount);
        exit(2);
      }
      else
        // this file wasn't necessary
        std::cout << "Absent." << std::endl;
    }
    free(fileName);
  }
  
  std::cout << "Loading the working image: " << delimiters[0].fileName << "... " << std::endl;
  char* fileName = (char*)calloc(strlen(delimiters[0].fileName)+strlen(fileEndings)+1, sizeof(char));
  strcat(fileName, delimiters[0].fileName);
  strcat(fileName, fileEndings);
  LMImageRepresentation* workingImage = loadImageData(workingPath, fileName);
  LMImageRepresentation* referenceImage = loadImageData(workingPath, fileName);
  free(fileName);
  if(workingImage == NULL || referenceImage == NULL)
  {
    std::cout << "FAIL!" << std::endl << std::endl;
    
    if(workingImage != NULL)
      delete workingImage;
    if(referenceImage != NULL)
      delete referenceImage;
    LMFreeDelimiters(delimiters, maximumDelimiterCount);
    exit(4);
  }
  else
    std::cout << "OK!" << std::endl << std::endl;
  
  std::cout << "Building valid delimiters..." << std::endl;
  // build the list for all
  int d = 0;
  const LMImageRepresentation** validDelimiters = (const LMImageRepresentation**)calloc(validDelimiterCount, sizeof(LMImageRepresentation*));
  for(int i=0; i<maximumDelimiterCount; i++)
  {
    if(i == 1)
      // discard AO
      continue;
    if(delimiters[i].image != NULL)
    {
      std::cout << "Using " << delimiters[i].fileName << std::endl;
      validDelimiters[d] = delimiters[i].image;
      d++;
    }
  }
  validDelimiterCount = d;
  std::cout << std::endl;
  
  std::cout << "Filtering surfaces... " << std::endl;
  if(workingImage->blurSurfaces(
                                blurFilterRadius,
                                delimiters[1].image,
                                validDelimiters, validDelimiterCount,
                                toleranceMin, toleranceMax
                                ))
  {
    std::cout << "OK." << std::endl;
    
    std::cout << "Filtering edges... " << std::endl;
    workingImage->blurEdges(edgeBlurWeight/100.0f,
                            referenceImage,
                            delimiters[0].image,
                            delimiters[2].image,
                            delimiters[3].image,
                            delimiters[4].image,
                            delimiters[8].image);
    std::cout << "OK." << std::endl;
    
    std::cout << "Saving result... " << std::endl;
    if(saveImageData(workingImage, workingPath, "__combined.png") == true)
      std::cout << "OK." << std::endl;
    else
      std::cout << "FAIL!" << std::endl;
  }
  else
  {
    std::cout << "FAIL!" << std::endl;
    
    if(workingImage != NULL)
      delete workingImage;
    if(referenceImage != NULL)
      delete referenceImage;
    LMFreeDelimiters(delimiters, maximumDelimiterCount);
    free(validDelimiters);
    exit(1);
  }
  
  if(workingImage != NULL)
    delete workingImage;
  if(referenceImage != NULL)
    delete referenceImage;
  LMFreeDelimiters(delimiters, maximumDelimiterCount);
  free(validDelimiters);
  return 0;
}

