/*
 * Copyright (c) 2012, 2013 Aldebaran Robotics. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the COPYING file.
 */

#include <fstream>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/filesystem.hpp>
#include <boost/chrono.hpp>

#include <qi/path.hpp>
#include <qi/os.hpp>
#include <qi/log.hpp>
#include <qi/types.hpp>

qiLogCategory("qi.os");

namespace bfs = boost::filesystem;

namespace qi {
  namespace os {
    /* Have the static variable we need inside the function so that we
     * give correct results at initialization time, but also force
     * initialization so that timestamp 0 corresponds to program start time.
     */
    static int64_t _unused_base = ustime();

    int64_t ustime()
    {
      static bool initialized = false;
      static int64_t base;
      timeval tv;
      gettimeofday(&tv);
      if (!initialized)
      {
        base = tv.tv_sec*1000000LL + tv.tv_usec;
        initialized = true;
      }
      return tv.tv_sec*1000000LL + tv.tv_usec - base;
    }


    static inline void normalize_tv(qi::os::timeval *t)
    {
      const qi::int64_t usecPerSec = 1000 * 1000;
      t->tv_sec += t->tv_usec/usecPerSec;
      t->tv_usec %= usecPerSec;
      if (t->tv_usec < 0) {
        t->tv_usec += usecPerSec;
        --t->tv_sec;
      }
    }

    qi::os::timeval operator+(const qi::os::timeval &lhs,
                              const qi::os::timeval &rhs)
    {
      qi::os::timeval res;
      res.tv_sec = lhs.tv_sec + rhs.tv_sec;
      res.tv_usec = lhs.tv_usec + rhs.tv_usec;
      normalize_tv(&res);

      return res;
    }

    qi::os::timeval operator+(const qi::os::timeval &lhs,
                              long                   us)
    {
      const long usecPerSec = 1000 * 1000;

      qi::os::timeval res;
      res.tv_sec = lhs.tv_sec + (us / usecPerSec);
      res.tv_usec = lhs.tv_usec + (us % usecPerSec);
      normalize_tv(&res);

      return res;
    }

    qi::os::timeval operator-(const qi::os::timeval &lhs,
                              const qi::os::timeval &rhs)
    {
      qi::os::timeval res;
      res.tv_sec = lhs.tv_sec - rhs.tv_sec;
      res.tv_usec = lhs.tv_usec - rhs.tv_usec;
      normalize_tv(&res);

      return res;
    }

    qi::os::timeval operator-(const qi::os::timeval &lhs,
                              long                   us)
    {
      const long usecPerSec = 1000 * 1000;

      qi::os::timeval res;
      res.tv_sec = lhs.tv_sec - (us / usecPerSec);
      res.tv_usec = lhs.tv_usec - (us % usecPerSec);
      normalize_tv(&res);

      return res;
    }

    /* getMachineId will return an uuid as a string.
     * If the uuid is not created yet, it will generate it and store it
     * in machine_id file.
     * Otherwise it will read the file to retrieve the uuid and cache it
     * for further uses.
     * If the file is removed, a new file with a different uuid will be
     * created.
     */
    std::string getMachineId()
    {
      static bool initialized = false;
      static std::string idString;

      if (initialized)
        return idString;

      static boost::mutex mutex;
      boost::mutex::scoped_lock lock(mutex);
      if (initialized)
        return idString;
      std::string idFilePath(qi::path::userWritableConfPath("qimessaging", "machine_id"));
      std::ifstream idFile(bfs::path(idFilePath, qi::unicodeFacet()).string().c_str());

      if (idFile)
      {
        idFile >> idString;
        idFile.close();
        initialized = true;
        if (!idString.empty()) {
          return idString;
        } //else machine id is empty...
        qiLogWarning() << "machine_id is empty, generating a new one";
      }

      std::ofstream newIdFile(idFilePath.c_str());

      idString = generateUuid();
      if (newIdFile)
      {
        newIdFile << idString;
        newIdFile.close();
        initialized = true;
      }
      else
      {
        qiLogError() << "Unable to create file: '" << idFilePath << "'";
      }

      return idString;
    }

    std::string generateUuid()
    {
      boost::uuids::uuid u = boost::uuids::random_generator()();
      return to_string(u);
    }

    void symlink(const qi::Path& source, const qi::Path& destination)
    {
      bfs::create_symlink(bfs::path(source.str(), qi::unicodeFacet()), bfs::path(destination.str(), qi::unicodeFacet()));
    }

    int gettimeofday(qi::os::timeval *t)
    {
      namespace bc = boost::chrono;

      const bc::system_clock::duration timeSinceEpoch = bc::system_clock::now().time_since_epoch();
      const bc::microseconds usSinceEpoch = bc::duration_cast<bc::microseconds>(timeSinceEpoch);
      const bc::seconds secsSinceEpoch = bc::duration_cast<bc::seconds>(timeSinceEpoch);

      t->tv_sec = secsSinceEpoch.count();
      t->tv_usec = (usSinceEpoch - secsSinceEpoch).count();

      return 0;
    }
  }
}
