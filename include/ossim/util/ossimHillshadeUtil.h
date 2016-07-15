//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#ifndef ossimHillshadeUtil_HEADER
#define ossimHillshadeUtil_HEADER 1

#include <ossim/base/ossimConstants.h>
#include <ossim/base/ossimReferenced.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/imaging/ossimImageSource.h>
#include <ossim/imaging/ossimSingleImageChain.h>
#include <ossim/imaging/ossimImageFileWriter.h>
#include <ossim/projection/ossimMapProjection.h>
#include <ossim/util/ossimChipProcUtil.h>

// Forward class declarations:
class ossimArgumentParser;
class ossimDpt;
class ossimFilename;
class ossimGeoPolygon;
class ossimGpt;
class ossimImageData;
class ossimImageFileWriter;
class ossimImageGeometry;
class ossimImageViewAffineTransform;
class ossimIrect;
class ossimKeywordlist;

class OSSIM_DLL ossimHillshadeUtil : public ossimChipProcUtil
{
public:
   /** default constructor */
   ossimHillshadeUtil();

   /** virtual destructor */
   virtual ~ossimHillshadeUtil();

   virtual void setUsage(ossimArgumentParser& ap);

   virtual bool initialize(ossimArgumentParser& ap);

   virtual ossimString getClassName() const { return "ossimHillshadeUtil"; }

   /** Used by ossimUtilityFactory */
   static const char* DESCRIPTION;

protected:

   virtual void initProcessingChain();

   /** @brief Hidden from use copy constructor. */
   ossimHillshadeUtil( const ossimHillshadeUtil& obj );

   /** @brief Hidden from use assignment operator. */
   const ossimHillshadeUtil& operator=( const ossimHillshadeUtil& rhs );
};

#endif /* #ifndef ossimHillshadeUtil_HEADER */
