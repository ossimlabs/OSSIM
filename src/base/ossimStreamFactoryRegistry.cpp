//*******************************************************************
//
// License:  See top level LICENSE.txt file.
//
// Author: Garrett Potts
//
//*******************************************************************
// $Id$

#include <ossim/base/ossimStreamFactoryRegistry.h>
#include <ossim/base/ossimStreamFactory.h>
#include <ossim/base/ossimIoStream.h>
#include <ossim/base/ossimFilename.h>

#include <fstream>
#include <algorithm>

ossim::StreamFactoryRegistry* ossim::StreamFactoryRegistry::m_instance = 0;

ossim::StreamFactoryRegistry::StreamFactoryRegistry()
{
}

ossim::StreamFactoryRegistry::~StreamFactoryRegistry()
{
}

ossim::StreamFactoryRegistry* ossim::StreamFactoryRegistry::instance()
{
   if(!m_instance)
   {
      m_instance = new ossim::StreamFactoryRegistry();

      // Add factory from the core.
      m_instance->registerFactory(ossim::StreamFactory::instance());
   }

   return m_instance;
}

std::shared_ptr<ossim::istream> ossim::StreamFactoryRegistry::createIstream(
   const std::string& connectionString, std::ios_base::openmode openMode) const
{
   std::shared_ptr<ossim::istream> result(0);
   ossim_uint32 i = 0;
   for(i = 0; (i < m_factoryList.size())&&(!result); ++i)
   {
      result = m_factoryList[i]->createIstream(connectionString, openMode);
   }
   return result;
}
      
std::shared_ptr<ossim::ostream> ossim::StreamFactoryRegistry::createOstream(
   const std::string& /*connectionString*/, std::ios_base::openmode /*openMode*/) const
{
   std::shared_ptr<ossim::ostream> result(0);
   return result;
}

std::shared_ptr<ossim::iostream> ossim::StreamFactoryRegistry::createIOstream(
   const std::string& /*connectionString*/, std::ios_base::openmode /*openMode*/) const
{
   std::shared_ptr<ossim::iostream> result(0);
   return result;
}

void ossim::StreamFactoryRegistry::registerFactory(ossim::StreamFactoryBase* factory)
{
   std::vector<ossim::StreamFactoryBase*>::iterator iter = std::find(
      m_factoryList.begin(), m_factoryList.end(), factory);
   if(iter == m_factoryList.end())
   {
      m_factoryList.push_back(factory);
   }
}

void ossim::StreamFactoryRegistry::unregisterFactory(StreamFactoryBase* factory)
{
   // OpenThreads::ScopedLock<OpenThreads::Mutex> lock(m_factoryListMutex);
   std::vector<ossim::StreamFactoryBase*>::iterator iter = std::find(
      m_factoryList.begin(), m_factoryList.end(), factory);
   if(iter != m_factoryList.end())
   {
      m_factoryList.erase( iter );
   }
}

// Deprecated code:
ossimStreamFactoryRegistry* ossimStreamFactoryRegistry::theInstance = 0;

ossimStreamFactoryRegistry::ossimStreamFactoryRegistry()
{
}

ossimStreamFactoryRegistry::~ossimStreamFactoryRegistry()
{
}

ossimStreamFactoryRegistry* ossimStreamFactoryRegistry::instance()
{
   if(!theInstance)
   {
      theInstance = new ossimStreamFactoryRegistry();
      theInstance->registerFactory(ossimStreamFactory::instance());
   }

   return theInstance;
}

std::shared_ptr<ossim::ifstream> ossimStreamFactoryRegistry::createIFStream(
   const ossimFilename& file, std::ios_base::openmode openMode) const
{
   std::shared_ptr<ossim::ifstream>result(0);
   
   for(ossim_uint32 idx = 0; ((idx < theFactoryList.size())&&(!result)); ++idx)
   {
      result = theFactoryList[idx]->createIFStream(file, openMode);
   }

   if(!result)
   {
      if(file.exists())
      {
         // there is a bug in gcc < 5.0 and we can't use constructors in the 
         // C++11 build.  Will refactor to do a new ifstream then use open
         //

         result = std::make_shared<ossim::ifstream>();
         result->open(file.c_str(), openMode);
         if(!result->is_open())
         {
            result.reset();
         }
      }
   }
   
   return result; 
   
}

ossimRefPtr<ossimIFStream>
ossimStreamFactoryRegistry::createNewIFStream(
   const ossimFilename& file,
   std::ios_base::openmode openMode) const
{
   ossim_uint32 idx = 0;
   ossimRefPtr<ossimIFStream> result = 0;
   for(idx = 0; ((idx < theFactoryList.size())&&(!result)); ++idx)
   {
      result = theFactoryList[idx]->createNewIFStream(file, openMode);
   }

   if(!result)
   {
      result = new ossimIFStream(file.c_str(),
                                 openMode);
//       result = new std::ifstream(file.c_str(),
//                                  openMode);
   }
   
   return result;
}


void ossimStreamFactoryRegistry::registerFactory(ossimStreamFactoryBase* factory)
{
   std::vector<ossimStreamFactoryBase*>::iterator iter = std::find(theFactoryList.begin(),
                                                                  theFactoryList.end(),
                                                                  factory);
   if(iter == theFactoryList.end())
   {
      theFactoryList.push_back(factory);
   }
}

ossimStreamFactoryRegistry::ossimStreamFactoryRegistry(const ossimStreamFactoryRegistry&)
{}
