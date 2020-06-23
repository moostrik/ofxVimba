#include "Parameter.h"

using namespace ofxVimba;

void Parameter::setCategory(const std::string& nextCategoryName) {
  descriptor = nextCategoryName + "/" + name;
  std::vector<std::string> nextCategory;

  // Split the string by /
  boost::split(nextCategory, nextCategoryName, boost::is_any_of("/"));

  // Remove empty strings
  nextCategory.erase(
      std::remove_if(nextCategory.begin(), nextCategory.end(),
                     [](const std::string& s) { return s == ""; }),
      nextCategory.end());

  setCategory(nextCategory);
}

bool Parameter::bind(const std::shared_ptr<Device>& nextDevice) {
  device = nextDevice;
  return true;
}

bool Parameter::unbind() {
  SP_RESET(device);
  return true;
};
