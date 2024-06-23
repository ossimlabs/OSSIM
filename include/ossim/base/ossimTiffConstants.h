//---
//
// License: MIT
//
// Description: TIFF constants. Here for stand alone code; i.e.,
// code that does not use libtiff.
// 
//---
// $Id$

#ifndef ossimTiffConstants_HEADER
#define ossimTiffConstants_HEADER 1

// Namespaced with ossim to avoid conflicts with libtiff.
namespace ossim
{
   /** Anonymous enumerations: */
   enum
   {
      PIXEL_IS_AREA                      = 1,
      PIXEL_IS_POINT                     = 2,
      TIFFTAG_SUBFILETYPE                = 254,
      TIFFTAG_IMAGEWIDTH                 = 256,
      TIFFTAG_IMAGELENGTH                = 257,
      TIFFTAG_BITSPERSAMPLE              = 258,
      TIFFTAG_COMPRESSION                = 259,
      TIFFTAG_PHOTOMETRIC                = 262,
      TIFFTAG_FILLORDER                  = 266,
      TIFFTAG_IMAGEDESCRIPTION           = 270,
      TIFFTAG_MAKE                       = 271,
      TIFFTAG_MODEL                      = 272,
      TIFFTAG_STRIPOFFSETS               = 273,
      TIFFTAG_ORIENTATION                = 274,
      TIFFTAG_SAMPLESPERPIXEL            = 277,
      TIFFTAG_ROWSPERSTRIP               = 278,
      TIFFTAG_STRIPBYTECOUNTS            = 279,
      TIFFTAG_MINSAMPLEVALUE             = 280,
      TIFFTAG_MAXSAMPLEVALUE             = 281,
      TIFFTAG_XRESOLUTION                = 282,
      TIFFTAG_YRESOLUTION                = 283,

      TIFFTAG_PLANARCONFIG               = 284,
      TIFFTAG_PLANARCONFIG_CONTIG        = 1,
      TIFFTAG_PLANARCONFIG_SEPARATE      = 2,
      
      TIFFTAG_RESOLUTIONUNIT             = 296,
      TIFFTAG_PAGENUMBER                 = 297,
      TIFFTAG_SOFTWARE                   = 305,

      // FORMAT is YYYY:MM:DD HH:MM:SS
      TIFFTAG_DATETIME                   = 306,

      TIFFTAG_ARTIST                     = 315,
      TIFFTAG_HOSTCOMPUTER               = 316,
      TIFFTAG_PREDICTOR                  = 317,
      TIFFTAG_WHITEPOINT                 = 318,
      TIFFTAG_PRIMARYCHROMATICITIES      = 319,
      TIFFTAG_COLORMAP                   = 320,
      TIFFTAG_SUBIFD                     = 330,
      TIFFTAG_TILEWIDTH                  = 322,
      TIFFTAG_TILELENGTH                 = 323,
      TIFFTAG_TILEOFFSETS                = 324,
      TIFFTAG_TILEBYTECOUNTS             = 325,
      TIFFTAG_EXTRASAMPLES               = 338,

      TIFFTAG_SAMPLEFORMAT               = 339,
      SAMPLEFORMAT_UINT                  = 1,
      SAMPLEFORMAT_INT                   = 2,
      SAMPLEFORMAT_IEEEFP                = 3,
      SAMPLEFORMAT_VOID                  = 4,
      SAMPLEFORMAT_COMPLEXINT            = 5,
      SAMPLEFORMAT_COMPLEXIEEEFP         = 6,

      TIFFTAG_SMINSAMPLEVALUE            = 340,
      TIFFTAG_SMAXSAMPLEVALUE            = 341,
      TIFFTAG_JPEGTABLES                 = 347,

      TIFFTAG_YCBCRSUBSAMPLING           = 530,
      YCBCRSUBSAMPLEHORIZ                = 0,
      YCBCRSUBSAMPLEVERT                 = 1,

      TIFFTAG_YCBCRPOSITIONING           = 531,
      YCBCRPOSITION_CENTERED             = 1,
      YCBCRPOSITION_COSITED              = 2,

      TIFFTAG_REFERENCEBLACKWHITE        = 532,
      TIFFTAG_XMLPACKET                  = 700,


      GT_MODEL_TYPE_GEO_KEY              = 1024,
      GT_RASTER_TYPE_GEO_KEY             = 1025,
      GT_CITATION_GEO_KEY                = 1026,
      GEOGRAPHIC_TYPE_GEO_KEY            = 2048,
      GEOG_CITATION_GEO_KEY              = 2049,
      GEOG_GEODETIC_DATUM_GEO_KEY        = 2050,
      GEOG_PRIME_MERIDIAN_GEOKEY         = 2051,
      GEOG_LINEAR_UNITS_GEO_KEY          = 2052,
      GEOG_ANGULAR_UNITS_GEO_KEY         = 2054,
      GEOG_ANGULAR_UNIT_SIZE_GEO_KEY     = 2055,
      GEOG_ELLIPSOID_GEO_KEY             = 2056,
      GEOG_SEMI_MAJOR_AXIS               = 2057,
      GEOG_SEMI_MINOR_AXIS               = 2058,
      GEOG_INV_FLATTENING_GEO_KEY        = 2059,
      GEOG_PRIME_MERIDIAN_LONG_GEO_KEY   = 2061,
      PROJECTED_CS_TYPE_GEO_KEY          = 3072,
      PCS_CITATION_GEO_KEY               = 3073,
      PROJECTION_GEO_KEY                 = 3074,
      PROJ_COORD_TRANS_GEO_KEY           = 3075,
      PROJ_LINEAR_UNITS_GEO_KEY          = 3076,
      PROJ_LINEAR_UNIT_SIZE_GEO_KEY      = 3077,
      PROJ_STD_PARALLEL1_GEO_KEY         = 3078,
      PROJ_STD_PARALLEL2_GEO_KEY         = 3079,
      PROJ_NAT_ORIGIN_LONG_GEO_KEY       = 3080,
      PROJ_NAT_ORIGIN_LAT_GEO_KEY        = 3081,
      PROJ_FALSE_EASTING_GEO_KEY         = 3082,
      PROJ_FALSE_NORTHING_GEO_KEY        = 3083,
      PROJ_FALSE_ORIGIN_LONG_GEO_KEY     = 3084,
      PROJ_FALSE_ORIGIN_LAT_GEO_KEY      = 3085,
      PROJ_FALSE_ORIGIN_EASTING_GEO_KEY  = 3086,
      PROJ_FALSE_ORIGIN_NORTHING_GEO_KEY = 3087,
      PROJ_CENTER_LONG_GEO_KEY           = 3088,
      PROJ_CENTER_LAT_GEO_KEY            = 3089,
      PROJ_CENTER_EASTING_GEO_KEY        = 3090,
      PROJ_CENTER_NORTHING_GEO_KEY       = 3091,
      PROJ_SCALE_AT_NAT_ORIGIN_GEO_KEY   = 3092,
      PROJ_SCALE_AT_CENTER_GEO_KEY       = 3093,
      VERTICAL_CS_TYPE_GEO_KEY           = 4096,
      VERTICAL_CITATION_GEO_KEY          = 4097,
      VERTICAL_DATUM_GEO_KEY             = 4098,
      VERTICAL_UNITS_GEO_KEY             = 4099,
      LINEAR_METER                       = 9001,
      LINEAR_FOOT                        = 9002,
      LINEAR_FOOT_US_SURVEY              = 9003,
      ANGULAR_DEGREE                     = 9102,
      ANGULAR_ARC_MINUTE                 = 9103,
      ANGULAR_ARC_SECOND                 = 9104,
      ANGULAR_GRAD                       = 9105,
      ANGULAR_GON                        = 9106,
      ANGULAR_DMS                        = 9107,
      ANGULAR_DMS_HEMISPHERE             = 9108,
      PCS_BRITISH_NATIONAL_GRID          = 27700,
      USER_DEFINED                       = 32767,
      TIFFTAG_COPYRIGHT                  = 33432,
      MODEL_PIXEL_SCALE_TAG              = 33550,
      MODEL_TIE_POINT_TAG                = 33922,
      MODEL_TRANSFORM_TAG                = 34264,
      TIFFTAG_PHOTOSHOP                  = 34377,
      GEO_KEY_DIRECTORY_TAG              = 34735,
      GEO_DOUBLE_PARAMS_TAG              = 34736,
      GEO_ASCII_PARAMS_TAG               = 34737,
      GDAL_METADATA_TAG                  = 42112,
      GDAL_NODATA                        = 42113,
      RPC_COEFFICIENT_TAG                = 50844
   };

   enum CompressType // Tag 259
   {
      // Not a complete list but handles the most common:
      COMPRESSION_UNKNOWN       = 0,
      COMPRESSION_NONE          = 1,
      COMPRESSION_CCITT_2       = 2,
      COMPRESSION_CCITT_3       = 3,
      COMPRESSION_CCITT_4       = 4,
      COMPRESSION_LZW           = 5,
      COMPRESSION_OJPEG         = 6,
      COMPRESSION_JPEG          = 7,
      COMPRESSION_ADOBE_DEFLATE = 8,
      COMPRESSION_DEFLATE       = 32946,
      COMPRESSION_PACKBIT       = 32773
   };

   enum PhotoInterpretation
   {
      PHOTO_MINISWHITE = 0, // min value is white
      PHOTO_MINISBLACK = 1, // min value is black
      PHOTO_RGB        = 2, // RGB color model
      PHOTO_PALETTE    = 3, // color map indexed
      PHOTO_MASK       = 4, // $holdout mask
      PHOTO_SEPARATED  = 5, // !color separations
      PHOTO_YCBCR      = 6, // !CCIR 601
      PHOTO_CIELAB     = 8, // !1976 CIE L*a*b*
      PHOTO_ICCLAB     = 9,
      PHOTO_ITULAB     = 10,
      PHOTO_CFA        = 32803,
      PHOTO_LOGL       = 32844,
      PHOTO_LOGLUV     = 32845
   };

   enum ModelType
   {
      UNKNOWN               = 0,
      MODEL_TYPE_PROJECTED  = 1,  // Projection Coordinate System
      MODEL_TYPE_GEOGRAPHIC = 2,  // Geographic latitude-longitude System
      MODEL_TYPE_GEOCENTRIC = 3
   };

   enum
   {
      TIFF_NOTYPE    = 0,  /* placeholder */
      TIFF_BYTE      = 1,  /* 8-bit unsigned integer */
      TIFF_ASCII     = 2,  /* 8-bit bytes w/ last byte null */
      TIFF_SHORT     = 3,  /* 16-bit unsigned integer */
      TIFF_LONG      = 4,  /* 32-bit unsigned integer */
      TIFF_RATIONAL  = 5,  /* 64-bit unsigned fraction */
      TIFF_SBYTE     = 6,  /* !8-bit signed integer */
      TIFF_UNDEFINED = 7,  /* !8-bit untyped data */
      TIFF_SSHORT    = 8,  /* !16-bit signed integer */
      TIFF_SLONG     = 9,  /* !32-bit signed integer */
      TIFF_SRATIONAL = 10, /* !64-bit signed fraction */
      TIFF_FLOAT     = 11, /* !32-bit IEEE floating point */
      TIFF_DOUBLE    = 12, /* !64-bit IEEE floating point */
      TIFF_IFD       = 13, /* %32-bit unsigned integer (offset) */
      TIFF_LONG8     = 16, /* BigTIFF 64-bit unsigned integer */
      TIFF_SLONG8    = 17, /* BigTIFF 64-bit signed integer */
      TIFF_IFD8      = 18  /* BigTIFF 64-bit unsigned integer (offset) */
   };
   
} // End: namespace ossim

#endif /* End of "#ifndef ossimTiffConstants_HEADER" */
