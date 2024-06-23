//---
//
// License: MIT
//
// File: ossim-tiff-photo-interp-lut-test.cpp
//---
// $Id$

#include <ossim/base/ossimArgumentParser.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimString.h>
#include <ossim/base/ossimTiffPhotoInterpLut.h>
#include <ossim/init/ossimInit.h>
#include <iostream>

int main(int argc, char *argv[])
{
   ossimArgumentParser ap(&argc, argv);

   // Initialize ossim stuff, factories, plugin, etc.
   ossimInit::instance()->initialize(ap);
   try
   {
      // Put your code here:
      ossimTiffPhotoInterpLut lut;
      ossim_uint32 TS = lut.getTableSize();
      std::cout << "table_size: " << TS << "\n";
      ossimString os;
      ossim_uint32 en;
      for(ossim_uint32 i = 0; i < TS; ++i)
      {
         os = lut.getTableIndexString(i);
         en = lut.getEntryNumber( os.c_str(), true );
         std::cout << "table_index: " << i
                   << "\ntable_index_string:   " << os
                   << "\nentry_key: " << en
                   << "\nentry_string: " << lut.getEntryString( en )
                   << "\n\n";
      }
      std::cout << std::endl;
   }
   catch(const std::exception& e)
   {
      ossimNotify(ossimNotifyLevel_WARN) << e.what() << std::endl;
      exit(1);
   }
   return 0;
}
