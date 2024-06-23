//---
// License: MIT
//
// Author:  David Burken
//
// Description: Timer with a start, stop, reset and count.
//---
// $Id$

#include <ossim/base/ossimStopwatch.h>

ossimStopwatch::ossimStopwatch()
   : m_running(false),
     m_t1(),
     m_count(0.0)
{
}

void ossimStopwatch::start()
{
   m_t1 = std::chrono::steady_clock::now();
   m_running = true;
}

void ossimStopwatch::stop()
{
   if ( m_running )
   {
      std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
      std::chrono::duration<double> time_span =
         std::chrono::duration_cast<std::chrono::duration<double>>(t2 - m_t1);
      m_count += time_span.count();
      m_running = false;
   }
}

double ossimStopwatch::count() const
{
   double result = m_count;
   if ( m_running )
   {
      std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
      std::chrono::duration<double> time_span =
         std::chrono::duration_cast<std::chrono::duration<double>>(t2 - m_t1);
      result += time_span.count();
   }
   return result;
}

void ossimStopwatch::reset()
{
   m_count = 0.0;
   m_running = false;
}
