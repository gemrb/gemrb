ADD_GEMRB_PLUGIN(INIImporter INIImporter.cpp)

ADD_GEMRB_PLUGIN_TEST(INIImporter
  INIImporter.cpp
  ../../tests/INIImporter/Test_INIImporter.cpp
)
