//---
// License: MIT
//
// Author:  David Burken
//
// Description: Timer with a start, stop, reset and count.
//---
// $Id$
#ifndef ossimStopwatch_H
#define ossimStopwatch_H 1

#include <ossim/base/ossimConstants.h>
#include <chrono>

class OSSIM_DLL ossimStopwatch
{
public:
   ossimStopwatch();

   /* @brief Starts stopwatch. */
   void start();

   /* @brief Stops stopwatch and adds (now-start) to accumulative count. */
   void stop();

   /* @brief Gets count in seconds as a double.
    *
    * Note: If the stopwatch is running the return will be the:
    * 
    * accumulative_count + now - start.
    *
    * @return Accumulative count in seconds as a double.
    */
   double count() const;

   /**
    * @breif Zeroes out count and sets running state to false.
    */
   void reset();
   
private:
   bool m_running;
   std::chrono::steady_clock::time_point m_t1;
   double m_count;
};

#endif /* End of "#ifndef ossimStopwatch_H" */
