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

#ifndef ossimTiffPhotoInterpLut_H
#define ossimTiffPhotoInterpLut_H 1

#include <ossim/base/ossimLookUpTable.h>

class OSSIMDLLEXPORT ossimTiffPhotoInterpLut : public ossimLookUpTable
{
public:
   
   ossimTiffPhotoInterpLut();
   
   virtual ossimKeyword getKeyword() const;
};

#endif
