//-------------------------------------------------------------------------
//
// This code was taken from Open Scene Graph and incorporated from into
// OSSIM.
//
//-------------------------------------------------------------------------
// $Id: ossimArgumentParser.cpp 22491 2013-11-26 18:17:29Z dburken $

#include <ossim/base/ossimArgumentParser.h>
#include <ossim/base/ossimApplicationUsage.h>
#include <ossim/base/ossimString.h>

#include <cstring>
#include <iostream>
#include <set>

using namespace std;

bool ossimArgumentParser::isOption(const char* str)
{
   return str && str[0]=='-';
}

bool ossimArgumentParser::isString(const char* str)
{
   if (!str) return false;

   return true;
//    return !isOption(str);
}

bool ossimArgumentParser::isNumber(const char* str)
{
   if (!str) return false;
   
   bool hadPlusMinus = false;
   bool hadDecimalPlace = false;
   bool hadExponent = false;
   bool couldBeInt = true;
   bool couldBeFloat = true;
   int noZeroToNine = 0;

   const char* ptr = str;

   // check if could be a hex number.
   if (std::strncmp(ptr,"0x",2)==0)
   {
      // skip over leading 0x, and then go through rest of string
      // checking to make sure all values are 0...9 or a..f.
      ptr+=2;
      while (
         *ptr!=0 &&
         ((*ptr>='0' && *ptr<='9') ||
          (*ptr>='a' && *ptr<='f') ||
          (*ptr>='A' && *ptr<='F'))
         )
      {
         ++ptr;
      }

      // got to end of string without failure, therefore must be a hex integer.
      if (*ptr==0) return true;
   }

   ptr = str;

   // check if a float or an int.
   while (*ptr!=0 && couldBeFloat)
   {
      if (*ptr=='+' || *ptr=='-')
      {
         if (hadPlusMinus)
         {
            couldBeInt = false;
            couldBeFloat = false;
         }
         else
         {
            hadPlusMinus = true;
         }
      }
      else if (*ptr>='0' && *ptr<='9')
      {
         noZeroToNine++;
      }
      else if (*ptr=='.')
      {
         if (hadDecimalPlace)
         {
            couldBeInt = false;
            couldBeFloat = false;
         }
         else
         {
            hadDecimalPlace = true;
            couldBeInt = false;
         }
      }
      else if (*ptr=='e' || *ptr=='E')
      {
         if (hadExponent || noZeroToNine==0)
         {
            couldBeInt = false;
            couldBeFloat = false;
         }
         else
         {
            hadExponent = true;
            couldBeInt = false;
            hadDecimalPlace = false;
            hadPlusMinus = false;
            noZeroToNine=0;
         }
      }
      else
      {
         couldBeInt = false;
         couldBeFloat = false;
      }
      ++ptr;
   }

   if (couldBeInt && noZeroToNine>0) return true;
   if (couldBeFloat && noZeroToNine>0) return true;

   return false;

}

bool ossimArgumentParser::ossimParameter::valid(const char* str) const
{
   switch(theType)
   {
      case ossimParameter::OSSIM_FLOAT_PARAMETER:        return isNumber(str);
      case ossimParameter::OSSIM_DOUBLE_PARAMETER:       return isNumber(str);
      case ossimParameter::OSSIM_INT_PARAMETER:          return isNumber(str);
      case ossimParameter::OSSIM_UNSIGNED_INT_PARAMETER: return isNumber(str);
      case ossimParameter::OSSIM_STRING_PARAMETER:       return isString(str);
   }
   return false;
}

bool ossimArgumentParser::ossimParameter::assign(const char* str)
{
   if (!valid(str))
      return false;

   switch(theType)
   {
   case ossimParameter::OSSIM_FLOAT_PARAMETER:
      *theValue.theFloat = (float)ossimString(str).toDouble();
      break;
   case ossimParameter::OSSIM_DOUBLE_PARAMETER:
      *theValue.theDouble = ossimString(str).toDouble();
      break;
   case ossimParameter::OSSIM_INT_PARAMETER:
      *theValue.theInt = ossimString(str).toInt();
      break;
   case ossimParameter::OSSIM_UNSIGNED_INT_PARAMETER:
      *theValue.theUint = ossimString(str).toUInt32();
      break;
   case ossimParameter::OSSIM_STRING_PARAMETER:
      *theValue.theString = ossimString(str).chars();
      break;
   }
   return true;
}

ossimArgumentParser::ossimArgumentParser(int* argc,char **argv):
   theArgc(argc),
   theArgv(argv),
   theUsage(ossimApplicationUsage::instance()),
   theMemAllocated(false)
{
   if (theArgc)
      theUsage->setApplicationName(argv[0]);
}

ossimArgumentParser::ossimArgumentParser(const ossimString& commandLine):
   theArgc(new int),
   theArgv(0),
   theUsage(ossimApplicationUsage::instance()),
   theMemAllocated(true)
{
   vector<ossimString> args = commandLine.split(" ", true);
   *theArgc = (int)args.size();
   if (theArgc > 0)
   {
      theArgv = new char* [*theArgc];
      for (size_t i=0; i<args.size(); i++)
      {
         size_t n = args[i].size();
         theArgv[i] = new char [n+1];
         strncpy(theArgv[i], args[i].chars(), n);
         theArgv[i][n] = '\0';
      }
      theUsage->setApplicationName(theArgv[0]);
   }
}

ossimArgumentParser::~ossimArgumentParser()
{
   if (theMemAllocated)
   {
      for (int i=0; i<*theArgc; ++i)
         delete theArgv[i];
      delete [] theArgv;
      delete theArgc;
   }
}

void ossimArgumentParser::initialize(int* argc, const char **argv)
{
   if(argc > 0) delete (theArgv);
   theArgc = argc;
   theArgv = new char*[*argc];
   for(int i=0;i<*argc;i++)
   {
      theArgv[i] = new char[strlen(argv[i])];
      strcpy(theArgv[i], argv[i]);
   }
}

std::string ossimArgumentParser::getApplicationName() const
{
   if (theArgc && *theArgc>0 ) return std::string(theArgv[0]);
   return "";
}

bool ossimArgumentParser::isOption(int pos) const
{
   return pos<*theArgc && isOption(theArgv[pos]);
}

bool ossimArgumentParser::isString(int pos) const
{
   return pos < *theArgc && isString(theArgv[pos]);
}

bool ossimArgumentParser::isNumber(int pos) const
{
   return pos < *theArgc && isNumber(theArgv[pos]);
}

int ossimArgumentParser::find(const std::string& str) const
{
   for(int pos=1;pos<*theArgc;++pos)
   {
      if (str==theArgv[pos])
      {
         return pos;
      }
   }
   return 0;
}

bool ossimArgumentParser::match(int pos, const std::string& str) const
{
   return pos<*theArgc && str==theArgv[pos];
}

bool ossimArgumentParser::containsOptions() const
{
   for(int pos=1;pos<*theArgc;++pos)
   {
      if (isOption(pos)) return true;
   }
   return false;
}

int ossimArgumentParser::numberOfParams(const std::string& str, const ossimParameter param) const
{
   int pos=find(str);
   if (pos<=0) 
      return -1;

   ++pos;
   int num_params = 0;
   while (param.valid(theArgv[pos+num_params]))
      ++num_params;
   return num_params;
}

void ossimArgumentParser::remove(int pos,int num)
{
   if (num==0) return;
   
   for(;pos+num<*theArgc;++pos)
   {
      theArgv[pos]=theArgv[pos+num];
   }
   for(;pos<*theArgc;++pos)
   {
      theArgv[pos]=0;
   }
   *theArgc-=num;
}

void ossimArgumentParser::insert(int pos, const ossimString& argstr)
{
   if (argstr.size()==0)
      return;

   // Split arg into components (separated by spaces). Need to reallocate args array to new size:
   vector<ossimString> components = argstr.split(" ");
   int new_argc = *theArgc + (int)components.size();
   char** new_argv = new char*[new_argc];

   // First copy the original list, leaving space for the new components:
   int j = 0;
   for (int i=0; i<*theArgc; ++i)
   {
      if (j == pos)
         j += (int)components.size();
      new_argv[j] = theArgv[i];
      ++j;
   }

   // Insert new components:
   for(ossim_uint32 i=0; i<components.size(); ++i)
      new_argv[pos+i]=components[i].stringDup();

   // Need to deallocate old arg storage?
   if (theMemAllocated)
      delete [] theArgv;
   theArgv = new_argv;
   *theArgc = new_argc;
   theMemAllocated = true;
}

bool ossimArgumentParser::read(const std::string& str)
{
   int pos=find(str);
   if (pos<=0) return false;
   remove(pos);
   return true;
}

bool ossimArgumentParser::read(const std::string& str, ossimParameter value1)
{
   int pos=find(str);
   if (pos<=0) return false;
   if (!value1.valid(theArgv[pos+1]))
   {
      reportError("argument to `"+str+"` is missing");
      return false;
   }
   value1.assign(theArgv[pos+1]);
   remove(pos,2);
   return true;
}

bool ossimArgumentParser::read(const std::string& str, ossimParameter value1,
                               ossimParameter value2)
{
   int pos=find(str);
   if (pos<=0) return false;
   if (!value1.valid(theArgv[pos+1]) ||
       !value2.valid(theArgv[pos+2]))
   {
      reportError("argument to `"+str+"` is missing");
      return false;
   }
   value1.assign(theArgv[pos+1]);
   value2.assign(theArgv[pos+2]);
   remove(pos,3);
   return true;
}

bool ossimArgumentParser::read(const std::string& str, ossimParameter value1,
                               ossimParameter value2, ossimParameter value3)
{
   int pos=find(str);
   if (pos<=0) return false;
   if (!value1.valid(theArgv[pos+1]) ||
       !value2.valid(theArgv[pos+2]) ||
       !value3.valid(theArgv[pos+3]))
   {
      reportError("argument to `"+str+"` is missing");
      return false;
   }
   value1.assign(theArgv[pos+1]);
   value2.assign(theArgv[pos+2]);
   value3.assign(theArgv[pos+3]);
   remove(pos,4);
   return true;
}

bool ossimArgumentParser::read(const std::string& str, ossimParameter value1,
                               ossimParameter value2, ossimParameter value3,
                               ossimParameter value4)
{
   int pos=find(str);
   if (pos<=0) return false;
   if (!value1.valid(theArgv[pos+1]) ||
       !value2.valid(theArgv[pos+2]) ||
       !value3.valid(theArgv[pos+3]) ||
       !value4.valid(theArgv[pos+4]))
   {
      reportError("argument to `"+str+"` is missing");
      return false;
   }
   value1.assign(theArgv[pos+1]);
   value2.assign(theArgv[pos+2]);
   value3.assign(theArgv[pos+3]);
   value4.assign(theArgv[pos+4]);
   remove(pos,5);
   return true;
}

bool ossimArgumentParser::read(const std::string& str, ossimParameter value1,
                               ossimParameter value2, ossimParameter value3,
                               ossimParameter value4, ossimParameter value5)
{
   int pos=find(str);
   if (pos<=0) return false;
   if (!value1.valid(theArgv[pos+1]) ||
       !value2.valid(theArgv[pos+2]) ||
       !value3.valid(theArgv[pos+3]) ||
       !value4.valid(theArgv[pos+4]) ||
       !value5.valid(theArgv[pos+5]))
   {
      reportError("argument to `"+str+"` is missing");
      return false;
   }
   value1.assign(theArgv[pos+1]);
   value2.assign(theArgv[pos+2]);
   value3.assign(theArgv[pos+3]);
   value4.assign(theArgv[pos+4]);
   value5.assign(theArgv[pos+5]);    
   remove(pos,6);
   return true;
}

bool ossimArgumentParser::read(const std::string& str, ossimParameter value1,
                               ossimParameter value2, ossimParameter value3,
                               ossimParameter value4, ossimParameter value5,
                               ossimParameter value6)
{
   int pos=find(str);
   if (pos<=0) return false;
   if (!value1.valid(theArgv[pos+1]) ||
       !value2.valid(theArgv[pos+2]) ||
       !value3.valid(theArgv[pos+3]) ||
       !value4.valid(theArgv[pos+4]) ||
       !value5.valid(theArgv[pos+5]) ||
       !value6.valid(theArgv[pos+6]))       
   {
      reportError("argument to `"+str+"` is missing");
      return false;
   }
   value1.assign(theArgv[pos+1]);
   value2.assign(theArgv[pos+2]);
   value3.assign(theArgv[pos+3]);
   value4.assign(theArgv[pos+4]);
   value5.assign(theArgv[pos+5]);
   value6.assign(theArgv[pos+6]);    
   remove(pos,7);
   return true;
}

bool ossimArgumentParser::read(const std::string& str, std::vector<ossimString>& param_list)
{
   // This method reads a comma-separated list.
   param_list.clear();

   int pos=find(str);
   if (pos<=0)
      return false;

   // Option is removed even if no values found:
   remove(pos, 1);
   bool includeNextItem = true;
   while (pos < *theArgc)
   {
      // Check for occurence of next option:
      if ((theArgv[pos][0] == '-'))
         break;

      // Skip a comma surrounded by spaces:
      ossimString arg = theArgv[pos];
      if (arg == ",")
      {
         remove(pos, 1);
         includeNextItem = true;
         continue;
      }

      if (!includeNextItem && (arg.string()[0] != ','))
         break;

      // Handle comma separated with no spaces (i.e., multiple args reflected as one in theArgv):
      vector<ossimString> sub_args = arg.split(",", true);
      for (ossim_uint32 i=0; i<sub_args.size(); ++i)
         param_list.push_back(sub_args[i]);

      // If current item ends with comma, the list continues:
      if (arg[arg.length()-1] != ',')
         includeNextItem = false;

      remove(pos, 1);
   }

   return true;
}

/** if the argument value at the posotion pos matches specified string, and subsequent
 * parameters are also matched then set the paramter values and remove the from the list of arguments.*/
bool ossimArgumentParser::read(int pos, const std::string& str)
{
   if (match(pos,str))
   {
      remove(pos,1);
      return true;
   }
   else
   {
      return false;
   }
}

bool ossimArgumentParser::read(int pos, const std::string& str, ossimParameter value1)
{
   if (match(pos,str) &&
       value1.valid(theArgv[pos+1]))
   {
      value1.assign(theArgv[pos+1]);
      remove(pos,2);
      return true;
   }
   else
   {
      return false;
   }
}

bool ossimArgumentParser::read(int pos, const std::string& str, ossimParameter value1, ossimParameter value2)
{
   if (match(pos,str) &&
       value1.valid(theArgv[pos+1]) &&
       value2.valid(theArgv[pos+2]))
   {
      value1.assign(theArgv[pos+1]);
      value2.assign(theArgv[pos+2]);
      remove(pos,3);
      return true;
   }
   else
   {
      return false;
   }
}

bool ossimArgumentParser::read(int pos, const std::string& str, ossimParameter value1, ossimParameter value2, ossimParameter value3)
{
   if (match(pos,str) &&
       value1.valid(theArgv[pos+1]) &&
       value2.valid(theArgv[pos+2]) &&
       value3.valid(theArgv[pos+3]))
   {
      value1.assign(theArgv[pos+1]);
      value2.assign(theArgv[pos+2]);
      value3.assign(theArgv[pos+3]);
      remove(pos,4);
      return true;
   }
   else
   {
      return false;
   }
}

bool ossimArgumentParser::read(int pos, const std::string& str, ossimParameter value1, ossimParameter value2, ossimParameter value3, ossimParameter value4)
{
   if (match(pos,str) &&
       value1.valid(theArgv[pos+1]) &&
       value2.valid(theArgv[pos+2]) &&
       value3.valid(theArgv[pos+3]) &&
       value4.valid(theArgv[pos+4]))
   {
      value1.assign(theArgv[pos+1]);
      value2.assign(theArgv[pos+2]);
      value3.assign(theArgv[pos+3]);
      value4.assign(theArgv[pos+4]);
      remove(pos,5);
      return true;
   }
   else
   {
      return false;
   }
}

bool ossimArgumentParser::errors(ossimErrorSeverity severity) const
{
   for(ossimErrorMessageMap::const_iterator itr=theErrorMessageMap.begin();
       itr!=theErrorMessageMap.end();
       ++itr)
   {
      if (itr->second>=severity) return true;
   }
   return false;
}

void ossimArgumentParser::reportError(const std::string& message, ossimErrorSeverity severity)
{
   theErrorMessageMap[message]=severity;
}

void ossimArgumentParser::reportRemainingOptionsAsUnrecognized(ossimErrorSeverity severity)
{
   std::set<std::string> options;
   if (theUsage)
   {
      // parse the usage options to get all the option that the application can potential handle.
      for(ossimApplicationUsage::UsageMap::const_iterator itr=theUsage->getCommandLineOptions().begin();
          itr!=theUsage->getCommandLineOptions().end();
          ++itr)
      {
         const std::string& option = itr->first;
         std::string::size_type prevpos = 0, pos = 0;
         while ((pos=option.find(' ',prevpos))!=std::string::npos)
         {
            if (option[prevpos]=='-')
            {
               options.insert(std::string(option,prevpos,pos-prevpos));
            }
            prevpos=pos+1;
         }
         if (option[prevpos]=='-')
         {

            options.insert(std::string(option,prevpos,std::string::npos));
         }
      }

   }

   for(int pos=1;pos<argc();++pos)
   {
      // if an option and havn't been previous querried for report as unrecognized.
      if (isOption(pos) && options.find(theArgv[pos])==options.end())
      {
         reportError(getApplicationName() +": unrecognized option "+theArgv[pos],severity);
      }
   }
}

ossimArgumentParser::ossimErrorMessageMap& ossimArgumentParser::getErrorMessageMap()
{
   return theErrorMessageMap;
}
   
const ossimArgumentParser::ossimErrorMessageMap& ossimArgumentParser::getErrorMessageMap() const
{
   return theErrorMessageMap;
}

void ossimArgumentParser::writeErrorMessages(std::ostream& output, ossimErrorSeverity severity)
{
   for(ossimErrorMessageMap::iterator itr=theErrorMessageMap.begin();
       itr!=theErrorMessageMap.end();
       ++itr)
   {
      if (itr->second>=severity)
      {
         output<< getApplicationName() << ": " << itr->first << std::endl;
      }
   }
}
