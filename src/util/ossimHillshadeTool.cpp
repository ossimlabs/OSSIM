//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include <ossim/base/ossimArgumentParser.h>
#include <ossim/base/ossimApplicationUsage.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimGeoPolygon.h>
#include <ossim/base/ossimIrect.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimProperty.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimRefreshEvent.h>
#include <ossim/base/ossimScalarTypeLut.h>
#include <ossim/base/ossimStdOutProgress.h>
#include <ossim/base/ossimStringProperty.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimVisitor.h>

#include <ossim/imaging/ossimBrightnessContrastSource.h>
#include <ossim/imaging/ossimBumpShadeTileSource.h>
#include <ossim/imaging/ossimFilterResampler.h>
#include <ossim/imaging/ossimFusionCombiner.h>
#include <ossim/imaging/ossimImageData.h>
#include <ossim/imaging/ossimImageFileWriter.h>
#include <ossim/imaging/ossimImageGeometry.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/imaging/ossimImageMosaic.h>
#include <ossim/imaging/ossimImageRenderer.h>
#include <ossim/imaging/ossimImageSource.h>
#include <ossim/imaging/ossimImageSourceFilter.h>
#include <ossim/imaging/ossimImageToPlaneNormalFilter.h>
#include <ossim/imaging/ossimImageWriterFactoryRegistry.h>
#include <ossim/imaging/ossimIndexToRgbLutFilter.h>
#include <ossim/imaging/ossimRectangleCutFilter.h>
#include <ossim/imaging/ossimScalarRemapper.h>
#include <ossim/imaging/ossimSFIMFusion.h>
#include <ossim/imaging/ossimTwoColorView.h>
#include <ossim/imaging/ossimImageSourceFactoryRegistry.h>
#include <ossim/init/ossimInit.h>

#include <ossim/projection/ossimEquDistCylProjection.h>
#include <ossim/projection/ossimImageViewAffineTransform.h>
#include <ossim/projection/ossimMapProjection.h>
#include <ossim/projection/ossimProjection.h>
#include <ossim/projection/ossimProjectionFactoryRegistry.h>
#include <ossim/projection/ossimUtmProjection.h>

#include <ossim/support_data/ossimSrcRecord.h>
#include <ossim/util/ossimHillshadeTool.h>
#include <cmath>
#include <sstream>
#include <string>

static ossimTrace traceDebug("ossimHillshadeUtil:debug");

static const std::string COLOR_BLUE_KW           = "color_blue";
static const std::string COLOR_GREEN_KW          = "color_green";
static const std::string COLOR_RED_KW            = "color_red";
static const std::string COLOR_SOURCE_KW         = "color_source";

const char*  ossimHillshadeTool::DESCRIPTION =
   "Computes shaded representation of input elevation surface with specified lighting parameters.";

ossimHillshadeTool::ossimHillshadeTool()
{
   m_kwl.setExpandEnvVarsFlag(true);
}

// Private/hidden from use.
ossimHillshadeTool::ossimHillshadeTool( const ossimHillshadeTool& /* obj */ )
{
}

// Private/hidden from use.
const ossimHillshadeTool& ossimHillshadeTool::operator=( const ossimHillshadeTool& /* rhs */)
{
   return *this;
}

ossimHillshadeTool::~ossimHillshadeTool()
{
   clear();
}

bool ossimHillshadeTool::initialize(ossimArgumentParser& ap)
{
   // Permit base class to pull out common options first.
   if (!ossimChipProcTool::initialize(ap))
      return false;
   if (m_helpRequested)
      return true;

   std::string tempString1;
   ossimArgumentParser::ossimParameter stringParam1(tempString1);
   std::string tempString2;
   ossimArgumentParser::ossimParameter stringParam2(tempString2);
   std::string tempString3;
   ossimArgumentParser::ossimParameter stringParam3(tempString3);
   std::string tempString4;
   ossimArgumentParser::ossimParameter stringParam4(tempString4);
   std::string tempString5;
   ossimArgumentParser::ossimParameter stringParam5(tempString5);
   std::string tempString6;
   ossimArgumentParser::ossimParameter stringParam6(tempString6);
   double tempDouble1;
   ossimArgumentParser::ossimParameter doubleParam1(tempDouble1);
   double tempDouble2;
   ossimArgumentParser::ossimParameter doubleParam2(tempDouble2);
   
   // Extract optional arguments and stuff them in a keyword list.
   if( ap.read("--azimuth", stringParam1) )
   {
      m_kwl.addPair( std::string(ossimKeywordNames::AZIMUTH_ANGLE_KW), tempString1 );
   }

   if( ap.read("--color", stringParam1, stringParam2, stringParam3) )
   {
      m_kwl.addPair( COLOR_RED_KW,   tempString1 );
      m_kwl.addPair( COLOR_GREEN_KW, tempString2 );
      m_kwl.addPair( COLOR_BLUE_KW,  tempString3 );
   }

   int color_source_idx = 0;
   while( ap.read("--color-source", stringParam1) )
   {
      ossimString key = COLOR_SOURCE_KW;
      key += ossimString::toString(color_source_idx++);
      key += ".";
      key += ossimKeywordNames::FILE_KW;
      m_kwl.addPair(key.string(), tempString1 );
   }

   if ( ap.read("--elevation", stringParam1) )
   {
      m_kwl.addPair( std::string(ossimKeywordNames::ELEVATION_ANGLE_KW), tempString1 );
   }

   processRemainingArgs(ap);
   return true;
}


void ossimHillshadeTool::initProcessingChain()
{
   // Need a mosaic of DEM over the AOI as an image mosaic:
   ossimRefPtr<ossimImageSource> demMosaic = mosaicDemSources();
   m_procChain->add(demMosaic.get());

   // Set up the normal source.
   ossimRefPtr<ossimImageToPlaneNormalFilter> normSource = new ossimImageToPlaneNormalFilter;
   normSource->setTrackScaleFlag(true);
   m_procChain->add( normSource.get() );

   // Set the smoothness factor.
   ossim_float64 gain = 1.0;
   normSource->setSmoothnessFactor(gain);

   // Create the bump shade.
   ossimRefPtr<ossimBumpShadeTileSource> bumpShade = new ossimBumpShadeTileSource;
   m_procChain->add(bumpShade.get());

   // Set the azimuth angle.
   ossim_float64 azimuthAngle = 180;
   ossimString lookup = m_kwl.findKey( ossimKeywordNames::AZIMUTH_ANGLE_KW );
   if ( lookup.size() )
   {
      ossim_float64 f = lookup.toFloat64();
      if ( (f >= 0) && (f <= 360) )
      {
         azimuthAngle = f;
      }
   }
   bumpShade->setAzimuthAngle(azimuthAngle);

   // Set the elevation angle.
   ossim_float64 elevationAngle = 45.0;
   lookup = m_kwl.findKey( ossimKeywordNames::ELEVATION_ANGLE_KW );
   if ( lookup.size() )
   {
      ossim_float64 f = lookup.toFloat64();
      if ( (f >= 0.0) && (f <= 90) )
      {
         elevationAngle = f;
      }
   }
   bumpShade->setElevationAngle(elevationAngle);


   // Color can be added via color image source:
   if (!m_imgLayers.empty())
   {
      // A color source image (or list) is provided. Add them as input to bump shade:
      ossimRefPtr<ossimImageSource> colorSource = combineLayers( m_imgLayers );
      bumpShade->connectMyInputTo(1, colorSource.get());
   }
   else
   {
      // Default colors are grey:
      ossim_uint8 r = 0xff;
      ossim_uint8 g = 0xff;
      ossim_uint8 b = 0xff;
      lookup = m_kwl.findKey( COLOR_RED_KW );
      if ( lookup.size() )
         r = lookup.toUInt8();
      lookup = m_kwl.findKey( COLOR_GREEN_KW );
      if ( lookup.size() )
         g = lookup.toUInt8();
      lookup = m_kwl.findKey( COLOR_BLUE_KW );
      if ( lookup.size() )
         b = lookup.toUInt8();
      bumpShade->setRgbColorSource(r, g, b);
   }
}

void ossimHillshadeTool::setUsage(ossimArgumentParser& ap)
{
   // Add global usage options.
   ossimInit::instance()->addOptions(ap);
   
   // Set app name.
   ossimString appName = ap.getApplicationName();
   ossimApplicationUsage* au = ap.getApplicationUsage();
   ossimString usageString = appName;
   usageString += " hillshade [option]... [input-option]... <input-file(s)> <output-file>\nNote at least one input is required either from one of the input options, e.g. --input-dem <my-dem.hgt> or adding to command line in front of the output file in which case the code will try to ascertain what type of input it is.\n\nAvailable traces:\n-T \"ossimChipperUtil:debug\"   - General debug trace to standard out.\n-T \"ossimChipperUtil:log\"     - Writes a log file to output-file.log.\n-T \"ossimChipperUtil:options\" - Writes the options to output-file-options.kwl.";
   au->setCommandLineUsage(usageString);

   // Add arguments.
   au->addCommandLineOption("--azimuth", "<azimuth>\nLight source azimuth angle for bump shade.\nRange: 0 to 360, Default = 180.0");
   au->addCommandLineOption("--color","<r> <g> <b>\nSet the red, green and blue color values to be used with hillshade.\nRange 0 to 255, Defualt r=255, g=255, b=255");
   au->addCommandLineOption("--color-source","<file>\nSpecifies the image file to use as a color source instead of a fixed RGB value.");
   au->addCommandLineOption("--elevation", "<elevation>\nhillshade option - Light source elevation angle for bumb shade.\nRange: 0 to 90, Default = 45.0");

   // Base class has its own:
   ossimChipProcTool::setUsage(ap);

   ostringstream description;
   description << DESCRIPTION << "\n\nNOTES:\n"
      << "1) Never use same base name in the same directory! Example is you have a Chicago.tif\n"
      << "   and you want a Chicago.jp2, output Chicago.jp2 to its own directory.\n"
      << "\nExample command to Hill shade: Hill shade two DEMs, output to a geotiff.\n"
      << appName << " --color 255 255 255 --azimuth 270 --elevation 45 --exaggeration 2.0 N37W123.hgt N38W123.hgt outputs/hillshade.tif\n"
      << "\n// Above command where all options are in a keyword list:\n"
      << appName << " --options r39-options.kwl\n"
      << std::endl;

   au->setDescription(description.str());

}

