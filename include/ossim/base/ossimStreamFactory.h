//*******************************************************************
// Copyright (C) 2005 Garrett Potts
//
// License: MIT
//
// See LICENSE.txt file in the top level directory for more details.
//
// Author: Garrett Potts
//
//
//*******************************************************************
// $Id$

#ifndef ossimStreamFactory_HEADER
#define ossimStreamFactory_HEADER 1

#include <ossim/base/ossimStreamFactoryBase.h>
#include <ossim/base/ossimIoStream.h>

namespace ossim
{
   class OSSIM_DLL StreamFactory : public StreamFactoryBase
   {
   public:
      static StreamFactory* instance();
      
      virtual ~StreamFactory();

      virtual std::shared_ptr<ossim::istream>
         createIstream(const std::string& connectionString,
                       std::ios_base::openmode mode=
                       std::ios_base::in|std::ios_base::binary) const;
      
      virtual std::shared_ptr<ossim::ostream>
         createOstream(const std::string& connectionString,
                       std::ios_base::openmode mode=
                       std::ios_base::out|std::ios_base::binary) const;
      
      virtual std::shared_ptr<ossim::iostream>
         createIOstream(const std::string& connectionString,
                        std::ios_base::openmode mode=
                        std::ios_base::in|std::ios_base::out|std::ios_base::binary) const;
   
   protected:
      StreamFactory();
      StreamFactory(const StreamFactory&);
      
      static StreamFactory* m_instance;
   };
}

// Deprecated code...
class OSSIM_DLL ossimStreamFactory : public ossimStreamFactoryBase
{
public:
   static ossimStreamFactory* instance();
   virtual ~ossimStreamFactory();

   virtual std::shared_ptr<ossim::ifstream>
      createIFStream(const ossimFilename& file,
                     std::ios_base::openmode mode) const;
   
   virtual ossimRefPtr<ossimIFStream>
      createNewIFStream(const ossimFilename& file,
                        std::ios_base::openmode mode) const;
   
protected:
   ossimStreamFactory();
   ossimStreamFactory(const ossimStreamFactory&);
   static ossimStreamFactory* theInstance;
   
};

#endif
