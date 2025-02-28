//---
// File: ossimInfo.cpp
// 
// License: MIT
// 
// Author:  David Burken
//
// Description: ossimInfo class definition
//
// Utility class for getting information from the ossim library.
// 
//---
// $Id$

#include <ossim/util/ossimInfo.h>
#include <ossim/ossimVersion.h>
#include <ossim/base/ossimArgumentParser.h>
#include <ossim/base/ossimApplicationUsage.h>
#include <ossim/base/ossimContainerProperty.h>
#include <ossim/base/ossimDatum.h>
#include <ossim/base/ossimDatumFactoryRegistry.h>
#include <ossim/base/ossimDrect.h>
#include <ossim/base/ossimEnvironmentUtility.h>
#include <ossim/base/ossimFontInformation.h>
#include <ossim/base/ossimObjectFactoryRegistry.h>
#include <ossim/base/ossimEcefPoint.h>
#include <ossim/base/ossimEllipsoid.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimGeoid.h>
#include <ossim/base/ossimGeoidManager.h>
#include <ossim/base/ossimGpt.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimPreferences.h>
#include <ossim/base/ossimProperty.h>
#include <ossim/base/ossimString.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimXmlDocument.h>
#include <ossim/elevation/ossimElevManager.h>
#include <ossim/font/ossimFont.h>
#include <ossim/font/ossimFontFactoryRegistry.h>
#include <ossim/imaging/ossimFilterResampler.h>
#include <ossim/imaging/ossimImageGeometry.h>
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/imaging/ossimImageWriterFactoryRegistry.h>
#include <ossim/imaging/ossimOverviewBuilderFactoryRegistry.h>
#include <ossim/init/ossimInit.h>
#include <ossim/plugin/ossimSharedPluginRegistry.h>
#include <ossim/projection/ossimProjectionFactoryRegistry.h>
#include <ossim/support_data/ossimInfoBase.h>
#include <ossim/support_data/ossimInfoFactoryRegistry.h>
#include <ossim/support_data/ossimSupportFilesList.h>
#include <ossim/support_data/ImageHandlerStateRegistry.h>

#include <iomanip>
#include <sstream>
#include <vector>
#include <memory>

using namespace std;

static const char BUILD_DATE_KW[]           = "build_date";
static const char CAN_OPEN_KW[]             = "can_open";
static const char CENTER_GROUND_KW[]        = "center_ground";
static const char CENTER_IMAGE_KW[]         = "center_image";
static const char CHECK_CONFIG_KW[]         = "check_config";
static const char CONFIGURATION_KW[]        = "configuration";
static const char DATUMS_KW[]               = "datums";
static const char DEG2RAD_KW[]              = "deg2rad";
static const char DUMP_KW[]                 = "dump";
static const char DUMP_NO_OVERVIEWS_KW[]    = "dump_no_overviews";
static const char EXTENSIONS_KW[]           = "extensions";
static const char FACTORIES_KW[]            = "factories";
static const char FACTORY_KEYWORD_LIST_KW[] = "factory_keyword_list";
static const char FONTS_KW[]                = "fonts";
static const char FORMAT_KW[]               = "format"; 
static const char FT2MTRS_KW[]              = "ft2mtrs";
static const char FT2MTRS_US_SURVEY_KW[]    = "ft2mtrs_us_survey";
static const char GEOM_INFO_KW[]            = "geometry_info";
static const char HEIGHT_KW[]               = "height";
static const char IMAGE_BOUNDS_KW[]         = "image_bounds";
static const char IMAGE_CENTER_KW[]         = "image_center";
static const char IMAGE_FILE_KW[]           = "image_file";
static const char IMAGE_INFO_KW[]           = "image_info";
static const char IMAGE_RECT_KW[]           = "image_rect";
static const char IMG2GRD_KW[]              = "img2grd";
static const char GRD2IMG_KW[]              = "grd2img";
static const char METADATA_KW[]             = "metadata";
static const char MTRS2FT_KW[]              = "mtrs2ft";
static const char MTRS2FT_US_SURVEY_KW[]    = "mtrs2ft_us_survey";
static const char MTRSPERDEG_KW[]           = "mtrs_per_deg";
static const char NORTH_UP_KW[]             = "north_up_angle";
static const char OUTPUT_FILE_KW[]          = "output_file";
static const char OVERVIEW_TYPES_KW[]       = "overview_types";
static const char OVERWRITE_KW[]            = "overwrite";
static const char PALETTE_KW[]              = "palette";
static const char PLUGINS_KW[]              = "plugins";
static const char PLUGIN_TEST_KW[]          = "plugin_test";
static const char PRETTY_PRINT_KW[]         = "pretty_print";
static const char PROJECTIONS_KW[]          = "projections";
static const char RAD2DEG_KW[]              = "rad2deg";
static const char READER_PROPS_KW[]         = "reader_props";
static const char RESAMPLER_FILTERS_KW[]    = "resampler_filters";
static const char REVISION_NUMBER_KW[]      = "revision_number";
static const char UP_IS_UP_KW[]             = "up_is_up";
static const char UP_IS_UP_GPT_KW[]         = "up_is_up_gpt";
static const char UP_IS_UP_IPT_KW[]         = "up_is_up_ipt";
static const char VERSION_KW[]              = "version";
static const char WRITERS_KW[]              = "writers_kw";
static const char WRITER_PROPS_KW[]         = "writer_props";
static const char ZOOM_LEVEL_GSDS_KW[]      = "zoom_level_gsds";
static const char ECEF2LLH_KW[]             = "ecef2llh";
static const char DUMP_STATE_KW[]           = "dump_state";
static const char STATE_KW[]                = "state";

const char* ossimInfo::DESCRIPTION =
      "Dumps metadata information about input image and OSSIM in general.";

// Static trace for debugging.  Use -T ossimInfo to turn on.
static ossimTrace traceDebug = ossimTrace("ossimInfo:debug");

ossimInfo::ossimInfo() :
         m_img(0)
{
}

ossimInfo::~ossimInfo()
{
}

void ossimInfo::setUsage(ossimArgumentParser& ap)
{
   // Add global usage options.
   ossimInit::instance()->addOptions(ap);
   
   // Set the general usage:
   ossimApplicationUsage* au = ap.getApplicationUsage();
   ossimString usageString = ap.getApplicationName();
   usageString += " [options] <optional-image | optional-state>";
   au->setCommandLineUsage(usageString);

   // Set the command line options:
   au->addCommandLineOption("--bounds", "Will print out the edge to edge image bounds.");
   
   au->addCommandLineOption("--build-date", "Build date of code.");

   au->addCommandLineOption("-c", "Will print ground and image center.");
   
   au->addCommandLineOption("--can-open", "return can_open: true or can_open: false");

   au->addCommandLineOption("--cg", "Will print out ground center.");

   au->addCommandLineOption("--check-config", "Checks configuration.");

   au->addCommandLineOption("--ci", "Will print out image center.");

   au->addCommandLineOption("--config", "Displays configuration info.");

   au->addCommandLineOption("-D", "A human-readable (i.e., not necessarily key:value pairs) dump of the image.");

   au->addCommandLineOption("-d", "A generic dump if one is available.");

   au->addCommandLineOption("--datums", "Prints datum list.");

   au->addCommandLineOption("--deg2rad", "<degrees> Gives radians from degrees.");

   au->addCommandLineOption("--dno", "A generic dump if one is available.  This option ignores overviews.");

   au->addCommandLineOption("--dump-state", "If the image supports a state object then the state object will be dumped.");

   au->addCommandLineOption("--ecef2llh", "<X> <Y> <Z> in ECEF coordinates and returns latitude longitude height position.");

   au->addCommandLineOption("--extensions", "Prints list of supported image extensions.");

   au->addCommandLineOption("-f", "<format> Will output the information specified format [KWL | XML].  Default is KWL.");

   au->addCommandLineOption("--factories", "<keyword_list_flag> Prints factory list.  If keyword_list_flag is true, the result of a saveState will be output for each object.");

   au->addCommandLineOption("--fonts", "Prints available fonts.");
   
   au->addCommandLineOption("--ft2mtrs", "<feet> Gives meters from feet (0.3048 meters per foot).");

   au->addCommandLineOption("--ft2mtrs-us-survey", "<feet> Gives meters from feet (0.3048006096 meters per foot).");

   au->addCommandLineOption("-h", "Display this information");

   au->addCommandLineOption("--height", "<latitude-in-degrees> <longitude-in-degrees> Returns the MSL and ellipsoid height given a latitude longitude position.");
   
   au->addCommandLineOption("-i", "Will print out the general image information.");

   au->addCommandLineOption("--img2grd", "<x> <y> Gives ground point from zero based image point.  Returns \"nan\" if point is outside of image area.");
   au->addCommandLineOption("--grd2img", "<lat> <lon> <height> Gives full res image point from lat lon height.");

   au->addCommandLineOption("-m", "Will print out meta data image information.");

   au->addCommandLineOption("--mtrsPerDeg", "<latitude> Gives meters per degree and meters per minute for a given latitude.");

   au->addCommandLineOption("--mtrs2ft", "<meters> Gives feet from meters (0.3048 meters per foot).");

   au->addCommandLineOption("--mtrs2ft-us-survey", "<meters> Gives feet from meters (0.3048006096 meters per foot).");

   au->addCommandLineOption("-n or --north-up", "Rotation angle to North for an image.");

   au->addCommandLineOption("-o", "<output-file> Will output the information to the file specified.  Default is to standard out.");

   au->addCommandLineOption("--overview-types", "Prints overview builder types.");

   au->addCommandLineOption("-p", "Will print out the image projection information.");

   au->addCommandLineOption("--palette", "Will print out the color palette if one exists.");

   au->addCommandLineOption("--plugins", "Prints plugin list.");

   au->addCommandLineOption("--plugin-test", "Test plugin passed to option.");

   au->addCommandLineOption("--projections", "Prints projections.");

   au->addCommandLineOption("-r", "Will print image rectangle.");

   au->addCommandLineOption("--rad2deg", "<radians> Gives degrees from radians.");

   au->addCommandLineOption("--reader-props", "Prints readers and properties.");

   au->addCommandLineOption("--resampler-filters", "Prints resampler filter list.");

   au->addCommandLineOption("--revision", "Revision of code.");

   au->addCommandLineOption("-s", "Force the ground rect to be the specified datum");

   au->addCommandLineOption("--up-is-up or -u", "Rotation angle to \"up is up\" for an image.\nWill return 0 if image's projection is not affected by elevation.");
   au->addCommandLineOption("--up-is-up-gpt", "Computes up angle given gpt: <lat> <lon>");
   au->addCommandLineOption("--up-is-up-ipt", "Computes up angle given full res image point: <x> <y>");
   au->addCommandLineOption("-v", "Overwrite existing geometry.");

   au->addCommandLineOption("-V or --version", "Version of code, e.g. 1.8.20");

   au->addCommandLineOption("--writer-props", "Prints writers and properties.");

   au->addCommandLineOption("--writers", "Prints list of available writers.");

   au->addCommandLineOption("--zoom-level-gsds", "Prints zoom level gsds for projections EPSG:4326 and EPSG:3857.");

   ostringstream description;
   description << DESCRIPTION << "\n\n Examples:\n\n"
         << "    ossim-info --version\n"
         << "    ossim-info -i ./myfile.tif\n"
         << "      prints out only general image information\n\n"
         << "    ossim-info -p ./myfile.tif\n"
         << "      prints out only image projection information\n\n"
         << "    ossim-info -p -s wge ./myfile.tif\n"
         << "      prints out only image projection information and shifts to WGS 84\n\n"
         << "    ossim-info -p -i ./myfile.tif\n"
         << "      prints out both image and projection information\n\n"
         << "    ossim-info -p -i ./myfile.tif -o ./myfile.geom\n"
         << "      writes geometry file with both image and projection information\n\n"
         << "    ossim-info -p -i ./myfile.tif -v -o ./myfile.geom\n"
         << "      writes geometry file with both image and projection information\n"
         << "      while overwriting existing .geom file.\n\n"
         << "    ossim-info -f XML ./myfile.tif\n"
         << "      prints out image and projection information as an XML document\n\n"
         << "    ossim-info -d myfile.ntf\n"
         << "      Dumps all data available, in this case, all NITF tags, from file.\n\n"
         << "    ossim-info -d a.toc\n"
         << "      Dumps all data available, in this case, all NITF and rpf tags, from file.\n\n"
         << "    ossim-info --dno a.toc\n"
         << "      \"dno\" for \"dump no overviews\" Dumps all data available,\n"
         << "       in this case, all NITF and RPF tags, from file ignoring overviews.\n\n"
         << "    ossim-info -d -i -p myfile.ntf\n"
         << "      Typical usage case, i.e. do a dump of tags and print out image and\n"
         << "      projection information.\n\n"
         << std::endl;
   au->setDescription(description.str());

} // void ossimInfo::addArguments(ossimArgumentParser& ap)

bool ossimInfo::initialize(ossimArgumentParser& ap)
{
   static const char M[] = "ossimInfo::initialize(ossimArgumentParser&)";
   if ( traceDebug() )
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << M << " entered...\n";
   }

   if (!ossimTool::initialize(ap))
      return false;
   if (m_helpRequested)
      return true;

   bool result = true;

   //---
   // Start with clean options keyword list.
   //---
   m_kwl.clear();

   bool requiresInputImage = false;

   while ( 1 ) //  While forever loop...
   {
      // Used throughout below:
      std::string ts1;
      ossimArgumentParser::ossimParameter sp1(ts1);
      std::string ts2;
      ossimArgumentParser::ossimParameter sp2(ts2);
      std::string ts3;
      ossimArgumentParser::ossimParameter sp3(ts3);
      const char TRUE_KW[] = "true";

      if( ap.read("--bounds") )
      {
         m_kwl.add( IMAGE_BOUNDS_KW, TRUE_KW );
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--build-date") )
      {
         m_kwl.add( BUILD_DATE_KW, TRUE_KW );
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("-c") )
      {
         m_kwl.add( IMAGE_CENTER_KW, TRUE_KW );
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--can-open") )
      {
         m_kwl.add( CAN_OPEN_KW, TRUE_KW );
         requiresInputImage = true;
         if ( ap.argc() < 2 )
         {
            break;
         }
      }


      if( ap.read("--cg") )
      {
         m_kwl.add( CENTER_GROUND_KW, TRUE_KW );
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--check-config") )
      {
         m_kwl.addPair( CHECK_CONFIG_KW, TRUE_KW );
         if ( ap.argc() < 2 )
         {
            break;
         }
      }
      
      if( ap.read("--ci") )
      {
         m_kwl.add( CENTER_IMAGE_KW, TRUE_KW );
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--config") || ap.read("--configuration") )
      {
         m_kwl.add( CONFIGURATION_KW, TRUE_KW );
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--datums") )
      {
         m_kwl.add( DATUMS_KW, TRUE_KW );
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--deg2rad", sp1) )
      {
         m_kwl.add( DEG2RAD_KW, ts1.c_str() );
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("-D") )
      {
         m_kwl.add( PRETTY_PRINT_KW, TRUE_KW );
         requiresInputImage = true;
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("-d") )
      {
         m_kwl.add( DUMP_KW, TRUE_KW );
         requiresInputImage = true;
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--dump-state") )
      {
         m_kwl.add( DUMP_STATE_KW, TRUE_KW );
         requiresInputImage = true;
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--dno") )
      {
         m_kwl.add( DUMP_KW, TRUE_KW );
         m_kwl.add( DUMP_NO_OVERVIEWS_KW, TRUE_KW );
         requiresInputImage = true;
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--extensions") )
      {
         m_kwl.add( EXTENSIONS_KW, TRUE_KW );
         if ( ap.argc() < 2 )
            break;
      }

      if( ap.read("-f", sp1) )
      {
         m_kwl.add( FORMAT_KW, ts1.c_str());
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--factories", sp1) )
      {
         m_kwl.add( FACTORIES_KW, TRUE_KW);
         m_kwl.add( FACTORY_KEYWORD_LIST_KW, ts1.c_str());
         if ( ap.argc() < 2 )
         {
            break;
         }
      }
 
      if( ap.read("--fonts") )
      {
         m_kwl.add( FONTS_KW, TRUE_KW );
         if ( ap.argc() < 2 )
         {
            break;
         }
      }
      
      if( ap.read("--ft2mtrs", sp1) )
      {
         m_kwl.add( FT2MTRS_KW, ts1.c_str());
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--ft2mtrs-us-survey", sp1) )
      {
         m_kwl.add( FT2MTRS_KW, ts1.c_str());
         m_kwl.add( FT2MTRS_US_SURVEY_KW, TRUE_KW);
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--height", sp1, sp2) )
      {
         ossimString lat = ts1;
         ossimString lon = ts2;
         ossimGpt gpt;
         gpt.lat = lat.toFloat64();
         gpt.lon = lon.toFloat64();
         m_kwl.add( HEIGHT_KW, gpt.toString().c_str() );
         if ( ap.argc() < 2 )
         {
            break;
         }
      }
      if( ap.read("--ecef2llh", sp1, sp2, sp3))
      {
         ossimString x = ts1;
         ossimString y = ts2;
         ossimString z = ts3;
         ossimEcefPoint ecefPoint(x.toFloat64(), y.toFloat64(), z.toFloat64());
         m_kwl.add( ECEF2LLH_KW, ecefPoint.toString().c_str() );
         if ( ap.argc() < 2 )
         {
            break;
         }

      }
      if( ap.read("-i") )
      {
         m_kwl.add( IMAGE_INFO_KW, TRUE_KW );
         requiresInputImage = true;
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--img2grd", sp1, sp2) )
      {
         requiresInputImage = true;
         ossimString x = ts1;
         ossimString y = ts2;
         ossimDpt dpt;
         dpt.x = x.toFloat64();
         dpt.y = y.toFloat64();
         m_kwl.add( IMG2GRD_KW, dpt.toString().c_str() );
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--grd2img", sp1, sp2, sp3) )
      {
         requiresInputImage = true;
         ossimString lat = ts1;
         ossimString lon = ts2;
         ossimString hgt = ts3;
         ossimGpt gpt;
         gpt.makeNan();
         gpt.latd(lat.toFloat64());
         gpt.lond(lon.toFloat64());
         if(hgt != "nan")
         {
           gpt.height(hgt.toFloat64());
         }
         m_kwl.add( GRD2IMG_KW, gpt.toString().c_str() );
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("-m") )
      {
         m_kwl.add( METADATA_KW, TRUE_KW );
         requiresInputImage = true;
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--mtrs2ft", sp1) )
      {
         m_kwl.add( MTRS2FT_KW, ts1.c_str());
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--mtrs2ft-us-survey", sp1) )
      {
         m_kwl.add( MTRS2FT_KW, ts1.c_str());
         m_kwl.add( MTRS2FT_US_SURVEY_KW, TRUE_KW);
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--mtrsPerDeg", sp1) )
      {
         m_kwl.add( MTRSPERDEG_KW, ts1.c_str());
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("-n") || ap.read("--north-up") )
      {
         m_kwl.add( NORTH_UP_KW, TRUE_KW );
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("-o", sp1) )
      {
         m_kwl.add( OUTPUT_FILE_KW, ts1.c_str());
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--overview-types") )
      {
         m_kwl.add( OVERVIEW_TYPES_KW, TRUE_KW );
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("-p") )
      {
         m_kwl.add( GEOM_INFO_KW, TRUE_KW );
         requiresInputImage = true;
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--palette") )
      {
         m_kwl.add( PALETTE_KW, TRUE_KW );
         requiresInputImage = true;
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--plugins") )
      {
         m_kwl.add( PLUGINS_KW, TRUE_KW );
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--plugin-test", sp1) )
      {
         m_kwl.add( PLUGIN_TEST_KW, ts1.c_str());
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--projections") )
      {
         m_kwl.add( PROJECTIONS_KW, TRUE_KW );
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("-r") )
      {
         m_kwl.add( IMAGE_RECT_KW, TRUE_KW );
         requiresInputImage = true;
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--rad2deg", sp1) )
      {
         m_kwl.add( RAD2DEG_KW, ts1.c_str());
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--reader-props") )
      {
         m_kwl.add( READER_PROPS_KW, TRUE_KW );
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--resampler-filters") )
      {
         m_kwl.add( RESAMPLER_FILTERS_KW, TRUE_KW );
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--revision") ||
          ap.read("--revision-number") ) // backwards compat
      {
         m_kwl.add( REVISION_NUMBER_KW, TRUE_KW );
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("-u") || ap.read("--up-is-up") )
      {
         requiresInputImage = true;
         m_kwl.add( UP_IS_UP_KW, TRUE_KW );
         if ( ap.argc() < 2 )
         {
            break;
         }
      }
      if( ap.read("--up-is-up-ipt", sp1, sp2))
      {
         requiresInputImage = true;
         m_kwl.add( UP_IS_UP_KW, TRUE_KW);
         m_kwl.add( UP_IS_UP_IPT_KW, (ts1 +" "+ts2).c_str() );
         if ( ap.argc() < 2 )
         {
            break;
         }
      }
      if(ap.read("--up-is-up-gpt",sp1, sp2))
      {
         requiresInputImage = true;
         m_kwl.add( UP_IS_UP_KW, TRUE_KW);
         m_kwl.add( UP_IS_UP_GPT_KW, (ts1 +" "+ ts2).c_str() );
         if ( ap.argc() < 2 )
         {
            break;
         }
      }
 
      if( ap.read("-v") )
      {
         m_kwl.add( OVERWRITE_KW, TRUE_KW );
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--version") || ap.read("-V") )
      {
         m_kwl.add( VERSION_KW, TRUE_KW );
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--writer-props") )
      {
         m_kwl.add( WRITER_PROPS_KW, TRUE_KW );
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--writers") )
      {
         m_kwl.add( WRITERS_KW, TRUE_KW );
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      if( ap.read("--zoom-level-gsds") )
      {
         m_kwl.add( ZOOM_LEVEL_GSDS_KW, TRUE_KW );
         if ( ap.argc() < 2 )
         {
            break;
         }
      }

      // End of arg parsing.
      ap.reportRemainingOptionsAsUnrecognized();
      if ( ap.errors() )
      {
         ap.writeErrorMessages(ossimNotify(ossimNotifyLevel_NOTICE));
         std::string errMsg = "Unknown option...";
         throw ossimException(errMsg);
      }

      break; // Break from while forever.

   } // End while (forever) loop.

   if ( ap.argc() == 2 )
   {
      m_kwl.add( IMAGE_FILE_KW, ap[1]  );
   }

   if ( (( ap.argc() == 1 ) && requiresInputImage) || (m_kwl.getSize() == 0) )
   {
      if ( requiresInputImage )
      {
         ossimNotify(ossimNotifyLevel_NOTICE) << "\nError: Option requires input image!\n\n";
      }
      setUsage(ap);
      ap.getApplicationUsage()->write(ossimNotify(ossimNotifyLevel_INFO));
      result = false;

   }

   if ( traceDebug() )
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
               << "m_kwl:\n" << m_kwl << "\n"
               << M << " exit result = " << (result?"true":"false")
               << "\n";
   }

   return result;
}

bool ossimInfo::execute()
{
   static const char M[] = "ossimInfo::execute()";

   const ossim_uint32 KEY_COUNT = m_kwl.getSize();

   if ( traceDebug() )
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
               << M << " entered..."
               << "\nMap size: " << KEY_COUNT << "\n";
   }

   if ( KEY_COUNT )
   {
      ossim_uint32 consumedKeys = 0;

      const char* lookup;

      lookup = m_kwl.find(IMAGE_FILE_KW);
      if ( lookup )
      {
         ++consumedKeys;
         ossimFilename image = lookup;

         consumedKeys += executeImageOptions(image);
      }

      if ( consumedKeys < KEY_COUNT )
      {
         ossimString value;

         if ( keyIsTrue( std::string(BUILD_DATE_KW)) )
         {
            getBuildDate( value.string() );
            ossimNotify(ossimNotifyLevel_INFO)
            << BUILD_DATE_KW << ": " << value << "\n";
         }

         value = m_kwl.findKey( std::string(CHECK_CONFIG_KW) );
         if ( value.size() )
         {
            ++consumedKeys;
            if ( ossimString(value).toBool() == true )
            {
               checkConfig( ossimNotify(ossimNotifyLevel_INFO) );
            }
         }

         lookup = m_kwl.find(CONFIGURATION_KW);
         if ( lookup )
         {
            ++consumedKeys;
            value = lookup;
            if ( value.toBool() )
            {
               printConfiguration();
            }
         }

         lookup = m_kwl.find(DATUMS_KW);
         if ( lookup )
         {
            ++consumedKeys;
            value = lookup;
            if ( value.toBool() )
            {
               printDatums();
            }
         }

         lookup = m_kwl.find(DEG2RAD_KW);
         if ( lookup )
         {
            ++consumedKeys;
            value = lookup;
            deg2rad( value.toFloat64() );
         }

         lookup = m_kwl.find(ECEF2LLH_KW);
         if(lookup)
         {
            ++consumedKeys;
            ossimEcefPoint ecefPoint;
            ecefPoint.toPoint(lookup);

            ecef2llh(ecefPoint, ossimNotify(ossimNotifyLevel_INFO));
         }

         lookup = m_kwl.find(EXTENSIONS_KW);
         if ( lookup )
         {
            ++consumedKeys;
            value = lookup;
            if ( value.toBool() )
            {
               printExtensions();
            }
         }

         lookup = m_kwl.find(FACTORIES_KW);
         if ( lookup )
         {
            ++consumedKeys;
            ossimString factories = lookup;
            bool keywordListFlag = false;
            lookup = m_kwl.find(FACTORY_KEYWORD_LIST_KW);
            if ( lookup )
            {
               ++consumedKeys;
               keywordListFlag = ossimString(lookup).toBool();
            }
            printFactories(keywordListFlag);
         }
         
         lookup = m_kwl.find(FONTS_KW);
         if ( lookup )
         {
            ++consumedKeys;
            value = lookup;
            if ( value.toBool() )
            {
               printFonts();
            }
         }
         
         lookup = m_kwl.find(FT2MTRS_KW);
         if ( lookup )
         {
            ++consumedKeys;
            value = lookup;
            bool us_survey = false;
            lookup = m_kwl.find(FT2MTRS_US_SURVEY_KW);
            if ( lookup )
            {
               ++consumedKeys;
               us_survey = ossimString(lookup).toBool();
            }
            ft2mtrs( value.toFloat64(), us_survey);
         }

         lookup = m_kwl.find(HEIGHT_KW);
         if ( lookup )
         {
            ++consumedKeys;
            value = lookup;
            ossimGpt gpt;
            gpt.toPoint(value);
            outputHeight(gpt);
         }

         lookup = m_kwl.find(MTRS2FT_KW);
         if ( lookup )
         {
            ++consumedKeys;
            value = lookup;
            bool us_survey = false;
            lookup = m_kwl.find(MTRS2FT_US_SURVEY_KW);
            if ( lookup )
            {
               ++consumedKeys;
               us_survey = ossimString(lookup).toBool();
            }
            mtrs2ft( value.toFloat64(), us_survey);
         }

         lookup = m_kwl.find(MTRSPERDEG_KW);
         if ( lookup )
         {
            ++consumedKeys;
            value = lookup;
            mtrsPerDeg( value.toFloat64() );
         }

         lookup = m_kwl.find(OVERVIEW_TYPES_KW);
         if ( lookup )
         {
            ++consumedKeys;
            value = lookup;
            if ( value.toBool() )
            {
               printOverviewTypes();
            }
         }

         lookup = m_kwl.find(PLUGINS_KW);
         if ( lookup )
         {
            ++consumedKeys;
            value = lookup;
            if ( value.toBool() )
            {
               printPlugins();
            }
         }

         lookup = m_kwl.find(PLUGIN_TEST_KW);
         if ( lookup )
         {
            ++consumedKeys;
            value = lookup;
            testPlugin(value);
         }

         lookup = m_kwl.find(PROJECTIONS_KW);
         if ( lookup )
         {
            ++consumedKeys;
            value = lookup;
            if ( value.toBool() )
            {
               printProjections();
            }
         }

         lookup = m_kwl.find(RAD2DEG_KW);
         if ( lookup )
         {
            ++consumedKeys;
            value = lookup;
            rad2deg( value.toFloat64() );
         }

         lookup = m_kwl.find(READER_PROPS_KW);
         if ( lookup )
         {
            ++consumedKeys;
            value = lookup;
            if ( value.toBool() )
            {
               printReaderProps();
            }
         }

         lookup = m_kwl.find(RESAMPLER_FILTERS_KW);
         if ( lookup )
         {
            ++consumedKeys;
            value = lookup;
            if ( value.toBool() )
            {
               printResamplerFilters();
            }
         }

         if ( keyIsTrue( std::string(REVISION_NUMBER_KW) ) )
         {
            getRevisionNumber( value.string() );
            ossimNotify(ossimNotifyLevel_INFO)
            << REVISION_NUMBER_KW << ": " << value << "\n";
         }

         if ( keyIsTrue( std::string(VERSION_KW) ) )
         {
            getVersion( value.string() );
            ossimNotify(ossimNotifyLevel_INFO)
            << VERSION_KW << ": " << value << "\n";
         }

         lookup = m_kwl.find(WRITERS_KW);
         if ( lookup )
         {
            ++consumedKeys;
            value = lookup;
            if ( value.toBool() )
            {
               printWriters();
            }
         }

         lookup = m_kwl.find(WRITER_PROPS_KW);
         if ( lookup )
         {
            ++consumedKeys;
            value = lookup;
            if ( value.toBool() )
            {
               printWriterProps();
            }
         }

         lookup = m_kwl.find(ZOOM_LEVEL_GSDS_KW);
         if ( lookup )
         {
            ++consumedKeys;
            value = lookup;
            if ( value.toBool() )
            {
               printZoomLevelGsds();
            }
         }

      } // if ( consumedKeys < KEY_COUNT )

      if ( traceDebug() )
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
                  << "KEY_COUNT:    " << KEY_COUNT
                  << "\nconsumedKeys: " << consumedKeys << "\n";
      }

   } // if ( KEY_COUNT )

   if ( traceDebug() )
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << M << " exited...\n";
   }
   return true;
}

ossim_uint32 ossimInfo::executeImageOptions(const ossimFilename& file)
{
   static const char M[] = "ossimInfo::executeImageOptions()";
   if ( traceDebug() )
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << M << " entered...\nfile: " << file << "\n";
   }

   // Output keyword list.
   ossimKeywordlist okwl;

   ossim_uint32 consumedKeys = 0;
   const char* lookup = 0;
   ossimString value  = "";

   bool dnoFlag       = false;
   bool overwriteFlag = false;   
   bool xmlOutFlag    = false;

   lookup = m_kwl.find( OVERWRITE_KW );
   if ( lookup )
   {
      ++consumedKeys;
      value = lookup;
      overwriteFlag = value.toBool();
   }

   // Check for xml format option.
   lookup = m_kwl.find( FORMAT_KW );
   if ( lookup )
   {
      ++consumedKeys;
      ossimString format = lookup;
      if ( format.upcase() == "XML" )
      {
         xmlOutFlag = true;
      }
   }

   lookup = m_kwl.find( OUTPUT_FILE_KW );
   ossimFilename outputFile;
   if ( lookup )
   {
      ++consumedKeys;
      outputFile = lookup;
   }

   lookup = m_kwl.find( PRETTY_PRINT_KW );
   if ( lookup )
   {
      ++consumedKeys;
      prettyPrint(file);
   }

   // Check for dump.  Does not require image to be opened.
   lookup = m_kwl.find( DUMP_KW );
   if ( lookup )
   {
      ++consumedKeys;
      lookup = m_kwl.find( DUMP_NO_OVERVIEWS_KW );
      if ( lookup )
      {
         ++consumedKeys;
         value = lookup;
         dnoFlag = value.toBool();
      }

      if ( !xmlOutFlag && ( outputFile == ossimFilename::NIL ) )
      {
         //---
         // Write to standard out:
         // This dump will come out in order so is preferred over going to
         // okwl(output keyword list) which will come out alphabetical.
         //---
         ossimKeywordlist kwl;
         dumpImage(file, dnoFlag, kwl);
         kwl.print(ossimNotify(ossimNotifyLevel_INFO));
      }
      else
      {
         // Save to output keyword list. Will be output later.
         dumpImage(file, dnoFlag, okwl);
      }
   }

   bool centerGroundFlag  = false;
   bool centerImageFlag   = false;
   bool imageBoundsFlag   = false;
   bool imageCenterFlag   = false;   
   bool imageGeomFlag     = false;
   bool imageInfoFlag     = false;
   bool imageRectFlag     = false;
   bool img2grdFlag       = false;
   bool grd2imgFlag       = false;
   bool metaDataFlag      = false;
   bool northUpFlag       = false;
   bool paletteFlag       = false;
   bool upIsUpFlag        = false;
   bool imageToGroundFlag = false;
   bool groundToImageFlag = false;
   bool dumpState         = false;
   bool canOpenFlag       = false;

   lookup = m_kwl.find( DUMP_STATE_KW );
   if ( lookup )
   {
      ++consumedKeys;
      value     = lookup;
      dumpState = value.toBool();
   }

   // Center Ground:
   lookup = m_kwl.find( CENTER_GROUND_KW );
   if ( lookup )
   {
      ++consumedKeys;
      value = lookup;
      centerGroundFlag = value.toBool();
   }

   // Center Image:
   lookup = m_kwl.find( CENTER_IMAGE_KW );
   if ( lookup )
   {
      ++consumedKeys;
      value = lookup;
      centerImageFlag = value.toBool();
   }

   // Metadata:
   lookup = m_kwl.find( METADATA_KW );
   if ( lookup )
   {
      ++consumedKeys;
      value = lookup;
      metaDataFlag = value.toBool();
   }

   // Palette:
   lookup = m_kwl.find( PALETTE_KW );
   if ( lookup )
   {
      ++consumedKeys;
      value = lookup;
      paletteFlag = value.toBool();
   }

   // Image bounds:
   lookup = m_kwl.find( IMAGE_BOUNDS_KW );
   if ( lookup )
   {
      ++consumedKeys;
      value = lookup;
      imageBoundsFlag = value.toBool();
   }

   // Image center:
   lookup = m_kwl.find( IMAGE_CENTER_KW );
   if ( lookup )
   {
      ++consumedKeys;
      value = lookup;
      imageCenterFlag = value.toBool();
   }

   // Image rect:
   lookup = m_kwl.find( IMAGE_RECT_KW );
   if ( lookup )
   {
      ++consumedKeys;
      value = lookup;
      imageRectFlag = value.toBool();
   }

   //---
   // General image info:
   // Defaulted ON if no image options set.
   //---
   lookup = m_kwl.find( IMAGE_INFO_KW );
   if ( lookup )
   {
      ++consumedKeys;
      value = lookup;
      imageInfoFlag = value.toBool();
   }

   lookup = m_kwl.find( IMG2GRD_KW );
   if ( lookup )
   {
      ++consumedKeys;
      img2grdFlag = true;
   }

   lookup = m_kwl.find( GRD2IMG_KW );
   if ( lookup )
   {
      ++consumedKeys;
      grd2imgFlag = true;
   }

   //---
   // Image geometry info:
   // Defaulted on if no image options set.
   //---
   lookup = m_kwl.find( GEOM_INFO_KW );
   if ( lookup )
   {
      ++consumedKeys;
      value = lookup;
      imageGeomFlag = value.toBool();
   }      

   // North up:
   lookup = m_kwl.find( NORTH_UP_KW );
   if ( lookup )
   {
      ++consumedKeys;
      value = lookup;
      northUpFlag = value.toBool();
   }

   // Up is up:
   lookup = m_kwl.find( UP_IS_UP_KW );
   if ( lookup )
   {
      ++consumedKeys;
      value = lookup;
      upIsUpFlag = value.toBool();
   }

   // Up is up:
   lookup = m_kwl.find( CAN_OPEN_KW );
   if ( lookup )
   {
      ++consumedKeys;
      value = lookup;
      canOpenFlag = value.toBool();
   }

   // If no options consumed default is image info and geom info:
   if ( consumedKeys == 0 )
   {
      imageInfoFlag = true;
      imageGeomFlag = true;
   }

   if ( centerGroundFlag || centerImageFlag || imageBoundsFlag || imageCenterFlag ||
        imageRectFlag || img2grdFlag || grd2imgFlag || metaDataFlag || paletteFlag ||
        imageInfoFlag || imageGeomFlag || northUpFlag || upIsUpFlag || dumpState ||
        imageToGroundFlag || groundToImageFlag || canOpenFlag)
   {
      // Requires open image.
      if ( ! m_img )
      {
         openImage(file);
      }
      
      if( canOpenFlag )
      {
         if(m_img)
         {
            okwl.add("can_open", "true", true);
         }
         else
         {
            okwl.add("can_open", "false", true);
         }
      }

      if ( centerGroundFlag )
      {
         getCenterGround(okwl);
      }

      if ( centerImageFlag )
      {
         getCenterImage(okwl);
      }

      if ( imageCenterFlag )
      {
         // -c option prints both ground and image point of center.
         getCenterGround(okwl);
         getCenterImage(okwl);
      }

      if ( imageBoundsFlag )
      {
         getImageBounds(okwl);
      }

      if ( imageRectFlag )
      {
         getImageRect(okwl);
      }

      if ( img2grdFlag )
      {
         getImg2grd(okwl);
      }
      if ( grd2imgFlag )
      {
         getGrd2img(okwl);
      }

      if ( metaDataFlag )
      {
         getImageMetadata(okwl);
      }

      if ( paletteFlag )
      {
         getImagePalette(okwl);
      }

      if ( imageInfoFlag )
      {
         getImageInfo(okwl, dnoFlag);
      }

      if ( imageGeomFlag )
      {
         getImageGeometryInfo(okwl, dnoFlag);
      }

      if ( imageRectFlag )
      {
         getImageRect(okwl);
      }

      if ( northUpFlag )
      {
         getNorthUpAngle( okwl );
      }

      if ( upIsUpFlag )
      {
         getUpIsUpAngle( okwl );
      }

      if(dumpState)
      {
         if(m_img)
         {
            if(m_img->getState())
            {
               m_img->getState()->save(okwl);               
            }
         }
      }

   } // if ( metaDataFlag || paletteFlag || imageInfoFlag || imageGeomFlag )

   if ( okwl.getSize() ) // Output section:
   {
      if ( outputFile == ossimFilename::NIL )
      {
         // Write to standard out:
         if ( !xmlOutFlag )
         {
            ossimNotify(ossimNotifyLevel_INFO) << okwl << std::endl;
         }
         else
         {
            outputXml( okwl );
         }
      }
      else
      {
         // Write to file:

         if ( !overwriteFlag && outputFile.exists() )
         {
            ossimNotify(ossimNotifyLevel_INFO)
                     << "ERROR: File already exists: "  << outputFile
                     << "\nUse -v option to overwrite."
                     << std::endl;
         }
         else
         {
            if ( !xmlOutFlag )
            {
               okwl.write( outputFile );
            }
            else
            {
               outputXml( okwl, outputFile );
            }
         }
      }

   } // if ( okwl )


   if ( traceDebug() )
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
               << "consumedKeys: " << consumedKeys << "\n"
               << M << " exited...\n";
   }

   return consumedKeys;

} // ossim_uint32 ossimInfo::executeImageOptions(const ossimFilename& file)

void ossimInfo::getImageInfo( const ossimFilename& file,
                              bool dumpFlag,
                              bool dnoFlag,
                              bool imageGeomFlag,
                              bool imageInfoFlag,
                              bool metaDataFlag,
                              bool paletteFlag,
                              ossimKeywordlist& kwl ) const
{
   if ( dumpFlag || dnoFlag )
   {
      dumpImage(file, dnoFlag, kwl);
   }

   // These flags requires open image.
   if ( imageGeomFlag || imageInfoFlag || metaDataFlag || paletteFlag )
   {
      // Note: openImageHandler throws ossimException if it can't open.
      ossimRefPtr<ossimImageHandler> ih = openImageHandler( file );
      if ( ih.valid() )
      {
         if ( metaDataFlag )
         {
            getImageMetadata( ih.get(), kwl );
         }
         if ( paletteFlag )
         {
            getImagePalette( ih.get(), kwl );
         }
         if ( imageInfoFlag )
         {
            getImageInfo( ih.get(), kwl, dnoFlag );
         }
         if ( imageGeomFlag )
         {
            getImageGeometryInfo( ih.get(), kwl, dnoFlag) ;
         }
      }
   }
}

bool ossimInfo::getImageInfo( const ossimFilename& file,
                              ossim_uint32 entry,
                              ossimKeywordlist& kwl ) const
{
   bool result = false;

   // Note: openImageHandler throws ossimException if it can't open.
   ossimRefPtr<ossimImageHandler> ih = openImageHandler( file );
   if ( ih.valid() )
   {
      if ( ih->setCurrentEntry( entry ) )
      {
         if ( getImageInfo( ih.get(), entry, kwl, false ) )
         {
            result = getImageGeometryInfo( ih.get(), entry, kwl, false );
         }
      }
      else
      {
         std::ostringstream errMsg;
         errMsg << "ossimInfo::getImageInfo ERROR:\nInvalid entry: " << entry
               << "\n";
         throw ossimException( errMsg.str() );
      }
   }

   return result;
}

void ossimInfo::openImage(const ossimFilename& file)
{
   if(file.ext().downcase()=="kwl")
   {
      openImageFromState(file);
   }
   else
   {
      m_img = openImageHandler( file );      
   }
}

void ossimInfo::openImageFromState(const ossimFilename& file)
{
   std::shared_ptr<ossim::ImageHandlerState> state;
   ossimKeywordlist kwl;
   if(kwl.addFile(file))
   {
      state = ossim::ImageHandlerStateRegistry::instance()->createState(kwl);
      if(state)
      {
         m_img = ossimImageHandlerRegistry::instance()->open(state);
      }
   }
}


ossimRefPtr<ossimImageHandler> ossimInfo::openImageHandler(const ossimFilename& file) const
{
   ossimRefPtr<ossimImageHandler> result;
   if(file.ext().downcase()=="kwl")
   {
      std::shared_ptr<ossim::ImageHandlerState> state;
      ossimKeywordlist kwl;
      if(kwl.addFile(file))
      {
         state = ossim::ImageHandlerStateRegistry::instance()->createState(kwl);
         if(state)
         {
            result = ossimImageHandlerRegistry::instance()->open(state);
         }
      }
   }
   else
   {
      // Go through new interface that passes a stream around. (drb 10 Nov. 2016)
      // ossimRefPtr<ossimImageHandler> result = ossimImageHandlerRegistry::instance()->open(file);
      result = ossimImageHandlerRegistry::instance()->
         openConnection(file);
   }
   // only throw an exception if the can-open option
   // is not specified
   ossimString canOpenFlag = m_kwl.find("can_open");
   if ( !result.valid() && !canOpenFlag.toBool())
   {
      std::string errMsg = "ossimInfo::openImage ERROR:\nCould not open: ";
      errMsg += file.string();
      throw ossimException(errMsg);
   }
   return result;
}

void ossimInfo::closeImage()
{
   m_img = 0;
}

ossimRefPtr<ossimImageHandler> ossimInfo::getImageHandler()
{
   return m_img;
}

void ossimInfo::prettyPrint(const ossimFilename& file) const
{
   std::shared_ptr<ossimInfoBase> info = ossimInfoFactoryRegistry::instance()->create(file);
   if (info)
   {
      //---
      // Old -d behavior was to dump all unless the dump no overview flag
      // was set. Need to see tiff tags in file order for all image file
      // directories(ifd's) so commenting out hard coded
      // info->setProcessOverviewFlag(false) that used to be settable with -d
      // + --dno options.  Note the old -d option used to do file order for
      // tiffs but now dumps to a keyword list that prints alphabetical(not
      // file order) so can't use that anymore.
      // 
      // drb - 15 Dec. 2016
      //---
      // info->setProcessOverviewFlag(false);
      
      info->print(ossimNotify(ossimNotifyLevel_INFO));
      info.reset();
   }
   else
   {
      ossimNotify(ossimNotifyLevel_INFO)
               << "No print available for:  " << file.c_str() << std::endl;
   }
}

void ossimInfo::dumpImage(const ossimFilename& file,
                          bool dnoFlag,
                          ossimKeywordlist& kwl) const
{
   std::shared_ptr<ossimInfoBase> info = ossimInfoFactoryRegistry::instance()->create(file);
   if (info)
   {
      if (dnoFlag) // Default info processes overviews.
      {
         info->setProcessOverviewFlag(false);
      }
      info->getKeywordlist(kwl);
      info.reset();
   }
   else
   {
      ossimNotify(ossimNotifyLevel_INFO)
               << "No dump available for:  " << file.c_str() << std::endl;
   }
}
void ossimInfo::getImageMetadata(ossimKeywordlist& kwl) const
{
   if ( m_img.valid() )
   {
      getImageMetadata( m_img.get(), kwl);
   }
}

void ossimInfo::getImageMetadata(const ossimImageHandler* ih, ossimKeywordlist& kwl) const
{
   if ( ih )
   {
      std::vector< ossimRefPtr< ossimProperty > > list;
      ih->getPropertyList(list);
      std::vector< ossimRefPtr< ossimProperty > >::const_iterator i = list.begin();
      while (i != list.end())
      {
         if ( (*i).valid() )
         {
            ossimString key;
            ossimString value;

            // Check for one level of nested container.
            if ((*i)->getClassName() == "ossimContainerProperty")
            {
               ossimContainerProperty* ptr = PTR_CAST(ossimContainerProperty, (*i).get());
               if (ptr)
               {
                  std::vector< ossimRefPtr< ossimProperty > > list2;    
                  ptr->getPropertyList(list2);

                  std::vector< ossimRefPtr< ossimProperty > >::const_iterator i2 = list2.begin();
                  while (i2 != list2.end())
                  {
                     key   = (*i2)->getName();
                     value = (*i2)->valueToString();
                     kwl.add(key.c_str(), value.c_str(), true);
                     ++i2;
                  }
               }
            }
            else // Not a container.
            {
               key   = (*i)->getName();
               value = (*i)->valueToString();
               kwl.add(key.c_str(), value.c_str(), true);
            }
         }
         ++i;
      }

   } // if ( ih )

} // End: getImageMetadata(ossimImageHandler* ih, ossimKeywordlist& kwl)

void ossimInfo::getImagePalette(ossimKeywordlist& kwl)
{
   if ( m_img.valid() )
   {
      getImagePalette( m_img.get(), kwl );
   }
}

void ossimInfo::getImagePalette(ossimImageHandler* ih, ossimKeywordlist& kwl) const
{
   if ( ih )
   {
      if(ih->getLut().valid())
      {
         ossim_uint32 entryIdx = 0;
         std::vector<ossim_uint32> entryList;
         ih->getEntryList(entryList);
         for(entryIdx = 0; entryIdx < ih->getNumberOfEntries();++entryIdx)
         {
            ih->setCurrentEntry(entryList[entryIdx]);
            ossimString prefix = "image";
            prefix = prefix + ossimString::toString(entryList[entryIdx]) + ".lut.";
            if(ih->getLut().valid())
            {
               ih->getLut()->saveState(kwl, prefix);
            }
         }
      }

   } // if ( ih )
}

void ossimInfo::getImageInfo(ossimKeywordlist& kwl, bool dnoFlag)
{
   if ( m_img.valid() )
   {
      getImageInfo( m_img.get(), kwl, dnoFlag );
   }
}

void ossimInfo::getImageInfo( ossimImageHandler* ih, ossimKeywordlist& kwl, bool dnoFlag ) const
{
   if ( ih )
   {
      ossim_uint32 numEntries = 0;

      std::vector<ossim_uint32> entryList;
      ih->getEntryList(entryList);

      std::vector<ossim_uint32>::const_iterator i = entryList.begin();
      while ( i != entryList.end() )
      {
         if ( getImageInfo( ih, (*i), kwl, dnoFlag ) )
         {
            ++numEntries;
         }
         ++i;
      }

      kwl.add(ossimKeywordNames::NUMBER_ENTRIES_KW, numEntries, true);

   } // if ( ih )
}

bool ossimInfo::getImageInfo( ossim_uint32 entry, ossimKeywordlist& kwl, bool dnoFlag )
{
   bool result = false;
   if ( m_img.valid() )
   {
      result = getImageInfo( m_img.get(), entry, kwl, dnoFlag );
   }
   return result;
}

bool ossimInfo::getImageInfo( ossimImageHandler* ih, ossim_uint32 entry, 
                              ossimKeywordlist& kwl, bool dnoFlag ) const
{
   bool result = false;

   if ( ih )
   {
      if ( ih->setCurrentEntry(entry) )
      {
         bool outputEntry = true;
         if ( dnoFlag )
         {
            if ( isImageEntryOverview() )
            {
               outputEntry = false;
            }
         }

         if ( outputEntry )
         {
            result = true;

            // Entry number:
            ossimString prefix = "image";
            prefix = prefix + ossimString::toString(entry) + ".";
            kwl.add(prefix.c_str(), ossimKeywordNames::ENTRY_KW, entry, true);

            // Get the entry_name (specialized multi-entry readers only):
            std::string entryName;
            ih->getEntryName( entry, entryName );
            if ( entryName.size() )
            {
               kwl.add(prefix.c_str(), "entry_name", entryName.c_str(), true);
            }

            // Type/class of reader:
            kwl.add(prefix, "type", ih->getClassName().c_str(), true);

            // Add RGB bands if available:
            getRgbBands( ih, entry, kwl );

            // Driver name if different from class name:
            if ( ih->getClassName() != ih->getShortName() )
            {
               kwl.add(prefix, "driver", ih->getShortName().c_str(), true);
            }

            // Type/class of overview reader:
            if (ih->getOverview())
            {
               kwl.add(prefix, "overview.type",
                       ih->getOverview()->getClassName().c_str(), true);
            }

            ossimDrect boundingRect = ih->getBoundingRect();
            kwl.add(prefix,ossimKeywordNames::UL_X_KW, boundingRect.ul().x, true);
            kwl.add(prefix,ossimKeywordNames::UL_Y_KW, boundingRect.ul().y, true);
            kwl.add(prefix,ossimKeywordNames::LR_X_KW, boundingRect.lr().x, true);
            kwl.add(prefix,ossimKeywordNames::LR_Y_KW, boundingRect.lr().y, true);

            const ossim_uint32 BANDS = ih->getNumberOfInputBands();
            kwl.add(prefix,ossimKeywordNames::NUMBER_INPUT_BANDS_KW, BANDS, true);
            kwl.add(prefix,ossimKeywordNames::NUMBER_OUTPUT_BANDS_KW,
                    ih->getNumberOfOutputBands(), true);
            kwl.add(prefix,ossimKeywordNames::NUMBER_LINES_KW,
                    boundingRect.height(), true);
            kwl.add(prefix,ossimKeywordNames::NUMBER_SAMPLES_KW,
                    boundingRect.width(), true);

            ossimScalarType scalar = ih->getOutputScalarType();

            for(ossim_uint32 i = 0; i < BANDS; ++i)
            {
               ossimString band = ossimString("band") + ossimString::toString(i) + ".";

               kwl.add(prefix, band+"null_value", ih->getNullPixelValue(i), true);
               kwl.add(prefix, band+"min_value", ih->getMinPixelValue(i), true);
               kwl.add(prefix, band+"max_value", ih->getMaxPixelValue(i), true);
            }

            // Output Radiometry.
            std::string rad;
            getRadiometry(scalar, rad);
            kwl.add(prefix, "radiometry", rad.c_str(), true);
            kwl.add(prefix,"number_decimation_levels", ih->getNumberOfDecimationLevels(), true);



         } // if ( outputEntry )

      } // if ( ih->setCurrentEntry(entry) )

   } // if ( ih )

   return result;

} // End: ossimInfo::getImageInfo( ih, entry...

void ossimInfo::getImageGeometryInfo(ossimKeywordlist& kwl, bool dnoFlag)
{
   if ( m_img.valid() )
   {
      getImageGeometryInfo( m_img.get(), kwl, dnoFlag );
   }
}

void ossimInfo::getImageGeometryInfo( ossimImageHandler* ih, ossimKeywordlist& kwl, bool dnoFlag) const
{
   if ( ih )
   {      ossim_uint32 numEntries = 0;

   std::vector<ossim_uint32> entryList;
   ih->getEntryList(entryList);

   std::vector<ossim_uint32>::const_iterator i = entryList.begin();
   while ( i != entryList.end() )
   {
      if ( getImageGeometryInfo( ih, (*i), kwl, dnoFlag ) )
      {
         ++numEntries;
      }
      ++i;
   }

   kwl.add(ossimKeywordNames::NUMBER_ENTRIES_KW, numEntries, true);

   } // if ( ih )
}

bool ossimInfo::getImageGeometryInfo(ossim_uint32 entry, ossimKeywordlist& kwl, bool dnoFlag)
{
   bool result = false; 
   if ( m_img.valid() )
   {
      getImageGeometryInfo( m_img.get(), entry, kwl, dnoFlag );
   }
   return result;
}

bool ossimInfo::getImageGeometryInfo( ossimImageHandler* ih,
                                      ossim_uint32 entry, 
                                      ossimKeywordlist& kwl, 
                                      bool dnoFlag) const
{
   bool result = false;

   if ( ih )
   {      
      if ( ih->setCurrentEntry(entry) )
      {
         bool outputEntry = true;
         if ( dnoFlag )
         {
            if ( isImageEntryOverview() )
            {
               outputEntry = false;
            }
         }

         if ( outputEntry )
         {
            ossimRefPtr<ossimImageGeometry> geom = ih->getImageGeometry();
            if(geom.valid())
            {
               result = true;

               ossimString prefix = "image";
               prefix = prefix + ossimString::toString(entry) + ossimString(".geometry.");

               geom->saveState(kwl, prefix);

               // Output support files list:
               ossimSupportFilesList::instance()->save(kwl, prefix);

               ossimGpt ulg;
               ossimGpt llg;
               ossimGpt lrg;
               ossimGpt urg;

               ossimDrect outputRect = ih->getBoundingRect();

               geom->localToWorld(outputRect.ul(), ulg);
               geom->localToWorld(outputRect.ll(), llg);
               geom->localToWorld(outputRect.lr(), lrg);
               geom->localToWorld(outputRect.ur(), urg);

               //---
               // *** HACK *** 
               // Encountered CADRG RPF imagery where the left edge was longitude -180 and
               // right edge +180. The projection code above reasonably maps all -180 to +180.
               // This however breaks the image footprint since it would appear that the left
               // and right edges were coincident instead of 360 degrees apart, i.e., a line
               // segment instead of a rect. So added check here for coincident left and right
               // edges and remapping left edge to -180.
               //---
               if ((ulg.lon == 180.0) && (urg.lon == 180.0))  
               {
                  ulg.lon = -180.0;
               }
               if ((llg.lon == 180.0) && (lrg.lon == 180.0))  
               {
                  llg.lon = -180.0;
               }

               kwl.add(prefix, "ul_lat", ulg.latd(), true);
               kwl.add(prefix, "ul_lon", ulg.lond(), true);
               kwl.add(prefix, "ll_lat", llg.latd(), true);
               kwl.add(prefix, "ll_lon", llg.lond(), true);
               kwl.add(prefix, "lr_lat", lrg.latd(), true);
               kwl.add(prefix, "lr_lon", lrg.lond(), true);
               kwl.add(prefix, "ur_lat", urg.latd(), true);
               kwl.add(prefix, "ur_lon", urg.lond(), true);

               if(!kwl.find(ossimKeywordNames::TIE_POINT_LAT_KW))
               {
                  kwl.add(prefix, ossimKeywordNames::TIE_POINT_LAT_KW, ulg.latd(), true);
                  kwl.add(prefix, ossimKeywordNames::TIE_POINT_LON_KW, ulg.lond(), true);
               }

               ossimDpt dpp;
               geom->getDegreesPerPixel( dpp );
               if ( dpp.hasNans() == false )
               {
                  kwl.add(prefix, ossimKeywordNames::DECIMAL_DEGREES_PER_PIXEL_LAT, dpp.lat, true);
                  kwl.add(prefix, ossimKeywordNames::DECIMAL_DEGREES_PER_PIXEL_LON, dpp.lon, true);
               }

               ossimDpt gsd = geom->getMetersPerPixel();
               kwl.add(prefix, ossimKeywordNames::METERS_PER_PIXEL_X_KW, gsd.x, true);
               kwl.add(prefix, ossimKeywordNames::METERS_PER_PIXEL_Y_KW, gsd.y, true);

            } // if(geom.valid())

         } // if ( outputEntry )

      } // if ( ih->setCurrentEntry(entry) )

      if ( !result )
      {
         ossimNotify(ossimNotifyLevel_WARN)
                  << "No geometry for file " << ih->getFilename() << std::endl;
      }

   } // if ( ih )

   return result;

} // End: ossimInfo::getImageGeometryInfo( ih, entry...

void ossimInfo::getCenterImage(ossimKeywordlist& kwl)
{
   if ( m_img.valid() )
   {
      getCenterImage( m_img.get(), kwl );
   }
}

void ossimInfo::getCenterImage( ossimImageHandler* ih, ossimKeywordlist& kwl) const
{
   if ( ih )
   {  
      std::vector<ossim_uint32> entryList;
      ih->getEntryList(entryList);

      std::vector<ossim_uint32>::const_iterator i = entryList.begin();
      while ( i != entryList.end() )
      {
         getCenterImage( ih, (*i), kwl );
         ++i;
      }
   } 
}

void ossimInfo::getCenterImage( ossimImageHandler* ih,
                                ossim_uint32 entry, 
                                ossimKeywordlist& kwl ) const
{
   if ( ih )
   {
      if ( ih->setCurrentEntry(entry) )
      {
         ossimString prefix = "image";
         prefix = prefix + ossimString::toString(entry) + ".";
         ossimDrect bounds = ih->getBoundingRect();

         if( !bounds.hasNans() )
         {
            ossimDpt iPt = bounds.midPoint();
            kwl.add(prefix, "center_image", iPt.toString().c_str(), true);
         }

      } // if ( ih->setCurrentEntry(entry) )
      else
      {
         ossimNotify(ossimNotifyLevel_WARN)
                  << "Could not get image center for: " << ih->getFilename() << std::endl;
      }

   } // if ( ih )
}

void ossimInfo::getCenterGround(ossimKeywordlist& kwl)
{
   if ( m_img.valid() )
   {
      getCenterGround( m_img.get(), kwl );
   }
}

void ossimInfo::getCenterGround( ossimImageHandler* ih, ossimKeywordlist& kwl) const
{
   if ( ih )
   {  
      std::vector<ossim_uint32> entryList;
      ih->getEntryList(entryList);

      std::vector<ossim_uint32>::const_iterator i = entryList.begin();
      while ( i != entryList.end() )
      {
         getCenterGround( ih, (*i), kwl );
         ++i;
      }
   } 
}

void ossimInfo::getCenterGround( ossimImageHandler* ih,
                                 ossim_uint32 entry, 
                                 ossimKeywordlist& kwl ) const
{
   if ( ih )
   {
      if ( ih->setCurrentEntry(entry) )
      {
         ossimString prefix = "image";
         prefix = prefix + ossimString::toString(entry) + ".";

         ossimRefPtr<ossimImageGeometry> geom = ih->getImageGeometry();
         if(geom.valid())
         {
            ossimDrect bounds;
            geom->getBoundingRect( bounds );

            if( !bounds.hasNans() )
            {
               ossimDpt iPt = bounds.midPoint();
               ossimGpt gPt;
               geom->localToWorld(iPt, gPt);
               kwl.add(prefix, "center_ground", gPt.toString().c_str(), true);
            }
         }

      } // if ( ih->setCurrentEntry(entry) )
      else
      {
         ossimNotify(ossimNotifyLevel_WARN)
                  << "Could not get ground center for: " << ih->getFilename() << std::endl;
      }

   } // if ( ih )
}

void ossimInfo::getImageBounds(ossimKeywordlist& kwl)
{
   if ( m_img.valid() )
   {
      getImageBounds( m_img.get(), kwl );
   }
}

void ossimInfo::getImageBounds( ossimImageHandler* ih, ossimKeywordlist& kwl) const
{
   if ( ih )
   {  
      std::vector<ossim_uint32> entryList;
      ih->getEntryList(entryList);

      std::vector<ossim_uint32>::const_iterator i = entryList.begin();
      while ( i != entryList.end() )
      {
         getImageBounds( ih, (*i), kwl );
         ++i;
      }
   } 
}

void ossimInfo::getImageBounds( ossimImageHandler* ih,
                                ossim_uint32 entry, 
                                ossimKeywordlist& kwl ) const
{
   if ( ih )
   {
      if ( ih->setCurrentEntry(entry) )
      {
         ossimString prefix = "image";
         prefix = prefix + ossimString::toString(entry) + ".bounds.";

         ossimRefPtr<ossimImageGeometry> geom = ih->getImageGeometry();
         if(geom.valid())
         {
            ossimDrect bounds;
            geom->getBoundingRect( bounds );

            // Make edge to edge.
            bounds.expand( ossimDpt(0.5, 0.5) );

            if( !bounds.hasNans() )
            {
               ossimGpt gPt;

               geom->localToWorld(bounds.ul(), gPt);
               kwl.add(prefix, "ul", gPt.toString().c_str(), true);

               geom->localToWorld(bounds.ur(), gPt);
               kwl.add(prefix, "ur", gPt.toString().c_str(), true);

               geom->localToWorld(bounds.lr(), gPt);
               kwl.add(prefix, "lr", gPt.toString().c_str(), true);

               geom->localToWorld(bounds.ll(), gPt);
               kwl.add(prefix, "ll", gPt.toString().c_str(), true);
            }
         }

      } // if ( ih->setCurrentEntry(entry) )
      else
      {
         ossimNotify(ossimNotifyLevel_WARN)
                  << "Could not get image bounds for: " << ih->getFilename() << std::endl;
      }

   } // if ( ih )
}

void ossimInfo::getImg2grd(ossimKeywordlist& kwl)
{
   if ( m_img.valid() )
   {
      getImg2grd( m_img.get(), kwl );
   }
}

void ossimInfo::getImg2grd( ossimImageHandler* ih, ossimKeywordlist& kwl) const
{
   if ( ih )
   {  
      std::vector<ossim_uint32> entryList;
      ih->getEntryList(entryList);

      std::vector<ossim_uint32>::const_iterator i = entryList.begin();
      while ( i != entryList.end() )
      {
         getImg2grd( ih, (*i), kwl );
         ++i;
      }
   } 
}

void ossimInfo::getImg2grd( ossimImageHandler* ih,
                            ossim_uint32 entry, 
                            ossimKeywordlist& kwl ) const
{
   if ( ih )
   {
      if ( ih->setCurrentEntry(entry) )
      {
         ossimString prefix = "image";
         prefix = prefix + ossimString::toString(entry) + ".";

         ossimRefPtr<ossimImageGeometry> geom = ih->getImageGeometry();
         if(geom.valid())
         {

            ossimDrect bounds;
            geom->getBoundingRect( bounds );

            if( !bounds.hasNans() )
            {
               //---
               // Expand the bounds out to edge of image so caller can do:
               // ossim-info --img2grd -0.5 -0.5 <image.tif>
               //---
               bounds.expand( ossimDpt(0.5, 0.5) );
               std::string value = m_kwl.findKey( IMG2GRD_KW );
               if ( value.size() )
               {
                  ossimDpt ipt;
                  ipt.toPoint( value );
                  if ( bounds.pointWithin( ipt ) )
                  {
                     ossimGpt gpt;
                     gpt.makeNan();
                     geom->localToWorld(ipt, gpt);
                     kwl.add(prefix, "ground_point", gpt.toString().c_str(), true);
                  }
                  else
                  {
                     kwl.add(prefix, "ground_point", "nan", true);
                  }
               }
            }
         }

      } // if ( ih->setCurrentEntry(entry) )
      else
      {
         ossimNotify(ossimNotifyLevel_WARN)
                  << "Could not get ground center for: " << ih->getFilename() << std::endl;
      }

   } // if ( ih )
}

void ossimInfo::getGrd2img(ossimKeywordlist& kwl)
{
   if ( m_img.valid() )
   {
      getGrd2img( m_img.get(), kwl );
   }
}

void ossimInfo::getGrd2img( ossimImageHandler* ih, ossimKeywordlist& kwl) const
{
   if ( ih )
   {  
      std::vector<ossim_uint32> entryList;
      ih->getEntryList(entryList);

      std::vector<ossim_uint32>::const_iterator i = entryList.begin();
      while ( i != entryList.end() )
      {
         getGrd2img( ih, (*i), kwl );
         ++i;
      }
   } 
}

void ossimInfo::getGrd2img( ossimImageHandler* ih,
                            ossim_uint32 entry, 
                            ossimKeywordlist& kwl ) const
{
   if ( ih )
   {
      if ( ih->setCurrentEntry(entry) )
      {
         ossimString prefix = "image";
         prefix = prefix + ossimString::toString(entry) + ".";

         ossimRefPtr<ossimImageGeometry> geom = ih->getImageGeometry();
         if(geom.valid())
         {
            std::string value = m_kwl.findKey( GRD2IMG_KW );
            if ( value.size() )
            {
               ossimGpt gpt;
               ossimDpt dpt;
               gpt.toPoint( value );
               dpt.makeNan();
               geom->worldToLocal(gpt, dpt);
               kwl.add(prefix, "image_point", dpt.toString().c_str(), true);
            }
         }

      } // if ( ih->setCurrentEntry(entry) )
      else
      {
         ossimNotify(ossimNotifyLevel_WARN)
                  << "Could not get ground center for: " << ih->getFilename() << std::endl;
      }

   } // if ( ih )
}

void ossimInfo::getUpIsUpAngle(ossimKeywordlist& kwl)
{
   if ( m_img.valid() )
   {
      getUpIsUpAngle( m_img.get(), kwl );
   }
}

void ossimInfo::getUpIsUpAngle( ossimImageHandler* ih, ossimKeywordlist& kwl) const
{
   if ( ih )
   {  
      std::vector<ossim_uint32> entryList;
      ih->getEntryList(entryList);

      std::vector<ossim_uint32>::const_iterator i = entryList.begin();
      while ( i != entryList.end() )
      {
         getUpIsUpAngle( ih, (*i), kwl );
         ++i;
      }
   } 
}

void ossimInfo::getUpIsUpAngle(ossim_uint32 entry, ossimKeywordlist& kwl)
{
   if ( m_img.valid() )
   {
      getUpIsUpAngle( m_img.get(), entry, kwl );
   }
}

void ossimInfo::getUpIsUpAngle( ossimImageHandler* ih,
                                ossim_uint32 entry, 
                                ossimKeywordlist& kwl ) const
{
   if ( ih )
   {
      bool result = false;

      if ( ih->setCurrentEntry(entry) )
      {
         ossimString prefix = "image";
         prefix = prefix + ossimString::toString(entry) + ".";

         ossimRefPtr<ossimImageGeometry> geom = ih->getImageGeometry();
         if(geom.valid())
         {
            ossim_float64 upIsUp = 0.0;
            if ( geom->isAffectedByElevation() )
            {
               ossimDpt imagePt;
               imagePt.makeNan();
               ossimString lookup = m_kwl.find(UP_IS_UP_GPT_KW);
               if(!lookup.empty())
               {
                  std::istringstream in(lookup.c_str());
                  ossim_float64 lat,lon;
                  in>>lat>>lon;
                  ossimGpt gpt(lat,lon);
                  geom->worldToLocal(gpt, imagePt);
               }
               else
               {
                  lookup = m_kwl.find(UP_IS_UP_IPT_KW);
                  if(!lookup.empty())
                  {
                     std::istringstream in(lookup.c_str());
                     ossim_float64 x,y;
                     in>>x>>y;
                     imagePt = ossimDpt(x,y);                     
                  }
               }
               upIsUp = geom->upIsUpAngle(imagePt);
               kwl.add(prefix, UP_IS_UP_KW, upIsUp, true);
            }
         }

         result = true;

      } // if ( ih->setCurrentEntry(entry) )

      if ( !result )
      {
         ossimNotify(ossimNotifyLevel_WARN)
                  << "Could not get up is up angle for: " << ih->getFilename() << std::endl;
      }

   } // if ( ih )
}


void ossimInfo::getNorthUpAngle(ossimKeywordlist& kwl)
{
   if ( m_img.valid() )
   {
      getNorthUpAngle( m_img.get(), kwl );
   }
}

void ossimInfo::getNorthUpAngle( ossimImageHandler* ih, ossimKeywordlist& kwl) const
{
   if ( ih )
   {  
      std::vector<ossim_uint32> entryList;
      ih->getEntryList(entryList);

      std::vector<ossim_uint32>::const_iterator i = entryList.begin();
      while ( i != entryList.end() )
      {
         getNorthUpAngle( ih, (*i), kwl );
         ++i;
      }
   } 
}

void ossimInfo::getNorthUpAngle(ossim_uint32 entry, ossimKeywordlist& kwl)
{
   if ( m_img.valid() )
   {
      getNorthUpAngle( m_img.get(), entry, kwl );
   }
}

void ossimInfo::getNorthUpAngle( ossimImageHandler* ih,
                                 ossim_uint32 entry, 
                                 ossimKeywordlist& kwl ) const
{
   if ( ih )
   {
      bool result = false;

      if ( ih->setCurrentEntry(entry) )
      {
         ossimString prefix = "image";
         prefix = prefix + ossimString::toString(entry) + ".";

         ossimRefPtr<ossimImageGeometry> geom = ih->getImageGeometry();
         if(geom.valid())
         {
            ossim_float64 northUp = geom->northUpAngle();
            kwl.add(prefix, NORTH_UP_KW, northUp, true);
         }

         result = true;

      } // if ( ih->setCurrentEntry(entry) )

      if ( !result )
      {
         ossimNotify(ossimNotifyLevel_WARN)
                  << "Could not get north up angle for: " << ih->getFilename() << std::endl;
      }

   } // if ( ih )
}

void ossimInfo::getImageRect(ossimKeywordlist& kwl)
{
   if ( m_img.valid() )
   {
      getImageRect( m_img.get(), kwl );
   }
}

void ossimInfo::getImageRect( ossimImageHandler* ih, ossimKeywordlist& kwl) const
{
   if ( ih )
   {  
      std::vector<ossim_uint32> entryList;
      ih->getEntryList(entryList);

      std::vector<ossim_uint32>::const_iterator i = entryList.begin();
      while ( i != entryList.end() )
      {
         getImageRect( ih, (*i), kwl );
         ++i;
      }
   } // if ( ih )
}

bool ossimInfo::getRgbBands(
      ossimImageHandler* ih, ossim_uint32 entry, ossimKeywordlist& kwl ) const
{
   bool result = false;
   if ( ih )
   {
      std::vector<ossim_uint32> bandList;
      result = ih->getRgbBandList( bandList );
      if ( result && ( bandList.size() == 3 ) )
      {
         ossimString os;
         ossim::toSimpleStringList<ossim_uint32>(os, bandList);
         if ( os.size() )
         {
            ossimString prefix = "image";
            prefix = prefix + ossimString::toString(entry) + ".";
            kwl.add(prefix, "rgb_bands", os.c_str(), true);
         }
      }
   }
   return result;

} // End: ossimInfo::getRgbBands( ... )

void ossimInfo::getImageRect(ossim_uint32 entry, ossimKeywordlist& kwl)
{
   if ( m_img.valid() )
   {
      getImageRect( m_img.get(), entry, kwl );
   }
}

void ossimInfo::getImageRect( ossimImageHandler* ih,
                              ossim_uint32 entry, 
                              ossimKeywordlist& kwl ) const
{
   if ( ih )
   {
      if ( ih->setCurrentEntry(entry) )
      {
         ossimString prefix = "image";
         prefix = prefix + ossimString::toString(entry) + ".";
         ossimIrect outputRect = ih->getBoundingRect();
         kwl.add(prefix, "image_rectangle", outputRect.toString().c_str(), true);

      } // if ( ih->setCurrentEntry(entry) )
      else
      {
         ossimNotify(ossimNotifyLevel_WARN)
                  << "Could not get image rectangle for: " << ih->getFilename() << std::endl;
      }

   } // if ( ih )

} // End: getImageRect( ih, entry...

// Note be sure to m_img->setCurrentEntry before calling.
bool ossimInfo::isImageEntryOverview() const
{
   bool result = false; // Have to prove it.
   if ( m_img.valid() )
   {
      result = isImageEntryOverview( m_img.get() );
   }
   return result;
}

bool ossimInfo::isImageEntryOverview( const ossimImageHandler* ih ) const
{
   bool result = false; // Have to prove it.
   if ( ih )
   {     
      ossimString s = "imag";
      ossimRefPtr<ossimProperty> prop = ih->getProperty(s);
      if (prop.valid())
      {
         ossimString s;
         prop->valueToString(s);
         if (s.toFloat32() < 1.0)
         {
            result = true;
         }
      }
   }
   return result;
}

void ossimInfo::printConfiguration() const
{
   printConfiguration( ossimNotify(ossimNotifyLevel_INFO) );
}

std::ostream& ossimInfo::printConfiguration(std::ostream& out) const
{
   out << "\npreferences_keyword_file: "
       << ossimPreferences::instance()->getPreferencesFilename() << "\n"
       << "preferences_keyword_list:\n"
       << ossimPreferences::instance()->preferencesKWL()
       << std::endl;
   return out;
}

void ossimInfo::printFactories(bool keywordListFlag)const
{
   std::vector<ossimString> typeList;
   ossimObjectFactoryRegistry::instance()->getTypeNameList(typeList);
   for(int i = 0; i < (int)typeList.size(); ++i)
   {
      if(keywordListFlag)
      {
         ossimObject* obj = ossimObjectFactoryRegistry::instance()->createObject(typeList[i]);
         if(obj)
         {
            cout << typeList[i] << endl;
            cout << "______________________________________________________" << endl;
            ossimKeywordlist kwl;
            obj->saveState(kwl);
            cout << kwl << endl;
            cout << "______________________________________________________" << endl;
            delete obj;
         }
      }
      else
      {
         cout << typeList[i] << endl;
      }
   }  
}

void ossimInfo::printDatums() const
{
   ossimInfo::printDatums( ossimNotify(ossimNotifyLevel_INFO) );
}

std::ostream& ossimInfo::printDatums(std::ostream& out) const
{
   // Capture the original flags.
   std::ios_base::fmtflags f = out.flags();

   std::vector<ossimString> datumList;
   ossimDatumFactoryRegistry::instance()->getList(datumList);

   std::vector<ossimString>::const_iterator i = datumList.begin();

   while ( i != datumList.end() )
   {
      const ossimDatum* datum = ossimDatumFactoryRegistry::instance()->create(*i);
      if (datum)
      {
         if ( datum->ellipsoid() )
         {
            out << setiosflags(ios::left)
                << setw(7)
                << datum->code().c_str()
                << setw(7)
                << datum->epsgCode()
                << setw(48)
                << datum->name().c_str()
                << setw(10)
                << "Ellipse:"
                << datum->ellipsoid()->name()
                << std::endl;
         }
         else
         {
            out << "No ellipsoid for code: " << (*i) << std::endl;
         }
      }
      else
      {
         out << "No datum for code: " << (*i) << std::endl;
      }

      ++i;
   }

   // Reset flags.
   out.setf(f);

   return out;
}

void ossimInfo::printFonts() const
{
   ossimInfo::printFonts( ossimNotify(ossimNotifyLevel_INFO) );
}

std::ostream& ossimInfo::printFonts(std::ostream& out) const
{
   std::vector<ossimFontInformation> fontInfoList;
   ossimFontFactoryRegistry::instance()->getFontInformation( fontInfoList );

   std::vector<ossimFontInformation>::const_iterator i = fontInfoList.begin();

   while ( i != fontInfoList.end() )
   {
      out << *(i) << endl;
      ++i;
   }

   // Get the default:
   ossimRefPtr<ossimFont> defaultFont = ossimFontFactoryRegistry::instance()->getDefaultFont();
   if ( defaultFont.valid() )
   {
      out << "default_font: " << defaultFont->getFamilyName() << std::endl;
   }

   return out;
}

void ossimInfo::deg2rad(const ossim_float64& degrees) const
{
   deg2rad( degrees, ossimNotify(ossimNotifyLevel_INFO) );
}

std::ostream& ossimInfo::deg2rad(const ossim_float64& degrees, std::ostream& out) const
{
   double radians = degrees * RAD_PER_DEG;

   // Capture the original flags.
   std::ios_base::fmtflags f = out.flags();

   out << std::setiosflags(std::ios::fixed) << std::setprecision(15)
   << "\n" << degrees << " degrees = "
   << radians << " radians.\n" << std::endl;

   // Reset flags.
   out.setf(f);

   return out;
}

std::ostream& ossimInfo::ecef2llh(const ossimEcefPoint& ecefPoint, std::ostream& out) const
{
   out << "ECEF:            " << ecefPoint.toString() << "\n"
       << "lat_lon_height:  " << ossimGpt(ecefPoint).toString() << "\n"; 

   return out;
}

void ossimInfo::rad2deg(const ossim_float64& radians) const
{
   rad2deg(radians, ossimNotify(ossimNotifyLevel_INFO));
}

std::ostream& ossimInfo::rad2deg(const ossim_float64& radians, std::ostream& out) const
{
   double degrees = radians * DEG_PER_RAD;

   // Capture the original flags.
   std::ios_base::fmtflags f = out.flags();

   out << std::setiosflags(std::ios::fixed) << std::setprecision(15)
   << "\n" << radians << " radians = "
   << degrees << " degrees.\n" << std::endl;

   // Reset flags.
   out.setf(f);

   return out;
}

void ossimInfo::ft2mtrs(const ossim_float64& feet, bool us_survey) const
{
   ft2mtrs( feet, us_survey, ossimNotify(ossimNotifyLevel_INFO) );
}

std::ostream& ossimInfo::ft2mtrs(const ossim_float64& feet,
                                 bool us_survey,
                                 std::ostream& out) const
{
   ossim_float64 meters = 0.0;
   std::string conversionString;
   if (us_survey)
   {
      meters = feet * US_METERS_PER_FT;
      conversionString = "0.3048006096";
   }
   else
   {
      meters = feet * MTRS_PER_FT;
      conversionString = "0.3048";
   }

   // Capture the original flags.
   std::ios_base::fmtflags f = out.flags();

   out << setiosflags(ios::fixed) << setprecision(15)
             << feet << " * " << conversionString << " = "
             << meters << " meters." << std::endl;

   // Reset flags.
   out.setf(f);

   return out;
}

void ossimInfo::mtrs2ft(const ossim_float64& meters, bool us_survey) const
{
   mtrs2ft(meters, us_survey, ossimNotify(ossimNotifyLevel_INFO));
}

std::ostream& ossimInfo::mtrs2ft(const ossim_float64& meters,
                                 bool us_survey,
                                 std::ostream& out) const
{
   // Capture the original flags.
   std::ios_base::fmtflags f = out.flags();

   double feet = 0.0;
   std::string conversionString;

   if (us_survey)
   {
      feet = meters / US_METERS_PER_FT;
      conversionString = "0.3048006096";
   }
   else
   {
      feet = meters / MTRS_PER_FT;
      conversionString = "0.3048";
   }

   out << setiosflags(ios::fixed) << setprecision(15)
             << meters << " / " << conversionString << " = "
             << feet << " feet." << std::endl;

   // Reset flags.
   out.setf(f);

   return out;
}

void ossimInfo::mtrsPerDeg(const ossim_float64& latitude) const
{
   mtrsPerDeg(latitude, ossimNotify(ossimNotifyLevel_INFO));
}

std::ostream& ossimInfo::mtrsPerDeg(const ossim_float64& latitude, std::ostream& out) const
{
   ossimGpt gpt(latitude, 0.0);
   ossimDpt      mpd          = gpt.metersPerDegree();  
   ossim_float64 radius       = gpt.datum()->ellipsoid()->geodeticRadius(latitude);
   ossim_float64 arcLengthLat = mpd.y/60.0;
   ossim_float64 arcLengthLon = mpd.x/60.0;
   out << setiosflags(ios::fixed) << setprecision(15)
             << "Meters per degree and minute at latitude of " << latitude << ":\n"
             << "Meters per degree latitude:   "
             << setw(20) << mpd.y << "\n"
             << "Meters per degree longitude:  "
             << setw(20) << mpd.x << "\n"
             << "Meters per minute latitude:   "
             << setw(20) << arcLengthLat << "\n"
             << "Meters per minute longitude:  "
             << setw(20) << arcLengthLon << "\n"
             << "Geodetic radius:              "
             << setw(20) << radius << "\n"
             << std::endl;
   return out;
}

void ossimInfo::outputHeight(const ossimGpt& gpt) const
{
   outputHeight(gpt, ossimNotify(ossimNotifyLevel_INFO));
}

std::ostream& ossimInfo::outputHeight(const ossimGpt& gpt, std::ostream& out) const
{
   // Capture the original flags.
   std::ios_base::fmtflags f = out.flags();
   
   ossimKeywordlist kwl;
   getHeight( gpt, kwl, std::string("") ); // no prefix
   out << kwl << std::endl;
   
   // Reset flags.
   out.setf(f);
   
   return out;
}

void ossimInfo::getHeight( const ossimGpt& gpt,
                           ossimKeywordlist& kwl,
                           const std::string& prefix ) const
{
   // Handle wrap conditions.
   ossimGpt copyGpt = gpt;
   copyGpt.wrap();

   ossim_float64 hgtAboveMsl = ossim::nan();
   ossim_float64 hgtAboveEllipsoid = ossim::nan();
   ossim_float64 geoidOffset = ossim::nan();
   std::string geoidName = "not_found";

   ossimFilename cellFilename;
   ossimElevManager::instance()->getCellFilenameForPoint( copyGpt, cellFilename );
   if ( cellFilename.empty() )
   {
      cellFilename = "not_found";
   }
   
   ossimRefPtr<ossimElevationDatabase> db =
      ossimElevManager::instance()->getElevationDatabaseForPoint( copyGpt );
   if ( db.valid() )
   {
      hgtAboveMsl = db->getHeightAboveMSL(copyGpt);
      hgtAboveEllipsoid = db->getHeightAboveEllipsoid(copyGpt);
      ossimGeoid* geoid = db->getGeoid();
      if ( geoid )
      {
         geoidOffset = geoid->offsetFromEllipsoid(copyGpt);
         geoidName = geoid->getShortName().string();
      }
   }

   //---
   // If no cell coverage was found for the ground point; perhaps over the
   // ocean, use the geoid offset from the geoid manager. This will typically
   // be the first/top geoid in the array if multiple geoids are loaded.
   //---
   if ( ossim::isnan( geoidOffset ) )
   {
      ossimRefPtr<ossimGeoid> geoid = ossimGeoidManager::instance()->
         getGeoidForPoint(copyGpt);
      if ( geoid.valid() )
      {
         geoidOffset = geoid->offsetFromEllipsoid(copyGpt);
         geoidName = geoid->getShortName().string();

         if ( ossim::isnan( hgtAboveEllipsoid ) )
         {
            hgtAboveEllipsoid = geoidOffset;
         }
      }
   }

   kwl.addPair( prefix, std::string("elevation.info.cell"), cellFilename.string() );
   kwl.addPair( prefix, std::string("elevation.info.gpt"), copyGpt.toString().string() );
   kwl.addPair( prefix, std::string("elevation.info.geoid_name"), geoidName );
   kwl.addPair( prefix, std::string("elevation.info.geoid_offset"),
                ossimString::toString(geoidOffset).string() );
   kwl.addPair( prefix, std::string("elevation.info.height_above_msl"),
                ossimString::toString(hgtAboveMsl).string() );
   kwl.addPair( prefix, std::string("elevation.info.height_above_ellipsoid"),
                ossimString::toString(hgtAboveEllipsoid).string() );
}

void ossimInfo::printExtensions() const
{
   printExtensions(ossimNotify(ossimNotifyLevel_INFO));
}

std::ostream& ossimInfo::printExtensions(std::ostream& out) const
{
   ossimImageHandlerFactoryBase::UniqueStringList extList;
   ossimImageHandlerRegistry::instance()->getSupportedExtensions(extList);
   const vector<ossimString>& list = extList.getList();

   if (list.empty())
   {
      out << "No image file extensions handled. This should never happen!" << std::endl;
      return out;
   }

   out<<"\nImage Entensions Supported:"<< endl;
   for (const auto& extension : list)
      out<<"  "<<extension<<endl;

   out<<endl;
   return out;
}

void ossimInfo::printPlugins() const
{
   printPlugins(ossimNotify(ossimNotifyLevel_INFO));
}

std::ostream& ossimInfo::printPlugins(std::ostream& out) const
{
   if(ossimSharedPluginRegistry::instance()->getNumberOfPlugins() > 0)
   {
      ossimSharedPluginRegistry::instance()->printAllPluginInformation(out);
   }
   else
   {
      out << "No plugins loaded in the OSSIM core library" << std::endl;
   }
   return out;
}

void ossimInfo::testPlugin(const ossimFilename& plugin) const
{
   testPlugin(plugin, ossimNotify(ossimNotifyLevel_INFO));
}

std::ostream& ossimInfo::testPlugin(const ossimFilename& plugin, std::ostream& out) const
{
   if( ossimSharedPluginRegistry::instance()->registerPlugin(plugin.expand()) )
   {
      out << "Plugin loaded: " << plugin << std::endl;
   }
   else
   {
      out << "Unable to load plugin: " << plugin << std::endl;
   }
   return out;
}

void ossimInfo::printOverviewTypes() const
{
   printOverviewTypes(ossimNotify(ossimNotifyLevel_INFO));
}

std::ostream& ossimInfo::printOverviewTypes(std::ostream& out) const
{
   out << "\nValid overview types: " << std::endl;

   std::vector<ossimString> outputType;

   ossimOverviewBuilderFactoryRegistry::instance()->getTypeNameList(outputType);
   std::copy(outputType.begin(),
             outputType.end(),
             std::ostream_iterator<ossimString>(out, "\t\n"));
   return out;
}

void ossimInfo::printProjections() const
{
   printProjections(ossimNotify(ossimNotifyLevel_INFO));
}

std::ostream& ossimInfo::printProjections(std::ostream& out) const
{
   out << "Projections:\n";

   std::vector<ossimString> list;
   ossimProjectionFactoryRegistry::instance()->
         getAllTypeNamesFromRegistry(list);

   std::vector<ossimString>::const_iterator i = list.begin();
   while ( i != list.end() )
   {
      out << *i << "\n";
      ++i;
   }
   out << std::endl;

   return out;
}

void ossimInfo::printReaderProps() const
{
   printReaderProps(ossimNotify(ossimNotifyLevel_INFO));
}

std::ostream& ossimInfo::printReaderProps(std::ostream& out) const
{
   return ossimImageHandlerRegistry::instance()->printReaderProps( out );
}

void ossimInfo::printResamplerFilters() const
{
   printResamplerFilters(ossimNotify(ossimNotifyLevel_INFO));
}

std::ostream& ossimInfo::printResamplerFilters(std::ostream& out) const
{
   std::vector<ossimString> list;
   ossimFilterResampler f;
   f.getFilterTypes(list);
   std::vector<ossimString>::const_iterator i = list.begin();
   while ( i != list.end() )
   {
      out << *i << "\n";
      ++i;
   }
   out << std::endl;
   return out;
}

void ossimInfo::printWriters() const
{
   printWriters(ossimNotify(ossimNotifyLevel_INFO));
}

std::ostream& ossimInfo::printWriters(std::ostream& out) const
{
   return ossimImageWriterFactoryRegistry::instance()->printImageTypeList( out );
}

void ossimInfo::printZoomLevelGsds() const
{
   printZoomLevelGsds(ossimNotify(ossimNotifyLevel_INFO));
}

std::ostream& ossimInfo::printZoomLevelGsds(std::ostream& out) const
{
   // Capture the original flags.
   std::ios_base::fmtflags f = out.flags();

   out << setprecision(15)<< setiosflags(std::ios_base::fixed|std::ios_base::right);

   const int MAX_LEVEL = 24;
   const double TILE_SIZE = 256.0;
   const double EPSG_4326_BOUNDS = 180.0;
   const double EPSG_3857_BOUNDS = 40075016.685578488;

   // From: ossim-info --mtrsPerDeg 0.0
   const double MTRS_PER_DEGREE_AT_EQUATOR = 111319.490793273565941;

   out << "Notes:\n"
         << "tile size: 256\n"
         << "dpp = \"degrees per pixel\"\n"
         << "mpp = \"meters per pixel\"\n\n";

   // Assuming square pixels, level 0 having (2 x 1) tiles.
   double level_0_gsd = EPSG_4326_BOUNDS / TILE_SIZE;
   double level_gsd = 0.0;
   int i = 0;
   int tilesX = 2;
   int tilesY = 1;

   out << "EPSG:4326 level info:\n"
       << "Note: Assuming square pixels, level 0 having (2x1) tiles.\n"
       << "bounds: 360.0 X 180.0\n"
       << "level[" << std::setw(2) << std::setfill('0') << i << "] dpp:"
       << std::setw(18) << std::setfill(' ') << level_0_gsd
       << "  equivalent mpp:" << std::setw(22)
       << (level_0_gsd * MTRS_PER_DEGREE_AT_EQUATOR)
       << " (" << tilesX << "x" << tilesY << ")" << "\n";

   for ( i = 1; i <= MAX_LEVEL; ++i )
   {
      tilesX = tilesX << 1;
      tilesY = tilesY << 1;
      level_gsd = level_0_gsd / std::pow( 2.0, i );
      out << "level[" << std::setw(2) << std::setfill('0') << i << "] dpp:"
          << std::setw(18) << std::setfill(' ') << level_gsd
          << "  equivalent mpp:" << std::setw(22)
          << (level_gsd * MTRS_PER_DEGREE_AT_EQUATOR)
          << " (" << tilesX << "x" << tilesY << ")"<< "\n";

   }

   // Assuming square pixels, level 0 having (1 x 1) tiles.
   level_0_gsd = EPSG_3857_BOUNDS / TILE_SIZE;
   level_gsd = 0.0;
   i = 0;
   tilesX = 1; // X and y the same.

   out << "\n\nEPSG:3857 level info:\n"
       << "Note: Assuming square pixels, level 0 having (1x1) tile.\n"
       << "bounds: 40075016.685578488 X 40075016.685578488\n"
       << "level[" << std::setw(2) << std::setfill('0') << i << "] mpp:"
       << std::setw(23) << std::setfill(' ') << level_0_gsd
       << " (" << tilesX << "x" << tilesX << ")" << "\n";

   for ( i = 1; i <= MAX_LEVEL; ++i )
   {
      tilesX = tilesX << 1;
      level_gsd = level_0_gsd / std::pow( 2.0, i );
      out << "level[" << std::setw(2) << std::setfill('0') << i << "] mpp:"
          << std::setw(23) << std::setfill(' ') << level_gsd
          << " (" << tilesX << "x" << tilesX << ")" << "\n";
   }

   // Reset flags.
   out.setf(f);

   return out;

} // End: ossimInfo::printZoomLevelGsds(std::ostream& out)

void ossimInfo::printWriterProps() const
{
   printWriterProps(ossimNotify(ossimNotifyLevel_INFO));
}

std::ostream& ossimInfo::printWriterProps(std::ostream& out) const
{
   return ossimImageWriterFactoryRegistry::instance()->printWriterProps( out );
}

void ossimInfo::getRadiometry(ossimScalarType scalar, std::string& s) const
{
   // Output Radiometry.
   switch(scalar)
   {
   case OSSIM_UINT8:
   {
      s = "8-bit";
      break;
   }
   case OSSIM_USHORT11:
   {
      s = "11-bit";
      break;
   }
   case OSSIM_USHORT12:
   {
      s = "12-bit";
      break;
   }
   case OSSIM_USHORT13:
   {
      s = "13-bit";
      break;
   }
   case OSSIM_USHORT14:
   {
      s = "14-bit";
      break;
   }
   case OSSIM_USHORT15:
   {
      s = "15-bit";
      break;
   }
   case OSSIM_UINT16:
   {
      s = "16-bit unsigned";
      break;
   }
   case OSSIM_SINT16:
   {
      s = "16-bit signed";
      break;
   }
   case OSSIM_UINT32:
   {
      s = "32-bit unsigned";
      break;
   }
   case OSSIM_SINT32:
   {
      s = "32-bit signed";
      break;
   }
   case OSSIM_FLOAT32:
   {
      s = "32-bit float";
      break;
   }
   case OSSIM_DOUBLE:
   {
      s = "64-bit double float";
      break;
   }
   case OSSIM_NORMALIZED_FLOAT:
   {
      s = "normalized 32-bit float";
      break;
   }
   case OSSIM_NORMALIZED_DOUBLE:
   {
      s = "normalized 64-bit double float";
      break;
   }
   default:
   {
      s = "unknown";
      break;
   }
   }
}

void ossimInfo::getBuildDate(std::string& s) const
{
#ifdef OSSIM_BUILD_DATE
   s = OSSIM_BUILD_DATE;
#else
   s = "unknown";
#endif
}

void ossimInfo::getRevisionNumber(std::string& s) const
{
#ifdef OSSIM_REVISION
   s = OSSIM_REVISION;
#else
   s = "unknown";
#endif
}

void ossimInfo::getVersion(std::string& s) const
{
#ifdef OSSIM_VERSION
   s = OSSIM_VERSION;
#else
   s = "unknown";
#endif
}

void ossimInfo::outputXml( const ossimKeywordlist& kwl ) const
{
   ossimXmlDocument document;
   document.fromKwl( kwl );
   ossimNotify(ossimNotifyLevel_INFO) << document << std::endl;
}

void ossimInfo::outputXml( const ossimKeywordlist& kwl, const ossimFilename& file  ) const
{
   ossimXmlDocument document;
   document.fromKwl( kwl );
   document.write( file );
}

bool ossimInfo::keyIsTrue( const std::string& key ) const
{
   bool result = false;
   std::string value = m_kwl.findKey( key );
   if ( value.size() )
   {
      result = ossimString(value).toBool();
   }
   return result;
}

void ossimInfo::checkConfig() const
{
   checkConfig( ossimNotify(ossimNotifyLevel_INFO) );
}

std::ostream& ossimInfo::checkConfig(std::ostream& out) const
{
   // Check some common environment variables:
   out << "\nChecking some common environment variables...\n\n";
   
   ossimString key = "OSSIM_PREFS_FILE";
   ossimString envVar = ossimEnvironmentUtility::instance()->
      getEnvironmentVariable( key );
   if ( envVar.size() )
   {
      out << key << " = " << envVar << "\n";
   }
   else
   {
      out << key << " is NOT set!\n";
   }

   key = "OSSIM_DATA";
   envVar = ossimEnvironmentUtility::instance()->getEnvironmentVariable( key );
   if ( envVar.size() )
   {
      out << key << " = " << envVar << "\n";
   }
   else
   {
      out << key << " is NOT set!\n";
   }

   key = "OSSIM_INSTALL_PREFIX";
   envVar = ossimEnvironmentUtility::instance()->getEnvironmentVariable( key );
   if ( envVar.size() )
   {
      out << key << " = " << envVar << "\n";
   }
   else
   {
      out << key << " is NOT set!\n";
   }
   
    // Check for an ossim preferences file:
    out << "\nChecking for ossim prefences file...\n\n";

    ossimFilename prefs = ossimPreferences::instance()->getPreferencesFilename();
    if ( prefs.exists() == true )
    {
       out << "Preferences file loaded:\n" << prefs << "\n";
   }
   else
   {
      out << "ERROR: ossim preferences file does NOT exists!\n"
          << "Set environment variable OSSIM_PREFS_FILE to point to a valid ossim preferences\n"
          << "or use the \"-P <pref_file>\" option on any ossim command line app to override\n"
          << "the environment variable.\n"
          << "Notes:\n"
          << "1) There is a template in:\n"
          << "   ossim_install_dir/share/ossim/ossim-preferences-template\n"
          << "   This can be used to create an ossim-site-preferences file.\n"
          << "2) The \"-P <pref_file>\" option is very useful at troubleshooting a new\n"
          << "   ossim preferences file.\n";

     
   }

   out << "\nChecking for plugins...\n\n";

   ossim_uint32 count = ossimSharedPluginRegistry::instance()->getNumberOfPlugins();
   if ( count )
   {
      out << "Plugins loaded:\n";
      for(ossim_uint32 index = 0; index < count; ++index)
      {
         std::vector<ossimString> classNames;
         const ossimPluginLibrary* pi = ossimSharedPluginRegistry::instance()->getPlugin(index);
         if(pi)
         {
            out << "plugin[" << index << "]: " << pi->getName() << "\n";
         }
      }
   }
   else
   {
      out << "WARNING: No plugins loaded!\n"
          << "Notes:\n"
          << "1) The plugins are set in the ossim preferences file.\n"
          << "2) Typical plugin line references the environment variable OSSIM_INSTALL_PREFIX.\n"
          << "   If so, make sure that is set.\n";
   }
   
   out << "\nChecking for geoids...\n\n";
   
   count = ossimGeoidManager::instance()->getNumberOfGeoids();
   if ( count )
   {
      out << "Geoids loaded:\n";
      for ( ossim_uint32 index = 0; index < count; ++index )
      {
         ossimRefPtr<ossimGeoid> geoid = ossimGeoidManager::instance()->getGeoid( index );
         if ( geoid.valid() )
         {
            out << "geoid[" << index << "] " << geoid->getShortName() << "\n";
         }
         else
         {
            out << "geoid[" << index << "] is NULL!\n";
         }
      }
   }
   else
   {
      out << "\nThere are no geoids loaded!\n"
          << "Typical elevation sources, e.g. DTED, are relative to some geoid.\n";
   }
   
   out << "\nChecking for elevation databases...\n\n";
   count = ossimElevManager::instance()->getNumberOfElevationDatabases();
   if ( count )
   {
      out << "elevation sources loaded:\n";
      for ( ossim_uint32 index = 0; index < count; ++index )
      {
         const ossimElevationDatabase* edb = ossimElevManager::instance()->getElevationDatabase( index );
         if ( edb )
         {
            ossimFilename f = edb->getConnectionString();
            out << "elevation_source[" << index << "].connetion_string: " << f << "\n";
            if ( f.exists() == false )
            {
               out << "\nWARNING: " << f << " doesn NOT exists!\n"
                   << "Correct the path, or disable this elevation source in the ossim prefences "
                   << "file.\n\n";
            }
         }
         else
         {
            out << "\nelevation_source[" << index << "] is NULL!\n";
         }
      }

      out << "\nTesting some elevation points...\n";

      ossimKeywordlist kwl;

      std::string prefix = "denver_airport_us.";
      ossimGpt gpt(39.850929, -104.696613, 0.0);
      getHeight( gpt, kwl, prefix );
      
      prefix = "dulles_airport_us.";
      gpt.lat = 38.938116;
      gpt.lon = -77.459796;
      getHeight( gpt, kwl, prefix );

      prefix = "hobart_airport_tasmania.";
      gpt.lat = -42.82896;
      gpt.lon = 147.50203;
      getHeight( gpt, kwl, prefix );

      prefix = "changjin_afb_north_korea.";
      gpt.lat = 40.36444;
      gpt.lon = 127.26422;
      getHeight( gpt, kwl, prefix );

      prefix = "wuhan_airport_china.";
      gpt.lat = 30.77235;
      gpt.lon = 114.196525;
      getHeight( gpt, kwl, prefix );

#if 0
      prefix = "mount_everest.";
      gpt.lat = 27.988055555555558;
      gpt.lon = 86.925277777777779;
      getHeight( gpt, kwl, prefix );
#endif

      prefix = "aleppo_airport_syria.";
      gpt.lat = 36.17915;
      gpt.lon = 37.24014;
      getHeight( gpt, kwl, prefix );

      prefix = "mehrabad_airport_iran.";
      gpt.lat = 35.694610;
      gpt.lon = 51.291831;
      getHeight( gpt, kwl, prefix );

      prefix = "jalalabad_afghanistan.";
      gpt.lat = 34.429708;
      gpt.lon = 70.451630;      ;
      getHeight( gpt, kwl, prefix );

      prefix = "buur_hakaba_somalia.";
      gpt.lat = 2.79716;
      gpt.lon = 44.07852;
      getHeight( gpt, kwl, prefix );
      
      prefix = "atlantic_ocean.";
      gpt.lat = 22.82;
      gpt.lon = -21.74;
      getHeight( gpt, kwl, prefix );

      prefix = "placetas_cuba.";
      gpt.lat = 22.32002;
      gpt.lon = -79.65626;
      getHeight( gpt, kwl, prefix );

      prefix = "bogota_columbia.";
      gpt.lat = 4.70044;
      gpt.lon = -74.01521;
      getHeight( gpt, kwl, prefix );

      out << "\n" << kwl << "\n";
   }
   else
   {
      out << "There are no elevation sources loaded!\n";
   }

   return out;
}
