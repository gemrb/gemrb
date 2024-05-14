ADD_GEMRB_PLUGIN (BMPImporter BMPImporter.cpp)

ADD_GEMRB_PLUGIN_TEST(BMPImporter
  BMPImporter.cpp
  ../../tests/BMPImporter/Test_BMPImporter.cpp
)
