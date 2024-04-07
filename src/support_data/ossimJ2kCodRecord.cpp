//---
//
// License: MIT
// 
// Author:  David Burken
//
// Description: Container class for J2K Coding style default (COD) record.
//
// See document BPJ2K01.00 Table 7-7 Image and tile size (15444-1 Annex A5.1)
// 
//---
// $Id$

#include <ossim/support_data/ossimJ2kCodRecord.h>
#include <ossim/base/ossimConstants.h>
#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimEndian.h>

#include <iostream>
#include <iomanip>


ossimJ2kCodRecord::ossimJ2kCodRecord()
   :
   m_marker(0xff52),
   m_lcod(0),
   m_scod(0),
   m_progressionOrder(0),
   m_numberOfLayers(0),
   m_multipleComponentTransform(0),
   m_numberOfDecompositionLevels(0),
   m_codeBlockWidth(0),
   m_codeBlockHeight(0),
   m_codeBlockStyle(0),
   m_transformation(0),
   m_precinctSize(0)
{
}

ossimJ2kCodRecord::~ossimJ2kCodRecord()
{
}

void ossimJ2kCodRecord::parseStream(std::istream& in)
{
   // Get the stream posistion.
   std::streamoff pos = in.tellg();

   // Note: Marker is not read.
   in.read((char*)&m_lcod, 2);
   in.read((char*)&m_scod, 1);
   in.read((char*)&m_progressionOrder, 1);
   in.read((char*)&m_numberOfLayers, 2);
   in.read((char*)&m_multipleComponentTransform, 1);
   in.read((char*)&m_numberOfDecompositionLevels, 1);
   in.read((char*)&m_codeBlockWidth, 1);
   in.read((char*)&m_codeBlockHeight, 1);
   in.read((char*)&m_codeBlockStyle, 1);
   in.read((char*)&m_transformation, 1);

   //---
   // SPcod - precinct size (only is defined, Scod = xxxx xxx1)
   //---   
   if ( 0 )
   {
      in.read((char*)&m_precinctSize, 1);
   }

   if (ossim::byteOrder() == OSSIM_LITTLE_ENDIAN)
   {
      // Stored big endian, must swap.
      ossimEndian s;
      s.swap(m_lcod);
      s.swap(m_numberOfLayers);
   }

   //---
   // Seek to next record.  This is needed because there are sometimes extra
   // bytes.
   //---
   in.seekg(pos + m_lcod, std::ios_base::beg);
}

std::ostream& ossimJ2kCodRecord::print(std::ostream& out,
                                       const std::string& prefix) const
{
   // Capture the original flags.
   std::ios_base::fmtflags f = out.flags();

   std::string pfx = prefix;
   pfx += "cod.";

   out << pfx << "marker: 0x" << std::hex << m_marker << "\n";

   out << pfx << "Lcod: " << m_lcod       << "\n"
       << pfx << "Scod: " << int(m_scod)  << "\n"
       << pfx << "SGcod_progression_order:  "
       << getProgressionOrderAsString(m_progressionOrder) << "\n"
       << pfx << "SGcod_number_of_layers:  " << m_numberOfLayers << "\n"
       << pfx << "SGcod_multiple_component_transform:  "
       << int(m_multipleComponentTransform) << "\n"
       << pfx << "SPcod_mumber_of_decomposition_levels: "
       << int(m_numberOfDecompositionLevels) << "\n"
       << pfx << "SPcod_code_block_width: " << int(m_codeBlockWidth) << "\n"
       << pfx << "SPcod_code_block_height: " << int(m_codeBlockHeight) << "\n"
       << pfx << "SPcod_code_block_style: " << int(m_codeBlockStyle) << "\n"
       << pfx << "SPcod_transformation: " << int(m_transformation) << "\n"
       << pfx << "SPcod_precinct_size: " << int(m_precinctSize)
       << std::endl;

   // Reset flags.
   out.setf(f);

   return out;
}

std::string ossimJ2kCodRecord::getProgressionOrderAsString( ossim_uint8 progressionOrder ) const
{
   std::string result;
   
   if ( progressionOrder == 0x00 )
   {
      result = "LRCP";
   }
   else if ( progressionOrder == 0x01 )
   {
      result = "RLCP";
   }
   else if ( progressionOrder == 0x02 )
   {
      result = "RPCL";
   }
   else if ( progressionOrder == 0x03 )
   {
      result = "PCRL";
   }
   else if ( progressionOrder == 0x04 )
   {
      result = "CPRL";
   }
   else
   {
      result = "unknown";
   }
   
   return result;
}

ossim_uint8 ossimJ2kCodRecord::getProgressionOrder() const
{
   return m_progressionOrder;
}

std::ostream& operator<<(std::ostream& out, const ossimJ2kCodRecord& obj)
{
   return obj.print(out);
}
