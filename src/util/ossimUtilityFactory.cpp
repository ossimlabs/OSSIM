//**************************************************************************************************
//
//     OSSIM Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include <ossim/util/ossimUtilityFactory.h>
#include <ossim/util/ossimHillshadeUtil.h>
#include <ossim/util/ossimHlzUtil.h>
#include <ossim/util/ossimShorelineUtil.h>
#include <ossim/util/ossimViewshedUtil.h>
#include <ossim/util/ossimSlopeUtil.h>
#include <ossim/util/ossimOrthoUtil.h>
#include <ossim/util/ossimVerticesFinderUtil.h>
#include <ossim/util/ossimInfo.h>
#include <ossim/util/ossimBandMergeUtil.h>
#include <ossim/util/ossimUtilityRegistry.h>

ossimUtilityFactory* ossimUtilityFactory::s_Instance = 0;

ossimUtilityFactory* ossimUtilityFactory::instance()
{
   if (!s_Instance)
      s_Instance = new ossimUtilityFactory;
   return s_Instance;
}

ossimUtilityFactory::ossimUtilityFactory()
{
}

ossimUtilityFactory::~ossimUtilityFactory()
{
   ossimUtilityRegistry::instance()->unregisterFactory(this);
}

ossimUtility* ossimUtilityFactory::createUtility(const std::string& argName) const
{
   ossimString utilName (argName);
   utilName.downcase();

   if ((utilName == "info") || (argName == "ossimInfo"))
      return new ossimInfo;

   if ((utilName == "hillshade") || (argName == "ossimHillshadeUtil"))
      return new ossimHillshadeUtil;

   if ((utilName == "viewshed") || (argName == "ossimViewshedUtil"))
      return new ossimViewshedUtil;

   if ((utilName == "slope") || (argName == "ossimSlopeUtil"))
      return new ossimSlopeUtil;

   if ((utilName == "hlz") || (argName == "ossimHlzUtil"))
      return new ossimHlzUtil;

   if ((utilName == "shoreline") || (argName == "ossimShorelineUtil"))
      return new ossimShorelineUtil;

   if ((utilName == "ortho") || (argName == "ossimOrthoUtil"))
      return new ossimOrthoUtil;

   if ((utilName == "vertices") || (argName == "ossimVerticesFinderUtil"))
      return new ossimVerticesFinderUtil;

   if ((utilName == "bandmerge") || (argName == "ossimBandMergeUtil"))
      return new ossimBandMergeUtil;

   return 0;
}

void ossimUtilityFactory::getCapabilities(std::map<std::string, std::string>& capabilities) const
{
   capabilities.insert(pair<string, string>("info", ossimInfo::DESCRIPTION));
   capabilities.insert(pair<string, string>("hillshade", ossimHillshadeUtil::DESCRIPTION));
   capabilities.insert(pair<string, string>("viewshed", ossimViewshedUtil::DESCRIPTION));
   capabilities.insert(pair<string, string>("slope", ossimSlopeUtil::DESCRIPTION));
   capabilities.insert(pair<string, string>("hlz", ossimHlzUtil::DESCRIPTION));
   capabilities.insert(pair<string, string>("shoreline", ossimShorelineUtil::DESCRIPTION));
   capabilities.insert(pair<string, string>("ortho", ossimOrthoUtil::DESCRIPTION));
   capabilities.insert(pair<string, string>("vertices", ossimVerticesFinderUtil::DESCRIPTION));
   capabilities.insert(pair<string, string>("bandmerge", ossimBandMergeUtil::DESCRIPTION));
}

std::map<std::string, std::string> ossimUtilityFactory::getCapabilities() const
{
   std::map<std::string, std::string> result;
   getCapabilities(result);
   return result;
}

void ossimUtilityFactory::getTypeNameList(vector<ossimString>& typeList) const
{
   typeList.push_back("ossimInfo");
   typeList.push_back("ossimHillshadeUtil");
   typeList.push_back("ossimViewshedUtil");
   typeList.push_back("ossimSlopeUtil");
   typeList.push_back("ossimHLZUtil");
   typeList.push_back("ossimShorelineUtil");
   typeList.push_back("ossimOrthoUtil");
   typeList.push_back("ossimVerticesFinderUtil");
   typeList.push_back("ossimBandMergeUtil");
}

