/*
**
** Author(s):
**  - Herve Cuche <hcuche@aldebaran-robotics.com>
**  - Cedric GESTES <gestes@aldebaran-robotics.com>
**
** Copyright (C) 2011 Aldebaran Robotics
*/

#include <qi/application.hpp>
#include <qi/os.hpp>
#include <numeric>
#include <boost/filesystem.hpp>
#include "src/filesystem.hpp"
#include <qi/locale.hpp>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif

namespace qi {
  static int         globalArgc = -1;
  static char**      globalArgv = 0;
  static std::string globalPrefix;
  static std::string globalProgram;

  static std::string guess_app_from_path(int argc, const char *argv[])
  {
    boost::filesystem::path execPath(argv[0], qi::utf8facet());
    execPath = boost::filesystem::system_complete(execPath).make_preferred();
    execPath = boost::filesystem::path(detail::normalizePath(execPath.string(qi::utf8facet())), qi::utf8facet());

    //arg0 does not exists, or is not a program (directory)
    if (!boost::filesystem::exists(execPath) || boost::filesystem::is_directory(execPath))
    {
      std::string filename = execPath.filename().string(qi::utf8facet());
      std::string envPath = qi::os::getenv("PATH");
      size_t begin = 0;
     #ifndef _WIN32
      size_t end = envPath.find(":", begin);
     #else
      size_t end = envPath.find(";", begin);
     #endif
      while (end != std::string::npos)
      {
        std::string realPath = "";

        realPath = envPath.substr(begin, end - begin);
        boost::filesystem::path p(realPath, qi::utf8facet());
        p /= filename;
        p = boost::filesystem::system_complete(p).make_preferred();
        p = boost::filesystem::path(detail::normalizePath(p.string(qi::utf8facet())), qi::utf8facet());

        if (boost::filesystem::exists(p) && !boost::filesystem::is_directory(p))
          return p.string(qi::utf8facet());

        begin = end + 1;
       #ifndef _WIN32
        end = envPath.find(":", begin);
       #else
        end = envPath.find(";", begin);
       #endif
      }
    }
    else
      return execPath.string(qi::utf8facet());
    return std::string();
  }

  void init(int argc, char *argv[])
  {
    globalArgc = argc;
    globalArgv = argv;
  }

  int argc()
  {
    return globalArgc;
  }

  const char** argv()
  {
    return (const char**)globalArgv;
  }

/*
  http://stackoverflow.com/questions/1023306/finding-current-executables-path-without-proc-self-exe
  Some OS-specific interfaces:
    Mac OS X: _NSGetExecutablePath() (man 3 dyld)
    Linux   : readlink /proc/self/exe
    Solaris : getexecname()
    FreeBSD : sysctl CTL_KERN KERN_PROC KERN_PROC_PATHNAME -1
    BSD with procfs: readlink /proc/curproc/file
    Windows : GetModuleFileName() with hModule = NULL

    The portable (but less reliable) method is to use argv[0].
    Although it could be set to anything by the calling program,
    by convention it is set to either a path name of the executable
    or a name that was found using $PATH.

    Some shells, including bash and ksh, set the environment variable "_"
    to the full path of the executable before it is executed. In that case
    you can use getenv("_") to get it. However this is unreliable because
    not all shells do this, and it could be set to anything or be left over
    from a parent process which did not change it before executing your program.
*/
  const char *program()
  {
    if (!globalProgram.empty())
      return globalProgram.c_str();

#ifdef __APPLE__
    {
      char *fname = (char *)malloc(PATH_MAX);
      uint32_t sz = PATH_MAX;
      fname[0] = 0;
      int ret;
      ret = _NSGetExecutablePath(fname, &sz);
      if (ret == 0)
      {
        globalProgram = fname;
        globalProgram = detail::normalizePath(globalProgram);
      }
      else
      {
        globalProgram = guess_app_from_path(::qi::argc(), ::qi::argv());
    }
      free(fname);
    }
#elif __linux__
    boost::filesystem::path p("/proc/self/exe");
    boost::filesystem::path fname = boost::filesystem::read_symlink(p);

    if (!boost::filesystem::is_empty(fname))
      globalProgram = fname.string().c_str();
    else
      globalProgram = guess_app_from_path(::qi::argc(), ::qi::argv());
#elif _WIN32
    char *fname = (char *)malloc(MAX_PATH); //always use MAX_PATH for filepaths
    int ret = GetModuleFileName(NULL, fname, MAX_PATH);
    if (ret != 0)
      globalProgram = fname;
    else
      globalProgram = guess_app_from_path(::qi::argc(), ::qi::argv());
    free(fname);
#else
    globalProgram = guess_app_from_path(::qi::argc(), ::qi::argv());
#endif
    return globalProgram.c_str();

  }

};
