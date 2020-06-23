#include "Parameters.h"

using namespace ofxVimba;

Parameters::Parameters() : logger("Parameters") { ; }
Parameters::~Parameters() {
  if (isBound()) unbind();
}

void Parameters::update() {
  for (const auto& entry : parameterMap) {
    entry.second->update();
  }
}

void Parameters::bind(const std::shared_ptr<Device>& nextDevice) {
  if (isBound()) unbind();

  device = nextDevice;
  logger.setScope(device->getId());
  logger.verbose("Binding parameters to device");

  buildParameters();

  // Rebuild our parameter group
  buildGroup();
}

void Parameters::unbind() {
  if (!isBound()) return;

  // Unbind all the parameters
  for (const auto& entry : parameterMap) {
    entry.second->unbind();
  }

  releaseParameters();

  // Rebuild our parameter group
  buildGroup();

  // Unbind the device
  device = nullptr;
  logger.verbose("Unbound from device");
  logger.clearScope();
}

void Parameters::buildParameters() {
  logger.verbose("Discovering device parameters");

  // add custom parameters
  addParameter(std::make_shared<ExposureParameter>());
  addParameter(std::make_shared<FrameRateParameter>());

  // add features
  AVT::VmbAPI::FeaturePtrVector features;
  auto error = device->getHandle()->GetFeatures(features);
  if (error != VmbErrorSuccess) return;

  std::string featureName;
  std::string featureCategory;
  VmbFeatureDataType featureType;

  for (const auto& feature : features) {
    error = feature->GetName(featureName);

    if (error == VmbErrorSuccess) error = feature->GetCategory(featureCategory);
    if (error == VmbErrorSuccess) error = feature->GetDataType(featureType);
    if (error == VmbErrorSuccess) {
      auto parameter = buildParameter(featureName, featureType);

      if (parameter) {
        parameter->setCategory(featureCategory);
        addParameter(parameter);
      }
    }
  }
}

void Parameters::releaseParameters() {
  for (auto it = parameterMap.begin(); it != parameterMap.end();) {
    it = parameterMap.erase(it);
  }
}

const std::shared_ptr<Parameter> Parameters::buildParameter(
    const std::string& name, const VmbFeatureDataType& dataType) const {
  switch (dataType) {
    case VmbFeatureDataBool:
      return std::make_shared<GenericFeatureParameter<bool>>(name);
    case VmbFeatureDataString:
      return std::make_shared<GenericFeatureParameter<std::string>>(name);
    case VmbFeatureDataInt:
      return std::make_shared<NumericFeatureParameter<int64_t>>(name);
    case VmbFeatureDataFloat:
      return std::make_shared<NumericFeatureParameter<double>>(name);
    case VmbFeatureDataEnum:
      return std::make_shared<EnumFeatureParameter>(name);
    case VmbFeatureDataCommand:
      return std::make_shared<CommandFeatureParameter>(name);
    default:
      logger.warning("Unsupported data type: " + ofToString(dataType) +
                     " for feature: " + name);
      return nullptr;
  }
}

bool Parameters::addParameter(const std::shared_ptr<Parameter>& parameter) {
  if (parameter == nullptr) return false;
  auto descriptor = parameter->getDescriptor();
  auto prevParameter = getParameter(descriptor);

  if (prevParameter) {
    if (prevParameter->getType() == parameter->getType()) {
      logger.notice("Parameter '" + descriptor + "' already known");
      return true;
    } else {
      logger.warning("Parameter '" + descriptor +
                     "' changed type, replacing parameter");
      if (isBound()) prevParameter->unbind();
    }
  }

  parameterMap.insert({descriptor, parameter});

  if (isBound()) parameter->bind(device);

  return true;
}

const std::shared_ptr<Parameter> Parameters::getParameter(
    const std::string& descriptor) const {
  auto it = parameterMap.find(descriptor);
  if (it == parameterMap.end()) return nullptr;
  return it->second;
}

std::vector<string> Parameters::listParameters(bool verbose) {
  if (!device) {
    logger.warning("cannot list parameters, device not found");
    return std::vector<string>();
  }

  vector<string> descriptors;

  for (auto& p : parameterMap) {
    descriptors.push_back(p.second->getDescriptor());
  }

  if (verbose) {
    cout << endl;
    cout << "##################################################################"
            "########################"
         << endl;
    cout << "##  LISTING PARAMETERS" << endl << "##" << endl;
    ;

    for (auto& d : descriptors) {
      cout << "##  " + d << endl;
    }

    cout << "##" << endl;
    cout << "################################################################"
            "##########################"
         << endl;
    cout << endl;
  }

  return descriptors;
}

// -- PARAMETER GROUP
// --------------------------------------------------------

void Parameters::buildGroup() {
  parameterGroup.clear();
  parameterGroup.setName("Camera: " +
                         ofToString(device ? device->getId() : "Disconnected"));

  if (!device) return;

  // Add all our discovered features in the order they appear in the map
  // ofParameterGroup features("Features");
  for (const auto& entry : parameterMap) {
    auto viP = entry.second;
    auto ofP = viP->getAbstractParameter();

    if (ofP) {
      auto category = viP->getCategory();
      auto group = getGroup(parameterGroup, category);

      if (!parameterGroup.contains(ofP->getName())) {
        group.add(*ofP.get());
      }
    }
  }
}

ofParameterGroup Parameters::getGroup(ofParameterGroup& root,
                                      const std::vector<std::string>& tree) {
  auto group = root;

  for (auto node : tree) {
    if (!group.contains(node)) {
      ofParameterGroup nodeGroup(node);
      group.add(nodeGroup);
    }

    group = group.getGroup(node);
  }

  return group;
}

ofParameterGroup Parameters::get(std::vector<string> _request,
                                 bool useCategories) {
  //  auto requestedParameters = std::make_shared<ofParameterGroup>();
  ofParameterGroup requestedParameters;
  requestedParameters.setName(
      "Camera: " + ofToString(device ? device->getId() : "Disconnected"));

  if (!device) {
    logger.verbose("parameters not avaialable, device not connected");
    return requestedParameters;
  }

  for (auto& req : _request) {
    vector<string> reqTree = getCategoryTree(req);
    string lastNode = reqTree.back();
    reqTree.pop_back();

    //  auto subGroup = getGroup(requestedParameters, reqVec);

    ofParameterGroup* srcGroup = &parameterGroup;
    ofParameterGroup* dstGroup = &requestedParameters;
    for (int i = 0; i < reqTree.size(); i++) {
      string node = reqTree[i];
      if (srcGroup->contains(node)) {
        srcGroup = &srcGroup->getGroup(node);
        if (useCategories) {
          if (!dstGroup->contains(node)) {
            ofParameterGroup nodeGroup(node);
            dstGroup->add(nodeGroup);
          }
          dstGroup = &dstGroup->getGroup(node);
        }
      } else {
        logger.warning("parameterGroup " + srcGroup->getName() +
                       " does not include " + node);
      }
    }

    if (srcGroup->contains(lastNode)) {
      dstGroup->add(srcGroup->get(lastNode));
    } else {
      logger.warning("parameterGroup " + srcGroup->getName() +
                     " does not include " + lastNode);
    }
  }

  return requestedParameters;
}

std::vector<std::string> Parameters::getCategoryTree(
    const std::string& nextCategoryName) {
  std::vector<std::string> nextCategory;

  // Split the string by /
  boost::split(nextCategory, nextCategoryName, boost::is_any_of("/"));

  // Remove empty strings
  nextCategory.erase(
      std::remove_if(nextCategory.begin(), nextCategory.end(),
                     [](const std::string& s) { return s == ""; }),
      nextCategory.end());

  return nextCategory;
}
