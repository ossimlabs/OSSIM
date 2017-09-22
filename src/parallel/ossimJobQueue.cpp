#include <ossim/parallel/ossimJobQueue.h>

#include <algorithm> /* for std::find */

ossimJobQueue::ossimJobQueue()
{
}

void ossimJobQueue::add(std::shared_ptr<ossimJob> job, 
                        bool guaranteeUniqueFlag)
{
   std::shared_ptr<Callback> cb;
   {
      {
         std::lock_guard<std::mutex> lock(m_jobQueueMutex);
         
         if(guaranteeUniqueFlag)
         {
            if(findByPointer(job) != m_jobQueue.end())
            {
               m_block.set(true);
               return;
            }
         }
         cb = m_callback;
      }
      if(cb) cb->adding(getSharedFromThis(), job);
      
      job->ready();
      m_jobQueueMutex.lock();
      m_jobQueue.push_back(job);
      m_jobQueueMutex.unlock();
   }
   if(cb)
   {
      cb->added(getSharedFromThis(), job);
   }
   m_block.set(true);
}

std::shared_ptr<ossimJob> ossimJobQueue::removeByName(const ossimString& name)
{
   std::shared_ptr<ossimJob> result;
   std::shared_ptr<Callback> cb;
   if(name.empty()) return result;
   {
      std::lock_guard<std::mutex> lock(m_jobQueueMutex);
      ossimJob::List::iterator iter = findByName(name);
      if(iter!=m_jobQueue.end())
      {
         result = *iter;
         m_jobQueue.erase(iter);
      }
      cb = m_callback;
   }      
   m_block.set(!m_jobQueue.empty());
   
   if(cb&&result)
   {
      cb->removed(getSharedFromThis(), result);
   }
   return result;
}
std::shared_ptr<ossimJob> ossimJobQueue::removeById(const ossimString& id)
{
   std::shared_ptr<ossimJob> result;
   std::shared_ptr<Callback> cb;
   if(id.empty()) return result;
   {
      std::lock_guard<std::mutex> lock(m_jobQueueMutex);
      ossimJob::List::iterator iter = findById(id);
      if(iter!=m_jobQueue.end())
      {
         result = *iter;
         m_jobQueue.erase(iter);
      }
      cb = m_callback;
      m_block.set(!m_jobQueue.empty());
   }
   if(cb&&result)
   {
      cb->removed(getSharedFromThis(), result);
   }
   return result;
}

void ossimJobQueue::remove(const std::shared_ptr<ossimJob> job)
{
   std::shared_ptr<ossimJob> removedJob;
   std::shared_ptr<Callback> cb;
   {
      std::lock_guard<std::mutex> lock(m_jobQueueMutex);
      ossimJob::List::iterator iter = std::find(m_jobQueue.begin(), m_jobQueue.end(), job);
      if(iter!=m_jobQueue.end())
      {
         removedJob = (*iter);
         m_jobQueue.erase(iter);
      }
      cb = m_callback;
   }
   if(cb&&removedJob)
   {
      cb->removed(getSharedFromThis(), removedJob);
   }
}

void ossimJobQueue::removeStoppedJobs()
{
   ossimJob::List removedJobs;
   std::shared_ptr<Callback> cb;
   {
      std::lock_guard<std::mutex> lock(m_jobQueueMutex);
      cb = m_callback;
      ossimJob::List::iterator iter = m_jobQueue.begin();
      while(iter!=m_jobQueue.end())
      {
         if((*iter)->isStopped())
         {
            removedJobs.push_back(*iter);
            iter = m_jobQueue.erase(iter);
         }
         else 
         {
            ++iter;
         }
      }
   }
   if(!removedJobs.empty())
   {
      if(cb)
      {
         ossimJob::List::iterator iter = removedJobs.begin();
         while(iter!=removedJobs.end())
         {
            cb->removed(getSharedFromThis(), (*iter));
            ++iter;
         }
      }
      removedJobs.clear();
   }
}

void ossimJobQueue::clear()
{
   ossimJob::List removedJobs(m_jobQueue);
   std::shared_ptr<Callback> cb;
   {
      std::lock_guard<std::mutex> lock(m_jobQueueMutex);
      m_jobQueue.clear();
      cb = m_callback;
   }
   if(cb)
   {
      for(ossimJob::List::iterator iter=removedJobs.begin();iter!=removedJobs.end();++iter)
      {
         cb->removed(getSharedFromThis(), (*iter));
      }
   }
}

std::shared_ptr<ossimJob> ossimJobQueue::nextJob(bool blockIfEmptyFlag,
                                                 ossim_int64 waitTimeInMillis)
{
   m_jobQueueMutex.lock();
   bool emptyFlag = m_jobQueue.empty();
   m_jobQueueMutex.unlock();
   if (blockIfEmptyFlag && emptyFlag)
   {
      if(waitTimeInMillis < 0 )
      {
         m_block.block();
      }
      else
      {
         m_block.block(waitTimeInMillis);
      }
   }
   
   std::shared_ptr<ossimJob> result;
   std::lock_guard<std::mutex> lock(m_jobQueueMutex);
   
   if (m_jobQueue.empty())
   {
      m_block.set(false);
      return result;
   }
   
   ossimJob::List::iterator iter= m_jobQueue.begin();
   while((iter != m_jobQueue.end())&&
         (((*iter)->isCanceled())))
   {
      (*iter)->finished(); // mark the ob as being finished 
      iter = m_jobQueue.erase(iter);
   }
   if(iter != m_jobQueue.end())
   {
      result = *iter;
      m_jobQueue.erase(iter);
   }
   m_block.set(!m_jobQueue.empty());

   return result;
}
void ossimJobQueue::releaseBlock()
{
   m_block.release();
}

void ossimJobQueue::releaseOneBlock()
{
   m_block.releaseOne();
}

bool ossimJobQueue::isEmpty()const
{
   // std::lock_guard<std::mutex> lock(m_jobQueueMutex);
   // return m_jobQueue.empty();
   m_jobQueueMutex.lock();
   bool result =  m_jobQueue.empty();
   m_jobQueueMutex.unlock();
   return result;
}

ossim_uint32 ossimJobQueue::size()
{
   std::lock_guard<std::mutex> lock(m_jobQueueMutex);
   return (ossim_uint32) m_jobQueue.size();
}

ossimJob::List::iterator ossimJobQueue::findById(const ossimString& id)
{
   if(id.empty()) return m_jobQueue.end();
   ossimJob::List::iterator iter = m_jobQueue.begin();
   while(iter != m_jobQueue.end())
   {
      if(id == (*iter)->id())
      {
         return iter;
      }
      ++iter;
   }  
   return m_jobQueue.end();
}

ossimJob::List::iterator ossimJobQueue::findByName(const ossimString& name)
{
   if(name.empty()) return m_jobQueue.end();
   ossimJob::List::iterator iter = m_jobQueue.begin();
   while(iter != m_jobQueue.end())
   {
      if(name == (*iter)->name())
      {
         return iter;
      }
      ++iter;
   }  
   return m_jobQueue.end();
}

ossimJob::List::iterator ossimJobQueue::findByPointer(const std::shared_ptr<ossimJob> job)
{
   return std::find(m_jobQueue.begin(),
                    m_jobQueue.end(),
                    job);
}

ossimJob::List::iterator ossimJobQueue::findByNameOrPointer(const std::shared_ptr<ossimJob> job)
{
   ossimString n = job->name();
   ossimJob::List::iterator iter = std::find_if(m_jobQueue.begin(), m_jobQueue.end(), [n, job](const std::shared_ptr<ossimJob> jobIter){
      bool result = (jobIter == job);
      if(!result&&!n.empty()) result = jobIter->name() == n;
      return result;
   });
   // ossimJob::List::iterator iter = m_jobQueue.begin();
   // while(iter != m_jobQueue.end())
   // {
   //    if((*iter) == job)
   //    {
   //       return iter;
   //    }
   //    else if((!n.empty())&&
   //            (job->name() == (*iter)->name()))
   //    {
   //       return iter;
   //    }
   //    ++iter;
//   }  
   
   return iter;
}

bool ossimJobQueue::hasJob(std::shared_ptr<ossimJob> job)
{
   ossimJob::List::const_iterator iter = m_jobQueue.begin();
   while(iter != m_jobQueue.end())
   {
      if(job == (*iter))
      {
         return true;
      }
      ++iter;
   }
   
   return false;
}

void ossimJobQueue::setCallback(std::shared_ptr<Callback> c)
{
   std::lock_guard<std::mutex> lock(m_jobQueueMutex);
   m_callback = c;
}

std::shared_ptr<ossimJobQueue::Callback> ossimJobQueue::callback()
{
   std::lock_guard<std::mutex> lock(m_jobQueueMutex);
   return m_callback;
}
