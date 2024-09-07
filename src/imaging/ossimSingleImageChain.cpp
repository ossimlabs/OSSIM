//---
//
// License: MIT
//
// Author:  David Burken
//
// Description: Utility class definition for a single image chain.
// 
//---
// $Id$

#include <ossim/imaging/ossimSingleImageChain.h>

#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimGeoPolygon.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/imaging/ossimImageGeometry.h>
#include <ossim/imaging/ossimImageHandlerRegistry.h>
// #include <ossim/imaging/ossimTiledImagePatch.h>
#include <ossim/support_data/ossimSrcRecord.h>

ossimSingleImageChain::ossimSingleImageChain()
   :
   ossimImageChain(),
   m_handler(0),
   m_bandSelector(0),
   m_histogramRemapper(0),
   m_gammaRemapper(0),
   m_brightnessContrast(0),
   m_sharpen(0),
   m_scalarRemapper(0),
   m_resamplerCache(0),
   m_resampler(0),
   m_chainCache(0),
   m_addNullPixelFlipFlag(false),
   m_addHistogramFlag(false),
   m_addGammaFlag(false),
   m_addResamplerCacheFlag(false),
   m_addChainCacheFlag(false),
   m_remapToEightBitFlag(false),
   m_threeBandFlag(false),
   m_threeBandReverseFlag(false),
   m_brightnessContrastFlag(false),
   m_sharpenFlag(false),
   m_geoPolyCutterFlag(false)
{
}

ossimSingleImageChain::ossimSingleImageChain(bool addNullPixelFlipFlag,
                                             bool addHistogramFlag,
                                             bool addResamplerCacheFlag,
                                             bool addChainCacheFlag,
                                             bool remapToEightBitFlag,
                                             bool threeBandFlag,
                                             bool threeBandReverseFlag,
                                             bool brightnessContrastFlag,
                                             bool sharpenFlag,
                                             bool geoPolyCutterFlag)
   :
   ossimImageChain(),
   m_handler(0),
   m_bandSelector(0),
   m_histogramRemapper(0),
   m_gammaRemapper(0),
   m_brightnessContrast(0),
   m_sharpen(0),
   m_scalarRemapper(0),
   m_resamplerCache(0),
   m_resampler(0),
   m_chainCache(0),
   m_addNullPixelFlipFlag(addNullPixelFlipFlag),
   m_addHistogramFlag(addHistogramFlag),
   m_addResamplerCacheFlag(addResamplerCacheFlag),
   m_addChainCacheFlag(addChainCacheFlag),
   m_remapToEightBitFlag(remapToEightBitFlag),
   m_threeBandFlag(threeBandFlag),
   m_threeBandReverseFlag(threeBandReverseFlag),
   m_brightnessContrastFlag(brightnessContrastFlag),
   m_sharpenFlag(sharpenFlag),
   m_geoPolyCutterFlag(geoPolyCutterFlag)
{
}

ossimSingleImageChain::~ossimSingleImageChain()
{
   m_handler            = 0;
   m_nullPixelFlip      = 0;
   m_bandSelector       = 0;
   m_histogramRemapper  = 0;
   m_gammaRemapper      = 0;
   m_brightnessContrast = 0;
   m_sharpen            = 0;
   m_scalarRemapper     = 0;
   m_resamplerCache     = 0;
   m_resampler          = 0;
   m_chainCache         = 0;
   m_geoPolyCutter      = 0;

}

void ossimSingleImageChain::reset()
{
   bool result = true;
   do
   {
      result = deleteLast();
   } while (result);

   m_handler                = 0;
   m_bandSelector           = 0;
   m_nullPixelFlip          = 0;
   m_histogramRemapper      = 0;
   m_gammaRemapper          = 0;
   m_brightnessContrast     = 0;
   m_sharpen                = 0;
   m_scalarRemapper         = 0;
   m_resamplerCache         = 0;
   m_resampler              = 0;
   m_geoPolyCutter          = 0;
   m_chainCache             = 0;

   m_addHistogramFlag       = false;
   m_addGammaFlag           = false;
   m_addNullPixelFlipFlag   = false;
   m_addResamplerCacheFlag  = false;
   m_addChainCacheFlag      = false;
   m_remapToEightBitFlag    = false;
   m_threeBandFlag          = false;
   m_threeBandReverseFlag   = false;
   m_brightnessContrastFlag = false;
   m_sharpenFlag            = false;
   m_geoPolyCutterFlag      = false;
}

void ossimSingleImageChain::close()
{
   if ( m_handler.valid() )
   {
      if ( removeChild(m_handler.get()) )
      {
         m_handler = 0;
      }
   }
}

ossimFilename ossimSingleImageChain::getFilename() const
{
   ossimFilename result;
   if ( m_handler.valid() )
   {
      result = m_handler->getFilename();
   }
   return result;
}

bool ossimSingleImageChain::open(const ossimFilename& file, bool openOverview)
{
   return addImageHandler(file, openOverview);
}

bool ossimSingleImageChain::isOpen() const
{
   return m_handler.valid();
}

bool ossimSingleImageChain::open(const ossimSrcRecord& src)
{
   return addImageHandler(src);
}

void ossimSingleImageChain::createRenderedChain()
{
   // Band selector after image handler only if needed.
   if ( m_handler.valid() )
   {
      // Only add if multiple bands.
      if ( m_handler->getNumberOfOutputBands() != 1 )
      {
         addBandSelector();
         if ( m_threeBandReverseFlag )
         {
            setToThreeBandsReverse();
         }
      }
   }
   else // No image handler so just add it.
   {
      addBandSelector();
   }

   // will do this here.  Not sure if we want flipped just before the resampler or if we want to do it here.
   // I think this is a better place for you can now manipulate the flipped pixel   
   if (m_addNullPixelFlipFlag)
   {
         addNullPixelFlip();
   }

   // histogram:
   if ( m_addHistogramFlag )
   {
      addHistogramRemapper();
   }

   if(m_addGammaFlag)
   {
      addGammaRemapper();
   }

   // brightness contrast:
   if ( m_brightnessContrastFlag )
   {
      addBrightnessContrast();
   }

   // sharpen filter:
   if ( m_sharpenFlag )
   {
      addSharpen();
   }

   // scalar remapper
   if ( m_remapToEightBitFlag )
   {
      if ( m_handler.valid() )
      {
         // See if it's eight bit.
         if (m_handler->getOutputScalarType() != OSSIM_UINT8)
         {
            addScalarRemapper();
         }
      }
      else
      {
         // Just add...
         addScalarRemapper(); 
      }
   }

   // resampler cache
   if ( m_addResamplerCacheFlag )
   {
      m_resamplerCache = addCache();

      // If input image is tiled set the cache input tile size to that.
      if ( m_handler.valid() )
      {
         if ( m_handler->isImageTiled() )
         {
            ossimIpt inputImageTileSize;
            inputImageTileSize.x = (ossim_int32)m_handler->getImageTileWidth();
            inputImageTileSize.y = (ossim_int32)m_handler->getImageTileHeight();
            if ( m_resamplerCache.valid() )
            {
               m_resamplerCache->setTileSize( inputImageTileSize );
            }
         }
      }
   }

#if 0 /* test code - drb */
   ossimRefPtr<ossimTiledImagePatch> tp = new ossimTiledImagePatch();
   if ( m_handler.valid() )
   {
      if ( m_handler->isImageTiled() )
      {
         ossimIpt inputImageTileSize;
         inputImageTileSize.x = (ossim_int32)m_handler->getImageTileWidth();
         inputImageTileSize.y = (ossim_int32)m_handler->getImageTileHeight();
         tp->setInputTileSize( inputImageTileSize );
      }
   }
   addFirst( tp.get() );
#endif   
   
   // resampler
   addResampler();

#if 0 /* test code - drb */
   ossimRefPtr<ossimTiledImagePatch> tp2 = new ossimTiledImagePatch();
   ossimIpt inputImageTileSize(64, 64);
   tp2->setInputTileSize( inputImageTileSize );
   addFirst( tp2.get() );
#endif   

   //---
   // Do this here so that if a band selector is added to the end of the
   // chain it will go in before the end of chain cache.
   //---
   if (m_threeBandFlag)
   {
      if (!m_bandSelector)
      {
         addBandSelector();
      }
      setToThreeBands();
   }

   //---
   // Ditto...
   //---
   if ( m_threeBandReverseFlag && !m_bandSelector )
   {
      addBandSelector();
      setToThreeBandsReverse();
   }

   // Put the geo cutter just before the cache.
   if(m_geoPolyCutterFlag)
   {
      addGeoPolyCutter();
   }

   // End of chain cache.
   if ( m_addChainCacheFlag )
   {
      m_chainCache = addCache();
   }

   initialize();

}

void ossimSingleImageChain::createRenderedChain(const ossimSrcRecord& src)
{
   // Band selector after image handler only if needed.
   if ( m_handler.valid() )
   {
      // Only add if needed.
      if ( ( m_handler->getNumberOfOutputBands() != 1 ) || src.getBands().size() )
      {
         addBandSelector(src);
         if ( m_threeBandReverseFlag )
         {
            setToThreeBandsReverse();
         }
      }
   }
   else // No image handler so just add it.
   {
      addBandSelector(src);
   }
   
   if(m_addNullPixelFlipFlag)
   {
      addNullPixelFlip();
   }
   // histogram
   if ( m_addHistogramFlag || src.getHistogramOp().size() )
   {
      addHistogramRemapper(src);
   }

   if(m_addGammaFlag)
   {
      addGammaRemapper();
   }
   // brightness contrast:
   if ( m_brightnessContrastFlag )
   {
      addBrightnessContrast();
   }

   // sharpen filter:
   if ( m_sharpenFlag )
   {
      addSharpen();
   }

   // scalar remapper
   if ( m_remapToEightBitFlag )
   {
      if ( m_handler.valid() )
      {
         // See if it's eight bit.
         if (m_handler->getOutputScalarType() != OSSIM_UINT8)
         {
            addScalarRemapper();
         }
      }
      else
      {
         // Just add...
         addScalarRemapper(); 
      }
   }

   // resampler cache
   if ( m_addResamplerCacheFlag )
   {
      m_resamplerCache = addCache();
   }
   
   // resampler
   addResampler();

   //---
   // Do this here so that if a band selector is added to the end of the
   // chain it will go in before the end of chain cache.
   //---
   if (m_threeBandFlag)
   {
      if (!m_bandSelector) // Input must be one band.
      {
         addBandSelector(src);
      }
      setToThreeBands();
   }

   //---
   // Ditto...
   //---
   if ( m_threeBandReverseFlag && !m_bandSelector )
   {
      addBandSelector();
      setToThreeBandsReverse();
   }

   // End of chain cache.
   if ( m_addChainCacheFlag )
   {
      m_chainCache = addCache();
   }

   initialize();

}

bool ossimSingleImageChain::addImageHandler(const ossimFilename& file, bool openOverview)
{
   bool result = false;

   close();
   
   // m_handler = ossimImageHandlerRegistry::instance()->open(file, true, openOverview);
   m_handler = ossimImageHandlerRegistry::instance()->openConnection(file, openOverview);
   
   if ( m_handler.valid() )
   {
      // Add to the chain.  Note: last is really first.
      addLast( m_handler.get() );
      
      result = true;
   }

   return result;
}

bool ossimSingleImageChain::addImageHandler(const ossimSrcRecord& src)
{
   bool result = false;

   close();
   
   // m_handler = ossimImageHandlerRegistry::instance()->open(file, true, openOverview);
   m_handler = ossimImageHandlerRegistry::instance()->open(src.getAttributesKwl());
   
   if ( m_handler.valid() )
   {
      // Add to the chain.  Note: last is really first.
      addLast( m_handler.get() );
      
      result = true;
   }

   return result;

//   bool result = addImageHandler( src.getFilename() );
#if 0  
   if (result)
   {
      //---
      // When loading from ossimSrcRecord typically the overview/histograms are
      // not in the same directory and the "support" keyword is not set.  For
      // the ossimImageHandler::getFilenameWithThisExtension to work correctly
      // the ossimImageHandler::theSupplementaryDirectory must be set.
      // So if the ossimSrcRecord::getSupportDir() is empty and the overview
      // or histogram is not co-located with the image we will set it here.
      //---
      ossimFilename supportDir = src.getSupportDir();
      if ( supportDir.empty() )
      {
         if ( src.getOverviewPath().size() )
         {
            if ( src.getOverviewPath().isDir() )
            {
               supportDir = src.getOverviewPath();
            }
            else
            {
               supportDir = src.getOverviewPath().path();
            }
         }
         else if ( src.getHistogramPath().size() )
         {
            if ( src.getHistogramPath().isDir() )
            {
               supportDir = src.getHistogramPath();
            }
            else
            {
               supportDir = src.getHistogramPath().path();
            }
         }
         else if ( src.getMaskPath().size() )
         {
            if ( src.getMaskPath().isDir() )
            {
               supportDir = src.getMaskPath();
            }
            else
            {
               supportDir = src.getMaskPath().path();
            }
         }
      }
      if ( supportDir.size() && (src.getFilename().path() != supportDir) )
      {
         m_handler->setSupplementaryDirectory( supportDir );
      }
      if ( src.getEntryIndex() > 0 ) // defaulted to -1.
      {
         m_handler->setCurrentEntry( static_cast<ossim_uint32>( src.getEntryIndex() ) );
      }
      if ( m_handler->getOverview() == 0 )
      {
         if ( src.getOverviewPath().size() )
         {
            m_handler->openOverview( src.getOverviewPath() );
         }
         else
         {
            ossimFilename ovrFile = m_handler->getFilenameWithThisExtension(ossimString(".ovr"));
            m_handler->openOverview( ovrFile ); 
         }
      }
   }
   #endif
   return result;
}

void ossimSingleImageChain::addBandSelector()
{
   if (!m_bandSelector)
   {
      m_bandSelector = new ossimBandSelector();

      // Add to the end of the chain.
      addFirst(m_bandSelector.get());
   }
}

void ossimSingleImageChain::addBandSelector(const ossimSrcRecord& src)
{
   if (!m_bandSelector)
   {
      m_bandSelector = new ossimBandSelector();
      
      // Add to the end of the chain.
      addFirst(m_bandSelector.get());
   }
   if ( src.getBands().size() )
   {
      m_bandSelector->setOutputBandList( src.getBands() );
   }
}

void ossimSingleImageChain::addNullPixelFlip()
{
   if(!m_nullPixelFlip)
   {
      m_nullPixelFlip = new ossimNullPixelFlip();
      addFirst(m_nullPixelFlip.get());
   }
}
void ossimSingleImageChain::addNullPixelFlip(const ossimSrcRecord& src)
{
   if(!m_nullPixelFlip)
   {
      m_nullPixelFlip = new ossimNullPixelFlip();
      addFirst(m_nullPixelFlip.get());
   }
}

void ossimSingleImageChain::addHistogramRemapper()
{
   if (!m_histogramRemapper)
   {
      m_histogramRemapper = new ossimHistogramRemapper();
      
      m_histogramRemapper->setEnableFlag(false);

      // Add to the end of the chain.
      addFirst(m_histogramRemapper.get());
   } 
}

void ossimSingleImageChain::addHistogramRemapper(const ossimSrcRecord& src)
{
   static const char MODULE[] =
      "ossimSingleImageChain::addHistogramRemapper(const ossimSrcRecord&)";
   
   if (!m_histogramRemapper)
   {
      m_histogramRemapper = new ossimHistogramRemapper();
      
      m_histogramRemapper->setEnableFlag(false);

      // Add to the end of the chain.
      addFirst(m_histogramRemapper.get());
   }

   if ( src.getHistogramOp().size() && m_handler.valid() )
   {
      // Create histogram code here???
      
      // Open the histogram if needed.
      if ( m_histogramRemapper->getHistogramFile() == ossimFilename::NIL )
      {
         ossimFilename f;
         if ( src.getSupportDir().size() )
         {
            f = src.getSupportDir();
            f.dirCat( m_handler->getFilename().fileNoExtension() );
            f.setExtension(".his");
         }
         else
         {
            f = m_handler->getFilenameWithThisExtension( ossimString("his") );
         }
         if ( m_histogramRemapper->openHistogram( f ) == false )
         {
            ossimNotify(ossimNotifyLevel_WARN)
               << MODULE << "\nCould not open:  " << f << "\n";
         }
      }

      // Set the histogram strech mode.
      if ( src.getHistogramOp().size() )
      {
         // Enable.
         m_histogramRemapper->setEnableFlag(true);
         
         // Set the histo mode:
         ossimString op = src.getHistogramOp();
         op.downcase();
         if ( op == "auto-minmax" )
         {
            m_histogramRemapper->setStretchMode( ossimHistogramRemapper::LINEAR_AUTO_MIN_MAX );
         }
         else if ( (op == "std-stretch-1") || (op == "std-stretch 1") )
         {
            m_histogramRemapper->setStretchMode( ossimHistogramRemapper::LINEAR_1STD_FROM_MEAN );
         } 
         else if ( (op == "std-stretch-2") || (op == "std-stretch 2") )
         {
            m_histogramRemapper->setStretchMode( ossimHistogramRemapper::LINEAR_2STD_FROM_MEAN );
         } 
         else if ( (op == "std-stretch-3") || (op == "std-stretch 3") )
         {
            m_histogramRemapper->setStretchMode( ossimHistogramRemapper::LINEAR_3STD_FROM_MEAN );
         }
         else
         {
            m_histogramRemapper->setEnableFlag(false);
            ossimNotify(ossimNotifyLevel_WARN)
               << MODULE << "\nUnhandled operation: " << op << "\n";
         }
      }
      
   } // End: if ( src.setHistogramOp().size() && m_handler.valid() )
}

ossimRefPtr<ossimCacheTileSource> ossimSingleImageChain::addCache()
{
   ossimRefPtr<ossimCacheTileSource> cache = new ossimCacheTileSource();

   // Add to the end of the chain.
   addFirst(cache.get());

   return cache;
}

void ossimSingleImageChain::addResampler()
{
   if ( !m_resampler )
   {
      m_resampler = new ossimImageRenderer();

      // Add to the end of the chain.
      addFirst(m_resampler.get());
   }
}
void ossimSingleImageChain::addRenderer()
{
   addResampler();
}

void ossimSingleImageChain::addScalarRemapper()
{
   if ( !m_scalarRemapper )
   {
      m_scalarRemapper = new ossimScalarRemapper();

      if ( m_resamplerCache.valid() )
      {
         // Add to the left of the resampler cache.
         insertLeft(m_scalarRemapper.get(), m_resamplerCache.get());
      }
      else
      {
         // Add to the end of the chain.
         addFirst(m_scalarRemapper.get());
      }
   }
}

void ossimSingleImageChain::addGammaRemapper()
{
   if ( !m_gammaRemapper )
   {
      m_gammaRemapper = new ossimGammaRemapper();

      // Add to the end of the chain.
      addFirst( m_gammaRemapper.get() );
   }
}

void ossimSingleImageChain::addGammaRemapper(const ossimSrcRecord& src)
{
   if ( !m_gammaRemapper )
   {
      m_gammaRemapper = new ossimGammaRemapper();

      m_gammaRemapper->setGamma(src.getGamma());
      // Add to the end of the chain.
      addFirst( m_gammaRemapper.get() );
   }
}


void ossimSingleImageChain::addBrightnessContrast()
{
   if ( !m_brightnessContrast )
   {
      m_brightnessContrast = new ossimBrightnessContrastSource();

      // Add to the end of the chain.
      addFirst( m_brightnessContrast.get() );
   }
}

void ossimSingleImageChain::addSharpen()
{
   if ( !m_sharpen )
   {
      m_sharpen = new ossimImageSharpenFilter();

      // Add to the end of the chain.
      addFirst( m_sharpen.get() );
   }
}

void ossimSingleImageChain::addGeoPolyCutter()
{
   if(!m_geoPolyCutter.valid())
   {
      m_geoPolyCutter = new ossimGeoPolyCutter();

      //---
      // ossimGeoPolyCutter requires a view geometry to transform the points.
      // So set prior to adding to end of chain if valid.  If not, user is
      // responsible for setting.
      //---
      ossimRefPtr<ossimImageGeometry> geom = getImageGeometry();
      if ( geom.valid() )
      {
         m_geoPolyCutter->setView( geom.get() );
      }
      
      addFirst( m_geoPolyCutter.get() );
   }
}

void ossimSingleImageChain::addGeoPolyCutterPolygon(const std::vector<ossimGpt>& polygon)
{
   if(!m_geoPolyCutter.valid())
   {
      addGeoPolyCutter();
   }
   
   // sanity check to verify that the add did not fail
   if(m_geoPolyCutter.valid())
   {
      m_geoPolyCutter->addPolygon(polygon);   
   }
}

void ossimSingleImageChain::addGeoPolyCutterPolygon(const ossimGeoPolygon& polygon)
{
   if(!m_geoPolyCutter.valid())
   {
      addGeoPolyCutter();
   }
   
   // sanity check to verify that the add did not fail
   if(m_geoPolyCutter.valid())
   {
      m_geoPolyCutter->addPolygon(polygon);
   }
}

ossimRefPtr<const ossimImageHandler> ossimSingleImageChain::getImageHandler() const
{
   return ossimRefPtr<const ossimImageHandler>( m_handler.get() );
}

ossimRefPtr<ossimImageHandler> ossimSingleImageChain::getImageHandler()
{
   return m_handler;
}

ossimRefPtr<const ossimBandSelector> ossimSingleImageChain::getBandSelector() const
{
   return ossimRefPtr<const ossimBandSelector>( m_bandSelector.get() );
}

ossimRefPtr<const ossimNullPixelFlip> ossimSingleImageChain::getNullPixelFlip() const
{
   return ossimRefPtr<const ossimNullPixelFlip>( m_nullPixelFlip.get() );
}


ossimRefPtr<ossimBandSelector> ossimSingleImageChain::getBandSelector()
{
   return m_bandSelector;
}

ossimRefPtr<const ossimHistogramRemapper> ossimSingleImageChain::getHistogramRemapper() const
{
   return ossimRefPtr<const ossimHistogramRemapper>( m_histogramRemapper.get() );
}

ossimRefPtr<ossimHistogramRemapper> ossimSingleImageChain::getHistogramRemapper()
{
   return m_histogramRemapper;
}

ossimRefPtr<const ossimGammaRemapper> ossimSingleImageChain::getGammaRemapper() const
{
   return ossimRefPtr<const ossimGammaRemapper>( m_gammaRemapper.get() );
}

ossimRefPtr<ossimGammaRemapper> ossimSingleImageChain::getGammaRemapper()
{
   return m_gammaRemapper;
}


ossimRefPtr<const ossimCacheTileSource> ossimSingleImageChain::getResamplerCache() const
{
   return ossimRefPtr<const ossimCacheTileSource>( m_resamplerCache.get() );
}

ossimRefPtr<ossimCacheTileSource> ossimSingleImageChain::getResamplerCache()
{
   return m_resamplerCache;
}

ossimRefPtr<const ossimImageRenderer> ossimSingleImageChain::getImageRenderer() const
{
   return ossimRefPtr<const ossimImageRenderer>( m_resampler.get() );
}

ossimRefPtr<ossimImageRenderer> ossimSingleImageChain::getImageRenderer()
{
   return m_resampler;
}

ossimRefPtr<const ossimScalarRemapper> ossimSingleImageChain::getScalarRemapper() const
{
   return ossimRefPtr<const ossimScalarRemapper>( m_scalarRemapper.get() );
}

ossimRefPtr<ossimScalarRemapper> ossimSingleImageChain::getScalarRemapper()
{
   return m_scalarRemapper;
}

ossimRefPtr<const ossimBrightnessContrastSource>
ossimSingleImageChain::getBrightnessContrast() const
{
   return ossimRefPtr<const ossimBrightnessContrastSource>( m_brightnessContrast.get() );
}

ossimRefPtr<ossimBrightnessContrastSource> ossimSingleImageChain::getBrightnessContrast()
{
   return m_brightnessContrast;
}

ossimRefPtr<const ossimImageSharpenFilter>
ossimSingleImageChain::getSharpenFilter() const
{
   return ossimRefPtr<const ossimImageSharpenFilter>( m_sharpen.get() );
}

ossimRefPtr<ossimImageSharpenFilter> ossimSingleImageChain::getSharpenFilter()
{
   return m_sharpen;
}

ossimRefPtr<const ossimCacheTileSource> ossimSingleImageChain::getChainCache() const
{
   return ossimRefPtr<const ossimCacheTileSource>( m_chainCache.get() );
}

ossimRefPtr<ossimCacheTileSource> ossimSingleImageChain::getChainCache()
{
   return m_chainCache;
}


void ossimSingleImageChain::setAddNullPixelFlipFlag(bool flag)
{
   m_addNullPixelFlipFlag = true;
}

bool ossimSingleImageChain::getNullPixelFlipFlag() const
{
   return m_addNullPixelFlipFlag;
}

void ossimSingleImageChain::setAddHistogramFlag(bool flag)
{
   m_addHistogramFlag = flag;
}

bool ossimSingleImageChain::getAddHistogramFlag() const
{
   return m_addHistogramFlag;
}

void ossimSingleImageChain::setAddGammaFlag(bool flag)
{
   m_addGammaFlag = flag;
}

bool ossimSingleImageChain::getAddGammaFlag() const
{
   return m_addGammaFlag;
}

void ossimSingleImageChain::setAddResamplerCacheFlag(bool flag)
{
   m_addResamplerCacheFlag = flag;
}

bool ossimSingleImageChain::getAddResamplerCacheFlag() const
{
   return m_addResamplerCacheFlag;
}

void ossimSingleImageChain::setAddChainCacheFlag(bool flag)
{
   m_addChainCacheFlag = flag;
}

bool ossimSingleImageChain::getAddChainCacheFlag() const
{
   return m_addChainCacheFlag;
}

void ossimSingleImageChain::setRemapToEightBitFlag(bool flag)
{
   m_remapToEightBitFlag = flag;
}

bool ossimSingleImageChain::getRemapToEightBitFlag() const
{
   return m_remapToEightBitFlag;
}

void ossimSingleImageChain::setThreeBandFlag(bool flag)
{
   m_threeBandFlag = flag;
}

bool ossimSingleImageChain::getThreeBandFlag() const
{
   return m_threeBandFlag;
}
   
void ossimSingleImageChain::setThreeBandReverseFlag(bool flag)
{
   m_threeBandReverseFlag = flag;
}

bool ossimSingleImageChain::getThreeBandReverseFlag() const
{
   return m_threeBandReverseFlag;
}

void ossimSingleImageChain::setBrightnessContrastFlag(bool flag)
{
   m_brightnessContrastFlag = flag;
}

bool ossimSingleImageChain::getBrightnessContrastFlag() const
{
   return m_brightnessContrastFlag;
}

void ossimSingleImageChain::setSharpenFlag(bool flag)
{
   m_sharpenFlag = flag;
}

bool ossimSingleImageChain::getSharpenFlag() const
{
   return m_sharpenFlag;
}

void ossimSingleImageChain::setToThreeBands()
{
   if (!m_bandSelector)
   {
      addBandSelector();
   }

   m_bandSelector->setEnableFlag(true);
   m_bandSelector->setThreeBandRgb();

   if ( m_histogramRemapper.valid() )
   {
      m_histogramRemapper->initialize();
   } 

#if 0
   if ( m_handler.valid() )
   {
      // Only do if not three bands already so the band list order is not wiped out.
      if ( !m_bandSelector.valid() ||
           ( m_bandSelector.valid() &&
             ( m_bandSelector->getNumberOfOutputBands() != 3 ) ) )
      {
         std::vector<ossim_uint32> bandList(3);

         if ( m_handler->getRgbBandList( bandList ) == false )
         {
            const ossim_uint32 BANDS = m_handler->getNumberOfInputBands();
            if(BANDS >= 3)
            {
               bandList[0] = 0;
               bandList[1] = 1;
               bandList[2] = 2;
            }
            else
            {
               bandList[0] = 0;
               bandList[1] = 0;
               bandList[2] = 0;
            }
         }
         setBandSelection(bandList);
      }
   }
#endif
}

void ossimSingleImageChain::setToThreeBandsReverse()
{
   if ( m_handler.valid() )
   {
      std::vector<ossim_uint32> bandList(3);
      const ossim_uint32 BANDS = m_handler->getNumberOfInputBands();
      if(BANDS >= 3)
      {
         bandList[0] = 2;
         bandList[1] = 1;
         bandList[2] = 0;
      }
      else
      {
         bandList[0] = 0;
         bandList[1] = 0;
         bandList[2] = 0;
      }
      setBandSelection(bandList);
   }
}

void ossimSingleImageChain::setBandSelection(
   const std::vector<ossim_uint32>& bandList)
{
   if (!m_bandSelector)
   {
      addBandSelector();
   }
   m_bandSelector->setEnableFlag(true);
   m_bandSelector->setOutputBandList(bandList);
   if ( m_histogramRemapper.valid() )
   {
      m_histogramRemapper->initialize();
   }
}

void ossimSingleImageChain::setDefaultBandSelection()
{
   if(!m_bandSelector)
   {
      addBandSelector();
   }
   m_bandSelector->setEnableFlag(true);

   if(m_bandSelector.valid())
   {
      m_bandSelector->setThreeBandRgb();
   }
}

ossimScalarType ossimSingleImageChain::getImageHandlerScalarType() const
{
   ossimScalarType result = OSSIM_SCALAR_UNKNOWN;
   if ( m_handler.valid() )
   {
      result = m_handler->getOutputScalarType();
   }
   return result;
}

bool ossimSingleImageChain::openHistogram( ossimHistogramRemapper::StretchMode mode )
{
   bool result = false;
   ossimRefPtr<ossimImageHandler> ih = getImageHandler();
   if ( ih.valid() )
   {
      ossimRefPtr<ossimHistogramRemapper> hr = getHistogramRemapper();
      if ( hr.valid() )
      {
         ossimFilename histoFile = ih->getFilenameWithThisExtension( ossimString("his") );
         bool openedHistogram = hr->openHistogram( histoFile );
         if ( !openedHistogram && (ih->getNumberOfEntries() > 1) &&
              ( ih->getCurrentEntry() == 0 ) )
         {
            //---
            // Check for filename with no entry("e0") in the name if current entry
            // is 0 and we're multi entry. This handles the case of NITF with
            // an image and cloud entry, assuming an existing dot histogram
            // belongs to entry zero. Example:
            // NITF file:      5V090205P0001912264B220000100282M_001508507.ntf
            // Histogram file: 5V090205P0001912264B220000100282M_001508507.his
            //---            
            ih->getFilenameWithThisExt( ossimString(".his"), histoFile );
            std::cout << "histoFile 2: " << histoFile << std::endl;
            openedHistogram = hr->openHistogram( histoFile );
         }

         if ( openedHistogram )
         {
            // Enable:
            hr->setEnableFlag(true);
            
            // Set the mode:
            hr->setStretchMode( mode );
            
            result = true;
         }
      }
   }
   return result;
}
