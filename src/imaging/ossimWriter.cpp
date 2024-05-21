//---
//
// License: MIT
// 
// Author:  David Burken
//
// Description: Generic image writer class.
// 
//---
// $Id$

#include <ossim/imaging/ossimWriter.h>
#include <ossim/base/ossimBooleanProperty.h>
#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimProperty.h>
#include <ossim/base/ossimStringProperty.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimTiffConstants.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/imaging/ossimImageGeometry.h>
#include <ossim/projection/ossimMapProjection.h>
#include <ossim/projection/ossimProjection.h>

#include <limits>
#include <ostream>

static const std::string ALIGN_TILES_KW         = "align_tiles";         // bool
static const std::string BLOCK_SIZE_KW          = "block_size";          // unsigned int
static const std::string FLUSH_TILES_KW         = "flush_tiles";         // bool
static const std::string INCLUDE_BLANK_TILES_KW = "include_blank_tiles"; // bool
static const std::string TILE_SIZE_KW           = "tile_size";           // (x,y) in pixels
static const std::string TRUE_KW                = "true";

static const ossimTrace traceDebug("ossimWriter:debug");

ossimWriter::ossimWriter()
   : ossimImageFileWriter(),
     m_str(0),
     m_ownsStreamFlag(false),
     m_kwl(new ossimKeywordlist()),
     m_outputTileSize(OSSIM_DEFAULT_TILE_WIDTH, OSSIM_DEFAULT_TILE_HEIGHT)
{
   // Set default options:
   ossim::defaultTileSize(m_outputTileSize);
   
   m_kwl->addPair( ALIGN_TILES_KW, TRUE_KW );
   m_kwl->addPair( BLOCK_SIZE_KW, "4096" );
   m_kwl->addPair( FLUSH_TILES_KW, TRUE_KW );
   m_kwl->addPair( INCLUDE_BLANK_TILES_KW, TRUE_KW );
   m_kwl->addPair( TILE_SIZE_KW, m_outputTileSize.toString().string() );
}

ossimWriter::~ossimWriter()
{
   close();

   // Not a leak, ref ptr.
   m_kwl = 0;
}

void ossimWriter::close()
{
   if (m_str)      
   {
      m_str->flush();

      if (m_ownsStreamFlag)
      {
         delete m_str;
         m_str = 0;
         m_ownsStreamFlag = false;
      }
   }
}

ossimString ossimWriter::getShortName() const
{
   return ossimString("ossim_writer");
}

ossimString ossimWriter::getLongName() const
{
   return ossimString("ossim writer");
}

ossimString ossimWriter::getClassName() const
{
   return ossimString("ossimWriter");
}

ossimString ossimWriter::getExtension() const
{
   ossimString result = "";
   if ( theOutputImageType == "ossim_ttbs" ) // tiled tiff band separate
   {
      result = "tif";
   }
   return result;
}

void ossimWriter::getImageTypeList(std::vector<ossimString>& imageTypeList) const
{
   // imageTypeList.push_back(ossimString("tiff_tiled_band_separate"));
   imageTypeList.push_back(ossimString("ossim_ttbs")); // tmp drb
}

bool ossimWriter::isOpen() const
{
   return ( m_str ? true : false );
}

bool ossimWriter::open()
{
   bool status = false;
   
   close();

   if ( theFilename.size() && hasImageType( theOutputImageType ) )
   {
      std::ofstream* os = new std::ofstream();
      os->open( theFilename.c_str(), std::ios::out | std::ios::binary );
      if( os->is_open() )
      {
         m_str = os;
         m_ownsStreamFlag = true;
         status = true;
      }
      else
      {
         delete os;
         os = 0;
      }
   }

   return status;
}

bool ossimWriter::hasImageType(const ossimString& imageType) const
{
   bool result = false;
   if ( (imageType == "ossim_ttbs") || (imageType == "image/tif") )
   {
      result = true;
   }
   return result;
}

bool ossimWriter::writeFile()
{
   bool status = true;
   
   if( theInputConnection.valid() &&
       (getErrorStatus() == ossimErrorCodes::OSSIM_OK) )
   {
      //---
      // Check for stream if master process.
      // Note only the master process is used for writing...
      //---
      if( theInputConnection->isMaster() )
      {
         if (!isOpen())
         {
            status = open();
         }
      }

      if ( status )
      {
         status = writeStream();

         // Flush and close the stream.
         close();
      }
   }

   return status;
}

bool ossimWriter::writeStream()
{
   //---
   // This can be called publically so we must to the same checks as the
   // writeFile method.
   //---
   bool status = false;

   // Must have a sequencer...
   if( theInputConnection.valid() )
   {
      if ( isOpen() )
      {
         if ( theOutputImageType == "ossim_ttbs" )
         {
            if ( (theInputConnection->getTileWidth()  !=
                  static_cast<ossim_uint32>(m_outputTileSize.x)) ||
                 (theInputConnection->getTileHeight() !=
                  static_cast<ossim_uint32>(m_outputTileSize.y)) )
            {
               theInputConnection->setTileSize(m_outputTileSize);
            }
            
            status = writeStreamTtbs();
         }
      }
   }
   
   return status;
}

bool ossimWriter::writeStreamTtbs()
{
   // Alway big tiff in native byte order.
   
   bool status = false;

   if ( writeTiffHdr() == true )
   {
      std::vector<ossim_uint64>  tile_offsets;
      std::vector<ossim_uint64>  tile_byte_counts;

      //---
      // Min/max arrays must start off empty for
      // ossimImageData::computeMinMaxPix code.
      //---
      std::vector<ossim_float64> minBands(0);
      std::vector<ossim_float64> maxBands(0);
      
      if ( writeTiffTilesBandSeparate(
              tile_offsets, tile_byte_counts, minBands, maxBands   ) == true )
      {
         status = writeTiffTags( tile_offsets, tile_byte_counts, minBands, maxBands );
      }
   }
   
   return status;
}

bool ossimWriter::writeTiffHdr()
{
   //---
   // First two bytes, byte order indication.
   // "MM"(big endian) or "II"(little endian.
   //---
   std::string s;
   if ( ossim::byteOrder() == OSSIM_LITTLE_ENDIAN )
   {
      s = "II";
   }
   else
   {
      s = "MM";
   }
   m_str->write( s.c_str(), 2 );

   // Version, 42=classic tiff, 43=big tiff.
   ossim_uint16 us16 = 43;
   m_str->write( (const char*)&us16, 2 );

   // Byte size of offsets.
   us16 = 8;
   m_str->write( (const char*)&us16, 2 );

   // Always 0:
   us16 = 0;
   m_str->write( (const char*)&us16, 2 );

   // Offset to the IFD(image file directory).
   ossim_uint64 ul64 = 16;
   m_str->write( (const char*)&ul64, 8 );
   
   return m_str->good();
}

bool ossimWriter::writeTiffTags( const std::vector<ossim_uint64>& tile_offsets,
                                 const std::vector<ossim_uint64>& tile_byte_counts,
                                 const std::vector<ossim_float64>& minBands,
                                 const std::vector<ossim_float64>& maxBands )
{
   bool status = false;

   ossimRefPtr<ossimMapProjection> mapProj = 0;
   ossimRefPtr<ossimImageGeometry> geom = theInputConnection->getImageGeometry();
   if ( geom.valid() )
   {
      ossimRefPtr<ossimProjection> proj = geom->getProjection();
      mapProj = dynamic_cast<ossimMapProjection*>( proj.get() );
   }
   
   // Seek to the IFD.
   m_str->seekp( 16, std::ios_base::beg );
   
   // tag count, this will be rewritten at the end:
   ossim_uint64 tagCount = 0;
   m_str->write( (const char*)&tagCount, 8 );

   //---
   // This is where the tile offsets, tile byte counts and arrays bytes are
   // written. Starting at byte position 512 which gives from
   // 16 -> 512(496 bytes) to write tags.
   //---
   std::streamoff arrayWritePos = 512;

   // Used throughout:
   ossim_uint16 tag;
   ossim_uint16 type;
   ossim_uint64 count;
   ossim_uint16 value_ui16;
   ossim_uint32 value_ui32;
   
   // image width tag 256:
   tag   = ossim::TIFFTAG_IMAGEWIDTH;
   count = 1;
   if ( theAreaOfInterest.width() <= OSSIM_DEFAULT_MAX_PIX_UINT16 )
   {
      type = ossim::TIFF_SHORT;
      value_ui16 = (ossim_uint16)theAreaOfInterest.width();
      writeTiffTag<ossim_uint16>( tag, type, count, &value_ui16, arrayWritePos );
   }
   else
   {
      type = ossim::TIFF_LONG;
      value_ui32 = theAreaOfInterest.width();
      writeTiffTag<ossim_uint32>( tag, type, count, &value_ui32, arrayWritePos );
   }
   ++tagCount;
   
   // image length tag 257:
   tag   = ossim::TIFFTAG_IMAGELENGTH;
   count = 1;
   if ( theAreaOfInterest.height() <= OSSIM_DEFAULT_MAX_PIX_UINT16 )
   {
      type = ossim::TIFF_SHORT;
      value_ui16 = (ossim_uint16)theAreaOfInterest.height();
      writeTiffTag<ossim_uint16>( tag, type, count, &value_ui16, arrayWritePos );
   }
   else
   {
      type = ossim::TIFF_LONG;
      value_ui32 = theAreaOfInterest.height();
      writeTiffTag<ossim_uint32>( tag, type, count, &value_ui32, arrayWritePos );
   }
   ++tagCount;

   // bits per sample tag 258:
   tag   = ossim::TIFFTAG_BITSPERSAMPLE;
   count = theInputConnection->getNumberOfOutputBands();
   type  = ossim::TIFF_SHORT;
   value_ui16 = (ossim_uint16)ossim::getBitsPerPixel( theInputConnection->getOutputScalarType() );
   if ( count == 1 )
   {
      writeTiffTag<ossim_uint16>( tag, type, count, &value_ui16, arrayWritePos );
   }
   else
   {
      std::vector<ossim_uint16> v(count, value_ui16);
      writeTiffTag<ossim_uint16>( tag, type, count, &v.front(), arrayWritePos );
   }
   ++tagCount;

   // compression tag 259:
   tag   = ossim::TIFFTAG_COMPRESSION;
   type  = ossim::TIFF_SHORT;
   count = 1;
   value_ui16 = ossim::COMPRESSION_NONE; // tmp only uncompressed supported.
   writeTiffTag<ossim_uint16>( tag, type, count, &value_ui16, arrayWritePos );
   ++tagCount;
   
   // photo interpretation tag 262:
   tag   = ossim::TIFFTAG_PHOTOMETRIC;
   type  = ossim::TIFF_SHORT;
   count = 1;
   if ( theInputConnection->getNumberOfOutputBands() == 3 )
   {
      value_ui16 = ossim::PHOTO_RGB;
   }
   else
   {
      value_ui16 = ossim::PHOTO_MINISBLACK;
   }
   writeTiffTag<ossim_uint16>( tag, type, count, &value_ui16, arrayWritePos );
   ++tagCount;

   // samples per pixel tag 277:
   tag   = ossim::TIFFTAG_SAMPLESPERPIXEL;
   type  = ossim::TIFF_SHORT;
   count = 1;
   value_ui16 = theInputConnection->getNumberOfOutputBands();
   writeTiffTag<ossim_uint16>( tag, type, count, &value_ui16, arrayWritePos );
   ++tagCount;

   // Writes two tags 280 and 281:
   if ( writeMinMaxTiffTags( arrayWritePos ) == true )
   {
      tagCount += 2;  
   }
   
   // planar conf tag 284:
   tag   = ossim::TIFFTAG_PLANARCONFIG;
   type  = ossim::TIFF_SHORT;
   count = 1;
   value_ui16 = ossim::TIFFTAG_PLANARCONFIG_SEPARATE;
   writeTiffTag<ossim_uint16>( tag, type, count, &value_ui16, arrayWritePos );
   ++tagCount;

   if ( isTiled() )
   {
      // tile width tag 322:
      tag   = ossim::TIFFTAG_TILEWIDTH;
      count = 1;
      if (  m_outputTileSize.x <= OSSIM_DEFAULT_MAX_PIX_UINT16 )
      {
         type = ossim::TIFF_SHORT;
         value_ui16 = (ossim_uint16)m_outputTileSize.x;
         writeTiffTag<ossim_uint16>( tag, type, count, &value_ui16, arrayWritePos );
      }
      else
      {
         type = ossim::TIFF_LONG;
         value_ui32 = (ossim_uint32)m_outputTileSize.x;
         writeTiffTag<ossim_uint32>( tag, type, count, &value_ui32, arrayWritePos );
      }
      ++tagCount;
      
      // tile length tag 323:
      tag   = ossim::TIFFTAG_TILELENGTH;
      count = 1;
      if (  m_outputTileSize.y <= OSSIM_DEFAULT_MAX_PIX_UINT16 )
      {
         type = ossim::TIFF_SHORT;
         value_ui16 = (ossim_uint16)m_outputTileSize.y;
         writeTiffTag<ossim_uint16>( tag, type, count, &value_ui16, arrayWritePos );
      }
      else
      {
         type = ossim::TIFF_LONG;
         value_ui32 = (ossim_uint32)m_outputTileSize.y;
         writeTiffTag<ossim_uint32>( tag, type, count, &value_ui32, arrayWritePos );
      }
      ++tagCount;
   }

   // tile offsets tag 324:
   tag   = ossim::TIFFTAG_TILEOFFSETS;
   count = tile_offsets.size();
   type  = ossim::TIFF_LONG8;
   writeTiffTag<ossim_uint64>( tag, type, count, &tile_offsets.front(), arrayWritePos );
   ++tagCount;

   // tile byte counts tag 325:
   tag   = ossim::TIFFTAG_TILEBYTECOUNTS;
   count = tile_byte_counts.size();
   type  = ossim::TIFF_LONG8;
   writeTiffTag<ossim_uint64>( tag, type, count, &tile_byte_counts.front(), arrayWritePos );
   ++tagCount;

   // sample format tag 339:
   tag   = ossim::TIFFTAG_SAMPLEFORMAT;
   count = theInputConnection->getNumberOfOutputBands();
   type  = ossim::TIFF_SHORT;
   value_ui16 = getTiffSampleFormat();
   if ( count == 1 )
   {
      writeTiffTag<ossim_uint16>( tag, type, count, &value_ui16, arrayWritePos );
   }
   else
   {
      std::vector<ossim_uint16> v(count, value_ui16);
      writeTiffTag<ossim_uint16>( tag, type, count, &v.front(), arrayWritePos );
   }
   ++tagCount;

   // Writes two tags 340 and 341 (conditional on scalar type):
   if ( writeSMinSMaxTiffTags( minBands, maxBands, arrayWritePos ) == true )
   {
      tagCount += 2;
   }

   // Write geo keys if valid map projection:
   if ( mapProj.valid() )
   {
      std::vector<ossim_float64> vf;
      ossimDpt scale;
      ossimDpt tie;
      
      if ( mapProj->isGeographic() )
      {
         ossimGpt gpt;
         mapProj->lineSampleToWorld( theAreaOfInterest.ul(), gpt );
         tie.x = gpt.lon;
         tie.y = gpt.lat;
         scale = mapProj->getDecimalDegreesPerPixel();
      }
      else
      {
         mapProj->lineSampleToEastingNorthing( theAreaOfInterest.ul(), tie );
         scale = mapProj->getMetersPerPixel();
      }

      // Need to decide whehter to specify the full 4x4 model transform (in the case of a rotated
      // image), or simply use scale and offset tags:
      if (mapProj->isRotated())
      {
         // Model transform needed -- tag 34264:
         auto transform = mapProj->getModelTransform();
         count = 16; // 4x4 transform matrix
         tag   = ossim::MODEL_TRANSFORM_TAG;
         type  = ossim::TIFF_DOUBLE;
         vf.resize( count );
         auto m = transform.getData();
         for (int i=0; i<(int)count; ++i)
            vf.emplace_back(m[i/4][i%4]);
         writeTiffTag<ossim_float64>( tag, type, count, &vf.front(), arrayWritePos );
         ++tagCount;
      }
      else
      {
         // model pixel scale tag 33550:
         tag = ossim::MODEL_PIXEL_SCALE_TAG;
         count = 3; // x, y, z
         type = ossim::TIFF_DOUBLE;
         vf.resize(count);
         vf[0] = scale.x;
         vf[1] = scale.y;
         vf[2] = 0.0;
         writeTiffTag<ossim_float64>(tag, type, count, &vf.front(), arrayWritePos);
         ++tagCount;

         // model tie point tag 33992:
         tag = ossim::MODEL_TIE_POINT_TAG;
         count = 6; // x, y, z
         type = ossim::TIFF_DOUBLE;
         vf.resize(count);
         vf[0] = 0.0;   // x image point
         vf[1] = 0.0;   // y image point
         vf[2] = 0.0;   // z image point
         vf[3] = tie.x; // longitude or easting
         vf[4] = tie.y; // latitude of northing
         vf[5] = 0.0;
         writeTiffTag<ossim_float64>(tag, type, count, &vf.front(), arrayWritePos);
         ++tagCount;
      }

      // geo key directory tag 34735:
      tag   = ossim::GEO_KEY_DIRECTORY_TAG;
      count = 0; // set later.
      type  = ossim::TIFF_SHORT;
      std::vector<ossim_uint16> vs(0);

      // No
      vs.push_back(1);
      vs.push_back(1);
      vs.push_back(0);
      vs.push_back(10); // Updated later.

      vs.push_back(ossim::GT_MODEL_TYPE_GEO_KEY); // 1024
      vs.push_back(0);
      vs.push_back(1);
      vs.push_back(mapProj->isGeographic() ? ossim::MODEL_TYPE_GEOGRAPHIC :
                   ossim::MODEL_TYPE_PROJECTED);

      vs.push_back(ossim::GT_RASTER_TYPE_GEO_KEY); // 1025
      vs.push_back(0);
      vs.push_back(1);
      vs.push_back(ossim::PIXEL_IS_POINT);

      if ( mapProj->isGeographic() )
      {
         vs.push_back(ossim::GEOGRAPHIC_TYPE_GEO_KEY); // 2048
         vs.push_back(0);
         vs.push_back(1);
         vs.push_back((ossim_uint16)(mapProj->getPcsCode()));
      }

      vs.push_back(ossim::GEOG_GEODETIC_DATUM_GEO_KEY); // 2050
      vs.push_back(0);
      vs.push_back(1);
      vs.push_back((ossim_uint16)(mapProj->getDatum()->epsgCode()));

      if ( mapProj->isGeographic() )
      {
         vs.push_back(ossim::GEOG_ANGULAR_UNITS_GEO_KEY); // 2054
         vs.push_back(0);
         vs.push_back(1);
         vs.push_back(ossim::ANGULAR_DEGREE);
      }
         
      vs.push_back(ossim::GEOG_ELLIPSOID_GEO_KEY); // 2056
      vs.push_back(0);
      vs.push_back(1);
      vs.push_back((ossim_uint16)(mapProj->getDatum()->ellipsoid()->getEpsgCode()));

      // Stored in external OOGEO_DOUBLE_PARAMS_TAG
      vs.push_back(ossim::GEOG_SEMI_MAJOR_AXIS); // 2057
      vs.push_back(ossim::GEO_DOUBLE_PARAMS_TAG); 
      vs.push_back(1);
      vs.push_back(0);
         
      vs.push_back(ossim::GEOG_SEMI_MINOR_AXIS); // 2058
      vs.push_back(ossim::GEO_DOUBLE_PARAMS_TAG);
      vs.push_back(1);
      vs.push_back(1);

      vs.push_back(ossim::PROJECTED_CS_TYPE_GEO_KEY); // 3072
      vs.push_back(0);
      vs.push_back(1);
      vs.push_back((ossim_uint16)(mapProj->getPcsCode()));

      vs.push_back(ossim::PROJECTION_GEO_KEY); // 3074
      vs.push_back(0);
      vs.push_back(1);
      vs.push_back((ossim_uint16)(mapProj->getPcsCode()));

      if ( mapProj->isGeographic() == false )
      {
         vs.push_back(ossim::PROJ_LINEAR_UNITS_GEO_KEY); // 3076
         vs.push_back(0);
         vs.push_back(1);
         vs.push_back(ossim::LINEAR_METER);
      }
      
      count = vs.size();
      vs[3] = (count / 4) - 1;
      writeTiffTag<ossim_uint16>( tag, type, count, &vs.front(), arrayWritePos );
      ++tagCount;
       
      // geo double params tag 33550:
      tag   = ossim::GEO_DOUBLE_PARAMS_TAG;
      count = 2; // ellipsoid major, minor axis
      type  = ossim::TIFF_DOUBLE;
      vf.resize( count );
      vf[0] = mapProj->getDatum()->ellipsoid()->a();
      vf[1] = mapProj->getDatum()->ellipsoid()->b();
      writeTiffTag<ossim_float64>( tag, type, count, &vf.front(), arrayWritePos );
      ++tagCount;
   }

   // Write trailing zero indicading no more IFDs.
   ossim_uint64 offsetToNextIfd = 0;
   m_str->write( (const char*)&offsetToNextIfd, 8 );

   // Seek back and re-write the tag count.
   m_str->seekp( 16, std::ios_base::beg );
   m_str->write( (const char*)&tagCount, 8 );
   
   status =  m_str->good();
   
   return status;
}

bool ossimWriter::writeMinMaxTiffTags( std::streamoff& arrayWritePos )
{
   bool status = true;

   // DEFAULT for OSSIM_UINT32.
   ossim_uint16 minValue = 1;
   ossim_uint16 maxValue = 255;

   switch( theInputConnection->getOutputScalarType() )
   {
      case OSSIM_UINT8:
      {
         break; // defaulted above
      }
      case OSSIM_USHORT11:
      {
         maxValue = 2047;
         break;
      }
      case OSSIM_USHORT12:
      {
         maxValue = 4095;
         break;
      }
      case OSSIM_USHORT13:
      {
         maxValue = 8191;
         break;
      }
      case OSSIM_USHORT14:
      {
         maxValue = 16383;
         break;
      }
      case OSSIM_USHORT15:
      {
         maxValue = 32767;
         break;
      }
      case OSSIM_UINT16:
      {
         maxValue = 65535;
         break;
      }
      default:
         status = false;
   }
   
   if ( status )
   {
      writeTiffTag<ossim_uint16>( ossim::TIFFTAG_MINSAMPLEVALUE,
                                  ossim::TIFF_SHORT,
                                  1, &minValue, arrayWritePos );
      writeTiffTag<ossim_uint16>( ossim::TIFFTAG_MAXSAMPLEVALUE,
                                  ossim::TIFF_SHORT,
                                  1, &maxValue, arrayWritePos );
   }
   
   return status;
}

bool ossimWriter::writeSMinSMaxTiffTags( const std::vector<ossim_float64>& minBands,
                                         const std::vector<ossim_float64>& maxBands,
                                         std::streamoff& arrayWritePos )
{
   bool status = false;
   
   if(minBands.size() && maxBands.size())
   {
      ossim_float64 minValue = *std::min_element(minBands.begin(), minBands.end());
      ossim_float64 maxValue = *std::max_element(maxBands.begin(), maxBands.end());

      switch( theInputConnection->getOutputScalarType() )
      {
         case OSSIM_SINT16: 
         case OSSIM_UINT32:
         case OSSIM_FLOAT32:
         case OSSIM_FLOAT64:
         case OSSIM_NORMALIZED_FLOAT:
         case OSSIM_NORMALIZED_DOUBLE:
         {
            ossim_float32 v = static_cast<ossim_float32>(minValue);
            writeTiffTag<ossim_float32>( ossim::TIFFTAG_SMINSAMPLEVALUE,
                                         ossim::TIFF_FLOAT,
                                         1, &v, arrayWritePos );
            v = static_cast<ossim_float32>(maxValue);
            writeTiffTag<ossim_float32>( ossim::TIFFTAG_SMAXSAMPLEVALUE,
                                         ossim::TIFF_FLOAT,
                                         1, &v, arrayWritePos );
            status = true;
            break;
         }
         default:
         {
            break;
         }
      }
   }
   return status;
}

template <class T>
void ossimWriter::writeTiffTag(
   ossim_uint16 tag, ossim_uint16 type, ossim_uint64 count,
   const T* value, std::streamoff& arrayWritePos )
{
   m_str->write( (const char*)&tag, 2 );
   m_str->write( (const char*)&type, 2 );
   m_str->write( (const char*)&count, 8 );

   ossim_uint64 bytes = sizeof( T ) * count;
   
   if ( bytes <= 8 )
   {
      m_str->write( (const char*)value, bytes );
      if ( bytes < 8 )
      {
         // Fill remaining bytes with 0.
         char c = '\0';
         m_str->write( (const char*)&c, (8-bytes) );
      }
   }
   else // Greater than 8 bytes, must write at end of file.
   {
      // Store the offset to array:
      m_str->write( (const char*)&arrayWritePos, 8 );

      // Capture posistion:
      std::streamoff currentPos = m_str->tellp();

      // Seek to end:
      m_str->seekp( arrayWritePos, std::ios_base::beg );
      
      // Write:
      m_str->write( (const char*)value, bytes );

      // Capture new offset for next array write.
      arrayWritePos = m_str->tellp();

      // Seek back:
      m_str->seekp( currentPos, std::ios_base::beg );
   }
}

bool ossimWriter::writeTiffTilesBandSeparate( std::vector<ossim_uint64>& tile_offsets,
                                              std::vector<ossim_uint64>& tile_byte_counts,
                                              std::vector<ossim_float64>& minBands,
                                              std::vector<ossim_float64>& maxBands )
{
   static const char* const MODULE = "ossimWriter::writeToTilesBandSeparate";
   if ( traceDebug() ) CLOG << " Entered...\n";

   // Start the sequence at the first tile.
   theInputConnection->setToStartOfSequence();

   // Control flags:
   bool alignTiles    = getAlignTilesFlag();
   bool flushTiles    = getFlushTilesFlag();
   bool writeBlanks   = getWriteBlanksFlag();
   bool computeMinMax = needsMinMax();

   // Block size for write:
   const std::streamsize BLOCK_SIZE = getBlockSize();
   
   const ossim_int32 BANDS       = (ossim_int32)theInputConnection->getNumberOfOutputBands();
   const ossim_int32 TILES_WIDE  = (ossim_int32)theInputConnection->getNumberOfTilesHorizontal();
   const ossim_int32 TILES_TOTAL = (ossim_int32)theInputConnection->getNumberOfTiles();

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "align tiles flag:     " << alignTiles
         << "\nflush tiles flag:     " << flushTiles
         << "\nwrite blanks flag:    " << writeBlanks
         << "\ncompute min max flag: " << computeMinMax
         << "\nwrite block size:     " << BLOCK_SIZE
         << "\nBANDS:                " << BANDS
         << "\nTILES_WIDE:           " << TILES_WIDE
         << "\nTILES_TOTAL:          " << TILES_TOTAL << "\n";
   }

   tile_offsets.resize( TILES_TOTAL*BANDS );
   tile_byte_counts.resize( TILES_TOTAL*BANDS );

   ossimDataObjectStatus tileStatus = OSSIM_STATUS_UNKNOWN;
   ossim_int64 ossimTileIndex    = 0;
   ossim_int64 tiffTileIndex     = 0;
   ossim_int64 tileSizeInBytes   = 0;
   ossim_int64 bandOffsetInBytes = 0;

   //---
   // Adjust the starting file position to make room for IFD tags, tile offset
   // and tile byte counts and arrays.
   //
   // Assuming:
   // IFD start = 16, end 512, gives 496 bytes for tags.
   // Array section start = 512, end is start + (16 * tile_count * bands) + 256 bytes
   // for geotiff array bytes.
   //---
   std::streamsize startPos = 512 + 16 * TILES_TOTAL * BANDS + 256;
   
   while ( ossimTileIndex < TILES_TOTAL )
   {
      ossimRefPtr<ossimImageData> id = theInputConnection->getNextTile();
      if(!id)
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << MODULE << " ERROR:"
            << "Error returned writing tiff tile:  " << ossimTileIndex
            << "\nNULL Tile from input encountered"
            << std::endl;
         return false;
      }

      tileStatus = id->getDataObjectStatus();
      
      if ( ossimTileIndex == 0 )
      {
         tileSizeInBytes = (ossim_int64)id->getSizePerBandInBytes();
         bandOffsetInBytes = tileSizeInBytes * TILES_TOTAL;
      }

      if ( computeMinMax )
      {
         if ( (tileStatus == OSSIM_FULL) || (tileStatus == OSSIM_PARTIAL) )
         {
            // Compute running min, max.
            id->computeMinMaxPix(minBands, maxBands);
         }
      }
      
      // Band loop.
      for (ossim_int32 band=0; band < BANDS; ++band)
      {
         tiffTileIndex = ossimTileIndex + band * TILES_TOTAL;
         
         if ( (writeBlanks == true) || (tileStatus == OSSIM_FULL) || (tileStatus == OSSIM_PARTIAL) )
         {
            // Grab a pointer to the tile for the band.
            const char* data = (const char*)id->getBuf(band);
            
            // Compress data here(future maybe, i.e. jpeg, j2k...
            
            //---
            // Write the tile.
            // Note: tiles laid out, all the red tiles, all the green tiles all the
            // blue tiles.
            //---
            if(data)
            {
               // Compute the stream position:
               std::streampos pos = startPos + ossimTileIndex * tileSizeInBytes +
                  band * bandOffsetInBytes;
               
               if ( alignTiles )
               {
                  // Snap to block boundary:
                  std::streampos overflow = pos % BLOCK_SIZE;
                  if ( overflow > 0 )
                  {
                     pos += BLOCK_SIZE - overflow;
                  }
               }
               
               m_str->seekp( pos );
               
               if ( m_str->good() )
               { 
                  tile_offsets[ tiffTileIndex ] = (ossim_uint64)pos;
                  tile_byte_counts[ tiffTileIndex ] = (ossim_uint64)tileSizeInBytes;

                  // Write the tile to stream:
                  m_str->write( data, (std::streamsize)tileSizeInBytes);

                  if ( flushTiles )
                  {
                     m_str->flush();
                  }
                  
                  // Check stream:
                  if ( m_str->fail() == true )
                  {
                     ossimNotify(ossimNotifyLevel_DEBUG)
                        << MODULE << " ERROR:\nWrite error on tiff tile:  " << ossimTileIndex
                        << std::endl;
                     return false;
                  }
               }
               else
               {
                  ossimNotify(ossimNotifyLevel_DEBUG)
                     << MODULE << " ERROR:\nStream has gone bad!" << std::endl;
                  return false;
               }
               
            }
            else
            {
               ossimNotify(ossimNotifyLevel_WARN)
                  << MODULE << " ERROR:\nNull input tile:  " << ossimTileIndex
                  << std::endl;
               return false;
            }
         }
         else
         {
            //---
            // Writing sparse tiff.
            // Set the offset and byte count to 0 to indicate blank tile.
            //---
            if (traceDebug())
            {
               ossimNotify(ossimNotifyLevel_DEBUG)
                  << "sparse blank tile[" << tiffTileIndex << "]: " << tiffTileIndex << "\n";
            }
            tile_offsets[ tiffTileIndex ] = 0;
            tile_byte_counts[ tiffTileIndex ] = 0;
         }
         
      } // End of band loop.

      ++ossimTileIndex;

      if( needsAborting() )
      {
         setPercentComplete(100);
         break; // Get out...
      }
      else if ( ossimTileIndex % TILES_WIDE )
      {
         // Output percent complete every row of tiles.
         double tileNum = ossimTileIndex;
         double numTiles = TILES_TOTAL;
         setPercentComplete(tileNum / numTiles * 100.0);
      }
      
   } // End: while ( ossimTileIndex < TILES_TOTAL )

   if ( traceDebug() ) CLOG << " Exited...\n";
   
   return m_str->good();
}

bool ossimWriter::setOutputStream(std::ostream& stream)
{
   if (m_ownsStreamFlag && m_str)
   {
      delete m_str;
   }
   m_str = &stream;
   m_ownsStreamFlag = false;
   return true;
}

void ossimWriter::setTileSize(const ossimIpt& tileSize)
{
   if ( (tileSize.x % 16) || (tileSize.y % 16) )
   {
      if(traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
                  << "ossimWriter::setTileSize ERROR:"
                  << "\nTile size must be a multiple of 16!"
                  << "\nSize remains:  " << m_outputTileSize
                  << std::endl;
      }
   }
   else
   {
      m_outputTileSize = tileSize;

      // For save state:
      m_kwl->addPair( TILE_SIZE_KW, m_outputTileSize.toString().string() );
   }
}

const ossimIpt& ossimWriter::getOutputTileSize() const
{
   return m_outputTileSize;
}

bool ossimWriter::saveState( ossimKeywordlist& kwl, const char* prefix) const
{
   // Lazy man save state...
   kwl.add( prefix, *(m_kwl.get()), true );
   return ossimImageFileWriter::saveState(kwl, prefix);
}

bool ossimWriter::loadState(const ossimKeywordlist& kwl, const char* prefix)
{
   bool result = false;
   if ( ossimImageFileWriter::loadState(kwl, prefix) )
   {
      if ( theOutputImageType!="ossim_ttbs")
      {
         result = true;
         
         std::string pfx = prefix?prefix:"";  
         std::string value;

         value = kwl.findKey( pfx, ALIGN_TILES_KW );
         if ( value.size() )
         {
            m_kwl->addPair( ALIGN_TILES_KW, value, true );
         }

         value = kwl.findKey( pfx, BLOCK_SIZE_KW );
         if ( value.size() )
         {
            m_kwl->addPair( BLOCK_SIZE_KW, value, true );
         }

         value = kwl.findKey( pfx, FLUSH_TILES_KW );
         if ( value.size() )
         {
            m_kwl->addPair( FLUSH_TILES_KW, value, true );
         }

         value = kwl.findKey( pfx, INCLUDE_BLANK_TILES_KW );
         if ( value.size() )
         {
            m_kwl->addPair( INCLUDE_BLANK_TILES_KW, value, true );
         }

         value = kwl.findKey( pfx, TILE_SIZE_KW );
         if ( value.size() )
         {
            m_outputTileSize.toPoint(value);
            m_kwl->addPair( TILE_SIZE_KW, m_outputTileSize.toString().string(), true );
         }
      }
   }

   return result;
}

void ossimWriter::setProperty(ossimRefPtr<ossimProperty> property)
{
   if( property.valid() )
   {
      // See if it's one of our properties:
      std::string key = property->getName().string();

      if ( traceDebug() )
      {
         ossimString value;
         property->valueToString(value);

         ossimNotify(ossimNotifyLevel_DEBUG)
            << "ossimWriter::setProperty DEBUG:"
            << "\nkey:   " << key
            << "\nvalue: " << value << "\n";
      }
      
      if ( ( key == ALIGN_TILES_KW ) ||
           ( key == BLOCK_SIZE_KW )  ||
           ( key == FLUSH_TILES_KW ) ||
           ( key == INCLUDE_BLANK_TILES_KW ) )
      {
         ossimString value;
         property->valueToString(value);
         m_kwl->addPair( key, value.string(), true );
      }
      else if ( key == TILE_SIZE_KW )
      {
         // Comes in as a single int, e.g.: 256
         ossimString value;
         property->valueToString(value);
         m_outputTileSize.x = value.toInt32();
         m_outputTileSize.y = m_outputTileSize.x;

         // Store in keywordlist / save state as a point, e.g.: ( 256, 256 )
         m_kwl->addPair( key,  m_outputTileSize.toString().string(), true );
      }
      else
      {
         ossimImageFileWriter::setProperty(property);
      }
   }
}

ossimRefPtr<ossimProperty> ossimWriter::getProperty(const ossimString& name)const
{
   ossimRefPtr<ossimProperty> prop = 0;

   if ( name.string() == ALIGN_TILES_KW )
   {
      std::string value = m_kwl->findKey( ALIGN_TILES_KW );
      ossimRefPtr<ossimBooleanProperty> boolProp =
         new ossimBooleanProperty(name, ossimString(value).toBool());
      prop = boolProp.get();
   }
   else if( name == BLOCK_SIZE_KW )
   {
      // Property a single int, e.g.: 4096
      ossim_int64 blockSize = getBlockSize();
      ossimRefPtr<ossimStringProperty> stringProp =
         new ossimStringProperty(name, ossimString::toString(blockSize), false); // editable flag
      prop = stringProp.get();
   }
   else if ( name.string() == FLUSH_TILES_KW )
   {
      std::string value = m_kwl->findKey( FLUSH_TILES_KW );
      ossimRefPtr<ossimBooleanProperty> boolProp =
         new ossimBooleanProperty(name, ossimString(value).toBool());
      prop = boolProp.get();
   }
   else if ( name.string() == INCLUDE_BLANK_TILES_KW )
   {
      std::string value = m_kwl->findKey( INCLUDE_BLANK_TILES_KW );
      ossimRefPtr<ossimBooleanProperty> boolProp =
         new ossimBooleanProperty(name, ossimString(value).toBool());
      prop = boolProp.get();
   }
   else if( name == TILE_SIZE_KW )
   {
      // Property a single int, e.g.: 256
      ossimRefPtr<ossimStringProperty> stringProp =
         new ossimStringProperty(name, ossimString::toString(m_outputTileSize.x), false); // editable flag
      stringProp->setReadOnlyFlag(false);
      stringProp->setChangeType(ossimProperty::ossimPropertyChangeType_AFFECTS_OTHERS);
      stringProp->addConstraint(ossimString("16"));
      stringProp->addConstraint(ossimString("32"));
      stringProp->addConstraint(ossimString("64"));
      stringProp->addConstraint(ossimString("128"));
      stringProp->addConstraint(ossimString("256"));      
      stringProp->addConstraint(ossimString("512"));      
      stringProp->addConstraint(ossimString("1024"));      
      stringProp->addConstraint(ossimString("2048"));
      prop = stringProp.get();
   }
   else
   {
      prop = ossimImageFileWriter::getProperty(name);
   }
   return prop;
}

void ossimWriter::getPropertyNames(std::vector<ossimString>& propertyNames) const
{
   propertyNames.push_back(ossimString(ALIGN_TILES_KW));
   propertyNames.push_back(ossimString(BLOCK_SIZE_KW));
   propertyNames.push_back(ossimString(FLUSH_TILES_KW));   
   propertyNames.push_back(ossimString(INCLUDE_BLANK_TILES_KW));
   propertyNames.push_back(ossimString(TILE_SIZE_KW));
   ossimImageFileWriter::getPropertyNames(propertyNames);
}

ossim_uint16 ossimWriter::getTiffSampleFormat() const
{
   ossim_uint16 result = 0;
   switch( theInputConnection->getOutputScalarType() )
   {
      case OSSIM_UINT8:
      case OSSIM_USHORT11:
      case OSSIM_USHORT12:
      case OSSIM_USHORT13:
      case OSSIM_USHORT14:
      case OSSIM_USHORT15:
      case OSSIM_UINT16:
      case OSSIM_UINT32:
         result = ossim::SAMPLEFORMAT_UINT;
         break;

      case OSSIM_SINT16:
         result = ossim::SAMPLEFORMAT_INT;
         break;

      case OSSIM_FLOAT32:
      case OSSIM_FLOAT64:
      case OSSIM_NORMALIZED_FLOAT:
      case OSSIM_NORMALIZED_DOUBLE:
         result = ossim::SAMPLEFORMAT_IEEEFP;
         break;

      default:
         break;
   }

   return result;
}

bool ossimWriter::isTiled() const
{
   return ( theOutputImageType == "ossim_ttbs" );
}

bool ossimWriter::getAlignTilesFlag() const
{
   bool result = true; // default
   std::string value = m_kwl->findKey( ALIGN_TILES_KW );
   if ( value.size() )
   {
      result = ossimString(value).toBool();
   }
   return result;
}

ossim_int64 ossimWriter::getBlockSize() const
{
   ossim_int64 result = 4096; // default
   std::string value = m_kwl->findKey( BLOCK_SIZE_KW );
   if ( value.size() )
   {
      result = ossimString(value).toInt64();

      // Disallow anything not on 1024 boundary.
      if ( result % 1024 )
      {
         result = 4096;
         ossimNotify(ossimNotifyLevel_DEBUG)
                  << "ossimWriter::getBlockSize ERROR:"
                  << "\nBlock size MUST be a multiple of 1024!"
                  << "\nBlock size remains:  " << result
                  << std::endl;
      }
   }
   return result;
}

bool ossimWriter::getFlushTilesFlag() const
{
   bool result = true; // default
   std::string value = m_kwl->findKey( FLUSH_TILES_KW );
   if ( value.size() )
   {
      result = ossimString(value).toBool();
   }
   return result;
}

bool ossimWriter::getWriteBlanksFlag() const
{
   bool result = true; // default
   std::string value = m_kwl->findKey( INCLUDE_BLANK_TILES_KW );
   if ( value.size() )
   {
      result = ossimString(value).toBool();
   }
   return result;
}

bool ossimWriter::needsMinMax() const
{
   bool result = false;
   switch( theInputConnection->getOutputScalarType() )
   {
      case OSSIM_SINT16: 
      case OSSIM_UINT32:
      case OSSIM_FLOAT32:
      case OSSIM_FLOAT64:
      case OSSIM_NORMALIZED_FLOAT:
      case OSSIM_NORMALIZED_DOUBLE:
      {
         result = true;
         break;
      }
      default:
      {
         break;
      }
   }
   return result;
}
