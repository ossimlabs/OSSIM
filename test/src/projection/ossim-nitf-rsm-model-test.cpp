//---
// File: ossim-nitf-rsm-model-test.cpp
//
// Description: Test app for ossimNitfRsmModel class.
// 
//---


#include <ossim/init/ossimInit.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimGpt.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/imaging/ossimImageGeometry.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/projection/ossimNitfRsmModel.h>
#include <ossim/projection/ossimProjection.h>
#include <ossim/projection/ossimRsmModel.h>
#include <ossim/support_data/ossimNitfRegisteredTag.h>
#include <ossim/support_data/ossimNitfRsmecaTag.h>
#include <ossim/support_data/ossimNitfRsmidaTag.h>
#include <ossim/support_data/ossimNitfRsmpcaTag.h>
#include <ossim/support_data/ossimNitfRsmpiaTag.h>
#include <ossim/support_data/ossimNitfTagFactoryRegistry.h>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

static ossimRefPtr<ossimNitfRegisteredTag> getTag( const std::string& tagLine );
static ossimRefPtr<ossimNitfRsmModel> getModel( const ossimFilename& file );
static ossimRefPtr<ossimNitfRsmModel> getModelFromExtFile( const ossimFilename& file );
static ossimRefPtr<ossimNitfRsmModel> getModelFromImage( const ossimFilename& file );
static void testGpts( ossimRefPtr<ossimNitfRsmModel>& model, const ossimKeywordlist& kwl );
static void testIpts( ossimRefPtr<ossimNitfRsmModel>& model, const ossimKeywordlist& kwl );

int main(int argc, char *argv[])
{
   ossimInit::instance()->initialize(argc, argv);

   if ( argc == 3 )
   {
      ossimFilename file = argv[1];
      ossimKeywordlist kwl;
      if ( kwl.addFile( argv[2] ) )
      {
         ossimRefPtr<ossimNitfRsmModel> model = getModel( file );
         if ( model.valid() )
         {
            testGpts( model, kwl );
            testIpts( model, kwl );
         }
         else
         {
            cerr << "Could not create model!" << endl;
         }
      }
      else
      {
         cerr << "Could not open keyword list: " << argv[2] << endl;
      }  
   }
   else
   {
      cout << "\n" << argv[0] << " <input-test-file> <options.kwl>"
           << "\ninput-test-file can be an input test file or a test.ext file.\n"
           << "\nPrints out debug info for model testing.."
           << "\ngpts test:"
           << "\nmodel->worldToLineSample(...), model->lineSampleToWorld(...)"
           << "\nipts test:"
           << "\nmodel->lineSampleHeightToWorld(...), model->worldToLineSample(...)"
           << "\noptions.kwl format example:\n"

           << "\n// Input line, sample type: area=upper left corner, point = center of pixel."
           << "\n// pixel_type: area"

           << "\ngtest_id0: E123456\n"
           << "gtest_gpt0: ( lat, lon, hgt )\n"

           << "gtest_gpt_id1: E123457\n"
           << "gtest_gpt1: ( lat, lon, hgt )\n"

           << "ggtest_pt_id2: E123458\n"
           << "gtest_gpt2: ( lat, lon, hgt )\n"

           << "gtest_gpt_id3: E123459\n"
           << "gtest_gpt3: ( lat, lon, hgt )\n"

           << "gtest_gpt_id4: E123459\n"
           << "gtest_pt4: ( lat, lon, hgt )\n"

           << "\nitest_height_units: feet\n"
         
           << "itest_id0: E123456\n"
           << "itest_ipt0: ( sample, line )\n"
           << "itest_hgt0: hgt\n"
         
           << "itest_id1: E123457\n"
           << "itest_ipt1: ( x, y )\n"
           << "itest_hgt1: hgt\n"

           << "itest_id2: E123458\n"
           << "itest_ipt2: ( x, y )\n"
           << "itest_hgt2: hgt\n"

           << "itest_id3: E123459\n"
           << "itest_ipt3: ( x, y )\n"
           << "itest_hgt3: hgt\n"

           << "itest_id4: E123459\n"
           << "itest_ipt4: ( x, y )\n"
           << "itest_hgt4: hgt\n"

           << "\nNotes:\n"
           << "* itest_hgt default = meters if units not specified.\n"
           << "* gpt0: ( lat, lon, hgt ) \"hgt\" is in meters.\n"
           << "* All test height output is in meters.\n"
           << "* \"rtd\"=round trip delta.\n"
           << endl;
   }

   return 0;
}


ossimRefPtr<ossimNitfRegisteredTag> getTag( const std::string& tagLine )
{
   ossimRefPtr<ossimNitfRegisteredTag> result = 0;

   if ( tagLine.size() > 6 )
   {
      ossimString tagName = tagLine.substr(0, 6);
      result = ossimNitfTagFactoryRegistry::instance()->create( tagName );
      if ( result.valid() )
      {
         cout << "tag_name: " << tagName << "\n";
         
         istringstream is( tagLine );
         if ( is.good() )
         {
            is.seekg( 6 );
            char tagLength[6];
            tagLength[5] = '\0';
            is.read( tagLength, 5 );
            cout << "tag_length: " << tagLength << "\n";
            result->parseStream( is );

            result->print( cout, std::string("") );
         }
      }
      else
      {
         cerr << "unhandled_tag: " << tagName << endl;
      }
   }
   return result;
}

ossimRefPtr<ossimNitfRsmModel> getModel( const ossimFilename& file )
{
   ossimRefPtr<ossimNitfRsmModel> result = 0;
   if ( file.size() )
   {
      // Get downcased extension:
      std::string ext = file.ext().downcase().string();
      
      if ( ext == "ext" )
      {
         result = getModelFromExtFile( file );
      }
      else if ( ( ext == "ntf" ) || ( ext == "nitf" ) )
      {
         result = getModelFromImage( file );
      }
   }
   return result;
}

ossimRefPtr<ossimNitfRsmModel> getModelFromExtFile( const ossimFilename& file )
{
   ossimRefPtr<ossimNitfRsmModel> result = 0;

   if ( file.exists() )
   {
      result = new ossimNitfRsmModel();
      if ( result->parseFile( file, 0 ) ) // Hard coded entry index of 0 for now.
      {
         cout << "Initialize from ext file success!" << endl;
      }
      else
      {
         result = 0;
         cerr << "Could not open: " << file << endl;
      }
   }
   else
   {
     cerr << "File does not exists: " << file << endl;
   }

   return result;
   
} // End: getModelFromExtFile(...)


ossimRefPtr<ossimNitfRsmModel> getModelFromImage( const ossimFilename& file )
{
   ossimRefPtr<ossimNitfRsmModel> result = 0;

   ossimRefPtr<ossimImageHandler> ih =
      ossimImageHandlerRegistry::instance()->open(file,
                                                  true,   // try suffix first
                                                  false); // open overview
   if ( ih.valid() )
   {
      ossimRefPtr<ossimImageGeometry> geom = ih->getImageGeometry();
      if ( geom.valid() )
      {
         ossimRefPtr<ossimProjection> proj = geom->getProjection();
         if ( proj.valid() )
         {
            result = dynamic_cast<ossimNitfRsmModel*>( proj.get() );
         }
      }
   }
   
   return result;  
}

void testGpts( ossimRefPtr<ossimNitfRsmModel>& model, const ossimKeywordlist& kwl )
{
   if ( model.valid() )
   {
      cout << std::setfill(' ') << setiosflags(ios::left);
      
      const std::string  ID_KW  = "gtest_id";
      const std::string  GPT_KW = "gtest_gpt";
      const ossim_uint32 POINTS = kwl.numberOf( ID_KW.c_str() );
      
      cout << "\ngtest begin ********************************\n\n"
           << "number_of_points_world_points: " << POINTS << "\n";
      
      ossim_uint32 foundPts = 0;
      ossim_uint32 i = 0;
      
      std::string key;
      std::string value;
      
      while ( foundPts < POINTS )
      {
         // ID:
         key = ID_KW + ossimString::toString( i ).string();
         value = kwl.findKey( key );
         if ( value.size() )
         {
            cout << "gtest_id" << std::setw(6) << i << ":  " << value << "\n";
         }
      
         // World point :
         key = GPT_KW + ossimString::toString( i ).string();
         value = kwl.findKey( key );
      
         if ( value.size() )
         {
            ossimDpt ipt; // image point
            ossimGpt wpt; // world point
            ossimGpt rtp; // round trip point
            ossimDpt rtd; // round trip delta;
         
            wpt.toPoint( value );

            cout << "gtest_gpt" << std::setw(5) << i << ":  " << wpt << "\n";

            model->worldToLineSample( wpt, ipt );

            if ( wpt.hasNans() == false )
            {
               model->lineSampleHeightToWorld( ipt, wpt.hgt, rtp );
            
               rtd.x = wpt.lon - rtp.lon;
               rtd.y = wpt.lat - rtp.lat;
            
               cout << "gtest_ipt" << std::setw(5) << i << ":  " << ipt << "\n"
                    << "gtest_rtp" << std::setw(5) << i << ":  " << rtp << "\n"
                    << "gtest_rtd" << std::setw(5) << i << ":  " << rtd << "\n\n";  
            }
            else
            {
               cerr << "model->worldToLineSample(...) result has nans!\n"
                    << wpt << endl;
            }
         
            ++foundPts;
         }
      
         ++i;
      
         if ( i > POINTS+100 )
         {
            break;
         }
      }

      cout << "\ngtest end **********************************\n\n";
   }
   
} // End: testGpts

void testIpts( ossimRefPtr<ossimNitfRsmModel>& model, const ossimKeywordlist& kwl )
{
   if ( model.valid() )
   {
      cout << std::setfill(' ') << setiosflags(ios::left);

      const std::string  ID_KW      = "itest_id";
      const std::string  IPT_KW     = "itest_ipt";
      const std::string  IPT_GT_KW  = "itest_gt"; // ground truth      
      const std::string  IPT_HGT_KW = "itest_hgt";
      const ossim_uint32 POINTS     = kwl.numberOf( ID_KW.c_str() );
   
      // Test data height values can be in feet.
      ossimUnitType heightUnits = OSSIM_METERS;
      std::string key = "itest_height_units";
      std::string value = kwl.findKey( key );
      if ( value.size() )
      {
         cout << key << ": " << value << "\n";
         if ( value == "feet" )
         {
            heightUnits = OSSIM_FEET;
         }
      }

      // Test the pixel type.
      ossim_float64 iptShift = 0.0;
      key = "pixel_type";
      value = kwl.findKey( key );
      if ( value.size() )
      {
         if ( value == "area" )
         {
            iptShift = -0.5;
            cout << key << ": " << value << "\n";
            cout << "input_line_sample_shift: " << iptShift << "\n";
         }
      }     
      
      cout << "\nitest begin ********************************\n\n"
           << "number_of_line_sample_points: " << POINTS << "\n";
      
      ossim_uint32 foundPts = 0;
      ossim_uint32 i = 0;

      while ( foundPts < POINTS )
      {
         // ID:
         key = ID_KW + ossimString::toString( i ).string();
         value = kwl.findKey( key );
         if ( value.size() )
         {
            cout << "itest_id" << std::setw(9) << i << ":  " << value << "\n";
         }
      
         // Image point, sample, line:
         key = IPT_KW + ossimString::toString( i ).string();
         value = kwl.findKey( key );
      
         if ( value.size() )
         {
            ossimDpt ipt; // image point
            ossimGpt wpt; // world point
            ossimGpt gt;  // ground truth
            ossimDpt gtd; // wpt to gt delta
            ossimDpt rtp; // round trip point
            ossimDpt rtd; // round trip delta;
         
            ipt.toPoint( value );
            ipt.x += iptShift;
            ipt.y += iptShift;
         
            cout << "itest_ipt" << std::setw(8) << i << ":  " << value << "\n";
         
            // Get the height above ellipsoid:
            ossim_float64 hgt = 0.0;
            key = IPT_HGT_KW + ossimString::toString( i ).string();
            value = kwl.findKey( key );
            if ( value.size() )
            {
               ossimString os ( value );
               hgt = os.toFloat64();
            
               if ( heightUnits == OSSIM_FEET )
               {
                  hgt *= MTRS_PER_FT;
               }
            }
            else
            {
               cerr << "missing height above ellipsoid for point!  Using 0.0."
                    << endl;
            }
         
            cout << "itest_hgt" << std::setw(8) << i << ":  " << value << "\n";
         
            model->lineSampleHeightToWorld( ipt, hgt, wpt );

            cout << "itest_wpt" << std::setw(8) << i << ":  " << wpt << "\n";
            
            if ( wpt.hasNans() == false )
            {
               model->worldToLineSample( wpt, rtp );
               
               // Get the ground truth;
               key = IPT_GT_KW + ossimString::toString( i ).string();
               value = kwl.findKey( key );
               if ( value.size() )
               {
                  gt.toPoint( value );
                  cout << "itest_gt" << std::setw(9) << i << ":  " << gt << "\n";
                  if ( gt.isNan() == false )
                  {
                     gtd.x = wpt.lon - gt.lon;
                     gtd.y = wpt.lat - gt.lat;
                     ossimDpt mpd = wpt.metersPerDegree();
                     ossimDpt gtm;
                     gtm.x = gtd.x * mpd.x;
                     gtm.y = gtd.y * mpd.y;
                     cout << "itest_gtd_dd" << std::setw(5) << i << ":  " << gtd << "\n";
                     cout << "itest_gtd_mtrs" << std::setw(3) << i << ":  " << gtm << "\n";                     
                  }
               }
               else
               {
                  gt.makeNan();
               }
            
               rtd = ipt - rtp;
               
               cout << "itest_rtp" << std::setw(8) << i << ":  " << rtp << "\n"
                    << "itest_rtd" << std::setw(8) << i << ":  " << rtd << "\n\n";
            }
            else
            {
               cerr << "model->worldToLineSample(...) result has nans!\n"
                    << wpt << endl;
            }
         
            ++foundPts;
         }
      
         ++i;
      
         if ( i > POINTS+100 )
         {
            break;
         }
      }

      cout << "\ntestIpts end **********************************\n\n";
   }
   
} // End: testIpts
