#include <ossim/support_data/TiffHandlerState.h>
#include <ossim/base/ossimStreamFactoryRegistry.h>
#include <ossim/support_data/ossimGeoTiff.h>
#include <ossim/base/ossimTieGptSet.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/projection/ossimUtmpt.h>
#include <ossim/projection/ossimMapProjection.h>
#include <ossim/projection/ossimBilinearProjection.h>
#include <ossim/projection/ossimEpsgProjectionFactory.h>
#include <ossim/projection/ossimProjectionFactoryRegistry.h>
#include <ossim/base/ossimGeoTiffCoordTransformsLut.h>
#include <ossim/base/ossimGeoTiffDatumLut.h>
#include <ossim/base/ossimStreamFactoryRegistry.h>
#include <ossim/support_data/TiffStreamAdaptor.h>
#include <ossim/support_data/ossimTiffInfo.h>
#include <xtiffio.h>
#include <geo_normalize.h>
#include <geotiff.h>
#include <geo_tiffp.h>
#include <cpl_serv.h>
#include <geovalues.h>
#include <cstddef> //nullptr
#include <sstream>
#include <ossim/projection/ossimEquDistCylProjection.h>

static ossimTrace traceDebug("ossim::TiffHandlerState");

static const ossimGeoTiffCoordTransformsLut COORD_TRANS_LUT;
static const ossimGeoTiffDatumLut DATUM_LUT;

const ossimString ossim::TiffHandlerState::m_typeName = "ossim::TiffHandlerState";

ossim::TiffHandlerState::TiffHandlerState()
{

}
      
ossim::TiffHandlerState::~TiffHandlerState()
{

}

const ossimString& ossim::TiffHandlerState::getTypeName()const
{
   return m_typeName;
}

const ossimString& ossim::TiffHandlerState::getStaticTypeName()
{
   return m_typeName;
}

void ossim::TiffHandlerState::addValue(const ossimString& key, 
                                       const ossimString& value)
{
  m_tags.add(key.c_str(), value.c_str());
}

bool ossim::TiffHandlerState::getValue(ossimString& value,
                                       const ossim_uint32 directory, 
                                        const ossimString& key)const
{
  return getValue(value, 
                  "tiff.image"+ossimString::toString(directory)+"."+key);
}

bool ossim::TiffHandlerState::getValue(ossimString& value,
                                       const ossimString& key)const
{
  bool result = false;

  const char* v = m_tags.find(key.c_str());
  if(v)
  {
    result = true;
    value  = v;
  }

  return result;
}

bool ossim::TiffHandlerState::exists(ossim_uint32 directory, const ossimString& key)const
{
  return exists("tiff.image"+
                ossimString::toString(directory)+
                "."+key);

}
bool ossim::TiffHandlerState::exists(const ossimString& key)const
{
  return (m_tags.find(key)!= 0);

}

bool ossim::TiffHandlerState::checkBool(ossim_uint32 directory, const ossimString& key)const
{
  return checkBool("tiff.image"+ossimString::toString(directory)+"."+key);
}

bool ossim::TiffHandlerState::checkBool(const ossimString& key)const
{
  bool result = false;
  const char* value = m_tags.find(key);
  if(value)
  {
    result = ossimString(value).toBool();
  }
  return result;
}

bool ossim::TiffHandlerState::loadDefaults(const ossimFilename& file, 
                                           ossim_uint32 entry)
{
  bool result = ImageHandlerState::loadDefaults(file, entry);
  
  if(result)
  {
    std::shared_ptr<std::istream> tiffStream = ossim::StreamFactoryRegistry::instance()->createIstream(file);
    if(tiffStream)
    {
       std::shared_ptr<ossim::TiffIStreamAdaptor> streamAdaptor = std::make_shared<ossim::TiffIStreamAdaptor>(tiffStream,
                                                                   file.c_str());
       TIFF* tiffPtr = XTIFFClientOpen(file.c_str(), "rm", 
                                    (thandle_t)streamAdaptor.get(),
                                    ossim::TiffIStreamAdaptor::tiffRead, 
                                    ossim::TiffIStreamAdaptor::tiffWrite, 
                                    ossim::TiffIStreamAdaptor::tiffSeek, 
                                    ossim::TiffIStreamAdaptor::tiffClose, 
                                    ossim::TiffIStreamAdaptor::tiffSize,
                                    ossim::TiffIStreamAdaptor::tiffMap, 
                                    ossim::TiffIStreamAdaptor::tiffUnmap);

       if(tiffPtr)
       {
         loadDefaults(tiffPtr);
         result = true;
         XTIFFClose(tiffPtr);
         tiffPtr = 0;
       }
    }    
  }

  return result;
}

void ossim::TiffHandlerState::loadDefaults(TIFF* tiffPtr)
{
  ossim_int64   currentDirectory = TIFFCurrentDirectory(tiffPtr);
  TIFFSetDirectory(tiffPtr, 0);
  ossim_int32   numberOfDirectories = TIFFNumberOfDirectories(tiffPtr);

  addValue("number_of_directories", ossimString::toString(numberOfDirectories));

  ossim_int32 idx=0;
  for(;idx < numberOfDirectories;++idx)
  {
    if (!TIFFSetDirectory(tiffPtr, idx))
    {
       break;
    }
    else
    {
      loadDirectory(tiffPtr, idx);
    }
  }
  TIFFSetDirectory(tiffPtr, currentDirectory);
}

void ossim::TiffHandlerState::loadDefaults(std::shared_ptr<ossim::istream> &str,
                                         const std::string &connectionString)
{
  setConnectionString(connectionString);
  ossim_int64 offset = str->tellg();
  ossimTiffInfo info;
  std::ostringstream out;
  str->seekg(0);
  if(info.open(str, connectionString))
  {
    std::vector<ossimString> prefixValues;
    ossim_int32 nValues=0;
    info.print(out);
    m_tags.parseString(out.str());
    m_tags.getSortedList(prefixValues, "tiff.image");
    nValues = prefixValues.size();
    addValue("tiff.number_of_directories", ossimString::toString(nValues));
    // ossim_uint32 idx = 0;
    //ossim_int64 h = getImageLength(0);
    //ossim_int64 w = getImageWidth(0);
    ossim_int64 tw = getTileWidth(0);
    ossim_int64 th = getTileLength(0);
    //bool isTiled = (th&&tw&& ((tw < w) && (th < h)));
    bool isTiled = (tw>0 && th>0); // Can be a single tile (no strip info)
    for (auto key : prefixValues)
    {
      if(isTiled)
      {
        addValue(key + ".is_tiled", "true");
      }
      else
      {
        addValue(key+".is_tiled", "false");
      }
    }
  }
  // std::cout << "______________________________________________\n"
  //           << m_tags
  //           << "_______________________________________________\n";
  str->clear();
  str->seekg(offset);
}

void ossim::TiffHandlerState::loadCurrentDirectory(TIFF* tiffPtr)
{
  if(tiffPtr) 
  {
    loadDirectory(tiffPtr, TIFFCurrentDirectory(tiffPtr));
  }
}

void ossim::TiffHandlerState::loadDirectory(TIFF* tiffPtr, 
                                            ossim_uint32 directory)
{
  ossim_uint32  imageWidth=0;
  ossim_uint32  imageLength=0;
  // ossim_int32   readMethod=0;
  ossim_uint16  planarConfig=0;
  ossim_uint16  photometric=0;
  ossim_uint32  rowsPerStrip=0;
  ossim_uint32  imageTileWidth=0;
  ossim_uint32  imageTileLength=0;
  // ossim_uint32  imageDirectoryList=0;
  ossim_int32   compressionType=0;
  ossim_uint32  subFileType=0;
  ossim_uint16  bitsPerSample=0;
  ossim_uint16  samplesPerPixel=0;
  ossim_uint16  sampleFormatUnit=0;
  ossim_float64 sampleValue=0;
  ossim_uint16  sampleUintValue = 0;

  ossim_int32   numberOfDirectories = TIFFNumberOfDirectories(tiffPtr);

  addValue("number_of_directories", ossimString::toString(numberOfDirectories));

  if(TIFFCurrentDirectory(tiffPtr) != directory)
  {
    if(!TIFFSetDirectory(tiffPtr, directory))
    {
      return;
    }
  }
  ossimString dirPrefix = "tiff.image"+ossimString::toString(directory)+".";

  if(TIFFGetField(tiffPtr, TIFFTAG_COMPRESSION, &compressionType))
  {
    addValue(dirPrefix+"compression", ossimString::toString(compressionType));
  } 

  if( TIFFGetField(tiffPtr, TIFFTAG_BITSPERSAMPLE, &bitsPerSample) )
  {
    addValue(dirPrefix+"bits_per_sample", ossimString::toString(bitsPerSample));
  }
  if( TIFFGetField(tiffPtr,
                   TIFFTAG_SAMPLESPERPIXEL,
                   &samplesPerPixel ) )
  {
    addValue(dirPrefix+"samples_per_pixel", ossimString::toString(samplesPerPixel));
  }

  if ( TIFFGetField( tiffPtr,
                     TIFFTAG_SAMPLEFORMAT,
                     &sampleFormatUnit ) )
  {
    addValue(dirPrefix+"sample_format", ossimString::toString(sampleFormatUnit));
  }

  if (TIFFGetField(tiffPtr,
                   TIFFTAG_SMAXSAMPLEVALUE,
                    &sampleValue ) )
  {
    addValue(dirPrefix+"max_sample_value", ossimString::toString(sampleValue));
  }
  else
  {
    if(TIFFGetField( tiffPtr,
                      TIFFTAG_MAXSAMPLEVALUE,
                      &sampleUintValue))
    {
      addValue(dirPrefix+"max_sample_value", ossimString::toString(sampleUintValue));
    }

  }

  if ( TIFFGetField( tiffPtr,
                     TIFFTAG_SMINSAMPLEVALUE,
                     &sampleValue ) )
  {
    addValue(dirPrefix+"min_sample_value", ossimString::toString(sampleValue));
  }
  else
  {
    if(TIFFGetField( tiffPtr,
                      TIFFTAG_MINSAMPLEVALUE,
                      &sampleUintValue))
    {
      addValue(dirPrefix+"min_sample_value", ossimString::toString(sampleUintValue));
    }
  }

    
  // lines:
  if ( TIFFGetField( tiffPtr,
                      TIFFTAG_IMAGELENGTH,
                      &imageLength ) )
  {
    addValue(dirPrefix+"image_length", 
             ossimString::toString(imageLength));
  }

  // samples:
  if ( TIFFGetField( tiffPtr,
                      TIFFTAG_IMAGEWIDTH,
                      &imageWidth ) )
  {
    addValue(dirPrefix+"image_width", 
             ossimString::toString(imageWidth));
  }
  
  if (TIFFGetField(tiffPtr,
                   TIFFTAG_SUBFILETYPE ,
                   &subFileType ) )
  {
    addValue(dirPrefix+"sub_file_type", ossimString::toString(subFileType));
  }

  if( TIFFGetField( tiffPtr, TIFFTAG_PLANARCONFIG,
                     &planarConfig ) )
  {
    addValue(dirPrefix+"planar_configuration", 
             ossimString::toString(planarConfig));
  }
  
  if( TIFFGetField( tiffPtr, TIFFTAG_PHOTOMETRIC,
                     &photometric ) )
  {
    addValue(dirPrefix + "photo_interpretation",
             ossimString::toString(photometric));
  }

  // Check for palette.
  ossim_uint16* red;
  ossim_uint16* green;
  ossim_uint16* blue;
  if(TIFFGetField(tiffPtr, TIFFTAG_COLORMAP, &red, &green, &blue))
  {
    if(bitsPerSample)
    {
      saveColorMap(dirPrefix, red, green, blue, 1<<bitsPerSample);
    }
  }

  if( TIFFIsTiled(tiffPtr))
  {
    addValue(dirPrefix+"is_tiled", "true");
    if ( TIFFGetField( tiffPtr,
                       TIFFTAG_TILEWIDTH,
                       &imageTileWidth ) )
    {
      addValue(dirPrefix+"tile_width", 
               ossimString::toString(imageTileWidth));
    }
    if ( TIFFGetField( tiffPtr,
                       TIFFTAG_TILELENGTH,
                       &imageTileLength ) )
    {
      addValue(dirPrefix+"tile_length", 
               ossimString::toString(imageTileLength));
    }
  }
  else
  {
    addValue(dirPrefix+"tiff_is_tiled", "false");
    if( TIFFGetField( tiffPtr, TIFFTAG_ROWSPERSTRIP,
                      &rowsPerStrip ) )
    {
      addValue(dirPrefix+"rows_per_strip", 
               ossimString::toString(rowsPerStrip));
    }
  }

  loadGeotiffTags(tiffPtr, dirPrefix);    
}


void ossim::TiffHandlerState::saveColorMap(const ossimString& dirPrefix,
                                            const ossim_uint16* red, 
                                            const ossim_uint16* green,
                                            const ossim_uint16* blue,
                                            ossim_uint32 numberOfEntries)
{
  ossim_uint32 idx = 0;
  std::ostringstream redStream;
  std::ostringstream greenStream;
  std::ostringstream blueStream;
  redStream << "(";
  greenStream << "(";
  blueStream << "(";  
  for(;idx < (numberOfEntries-1);++idx,++red,++green,++blue)
  {
    redStream   << *red << ",";
    greenStream << *green << ",";
    blueStream  << *blue << ",";
    if((idx%16)==0)
    {
      redStream   << "\n";
      greenStream << "\n";
      blueStream  << "\n";      
    }
  }  
  redStream   << *red << ")";
  greenStream << *green << ")";
  blueStream  << *blue << ")";
  
  addValue(dirPrefix+"colormap.red", 
           redStream.str());
  addValue(dirPrefix+"colormap.green", 
           greenStream.str());
  addValue(dirPrefix+"colormap.blue", 
           blueStream.str());
}

void ossim::TiffHandlerState::loadGeotiffTags(TIFF* tiffPtr,
                                              const ossimString& dirPrefix )
{
  const int CITATION_STRING_SIZE = 512;
  ossim_uint16 rasterType = 0;
  ossim_uint16 pcsCode = 0;
  ossim_uint16 coordTransGeoCode = 0;
  // ossim_uint32 idx = 0;
  GTIF* gtif = GTIFNew(tiffPtr);
  char citationStrPtr[CITATION_STRING_SIZE];
  char* buf = 0;
  ossim_uint16 doubleArraySize = 0;
  double tempDouble=0.0;
  double* doubleArray=0;
  ossimString doubleArrayStr;
  bool loadedGeotiff = false;
  if(!gtif)
  {
    addValue(dirPrefix+"is_geotiff", "false");
    return;
  } 

  // Note:  For some reason I am having troubles using
  // GTIFGetDefn so I have to query them directly.  I was getting
  // Core dumps in the JNI calls to the OSSIM core library.
  // Until this is resolved I will use the direct method of getting
  // the values I need.
  //
  ossim_uint16 tempUint16 = 0; 
  if(GTIFKeyGet(gtif, GTModelTypeGeoKey, &tempUint16, 0, 1))
  {
    loadedGeotiff = true;
    addValue(dirPrefix+"model_type", 
           ossimString::toString(tempUint16));
  }
  if(GTIFKeyGet(gtif, GeographicTypeGeoKey, &tempUint16, 0, 1))
  {
    loadedGeotiff = true;
    addValue(dirPrefix+"gcs_code", 
           ossimString::toString(tempUint16));
  }
  if(GTIFKeyGet(gtif, GeogGeodeticDatumGeoKey, &tempUint16, 0, 1))
  {
    loadedGeotiff = true;
    addValue(dirPrefix+"datum_code", 
           ossimString::toString(tempUint16));
  }
  if(GTIFKeyGet(gtif, GeogAngularUnitsGeoKey, &tempUint16, 0, 1))
  {
    loadedGeotiff = true;
    addValue(dirPrefix+"angular_units", 
           ossimString::toString(tempUint16));
  }
  if(GTIFKeyGet(gtif, GeogLinearUnitsGeoKey, &tempUint16, 0, 1))
  {
    loadedGeotiff = true;
    addValue(dirPrefix+"linear_units", 
           ossimString::toString(tempUint16));
  }

  if(GTIFKeyGet(gtif, ProjStdParallel1GeoKey, &tempDouble, 0, 1))
  {
    loadedGeotiff = true;
    addValue(dirPrefix+"std_parallel_1", 
            ossimString::toString(tempDouble));
  }
  if(GTIFKeyGet(gtif, ProjStdParallel2GeoKey, &tempDouble, 0, 1))
  {
    loadedGeotiff = true;
    addValue(dirPrefix+"std_parallel_2", 
            ossimString::toString(tempDouble));
  }
  if(GTIFKeyGet(gtif, ProjNatOriginLongGeoKey, &tempDouble, 0, 1))
  {
    loadedGeotiff = true;
    addValue(dirPrefix+"origin_lon", 
            ossimString::toString(tempDouble));
  }
  else if(GTIFKeyGet(gtif, ProjOriginLongGeoKey, &tempDouble, 0, 1))
  {
    loadedGeotiff = true;
    addValue(dirPrefix+"origin_lon", 
            ossimString::toString(tempDouble));
  }
  if(GTIFKeyGet(gtif, ProjNatOriginLatGeoKey, &tempDouble, 0, 1))
  {
    loadedGeotiff = true;
    addValue(dirPrefix+"origin_lat", 
            ossimString::toString(tempDouble));
  }
  else if(GTIFKeyGet(gtif, ProjOriginLatGeoKey, &tempDouble, 0, 1))
  {
    loadedGeotiff = true;
    addValue(dirPrefix+"origin_lat", 
            ossimString::toString(tempDouble));
  }
  if(GTIFKeyGet(gtif, ProjFalseEastingGeoKey, &tempDouble, 0, 1))
  {
    loadedGeotiff = true;
    addValue(dirPrefix+"false_easting", 
            ossimString::toString(tempDouble));
  }
  if(GTIFKeyGet(gtif, ProjFalseNorthingGeoKey, &tempDouble, 0, 1))
  {
    loadedGeotiff = true;
    addValue(dirPrefix+"false_northing", 
            ossimString::toString(tempDouble));
  }
  if(GTIFKeyGet(gtif, ProjCenterLongGeoKey, &tempDouble, 0, 1))
  {
    loadedGeotiff = true;
    addValue(dirPrefix+"center_lon", 
            ossimString::toString(tempDouble));
  }
  if(GTIFKeyGet(gtif, ProjCenterLatGeoKey, &tempDouble, 0, 1))
  {
    loadedGeotiff = true;
    addValue(dirPrefix+"center_lat", 
            ossimString::toString(tempDouble));
  }
  if(GTIFKeyGet(gtif, ProjScaleAtNatOriginGeoKey, &tempDouble, 0, 1))
  {
    loadedGeotiff = true;
    addValue(dirPrefix+"scale_factor", 
            ossimString::toString(tempDouble));
  }
  if(GTIFKeyGet(gtif, GTRasterTypeGeoKey, &rasterType, 0, 1))
  {
    loadedGeotiff = true;
    addValue(dirPrefix+"raster_type", 
             ossimString::toString(rasterType));

  }
  if (GTIFKeyGet(gtif, ProjectedCSTypeGeoKey, &pcsCode, 0, 1))
  {
    loadedGeotiff = true;
    addValue(dirPrefix+"pcs_code", 
             ossimString::toString(pcsCode));
  }

  if ( GTIFKeyGet(gtif, GTCitationGeoKey, &citationStrPtr ,
                  0, CITATION_STRING_SIZE))
  {
    loadedGeotiff = true;
    addValue(dirPrefix+"citation", 
             ossimString(citationStrPtr));
  }
  if(GTIFKeyGet(gtif, PCSCitationGeoKey , &buf, 0, 1))
  {
    loadedGeotiff = true;
    addValue(dirPrefix+"pcs_citation", 
             buf);
  }
  if(GTIFKeyGet(gtif, ProjCoordTransGeoKey , &coordTransGeoCode, 0, 1))
  {
    loadedGeotiff = true;
    addValue(dirPrefix+"coord_trans_code", 
             ossimString::toString(coordTransGeoCode));
  }

  if(TIFFGetField(tiffPtr, TIFFTAG_GEOPIXELSCALE, &doubleArraySize, &doubleArray))
  {
    loadedGeotiff = true;
    convertArrayToStringList(doubleArrayStr, doubleArray, doubleArraySize);
    if(!doubleArrayStr.empty())
    {
      addValue(dirPrefix+"geo_pixel_scale", doubleArrayStr);
    }
  }
  if(TIFFGetField(tiffPtr, TIFFTAG_GEOTIEPOINTS,  &doubleArraySize, &doubleArray))
  {
    loadedGeotiff = true;
    convertArrayToStringList(doubleArrayStr, doubleArray, doubleArraySize);
    if(!doubleArrayStr.empty())
    {
      addValue(dirPrefix+"geo_tie_points", doubleArrayStr);
    }
  }
  if(TIFFGetField(tiffPtr, TIFFTAG_GEODOUBLEPARAMS, &doubleArraySize, &doubleArray))
  {
    loadedGeotiff = true;
    convertArrayToStringList(doubleArrayStr, doubleArray, doubleArraySize);
    if(!doubleArrayStr.empty())
    {
      addValue(dirPrefix+"geo_double_params", doubleArrayStr);
    }
  }
  if(TIFFGetField(tiffPtr, TIFFTAG_GEOTRANSMATRIX, &doubleArraySize, &doubleArray))
  {
    loadedGeotiff = true;
    convertArrayToStringList(doubleArrayStr, doubleArray, doubleArraySize);
    if(!doubleArrayStr.empty())
    {
      addValue(dirPrefix+"geo_trans_matrix", doubleArrayStr);
    }
  }
  if(TIFFGetField(tiffPtr, TIFFTAG_GEOASCIIPARAMS, &buf))
  {
    loadedGeotiff = true;
    addValue(dirPrefix+"geo_ascii_params", buf);
  }
#if 0
  if(gtif)
  {
    GTIFDefn *defs = GTIFAllocDefn();
    GTIFGetDefn(gtif, defs);
    if (!exists(dirPrefix + "angular_units"))
    {
      loadedGeotiff = true;
      addValue(dirPrefix + "angular_units",
               ossimString::toString(defs->UOMAngle));
    }
    if (!exists(dirPrefix + "linear_units"))
    {
      loadedGeotiff = true;
      addValue(dirPrefix + "linear_units",
               ossimString::toString(defs->UOMLength));
    }
    if (!exists(dirPrefix + "datum_code"))
    {
      loadedGeotiff = true;
      addValue(dirPrefix + "datum_code",
               ossimString::toString(defs->Datum));
    }
    if (!exists(dirPrefix + "pcs_code"))
    {
      loadedGeotiff = true;
      addValue(dirPrefix + "pcs_code",
               ossimString::toString(defs->PCS));
    }
    if (!exists(dirPrefix + "gcs_code"))
    {
      loadedGeotiff = true;
      addValue(dirPrefix + "gcs_code",
               ossimString::toString(defs->GCS));
    }
    if (!exists(dirPrefix + "model_type"))
    {
      loadedGeotiff = true;
      addValue(dirPrefix + "model_type",
               ossimString::toString(defs->Model));
    }
    GTIFFreeDefn(defs);
  }
#endif

  addValue(dirPrefix+"is_geotiff", ossimString::toString(loadedGeotiff));
  GTIFFree(gtif);
}

bool ossim::TiffHandlerState::isDigitalGlobe() const
{
  ossimString value;
  bool result = false;

  if(getValue(value, "tiff.is_digital_globe"))
  {
    result = value.toBool();
  }

  return result;
}

bool ossim::TiffHandlerState::isReduced(ossim_uint32 directory)const
{
   // return getSubFileType(directory) & FILETYPE_REDUCEDIMAGE;
   return getSubFileType(directory) == FILETYPE_REDUCEDIMAGE;
}

bool ossim::TiffHandlerState::isMask(ossim_uint32 directory)const
{
   //---
   // NOTE:
   // sub_file_type of 5(bit mask) from cloud optimized geotiff(COG) sample
   // is not in tiff.h yet. Not sure if it's standard yet. drb - 20211109
   //---
   
   // return getSubFileType(directory) & FILETYPE_MASK;
   ossim_int32 sub_file_type = getSubFileType(directory);
   return (sub_file_type == FILETYPE_MASK) || (sub_file_type == 5);
}

bool ossim::TiffHandlerState::isPage(ossim_uint32 directory)const
{
   // return getSubFileType(directory) & FILETYPE_PAGE;
   return getSubFileType(directory) == FILETYPE_PAGE;
}

bool ossim::TiffHandlerState::isTiled(ossim_uint32 directory)const
{
  return checkBool(directory, "is_tiled");
}

void ossim::TiffHandlerState::convertArrayToStringList(ossimString& result, double* doubleArray, ossim_int32 doubleArraySize)const
{
  ossim_int32 idx = 0;
  result = "";
  if(doubleArray && (doubleArraySize > 0))
  {
     // ossim_int32 precision = 20;
    std::ostringstream doubleArrayStream;
    doubleArrayStream << "(";
    for(idx = 0; idx < doubleArraySize-1;++idx)
    {
      doubleArrayStream << ossimString::toString(doubleArray[idx],20) << ",";
    }
    doubleArrayStream << ossimString::toString(doubleArray[idx],20) << ")";
    result = doubleArrayStream.str();
  }
}

ossim_int32 ossim::TiffHandlerState::getModelType(ossim_int32 directory)const
{
  ossimString tempStr;
  ossim_int32 result = 0;

  if (!getValue(tempStr, directory, "model_type"))
  {
    tempStr = "unknown";
  }

  if (tempStr == "geographic")
  {
    result = ModelTypeGeographic;
  }
  else if (tempStr == "projected")
  {
    result = ModelTypeProjected;
  }
  else if (tempStr == "geocentric")
  {
    result = ModelTypeGeocentric;
  }

  return result;
}

ossim_int32 ossim::TiffHandlerState::getAngularUnits(ossim_int32 directory)const
{
  return getInt32Value("angular_units", directory);
}

ossim_int32 ossim::TiffHandlerState::getLinearUnits(ossim_int32 directory)const
{
  return getInt32Value("linear_units", directory);
}

ossim_int32 ossim::TiffHandlerState::getDatumCode(ossim_int32 directory)const
{
  return getInt32Value("datum_code", directory);
}

ossim_int32 ossim::TiffHandlerState::getPcsCode(ossim_int32 directory)const
{
  return getInt32Value("pcs_code", directory);
}

ossim_int32 ossim::TiffHandlerState::getGcsCode(ossim_int32 directory)const
{

  return getInt32Value("gcs_code", directory);
}

ossim_int32 ossim::TiffHandlerState::getInt32Value(const ossimString& key, 
                                                    ossim_int32 directory)const
{
  ossimString tempStr;
  
  if(getValue(tempStr, directory, key))
  {
    return tempStr.toInt32();
  }
  
  return ossimGeoTiff::UNDEFINED;    
}


ossim_float64 ossim::TiffHandlerState::getDoubleValue(const ossimString& key, 
                                                      ossim_int32 directory)const
{
  ossimString tempStr;
  
  if(getValue(tempStr, directory, key))
  {
    return tempStr.toDouble();
  }
  
  return ossim::nan();  
}

ossim_float64 ossim::TiffHandlerState::getOriginLat(ossim_int32 directory)const
{
  return getDoubleValue("origin_lat", directory);  
}

ossim_float64 ossim::TiffHandlerState::getOriginLon(ossim_int32 directory)const
{
  return getDoubleValue("origin_lon", directory);  
}

ossim_float64 ossim::TiffHandlerState::getStandardParallel1(ossim_int32 directory)const
{
  return getDoubleValue("std_parallel_1", directory);  
}

ossim_float64 ossim::TiffHandlerState::getStandardParallel2(ossim_int32 directory)const
{
  return getDoubleValue("std_parallel_2", directory);  
}

ossim_float64 ossim::TiffHandlerState::getFalseEasting(ossim_int32 directory)const
{
  return getDoubleValue("false_easting", directory);  
}

ossim_float64 ossim::TiffHandlerState::getFalseNorthing(ossim_int32 directory)const
{
  return getDoubleValue("false_northing", directory);  

}

ossim_float64 ossim::TiffHandlerState::getScaleFactor(ossim_int32 directory)const
{
  return getDoubleValue("scale_factor", directory);  
}

ossim_int32 ossim::TiffHandlerState::getCompressionType(ossim_int32 directory) const
{
  ossim_int32 result = 0;
  ossimString tempValue;

  if (getValue(tempValue, directory, "compression"))
  {
    result = tempValue.toInt32();
  }

  return result;
}

ossim_uint16 ossim::TiffHandlerState::getBitsPerSample(ossim_int32 directory) const
{
  ossim_uint16 result = 8;
  ossimString tempValue;

  if (getValue(tempValue, directory, "bits_per_sample"))
  {
    result = tempValue.toUInt16();
  }

  return result;
}

ossim_uint16 ossim::TiffHandlerState::getSamplesPerPixel(ossim_int32 directory) const
{
  ossim_uint16 result = 0;
  ossimString tempValue;

  if (getValue(tempValue, directory, "samples_per_pixel"))
  {
    result = tempValue.toUInt16();
  }

  return result;
}

ossim_uint16 ossim::TiffHandlerState::getSampleFormat(ossim_int32 directory) const
{
  ossim_uint16 result = 1;
  ossimString tempValue;

  if (getValue(tempValue, directory, "sample_format"))
  {
    result = tempValue.toUInt16();
  }

  return result;
}

bool ossim::TiffHandlerState::getMinSampleValue(ossim_float64 &minSampleValue, ossim_int32 directory)const
{
  bool result = false;
  ossimString tempValue;
  
  if (getValue(tempValue, directory, "min_sample_value"))
  {
    minSampleValue = tempValue.toFloat64();
    result = true;
  }
  else if (getValue(tempValue, directory, "smin_sample_value"))
  {
    minSampleValue = tempValue.toFloat64();
    result = true;
  }

  return result;
}

bool ossim::TiffHandlerState::getMaxSampleValue(ossim_float64 &maxSampleValue, ossim_int32 directory)const
{
  bool result = false;
  ossimString tempValue;

  if (getValue(tempValue, directory, "max_sample_value"))
  {
    maxSampleValue = tempValue.toFloat64();
    result = true;
  }
  else if (getValue(tempValue, directory, "smax_sample_value"))
  {
    maxSampleValue = tempValue.toFloat64();
    result = true;
  }

  return result;
}
ossim_uint32 ossim::TiffHandlerState::getRowsPerStrip(ossim_int32 directory) const
{
  ossim_uint32 result = 0;
  ossimString tempValue;
  if(getValue(tempValue, directory, "rows_per_strip"))
  {
    result = tempValue.toUInt32();
  }
  
  return result;
}

bool ossim::TiffHandlerState::hasColorMap(ossim_int32 directory)const
{
  return exists(directory, "colormap");
}

#if 0 /* Overloaded methods for int32 dir not called in core. */
bool ossim::TiffHandlerState::isReduced(ossim_int32 directory) const
{
  
  return getSubFileType(directory) == FILETYPE_REDUCEDIMAGE;
}

bool ossim::TiffHandlerState::isPage(ossim_int32 directory) const
{
  return getSubFileType(directory) == FILETYPE_PAGE;
}

bool ossim::TiffHandlerState::isMask(ossim_int32 directory) const
{
  return getSubFileType(directory) == FILETYPE_MASK;
}
#endif

ossim_int32 ossim::TiffHandlerState::getSubFileType(ossim_int32 directory) const
{
  ossim_int32 result = 0;
  ossimString tempValue;
  if (getValue(tempValue, directory, "sub_file_type"))
  {
    result = tempValue.toInt32();
  }
  return result;
}

ossim_uint32 ossim::TiffHandlerState::getNumberOfDirectories() const
{
  ossim_uint32 result = 0;
  ossimString tempValue;

  if(getValue(tempValue, "tiff.number_of_directories"))
  {
    result = tempValue.toUInt32();
  }

  return result;
}

bool ossim::TiffHandlerState::getColorMap(std::vector<ossim_uint16> &red,
                                          std::vector<ossim_uint16> &green,
                                          std::vector<ossim_uint16> &blue,
                                          ossim_int32 directory) const
{
  ossimString tempStr;
  red.clear();
  green.clear();
  blue.clear();
  bool result = false;

  if (getValue(tempStr, directory, "colormap"))
  {
    std::vector<ossim_uint16> values;
    ossim::toSimpleVector(values, "("+tempStr.substitute(" ", ",", true)+")");
    ossim_uint32 offset = 0;
    if(values.size() == 768)
    {
      offset = 256;
    }
    else if (values.size() == 196608)
    {
      offset = 65536;
    }
    if(offset)
    {
      ossim_uint16 *valuesPtr = &values.front();
      red = std::vector<ossim_uint16>(valuesPtr, (valuesPtr + offset));
      valuesPtr += offset;
      green = std::vector<ossim_uint16>(valuesPtr, (valuesPtr + offset));
      valuesPtr += offset;
      blue = std::vector<ossim_uint16>(valuesPtr, (valuesPtr + offset));
      result = true;
    }
  }
  return result;
}

ossim_int32 ossim::TiffHandlerState::getPlanarConfig(ossim_int32 directory) const
{
  ossimString tempStr;
  ossim_int32 result = 0;
  if (getValue(tempStr, directory, "planar_configuration"))
  {
     //---
     // ossimTiffInfo Key changed:
     // planar_configuration: contains integer code from tiff file
     // planar_configuration_string: is string (drb)
     //---
     result = tempStr.toInt32();
#if 0
    if (tempStr.contains("separate"))
    {
      result = PLANARCONFIG_SEPARATE;
    }
    else if(tempStr.contains("contig"))
    {
      result = PLANARCONFIG_CONTIG;
    }
#endif
  }
  return result;
}

ossim_int32 ossim::TiffHandlerState::getRasterType(ossim_int32 directory) const
{
  ossimString tempStr;
  ossimPixelType result = OSSIM_PIXEL_IS_POINT;
  if (getValue(tempStr, directory, "raster_type"))
  {
    if (tempStr.contains("area"))
    {
      result = OSSIM_PIXEL_IS_AREA;
    }
  }
  return result;
}

ossim_int32 ossim::TiffHandlerState::getPhotoInterpretation(ossim_int32 directory) const {

  ossimString tempStr;
  ossim_int32 result = 0;
  if (getValue(tempStr, directory, "photo_interpretation"))
  {
     //---
     // ossimTiffInfo key changed:
     // photo_interpretation: contains integer code from tiff file
     // photo_interpretation_string: is string (drb)
     //---
     result = tempStr.toInt32();
#if 0
    if (tempStr.contains("MINISWHITE"))
    {
      result = PHOTOMETRIC_MINISWHITE;
    }
    else if (tempStr.contains("MINISBLACK"))
    {
      result = PHOTOMETRIC_MINISBLACK;
    }
    else if (tempStr.contains("RGB"))
    {
      result = PHOTOMETRIC_RGB;
    }
    else if (tempStr.contains("PALETTE"))
    {
      result = PHOTOMETRIC_PALETTE;
    }
    else if (tempStr.contains("MASK"))
    {
      result = PHOTOMETRIC_MASK;
    }
    else if (tempStr.contains("SEPARATED"))
    {
      result = PHOTOMETRIC_SEPARATED;
    }
    else if (tempStr.contains("YCBCR"))
    {
      result = PHOTOMETRIC_YCBCR;
    }
    else if (tempStr.contains("CIELAB"))
    {
      result = PHOTOMETRIC_CIELAB;
    }
    else if (tempStr.contains("ICCLAB"))
    {
      result = PHOTOMETRIC_ICCLAB;
    }
    else if (tempStr.contains("ITULAB"))
    {
      result = PHOTOMETRIC_ITULAB;
    }
    else if (tempStr.contains("LOGL"))
    {
      result = PHOTOMETRIC_LOGL;
    }
    else if (tempStr.contains("LOGLUV"))
    {
      result = PHOTOMETRIC_LOGLUV;
    }
#endif
  }

  return result;
}

ossim_int64 ossim::TiffHandlerState::getTileLength(ossim_int32 directory) const
{
  ossimString tempStr;
  ossim_int64 result = 0;
  if (getValue(tempStr, directory, "tile_length"))
  {
    result = tempStr.toInt64();
  }

  return result;
}
ossim_int64 ossim::TiffHandlerState::getImageLength(ossim_int32 directory) const
{
  ossimString tempStr;
  ossim_int64 result = 0;

  if (getValue(tempStr, directory, "image_length"))
  {
    result = tempStr.toInt64();
  }

  return result;
}

ossim_int64 ossim::TiffHandlerState::getTileWidth(ossim_int32 directory) const
{
  ossimString tempStr;
  ossim_int64 result = 0;

  if (getValue(tempStr, directory, "tile_width"))
  {
    result = tempStr.toInt64();
  }

  return result;
}

ossim_int64 ossim::TiffHandlerState::getImageWidth(ossim_int32 directory) const
{
  ossimString tempStr;
  ossim_int64 result = 0;

  if (getValue(tempStr, directory, "image_width"))
  {
    result = tempStr.toInt64();
  }

  return result;
}

bool ossim::TiffHandlerState::getDoubleArray(std::vector<ossim_float64>& result, 
                                              ossim_int32 directory,
                                              const ossimString& key)const
{
  ossimString tempStr;

  if(getValue(tempStr, directory, key))
  {
    if(!tempStr.empty())
    {
      tempStr=tempStr.substitute(" ", ",", true);
      if(!toSimpleVector(result, "("+tempStr+")"))
      {
        result.clear();
      }
      tempStr="";
    }
  }

  return !result.empty();

}

bool ossim::TiffHandlerState::getGeoDoubleParams(std::vector<ossim_float64>& result,
                                                 ossim_int32 directory)const
{
  return getDoubleArray(result, directory, "geo_double_params");
}

bool ossim::TiffHandlerState::getGeoPixelScale(std::vector<ossim_float64>& result,
                                                 ossim_int32 directory)const
{
  return getDoubleArray(result, directory, "model_pixel_scale");
}

bool ossim::TiffHandlerState::getGeoTiePoints(std::vector<ossim_float64>& result,
                                              ossim_int32 directory)const
{
  return getDoubleArray(result, directory, "model_tie_point");
}

bool ossim::TiffHandlerState::getImageModelTransform (std::vector<ossim_float64>& result,
                                                      ossim_int32 directory)const
{
  return getDoubleArray(result, directory, "model_transform");
}

bool ossim::TiffHandlerState::getCitation(ossimString &citation, ossim_int32 directory) const
{
  bool result = true;

  if (!getValue(citation, directory, "citation"))
  {
    result = false;
  }

  return result;
}

bool ossim::TiffHandlerState::getCopyright(ossimString &copyright, ossim_int32 directory) const
{
  bool result = true;

  if (!getValue(copyright, directory, "copyright"))
  {
    result = false;
  }

  return result;
}

bool ossim::TiffHandlerState::load(const ossimKeywordlist& kwl,
                                   const ossimString& prefix)
{
  bool result = ossim::ImageHandlerState::load(kwl, prefix);
  m_tags.clear();

  kwl.extractKeysThatMatch(m_tags, "^("+prefix+"dir[0-9]+)");
  if(!prefix.empty())
  {
    m_tags.stripPrefixFromAll("^("+prefix+")");
  }
  ossimString numberOfDirectories = kwl.find(prefix, "number_of_directories");

  if(!numberOfDirectories.empty()) m_tags.add("number_of_directories", numberOfDirectories, true);

  return result;
}

bool ossim::TiffHandlerState::save(ossimKeywordlist& kwl,
                                   const ossimString& prefix)const
{
   bool result = ossim::ImageHandlerState::save(kwl, prefix);
   
   kwl.add(prefix.c_str(), m_tags);

   return result;
}
