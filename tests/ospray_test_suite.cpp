#include "ospray_test_fixture.h"

OSPRayEnvironment * ospEnv;

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ospEnv = new OSPRayEnvironment(argc, argv);
  AddGlobalTestEnvironment(ospEnv);
  return RUN_ALL_TESTS();
}
