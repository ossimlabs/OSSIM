//---
//
// File: ossimImageElevationDatabase.cpp
// 
// License: MIT
// 
// Author:  David Burken
//
// Description:  See class desciption in header file.
// 
//---
// $Id$

#include <ossim/elevation/ossimImageElevationDatabase.h>
#include <ossim/base/ossimDpt.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimString.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/elevation/ossimImageElevationHandler.h>
#include <ossim/util/ossimFileWalker.h>
#include <cmath>
#include <ostream>

#define TRACE_TIME 0 /* For function level time stats. */
#if TRACE_TIME
#  include <ossim/base/ossimStopwatch.h>
#endif

static const std::string ELEV_CELL_MAP = "elev_cell_map.kwl";

static ossimTrace traceDebug(ossimString("ossimImageElevationDatabase:debug"));

RTTI_DEF1(ossimImageElevationDatabase, "ossimImageElevationDatabase", ossimElevationCellDatabase);

ossimImageElevationDatabase::ossimImageElevationDatabase()
   :
   ossimElevationCellDatabase(),
   ossimFileProcessorInterface(),
   m_entryMap(),
   m_lastMapKey(0),
   m_lastAccessedId(0)
{
}

// Protected destructor as this is derived from ossimRefenced.
ossimImageElevationDatabase::~ossimImageElevationDatabase()
{
}

bool ossimImageElevationDatabase::open(const ossimString& connectionString)
{
   static const char M[] = "ossimImageElevationDatabase::open";
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << M << " entered...\n"
         << "\nConnection string: " << connectionString << "\n";
   }                   
   
   bool result = false;

   close();

   if ( connectionString.size() )
   {
      m_connectionString = connectionString.c_str();

      if ( loadMapFromKwl() == false )
      {
         loadFileMap();
      }

      if ( m_entryMap.size() )
      {
         result = true;
      }
      else
      {
         m_connectionString.clear();
      }
   }
   
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << M << " result=" << (result?"true\n":"false\n");
   }

   return result;
}

void ossimImageElevationDatabase::close()
{
   m_meanSpacing = 0.0;
   m_geoid = 0;
   m_connectionString.clear();
}

double ossimImageElevationDatabase::getHeightAboveMSL(const ossimGpt& gpt)
{
   double h = ossim::nan();
   if(isSourceEnabled())
   {
      ossimRefPtr<ossimElevCellHandler> handler = getOrCreateCellHandler(gpt);
      if(handler.valid())
      {
         h = handler->getHeightAboveMSL(gpt); // still need to shift

         // Save the elev source's post spacing as the database's mean spacing:
         // m_meanSpacing = handler->getMeanSpacingMeters();
      }
   }

   return h;
}

double ossimImageElevationDatabase::getHeightAboveEllipsoid(const ossimGpt& gpt)
{
   double h = getHeightAboveMSL(gpt);
   if(!ossim::isnan(h))
   {
      h += getOffsetFromEllipsoid(gpt);
   }
   return h;
}

ossimRefPtr<ossimElevCellHandler> ossimImageElevationDatabase::createCell(
   const ossimGpt& gpt)
{
   ossimRefPtr<ossimElevCellHandler> result = 0;
   
   // Need to disable elevation while loading the DEM image to prevent recursion:
   disableSource();

   std::map<ossim_uint64, ossimImageElevationFileEntry>::iterator i = m_entryMap.begin();
   while ( i != m_entryMap.end() )
   {
      if ( (*i).second.m_loadedFlag == false )
      {
         // not loaded
         ossimRefPtr<ossimImageElevationHandler> h = new ossimImageElevationHandler();

         if ( (*i).second.m_rect.isLonLatNan() )
         {
            if ( h->open( (*i).second.m_file ) )
            {
               // First time opened.  Capture the rectangle. for next time.
               (*i).second.m_rect = h->getBoundingGndRect();

               if ( ossim::isnan(m_meanSpacing) )
               {
                  // Save the elev source's post spacing as the database's mean spacing:
                  m_meanSpacing = h->getMeanSpacingMeters();
               }
            }
            else
            {
               ossimNotify(ossimNotifyLevel_WARN)
                  << "ossimImageElevationDatabase::createCell WARN:\nCould not open: "
                  << (*i).second.m_file << "\nRemoving file from map!" << std::endl;

               // Get a copy of the iterator to delet.
               std::map<ossim_uint64, ossimImageElevationFileEntry>::iterator badIter = i;
               
               ++i; // Go to next image.

               // Must put lock around erase.
               m_cacheMapMutex.lock();
               m_entryMap.erase(badIter);
               m_cacheMapMutex.unlock();
               
               continue; // Skip the rest of this loop.
            }
         }

         // Check the North up bounding rectangle for intersect.
         if ( (*i).second.m_rect.pointWithin(gpt) )
         {
            if ( h->isOpen() == false )
            {
               h->open( (*i).second.m_file );
            }

            if ( h->isOpen() )
            {
               //---
               // Check point coverage again as image may not be geographic and pointHasCoverage
               // has a check on worldToLocal point.
               //---
               if (  h->pointHasCoverage(gpt) )
               {
                  m_lastAccessedId = (*i).first;
                  (*i).second.m_loadedFlag = true;
                  result = h.get();
                  break;
               }
               else
               {
                  h = 0;
               }
            }
         }
         else
         {
            h = 0;
         }
      }

      ++i;
   }
   
   enableSource();
   return result;
}

ossimRefPtr<ossimElevCellHandler> ossimImageElevationDatabase::getOrCreateCellHandler(
   const ossimGpt& gpt)
{
   ossimRefPtr<ossimElevCellHandler> result = 0;
   
   // Note: Must do mutex lock / unlock around any cach map access.
   m_cacheMapMutex.lock();

   if ( m_cacheMap.size() )
   {
      //---
      // Look in existing map for handler.
      //
      // Note: Cannot key off of id from gpt as cells can be any arbituary dimensions.
      //---

      CellMap::iterator lastAccessedCellIter = m_cacheMap.find(m_lastAccessedId);
      CellMap::iterator iter = lastAccessedCellIter;
        
      // Check from last accessed to end.
      while ( iter != m_cacheMap.end() )
      {
         if ( iter->second->m_handler->pointHasCoverage(gpt) )
         {
            result = iter->second->m_handler.get();
            break;
         }
         ++iter;
      }
     
      if ( result.valid() == false )
      {
         iter = m_cacheMap.begin();
              
         // Beginning to last accessed.
         while ( iter != lastAccessedCellIter)
         {
            if ( iter->second->m_handler->pointHasCoverage(gpt) )
            {
               result = iter->second->m_handler.get();
               break;
            }
            ++iter;
         }
      }

      if ( result.valid() )
      {
         m_lastAccessedId  = iter->second->m_id;
         iter->second->updateTimestamp();
      }
   }
   m_cacheMapMutex.unlock();
  
   if ( result.valid() == false )
   {
      // Not in m_cacheMap.  Create a new cell for point if we have coverage.
      result = createCell(gpt);

      if(result.valid())
      {
         std::lock_guard<std::mutex> lock(m_cacheMapMutex);

         //---
         // Add the cell to map.
         // NOTE: ossimImageElevationDatabase::createCell sets m_lastAccessedId to that of
         // the entries map key.
         //---
         m_cacheMap.insert(std::make_pair(m_lastMapKey,
                                          new CellInfo(m_lastMapKey, result.get())));

         ++m_lastMapKey;

         // Check the map size and purge cells if needed.
         if(m_cacheMap.size() > m_maxOpenCells)
         {
            flushCacheToMinOpenCells();
         }
      }
   }

   return result;
}

bool ossimImageElevationDatabase::pointHasCoverage(const ossimGpt& gpt) const
{
   //---
   // NOTE:
   //
   // The bounding rect is the North up rectangle.  So if the underlying image projection is not
   // a geographic projection and there is a rotation this could return false positives.  Inside
   // the ossimImageElevationDatabase::createCell there is a call to
   // ossimImageElevationHandler::pointHasCoverage which does a real check from the
   // ossimImageGeometry of the image.
   //---
   bool result = false;
   std::map<ossim_uint64, ossimImageElevationFileEntry>::const_iterator i = m_entryMap.begin();
   while ( i != m_entryMap.end() )
   {
      if ( (*i).second.m_rect.pointWithin(gpt) )
      {
         result = true;
         break;
      }
      ++i;
   }
   return result;
}

void ossimImageElevationDatabase::getBoundingRect(ossimGrect& rect) const
{
   // The bounding rect is the North up rectangle.  So if the underlying image projection is not
   // a geographic projection and there is a rotation this will include null coverage area.
   rect.makeNan();
   std::map<ossim_uint64, ossimImageElevationFileEntry>::const_iterator i = m_entryMap.begin();
   ossimGrect subRect;
   while ( i != m_entryMap.end() )
   {
      subRect = i->second.m_rect;
      if (subRect.isLonLatNan())
      {
         // The DEM source was not yet initialized:
         ossimRefPtr<ossimImageElevationHandler> h = new ossimImageElevationHandler();
         if ( h->open( i->second.m_file ) )
            subRect = h->getBoundingGndRect();
         else
         {
            ++i;
            continue;
         }
      }
      if (rect.isLonLatNan())
         rect = subRect;
      else
         rect = rect.combine(subRect);
      ++i;
   }
}


bool ossimImageElevationDatabase::getAccuracyInfo(ossimElevationAccuracyInfo& info, const ossimGpt& gpt) const
{
   if(pointHasCoverage(gpt))
   {
     info.m_surfaceName = "Image Elevation";
   }

   return false;
}

bool ossimImageElevationDatabase::loadState(const ossimKeywordlist& kwl, const char* prefix)
{
   static const char M[] = "ossimImageElevationDatabase::loadState";
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << M << " entered..." << "\nkwl:\n" << kwl << "\n";
   }     
   bool result = false;
   const char* lookup = kwl.find(prefix, "type");
   if ( lookup )
   {
      std::string type = lookup;
      if ( ( type == "image_directory" ) || ( type == "ossimImageElevationDatabase" ) )
      {
         result = ossimElevationCellDatabase::loadState(kwl, prefix);
         if ( result )
         {
            if ( loadMapFromKwl() == false )
            {
               loadFileMap();
            }
         }
      }
   }

   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << M << " result=" << (result?"true\n":"false\n");
   }

   return result;
}

bool ossimImageElevationDatabase::saveState(ossimKeywordlist& kwl, const char* prefix) const
{
   return ossimElevationCellDatabase::saveState(kwl, prefix);
}

void ossimImageElevationDatabase::processFile(const ossimFilename& file)
{
   static const char M[] = "ossimImageElevationDatabase::processFile";
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << M << " entered...\n" << "file: " << file << "\n";
   }

   // Add the file.
   m_entryMap.insert( std::make_pair(m_lastMapKey++, ossimImageElevationFileEntry(file)) );

   if(traceDebug())
   {
      // Since ossimFileWalker is threaded output the file so we know which job exited.
      ossimNotify(ossimNotifyLevel_DEBUG) << M << "\nfile: " << file << "\nexited...\n";
   } 
}

bool ossimImageElevationDatabase::loadMapFromKwl()
{
#if TRACE_TIME
   ossimStopwatch sw;
   sw.start();
#endif

   bool result = false;
   if ( m_connectionString.size() )
   {
      ossimFilename f = m_connectionString;

      f = f.dirCat( ossimFilename(ELEV_CELL_MAP ) );
      if ( f.exists() )
      {
         ossimKeywordlist kwl;
         if ( kwl.addFile( f ) == true )
         {
            ossimString regExp = "elev_cell[0-9]*\\.file";
            ossim_uint32 count = kwl.getNumberOfKeysThatMatch( regExp );
            const ossim_uint32 MAX_LOOKUP = count + 100; // To allow for skipage.
            ossim_uint32 index = 0;
            ossim_uint32 found = 0;
            std::string basePrefix = "elev_cell";
            std::string dot = ".";
            std::string prefix;
            while ( index < MAX_LOOKUP )
            {
               prefix = basePrefix + ossimString::toString( index ).string() + dot;
               ossimImageElevationFileEntry entry;
               if ( entry.loadState( kwl, prefix, m_connectionString ) == true )
               {
                  // Add the file.
                  m_entryMap.insert( std::make_pair( m_lastMapKey++, entry ) );
                  ++found;
                  if ( found == count ) break;
               }
               ++index;
            }

            if ( m_entryMap.size() == count )
            {
               result = true;
            }
         }
      }
   }

#if TRACE_TIME
   sw.stop();
   ossimNotify(ossimNotifyLevel_NOTICE)
      << "ossimImageElevationDatabase::loadMapFromKwl() time in seconds: "
      << std::fixed << std::setprecision(8) << sw.count() << "\n";
#endif 

   return result;
}

void ossimImageElevationDatabase::loadFileMap()
{
#if TRACE_TIME
   ossimStopwatch sw;
   sw.start();
#endif
   
   if ( m_connectionString.size() )
   {
      // Create a file walker which will find files we can load from the connection string.
      ossimFileWalker* fw = new ossimFileWalker();
      
      fw->initializeDefaultFilterList();

      // This links the file walker back to our "processFile" method.
      fw->setFileProcessor( this );
      
      ossimFilename f = m_connectionString;

      // ossimFileWalker::walk will in turn call back to processFile method for each file it finds.
      fw->walk(f);

      // Save the state off for future.
      ossim_int32 index = 0;
      std::string basePrefix = "elev_cell";
      std::string dot = ".";
      std::string prefix;
      ossimKeywordlist kwl;
      const auto& cv = m_entryMap;
      for (auto&& i : cv)
      {
         std::string prefix = basePrefix + ossimString::toString(index).string() + dot;
         i.second.saveState( kwl, prefix );
         ++index;
      }
      f = m_connectionString;
      f = f.dirCat( ossimFilename("elev_cell_map.kwl") );
      if ( kwl.write( f ) == true )
      {
         ossimNotify(ossimNotifyLevel_NOTICE)
            << "ossimImageElevationDatabase::loadFileMap() NOTICE:"
            << "\nWrote file: " << f << "\n";
      }
      else
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << "ossimImageElevationDatabase::loadFileMap() WARNING:"
            << "\nCould not open file for write: " << f << std::endl;
      }
      
      delete fw;
      fw = 0;
   }

#if TRACE_TIME
   sw.stop();
   ossimNotify(ossimNotifyLevel_NOTICE)
      << "ossimImageElevationDatabase::loadFileMap() time in seconds: "
      << std::fixed << std::setprecision(8) << sw.count() << "\n";
#endif  
}

// Hidden from use:
ossimImageElevationDatabase::ossimImageElevationDatabase(const ossimImageElevationDatabase& copy)
: ossimElevationCellDatabase(copy)
{
   m_entryMap = copy.m_entryMap;
   m_lastMapKey = copy.m_lastMapKey;
   m_lastAccessedId = copy.m_lastAccessedId;
}

// Private container class:
ossimImageElevationDatabase::ossimImageElevationFileEntry::ossimImageElevationFileEntry()
   : m_file(),
     m_rect(),
     // m_nominalGSD(),
     m_loadedFlag(false)
{
   m_rect.makeNan();
   // m_nominalGSD.makeNan();
}

// Private container class:
ossimImageElevationDatabase::ossimImageElevationFileEntry::ossimImageElevationFileEntry(
   const ossimFilename& file)
   : m_file(file),
     m_rect(),
     // m_nominalGSD(),
     m_loadedFlag(false)
{
   m_rect.makeNan();
   // m_nominalGSD.makeNan();

   ossimRefPtr<ossimImageElevationHandler> h = new ossimImageElevationHandler();
   h->open( m_file );
   m_rect = h->getBoundingGndRect();
   // m_nominalGSD.x = h->getMeanSpacingMeters();
   // m_nominalGSD.y = m_nominalGSD.x;
}

ossimImageElevationDatabase::ossimImageElevationFileEntry::ossimImageElevationFileEntry
(const ossimImageElevationFileEntry& copy_this)
   : m_file(copy_this.m_file),
     m_rect(copy_this.m_rect),
     // m_nominalGSD(copy_this.m_nominalGSD),
     m_loadedFlag(copy_this.m_loadedFlag)
{
}

void ossimImageElevationDatabase::ossimImageElevationFileEntry::saveState(
   ossimKeywordlist& kwl, const std::string& prefix ) const
{
   kwl.addPair( prefix, std::string(ossimKeywordNames::FILE_KW), m_file.string() );
   kwl.addPair( prefix, std::string("grect"), m_rect.toString() );
   // kwl.addPair( prefix, std::string("gsd"), m_nominalGSD.toString().string() );
}

bool ossimImageElevationDatabase::ossimImageElevationFileEntry::loadState(
   const ossimKeywordlist& kwl, const std::string& prefix, const ossimFilename& connectionString )
{
   bool result = false;
   std::string key = ossimKeywordNames::FILE_KW;
   std::string value = kwl.findKey( prefix, key );
   if ( value.size() )
   {
      m_file = value;
      // enable paths relative to connectionString
      if ( m_file[0] == '.' )
      {
         m_file = connectionString.dirCat( m_file );
      }

      key = "grect";
      value = kwl.findKey( prefix, key );
      if ( value.size() )
      {
         if ( m_rect.toRect( value ) )
         {
            result = true;
#if 0
            key = "gsd";
            value = kwl.findKey( prefix, key );
            if ( value.size() )
            {
               m_nominalGSD.toPoint( value );
               result = true;
            }
#endif
         }
      }
   }
   return result;
}

std::ostream& ossimImageElevationDatabase::print(std::ostream& out) const
{
   ossimKeywordlist kwl;
   saveState(kwl);
   out << "\nossimImageElevationDatabase @ "<< (ossim_uint64) this << "\n"
       << kwl << std::endl;
   return out;
}
