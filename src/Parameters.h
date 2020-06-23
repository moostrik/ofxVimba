#pragma once

#include <boost/algorithm/string.hpp>
#include <string>
#include <vector>

#include "VimbaCPP/Include/VimbaCPP.h"
#include "ofMain.h"
#include "ofxXmlSettings.h"

#include "Device.h"
#include "Logger.h"
#include "System.h"

#include "Parameter/CommandFeatureParameter.h"
#include "Parameter/EnumFeatureParameter.h"
#include "Parameter/ExposureParameter.hpp"
#include "Parameter/FrameRateParameter.hpp"
#include "Parameter/GenericFeatureParameter.hpp"
#include "Parameter/NumericFeatureParameter.hpp"
#include "Parameter/Parameter.h"

namespace ofxVimba {

static const std::vector<std::string> PARAMETERS_BASIC = {
    "/Custom", "/ImageFormat/Width", "/ImageFormat/Height",
    "/ImageFormat/OffsetX", "/ImageFormat/OffsetY"};

static const std::vector<std::string> PARAMETERS_AUTOEXPOSURE = {
    "/ImageFormat/Width",
    "/ImageFormat/Height",
    "/ImageFormat/OffsetX",
    "/ImageFormat/OffsetY",
    "/Custom/CustomFrameRate",
    "/Custom/CustomExposure",
    "/Controls/Exposure/ExposureAuto",
    "/Controls/Exposure/ExposureAutoControl/ExposureAutoTarget",
    "/Controls/Exposure/ExposureAutoControl/ExposureAutoRate",
    "/Controls/Exposure/ExposureAutoControl/ExposureAutoOutliers"};

class Parameters {
 public:
  Parameters(Parameters const&) = delete;
  Parameters& operator=(Parameters const&) = delete;

  Parameters();
  ~Parameters();

  void update();

  // Bind and unbind this instance to a specific device
  void bind(const std::shared_ptr<Device>& device);
  void unbind();

  bool isBound() const { return device != nullptr; }
  bool isUnbound() const { return device == nullptr; }

  std::vector<string> listParameters(bool verbose = true);

  ofParameterGroup& get() { return parameterGroup; }
  ofParameterGroup get(std::vector<string> _request, bool useCategories = true);

 private:
  Logger logger;

  std::shared_ptr<Device> device;

  std::map<const std::string, shared_ptr<Parameter> > parameterMap;

  AVT::VmbAPI::FeaturePtrVector getAvalableFeatures();

  void buildParameters();
  void releaseParameters();

  const std::shared_ptr<Parameter> buildParameter(
      const std::string& name, const VmbFeatureDataType& dataType) const;
  bool addParameter(const std::shared_ptr<Parameter>& parameter);
  const std::shared_ptr<Parameter> getParameter(const std::string& id) const;

  ofParameterGroup parameterGroup;
  void buildGroup();
  ofParameterGroup getGroup(ofParameterGroup& root,
                            const std::vector<std::string>& tree);

  std::vector<std::string> getCategoryTree(const std::string& nextCategoryName);
};
}  // namespace ofxVimba
