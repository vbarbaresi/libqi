/*
** qipaththrow_test.cpp
** Login : <hcuche@hcuche-de>
** Started on  Tue May 10 11:24:23 2011 Herve Cuche
** $Id$
**
** Author(s):
**  - Herve Cuche <hcuche@aldebaran-robotics.com>
**
*/

#include <gtest/gtest.h>

#include <qi/path/sdklayout.hpp>
#include <qi/path.hpp>
#include <qi/error.hpp>


//should not throw, it depends on qi::program()
#if 0
TEST(qiPathThrowTests, qiPathFuncThrow)
{
  ASSERT_THROW({qi::path::sdkPrefix();}, qi::os::QiException);
  ASSERT_THROW({qi::path::getSdkPrefixes();}, qi::os::QiException);
  ASSERT_THROW({qi::path::addOptionalSdkPrefix("/build/sdk");}, qi::os::QiException);
  ASSERT_THROW({qi::path::clearOptionalSdkPrefix();}, qi::os::QiException);
  ASSERT_THROW({qi::path::findBin("qipath_test");}, qi::os::QiException);
  ASSERT_THROW({qi::path::findLib("libqi.so");}, qi::os::QiException);
  ASSERT_THROW({qi::path::findConf("naoqi", "autoinit");}, qi::os::QiException);
  ASSERT_THROW({qi::path::findData("naoqi", "");}, qi::os::QiException);
  ASSERT_THROW({qi::path::confPaths();}, qi::os::QiException);
  ASSERT_THROW({qi::path::dataPaths();}, qi::os::QiException);
  ASSERT_THROW({qi::path::binPaths();}, qi::os::QiException);
  ASSERT_THROW({qi::path::libPaths();}, qi::os::QiException);
  ASSERT_THROW({qi::path::userWritableDataPath("naoqi", "");}, qi::os::QiException);
  ASSERT_THROW({qi::path::userWritableConfPath("naoqi", "");}, qi::os::QiException);
}

TEST(qiPathThrowTests, sdkLayoutFuncThrow)
{

  qi::SDKLayout* sdkl = new qi::SDKLayout();

  ASSERT_THROW({sdkl->sdkPrefix();}, qi::os::QiException);
  ASSERT_THROW({sdkl->getSdkPrefixes();}, qi::os::QiException);
  ASSERT_THROW({sdkl->addOptionalSdkPrefix("/build/sdk");}, qi::os::QiException);
  ASSERT_THROW({sdkl->clearOptionalSdkPrefix();}, qi::os::QiException);
  ASSERT_THROW({sdkl->findBin("qipath_test");}, qi::os::QiException);
  ASSERT_THROW({sdkl->findLib("libqi.so");}, qi::os::QiException);
  ASSERT_THROW({sdkl->findConf("naoqi", "autoinit");}, qi::os::QiException);
  ASSERT_THROW({sdkl->findData("naoqi", "");}, qi::os::QiException);
  ASSERT_THROW({sdkl->confPaths();}, qi::os::QiException);
  ASSERT_THROW({sdkl->dataPaths();}, qi::os::QiException);
  ASSERT_THROW({sdkl->binPaths();}, qi::os::QiException);
  ASSERT_THROW({sdkl->libPaths();}, qi::os::QiException);
  ASSERT_THROW({sdkl->userWritableDataPath("naoqi", "");}, qi::os::QiException);
  ASSERT_THROW({sdkl->userWritableConfPath("naoqi", "");}, qi::os::QiException);

  delete sdkl;
}
#endif
