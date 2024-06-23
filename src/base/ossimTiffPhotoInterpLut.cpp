//---
//
// License: MIT
//
// Author: David Burken
//
// Description:
// 
// Lookup table to map a tiff photometric interpretation code
// to a string.
//
// Codes from libtiff tiff.h.
//---
// $Id$

#include <ossim/base/ossimTiffPhotoInterpLut.h>
#include <ossim/base/ossimTiffConstants.h>
#include <ossim/base/ossimString.h>

static const int TABLE_SIZE = 13;

//*******************************************************************
// Public Constructor:
//*******************************************************************
ossimTiffPhotoInterpLut::ossimTiffPhotoInterpLut()
   :
      ossimLookUpTable(TABLE_SIZE)
{
   //---
   // Complete initialization of data member "theTable".
   // Note: Enums defined in ossimTiffConstants.h file.
   //---
   theTable[0].theKey    = ossim::PHOTO_MINISWHITE;
   theTable[0].theValue  = "MINISWHITE";
   
   theTable[1].theKey    = ossim::PHOTO_MINISBLACK;
   theTable[1].theValue  = "MINISBLACK";
   
   theTable[2].theKey    = ossim::PHOTO_RGB;
   theTable[2].theValue  = "RGB";
   
   theTable[3].theKey    = ossim::PHOTO_PALETTE;
   theTable[3].theValue  = "PALETTE";
   
   theTable[4].theKey    = ossim::PHOTO_MASK;
   theTable[4].theValue  = "MASK";
   
   theTable[5].theKey    = ossim::PHOTO_SEPARATED;
   theTable[5].theValue  = "SEPARATED";
   
   theTable[6].theKey    = ossim::PHOTO_YCBCR;
   theTable[6].theValue  = "YCBCR";
   
   theTable[7].theKey    = ossim::PHOTO_CIELAB;
   theTable[7].theValue  = "CIELAB";
   
   theTable[8].theKey    = ossim::PHOTO_ICCLAB;
   theTable[8].theValue  = "ICCLAB";
   
   theTable[9].theKey    = ossim::PHOTO_ITULAB;
   theTable[9].theValue  = "ITULAB";
   
   theTable[10].theKey   = ossim::PHOTO_CFA;
   theTable[10].theValue = "CFA";
   
   theTable[11].theKey   = ossim::PHOTO_LOGL;
   theTable[11].theValue = "LOGL";
   
   theTable[12].theKey   = ossim::PHOTO_LOGLUV;
   theTable[12].theValue = "LOGLUV";
}

ossimKeyword ossimTiffPhotoInterpLut::getKeyword() const
{
   return ossimKeyword("photo_interpretation", "");
}
