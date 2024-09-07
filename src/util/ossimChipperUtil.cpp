//---
//
// File: ossimChipperUtil.cpp
//
// License: MIT
//
// Author:  David Burken
//
// Description: Utility class definition processing digital elevation
// models(dems).
//
//---
// $Id$

#include <ossim/util/ossimChipperUtil.h>

#include <ossim/base/ossimArgumentParser.h>
#include <ossim/base/ossimApplicationUsage.h>
#include <ossim/base/ossimConnectableContainer.h>
#include <ossim/base/ossimConnectableObject.h>
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
#include <ossim/base/ossimGrect.h>

#include <ossim/imaging/ossimAnnotationFontObject.h>
#include <ossim/imaging/ossimAnnotationLineObject.h>
#include <ossim/imaging/ossimAnnotationSource.h>
#include <ossim/imaging/ossimBrightnessContrastSource.h>
#include <ossim/imaging/ossimBumpShadeTileSource.h>
#include <ossim/imaging/ossimFilterResampler.h>
#include <ossim/imaging/ossimFusionCombiner.h>
#include <ossim/imaging/ossimGammaRemapper.h>
#include <ossim/imaging/ossimImageData.h>
#include <ossim/imaging/ossimImageFileWriter.h>
#include <ossim/imaging/ossimImageGeometry.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/imaging/ossimImageHistogramSource.h>
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

#include <cmath>
#include <sstream>
#include <string>

using namespace std;

static ossimTrace traceDebug("ossimChipperUtil:debug");
static ossimTrace traceLog("ossimChipperUtil:log");
static ossimTrace traceOptions("ossimChipperUtil:options");

static const std::string BRIGHTNESS_KW = "brightness";
static const std::string COLOR_BLUE_KW = "color_blue";
static const std::string COLOR_GREEN_KW = "color_green";
static const std::string COLOR_RED_KW = "color_red";
static const std::string CONTRAST_KW = "contrast";
static const std::string CLIP_WMS_BBOX_LL_KW = "clip_wms_bbox_ll";
static const std::string CLIP_POLY_LAT_LON_KW = "clip_poly_lat_lon";
static const std::string CUT_BBOX_XYWH_KW = "cut_bbox_xywh";
static const std::string CUT_WMS_BBOX_KW = "cut_wms_bbox";
static const std::string CUT_WMS_BBOX_LL_KW = "cut_wms_bbox_ll";
static const std::string CUT_CENTER_LAT_KW = "cut_center_lat";
static const std::string CUT_CENTER_LON_KW = "cut_center_lon";
static const std::string CUT_RADIUS_KW = "cut_radius"; // meters
static const std::string CUT_HEIGHT_KW = "cut_height"; // pixels
static const std::string CUT_MAX_LAT_KW = "cut_max_lat";
static const std::string CUT_MAX_LON_KW = "cut_max_lon";
static const std::string CUT_MIN_LAT_KW = "cut_min_lat";
static const std::string CUT_MIN_LON_KW = "cut_min_lon";
static const std::string CUT_WIDTH_KW = "cut_width"; // pixels
static const std::string DEM_KW = "dem";
static const std::string GAIN_KW = "gain";
static const std::string GAMMA_KW = "gamma";
static const std::string FILE_KW = "file";
static const std::string FULLRES_XYS_KW = "fullres_xys";
static const std::string HIST_AOI_KW = "hist_aoi";
static const std::string HIST_CENTER_KW = "hist_center";
static const std::string HIST_LLWH_KW = "hist_llwh";
static const std::string HIST_OP_KW = "hist_op";
static const std::string HIST_LINEAR_CLIP_KW = "hist_linear_clip";
static const std::string HIST_LINEAR_NORM_CLIP_KW = "hist_linear_norm_clip";
static const std::string HIST_CENTER_CLIP_KW = "hist_center_clip";
static const std::string HIST_BIAS_FACTOR_KW = "hist_bias_factor";
static const std::string IMAGE_SPACE_SCALE_X_KW = "image_space_scale_x";
static const std::string IMAGE_SPACE_SCALE_Y_KW = "image_space_scale_y";
static const std::string IMG_KW = "image";
static const std::string LUT_FILE_KW = "lut_file";
static const std::string DEGREES_X_KW = "degrees_x";
static const std::string DEGREES_Y_KW = "degrees_y";
static const std::string METERS_KW = "meters";
static const std::string NORTH_UP_KW = "north_up"; // bool
static const std::string OP_KW = "operation";
static const std::string OUTPUT_RADIOMETRY_KW = "output_radiometry";
static const std::string PAD_THUMBNAIL_KW = "pad_thumbnail"; // bool
static const std::string READER_PROPERTY_KW = "reader_property";
static const std::string RESAMPLER_FILTER_KW = "resampler_filter";
static const std::string ROTATION_KW = "rotation";
static const std::string ROTATE_TO_INPUT = "rotate_to_input";
static const std::string RRDS_KW = "rrds";
static const std::string SCALE_2_8_BIT_KW = "scale_2_8_bit";
static const std::string SHARPEN_MODE_KW = "sharpen_mode";
static const std::string SHARPEN_PERCENT_KW = "sharpen_percent";
static const std::string SNAP_TIE_TO_ORIGIN_KW = "snap_tie_to_origin";
static const std::string SRC_FILE_KW = "src_file";
static const std::string SRS_KW = "srs";
static const std::string THREE_BAND_OUT_KW = "three_band_out";					// bool
static const std::string THUMBNAIL_RESOLUTION_KW = "thumbnail_resolution"; // pixels
static const std::string TILE_SIZE_KW = "tile_size";								// pixels
static const std::string TRUE_KW = "true";
static const std::string UP_IS_UP_KW = "up_is_up"; // bool
static const std::string WRITER_KW = "writer";
static const std::string WRITER_PROPERTY_KW = "writer_property";
static const std::string COMBINER_TYPE_KW = "combiner_type";
static const std::string NULL_PIXEL_FLIP_KW = "null_pixel_flip";

static const std::string TWOCMV_OLD_INPUT_BAND_KW = "2cmv_old_input_band";
static const std::string TWOCMV_NEW_INPUT_BAND_KW = "2cmv_new_input_band";
static const std::string TWOCMV_RED_OUTPUT_SOURCE_KW = "2cmv_red_output_source";
static const std::string TWOCMV_GREEN_OUTPUT_SOURCE_KW = "2cmv_green_output_source";
static const std::string TWOCMV_BLUE_OUTPUT_SOURCE_KW = "2cmv_blue_output_source";

ossimChipperUtil::ossimChipperUtil()
    : ossimReferenced(),
      m_operation(OSSIM_CHIPPER_OP_UNKNOWN),
      m_kwl(new ossimKeywordlist()),
      m_srcKwl(0),
      m_geom(0),
      m_ivt(0),
      m_demLayer(0),
      m_imgLayer(0),
      m_viewPortStretchEnabled(false)
{
   // traceDebug.setTraceFlag(true);

   m_kwl->setExpandEnvVarsFlag(true);
}

// Private/hidden from use.
ossimChipperUtil::ossimChipperUtil(const ossimChipperUtil & /* obj */)
   : m_operation(OSSIM_CHIPPER_OP_UNKNOWN)
{
}

// Private/hidden from use.
const ossimChipperUtil &ossimChipperUtil::operator=(const ossimChipperUtil & /* rhs */)
{
   return *this;
}

ossimRefPtr<ossimImageSource> ossimChipperUtil::createCombiner() const
{
   ossimRefPtr<ossimImageSource> result;
   ossimString combinerType = m_kwl->find(COMBINER_TYPE_KW.c_str());
   if (combinerType.empty())
   {
      combinerType = "ossimImageMosaic";
   }

   result = ossimImageSourceFactoryRegistry::instance()->createImageSource(combinerType);
   if (!result.valid())
   {
      result = new ossimImageMosaic();
   }

   return result;
}

ossimChipperUtil::~ossimChipperUtil()
{
   clear();
}

void ossimChipperUtil::addArguments(ossimArgumentParser &ap)
{
   ossimString usageString = ap.getApplicationName();
   usageString += " [option]... [input-option]... <input-file(s)> <output-file>\nNote at least one input is required either from one of the input options, e.g. --input-dem <my-dem.hgt> or adding to command line in front of the output file in which case the code will try to ascertain what type of input it is.\n\nAvailable traces:\n-T \"ossimChipperUtil:debug\"   - General debug trace to standard out.\n-T \"ossimChipperUtil:log\"     - Writes a log file to output-file.log.\n-T \"ossimChipperUtil:options\" - Writes the options to output-file-options.kwl.";

   ossimApplicationUsage *au = ap.getApplicationUsage();

   au->setCommandLineUsage(usageString);

   au->setDescription(ap.getApplicationName() + " Utility application for generating elevation products from dem data.");

   au->addCommandLineOption("--azimuth", "<azimuth>\nhillshade option - Light source azimuth angle for bump shade.\nRange: 0 to 360, Default = 180.0");

   au->addCommandLineOption("-b or --bands <n,n...>", "Use the specified bands in given order: e.g. \"3,2,1\" will select bands 3, 2 and 1 of the input image.\nNote: it is 1 based");

   au->addCommandLineOption("--brightness", "<brightness>\nApply brightness to input image(s). Valid range: -1.0 to 1.0");

   au->addCommandLineOption("--central-meridian", "<central_meridian_in_decimal_degrees>\nNote if set this will be used for the central meridian of the projection.  This can be used to lock the utm zone.");

   au->addCommandLineOption("--color", "<r> <g> <b>\nhillshade option - Set the red, green and blue color values to be used with hillshade.\nThis option can be used with or without an image source for color.\nRange 0 to 255, Default r=255, g=255, b=255");

   au->addCommandLineOption("--color-table", "<color-table.kwl>\nhillshade or color-relief option - Keyword list containing color table for color-relief option.");

   au->addCommandLineOption("--contrast", "<contrast>\nApply contrast to input image(s). Valid range: -1.0 to 1.0");

   au->addCommandLineOption("--cut-bbox-xywh", "<x>,<y>,<width>,<height>\nSpecify a comma separated bounding box.");

   au->addCommandLineOption("--cut-wms-bbox", "<minx>,<miny>,<maxx>,<maxy>\nSpecify a comma separated list in the format of a WMS BBOX.\nThe units are in the units of the projector defined by the --srs key");

   au->addCommandLineOption("--cut-wms-bbox-ll", "<minx>,<miny>,<maxx>,<maxy>\nSpecify a comma separated list in the format of a WMS BBOX.\nThe units are always decimal degrees");

   au->addCommandLineOption("--cut-width", "<width>\nSpecify the cut width in pixel");

   au->addCommandLineOption("--cut-height", "<height>\nSpecify the cut height in pixel");

   au->addCommandLineOption("--clip-wms-bbox-ll", "<minx>,<miny>,<maxx>,<maxy>\nSpecify a comma separated list in the format of a WMS BBOX.\nThe units are always decimal degrees");

   au->addCommandLineOption("--clip-poly-lat-lon", "Polygon in the form of a string: (lat,lon),(lat,lon),...(lat,lon)");

   au->addCommandLineOption("--cut-bbox-ll", "<min_lat> <min_lon> <max_lat> <max_lon>\nSpecify a bounding box with the minimum latitude/longitude and max latitude/longitude in decimal degrees.\nNote coordinates are edge to edge.");

   au->addCommandLineOption("--cut-bbox-llwh", "<min_lat> <min_lon> <max_lat> <max_lon> <width> <height>\nSpecify a bounding box with the minimum latitude/longitude, max latitude/longitude in decimal degrees and width/height in pixels.\nNote coordinates are edge to edge.");

   au->addCommandLineOption("--cut-center-llwh", "<latitude> <longitude> <width> <height>\nSpecify the center cut in latitude longitude space with width and height in pixels.");

   au->addCommandLineOption("--cut-center-llr", "<latitude> <longitude> <radius_in_meters>\nSpecify the center cut in latitude longitude space with radius in meters.");

   au->addCommandLineOption("--degrees", "<dpp_xy> | <dpp_x> <dpp_y>\nSpecifies an override for degrees per pixel. Takes either a single value applied equally to x and y directions, or two values applied correspondingly to x then y. This option takes precedence over the \"--meters\" option.");

   au->addCommandLineOption("--enable-null-pixel-flip", "Will add to the start of the chain and will flip interior pixels that are null to valid.");
   au->addCommandLineOption("--elevation", "<elevation>\nhillshade option - Light source elevation angle for bumb shade.\nRange: 0 to 90, Default = 45.0");

   au->addCommandLineOption("-e or --entry", "<entry> For multi image handlers which entry do you wish to extract. For list of entries use: \"ossim-info -i <your_image>\" ");

   au->addCommandLineOption("--exaggeration", "<factor>\nMultiplier for elevation values when computing surface normals. Has the effect of lengthening shadows for oblique lighting.\nRange: .0001 to 50000, Default = 1.0");

   au->addCommandLineOption("--fullres-xys", "<full res center x>,<full res center y>,<scale>[,<scale>]\nSpecify a full resolution x,y point (Used as pivot and center cut) and scale, comma separated with no spaces.  If two scales are specified then first is x and second is y else x and y are set to equal scales");

   au->addCommandLineOption("--gamma", "Good ranges are between .1 and 6.  Probably try about .7,  A value of 1 will not apply anything");

   au->addCommandLineOption("-h or --help", "Display this help and exit.");

   au->addCommandLineOption("--hemisphere", "<hemisphere>\nSpecify a projection hemisphere if supported. E.g. UTM projection. This will lock the hemisphere even if input scene center is the other hemisphere. Valid values for UTM are \"N\" and \"S\"");

   au->addCommandLineOption("--histogram-aoi", "<x>,<y>,<width>,<height>\nSets single image region of interest(roi) to compute histogram from. Coordinates are zero based image space.  Single image operation only. Comma separated, no spaces.");

   au->addCommandLineOption("--histogram-center-tile", "Will compute histogram from center tile of image.");

   au->addCommandLineOption("--histogram-llwh", "<latitude>,<longitude>,<width>,<height>\nSpecify the region of interest(roi) to compute histogram from. Latitude and longitude will be roi center space with width and height in pixels. Comma separated, no spaces.");

   au->addCommandLineOption("--histogram-op", "<operation>\nHistogram operation to perform. Valid operations are \"linear\" \"auto-minmax\", \"auto-percentile\", \"std-stretch-1\", \"std-stretch-2\" and \"std-stretch-3\".");

   au->addCommandLineOption("--histogram-linear-clip", "<low>,<high> in actual DN value.  If it's 8 bit then it will be values between 0 and 255");
   au->addCommandLineOption("--histogram-linear-norm-clip", "<low>,<high> normalized value that range from 0 to 1. Example .2,.85");
   au->addCommandLineOption("--histogram-center-clip", "<center> normalized value that range from 0 to 1. Default is 0.5 which is the center of the min max clip points.");
   au->addCommandLineOption("--histogram-bias-factor", "<bias factor> values are typically between 0 to 2.  Allows small adjustments to the auto clip points.  Currently only works for auto min max.");

   au->addCommandLineOption("--image-space-scale", "<x> <y>\nSpecifies an image space scale for x and y direction. \"chip\" operation only.");

   au->addCommandLineOption("--input-dem", "<dem> Input dem to process.");

   au->addCommandLineOption("--input-img", "<image> Input image to process.");

   au->addCommandLineOption("--input-src", "<file.src> Input source file list keyword list with list of dems or images or both to process.");

   au->addCommandLineOption("--meters", "<meters>\nSpecifies an override for the meters per pixel");

   au->addCommandLineOption("-n or --north-up", "Rotates image North up. \"chip\" operation only.");

   au->addCommandLineOption("--op", "<operation>\nOperation to perform. Valid operations are \"chip\", \"color-relief\", \"hillshade\", \"psm\"(pan sharpened multispectral), \"2cmv\"(two color multi view) and \"ortho\".\nchip = input projection = output projection(image space), single image operation only.");

   au->addCommandLineOption("--options", "<options.kwl>  This can be all or part of the application options.  To get a template you can turn on trace to the ossimChipperUtil class by adding \"-T ossimChipperUtil\" to your command.");

   au->addCommandLineOption("--origin-latitude", "<latidude_in_decimal_degrees>\nNote if set this will be used for the origin latitude of the projection.  Setting this to something other than 0.0 with a geographic projection creates a scaled geographic projection.");

   au->addCommandLineOption("--output-radiometry", "<R>\nSpecifies the desired product's pixel radiometry type. Possible values for <R> are: U8, U11, U16, S16, F32. Note this overrides the deprecated option \"scale-to-8-bit\".");

   au->addCommandLineOption("--pad-thumbnail", "<boolean>\nIf true, output thumbnail dimensions will be padded in width or height to make square; else, it will have the aspect ratio of input,  Default=false");

   au->addCommandLineOption("--projection", "<output_projection> Valid projections: geo, geo-scaled, input or utm\ngeo = Equidistant Cylindrical, origin latitude = 0.0\ngeo-scaled = Equidistant Cylindrical, origin latitude = image center\ninput Use first images projection. Must be a map projecion.\nutm = Universal Tranverse Mercator\nIf input and multiple sources the projection of the first image will be used.\nIf utm the zone will be set from the scene center of first image.\nNOTE: --srs takes precedence over this option.");

   au->addCommandLineOption("--resample-filter", "<type>\nSpecify what resampler filter to use, e.g. nearest neighbor, bilinear, cubic, sinc.\nSee ossim-info --resampler-filters");

   au->addCommandLineOption("-r or --rotate", "<degrees>\nRotate image by degrees. \"chip\" operation only.");

   au->addCommandLineOption("--reader-prop", "<string>Adds a property to send to the reader. format is name=value");

   au->addCommandLineOption("--rotate-to-input", "\nOnly if the operation is \"ortho\", output a rotated map so that the top edge is oriented with the input image top edge. Only available for single-image products in GeoTIFF output format.");

   au->addCommandLineOption("--rrds", "<rrds> Reduced resolution data set where 0 is full resolution. \"chip\" operation only.");

   au->addCommandLineOption("--scale-to-8-bit", "Scales the output to unsigned eight bits per band. This option has been deprecated by the newer \"--output-radiometry\" option.");

   au->addCommandLineOption("--sharpen-mode", "<mode> Applies sharpness to image chain(s). Valid modes: \"light\", \"medium\",\"heavy\"");
   au->addCommandLineOption("--sharpen-percent", "<sharpen-percent> Applies sharpness to image chain(s). Valid percentage is between 0 and 1");

   au->addCommandLineOption("--snap-tie-to-origin",
                            "Snaps tie point to projection origin so that (tie-origin)/gsd come out on an even integer boundary.");

   au->addCommandLineOption("--srs", "<src_code>\nSpecify a spatial reference system(srs) code for the output projection. Example: --srs EPSG:4326");

   au->addCommandLineOption("-t or --thumbnail", "<max_dimension>\nSpecify a thumbnail resolution.\nScale will be adjusted so the maximum dimension = argument given.");

   au->addCommandLineOption("--three-band-out", "Force three band output even if input is not. Attempts to map bands to RGB if possible.");

   au->addCommandLineOption("--tile-size", "<size_in_pixels>\nSets the output tile size if supported by writer.  Notes: This sets both dimensions. Must be a multiple of 16, e.g. 1024.");

   au->addCommandLineOption("-u or --up-is-up", "Rotates image to up is up. \"chip\" operation only.");

   au->addCommandLineOption("-w or --writer", "<writer>\nSpecifies the output writer.  Default uses output file extension to determine writer. For valid output writer types use: \"ossim-info --writers\"\n");

   au->addCommandLineOption("--writer-prop", "<writer-property>\nPasses a name=value pair to the writer for setting it's property. Any number of these can appear on the line.");

   au->addCommandLineOption("--zone", "<zone>\nSpecify a projection zone if supported.  E.g. UTM projection. This will lock the zone even if input scene center is in another zone. Valid values for UTM are \"1\" to \"60\"");

   au->addCommandLineOption("--2cmv-old-input-band", "<band>\nBand to use for two color multi view old input.\n");
   au->addCommandLineOption("--2cmv-new-input-band", "<band>\nBand to use for two color multi view new input.\n");

   au->addCommandLineOption("--2cmv-red-output-source", "<source>\nTwo color multi view source input for red output.  Either, old, new, or mp(min pix).  Default=old.\n");

   au->addCommandLineOption("--2cmv-green-output-source", "<source>\nTwo color multi view source input for green output.  Either, old, new, or mp(min pix).  Default=new.\n");

   au->addCommandLineOption("--2cmv-blue-output-source", "<source>\nTwo color multi view source input for blue output.  Either, old, new, or mp(min pix).  Default=new.\n");
   au->addCommandLineOption("--combiner-type", "<type>\nossimBlendMosaic, ossimFeatherMosaic, ossimImageMosaic.  Default: ossimImageMosaic.  Example --combiner-type ossimImageMosaic\n");

} // End: ossimChipperUtil::addArguments

void ossimChipperUtil::clear()
{
   // Must disonnect chains so that they destroy.
   std::vector<ossimRefPtr<ossimSingleImageChain>>::iterator i = m_imgLayer.begin();
   while (i != m_imgLayer.end())
   {
      (*i)->disconnect();
      (*i) = 0;
      ++i;
   }
   m_imgLayer.clear();

   i = m_demLayer.begin();
   while (i != m_demLayer.end())
   {
      (*i)->disconnect();
      (*i) = 0;
      ++i;
   }
   m_demLayer.clear();

   if (m_writer.valid())
   {
      m_writer->disconnect();
      m_writer = 0;
   }
}

bool ossimChipperUtil::initialize(ossimArgumentParser &ap)
{
   static const char MODULE[] = "ossimChipperUtil::initialize(ossimArgumentParser&)";

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   clear();
   if (ap.read("-h") || ap.read("--help") || (ap.argc() == 1))
   {
      usage(ap);

      return false; // Indicates process should be terminated to caller.
   }

   // Start with clean options keyword list.
   m_kwl->clear();

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

   ossim_uint32 demIdx = 0;
   ossim_uint32 imgIdx = 0;
   ossim_uint32 readerPropIdx = 0;
   ossim_uint32 writerPropIdx = 0;
   ossimString key = "";

   // Extract optional arguments and stuff them in a keyword list.
   if (ap.read("--azimuth", stringParam1))
   {
      m_kwl->addPair(std::string(ossimKeywordNames::AZIMUTH_ANGLE_KW), tempString1);
   }

   if (ap.read("-b", stringParam1) || ap.read("--bands", stringParam1))
   {
      m_kwl->addPair(std::string(ossimKeywordNames::BANDS_KW), tempString1);
   }

   if (ap.read("--brightness", stringParam1))
   {
      m_kwl->addPair(BRIGHTNESS_KW, tempString1);
   }

   if (ap.read("--central-meridian", stringParam1))
   {
      m_kwl->addPair(std::string(ossimKeywordNames::CENTRAL_MERIDIAN_KW), tempString1);
   }

   if (ap.read("--color", stringParam1, stringParam2, stringParam3))
   {
      m_kwl->addPair(COLOR_RED_KW, tempString1);
      m_kwl->addPair(COLOR_GREEN_KW, tempString2);
      m_kwl->addPair(COLOR_BLUE_KW, tempString3);
   }

   if (ap.read("--color-table", stringParam1))
   {
      m_kwl->addPair(LUT_FILE_KW, tempString1);
   }

   if (ap.read("--contrast", stringParam1))
   {
      m_kwl->addPair(CONTRAST_KW, tempString1);
   }

   if (ap.read("--cut-width", stringParam1))
   {
      m_kwl->addPair(CUT_WIDTH_KW, tempString1);
   }
   if (ap.read("--cut-height", stringParam1))
   {
      m_kwl->addPair(CUT_HEIGHT_KW, tempString1);
   }
   if (ap.read("--cut-bbox-xywh", stringParam1))
   {
      m_kwl->addPair(CUT_BBOX_XYWH_KW, tempString1);
   }
   if (ap.read("--cut-wms-bbox", stringParam1))
   {
      m_kwl->addPair(CUT_WMS_BBOX_KW, tempString1);
   }
   if (ap.read("--cut-wms-bbox-ll", stringParam1))
   {
      m_kwl->addPair(CUT_WMS_BBOX_LL_KW, tempString1);
   }

   if (ap.read("--clip-wms-bbox-ll", stringParam1))
   {
      m_kwl->addPair(CLIP_WMS_BBOX_LL_KW, tempString1);
   }

   if (ap.read("--clip-poly-lat-lon", stringParam1))
   {
      //std::vector<ossimGpt> result;
      //ossim::toVector(result, ossimString(tempString1));
      //std::cout << result[0] << std::endl;
      //std::cout <<tempString1<<std::endl;
      //exit(0);
      m_kwl->addPair(CLIP_POLY_LAT_LON_KW, tempString1);
   }

   if (ap.read("--cut-bbox-ll", stringParam1, stringParam2, stringParam3, stringParam4))
   {
      m_kwl->addPair(CUT_MIN_LAT_KW, tempString1);
      m_kwl->addPair(CUT_MIN_LON_KW, tempString2);
      m_kwl->addPair(CUT_MAX_LAT_KW, tempString3);
      m_kwl->addPair(CUT_MAX_LON_KW, tempString4);
   }

   if (ap.read("--cut-bbox-llwh", stringParam1, stringParam2, stringParam3,
               stringParam4, stringParam5, stringParam6))
   {
      m_kwl->addPair(CUT_MIN_LAT_KW, tempString1);
      m_kwl->addPair(CUT_MIN_LON_KW, tempString2);
      m_kwl->addPair(CUT_MAX_LAT_KW, tempString3);
      m_kwl->addPair(CUT_MAX_LON_KW, tempString4);
      m_kwl->addPair(CUT_WIDTH_KW, tempString5);
      m_kwl->addPair(CUT_HEIGHT_KW, tempString6);
   }

   if (ap.read("--cut-center-llwh", stringParam1, stringParam2, stringParam3, stringParam4))
   {
      m_kwl->addPair(CUT_CENTER_LAT_KW, tempString1);
      m_kwl->addPair(CUT_CENTER_LON_KW, tempString2);
      m_kwl->addPair(CUT_WIDTH_KW, tempString3);
      m_kwl->addPair(CUT_HEIGHT_KW, tempString4);
   }

   if (ap.read("--cut-center-llr", stringParam1, stringParam2, stringParam3))
   {
      m_kwl->addPair(CUT_CENTER_LAT_KW, tempString1);
      m_kwl->addPair(CUT_CENTER_LON_KW, tempString2);
      m_kwl->addPair(CUT_RADIUS_KW, tempString3);
   }

   int num_params = ap.numberOfParams("--degrees", doubleParam1);
   if (num_params == 1)
   {
      ap.read("--degrees", doubleParam1);
      m_kwl->add(DEGREES_X_KW.c_str(), tempDouble1);
      m_kwl->add(DEGREES_Y_KW.c_str(), tempDouble1);
   }
   else if (num_params == 2)
   {
      ap.read("--degrees", doubleParam1, doubleParam2);
      m_kwl->add(DEGREES_X_KW.c_str(), tempDouble1);
      m_kwl->add(DEGREES_Y_KW.c_str(), tempDouble2);
   }

   if (ap.read("--elevation", stringParam1))
   {
      m_kwl->addPair(std::string(ossimKeywordNames::ELEVATION_ANGLE_KW), tempString1);
   }

   if (ap.read("-e", stringParam1) || ap.read("--entry", stringParam1))
   {
      m_kwl->addPair(std::string(ossimKeywordNames::ENTRY_KW), tempString1);
   }

   if(ap.read("--gamma", stringParam1))
   {
      m_kwl->addPair(GAMMA_KW, tempString1);
   }

   if (ap.read("--exaggeration", stringParam1))
   {
      m_kwl->addPair(GAIN_KW, tempString1);
   }

   if (ap.read("--fullres-xys", stringParam1))
   {
      m_kwl->addPair(FULLRES_XYS_KW, tempString1);

      std::vector<ossimString> values;
      ossimString(tempString1).split(values, ",");
      double sx, sy;
      if (values.size() > 2)
      {
         sx = values[2].toDouble();
         sy = sx;
         if (values.size() > 3)
         {
            sy = values[3].toDouble();
         }
         m_kwl->add(IMAGE_SPACE_SCALE_X_KW.c_str(), sx);
         m_kwl->add(IMAGE_SPACE_SCALE_Y_KW.c_str(), sy);
      }
   }

   if (ap.read("--hemisphere", stringParam1))
   {
      m_kwl->addPair(std::string(ossimKeywordNames::HEMISPHERE_KW), tempString1);
   }

   if (ap.read("--histogram-aoi", stringParam1))
   {
      m_kwl->addPair(HIST_AOI_KW, tempString1);
   }

   if (ap.read("--enable-null-pixel-flip"))
   {
      m_kwl->addPair(NULL_PIXEL_FLIP_KW, TRUE_KW);
   }

   if (ap.read("--histogram-center-tile"))
   {
      m_kwl->addPair(HIST_CENTER_KW, TRUE_KW);
   }

   if (ap.read("--histogram-llwh", stringParam1))
   {
      m_kwl->addPair(HIST_LLWH_KW, tempString1);
   }

   if (ap.read("--histogram-op", stringParam1))
   {
      m_kwl->addPair(HIST_OP_KW, tempString1);
   }

   if (ap.read("--histogram-bias-factor", stringParam1))
   {
      m_kwl->addPair(HIST_BIAS_FACTOR_KW, tempString1);
   }
   if (ap.read("--histogram-linear-clip", stringParam1))
   {
      m_kwl->addPair(HIST_LINEAR_CLIP_KW, tempString1);
   }
   if (ap.read("--histogram-linear-norm-clip", stringParam1))
   {
      m_kwl->addPair(HIST_LINEAR_NORM_CLIP_KW, tempString1);
   }
   if (ap.read("--histogram-center-clip", stringParam1))
   {
      m_kwl->addPair(HIST_CENTER_CLIP_KW, tempString1);
   }
   if (ap.read("--image-space-scale", doubleParam1, doubleParam2))
   {
      m_kwl->add(IMAGE_SPACE_SCALE_X_KW.c_str(), tempDouble1);
      m_kwl->add(IMAGE_SPACE_SCALE_Y_KW.c_str(), tempDouble2);
   }

   while (ap.read("--input-dem", stringParam1))
   {
      key = DEM_KW;
      key += ossimString::toString(demIdx);
      key += ".";
      key += FILE_KW;
      m_kwl->addPair(key.string(), tempString1);
      ++demIdx;
   }

   while (ap.read("--input-img", stringParam1))
   {
      key = IMG_KW;
      key += ossimString::toString(imgIdx);
      key += ".";
      key += FILE_KW;
      m_kwl->addPair(key.string(), tempString1);
      ++imgIdx;
   }

   if (ap.read("--input-src", stringParam1))
   {
      m_kwl->addPair(SRC_FILE_KW, tempString1);
   }

   if (ap.read("--meters", stringParam1))
   {
      m_kwl->addPair(METERS_KW, tempString1);
   }

   if (ap.read("-n") || ap.read("--north-up"))
   {
      m_kwl->addPair(NORTH_UP_KW, TRUE_KW);
   }

   if (ap.read("--op", stringParam1))
   {
      m_kwl->addPair(OP_KW, tempString1);
   }

   //---
   // Deprecated: "--options-keyword-list"
   //---
   if (ap.read("--options", stringParam1))
   {
      ossimFilename optionsKwl = tempString1;
      if (optionsKwl.exists())
      {
         if (m_kwl->addFile(optionsKwl) == false)
         {
            std::string errMsg = "ERROR could not open options keyword list file: ";
            errMsg += optionsKwl.string();
            throw ossimException(errMsg);
         }
      }
      else
      {
         std::string errMsg = "ERROR options keyword list file does not exists: ";
         errMsg += optionsKwl.string();
         throw ossimException(errMsg);
      }
   }

   if (ap.read("--origin-latitude", stringParam1))
   {
      m_kwl->addPair(std::string(ossimKeywordNames::ORIGIN_LATITUDE_KW), tempString1);
   }

   if (ap.read("--output-radiometry", stringParam1))
   {
      m_kwl->addPair(OUTPUT_RADIOMETRY_KW, tempString1);
   }

   if (ap.read("--pad-thumbnail", stringParam1))
   {
      m_kwl->addPair(PAD_THUMBNAIL_KW, tempString1);
   }

   if (ap.read("--projection", stringParam1))
   {
      m_kwl->addPair(std::string(ossimKeywordNames::PROJECTION_KW), tempString1);
   }

   if (ap.read("--resample-filter", stringParam1))
   {
      m_kwl->addPair(RESAMPLER_FILTER_KW, tempString1);
   }

   if (ap.read("-r", stringParam1) || ap.read("--rotate", stringParam1))
   {
      m_kwl->addPair(ROTATION_KW, tempString1);
   }

   if (ap.read("--rotate-to-input"))
   {
      m_kwl->addPair(ROTATE_TO_INPUT, "true");
   }

   while (ap.read("--reader-prop", stringParam1))
   {
      key = READER_PROPERTY_KW;
      key += ossimString::toString(readerPropIdx);
      m_kwl->addPair(key.string(), tempString1);
      ++readerPropIdx;
   }

   if (ap.read("--rrds", stringParam1))
   {
      m_kwl->addPair(RRDS_KW, tempString1);
   }

   if (ap.read("--scale-to-8-bit"))
   {
      m_kwl->addPair(SCALE_2_8_BIT_KW, TRUE_KW);
   }

   if (ap.read("--sharpen-mode", stringParam1))
   {
      m_kwl->addPair(SHARPEN_MODE_KW, tempString1);
   }

   if (ap.read("--sharpen-percent", stringParam1))
   {
      m_kwl->addPair(SHARPEN_PERCENT_KW, tempString1);
   }

   if (ap.read("--snap-tie-to-origin"))
   {
      m_kwl->addPair(SNAP_TIE_TO_ORIGIN_KW, TRUE_KW);
   }

   if (ap.read("--srs", stringParam1))
   {
      ossimString os = tempString1;
      if (os.contains("EPSG:"))
      {
         os.gsub(ossimString("EPSG:"), ossimString(""));
      }
      m_kwl->addPair(SRS_KW, os.string());
   }

   if (ap.read("-t", stringParam1) || ap.read("--thumbnail", stringParam1))
   {
      m_kwl->addPair(THUMBNAIL_RESOLUTION_KW, tempString1);
   }

   if (ap.read("--three-band-out"))
   {
      m_kwl->addPair(THREE_BAND_OUT_KW, TRUE_KW);
   }

   if (ap.read("--tile-size", stringParam1))
   {
      m_kwl->addPair(TILE_SIZE_KW, tempString1);
   }

   if (ap.read("-u") || ap.read("--up-is-up"))
   {
      m_kwl->addPair(UP_IS_UP_KW, TRUE_KW);
   }

   if (ap.read("-w", stringParam1) || ap.read("--writer", stringParam1))
   {
      m_kwl->addPair(WRITER_KW, tempString1);
   }

   while (ap.read("--writer-prop", stringParam1))
   {
      key = WRITER_PROPERTY_KW;
      key += ossimString::toString(writerPropIdx);
      m_kwl->addPair(key.string(), tempString1);
      ++writerPropIdx;
   }

   if (ap.read("--zone", stringParam1))
   {
      m_kwl->addPair(std::string(ossimKeywordNames::ZONE_KW), tempString1);
   }

   if (ap.read("--2cmv-old-input-band", stringParam1))
   {
      m_kwl->addPair(TWOCMV_OLD_INPUT_BAND_KW, tempString1);
   }

   if (ap.read("--2cmv-new-input-band", stringParam1))
   {
      m_kwl->addPair(TWOCMV_NEW_INPUT_BAND_KW, tempString1);
   }
   if (ap.read("--2cmv-red-output-source", stringParam1))
   {
      m_kwl->addPair(TWOCMV_RED_OUTPUT_SOURCE_KW, tempString1);
   }

   if (ap.read("--2cmv-green-output-source", stringParam1))
   {
      m_kwl->addPair(TWOCMV_GREEN_OUTPUT_SOURCE_KW, tempString1);
   }

   if (ap.read("--2cmv-blue-output-source", stringParam1))
   {
      m_kwl->addPair(TWOCMV_BLUE_OUTPUT_SOURCE_KW, tempString1);
   }
   if (ap.read("--combiner-type", stringParam1))
   {
      m_kwl->addPair(COMBINER_TYPE_KW, tempString1);
   }
   // End of arg parsing.
   ap.reportRemainingOptionsAsUnrecognized();
   if (ap.errors())
   {
      ap.writeErrorMessages(ossimNotify(ossimNotifyLevel_NOTICE));
      std::string errMsg = "Unknown option...";
      throw ossimException(errMsg);
   }

   if (ap.argc() >= 2)
   {
      // Output file is last arg:
      m_kwl->add(ossimKeywordNames::OUTPUT_FILE_KW, ap[ap.argc() - 1]);
   }
   else
   {
      if (!m_kwl->find(ossimKeywordNames::OUTPUT_FILE_KW))
      {
         ap.writeErrorMessages(ossimNotify(ossimNotifyLevel_NOTICE));
         std::string errMsg = "Must supply an output file.";
         throw ossimException(errMsg);
      }
   }

   if (ap.argc() > 2) // User passed inputs in front of output file.
   {
      int pos = 1; // ap.argv[0] is application name.
      while (pos < (ap.argc() - 1))
      {
         ossimFilename file = ap[pos];
         if (traceDebug())
         {
            ossimNotify(ossimNotifyLevel_DEBUG)
               << "argv[" << pos << "]: " << file << "\n";
         }

         if (isDemFile(file))
         {
            key = DEM_KW;
            key += ossimString::toString(demIdx);
            key += ".";
            key += FILE_KW;
            m_kwl->addPair(key, file.string());
            ++demIdx;
         }
         else if (isSrcFile(file))
         {
            if (m_kwl->find(SRC_FILE_KW.c_str())) // --input-src used also
            {
               std::string errMsg = MODULE;
               errMsg += "ERROR Multiple src files passed in.  Please combine into one.";
               throw ossimException(errMsg);
            }

            m_kwl->addPair(SRC_FILE_KW, file.string());
         }
         else // Add as an input image.
         {
            key = IMG_KW;
            key += ossimString::toString(imgIdx);
            key += ".";
            key += FILE_KW;
            m_kwl->addPair(key.string(), file.string());
            ++imgIdx;
         }

         ++pos; // Go to next arg...

      } // End: while ( pos < (ap.argc()-1) )

   } // End: if ( ap.argc() > 2 )

   initialize();

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " exited..." << std::endl;
   }
   return true;

} // End: void ossimChipperUtil::initialize(ossimArgumentParser& ap)

void ossimChipperUtil::initialize(const ossimKeywordlist &kwl)
{
   clear();

   // Start with clean options keyword list.
   m_kwl->clear();

   m_kwl->addList(kwl, true);

   initialize();
}

void ossimChipperUtil::initialize()
{
   static const char MODULE[] = "ossimChipperUtil::initialize()";

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   if (traceOptions())
   {
      ossimFilename optionsFile;
      getOutputFilename(optionsFile);
      optionsFile = optionsFile.noExtension();
      optionsFile += "-options.kwl";
      ossimString comment = " Can be use for --options argument.";
      m_kwl->write(optionsFile.c_str(), comment.c_str());
   }

   // Determine the operation to do.
   std::string op = m_kwl->findKey(OP_KW);
   if (op.size())
   {
      ossimString s = op;
      s.downcase();

      if (s == "chip")
      {
         m_operation = OSSIM_CHIPPER_OP_CHIP;
      }
      else if (s == "hillshade")
      {
         m_operation = OSSIM_CHIPPER_OP_HILL_SHADE;
      }

      else if (s == "color-relief")
      {
         m_operation = OSSIM_CHIPPER_OP_COLOR_RELIEF;
      }
      else if (s == "ortho")
      {
         m_operation = OSSIM_CHIPPER_OP_ORTHO;
      }
      else if (s == "psm")
      {
         m_operation = OSSIM_CHIPPER_OP_PSM;
      }
      else if (s == "2cmv")
      {
         m_operation = OSSIM_CHIPPER_OP_2CMV;
      }
      else
      {
         std::string errMsg = "unknown operation: ";
         errMsg += s.string();
         throw ossimException(errMsg);
      }
   }
   else
   {
      std::string errMsg = "keyword not found: ";
      errMsg += OP_KW;
      errMsg += "\nUse --op option to specify operation.\n";
      throw ossimException(errMsg);
   }

   //---
   // Populate the m_srcKwl if --src option was set.
   // Note do this before creating chains.
   //---
   initializeSrcKwl();

   // Check for required inputs. Do this after initializeSrcKwl.
   if (m_operation == OSSIM_CHIPPER_OP_CHIP)
   {
      if (getNumberOfInputs() != 1)
      {
         std::ostringstream errMsg;
         errMsg << op << " operation takes one input.";
         throw ossimException(errMsg.str());
      }
   }

   if ((m_operation == OSSIM_CHIPPER_OP_2CMV) || (m_operation == OSSIM_CHIPPER_OP_PSM))
   {
      if (getNumberOfInputs() != 2)
      {
         std::ostringstream errMsg;
         errMsg << op << " operation requires two inputs.";
         throw ossimException(errMsg.str());
      }
   }

   // Sanity check rotation options.
   if (upIsUp() || northUp() || hasRotation())
   {
      std::string option;
      ossim_uint32 rotationOptionCount = 0;
      if (upIsUp())
      {
         option = UP_IS_UP_KW;
         ++rotationOptionCount;
      }
      if (northUp())
      {
         option = NORTH_UP_KW;
         ++rotationOptionCount;
      }
      if (hasRotation())
      {
         option = ROTATION_KW;
         ++rotationOptionCount;
      }

      // Can only do ONE rotation option.
      if (rotationOptionCount > 1)
      {
         std::ostringstream errMsg;
         if (upIsUp())
         {
            errMsg << UP_IS_UP_KW << " is on.\n";
         }
         if (northUp())
         {
            errMsg << NORTH_UP_KW << " is on.\n";
         }
         if (hasRotation())
         {
            errMsg << ROTATION_KW << " is on.\n";
         }
         errMsg << "Multiple rotation options do not make sense!";
         throw ossimException(errMsg.str());
      }

      // One input, chip operation only.
      if (getNumberOfInputs() != 1)
      {
         std::ostringstream errMsg;
         errMsg << option << " option takes one input.";
         throw ossimException(errMsg.str());
      }

      if (m_operation != OSSIM_CHIPPER_OP_CHIP)
      {
         std::ostringstream errMsg;
         errMsg << option << " option only valid with \"chip\" operation.";
         throw ossimException(errMsg.str());
      }
   }

   // Create chains for any dem sources.
   addDemSources();

   // Create chains for any image sources.
   addImgSources();

   // Initialize projection and propagate to chains.
   initializeOutputProjection();

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "options keyword list:\n"
         << *(m_kwl.get()) << "\n";

      if (m_srcKwl.valid())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "support record keyword list:\n"
            << *(m_srcKwl.get()) << "\n";
      }

      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " exited...\n";
   }

} // End: void ossimChipperUtil::initialize()

ossimRefPtr<ossimImageSource> ossimChipperUtil::initializeChain(ossimIrect &aoi)
{
   static const char MODULE[] = "ossimChipperUtil::initializeChain";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   ossimString lookup; // used throughout...

   ossimRefPtr<ossimImageSource> source = 0;

   if (hasBumpShadeArg())
   {
      // Create chain:
      source = initializeBumpShadeChain();
   }
   else if (m_operation == OSSIM_CHIPPER_OP_COLOR_RELIEF)
   {
      source = initializeColorReliefChain();
   }
   else if ((m_operation == OSSIM_CHIPPER_OP_CHIP) ||
            (m_operation == OSSIM_CHIPPER_OP_ORTHO))
   {
      source = combineLayers();
   }
   else if (m_operation == OSSIM_CHIPPER_OP_2CMV)
   {
      source = initialize2CmvChain(); // Two Color Multiview.
   }
   else if (m_operation == OSSIM_CHIPPER_OP_PSM)
   {
      source = initializePsmChain(); // Pan sharpened multispectral.
   }

   if (source.valid())
   {
      //---
      // This is conditional.  Output radiometry may of may not be set.  This can also be set at
      // the ossimSingleImageChain level.
      //---
      if ((getOutputScalarType() != OSSIM_SCALAR_UNKNOWN) &&
          (source->getOutputScalarType() != getOutputScalarType()))
      {
         // source = addScalarRemapper(source, getOutputScalarType());
      }

      //---
      // Get the area of interest. This will be the scene bounding rect if not
      // explicitly set by user with one of the --cut options.
      //  Need to get this before the thumbnail code.
      //---
      getAreaOfInterest(source.get(), aoi);

      //---
      // Set the image size here.  Note must be set after combineLayers.  This is needed for
      // the ossimImageGeometry::worldToLocal call for a geographic projection to handle wrapping
      // accross the date line.
      //---
      if (m_geom.valid())
      {
         m_geom->setImageSize(aoi.size());
      }

      if (hasThumbnailResolution())
      {
         //---
         // Adjust the projection scale and get the new rect.
         // Note this will resize the ossimImageGeometry::m_imageSize is scale changes.
         //---
         initializeThumbnailProjection(aoi, aoi);

         // Reset the source bounding rect if it changed.
         source->initialize();
      }

      // GP 2019: we will add cutters at the end
      // Will remove temporarily bt leave the code hear just in
      // case I messed something up
      if (source.valid() && !aoi.hasNans())
      {
         //---
         // Add a cut filter. This will:
         // 1) Null out/clip any data pulled in.
         // 2) Speed up by not propagating get tile request outside the cut or "aoi"
         //    to the left hand side(input).
         //---
         // ossimRefPtr<ossimRectangleCutFilter> cutter = new ossimRectangleCutFilter();

         // // Set the cut rectangle:
         // cutter->setRectangle(aoi);

         // // Null outside.
         // cutter->setCutType(ossimRectangleCutFilter::OSSIM_RECTANGLE_NULL_OUTSIDE);

         // // Connect cutter input to source chain.
         // cutter->connectMyInputTo(0, source.get());

         // source = cutter.get();

         // // Dependent on correct aoi so place after the cutter.
         // if (hasAnnotations())
         // {
         //    // Put annotations after scalar remapper.
         //    ossimRefPtr<ossimImageSource> result = addAnnotations(source);
         //    if (result.valid())
         //    {
         //       source = result.get();
         //    }
         // }
      }
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " exited...\n";
   }

   return source;
}

void ossimChipperUtil::setOptionsToChain(
   ossimIrect &aoi, const ossimKeywordlist & /* kwl */)
{
   getAreaOfInterest(m_source.get(), aoi);
}

ossimRefPtr<ossimImageSource> ossimChipperUtil::initializeBumpShadeChain()
{
   static const char MODULE[] = "ossimChipperUtil::initializeBumpShadeChain";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   ossimString lookup;
   ossimRefPtr<ossimImageSource> source = 0;

   // Combine the dems.
   ossimRefPtr<ossimImageSource> demSource = combineLayers(m_demLayer);

   // Set up the normal source.
   ossimRefPtr<ossimImageToPlaneNormalFilter> normSource = new ossimImageToPlaneNormalFilter;

   //---
   // Set the track scale flag to true.  This enables scaling the surface
   // normals by the GSD in order to maintain terrain proportions.
   //---
   normSource->setTrackScaleFlag(true);

   // Connect to dems.
   normSource->connectMyInputTo(demSource.get());

   // Set the smoothness factor.
   ossim_float64 gain = 1.0;
   lookup = m_kwl->findKey(GAIN_KW);
   if (lookup.size())
   {
      gain = lookup.toFloat64();
   }
   normSource->setSmoothnessFactor(gain);

   ossimRefPtr<ossimImageSource> colorSource = 0;
   if (hasLutFile())
   {
      if (m_imgLayer.size())
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << MODULE << " WARNING:"
            << "\nBoth a color table and image(s) have been provided for a color source.\n"
            << "Choosing color table of image(s).\n";
      }

      colorSource = addIndexToRgbLutFilter(demSource);
   }
   else
   {
      // Combine the images and set as color source for bump shade.
      colorSource = combineLayers(m_imgLayer);
   }

   // Create the bump shade.
   ossimRefPtr<ossimBumpShadeTileSource> bumpShade = new ossimBumpShadeTileSource;

   // Set the azimuth angle.
   ossim_float64 azimuthAngle = 180;
   lookup = m_kwl->findKey(ossimKeywordNames::AZIMUTH_ANGLE_KW);
   if (lookup.size())
   {
      ossim_float64 f = lookup.toFloat64();
      if ((f >= 0) && (f <= 360))
      {
         azimuthAngle = f;
      }
   }
   bumpShade->setAzimuthAngle(azimuthAngle);

   // Set the elevation angle.
   ossim_float64 elevationAngle = 45.0;
   lookup = m_kwl->findKey(ossimKeywordNames::ELEVATION_ANGLE_KW);
   if (lookup.size())
   {
      ossim_float64 f = lookup.toFloat64();
      if ((f >= 0.0) && (f <= 90))
      {
         elevationAngle = f;
      }
   }
   bumpShade->setElevationAngle(elevationAngle);

   if (!hasLutFile())
   {
      // Set the color.
      ossim_uint8 r = 0xff;
      ossim_uint8 g = 0xff;
      ossim_uint8 b = 0xff;
      lookup = m_kwl->findKey(COLOR_RED_KW);
      if (lookup.size())
      {
         r = lookup.toUInt8();
      }
      lookup = m_kwl->findKey(COLOR_GREEN_KW);
      if (lookup.size())
      {
         g = lookup.toUInt8();
      }
      lookup = m_kwl->findKey(COLOR_BLUE_KW);
      if (lookup.size())
      {
         b = lookup.toUInt8();
      }
      bumpShade->setRgbColorSource(r, g, b);
   }

   // Connect the two sources.
   bumpShade->connectMyInputTo(0, normSource.get());
   bumpShade->connectMyInputTo(1, colorSource.get());

   if (traceDebug())
   {
      ossim_uint8 r = 0xff;
      ossim_uint8 g = 0xff;
      ossim_uint8 b = 0xff;
      bumpShade->getRgbColorSource(r, g, b);
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "\nazimuthAngle:      " << azimuthAngle
         << "\nelevation angle:   " << elevationAngle
         << "\ngain factor:       " << gain
         << "\nr:                 " << int(r)
         << "\ng:                 " << int(g)
         << "\nb:                 " << int(b)
         << "\n";
   }

   // Capture the pointer to give to the writer.
   source = bumpShade.get();

   return source;

} // End: ossimChipperUtil::initializeBumpShadeChain()

ossimRefPtr<ossimImageSource> ossimChipperUtil::initializeColorReliefChain()
{
   ossimRefPtr<ossimImageSource> result = combineLayers();
   if (hasLutFile())
   {
      result = addIndexToRgbLutFilter(result);
   }
   else
   {
      // No LUT file provided, so doing the default 8-bit linear stretch:
      if (result->getOutputScalarType() != OSSIM_UINT8)
      {
         //result = addScalarRemapper(result, OSSIM_UINT8);
      }
   }
   return result;
}

ossimRefPtr<ossimImageSource> ossimChipperUtil::initializePsmChain()
{
   ossimRefPtr<ossimImageSource> result = 0;

   ossim_uint32 layerCount = (ossim_uint32)(m_demLayer.size() + m_imgLayer.size());

   // Must have two and only two inputs.
   if (layerCount == 2)
   {
      ossimRefPtr<ossimSingleImageChain> input1 = 0; // First input should be color.
      ossimRefPtr<ossimSingleImageChain> input2 = 0; // Second input should be pan.

      // Most likely case, two image layers.
      if (m_imgLayer.size())
      {
         input1 = m_imgLayer[0].get();

         if (m_imgLayer.size() == 2)
         {
            input2 = m_imgLayer[1].get();
         }
      }

      if (m_demLayer.size())
      {
         if (!input1.valid())
         {
            input1 = m_demLayer[0].get();
         }

         if (!input2.valid())
         {
            if (m_demLayer.size() == 1)
            {
               input2 = m_demLayer[0].get();
            }
            else if (m_demLayer.size() == 2)
            {
               input2 = m_demLayer[1].get();
            }
         }
      }

      if (input1.valid() && input2.valid())
      {
         // Make the color input the first connection to the combiner.
         if (input1->getNumberOfOutputBands() == 1)
         {
            // Swap:
            ossimRefPtr<ossimSingleImageChain> tmpChain = input1;
            input1 = input2;
            input2 = tmpChain;
         }

         //---
         // Check the pan source for one band:
         // This really shouldn't happen, i.e. caller should typically pass a
         // one band pan image that is higher resolution than the color source.
         //---
         if (input2->getNumberOfOutputBands() > 1)
         {
            // Note this will add a band selector if there isn't one in chain.
            std::vector<ossim_uint32> bandList(1);
            bandList[0] = 0;
            input2->setBandSelection(bandList);
         }

         // TODO: Make dynamic by type.
         ossimRefPtr<ossimFusionCombiner> psm = new ossimSFIMFusion();
         psm->connectMyInputTo(0, input1.get());
         psm->connectMyInputTo(1, input2.get());
         psm->initialize();
         result = dynamic_cast<ossimImageSource *>(psm.get());
      }

   } // Matches: if ( layerCount == 2 )

   return result;

} // End: ossimChipperUtil::initializePsmChain()

void ossimChipperUtil::initializeOutputProjection()
{
   if (isChipMode())
   {
      createIdentityProjection();
   }
   else
   {
      // Create the output projection.
      createOutputProjection();

      // Setup the view in all the chains.
      propagateOutputProjectionToChains();
   }
}

ossimIrect clampViewportStretch(const ossimIrect &viewRect)
{
   ossimIrect result = viewRect;

   if (result.width() > 1024 || result.height() > 1024)
   {
      ossim_int64 w = ossim::min(static_cast<ossim_int64>(result.width()), static_cast<ossim_int64>(1024));
      ossim_int64 h = ossim::min(static_cast<ossim_int64>(result.height()), static_cast<ossim_int64>(1024));
      if (w < 256)
         w = 256;
      if (h < 256)
         h = 256;

      result = ossimIrect(ossimIpt(result.midPoint()), w, h);
   }
   return result;
}

void ossimChipperUtil::setStretch(ossimRefPtr<ossimHistogramRemapper> remapper)const
{
   ossim_int32 mode = getHistoMode();
   ossimString value;
   if (mode == ossimHistogramRemapper::LINEAR_ONE_PIECE)
   {
      value = m_kwl->findKey(HIST_LINEAR_CLIP_KW);
      std::vector<ossimString> splitValues;
      if (!value.empty())
      {
         splitValues = value.split(",");
         if(splitValues.size() == 2)
         {
            remapper->setLowClipPoint(splitValues[0].toFloat32());
            remapper->setHighClipPoint(splitValues[1].toFloat32());
         }
      }
      else
      {
         value = m_kwl->findKey(HIST_LINEAR_NORM_CLIP_KW);
         if(!value.empty())
         {
            splitValues = value.split(",");
            if (splitValues.size() == 2)
            {
               ossim_float32 low = splitValues[0].toFloat32();
               ossim_float32 high = splitValues[1].toFloat32();
               remapper->setLowNormalizedClipPoint(low);
               remapper->setHighNormalizedClipPoint(high);
            }
         }
      }
   }
   value = m_kwl->findKey(HIST_BIAS_FACTOR_KW);
   if(!value.empty())
   {
      remapper->setBiasFactor(value.toDouble());
   }

   value = m_kwl->findKey(HIST_CENTER_CLIP_KW);
   if (!value.empty())
   {
      remapper->setMidPoint(value.toFloat64());
   }
}

ossimRefPtr<ossimImageSource> ossimChipperUtil::getFinalInput(const ossimIrect& aoi, 
                                                              ossimRefPtr<ossimImageSource> currentSource)
{
   ossimRefPtr<ossimImageSource> result = currentSource;

   if(m_container.valid())
   {
      m_container->deleteAllChildren();
      m_container->disconnect();
   }
   else
   {
      m_container = new ossimConnectableContainer();
   }
   ossimRefPtr<ossimRectangleCutFilter> cut = new ossimRectangleCutFilter();
   cut->connectMyInputTo(result.get());
   cut->setRectangle(aoi);
   m_container->addChild(cut.get());
   result = cut.get();
   if(m_viewPortStretchEnabled)
   {
      if(m_geom)
      {
         ossimDpt iptCenter;
         ossimDrect histogramSourceRect;
         ossimRefPtr<ossimImageHistogramSource>
             histoSource = new ossimImageHistogramSource();
         // ossimTypeNameVisitor imageRendererVisitor("ossimImageRenderer", true);
         // currentSource->accept(imageRendererVisitor);
         ossim_float64 cutWidth  = ossim::clamp<double>(aoi.width(), 256, 1024);
         ossim_float64 cutHeight = ossim::clamp<double>(aoi.height(), 256, 1024);
         ossimRefPtr<ossimRectangleCutFilter> viewportCut = new ossimRectangleCutFilter();
         ossimIrect cutRect = ossimIrect(ossimIpt(aoi.midPoint()), cutWidth, cutHeight);
         m_container->addChild(viewportCut.get());
         viewportCut->connectMyInputTo(currentSource.get());
         viewportCut->setRectangle(cutRect);
         histoSource->connectMyInputTo(viewportCut.get());

         // ossimRefPtr<ossimImageRenderer> rendererObj = imageRendererVisitor.getObjectAs<ossimImageRenderer>(0);
         // imageRendererVisitor.reset();
         // need a rectangle cut that we will connect to an input source that is before the resampling
         // process so we will need to invert the view window center and then expand the sample rect to a
         // fixed size.  Probably like 256x256 for now
         //
 
         // bool setupOk=false;
         // if (rendererObj)
         // {
         //    ossimRefPtr<ossimImageViewTransform> trans = rendererObj->getImageViewTransform();
         //    if(trans)
         //    {
         //       m_container->addChild(viewportCut.get());
         //       trans->viewToImage(aoi.midPoint(), iptCenter);
         //       histogramSourceRect = ossimIrect(ossimIpt(iptCenter), 1024, 1024);
         //       viewportCut->connectMyInputTo(rendererObj->getInput(0));
         //       viewportCut->setRectangle(histogramSourceRect);
         //       histoSource->connectMyInputTo(viewportCut.get());
         //       // ossimKeywordlist  kwl;
         //       // histoSource->getHistogram()->saveState(kwl);
         //       //std::cout << "KWL: " <<  kwl << "\n";
         //       setupOk = true;
         //    }
         // }
         // if(!setupOk)
         // {
         //   histoSource->connectMyInputTo(result.get());
         // }
         ossimRefPtr<ossimHistogramRemapper> remapper = new ossimHistogramRemapper();
         remapper->connectMyInputTo(result.get());
         remapper->connectMyInputTo(histoSource.get());
         remapper->setStretchMode(static_cast<ossimHistogramRemapper::StretchMode>(getHistoMode()));
         remapper->setEnableFlag(true);

         result = remapper.get();
         m_container->addChild(histoSource.get());
         m_container->addChild(remapper.get());
         setStretch(remapper);
      }
   }
   if(scaleToEightBit())
   {
      if(result->getOutputScalarType() != OSSIM_UINT8)
      {
         ossimRefPtr<ossimScalarRemapper>  scalarRemapper = new ossimScalarRemapper();
         scalarRemapper->setOutputScalarType(OSSIM_UINT8);
         scalarRemapper->connectMyInputTo(result.get());
         m_container->addChild(scalarRemapper.get());
         result = scalarRemapper.get();
      }
   }
   return result;
}

void ossimChipperUtil::execute()
{
   static const char MODULE[] = "ossimChipperUtil::execute";

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   ossimIrect aoi;
   ossimRefPtr<ossimImageSource> chain = initializeChain(aoi);
 //  ossimRefPtr<ossimConnectableContainer> container = new ossimConnectableContainer();
   ossimRefPtr<ossimImageSource> source = getFinalInput(aoi, chain);//chain.get();
   if (source.valid() && !aoi.hasNans())
   {
      // Set up the writer.
      m_writer = createNewWriter();

      // Connect the writer to the cutter.
      m_writer->connectMyInputTo(0, source.get());

      //---
      // Set the area of interest.
      // NOTE: This must be called after the writer->connectMyInputTo as
      // ossimImageFileWriter::initialize incorrectly resets theAreaOfInterest
      // back to the bounding rect.
      //---
      // m_writer->setAreaOfInterest(aoi);

      if (m_writer->getErrorStatus() == ossimErrorCodes::OSSIM_OK)
      {
         // Add a listener to get percent complete.
         ossimStdOutProgress prog(0, true);
         m_writer->addListener(&prog);

         if (traceLog())
         {
            ossimKeywordlist logKwl;
            m_writer->saveStateOfAllInputs(logKwl);

            ossimFilename logFile;
            getOutputFilename(logFile);
            logFile.setExtension("log");

            logKwl.write(logFile.c_str());
         }

         // Write the file:
         m_writer->execute();

         m_writer->removeListener(&prog);

         if (m_writer->isAborted())
         {
            throw ossimException("Writer Process aborted!");
         }
         m_writer->disconnect();
         m_container->deleteAllChildren();
         m_container->disconnect();
      }
      else
      {
         throw ossimException("Unable to initialize writer for execution");
      }
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " exited...\n";
   }
}
void ossimChipperUtil::abort()
{
   if (m_writer.valid())
   {
      m_writer->abort();
   }
}

ossimRefPtr<ossimImageData> ossimChipperUtil::getChip(const ossimKeywordlist &optionsKwl)
{
   ossimRefPtr<ossimImageData> result = 0;

   ossimIrect aoi;

   if (optionsKwl.getSize() > 0)
   {
      m_kwl->addList(optionsKwl, true);
   }
   

   // (GP)
   // Until we add more ellaborate code to check for scale changes
   // as well as moving windows we will just always initialize
   // the output projection
   //

   initializeOutputProjection();

   if (!m_source.valid())
   {
      m_source = initializeChain(aoi);
   }

   if (optionsKwl.getSize() > 0)
   {
      
      //---
      // Only do this if new options were passed in. This was causing an off by
      // one error when now options were passed in using thumbnail option.
      // The m_source->getBoundingRect(...) was returning 257 instead of 256.
      // Need to hunt that down but for right now this fixes problem.
      // (drb 20151222)
      //---
      getAreaOfInterest(m_source.get(), aoi);

      m_geom->setImageSize(aoi.size());
   }
   else
   {
      getAreaOfInterest(m_source.get(), aoi);
   }

   if (m_source.valid())
   {
      ossimRefPtr<ossimImageSource> source = getFinalInput(aoi, m_source.get());
      result = source->getTile(aoi);
   }

   return result;
}

void ossimChipperUtil::addDemSources()
{
   static const char MODULE[] = "ossimChipperUtil::addDemSources";

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   std::vector<ossimString> sortedList;
   m_kwl->getSortedList(sortedList, DEM_KW);
   for (auto record : sortedList)
   {
      ossimString key = record + ".";
      ossimString file = key + FILE_KW;
      ossimString entry = key + ossimKeywordNames::ENTRY_KW;
      ossimFilename f = m_kwl->findKey(file.string());
      if (f.size())
      {
         // Look for the entry key, e.g. dem0.entry: 10
         ossim_uint32 entryIndex = 0;
         std::string value = m_kwl->findKey(entry.string());
         if (value.size())
         {
            entryIndex = ossimString(value).toUInt32();
         }
         else
         {
            // Get global entry.  Set by "-e" on command line apps.
            entryIndex = getEntryNumber();
         }
         ossimSrcRecord srcRecord;
         srcRecord.setFilename(f);
         srcRecord.setEntryIndex(entryIndex);
         // addDemSource( f, entryIndex );
         addDemSource(srcRecord);
      }
   }
   sortedList.clear();
   if (m_srcKwl.valid())
   {
      // Add stuff from src keyword list.
      m_srcKwl->getSortedList(sortedList, DEM_KW);
      for (auto record : sortedList)
      {
         ossimString prefix = record + ".";
         ossimSrcRecord src;
         if (src.loadState(*(m_srcKwl.get()), prefix))
         {
            addDemSource(src);
         }
      }
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " exited...\n";
   }
}

void ossimChipperUtil::addDemSource(const ossimFilename &file, ossim_uint32 entryIndex)
{
   static const char MODULE[] = "ossimChipperUtil::addDemSource(const ossimFilename&)";

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   ossimRefPtr<ossimSingleImageChain> ic = createChain(file, entryIndex, true);
   if (ic.valid())
   {
      m_demLayer.push_back(ic);
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " exiting...\n";
   }
}

void ossimChipperUtil::addDemSource(const ossimSrcRecord &rec)
{
   static const char MODULE[] = "ossimChipperUtil::addDemSource(const ossimSrcRecord&)";

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   ossimRefPtr<ossimSingleImageChain> ic = createChain(rec, true);
   if (ic.valid())
   {
      m_demLayer.push_back(ic);
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " exiting...\n";
   }
}

void ossimChipperUtil::addImgSources()
{
   static const char MODULE[] = "ossimChipperUtil::addImgSources";

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }
   std::vector<ossimString> sortedList;
   m_kwl->getSortedList(sortedList, IMG_KW);
   for (auto record : sortedList)
   {
      ossimString fileKey = record + "." + FILE_KW;
      ossimFilename f = m_kwl->findKey(fileKey.string());
      if (!f.empty())
      {
         ossimString imagePrefix = record + ".";
         std::shared_ptr<ossimSrcRecord> srcRecord = std::make_shared<ossimSrcRecord>();
         if (srcRecord->loadState(*m_kwl, imagePrefix.c_str()))
         {
            // Use the global entry only if not set already.
            if ( srcRecord->getEntryIndex() == -1 )
            {
               srcRecord->setEntryIndex( (ossim_int32)getEntryNumber() );
            }
            addImgSource(*srcRecord);
         }
      }
   }
   sortedList.clear();
   if (m_srcKwl.valid())
   {
      m_srcKwl->getSortedList(sortedList, IMG_KW);
      for (auto record : sortedList)
      {
         ossimString prefix = record + ".";
         std::shared_ptr<ossimSrcRecord> src = std::make_shared<ossimSrcRecord>();
         if (src->loadState(*(m_srcKwl.get()), prefix))
         {
            addImgSource(*src);
         }
      }
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " exited...\n";
   }
}

void ossimChipperUtil::addImgSource(const ossimFilename &file, ossim_uint32 entryIndex)
{
   static const char MODULE[] = "ossimChipperUtil::addImgSource";

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " entered...\nFile: " << file << "\n";
   }

   ossimRefPtr<ossimSingleImageChain> ic = createChain(file, entryIndex, false);
   if (ic.valid())
   {
      m_imgLayer.push_back(ic);
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " exiting...\n";
   }
}

void ossimChipperUtil::addImgSource(const ossimSrcRecord &rec)
{
   static const char MODULE[] = "ossimChipperUtil::addImgSource(const ossimSrcRecord&)";

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   ossimRefPtr<ossimSingleImageChain> ic = createChain(rec, false);
   if (ic.valid())
   {
      m_imgLayer.push_back(ic);
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " exiting...\n";
   }
}

ossimRefPtr<ossimSingleImageChain> ossimChipperUtil::createChain(const ossimFilename &file,
                                                                 ossim_uint32 entryIndex,
                                                                 bool isDemSource) const
{
   static const char MODULE[] = "ossimChipperUtil::createChain(const ossimFilename&";

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " entered..."
         << "\nfile: " << file
         << "\nentry: " << entryIndex
         << "\nisDemSource: " << (isDemSource ? "true" : "false")
         << "\n";
   }

   ossimRefPtr<ossimSingleImageChain> ic = 0;

   if (!file.empty() && file.exists())
   {
      ic = new ossimSingleImageChain;
      if (ic->open(file))
      {
         // Set any reader props:
         setReaderProps(ic->getImageHandler().get());

         // we can't guarantee the state of the image handler at this point so
         // let's make sure that the entry is always set to the requested location
         //  On Cib/Cadrg we were having problems.  Removed the compare for entry 0
         //

         if (setChainEntry(ic, entryIndex) == false)
         {
            std::ostringstream errMsg;
            errMsg << MODULE << " ERROR:\nEntry " << entryIndex << " out of range!"
                   << std::endl;
            throw ossimException(errMsg.str());
         }

         //---
         // If PSM (pan sharpening) operation and this input is one band, don't
         // mess with its bands.
         //---
         bool psmPanInput = false;
         if ((m_operation == OSSIM_CHIPPER_OP_PSM) && (ic->getNumberOfOutputBands() == 1))
         {
            psmPanInput = true;
         }

         // Bands selection.  Note: Not performed on PSM pan band.
         if (!psmPanInput)
         {
            if (isThreeBandOut())
            {
               //---
               // This will guarantee three bands out.  Will put band
               // selector at the end of the chain if input is one band. If input image
               // handler has implemented a getRgbBandlist(...) it will also set the
               // rgb band order.
               //---
               ic->setThreeBandFlag(true);
            }

            if (hasBandSelection())
            {
               // User entered band list.
               std::vector<ossim_uint32> bandList(0);
               getBandList(bandList);
               if (bandList.size())
               {
                  ic->setBandSelection(bandList);
               }
               else
               {
                  ic->setDefaultBandSelection();
               }
            }
         }

         ossimString nullPixelFlip = m_kwl->find(NULL_PIXEL_FLIP_KW.c_str());
         if (nullPixelFlip.toBool())
         {
            ic->setAddNullPixelFlipFlag(true);
         }
         //---
         // If multiple inputs and scaleToEightBit do it at the end of the processing
         // chain to alleviate un-even stretches between inputs.
         //
         // GP 2019:  Going to move this to the end of the chain.  Should always be full bit depth until the end
         //---
         // const ossim_uint32 INPUT_COUNT = getNumberOfInputs();
         // bool scaleFlag = (scaleToEightBit() && (INPUT_COUNT == 1));
         //ic->setRemapToEightBitFlag(scaleFlag);

         // Always have resampler cache.
         ic->setAddResamplerCacheFlag(true);

         //---
         // Don't need a chain cache as we're doing a sequential write.  So the same tile
         // should never be visited more than once.
         //---
         ic->setAddChainCacheFlag(false);

         //---
         // Histogram:
         // Don't apply histogram stretch to dem sources for hill shade
         // operation.
         //---
         if ((isDemSource == false) ||
             (isDemSource && (m_operation != OSSIM_CHIPPER_OP_HILL_SHADE)))
         {
            ic->setAddHistogramFlag(hasHistogramOperation());
         }

         if(hasGammaCorrection())
         {
            ic->setAddGammaFlag(true);
         }
         // Brightness, contrast. Note in same filter.
         if (hasBrightnesContrastOperation())
         {
            ic->setBrightnessContrastFlag(true);
         }

         std::string sharpnessMode = getSharpenMode();
         std::string sharpenPercent = getSharpenPercent();
         if (sharpnessMode.size()|| sharpenPercent.size())
         {
            ic->setSharpenFlag(true);
         }

         // Create the chain.
         ic->createRenderedChain();

         if(hasGammaCorrection())
         {
            ic->getGammaRemapper()->setGamma(getGamma());
         }
         // Set the filter type if needed.
         ossimString lookup = m_kwl->findKey(RESAMPLER_FILTER_KW);
         if (lookup.size())
         {
            // Assumption image renderer is in chain:
            ic->getImageRenderer()->getResampler()->setFilterType(lookup);
         }

         // Histogram setup.
         if (hasHistogramOperation())
         {
            setupChainHistogram(ic);
         }

         // Brightness contrast setup:
         if (hasBrightnesContrastOperation())
         {
            // Assumption bright contrast filter in chain:

            ossim_float64 value = getBrightness();
            ic->getBrightnessContrast()->setBrightness(value);

            value = getContrast();
            ic->getBrightnessContrast()->setContrast(value);
         }

         // Sharpness:
         if (sharpnessMode.size())
         {
            if (sharpnessMode == "light")
            {
               ic->getSharpenFilter()->setSharpenPercent(0.2);
               // ic->getSharpenFilter()->setWidthAndSigma(3, 0.5);
            }
            else if (sharpnessMode == "medium")
            {
               ic->getSharpenFilter()->setSharpenPercent(0.5);
            }
            else if (sharpnessMode == "heavy")
            {
               ic->getSharpenFilter()->setSharpenPercent(0.8);
            }
         }
         else if(sharpenPercent.size())
         {
             ic->getSharpenFilter()->setSharpenPercent(ossimString(sharpenPercent).toDouble());
         }

         if (hasGeoPolyCutterOption())
         {
            ossimGeoPolygon polygon;
            getClipPolygon(polygon);
            if (polygon.size() > 0)
            {
               ic->addGeoPolyCutterPolygon(polygon);
            }
         }
      }
      else
      {
         ic = 0;
      }
   }

   if (!ic.valid())
   {
      std::string errMsg = "Could not open: ";
      errMsg += file.string();
      throw ossimException(errMsg);
   }

   if (traceDebug())
   {
      ossimKeywordlist kwl;
      ic->saveState(kwl, 0);

      ossimNotify(ossimNotifyLevel_DEBUG)
         << "chain:\n"
         << kwl << "\n"
         << MODULE << " exiting...\n";
   }

   return ic;
}

ossimRefPtr<ossimSingleImageChain> ossimChipperUtil::createChain(const ossimSrcRecord &rec,
                                                                 bool isDemSource) const
{
   static const char MODULE[] = "ossimChipperUtil::createChain(const ossimSrcRecord&)";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   ossimRefPtr<ossimSingleImageChain> ic = new ossimSingleImageChain;
   if (ic->open(rec))
   {
      // Set any reader props:
      setReaderProps(ic->getImageHandler().get());

      // Entry is set in ossimSingleImageChain::open(src)

      //---
      // If PSM (pan sharpening) operation and this input is one band, don't
      // mess with its bands.
      //---
      bool psmPanInput = false;
      if ((m_operation == OSSIM_CHIPPER_OP_PSM) && (ic->getNumberOfOutputBands() == 1))
      {
         psmPanInput = true;
      }

      // Bands selection.  Note: Not performed on PSM pan band.
      if (!psmPanInput)
      {
         if (isThreeBandOut())
         {
            //---
            // This will guarantee three bands out.  Will put band
            // selector at the end of the chain if input is one band. If input image
            // handler has implemented a getRgbBandlist(...) it will also set the
            // rgb band order.
            //---
            ic->setThreeBandFlag(true);
         }

         if (hasBandSelection())
         {
            // User entered band list.
            std::vector<ossim_uint32> bandList(0);
            getBandList(bandList);
            if (bandList.size())
            {
               ic->setBandSelection(bandList);
            }
            else
            {
               ic->setDefaultBandSelection();
            }
         }
      }
      ossimString nullPixelFlip = m_kwl->find(NULL_PIXEL_FLIP_KW.c_str());
      if (nullPixelFlip.toBool())
      {
         ic->setAddNullPixelFlipFlag(true);
      }

      //---
      // If multiple inputs and scaleToEightBit do it at the end of the processing
      // chain to alleviate un-even stretches between inputs.
      //
      // GP 2019:  Going to move this to the end of the chain.  Should always be full bit depth until the end
      //---
      // const ossim_uint32 INPUT_COUNT = getNumberOfInputs();
      // bool scaleFlag = (scaleToEightBit() && (INPUT_COUNT == 1));
      //ic->setRemapToEightBitFlag(scaleFlag);

      // Always have resampler cache.
      ic->setAddResamplerCacheFlag(true);

      //---
      // Don't need a chain cache as we're doing a sequential write.  So the same tile
      // should never be visited more than once.
      //---
      ic->setAddChainCacheFlag(false);

      //---
      // Histogram:
      // Don't apply histogram stretch to dem sources for hill shade
      // operation.
      //---
      if ((isDemSource == false) ||
          (isDemSource && (m_operation != OSSIM_CHIPPER_OP_HILL_SHADE)))
      {
         ic->setAddHistogramFlag(hasHistogramOperation());
      }

      if(hasGammaCorrection())
      {
         ic->setAddGammaFlag(true);
      }
      // Brightness, contrast. Note in same filter.
      if (hasBrightnesContrastOperation())
      {
         ic->setBrightnessContrastFlag(true);
      }

      std::string sharpnessMode = getSharpenMode();
      std::string sharpenPercent = getSharpenPercent();
      if (sharpnessMode.size() || sharpenPercent.size())
      {
         ic->setSharpenFlag(true);
      }

      //---
      // Create the chain.
      //
      // NOTE: Histogram and band selector can be set in ic->createRenderedChain(rec)
      // if the right keywords are there.
      //---
      ic->createRenderedChain(rec);

      // Set the filter type if needed.
      ossimString lookup = m_kwl->findKey(RESAMPLER_FILTER_KW);
      if (lookup.size())
      {
         // Assumption image renderer is in chain:
         ic->getImageRenderer()->getResampler()->setFilterType(lookup);
      }

      // Histogram setup.
      if (hasHistogramOperation())
      {
         setupChainHistogram(ic, std::make_shared<ossimSrcRecord>(rec));
      }

      if(hasGammaCorrection())
      {
         ic->getGammaRemapper()->setGamma(getGamma());
      }
      // Brightness contrast setup:
      if (hasBrightnesContrastOperation())
      {
         // Assumption bright contrast filter in chain:

         ossim_float64 value = getBrightness();
         ic->getBrightnessContrast()->setBrightness(value);

         value = getContrast();
         ic->getBrightnessContrast()->setContrast(value);
      }

      // Sharpness:
      if (sharpnessMode.size())
      {
         if (sharpnessMode == "light")
         {
            ic->getSharpenFilter()->setSharpenPercent(0.2);
         }
         else if(sharpnessMode == "medium")
         {
            ic->getSharpenFilter()->setSharpenPercent(0.5);
         }
         else if (sharpnessMode == "heavy")
         {
            ic->getSharpenFilter()->setSharpenPercent(0.8);
         }
      }
      else if(sharpenPercent.size())
      {
         ic->getSharpenFilter()->setSharpenPercent(ossimString(sharpenPercent).toDouble());
      }

      if (hasGeoPolyCutterOption())
      {
         ossimGeoPolygon polygon;
         getClipPolygon(polygon);
         if (polygon.size() > 0)
         {
            ic->addGeoPolyCutterPolygon(polygon);
         }
      }
   }
   else // Open failed.
   {
      std::string errMsg = "Could not open from src record!";
      throw ossimException(errMsg);
   }

   if (traceDebug())
   {
      ossimKeywordlist kwl;
      ic->saveState(kwl, 0);

      ossimNotify(ossimNotifyLevel_DEBUG)
         << "chain:\n"
         << kwl << "\n"
         << MODULE << " exiting...\n";
   }

   return ic;
}

void ossimChipperUtil::createOutputProjection()
{
   static const char MODULE[] = "ossimChipperUtil::createOutputProjection";

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   std::string op = m_kwl->findKey(std::string(ossimKeywordNames::PROJECTION_KW));
   std::string srs = m_kwl->findKey(SRS_KW);

   if (op.size() && srs.size())
   {
      ossimNotify(ossimNotifyLevel_WARN)
         << MODULE << " WARNING:"
         << "\nBoth " << SRS_KW << " and " << ossimKeywordNames::PROJECTION_KW
         << " keywords are set!"
         << "\nsrs:               " << srs
         << "\noutput_projection: " << op
         << "\nTaking " << srs << " over " << op << "\n";
   }

   bool usingInput = false;
   ossimChipperOutputProjection projType = getOutputProjectionType();
   ossimRefPtr<ossimMapProjection> proj = 0;

   // If an srs code use that first.
   if (srs.size())
   {
      proj = getNewProjectionFromSrsCode(srs);
   }
   else if (op.size())
   {
      switch (projType)
      {
         case ossimChipperUtil::OSSIM_CHIPPER_PROJ_GEO:
         {
            proj = getNewGeoProjection();
            break;
         }
         case ossimChipperUtil::OSSIM_CHIPPER_PROJ_GEO_SCALED:
         {
            proj = getNewGeoScaledProjection();
            break;
         }
         case ossimChipperUtil::OSSIM_CHIPPER_PROJ_INPUT:
         {
            proj = getFirstInputProjection();
            usingInput = true;
            break;
         }
         case ossimChipperUtil::OSSIM_CHIPPER_PROJ_UTM:
         {
            proj = getNewUtmProjection();
            break;
         }
         default:
         {
            break; // Just for un-handled type warning.
         }
      }
   }


   // Check for identity projection:
   ossimRefPtr<ossimMapProjection> inputProj = getFirstInputProjection();
   if (proj.valid() && inputProj.valid())
   {
      if (!usingInput && (*(inputProj.get()) == *(proj.get())))
      {
         if (projType == OSSIM_CHIPPER_PROJ_GEO_SCALED)
         {
            // Get the origin used for scaling.
            ossimGpt origin = proj->getOrigin();

            // Copy the input projection to our projection.  Has the tie and scale we need.
            proj = inputProj;

            // Set the origin for scaling.
            proj->setOrigin(origin);
         }
         else
         {
            proj = inputProj;
         }
         usingInput = true;
      }
   }
   if (!proj.valid())
   {
      // Try first input. If map projected use that.
      if (inputProj.valid())
      {
         proj = inputProj;
         usingInput = true;
         if (traceDebug())
         {
            ossimNotify(ossimNotifyLevel_WARN)
               << "WARNING: No projection set!"
               << "\nDefaulting to first input's projection.\n";
         }
      }
      else
      {
         proj = getNewGeoScaledProjection();
         if (traceDebug())
         {
            ossimNotify(ossimNotifyLevel_WARN)
               << "WARNING: No projection set!"
               << "\nDefaulting to scaled geographic at scene center.\n";
         }
      }
   }

   // Create our ossimImageGeometry with projection (no transform).
   m_geom = new ossimImageGeometry(0, proj.get());

   //---
   // If the input is the same as output projection do not modify; else, set
   // the gsd to user selected "METERS_KW" or the best resolution of the inputs,
   // set the tie and then snap it to the projection origin.
   //---
   if (!usingInput || hasScaleOption())
   {
      // Set the scale.
      initializeProjectionGsd();
   }

   // Set the tie.
   initializeProjectionTiePoint();

   if (snapTieToOrigin())
   {
      // Adjust the projection tie to the origin.
      proj->snapTiePointToOrigin();
   }

   // May need to rotate the output map if requested:
   bool rotateMap=false;
   m_kwl->getBoolKeywordValue(rotateMap, ROTATE_TO_INPUT.c_str());
   if (rotateMap)
   {
      rotateMapToInput();
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "using input projection: " << (usingInput ? "true" : "false")
         << "\noutput image geometry:\n";

      m_geom->print(ossimNotify(ossimNotifyLevel_DEBUG));

      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " exited...\n";
   }

} // End: ossimChipperUtil::createOutputProjection()

void ossimChipperUtil::rotateMapToInput()
{
   static const char MODULE[] = "ossimChipperUtil::rotateMapToInput";
   if (traceDebug())
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";

   try
   {
      if (!m_geom)
         throw ossimException("Null projection geometry encountered.");
      ossimRefPtr<ossimMapProjection> mapProj =
            dynamic_cast<ossimMapProjection*>(m_geom->getProjection());
      if (!mapProj)
         throw ossimException("Output projection must be a map projection.");
      if (m_imgLayer.size() != 1)
         throw ossimException("Optimal rotation output requested but this feature is not available for mosaics.");
      ossimRefPtr<ossimImageHandler> ih = m_imgLayer[0]->getImageHandler();
      if (!ih)
         throw ossimException("Null image handler encountered.");
      ossimRefPtr<ossimImageGeometry> inputGeom = ih->getImageGeometry();
      if (!inputGeom)
         throw ossimException("Null image geometry encountered.");

      // Determine azimuth of +X axis:
      ossimIpt inputImgSize (inputGeom->getImageSize());
      if (inputImgSize.x == 0)
         throw ossimException("Image size is zero.");

      ossimDpt pUL (0,0);
      ossimDpt pUR (inputImgSize.x-1, 0);
      ossimDpt pLR (inputImgSize.x-1, inputImgSize.y-1);
      ossimDpt pLL (0, inputImgSize.y-1);
      ossimGpt gUL, gUR, gLR, gLL;
      inputGeom->localToWorld(pUL, gUL);
      inputGeom->localToWorld(pUR, gUR);
      inputGeom->localToWorld(pLR, gLR);
      inputGeom->localToWorld(pLL, gLL);

      // The -y axis relative to North is the number we need to rotate, so subtract 90 deg from
      // x-axis orientation.
      double mapRotation = gLL.azimuthTo(gUL);
      cout << "\n Rotating map by "<<mapRotation<<" deg\n"<<endl; //###
      mapProj->applyRotation(mapRotation);

      // Not sure where the UL of the rotated map will be, so assume it will coincide with the image
      // UL for now, then scan for corners outside:
      mapProj->setUlGpt(gUL);
      mapProj->worldToLineSample(gUR, pUR);
      mapProj->worldToLineSample(gLR, pLR);
      mapProj->worldToLineSample(gLL, pLL);
      ossimDrect drect (pUL, pUR, pLR, pLL);

      // A new output image rect has been established, find the corresponding UL ground point
      // and set projection's tiepoint:
      mapProj->lineSampleToWorld(drect.ul(), gUL);
      mapProj->setUlGpt(gUL);
   }
   catch (ossimException& e)
   {
      ossimNotify(ossimNotifyLevel_WARN) << "ossimChipperUtil::rotateMapToInput() -- Exception: "
                                         << e.what() <<" Ignoring request.\n";
   }
   return;
}

void ossimChipperUtil::createIdentityProjection()
{
   static const char MODULE[] = "ossimChipperUtil::createIdentityProjection";

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   // Get the singe image chain.  Sould be only one.
   ossimRefPtr<ossimSingleImageChain> sic = 0;
   if (m_demLayer.size())
   {
      sic = m_demLayer[0];
   }
   else if (m_imgLayer.size())
   {
      sic = m_imgLayer[0];
   }

   if (sic.valid())
   {
      // Get the image handler.
      ossimRefPtr<ossimImageHandler> ih = sic->getImageHandler();

      // Resampler:
      ossimRefPtr<ossimImageRenderer> resampler = sic->getImageRenderer();

      if (ih.valid())
      {
         //---
         // Get the geometry from the image handler.  Since we're in "identity"
         // mode use the inputs for the outputs.
         //---
         m_geom = ih->getImageGeometry();

         if (m_geom.valid())
         {
            ossim_float64 rotation = 0.0;

            if (upIsUp())
            {
               ossimRefPtr<ossimProjection> proj = m_geom->getProjection();
               if (proj.valid())
               {
                  rotation = m_geom->upIsUpAngle();
               }
               else
               {
                  ossimNotify(ossimNotifyLevel_WARN)
                     << "WARNING: No projection to compute the up is up angle!" << std::endl;
               }
            }
            else if (northUp())
            {
               ossimRefPtr<ossimProjection> proj = m_geom->getProjection();
               if (proj.valid())
               {
                  rotation = m_geom->northUpAngle();
               }
               else
               {
                  ossimNotify(ossimNotifyLevel_WARN)
                     << "WARNING: No projection to compute the North up angle!" << std::endl;
               }
            }
            else if (hasRotation())
            {
               rotation = getRotation();
            }

            if (ossim::isnan(rotation))
            {
               rotation = 0.0;
            }

            ossimDpt imageSpaceScale;
            getImageSpaceScale(imageSpaceScale);

            //ossimDrect rect;
            //m_geom->getBoundingRect(rect);
            ossimDpt midPt; // = rect.midPoint();

            getImageSpacePivot(midPt);

            if (traceDebug())
            {
               ossimNotify(ossimNotifyLevel_DEBUG)
                  << MODULE
                  << "\nAffine transform parameters:"
                  << "\nrotation:  " << rotation
                  << "\nmid point: " << midPt << std::endl;
            }

            m_ivt = new ossimImageViewAffineTransform(-rotation,
                                                      imageSpaceScale.x, // image space scale x
                                                      imageSpaceScale.y, // image space scale y
                                                      1.0, 1.0,			 //scale x and y
                                                      0.0, 0.0,			 // translate x,y
                                                      midPt.x * imageSpaceScale.x,
                                                      midPt.y * imageSpaceScale.y); // pivot point

            if (m_kwl->hasKey(METERS_KW) ||
                m_kwl->hasKey(DEGREES_X_KW) ||
                m_kwl->hasKey(RRDS_KW))
            {
               // Set the image view transform scale.
               initializeIvtScale();
            }

            resampler->setImageViewTransform(m_ivt.get());

         } // Matches: if ( m_geom.valid() )

      } // Matches: if ( ih.valid() )

   } // Matches: if ( sic.valid()

} // End: createIdentityProjection()

void ossimChipperUtil::initializeIvtScale()
{
   if (isChipMode() && m_ivt.valid() && m_geom.valid())
   {
      ossimDpt scale;
      scale.makeNan();

      // Check for GSD spec. Degrees/pixel takes priority over meters/pixel:
      ossimString lookup;
      lookup.string() = m_kwl->findKey(DEGREES_X_KW);
      if (lookup.size())
      {
         ossimDpt outputDpp;
         outputDpp.makeNan();

         outputDpp.x = lookup.toFloat64();

         lookup.string() = m_kwl->findKey(DEGREES_Y_KW);
         if (lookup.size())
         {
            outputDpp.y = lookup.toFloat64();
         }

         if (!outputDpp.hasNans())
         {
            // Input degress per pixel.  Consider this a scale of 1.0.
            ossimDpt inputDpp;
            m_geom->getDegreesPerPixel(inputDpp);

            if (!inputDpp.hasNans())
            {
               scale.x = inputDpp.x / outputDpp.x;
               scale.y = inputDpp.y / outputDpp.y;
            }
         }
      }

      if (scale.hasNans())
      {
         lookup = m_kwl->findKey(METERS_KW);
         if (lookup.size())
         {
            ossimDpt outputMpp;
            outputMpp.makeNan();
            outputMpp.x = lookup.toFloat64();
            outputMpp.y = outputMpp.x;

            if (!outputMpp.hasNans())
            {
               // Input meters per pixel.  Consider this a scale of 1.0.
               ossimDpt inputMpp;
               m_geom->getMetersPerPixel(inputMpp);

               if (!inputMpp.hasNans())
               {
                  scale.x = inputMpp.x / outputMpp.x;
                  scale.y = inputMpp.y / outputMpp.y;
               }
            }
         }
      }

      if (scale.hasNans())
      {
         lookup = m_kwl->findKey(RRDS_KW);
         if (lookup.size())
         {
            ossim_float64 d = lookup.toInt32();
            if (d == 0.0)
            {
               scale.x = 1.0;
            }
            else
            {
               scale.x = 1.0 / std::pow(2.0, d);
            }
            scale.y = scale.x;
         }
      }

      if (!scale.hasNans())
      {
         m_ivt->scale(scale.x, scale.y);
      }
      else
      {
         std::string errMsg = "ossimChipperUtil::initializeIvtScale failed!";
         throw ossimException(errMsg);
      }

   } // Matches: if ( isChipMode() && ... )

} // End: ossimChipperUtil::initializeIvtScale()

void ossimChipperUtil::initializeProjectionTiePoint ()
{
   static const char MODULE[] = "ossimChipperUtil::initializeProjectionTiePoint()";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   // Get the map projection from the output geometry:
   ossimRefPtr<ossimMapProjection> mapProj = getMapProjection();

   if (mapProj.valid())
   {
      //---
      // If the output is geographic of there are sensor model inputs, get the tie
      // using the ground point.
      //---
      if (mapProj->isGeographic() || hasSensorModelInput())
      {
         ossimGpt tiePoint;
         tiePoint.makeNan();
         getTiePoint(tiePoint);

         if (!tiePoint.hasNans())
         {
            //---
            // The tie point coordinates currently reflect the UL edge of the UL pixel.
            // We'll need to shift the tie point bac from the edge to the center base on the
            // output gsd.
            //---
            ossimDpt half_pixel_shift = m_geom->getDegreesPerPixel() * 0.5;
            tiePoint.lat -= half_pixel_shift.lat;
            tiePoint.lon += half_pixel_shift.lon;
            mapProj->setUlTiePoints(tiePoint);
         }
         else
         {
            std::string errMsg = MODULE;
            errMsg += " tie point has nans!";
            throw(ossimException(errMsg));
         }

         if (traceDebug())
         {
            ossimNotify(ossimNotifyLevel_DEBUG)
               << "projection tie point: " << tiePoint << "\n"
               << MODULE << " exited...\n";
         }
      }
      else
      {
         //---
         // TODO: Add test for like input projections and use above geographic tie
         // code if not.
         //---
         ossimDpt tiePoint;
         tiePoint.makeNan();
         getTiePoint(tiePoint);

         if (!tiePoint.hasNans())
         {
            //---
            // The tie point coordinates currently reflect the UL edge of the UL pixel.
            // We'll need to shift the tie point bac from the edge to the center base on the
            // output gsd.
            //---
            ossimDpt half_pixel_shift = m_geom->getMetersPerPixel() * 0.5;
            tiePoint.y -= half_pixel_shift.y;
            tiePoint.x += half_pixel_shift.x;
            mapProj->setUlTiePoints(tiePoint);
         }
         else
         {
            std::string errMsg = MODULE;
            errMsg += " tie point has nans!";
            throw(ossimException(errMsg));
         }

         if (traceDebug())
         {
            ossimNotify(ossimNotifyLevel_DEBUG)
               << "projection tie point: " << tiePoint << "\n"
               << MODULE << " exited...\n";
         }
      }

   } // Matches: if ( mapProj.valid() )
   else
   {
      std::string errMsg = MODULE;
      errMsg += "m_projection is null!";
      throw(ossimException(errMsg));
   }
}

void ossimChipperUtil::initializeProjectionGsd()
{
   static const char MODULE[] = "ossimChipperUtil::initializeProjectionGsd()";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }
   ossimRefPtr<ossimMapProjection> mapProj = getMapProjection();
   if (!mapProj.valid())
   {
      std::string errMsg = MODULE;
      errMsg += " projection is null!";
      throw(ossimException(errMsg));
   }

   ossimDpt gsd;
   gsd.makeNan();

   ossimString degreesX;
   ossimString degreesY;
   ossimString meters;
   degreesX.string() = m_kwl->findKey(DEGREES_X_KW);
   degreesY.string() = m_kwl->findKey(DEGREES_Y_KW);
   meters.string() = m_kwl->findKey(METERS_KW);

   if (hasCutBoxWidthHeight())
   {
      // --cut-bbox-llwh Implies a scale...
      if (degreesX.size() || degreesY.size() || meters.size())
      {
         std::ostringstream errMsg;
         errMsg << MODULE << " ERROR: Ambiguous scale keywords!\n"
                << "Do not combine meters or degrees with cut box with a width and height.\n";
         throw(ossimException(errMsg.str()));
      }

      ossimString cutMinLat;
      ossimString cutMinLon;
      ossimString cutMaxLat;
      ossimString cutMaxLon;
      ossimString cutWidth;
      ossimString cutHeight;
      cutMinLat.string() = m_kwl->findKey(CUT_MIN_LAT_KW);
      cutMinLon.string() = m_kwl->findKey(CUT_MIN_LON_KW);
      cutMaxLat.string() = m_kwl->findKey(CUT_MAX_LAT_KW);
      cutMaxLon.string() = m_kwl->findKey(CUT_MAX_LON_KW);
      cutWidth.string() = m_kwl->findKey(CUT_WIDTH_KW);
      cutHeight.string() = m_kwl->findKey(CUT_HEIGHT_KW);
      if (cutMinLat.size() && cutMinLon.size() && cutMaxLat.size() &&
          cutMaxLon.size() && cutWidth.size() && cutHeight.size())
      {
         ossim_float64 minLat = cutMinLat.toFloat64();
         ossim_float64 minLon = cutMinLon.toFloat64();
         ossim_float64 maxLat = cutMaxLat.toFloat64();
         ossim_float64 maxLon = cutMaxLon.toFloat64();
         ossim_float64 width = cutWidth.toFloat64();
         ossim_float64 height = cutHeight.toFloat64();
         if (!ossim::isnan(minLat) && !ossim::isnan(minLon) && !ossim::isnan(maxLat) &&
             !ossim::isnan(maxLon) && !ossim::isnan(width) && !ossim::isnan(height))
         {
            gsd.x = std::fabs(maxLon - minLon) / width;
            gsd.y = std::fabs(maxLat - minLat) / height;

            mapProj->setDecimalDegreesPerPixel(gsd);
         }
      }
   }
   else if (hasWmsBboxCutWidthHeight())
   {
      ossimString cutWidth;
      ossimString cutHeight;
      ossimString cutWmsBbox;

      cutWidth.string() = m_kwl->findKey(CUT_WIDTH_KW);
      cutHeight.string() = m_kwl->findKey(CUT_HEIGHT_KW);
      cutWmsBbox.string() = m_kwl->findKey(CUT_WMS_BBOX_KW);

      cutWmsBbox = cutWmsBbox.upcase().replaceAllThatMatch("BBOX:", "");
      std::vector<ossimString> cutBox = cutWmsBbox.split(",");
      if (cutBox.size() == 4)
      {
         ossim_float64 minx = cutBox[0].toFloat64();
         ossim_float64 miny = cutBox[1].toFloat64();
         ossim_float64 maxx = cutBox[2].toFloat64();
         ossim_float64 maxy = cutBox[3].toFloat64();
         ossim_float64 width = cutWidth.toFloat64();
         ossim_float64 height = cutHeight.toFloat64();
         gsd.x = std::fabs(maxx - minx) / width;
         gsd.y = std::fabs(maxy - miny) / height;

         // bbox is in the units of the projector
         if (mapProj->isGeographic())
         {

            mapProj->setDecimalDegreesPerPixel(gsd);
         }
         else
         {
            mapProj->setMetersPerPixel(gsd);
         }
      }
      else
      {
         std::ostringstream errMsg;
         errMsg << MODULE << " ERROR: cut box does not have 4 values!\n";
         throw(ossimException(errMsg.str()));
      }
   }
   else
   {
      if (meters.size() && (degreesX.size() || degreesY.size()))
      {
         std::ostringstream errMsg;
         errMsg << MODULE << " ERROR: Ambiguous scale keywords!\n"
                << "Do not combine meters with degrees.\n";
         throw(ossimException(errMsg.str()));
      }

      if (degreesX.size())
      {
         // --degrees
         gsd.x = degreesX.toFloat64();

         if (degreesY.size())
         {
            gsd.y = degreesY.toFloat64();
         }
         if (!gsd.hasNans())
         {
            mapProj->setDecimalDegreesPerPixel(gsd);
         }
      }
      else if (meters.size())
      {
         // --meters
         gsd.x = meters.toFloat64();
         gsd.y = gsd.x;
         if (!gsd.hasNans())
         {
            mapProj->setMetersPerPixel(gsd);
         }
      }
   }

   if (gsd.hasNans())
   {
      // Get the best resolution from the inputs.
      getMetersPerPixel(gsd);

      // See if the output projection is geo-scaled; if so, make the pixels square in meters.
      if (getOutputProjectionType() == ossimChipperUtil::OSSIM_CHIPPER_PROJ_GEO_SCALED)
      {
         // Pick the best resolution and make them both the same.
         gsd.x = ossim::min<ossim_float64>(gsd.x, gsd.y);
         gsd.y = gsd.x;
      }

      // Set to input gsd.
      mapProj->setMetersPerPixel(gsd);
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "projection gsd: " << gsd << "\n"
         << MODULE << " exited...\n";
   }
}

void ossimChipperUtil::getTiePoint(ossimGpt &tie)
{
   static const char MODULE[] = "ossimChipperUtil::getTiePoint(ossimGpt&)";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   std::vector<ossimRefPtr<ossimSingleImageChain>>::iterator chainIdx;

   tie.lat = ossim::nan();
   tie.lon = ossim::nan();
   tie.hgt = 0.0;

   // Loop through dem layers.
   ossimGpt chainTiePoint;
   chainIdx = m_demLayer.begin();
   while (chainIdx != m_demLayer.end())
   {
      getTiePoint((*chainIdx).get(), chainTiePoint);
      if (tie.hasNans())
      {
         tie = chainTiePoint;
      }
      else
      {
         if (chainTiePoint.lat > tie.lat)
         {
            tie.lat = chainTiePoint.lat;
         }
         if (chainTiePoint.lon < tie.lon)
         {
            tie.lon = chainTiePoint.lon;
         }
      }
      ++chainIdx;
   }

   // Loop through image layers.
   chainIdx = m_imgLayer.begin();
   while (chainIdx != m_imgLayer.end())
   {
      getTiePoint((*chainIdx).get(), chainTiePoint);
      if (tie.hasNans())
      {
         tie = chainTiePoint;
      }
      else
      {
         if (chainTiePoint.lat > tie.lat)
         {
            tie.lat = chainTiePoint.lat;
         }
         if (chainTiePoint.lon < tie.lon)
         {
            tie.lon = chainTiePoint.lon;
         }
      }
      ++chainIdx;
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "tie point: " << tie << "\n"
         << MODULE << " exited...\n";
   }
}

void ossimChipperUtil::getTiePoint(ossimSingleImageChain *chain, ossimGpt &tie)
{
   static const char MODULE[] = "ossimChipperUtil::getTiePoint(ossimSingleImageChain*,ossimGpt&)";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   if (chain && m_geom.valid())
   {
      //---
      // The view is not set yet in the chain so we get the tie point from the
      // image handler geometry not from the chain which will come from the
      // ossimImageRenderer.
      //---
      ossimRefPtr<ossimImageHandler> ih = chain->getImageHandler();
      if (ih.valid())
      {
         ossimRefPtr<ossimImageGeometry> geom = ih->getImageGeometry();
         if (geom.valid())
         {
            geom->getTiePoint(tie, true);
         }

         // Set height to 0.0 even though it's not used so hasNans test works.
         tie.hgt = 0.0;

         if (tie.hasNans())
         {
            std::string errMsg = MODULE;
            errMsg += "\ngeom->localToWorld returned nan for chain.";
            errMsg += "\nChain: ";
            errMsg += chain->getFilename().string();
            throw ossimException(errMsg);
         }
      }
      else
      {
         std::string errMsg = MODULE;
         errMsg += "\nNo geometry for chain: ";
         errMsg += chain->getFilename().string();
         throw ossimException(errMsg);
      }
   }
   else
   {
      std::string errMsg = MODULE;
      errMsg += " ERROR: Null chain passed to method!";
      throw ossimException(errMsg);
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "chain name: " << chain->getFilename()
         << "\ntie point:  " << tie << "\n"
         << MODULE << " exited...\n";
   }
}

void ossimChipperUtil::getTiePoint(ossimDpt &tie)
{
   static const char MODULE[] = "ossimChipperUtil::getTiePoint(ossimDpt&)";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   std::vector<ossimRefPtr<ossimSingleImageChain>>::iterator chainIdx;

   tie.makeNan();

   // Loop through dem layers.
   ossimDpt chainTiePoint;
   chainIdx = m_demLayer.begin();
   while (chainIdx != m_demLayer.end())
   {
      getTiePoint((*chainIdx).get(), chainTiePoint);
      if (tie.hasNans())
      {
         tie = chainTiePoint;
      }
      else
      {
         if (chainTiePoint.y > tie.y)
         {
            tie.y = chainTiePoint.y;
         }
         if (chainTiePoint.x < tie.x)
         {
            tie.x = chainTiePoint.x;
         }
      }
      ++chainIdx;
   }

   // Loop through image layers.
   chainIdx = m_imgLayer.begin();
   while (chainIdx != m_imgLayer.end())
   {
      getTiePoint((*chainIdx).get(), chainTiePoint);
      if (tie.hasNans())
      {
         tie = chainTiePoint;
      }
      else
      {
         if (chainTiePoint.y > tie.y)
         {
            tie.y = chainTiePoint.y;
         }
         if (chainTiePoint.x < tie.x)
         {
            tie.x = chainTiePoint.x;
         }
      }
      ++chainIdx;
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "tie point: " << tie << "\n"
         << MODULE << " exited...\n";
   }
}

void ossimChipperUtil::getTiePoint(ossimSingleImageChain *chain, ossimDpt &tie)
{
   static const char MODULE[] = "ossimChipperUtil::getTiePoint(ossimSingleImageChain*,ossimDpt&)";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   if (chain && m_geom.valid())
   {
      //---
      // The view is not set yet in the chain so we get the tie point from the
      // image handler geometry not from the chain which will come from the
      // ossimImageRenderer.
      //---
      ossimRefPtr<ossimImageHandler> ih = chain->getImageHandler();
      if (ih.valid())
      {
         ossimRefPtr<ossimImageGeometry> geom = ih->getImageGeometry();
         if (geom.valid())
         {
            geom->getTiePoint(tie, true);
         }

         if (tie.hasNans())
         {
            std::string errMsg = MODULE;
            errMsg += "\ngeom->localToWorld returned nan for chain.";
            errMsg += "\nChain: ";
            errMsg += chain->getFilename().string();
            throw ossimException(errMsg);
         }
      }
      else
      {
         std::string errMsg = MODULE;
         errMsg += "\nNo geometry for chain: ";
         errMsg += chain->getFilename().string();
         throw ossimException(errMsg);
      }
   }
   else
   {
      std::string errMsg = MODULE;
      errMsg += " ERROR: Null chain passed to method!";
      throw ossimException(errMsg);
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "chain name: " << chain->getFilename()
         << "\ntie point:  " << tie << "\n"
         << MODULE << " exited...\n";
   }
}

void ossimChipperUtil::getMetersPerPixel(ossimDpt &gsd)
{
   static const char MODULE[] = "ossimChipperUtil::getMetersPerPixel(ossimDpt&)";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   gsd.makeNan();

   ossimDpt chainGsd;
   std::vector<ossimRefPtr<ossimSingleImageChain>>::iterator chainIdx;

   // Loop through dem layers.
   chainIdx = m_demLayer.begin();
   while (chainIdx != m_demLayer.end())
   {
      getMetersPerPixel((*chainIdx).get(), chainGsd);
      if (gsd.hasNans() || (chainGsd.x < gsd.x))
      {
         gsd = chainGsd;
      }
      ++chainIdx;
   }

   // Loop through image layers.
   chainIdx = m_imgLayer.begin();
   while (chainIdx != m_imgLayer.end())
   {
      getMetersPerPixel((*chainIdx).get(), chainGsd);
      if (gsd.hasNans() || (chainGsd.x < gsd.x))
      {
         gsd = chainGsd;
      }
      ++chainIdx;
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "gsd: " << gsd << "\n"
         << MODULE << " exited...\n";
   }
}

void ossimChipperUtil::getMetersPerPixel(ossimSingleImageChain *chain, ossimDpt &gsd)
{
   static const char MODULE[] = "ossimChipperUtil::getMetersPerPixel(ossimSingleImageChain*,ossimDpt&)";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   if (chain)
   {
      ossimRefPtr<ossimImageGeometry> geom = chain->getImageGeometry();
      if (geom.valid())
      {
         geom->getMetersPerPixel(gsd);
         if (gsd.hasNans())
         {
            std::string errMsg = MODULE;
            errMsg += "\ngeom->getMetersPerPixel returned nan for chain.";
            errMsg += "\nChain: ";
            errMsg += chain->getFilename().string();
            throw ossimException(errMsg);
         }
      }
      else
      {
         std::string errMsg = MODULE;
         errMsg += "\nNo geometry for chain: ";
         errMsg += chain->getFilename().string();
         throw ossimException(errMsg);
      }
   }
   else
   {
      std::string errMsg = MODULE;
      errMsg += " ERROR: Null chain passed to method!";
      throw ossimException(errMsg);
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "chain name: " << chain->getFilename()
         << "\nmeters per pixel: " << gsd << "\n"
         << MODULE << " exited...\n";
   }
}

ossim_float64 ossimChipperUtil::getCentralMeridian() const
{
   ossim_float64 result = ossim::nan();
   ossimString lookup = m_kwl->findKey(std::string(ossimKeywordNames::CENTRAL_MERIDIAN_KW));
   if (lookup.size())
   {
      result = lookup.toFloat64();
      if ((result < -180.0) || (result > 180.0))
      {
         std::string errMsg = "central meridian range error!";
         errMsg += " Valid range: -180 to 180";
         throw ossimException(errMsg);
      }
   }
   return result;
}

ossim_float64 ossimChipperUtil::getOriginLatitude() const
{
   ossim_float64 result = ossim::nan();
   ossimString lookup = m_kwl->find(ossimKeywordNames::ORIGIN_LATITUDE_KW);
   if (lookup.size())
   {
      result = lookup.toFloat64();
      if ((result < -90) || (result > 90.0))
      {
         std::string errMsg = "origin latitude range error!";
         errMsg += " Valid range: -90 to 90";
         throw ossimException(errMsg);
      }
   }
   return result;
}

void ossimChipperUtil::getSceneCenter(ossimGpt &gpt)
{
   static const char MODULE[] = "ossimChipperUtil::getSceneCenter(ossimGpt&)";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   std::vector<ossimGpt> centerGptArray;
   ossimGpt centerGpt;

   std::vector<ossimRefPtr<ossimSingleImageChain>>::iterator chainIdx;

   // Loop through dem layers.
   chainIdx = m_demLayer.begin();
   while (chainIdx != m_demLayer.end())
   {
      getSceneCenter((*chainIdx).get(), centerGpt);
      if (!centerGpt.hasNans())
      {
         centerGptArray.push_back(centerGpt);
      }
      ++chainIdx;
   }

   // Loop through image layers.
   chainIdx = m_imgLayer.begin();
   while (chainIdx != m_imgLayer.end())
   {
      getSceneCenter((*chainIdx).get(), centerGpt);
      if (!centerGpt.hasNans())
      {
         centerGptArray.push_back(centerGpt);
      }
      ++chainIdx;
   }

   ossim_float64 lat = 0.0;
   ossim_float64 lon = 0.0;

   std::vector<ossimGpt>::const_iterator pointIdx = centerGptArray.begin();
   while (pointIdx != centerGptArray.end())
   {
      lat += (*pointIdx).lat;
      lon += (*pointIdx).lon;
      ++pointIdx;
   }

   lat /= centerGptArray.size();
   lon /= centerGptArray.size();

   if ((lat >= -90.0) && (lat <= 90.0) && (lon >= -180.0) && (lon <= 180.0))
   {
      gpt.lat = lat;
      gpt.lon = lon;
   }
   else
   {
      std::ostringstream errMsg;
      errMsg << MODULE << " range error!\nlatitude = "
             << ossimString::toString(lat).string()
             << "\nlongitude = "
             << ossimString::toString(lon).string();
      throw ossimException(errMsg.str());
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "scene center: " << gpt << "\n"
         << MODULE << " exited...\n";
   }
}

void ossimChipperUtil::getSceneCenter(ossimSingleImageChain *chain, ossimGpt &gpt)
{
   static const char MODULE[] =
      "ossimChipperUtil::getSceneCenter(const ossimSingleImageChain*,ossimGpt&)";

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   if (chain)
   {
      ossimRefPtr<ossimImageGeometry> geom = chain->getImageGeometry();
      if (geom.valid())
      {
         ossimIrect boundingRect = chain->getBoundingRect();
         ossimDpt midPoint = boundingRect.midPoint();
         geom->localToWorld(midPoint, gpt);
         gpt.hgt = 0.0;

         if (gpt.hasNans())
         {
            std::string errMsg = MODULE;
            errMsg += "\ngeom->localToWorld returned nan for chain.";
            errMsg += "\nChain: ";
            errMsg += chain->getFilename().string();
            throw ossimException(errMsg);
         }
      }
      else
      {
         std::string errMsg = MODULE;
         errMsg += "\nNo geometry for chain: ";
         errMsg += chain->getFilename().string();
         throw ossimException(errMsg);
      }
   }
   else
   {
      std::string errMsg = MODULE;
      errMsg += " ERROR: Null chain passed to method!";
      throw ossimException(errMsg);
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "chain name: " << chain->getFilename()
         << "\nscene center: " << gpt << "\n"
         << MODULE << " exited...\n";
   }
}

ossimRefPtr<ossimMapProjection> ossimChipperUtil::getFirstInputProjection()
{
   static const char MODULE[] = "ossimChipperUtil::getFirstInputProjection";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   ossimRefPtr<ossimImageHandler> ih = 0;
   ossimRefPtr<ossimMapProjection> result = 0;

   // Get the first image handler.
   if (m_demLayer.size())
   {
      ih = m_demLayer[0]->getImageHandler();
   }
   else if (m_imgLayer.size())
   {
      ih = m_imgLayer[0]->getImageHandler();
   }

   if (ih.valid())
   {
      // Get the geometry from the first image handler.
      ossimRefPtr<ossimImageGeometry> geom = ih->getImageGeometry();
      if (geom.valid())
      {
         // Get the image projection.
         ossimRefPtr<ossimProjection> proj = geom->getProjection();
         if (proj.valid())
         {
            // Cast and assign to result.
            ossimMapProjection *mapProj = dynamic_cast<ossimMapProjection*>(proj.get());
            if (mapProj)
            {
               // Must duplicate in case the output projection gets modified.
               result = (ossimMapProjection *)mapProj->dup();
            }
            if (!result.valid() && traceDebug())
            {
               ossimNotify(ossimNotifyLevel_WARN) << "Could not cast to map projection.\n";
            }
         }
         else if (traceDebug())
         {
            ossimNotify(ossimNotifyLevel_WARN) << "No projection in first chain...\n";
         }
      }
   }
   else if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_WARN) << "No image handler in first chain...\n";
   }

   if (traceDebug())
   {
      if (result.valid())
      {
         result->print(ossimNotify(ossimNotifyLevel_DEBUG));
      }
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " exited...\n";
   }

   return result;
}

ossimRefPtr<ossimMapProjection> ossimChipperUtil::getNewGeoProjection()
{
   return ossimRefPtr<ossimMapProjection>(new ossimEquDistCylProjection());
}

ossimRefPtr<ossimMapProjection> ossimChipperUtil::getNewGeoScaledProjection()
{
   // Make projection:
   ossimRefPtr<ossimMapProjection> result = getNewGeoProjection();

   // Set the origin for scaling:

   // First check for user set "central_meridian" and "origin_latitude":
   ossimGpt origin;
   origin.lat = getOriginLatitude();
   origin.lon = getCentralMeridian();
   origin.hgt = 0.0;

   if (origin.hasNans())
   {
      // Use the scene center from the input.
      getSceneCenter(origin);

      //---
      // Note only latitude used for scaling, origin kept at 0.0.
      // This is a fix/hack for ossimEquDistCylProjection wrapping issues.
      //---
      origin.lon = 0.0;
   }

   if (!origin.hasNans())
   {
      result->setOrigin(origin);
   }
   else
   {
      std::string errMsg = "ossimChipperUtil::getNewGeoScaledProjection ERROR";
      errMsg += "\nOrigin has nans!";
      throw ossimException(errMsg);
   }

   return result;
}

ossimRefPtr<ossimMapProjection> ossimChipperUtil::getNewProjectionFromSrsCode(
   const std::string &code)
{
   ossimRefPtr<ossimMapProjection> result = 0;

   ossimString os = code;
   os.downcase();

   if ((os == "epsg:4326") || (code == "4326"))
   {
      // Avoid factory call for this.
      result = new ossimEquDistCylProjection();
   }
   else
   {
      ossimRefPtr<ossimProjection> proj = ossimProjectionFactoryRegistry::instance()->createProjection(code);
      if (proj.valid())
      {
         result = PTR_CAST(ossimMapProjection, proj.get());
      }
   }
   return result;
}

ossimRefPtr<ossimMapProjection> ossimChipperUtil::getNewUtmProjection()
{
   // Make projection:
   ossimRefPtr<ossimUtmProjection> utm = new ossimUtmProjection;

   // Set the zone from keyword option:
   bool setZone = false;
   ossim_int32 zone = getZone();
   if ((zone > 0) && (zone < 61))
   {
      utm->setZone(zone);
      setZone = true;
   }

   // Set the hemisphere from keyword option:
   bool setHemisphere = false;
   std::string hemisphere = getHemisphere();
   if (hemisphere.size())
   {
      ossimString h(hemisphere);
      h.upcase();
      if ((h == "N") || (h == "NORTH"))
      {
         char c = 'N';
         utm->setHemisphere(c);
         setHemisphere = true;
      }
      if ((h == "S") || (h == "SOUTH"))
      {
         char c = 'S';
         utm->setHemisphere(c);
         setHemisphere = true;
      }
   }

   if (!setZone || !setHemisphere)
   {
      // First check for user set "central_meridian" and "origin_latitude":
      ossimGpt origin;
      origin.lat = getOriginLatitude();
      origin.lon = getCentralMeridian();
      origin.hgt = 0.0;

      if (origin.hasNans())
      {
         // Use the scene center from the input.
         getSceneCenter(origin);
      }

      if (!origin.hasNans())
      {
         if (!setZone)
         {
            utm->setZone(origin);
         }
         if (!setHemisphere)
         {
            utm->setHemisphere(origin);
         }
      }
      else
      {
         std::string errMsg = "ossimChipperUtil::getNewUtmProjection ERROR";
         errMsg += "\nOrigin has nans!";
         throw ossimException(errMsg);
      }
   }

   return ossimRefPtr<ossimMapProjection>(utm.get());
}

ossimRefPtr<ossimMapProjection> ossimChipperUtil::getMapProjection()
{
   ossimRefPtr<ossimMapProjection> mp = 0;
   if (m_geom.valid())
   {
      mp = dynamic_cast<ossimMapProjection *>(m_geom->getProjection());
   }
   return mp;
}

ossimRefPtr<ossimImageFileWriter> ossimChipperUtil::createNewWriter() const
{
   static const char MODULE[] = "ossimChipperUtil::createNewWriter()";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   ossimFilename outputFile;
   getOutputFilename(outputFile);

   if (outputFile == ossimFilename::NIL)
   {
      std::string errMsg = MODULE;
      errMsg += " ERROR no output file name!";
      throw ossimException(errMsg);
   }

   ossimRefPtr<ossimImageFileWriter> writer = 0;

   ossimString lookup = m_kwl->findKey(WRITER_KW);
   if (lookup.size())
   {
      writer = ossimImageWriterFactoryRegistry::instance()->createWriter(lookup);
      if (!writer.valid())
      {
         std::string errMsg = MODULE;
         errMsg += " ERROR creating writer: ";
         errMsg += lookup.string();
         throw ossimException(errMsg);
      }
   }
   else // Create from output file extension.
   {
      writer = ossimImageWriterFactoryRegistry::instance()->createWriterFromExtension(outputFile.ext());

      if (!writer.valid())
      {
         std::string errMsg = MODULE;
         errMsg += " ERROR creating writer from extension: ";
         errMsg += outputFile.ext().string();
         throw ossimException(errMsg);
      }
   }

   // Set the output name.
   writer->setFilename(outputFile);

   // Add any writer props.
   ossim_uint32 count = m_kwl->numberOf(WRITER_PROPERTY_KW.c_str());
   for (ossim_uint32 i = 0; i < count; ++i)
   {
      ossimString key = WRITER_PROPERTY_KW;
      key += ossimString::toString(i);
      lookup = m_kwl->findKey(key.string());
      if (lookup.size())
      {
         std::vector<ossimString> splitArray;
         lookup.split(splitArray, "=");
         if (splitArray.size() == 2)
         {
            ossimRefPtr<ossimProperty> prop =
               new ossimStringProperty(splitArray[0], splitArray[1]);

            if (traceDebug())
            {
               ossimNotify(ossimNotifyLevel_DEBUG)
                  << "Setting writer prop: " << splitArray[0] << "=" << splitArray[1] << "\n";
            }

            writer->setProperty(prop);
         }
      }
   }

   // Output tile size:
   lookup = m_kwl->findKey(TILE_SIZE_KW);
   if (lookup.size())
   {
      ossimIpt tileSize;
      tileSize.x = lookup.toInt32();
      if ((tileSize.x % 16) == 0)
      {
         tileSize.y = tileSize.x;
         writer->setTileSize(tileSize);
      }
      else if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_NOTICE)
            << MODULE << " NOTICE:"
            << "\nTile width must be a multiple of 16! Using default.."
            << std::endl;
      }
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "writer name: " << writer->getClassName() << "\n"
         << MODULE << " exited...\n";
   }

   return writer;
}
ossimIrect viewToImage(ossimRefPtr<ossimImageViewTransform> ivt, const ossimIrect& viewRect)
{
   ossimIrect result;
   ossimDpt ul,ur,lr,ll;

   ivt->viewToImage(viewRect.ul(), ul);
   ivt->viewToImage(viewRect.ur(), ur);
   ivt->viewToImage(viewRect.lr(), lr);
   ivt->viewToImage(viewRect.ll(), ll);

   result = ossimDrect(ul, ur, lr, ll);

   if(result.width() > 1024 || result.height() > 1024)
   {
      ossim_int64 w = ossim::min(static_cast<ossim_int64>(result.width()), static_cast<ossim_int64>(1024));
      ossim_int64 h = ossim::min(static_cast<ossim_int64>(result.height()), static_cast<ossim_int64>(1024));
      if(w < 64) w = 64;
      if(h < 64) h = 64;

      result = ossimIrect(ossimIpt(result.midPoint()), w, h);
   }
   return result;
}

void ossimChipperUtil::propagateViewportStretch(const ossimIrect& rect)
{
   //for now we will do it on each image chaing
   std::vector<ossimRefPtr<ossimSingleImageChain> >::iterator chainIdx;

   // Loop through dem layers.
   chainIdx = m_demLayer.begin();
   while (chainIdx != m_demLayer.end())
   {
      ossimRefPtr<ossimHistogramRemapper> remapper = (*chainIdx)->getHistogramRemapper();
      ossimRefPtr<ossimImageRenderer> resampler = (*chainIdx)->getImageRenderer();
      if (resampler.valid())
      {
         ossimRefPtr<ossimImageViewTransform> ivt = resampler->getImageViewTransform();
         if (ivt.valid() && remapper.valid())
         {
            ossimIrect imageRect = viewToImage(ivt, rect);
            remapper->setEnableFlag(true);
            remapper->computeHistogram(imageRect);
         }
      }
      ++chainIdx;
   }

   // Loop through image layers.
   chainIdx = m_imgLayer.begin();
   while (chainIdx != m_imgLayer.end())
   {
      ossimRefPtr<ossimHistogramRemapper> remapper = (*chainIdx)->getHistogramRemapper();
      ossimRefPtr<ossimImageRenderer>     resampler = (*chainIdx)->getImageRenderer();
      if (resampler.valid())
      {
         ossimRefPtr<ossimImageViewTransform> ivt = resampler->getImageViewTransform();
         if (ivt.valid()&&remapper.valid())
         {
            ossimIrect imageRect = viewToImage(ivt, rect);
            remapper->setEnableFlag(true);
            remapper->computeHistogram(imageRect);
         
         }
      }
      ++chainIdx;
   }
}

void ossimChipperUtil::propagateOutputProjectionToChains()
{
   static const char MODULE[] = "ossimChipperUtil::propagateOutputProjectionToChains()";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   std::vector<ossimRefPtr<ossimSingleImageChain>>::iterator chainIdx;

   // we need to make sure the outputs are refreshed so they can reset themselves
   // Needed when we are doing interactive update to the GSD and clip window
   ossimRefPtr<ossimRefreshEvent> refreshEvent = new ossimRefreshEvent();
   ossimEventVisitor eventVisitor(refreshEvent.get());
   ossimViewInterfaceVisitor viewVisitor(m_geom.get());
   // Loop through dem layers.
   chainIdx = m_demLayer.begin();
   while (chainIdx != m_demLayer.end())
   {
      viewVisitor.reset();
      eventVisitor.reset();
      (*chainIdx)->accept(viewVisitor);
      (*chainIdx)->accept(eventVisitor);

      ossimRefPtr<ossimImageRenderer> resampler = (*chainIdx)->getImageRenderer();
      if (resampler.valid())
      {
         //resampler->setView( m_geom.get() );
         // resampler->propagateEventToOutputs(refreshEvent);
      }
      else
      {
         std::string errMsg = MODULE;
         errMsg += " chain has no resampler!";
         throw(ossimException(errMsg));
      }
      ++chainIdx;
   }

   // Loop through image layers.
   chainIdx = m_imgLayer.begin();
   while (chainIdx != m_imgLayer.end())
   {
      ossimRefPtr<ossimImageRenderer> resampler = (*chainIdx)->getImageRenderer();
      eventVisitor.reset();
      viewVisitor.reset();
      (*chainIdx)->accept(viewVisitor);
      (*chainIdx)->accept(eventVisitor);

      if (resampler.valid())
      {
         //resampler->setView( m_geom.get() );
         //resampler->propagateEventToOutputs(refreshEvent);
      }
      else
      {
         std::string errMsg = MODULE;
         errMsg += " chain has no resampler!";
         throw(ossimException(errMsg));
      }
      ++chainIdx;
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " exited...\n";
   }
}

ossimRefPtr<ossimImageSource> ossimChipperUtil::combineLayers(
   std::vector<ossimRefPtr<ossimSingleImageChain>> &layers) const
{
   static const char MODULE[] = "ossimChipperUtil::combineLayers(layers)";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   ossimRefPtr<ossimImageSource> result = 0;

   if (layers.size() == 1)
   {
      result = layers[0].get();
   }
   else if (layers.size() > 1)
   {

      result = createCombiner(); //new ossimImageMosaic;

      std::vector<ossimRefPtr<ossimSingleImageChain>>::iterator chainIdx = layers.begin();
      while (chainIdx != layers.end())
      {
         result->connectMyInputTo((*chainIdx).get());
         ++chainIdx;
      }
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " exited...\n";
   }

   return result;
}

ossimRefPtr<ossimImageSource> ossimChipperUtil::combineLayers()
{
   static const char MODULE[] = "ossimChipperUtil::combineLayers()";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   ossimRefPtr<ossimImageSource> result = 0;

   ossim_uint32 layerCount = (ossim_uint32)(m_demLayer.size() + m_imgLayer.size());

   if (layerCount)
   {
      if (layerCount == 1)
      {
         if (m_imgLayer.size())
         {
            result = m_imgLayer[0].get();
         }
         else
         {
            result = m_demLayer[0].get();
         }
      }
      else
      {
         result = createCombiner(); //new ossimImageMosaic;

         // Combine the images.  Note we'll put the images on top of the dems.
         if (m_imgLayer.size())
         {
            std::vector<ossimRefPtr<ossimSingleImageChain>>::iterator chainIdx =
               m_imgLayer.begin();
            while (chainIdx != m_imgLayer.end())
            {
               result->connectMyInputTo((*chainIdx).get());
               ++chainIdx;
            }
         }
         if (m_demLayer.size()) // Combine any dem layers.
         {
            std::vector<ossimRefPtr<ossimSingleImageChain>>::iterator chainIdx =
               m_demLayer.begin();
            while (chainIdx != m_demLayer.end())
            {
               result->connectMyInputTo((*chainIdx).get());
               ++chainIdx;
            }
         }
      }
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " exited...\n";
   }

   return result;

} // End: ossimChipperUtil::combineLayers

ossimRefPtr<ossimImageSource> ossimChipperUtil::initialize2CmvChain()
{
   ossimRefPtr<ossimImageSource> result = 0;

   ossim_uint32 layerCount = (ossim_uint32)(m_demLayer.size() + m_imgLayer.size());

   // Must have two and only two inputs.
   if (layerCount == 2)
   {
      ossimRefPtr<ossimConnectableObject> oldImg = 0;
      ossimRefPtr<ossimConnectableObject> newImg = 0;

      //---
      // Expecting two image layers.  We'll code it for flexabilty though...
      // - Take old and new from m_imgLayer if present.
      // - Use m_demLayer only if missing.
      // - Using first image found as old, second new.
      //---

      // Most likely case, two image layers.
      if (m_imgLayer.size())
      {
         oldImg = m_imgLayer[0].get();

         if (m_imgLayer.size() == 2)
         {
            newImg = m_imgLayer[1].get();
         }
      }

      if (m_demLayer.size())
      {
         if (!oldImg.valid())
         {
            oldImg = m_demLayer[0].get();
         }

         if (!newImg.valid())
         {
            if (m_demLayer.size() == 1)
            {
               newImg = m_demLayer[0].get();
            }
            else if (m_demLayer.size() == 2)
            {
               newImg = m_demLayer[1].get();
            }
         }
      }

      if (newImg.valid() && oldImg.valid())
      {
         // Input 0 is old, 1 is new.
         ossimRefPtr<ossimTwoColorView> tcmv = new ossimTwoColorView;
         tcmv->connectMyInputTo(0, oldImg.get());
         tcmv->connectMyInputTo(1, newImg.get());

         // Look for 2cmv options.
         ossim_uint32 oldInputBandIndex = 0;
         ossim_uint32 newInputBandIndex = 0;
         ossimTwoColorView::ossimTwoColorMultiViewOutputSource redOutputSource =
            ossimTwoColorView::OLD;
         ossimTwoColorView::ossimTwoColorMultiViewOutputSource grnOutputSource =
            ossimTwoColorView::NEW;
         ossimTwoColorView::ossimTwoColorMultiViewOutputSource bluOutputSource =
            ossimTwoColorView::NEW;

         ossimString os;
         std::string key = TWOCMV_OLD_INPUT_BAND_KW;
         std::string val = m_kwl->findKey(key);

         if (val.size())
         {
            os = val;
            oldInputBandIndex = os.toUInt32();
         }

         key = TWOCMV_NEW_INPUT_BAND_KW;
         val = m_kwl->findKey(key);
         if (val.size())
         {
            os = val;
            newInputBandIndex = os.toUInt32();
         }

         key = TWOCMV_RED_OUTPUT_SOURCE_KW;
         val = m_kwl->findKey(key);
         if (val.size())
         {
            os = val;
            os.downcase();

            if (os == "new")
            {
               redOutputSource = ossimTwoColorView::NEW;
            }
            else if (os == "MIN")
            {
               redOutputSource = ossimTwoColorView::MIN;
            }
         }

         key = TWOCMV_GREEN_OUTPUT_SOURCE_KW;
         val = m_kwl->findKey(key);
         if (val.size())
         {
            os = val;
            os.downcase();

            if (os == "old")
            {
               grnOutputSource = ossimTwoColorView::OLD;
            }
            else if (os == "MIN")
            {
               grnOutputSource = ossimTwoColorView::MIN;
            }
         }

         key = TWOCMV_BLUE_OUTPUT_SOURCE_KW;
         val = m_kwl->findKey(key);
         if (val.size())
         {
            os = val;
            os.downcase();

            if (os == "old")
            {
               bluOutputSource = ossimTwoColorView::OLD;
            }
            else if (os == "MIN")
            {
               bluOutputSource = ossimTwoColorView::MIN;
            }
         }

         // Set options.
         tcmv->setBandIndexMapping(oldInputBandIndex,
                                   newInputBandIndex,
                                   redOutputSource,
                                   grnOutputSource,
                                   bluOutputSource);
         tcmv->initialize();

         result = tcmv.get();
      }
   }

   return result;

} // ossimChipperUtil::initialize2CmvChain()

ossimRefPtr<ossimImageSource> ossimChipperUtil::addIndexToRgbLutFilter(
   ossimRefPtr<ossimImageSource> &source) const
{
   static const char MODULE[] = "ossimChipperUtil::addIndexToRgbLutFilter(source)";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   ossimRefPtr<ossimImageSource> result = 0;

   if (source.valid())
   {
      ossimRefPtr<ossimIndexToRgbLutFilter> lut = new ossimIndexToRgbLutFilter();
      ossimFilename lutFile;
      lutFile.string() = m_kwl->findKey(LUT_FILE_KW);
      if (lutFile.exists())
      {
         //---
         // Connect to dems:
         // Must do this first so that the min and max get set from the input
         // connection prior to initializing the lut.
         //---
         lut->connectMyInputTo(source.get());

         // Note sure about this.  Make option maybe? (drb)
         lut->setMode(ossimIndexToRgbLutFilter::REGULAR);

         lut->setLut(lutFile);

         // Set as color source for bump shade.
         result = lut.get();
      }
      else
      {
         std::string errMsg = MODULE;
         errMsg += " color table does not exists: ";
         errMsg += lutFile.string();
         throw ossimException(errMsg);
      }
   }
   else
   {
      std::string errMsg = MODULE;
      errMsg += " ERROR: Null source passed to method!";
      throw ossimException(errMsg);
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " exited...\n";
   }

   return result;
}

ossimRefPtr<ossimImageSource> ossimChipperUtil::addScalarRemapper(
   ossimRefPtr<ossimImageSource> &source, ossimScalarType scalar) const
{
   static const char MODULE[] = "ossimChipperUtil::addScalarRemapper(source)";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   ossimRefPtr<ossimImageSource> result = 0;

   if (source.valid())
   {
      if ((scalar != OSSIM_SCALAR_UNKNOWN) && (source->getOutputScalarType() != scalar))
      {
         ossimRefPtr<ossimScalarRemapper> remapper = new ossimScalarRemapper();
         remapper->setOutputScalarType(scalar);
         remapper->connectMyInputTo(source.get());
         result = remapper.get();

         if (traceDebug())
         {
            ossimNotify(ossimNotifyLevel_DEBUG)
               << "\nOutput remapped to: "
               << ossimScalarTypeLut::instance()->getEntryString(scalar) << "\n";
         }
      }
      else
      {
         result = source;
      }
   }
   else
   {
      std::string errMsg = MODULE;
      errMsg += " ERROR: Null source passed to method!";
      throw ossimException(errMsg);
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " exited...\n";
   }

   return result;
}

ossimRefPtr<ossimImageSource> ossimChipperUtil::addAnnotations(
   ossimRefPtr<ossimImageSource> &source) const
{
   static const char MODULE[] = "ossimChipperUtil::addAnnotations(source)";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   ossimRefPtr<ossimImageSource> result = 0;

   if (source.valid() & m_geom.valid())
   {
      ossimString regularExpression = "annotation[0-9]*\\.type";
      const ossim_uint32 COUNT = m_kwl->getNumberOfKeysThatMatch(regularExpression);
      if (COUNT > 0)
      {
         // Create annotator:
         ossimRefPtr<ossimAnnotationSource> annotator = new ossimAnnotationSource();

         // Connect it up:
         annotator->connectMyInputTo(source.get());

         // Loop through keyword list and add all annotation objects.
         ossim_uint32 i = 0;
         ossim_uint32 found = 0;
         std::string prefixBase = "annotation";
         std::string prefix;
         std::string key;
         std::string value;
         while ((found < COUNT) && (i < (COUNT + 100)))
         {
            prefix = prefixBase + ossimString::toString(i).string() + ".";
            key = "type";
            value = m_kwl->findKey(prefix, key);
            if (value.size())
            {
               ++found;

               if (value == "cross_hair")
               {
                  addCrossHairAnnotation(annotator, prefix);
               }
            }
            ++i;
         }

         annotator->initialize();

         // Assign return result:
         result = annotator.get();
      }
   }
   else
   {
      std::string errMsg = MODULE;
      errMsg += " ERROR: Null source passed to method!";
      throw ossimException(errMsg);
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " exited...\n";
   }

   return result;
}

void ossimChipperUtil::addCrossHairAnnotation(
   ossimRefPtr<ossimAnnotationSource> annotator, const std::string &prefix) const
{
   if (annotator.valid() && prefix.size())
   {
      std::string linePrefix = prefix + std::string("line.");
      std::string textPrefix = prefix + std::string("text.");
      ossimDpt centerPt;
      centerPt.makeNan();
      ossim_float64 lineSize = 8.0;
      ossimDpt start;
      ossimDpt end;

      // Cross hair line location:
      std::string key = "location";
      std::string value = m_kwl->findKey(linePrefix, key);
      if (value.size())
      {
         if (value == "scene_center")
         {
            ossimIrect boundingRect = annotator->getBoundingRect(0);
            centerPt = boundingRect.midPoint();
         }
      }
      else
      {
         key = "location.gpt";
         value = m_kwl->findKey(linePrefix, key);
         if (value.size())
         {
            ossimGpt gpt;
            gpt.toPoint(value);
            if (gpt.hasNans() == false)
            {
               // Convert to view coordinate.
               ossimDpt imgPt;
               m_geom->worldToLocal(gpt, imgPt);
               if (m_ivt.valid())
               {
                  m_ivt->imageToView(imgPt, centerPt);
               }
               else
               {
                  centerPt = imgPt;
               }
            }
         }
      }

      if (centerPt.hasNans() == false)
      {
         // Cross hair size:
         key = "size";
         value = m_kwl->findKey(linePrefix, key);
         if (value.size())
         {
            lineSize = ossimString::toInt32(ossimString(value));
         }
         ossim_float64 halfLine = lineSize / 2.0;

         // Create lines.
         ossimRefPtr<ossimAnnotationLineObject> line1 = new ossimAnnotationLineObject();

         // Pick up r,g,b,thickness.
         line1->loadState(*(m_kwl.get()), linePrefix.c_str());

         start.x = centerPt.x;
         start.y = centerPt.y - halfLine;
         end.x = centerPt.x;
         end.y = centerPt.y + halfLine;

         // ossimAnnotationLineObject::setLine recomputes the bounding box.
         line1->setLine(start, end);

         annotator->addObject(line1.get());

         ossimRefPtr<ossimAnnotationLineObject> line2 = new ossimAnnotationLineObject();

         // This will pick up r,g,b,thickness.
         line2->loadState(*(m_kwl.get()), linePrefix.c_str());

         start.x = centerPt.x - halfLine;
         start.y = centerPt.y;
         end.x = centerPt.x + halfLine;
         end.y = centerPt.y;

         // ossimAnnotationLineObject::setLine recomputes the bounding box.
         line2->setLine(start, end);

         annotator->addObject(line2.get());

         // Set text if any:
         key = "string";
         value = m_kwl->findKey(textPrefix, key);
         if (value.size())
         {
            ossimRefPtr<ossimAnnotationFontObject> text = new ossimAnnotationFontObject();
            text->loadState(*(m_kwl.get()), textPrefix.c_str());

            text->setString(ossimString(value));

            // text scale:
            key = "scale";
            value = m_kwl->findKey(textPrefix, key);
            if (value.size())
            {
               ossimDpt scale;
               scale.toPoint(value);
               text->setScale(scale);
            }

            // text point size:
            key = "point_size";
            value = m_kwl->findKey(textPrefix, key);
            if (value.size())
            {
               ossimIpt ptSize;
               ptSize.toPoint(value);
               text->setPointSize(ptSize);
            }

            // text location:
            key = "location";
            value = m_kwl->findKey(textPrefix, key);
            if (value.size())
            {
               if (value == "bottom")
               {
                  ossimIpt ipt;
                  ipt.x = (ossim_int32)centerPt.x;
                  ipt.y = (ossim_int32)(centerPt.y + halfLine + 10);
                  text->setUpperLeftPosition(ipt);

                  ossimDrect rect;
                  text->getBoundingRect(rect);
                  ipt.x = ipt.x - rect.width() / 2;
                  text->setUpperLeftPosition(ipt);
               }
            }

            annotator->addObject(text.get());
         }
      }
   }

} // End: ossimChipperUtil::addCrossHairAnnotations( ... )

bool ossimChipperUtil::setupChainHistogram(ossimRefPtr<ossimSingleImageChain> &chain,
                                           std::shared_ptr<ossimSrcRecord> srcRecordPtr) const
{
   static const char MODULE[] = "ossimChipperUtil::setupChainHistogram(chain)";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   bool result = false;

   ossimHistogramRemapper::StretchMode mode =
      (ossimHistogramRemapper::StretchMode)getHistoMode();

   if (chain.valid())
   {
      ossimRefPtr<ossimImageHandler> ih = chain->getImageHandler();
      ossimRefPtr<ossimHistogramRemapper> remapper = chain->getHistogramRemapper();

      if (ih.valid() && remapper.valid() &&
          (mode != ossimHistogramRemapper::STRETCH_UNKNOWN))
      {
         result = true;
         ossimString histCenterKw = m_kwl->findKey(HIST_CENTER_KW);
         if (histCenterKw.empty())
         {
            histCenterKw = "false";
         }
         bool roiStretch = (m_kwl->hasKey(HIST_AOI_KW) || m_kwl->hasKey(HIST_LLWH_KW) ||
                            histCenterKw.toBool());

         if (!roiStretch)
         {
            bool openedHistogram = false;
            if (remapper->getHistogramFile() == ossimFilename::NIL)
            {
               ossimFilename f;
               if (srcRecordPtr)
               {
                  f = srcRecordPtr->getHistogramPath();
               }
               // Open histogram file.
               if (f.empty() || !f.exists())
               {
                  f = ih->getFilenameWithThisExtension(ossimString("his"));
                  if ( (f.empty() || (f.exists() == false) ) &&
                       (ih->getNumberOfEntries() > 1) && ( ih->getCurrentEntry() == 0 ) )
                  {
                     //---
                     // Check for filename with no entry("e0") in the name if current entry
                     // is 0 and we're multi entry. This handles the case of NITF with
                     // an image and cloud entry, assuming an existing dot histogram
                     // belongs to entry zero. Example:
                     // NITF file:      5V090205P0001912264B220000100282M_001508507.ntf
                     // Histogram file: 5V090205P0001912264B220000100282M_001508507.his
                     //---
                     ih->getFilenameWithThisExt( ossimString(".his"), f );
                     if (!f.exists()) f.clear();
                  }
               }

               if (!f.empty())
               {
                  openedHistogram = remapper->openHistogram(f);
                  if (!openedHistogram && traceDebug())
                  {
                     ossimNotify(ossimNotifyLevel_WARN)
                        << MODULE << " WARNING:"
                        << "\nCould not open:  " << f << "\n";
                  }
               }

               if (!openedHistogram)
               {
                  //---
                  // User requested a histogram operation but does not have
                  // external histogram file. This will cause downstream code
                  // to computer from center tile of image.
                  //---
                  roiStretch = true;
               }
            }
         }

         // Enable and set mode:
         remapper->setEnableFlag(true);
         remapper->setStretchMode(mode);

         if (roiStretch)
         {
            ossimIrect aoi;
            std::string value = m_kwl->findKey(HIST_AOI_KW);
            if (value.size())
            {
               result = getIrect(value, aoi);
            }
            else
            {
               value = m_kwl->findKey(HIST_LLWH_KW);
               if (value.size())
               {
                  result = getIrect(chain, value, aoi);
               }
               else //  Use center of image.
               {
                  // Will need to mark it to do a true computation later
                  // for each chain.  We do not set the request rect yet.
                  //
                  m_viewPortStretchEnabled = true;
                  result = false;//getIrect(chain, aoi);
               }
            }

            if (traceDebug())
            {
               ossimNotify(ossimNotifyLevel_DEBUG)
                  << "ROI of histogram: " << aoi << std::endl;
            }

            if (result)
            {
               ossimIrect newAoi = aoi;   
               ossimDpt centerPoint;

               // we will clamp the region of interest if too large
               if ( aoi.width() > 2048 || aoi.height() > 2048)
               {
                  ossim_int64 w = ossim::min(static_cast<ossim_int64>(aoi.width()), static_cast<ossim_int64>(2048));
                  ossim_int64 h = ossim::min(static_cast<ossim_int64>(aoi.height()), static_cast<ossim_int64>(2048));
                  aoi.getCenter(centerPoint);
                  newAoi = ossimIrect(ossimIpt(centerPoint), w, h);
               }
               //---
               // Note: both of these sections work.
               // Going with separate connection for now. drb - 20 Feb. 2016
               //
               //---
#if 1
               ossimRefPtr<ossimImageHistogramSource> ihist =
                  new ossimImageHistogramSource();
               ihist->setAreaOfInterest(aoi);
               ihist->connectMyInputTo(remapper->getInput()); //ih.get());
               remapper->connectMyInputTo(ihist.get());
#else
               remapper->computeHistogram(newAoi);
#endif
            }
         }

         if (!result)
         {
            remapper->setEnableFlag(false);
         }
         setStretch(remapper);
      } // Matches: if ( ih.valid() && remapper.valid() && mode... )
   } // Matches: if ( chain.valid() )

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " exited...\n";
   }

   return result;
}

bool ossimChipperUtil::setChainEntry(
   ossimRefPtr<ossimSingleImageChain> &chain, ossim_uint32 entryIndex) const
{
   bool result = false;
   if (chain.valid())
   {
      ossimRefPtr<ossimImageHandler> ih = chain->getImageHandler();
      if (ih.valid())
      {
         result = ih->setCurrentEntry(entryIndex);
      }
   }
   return result;
}

void ossimChipperUtil::getOutputFilename(ossimFilename &f) const
{
   f.string() = m_kwl->findKey(std::string(ossimKeywordNames::OUTPUT_FILE_KW));
}

void ossimChipperUtil::getAreaOfInterest(ossimImageSource *source, ossimIrect &rect) const
{
   static const char MODULE[] = "ossimChipperUtil::getAreaOfInterest()";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   // Nan rect for starters.
   rect.makeNan();

   if (source)
   {
      source->initialize(); // Ensure all bounding rectangles are set.

      if (m_kwl->hasKey(CUT_BBOX_XYWH_KW))
      {
         // <x>,<y>,<w>,<h>
         std::string cutBbox = m_kwl->findKey(CUT_BBOX_XYWH_KW);
         getIrect(cutBbox, rect);
      }
      if (rect.hasNans() &&
          m_kwl->hasKey(FULLRES_XYS_KW) &&
          m_kwl->hasKey(CUT_WIDTH_KW) &&
          m_kwl->hasKey(CUT_HEIGHT_KW))
      {
         ossimString tempFullXys = m_kwl->findKey(FULLRES_XYS_KW);
         ossimString tempWidth = m_kwl->findKey(CUT_WIDTH_KW);
         ossimString tempHeight = m_kwl->findKey(CUT_HEIGHT_KW);
         if (m_geom && m_ivt)
         {
            std::vector<ossimString> values;
            tempFullXys.split(values, ",");
            ossimDpt scale;
            ossimDpt location;
            scale.makeNan();
            location.makeNan();
            double w = tempWidth.toDouble();
            double h = tempHeight.toDouble();
            if (values.size() > 2)
            {
               location.x = values[0].toDouble();
               location.y = values[1].toDouble();
               ossimDpt mid;
               m_ivt->imageToView(location, mid);
               ossimIpt ul(ossim::round<int>(mid.x - (w / 2)),
                           ossim::round<int>(mid.y - (h / 2)));
               ossimIpt lr((ul.x + w - 1), ul.y + h - 1);
               rect = ossimIrect(ul, lr);
            }
         }
      }
      if (rect.hasNans())
      {
         if (m_geom.valid())
         {
            if (m_kwl->find(CUT_CENTER_LAT_KW.c_str()))
            {
               // "Cut Center" with: --cut-center-llwh or --cut-center-llr:

               ossimString latStr = m_kwl->findKey(CUT_CENTER_LAT_KW);
               ossimString lonStr = m_kwl->findKey(CUT_CENTER_LON_KW);
               if (latStr.size() && lonStr.size())
               {
                  ossimGpt centerGpt;

                  //---
                  // Want the height nan going into worldToLocal call so it gets picked
                  // up by the elevation manager.
                  //---
                  centerGpt.makeNan();

                  centerGpt.lat = latStr.toFloat64();
                  centerGpt.lon = lonStr.toFloat64();

                  if (!centerGpt.isLatNan() && !centerGpt.isLonNan())
                  {
                     // Ground "cut center" to view:
                     ossimDpt centerDpt(0.0, 0.0);
                     m_geom->worldToLocal(centerGpt, centerDpt);

                     if (!centerDpt.hasNans())
                     {
                        if (isChipMode() && m_ivt.valid()) // Chipping in image space.
                        {
                           // Tranform image center point to view:
                           ossimDpt ipt = centerDpt;
                           m_ivt->imageToView(ipt, centerDpt);
                        }

                        // --cut-center-llwh:
                        ossimString widthStr = m_kwl->findKey(CUT_WIDTH_KW);
                        ossimString heightStr = m_kwl->findKey(CUT_HEIGHT_KW);
                        if (widthStr.size() && heightStr.size())
                        {
                           ossim_int32 width = widthStr.toInt32();
                           ossim_int32 height = heightStr.toInt32();
                           if (width && height)
                           {
                              ossimIpt ul(ossim::round<int>(centerDpt.x - (width / 2)),
                                          ossim::round<int>(centerDpt.y - (height / 2)));
                              ossimIpt lr((ul.x + width - 1), ul.y + height - 1);
                              rect = ossimIrect(ul, lr);
                           }
                        }
                        else // --cut-center-llr:
                        {
                           ossimString radiusStr = m_kwl->findKey(CUT_RADIUS_KW);
                           if (radiusStr.size())
                           {
                              ossim_float64 radius = radiusStr.toFloat64();
                              if (radius)
                              {
                                 ossimDpt mpp;
                                 m_geom->getMetersPerPixel(mpp);

                                 if (!mpp.hasNans())
                                 {
                                    ossim_float64 rx = radius / mpp.x;
                                    ossim_float64 ry = radius / mpp.y;

                                    ossimIpt ul(ossim::round<int>(centerDpt.x - rx),
                                                ossim::round<int>(centerDpt.y - ry));
                                    ossimIpt lr(ossim::round<int>(centerDpt.x + rx),
                                                ossim::round<int>(centerDpt.y + ry));
                                    rect = ossimIrect(ul, lr);
                                 }
                              }
                           }
                        }
                     }

                  } // Matches: if ( !centerGpt.hasNans() )

               } // Matches: if ( latStr && lonStr )

            } // Matches: if ( m_kwl->find( CUT_CENTER_LAT_KW ) )

            else if (m_kwl->find(CUT_MAX_LAT_KW.c_str()) ||
                     m_kwl->find(CUT_WMS_BBOX_LL_KW.c_str()))
            {
               ossimString maxLat;
               ossimString maxLon;
               ossimString minLat;
               ossimString minLon;

               // --cut-bbox-ll or --cut-bbox-llwh
               if (m_kwl->find(CUT_MAX_LAT_KW.c_str()))
               {
                  maxLat = m_kwl->findKey(CUT_MAX_LAT_KW);
                  maxLon = m_kwl->findKey(CUT_MAX_LON_KW);
                  minLat = m_kwl->findKey(CUT_MIN_LAT_KW);
                  minLon = m_kwl->findKey(CUT_MIN_LON_KW);
               }
               else
               {
                  ossimString cutBbox = m_kwl->findKey(CUT_WMS_BBOX_LL_KW);

                  cutBbox = cutBbox.upcase().replaceAllThatMatch("BBOX:");
                  std::vector<ossimString> cutBox = cutBbox.split(",");
                  if (cutBox.size() > 3)
                  {
                     minLon = cutBox[0];
                     minLat = cutBox[1];
                     maxLon = cutBox[2];
                     maxLat = cutBox[3];
                  }
               }

               if (maxLat.size() && maxLon.size() && minLat.size() && minLon.size())
               {
                  ossim_float64 minLatF = minLat.toFloat64();
                  ossim_float64 maxLatF = maxLat.toFloat64();
                  ossim_float64 minLonF = minLon.toFloat64();
                  ossim_float64 maxLonF = maxLon.toFloat64();

                  //---
                  // Check for swap so we don't get a negative height.
                  // Note no swap check for longitude as box could cross date line.
                  //---
                  if (minLatF > maxLatF)
                  {
                     ossim_float64 tmpF = minLatF;
                     minLatF = maxLatF;
                     maxLatF = tmpF;
                  }

                  //---
                  // Assume cut box is edge to edge or "Pixel Is Area". Our
                  // AOI(area of interest) uses center of pixel or "Pixel Is Point"
                  // so get the degrees per pixel and shift AOI to center.
                  //---
                  ossimDpt halfDpp;
                  m_geom->getDegreesPerPixel(halfDpp);
                  halfDpp = halfDpp / 2.0;

                  ossimGpt gpt(0.0, 0.0, 0.0);
                  ossimDpt ulPt;
                  ossimDpt lrPt;

                  // Upper left:
                  gpt.lat = maxLatF - halfDpp.y;
                  gpt.lon = minLonF + halfDpp.x;
                  m_geom->worldToLocal(gpt, ulPt);

                  // Lower right:
                  gpt.lat = minLatF + halfDpp.y;
                  gpt.lon = maxLonF - halfDpp.x;
                  m_geom->worldToLocal(gpt, lrPt);

                  if (isChipMode() && m_ivt.valid())
                  {
                     // Chipping in image space:

                     // Tranform image ul point to view:
                     ossimDpt ipt = ulPt;
                     m_ivt->imageToView(ipt, ulPt);

                     // Tranform image lr point to view:
                     ipt = lrPt;
                     m_ivt->imageToView(ipt, lrPt);
                  }

                  rect = ossimIrect(ossimIpt(ulPt), ossimIpt(lrPt));
               }
            }
            else if (m_kwl->find(CUT_WMS_BBOX_KW.c_str()))
            {
               ossimString cutBbox = m_kwl->findKey(CUT_WMS_BBOX_KW);

               cutBbox = cutBbox.upcase().replaceAllThatMatch("BBOX:");
               std::vector<ossimString> cutBox = cutBbox.split(",");
               if (cutBox.size() == 4)
               {

                  ossim_float64 minx = cutBox[0].toFloat64();
                  ossim_float64 miny = cutBox[1].toFloat64();
                  ossim_float64 maxx = cutBox[2].toFloat64();
                  ossim_float64 maxy = cutBox[3].toFloat64();

                  const ossimMapProjection *mapProj = m_geom->getAsMapProjection();
                  if (mapProj)
                  {
                     std::vector<ossimDpt> pts(4);
                     ossimDpt *ptsArray = &pts.front();
                     if (mapProj->isGeographic())
                     {
                        ossimDpt halfDpp;
                        m_geom->getDegreesPerPixel(halfDpp);
                        halfDpp = halfDpp / 2.0;
                        ossimGpt gpt(0.0, 0.0, 0.0);
                        ossimDpt ulPt;
                        ossimDpt lrPt;

                        // Upper left:
                        gpt.lat = maxy - halfDpp.y;
                        gpt.lon = minx + halfDpp.x;
                        m_geom->worldToLocal(gpt, ptsArray[0]);
                        // Upper right:
                        gpt.lat = maxy - halfDpp.y;
                        gpt.lon = maxx - halfDpp.x;
                        m_geom->worldToLocal(gpt, ptsArray[1]);

                        // Lower right:
                        gpt.lat = miny + halfDpp.y;
                        gpt.lon = maxx - halfDpp.x;
                        m_geom->worldToLocal(gpt, ptsArray[2]);

                        //Lower left
                        gpt.lat = miny + halfDpp.y;
                        gpt.lon = minx + halfDpp.x;
                        m_geom->worldToLocal(gpt, ptsArray[3]);
                     }
                     else
                     {
                        ossimDpt halfMpp;
                        ossimDpt eastingNorthing;
                        m_geom->getMetersPerPixel(halfMpp);
                        halfMpp = halfMpp / 2.0;

                        eastingNorthing.x = minx + halfMpp.x;
                        eastingNorthing.y = miny + halfMpp.y;
                        mapProj->eastingNorthingToLineSample(eastingNorthing, ptsArray[0]);
                        eastingNorthing.x = minx + halfMpp.x;
                        eastingNorthing.y = maxy - halfMpp.y;
                        mapProj->eastingNorthingToLineSample(eastingNorthing, ptsArray[1]);
                        eastingNorthing.x = maxx - halfMpp.x;
                        eastingNorthing.y = maxy - halfMpp.y;
                        mapProj->eastingNorthingToLineSample(eastingNorthing, ptsArray[2]);
                        eastingNorthing.x = maxx - halfMpp.x;
                        eastingNorthing.y = miny + halfMpp.y;
                        mapProj->eastingNorthingToLineSample(eastingNorthing, ptsArray[3]);
                     }
                     rect = ossimIrect(pts);
                  }
               }
            }

         } // if ( m_getOutputGeometry.valid() )
         else
         {
            // Should never happer...
            std::string errMsg = MODULE;
            if (!source)
            {
               errMsg += " image source null!";
               throw(ossimException(errMsg));
            }
            // do not have to have a projection to output in image space
            // So commenting out here
            //
            // else
            // {
            //    errMsg += " output projection null!";
            // }
         }

         // If no user defined rect set to scene bounding rect.
         if (rect.hasNans())
         {
            // Get the rectangle from the input chain:
            rect = source->getBoundingRect(0);
         }

      } // if ( rect.hasNans() )

   } // if ( source )

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "aoi: " << rect << "\n"
         << MODULE << " exited...\n";
   }
} // End: ossimChipperUtil::getAreaOfInterest

bool ossimChipperUtil::getIrect(const std::string &s, ossimIrect &rect) const
{
   bool result = false;
   if (s.size())
   {
      // <x>,<y>,<w>,<h>
      ossimString cutBbox = s;
      std::vector<ossimString> keys;
      cutBbox.split(keys, ",");
      if (keys.size() > 3)
      {
         ossimIpt ul;
         ossimIpt lr;
         ul.x = keys[0].toInt32();
         ul.y = keys[1].toInt32();
         lr.x = ul.x + keys[2].toInt32() - 1;
         lr.y = ul.y + keys[3].toInt32() - 1;
         rect = ossimIrect(ul, lr);
         result = true;
      }
   }
   return result;
}

bool ossimChipperUtil::getIrect(ossimRefPtr<ossimSingleImageChain> &chain,
                                const std::string &s,
                                ossimIrect &rect) const
{
   bool result = false;

   if (chain.valid())
   {
      ossimRefPtr<ossimImageHandler> ih = chain->getImageHandler();
      if (ih.valid())
      {
         // Get the geometry from the image handler.
         ossimRefPtr<ossimImageGeometry> geom = ih->getImageGeometry();
         if (geom.valid())
         {
            // <lat>,<lon>,<w>,<h>
            ossimString cutBbox = s;
            std::vector<ossimString> keys;
            cutBbox.split(keys, ",");
            if (keys.size() > 3)
            {
               ossimGpt gpt;
               gpt.lat = keys[0].toFloat64();
               gpt.lon = keys[1].toFloat64();
               ossim_int32 w = keys[2].toInt32();
               ossim_int32 h = keys[3].toInt32();
               ossimDpt dpt;
               geom->worldToLocal(gpt, dpt);
               ossimIpt ul = dpt;
               ul.x -= w / 2;
               ul.y -= h / 2;
               ossimIpt lr(ul.x + w - 1, ul.y + h - 1);
               rect = ossimIrect(ul, lr);
               result = true;
            }
         }
      }
   }

   return result;
}

bool ossimChipperUtil::getIrect(ossimRefPtr<ossimSingleImageChain> &chain,
                                ossimIrect &rect) const
{
   bool result = false;
   if (chain.valid())
   {
      
      ossimRefPtr<ossimImageHandler> ih = chain->getImageHandler();
      if (ih.valid())
      {
         const ossim_int32 MAX = 512;
         ossimIrect r = ih->getImageRectangle();
         ossimIpt size = r.size();
         ossim_int32 w = ossim::min(MAX, size.x);
         ossim_int32 h = ossim::min(MAX, size.y);
         ossimIpt ul = r.midPoint();
         ul.x -= w / 2;
         ul.y -= h / 2;
         ossimIpt lr(ul.x + w - 1, ul.y + h - 1);
         rect = ossimIrect(ul, lr);
         result = true;
      }
   }
   return result;
}

void ossimChipperUtil::initializeThumbnailProjection(const ossimIrect &originalRect,
                                                     ossimIrect &adjustedRect)
{
   static const char MODULE[] = "ossimChipperUtil::initializeThumbnailProjection";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " entered...\n"
         << "origial rect:  " << originalRect << "\n";

      if (m_geom.valid())
      {
         m_geom->print(ossimNotify(ossimNotifyLevel_DEBUG));
      }
   }

   if (!originalRect.hasNans() && m_geom.valid())
   {
      //---
      // Thumbnail setup:
      //---
      ossimString thumbRes = m_kwl->findKey(THUMBNAIL_RESOLUTION_KW);
      if (thumbRes.size())
      {
         ossim_float64 thumbSize = thumbRes.toFloat64();
         ossim_float64 maxRectDimension =
            ossim::max(originalRect.width(), originalRect.height());
         ossim_uint32 tw = originalRect.width();
         ossim_uint32 th = originalRect.height();
         ossim_float64 scale = 1.0;
         if (maxRectDimension > thumbSize)
         {
            // Need to adjust scale:

            // Get the corners before the scale change:
            ossimGpt ulGpt;
            ossimGpt lrGpt;

            m_geom->localToWorld(ossimDpt(originalRect.ul()), ulGpt);
            m_geom->localToWorld(ossimDpt(originalRect.lr()), lrGpt);

            if (isChipMode() && m_ivt.valid()) // Chipping in image space.)
            {
               scale = thumbSize / maxRectDimension;
               if (m_ivt->getScale().hasNans())
               {
                  m_ivt->scale(scale, scale);
               }
               else
               {
                  m_ivt->scale(m_ivt->getScale().x * scale, m_ivt->getScale().y * scale);
               }
               tw *= scale;
               th *= scale;
            }
            else
            {
               scale = maxRectDimension / thumbSize;
               //---
               // Adjust the projection scale.  Note the "true" is to recenter
               // the tie point so it falls relative to the projection origin.
               //
               // This call also scales: ossimImageGeometry::m_imageSize
               //---
               m_geom->applyScale(ossimDpt(scale, scale), true);
               tw /= scale;
               th /= scale;
            }
            if (tw < 1)
               tw = 1;
            if (th < 1)
               th = 1;
            // Must call to reset the ossimImageRenderer's bounding rect for each input.
            propagateOutputProjectionToChains();

            // Get the new upper left in view space.
            ossimDpt dpt;
            m_geom->worldToLocal(ulGpt, dpt);
            ossimIpt ul(dpt);

            // Get the new lower right in view space.
            m_geom->worldToLocal(lrGpt, dpt);
            ossimIpt lr(dpt);

            //---
            // Clamp to thumbnail bounds with padding if turned on.
            // Padding is optional. If padding turned on alway make square.
            //---
            ossim_int32 ts = thumbSize;
            bool pad = padThumbnail();
            if (pad)
            {
               lr.x = ul.x + ts - 1;
               lr.y = ul.y + ts - 1;
            }
            else // let it vary
            {
               lr.x = ul.x + tw - 1;
               lr.y = ul.y + th - 1;
            }

            adjustedRect = ossimIrect(ul, lr);
         }
      }

   } // if ( !originalRect.hasNans() && m_geom.valid() )
   else
   {
      // Should never happer...
      std::string errMsg = MODULE;
      if (originalRect.hasNans())
      {
         errMsg += " passed in rect has nans!";
      }
      else
      {
         errMsg += " output projection null!";
      }
      throw(ossimException(errMsg));
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << "\nadjusted rect: " << adjustedRect << "\n";
      if (m_geom.valid())
      {
         m_geom->print(ossimNotify(ossimNotifyLevel_DEBUG));
      }
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " exited...\n";
   }
}

bool ossimChipperUtil::hasBandSelection() const
{
   bool result = false;
   if (m_kwl.valid())
   {
      result = m_kwl->hasKey(std::string(ossimKeywordNames::BANDS_KW));
   }
   return result;
}

bool ossimChipperUtil::hasWmsBboxCutWidthHeight() const
{
   bool result = false;

   if (m_kwl.valid())
   {
      result = (m_kwl->hasKey(CUT_HEIGHT_KW) &&
                m_kwl->hasKey(CUT_WIDTH_KW) &&
                (m_kwl->hasKey(CUT_WMS_BBOX_KW) ||
                 m_kwl->hasKey(CUT_WMS_BBOX_LL_KW)));
   }

   return result;
}

bool ossimChipperUtil::hasCutBoxWidthHeight() const
{
   bool result = false;
   if (m_kwl.valid())
   {
      if (m_kwl->hasKey(CUT_HEIGHT_KW))
      {
         if (m_kwl->hasKey(CUT_WIDTH_KW))
         {
            if (m_kwl->hasKey(CUT_MIN_LAT_KW))
            {
               if (m_kwl->hasKey(CUT_MIN_LON_KW))
               {
                  if (m_kwl->hasKey(CUT_MAX_LAT_KW))
                  {
                     if (m_kwl->hasKey(CUT_MAX_LON_KW))
                     {
                        result = true;
                     }
                  }
               }
            } // if lat and lon WMS style bbox is specified then we will behave the same as above
            else if (m_kwl->hasKey(CUT_WMS_BBOX_LL_KW))
            {
               result = true;
            }
         }
      }
   }

   return result;
}

bool ossimChipperUtil::hasScaleOption() const
{
   bool result = false;
   if (m_kwl.valid())
   {
      if (m_kwl->hasKey(METERS_KW.c_str()) ||
          m_kwl->hasKey(DEGREES_X_KW.c_str()) ||
          hasCutBoxWidthHeight() ||
          hasWmsBboxCutWidthHeight())
      {
         result = true;
      }
   }
   return result;
}

bool ossimChipperUtil::isThreeBandOut() const
{
   return keyIsTrue(THREE_BAND_OUT_KW);
}

bool ossimChipperUtil::padThumbnail() const
{
   return keyIsTrue(PAD_THUMBNAIL_KW);
}

void ossimChipperUtil::setReaderProps(ossimImageHandler *ih) const
{
   if (ih && m_kwl.valid())
   {
      ossim_uint32 count = m_kwl->numberOf(READER_PROPERTY_KW.c_str());
      for (ossim_uint32 i = 0; i < count; ++i)
      {
         ossimString key = READER_PROPERTY_KW;
         key += ossimString::toString(i);
         ossimString value = m_kwl->findKey(key.string());
         if (value.size())
         {
            std::vector<ossimString> splitArray;
            value.split(splitArray, "=");
            if (splitArray.size() == 2)
            {
               ossimRefPtr<ossimProperty> prop =
                  new ossimStringProperty(splitArray[0], splitArray[1]);

               ih->setProperty(prop);
            }
         }
      }
   }
}

void ossimChipperUtil::getBandList(std::vector<ossim_uint32> &bandList) const
{
   bandList.clear();
   if (m_kwl.valid())
   {
      ossimString os;
      os.string() = m_kwl->findKey(std::string(ossimKeywordNames::BANDS_KW));
      if (os.size() && (os != "default"))
      {
         std::vector<ossimString> band_list(0);
         os.split(band_list, ossimString(","), false);
         if (band_list.size())
         {
            std::vector<ossimString>::const_iterator i = band_list.begin();
            while (i != band_list.end())
            {
               ossim_uint32 band = (*i).toUInt32();
               if (band) // One based so we need to subtract.
               {
                  bandList.push_back(band - 1);
               }
               ++i;
            }
         }
      }
   }

} // End: ossimChipperUtil::getBandList

bool ossimChipperUtil::hasAnnotations() const
{
   bool result = false;
   if (m_kwl.valid())
   {
      ossimString regularExpression = "annotation[0-9]*\\.type";
      ossim_uint32 count = m_kwl->getNumberOfKeysThatMatch(regularExpression);
      if (count > 0)
      {
         result = true;
      }
   }
   return result;
}

bool ossimChipperUtil::hasLutFile() const
{
   bool result = false;
   if (m_kwl.valid())
   {
      result = (m_kwl->find(LUT_FILE_KW.c_str()) != 0);
   }
   return result;
}

bool ossimChipperUtil::hasBrightnesContrastOperation() const
{
   bool result = false;
   std::string value = m_kwl->findKey(BRIGHTNESS_KW);
   if (value.size())
   {
      result = true;
   }
   else
   {
      value = m_kwl->findKey(CONTRAST_KW);
      if (value.size())
      {
         result = true;
      }
   }
   return result;
}

bool ossimChipperUtil::hasGeoPolyCutterOption() const
{
   bool result = (m_kwl->find(CLIP_WMS_BBOX_LL_KW.c_str()) ||
                  m_kwl->find(CLIP_POLY_LAT_LON_KW.c_str()));

   return result;
}

bool ossimChipperUtil::hasBumpShadeArg() const
{
   bool result = (m_operation == OSSIM_CHIPPER_OP_HILL_SHADE);
   if (!result && m_kwl.valid())
   {
      result = (m_kwl->find(ossimKeywordNames::AZIMUTH_ANGLE_KW) ||
                m_kwl->find(COLOR_RED_KW.c_str()) ||
                m_kwl->find(COLOR_GREEN_KW.c_str()) ||
                m_kwl->find(COLOR_BLUE_KW.c_str()) ||
                m_kwl->find(ossimKeywordNames::ELEVATION_ANGLE_KW) ||
                m_kwl->find(GAIN_KW.c_str()));
   }
   return result;
}

bool ossimChipperUtil::hasThumbnailResolution() const
{
   bool result = false;
   if (m_kwl.valid())
   {
      result = (m_kwl->find(THUMBNAIL_RESOLUTION_KW.c_str()) != 0);
   }
   return result;
}

bool ossimChipperUtil::hasHistogramOperation() const
{
   bool result = false;
   if (m_kwl.valid())
   {
      result = m_kwl->hasKey(HIST_OP_KW);
   }
   return result;
}

bool ossimChipperUtil::hasGammaCorrection() const
{
   bool result = false;
   if (m_kwl.valid())
   {
      result = m_kwl->hasKey(GAMMA_KW);
   }
   return result;

}

bool ossimChipperUtil::isDemFile(const ossimFilename &file) const
{
   bool result = false;
   ossimString ext = file.ext();
   if (ext.size() >= 2)
   {
      ext.downcase();
      if ((ext == "hgt") || (ext == "dem") ||
          (((*ext.begin()) == 'd') && ((*(ext.begin() + 1)) == 't')))
      {
         result = true;
      }
   }
   return result;
}

bool ossimChipperUtil::isSrcFile(const ossimFilename &file) const
{
   bool result = false;
   ossimString ext = file.ext();
   ext.downcase();
   if (ext == "src")
   {
      result = true;
   }
   return result;
}

ossimScalarType ossimChipperUtil::getOutputScalarType() const
{
   ossimScalarType scalar = OSSIM_SCALAR_UNKNOWN;
   ossimString lookup = m_kwl->findKey(OUTPUT_RADIOMETRY_KW);
   if (lookup.size())
   {
      scalar = ossimScalarTypeLut::instance()->getScalarTypeFromString(lookup);
   }
   if (scalar == OSSIM_SCALAR_UNKNOWN)
   {
      // deprecated keyword...
      if (keyIsTrue(std::string(SCALE_2_8_BIT_KW)))
      {
         scalar = OSSIM_UINT8;
      }
   }
   return scalar;
}

bool ossimChipperUtil::scaleToEightBit() const
{
   bool result = false;
   if (getOutputScalarType() == OSSIM_UINT8)
   {
      result = true;
   }
   return result;
}

bool ossimChipperUtil::snapTieToOrigin() const
{
   return keyIsTrue(SNAP_TIE_TO_ORIGIN_KW);
}

void ossimChipperUtil::getImageSpaceScale(ossimDpt &imageSpaceScale) const
{
   imageSpaceScale.x = 1.0;
   imageSpaceScale.y = 1.0;
   ossimString lookup;

   if (m_kwl->hasKey(FULLRES_XYS_KW))
   {
      lookup = m_kwl->findKey(FULLRES_XYS_KW);
      std::vector<ossimString> values;
      lookup.trim().split(values, ",");
      if (values.size() > 2)
      {
         imageSpaceScale.x = values[2].toDouble();
         imageSpaceScale.y = imageSpaceScale.x;
         if (values.size() > 3)
         {
            imageSpaceScale.y = values[3].toDouble();
         }
      }
   }
   else
   {
      std::string value = m_kwl->findKey(IMAGE_SPACE_SCALE_X_KW);
      if (value.size())
      {
         imageSpaceScale.x = ossimString(value).toFloat64();
      }
      else
      {
         imageSpaceScale.x = 1.0;
      }
      value = m_kwl->findKey(IMAGE_SPACE_SCALE_Y_KW);
      if (value.size())
      {
         imageSpaceScale.y = ossimString(value).toFloat64();
      }
      else
      {
         imageSpaceScale.y = 1.0;
      }
   }
}

void ossimChipperUtil::getImageSpacePivot(ossimDpt &imageSpacePivot) const
{
   ossimString lookup;
   imageSpacePivot.makeNan();
   if (m_kwl->hasKey(FULLRES_XYS_KW))
   {
      lookup = m_kwl->findKey(FULLRES_XYS_KW);
      std::vector<ossimString> values;
      lookup.trim().split(values, ",");

      if (values.size() > 1)
      {
         imageSpacePivot.x = values[0].toDouble();
         imageSpacePivot.y = values[1].toDouble();
      }
   }
   else
   {
      if (m_geom)
      {
         ossimDrect rect;
         m_geom->getBoundingRect(rect);
         imageSpacePivot = rect.midPoint();
      }
   }
}

ossim_float64 ossimChipperUtil::getRotation() const
{
   ossim_float64 result = ossim::nan();
   if (m_kwl.valid())
   {
      std::string value = m_kwl->findKey(ROTATION_KW);
      if (value.size())
      {
         result = ossimString(value).toFloat64();
         if (result < 0)
         {
            result += 360.0;
         }

         // Range check:
         if ((result < 0.0) || (result > 360.0))
         {
            std::ostringstream errMsg;
            errMsg << "ossimChipperUtil::getRotation range error!\n"
                   << "rotation = " << result
                   << "\nMust be between 0 and 360.";
            throw ossimException(errMsg.str());
         }
      }
   }
   return result;
}

bool ossimChipperUtil::upIsUp() const
{
   return keyIsTrue(std::string(UP_IS_UP_KW));
}

bool ossimChipperUtil::hasRotation() const
{
   bool result = false;
   std::string value = m_kwl->findKey(std::string(ROTATION_KW));
   if (value.size())
   {
      result = true;
   }
   return result;
}

bool ossimChipperUtil::northUp() const
{
   return keyIsTrue(std::string(NORTH_UP_KW));
}

bool ossimChipperUtil::isChipMode() const
{
   return (m_operation == OSSIM_CHIPPER_OP_CHIP);
}

bool ossimChipperUtil::keyIsTrue(const std::string &key) const
{
   bool result = false;
   if (m_kwl.valid())
   {
      std::string value = m_kwl->findKey(key);
      if (value.size())
      {
         result = ossimString(value).toBool();
      }
   }
   return result;
}

ossim_uint32 ossimChipperUtil::getEntryNumber() const
{
   ossim_uint32 result = 0;
   if (m_kwl.valid())
   {
      std::string value = m_kwl->findKey(std::string(ossimKeywordNames::ENTRY_KW));
      if (value.size())
      {
         result = ossimString(value).toUInt32();
      }
   }
   return result;
}

ossim_int32 ossimChipperUtil::getZone() const
{
   ossim_int32 result = 0;
   if (m_kwl.valid())
   {
      std::string value = m_kwl->findKey(std::string(ossimKeywordNames::ZONE_KW));
      if (value.size())
      {
         result = ossimString(value).toUInt32();
      }
   }
   return result;
}

std::string ossimChipperUtil::getHemisphere() const
{
   std::string result;
   if (m_kwl.valid())
   {
      result = m_kwl->findKey(std::string(ossimKeywordNames::HEMISPHERE_KW));
   }
   return result;
}

bool ossimChipperUtil::hasSensorModelInput()
{
   bool result = false;

   // Test image layers.
   std::vector<ossimRefPtr<ossimSingleImageChain>>::iterator chainIdx = m_imgLayer.begin();
   while (chainIdx != m_imgLayer.end())
   {
      // Get the image handler:
      ossimRefPtr<ossimImageHandler> ih = (*chainIdx)->getImageHandler();
      if (ih.valid())
      {
         // Get the geometry from the first image handler.
         ossimRefPtr<ossimImageGeometry> geom = ih->getImageGeometry();
         if (geom.valid())
         {
            // Get the image projection.
            ossimRefPtr<ossimProjection> proj = geom->getProjection();
            if (proj.valid())
            {
               // Cast and assign to result.
               ossimMapProjection *mapProj = PTR_CAST(ossimMapProjection, proj.get());
               if (!mapProj)
               {
                  result = true;
                  break;
               }
            }
         }
      }
      ++chainIdx;
   }

   if (!result)
   {
      // Test dem layers.
      chainIdx = m_demLayer.begin();
      while (chainIdx != m_demLayer.end())
      {
         // Get the image handler:
         ossimRefPtr<ossimImageHandler> ih = (*chainIdx)->getImageHandler();
         if (ih.valid())
         {
            // Get the geometry from the first image handler.
            ossimRefPtr<ossimImageGeometry> geom = ih->getImageGeometry();
            if (geom.valid())
            {
               // Get the image projection.
               ossimRefPtr<ossimProjection> proj = geom->getProjection();
               if (proj.valid())
               {
                  // Cast and assign to result.
                  ossimMapProjection *mapProj = PTR_CAST(ossimMapProjection, proj.get());
                  if (!mapProj)
                  {
                     result = true;
                     break;
                  }
               }
            }
         }
         ++chainIdx;
      }
   }

   return result;
}

void ossimChipperUtil::initializeSrcKwl()
{
   static const char MODULE[] = "ossimChipperUtil::initializeSrcKwl";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " entered...\n";
   }

   std::string value = m_kwl->findKey(std::string(SRC_FILE_KW));
   if (value.size())
   {
      m_srcKwl = new ossimKeywordlist();
      m_srcKwl->setExpandEnvVarsFlag(true);
      if (m_srcKwl->addFile(value.c_str()) == false)
      {
         m_srcKwl = 0;
      }
   }
   else
   {
      m_srcKwl = 0;
   }

   if (traceDebug())
   {
      if (m_srcKwl.valid())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "src keyword list:\n"
            << *(m_srcKwl.get()) << "\n";
      }
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " exited...\n";
   }
}

ossim_uint32 ossimChipperUtil::getNumberOfInputs() const
{
   ossim_uint32 result = 0;

   ossimString demRegExpr = "dem[0-9]*\\.file";
   ossimString imgRegExpr = "image[0-9]*\\.file";

   if (m_kwl.valid())
   {
      // Look for dems, e.g. dem0.file: foo.tif
      result = m_kwl->getNumberOfKeysThatMatch(demRegExpr);

      // Look for images, e.g. image0.file: foo.tif
      result += m_kwl->getNumberOfKeysThatMatch(imgRegExpr);
   }

   if (m_srcKwl.valid())
   {
      // Look for dems, e.g. dem0.file: foo.tif
      result += m_srcKwl->getNumberOfKeysThatMatch(demRegExpr);

      // Look for images, e.g. image0.file: foo.tif
      result += m_srcKwl->getNumberOfKeysThatMatch(imgRegExpr);
   }

   return result;
}

ossimChipperUtil::ossimChipperOutputProjection ossimChipperUtil::getOutputProjectionType() const
{
   ossimChipperOutputProjection result = ossimChipperUtil::OSSIM_CHIPPER_PROJ_UNKNOWN;
   const char *op = m_kwl->find(ossimKeywordNames::PROJECTION_KW);
   if (op)
   {
      ossimString os = op;
      os.downcase();
      if (os == "geo")
      {
         result = ossimChipperUtil::OSSIM_CHIPPER_PROJ_GEO;
      }
      else if (os == "geo-scaled")
      {
         result = ossimChipperUtil::OSSIM_CHIPPER_PROJ_GEO_SCALED;
      }
      else if (os == "input")
      {
         result = ossimChipperUtil::OSSIM_CHIPPER_PROJ_INPUT;
      }
      else if ((os == "utm") || (os == "ossimutmprojection"))
      {
         result = ossimChipperUtil::OSSIM_CHIPPER_PROJ_UTM;
      }
   }
   return result;
}

void ossimChipperUtil::getClipPolygon(ossimGeoPolygon &polygon) const
{
   ossimString param = m_kwl->find(CLIP_WMS_BBOX_LL_KW.c_str());
   if (!param.empty())
   {
      if (!polygon.addWmsBbox(param))
      {
         polygon.clear();
      }
   }
   else
   {
      param = m_kwl->find(CLIP_POLY_LAT_LON_KW.c_str());
      if (!param.empty())
      {
         std::vector<ossimGpt> points;
         ossim::toVector(points, param);
         if (!points.empty())
         {
            polygon = points;
         }
      }
   }
}

ossim_float64 ossimChipperUtil::getBrightness() const
{
   ossim_float64 brightness = 0.0;
   std::string value = m_kwl->findKey(BRIGHTNESS_KW);
   if (value.size())
   {
      brightness = ossimString(value).toFloat64();

      // Range check it:
      if ((brightness < -1.0) || (brightness > 1.0))
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << "ossimChipperUtil::getBrightness range error!"
            << "\nbrightness: " << brightness
            << "\nvalid range: -1.0 to 1.0"
            << "\nReturned brightness has been reset to: 0.0"
            << std::endl;

         brightness = 0.0;
      }
   }
   return brightness;
}

ossim_float64 ossimChipperUtil::getContrast() const
{
   ossim_float64 contrast = 1.0;
   std::string value = m_kwl->findKey(CONTRAST_KW);
   if (value.size())
   {
      contrast = ossimString(value).toFloat64();

      // Range check it:
      if ((contrast < 0.0) || (contrast > 20.0))
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << "ossimChipperUtil::getContrast range error!"
            << "\ncontrast: " << contrast
            << "\nvalid range: 0 to 20.0"
            << "\nReturned contrast has been reset to: 1.0"
            << std::endl;

         contrast = 1.0;
      }
   }
   return contrast;
}

ossim_float64 ossimChipperUtil::getGamma() const
{
   ossim_float64 gamma = 1.0;
   std::string value = m_kwl->findKey(GAMMA_KW);
   if (value.size())
   {
      gamma = ossimString(value).toFloat64();

      // Range check it:
   }
   return gamma;

}

std::string ossimChipperUtil::getSharpenMode() const
{
   ossimString mode = m_kwl->findKey(SHARPEN_MODE_KW);
   if (mode.size())
   {
      mode.downcase();
      if ((mode != "light") && (mode != "heavy") && (mode != "medium") && (mode != "none"))
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << "ossimChipperUtil::getSharpnessMode WARNING!"
            << "\nInvalid sharpness mode: " << mode
            << "\nValid modes: \"light\", \"medium\", and \"heavy\""
            << std::endl;
         mode = "";
      }
      if (mode == "none")
         mode = "";
   }

   return mode.string();
}

std::string ossimChipperUtil::getSharpenPercent() const
{
   ossimString percent = m_kwl->findKey(SHARPEN_PERCENT_KW);
   if (percent.size())
   {
      ossim_float64 value = percent.toDouble();
      if (value < 0 || value > 1)
      {
         ossimNotify(ossimNotifyLevel_WARN)
             << "ossimChipperUtil::getSharpenPercent WARNING!"
             << "\nInvalid sharpen percent: " << percent
             << "\nValid percent: Between 0 and 1\n";
         percent = "";
      }
   }
   return percent;
}

int ossimChipperUtil::getHistoMode() const
{
   int result = ossimHistogramRemapper::STRETCH_UNKNOWN;

   ossimString op = m_kwl->findKey(HIST_OP_KW);
   op.downcase();

   if ((op.string() == "auto-minmax"))
   {
      result = ossimHistogramRemapper::LINEAR_AUTO_MIN_MAX;
   }
   else if ((op.string() == "auto-percentile"))
   {
      result = ossimHistogramRemapper::LINEAR_AUTO_PERCENTILE;
   }
   else if ((op == "std-stretch-1") || (op == "std-stretch 1"))
   {
      result = ossimHistogramRemapper::LINEAR_1STD_FROM_MEAN;
   }
   else if ((op == "std-stretch-2") || (op == "std-stretch 2"))
   {
      result = ossimHistogramRemapper::LINEAR_2STD_FROM_MEAN;
   }
   else if ((op == "std-stretch-3") || (op == "std-stretch 3"))
   {
      result = ossimHistogramRemapper::LINEAR_3STD_FROM_MEAN;
   }
   else if((op == "linear"))
   {
      result = ossimHistogramRemapper::LINEAR_ONE_PIECE;
   }
   else if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_WARN)
         << "ossimChipperUtil::getHistoMode: Unhandled operation: " << op << "\n";
   }

   return result;
}

void ossimChipperUtil::usage(ossimArgumentParser &ap)
{
   // Add global usage options.
   ossimInit::instance()->addOptions(ap);

   // Set app name.
   std::string appName = ap.getApplicationName();
   ap.getApplicationUsage()->setApplicationName(ossimString(appName));

   // Add options.
   addArguments(ap);

   // Write usage.
   ap.getApplicationUsage()->write(ossimNotify(ossimNotifyLevel_INFO));

   // Keeping single line in tact for examples for cut and paste purposes.
   ossimNotify(ossimNotifyLevel_INFO)

      << "NOTES:\n"
      << "1) Never use same base name in the same directory! Example is you have a Chicago.tif\n"
      << "   and you want a Chicago.jp2, output Chicago.jp2 to its own directory.\n"

      << "\nExample commands:\n"

      << "\n// File conversion: Convert geotiff to a jp2 file.\n"
      << appName << " --op chip -w ossim_kakadu_jp2 Chicago.tif outputs/Chicago.jp2\n"

      << "\n// Orthorectification: Orthorectify a nitf with RPC model out to a geotiff.\n"
      << appName << " --op ortho 5V090205P0001912264B220000100282M_001508507.ntf outputs/ortho.tif\n"

      << "\n// Mosaic: Mosaic multiple images together and output to a geotiff.\n"
      << appName << " --combiner-type ossimImageMosaic --op ortho f1.tif f2.tif f3.tif outputs/mosaic.tif\n"

      << "\n// Mosaic: Feather Mosaic multiple images together and output to a geotiff.\n"
      << appName << " --combiner-type ossimFeatherMosaic --op ortho f1.tif f2.tif f3.tif outputs/feather.tif\n"

      << "\n// Color relief: Colorize two DEMs from a lut, output to a geotiff.\n"
      << appName << " --op color-relief --color-table ossim-dem-color-table-template.kwl N37W123.hgt N38W123.hgt outputs/color-relief.tif\n"

      << "\n// Color relief: Colorize two DEMs from a lut, output to a png thumbnail.\n"
      << appName << " --op color-relief --color-table ossim-dem-color-table-template.kwl -t 1024 -w ossim_png N37W123.hgt N38W123.hgt outputs/color-relief.png\n"

      << "\n// Hill shade: Hill shade two DEMs, output to a geotiff.\n"
      << appName << " --color 255 255 255 --azimuth 270 --elevation 45 --exaggeration 2.0 --op  hillshade N37W123.hgt N38W123.hgt outputs/hillshade.tif\n"

      << "\n// Two color multi view with cut box.  First image is old, second image is new:\n"
      << appName << " --cut-bbox-ll 28.092885092033352 -80.664539599998633 28.109128691071547 -80.626914963229325 --op 2cmv oldMLB.tif newMLB.tif outputs/2cmv-test1.tif\n"

      << "\n// Ortho about point, 512x512, with histogram stretch, and 3,2,1 band order:\n"
      << appName << " --op ortho -b 3,2,1 --histogram-op auto-minmax --cut-center-llwh -42.819784401784275 147.265811350983 512 512 5V090205M0001912264B220000100072M_001508507.ntf orth.tif\n"

      << "\n// Chip, in image space, about point, 512x512, with histogram stretch, and 3,2,1 band order:\n"
      << appName << " --op chip -b 3,2,1 --histogram-op auto-minmax --cut-center-llwh -42.819784401784275 147.265811350983 512 512 5V090205M0001912264B220000100072M_001508507.ntf chip.tif\n"

      << "\n// Chip in image space, rotate \"up is up\"(-u option) about point, 512x512 with histogram stretch and 3,2,1 band order:\n"
      << appName << " --op chip -u -b 3,2,1 --histogram-op auto-minmax --cut-center-llwh -42.819784401784275 147.265811350983 512 512 5V090205M0001912264B220000100072M_001508507.ntf up-is-up-chip.tif\n"

      << "\n// Chip in image space, rotate 39 degrees (-r option) about point, 1024x1024, scaled to eight bit:\n"
      << appName << " --op chip -r 39 --histogram-op auto-minmax --cut-center-llwh -42.883809539602893 147.331984112985765 1024 1024 --output-radiometry ossim_uint8 5V090205P0001912264B220000100282M_001508507.ntf outputs/r39.png\n"

      << "\n// Above command where all options are in a keyword list:\n"
      << appName << " --options r39-options.kwl\n"
      << std::endl;
}
