ADD_GEMRB_PLUGIN(2DAImporter 2DAImporter.cpp 2DAImporter.h)

ADD_GEMRB_PLUGIN_TEST(2DAImporter
  2DAImporter.cpp
  ../../tests/2DAImporter/Test_2DAImporter.cpp
)
