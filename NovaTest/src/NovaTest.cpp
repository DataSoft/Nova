#include <gtest/gtest.h>


#include "tester_Config.h"
#include "tester_EvidenceTable.h"
#include "tester_Suspect.h"
#include "tester_ClassificationEngine.h"
#include "tester_RequestMessage.h"
#include "tester_VendorMacDb.h"
#include "tester_HoneydConfiguration.h"
#include "tester_WhitelistConfiguration.h"
#include "tester_Database.h"
#include "tester_messageSerialization.h"
#include "tester_Profile.h"

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
