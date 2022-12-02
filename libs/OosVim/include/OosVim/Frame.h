#pragma once

#include "VimbaCPP/Include/VimbaCPP.h"

#include "Common.h"
#include "Device.h"

namespace OosVimba {
class Stream;
class Frame {
  friend Stream;

 public:
  Frame(std::shared_ptr<Device>& _device);
  ~Frame();

  const uint64_t& getId() const { return id; }
  const uint64_t& getTimestamp() const { return timestamp; }
  const uint64_t& geFrameCount() const { return frameCount; }
  const uint32_t& getWidth() const { return width; }
  const uint32_t& getHeight() const { return height; }
  const uint32_t& getImageSize() const { return size; }
  const unsigned char* getImageData() const { return data; }
  const VmbPixelFormatType& getImageFormat() const { return format; }
  const std::shared_ptr<Device>& getDevice() const { return device; }

  bool getAncillaryFeature(const std::string& name, AVT::VmbAPI::FeaturePtr& feature) const;

  template <typename ValueType>
  bool getAncillary(const std::string& name, ValueType& value) const {
    if (SP_ISNULL(ancilleryData)) return false;
    AVT::VmbAPI::FeaturePtr feature;
    if (getAncillaryFeature(name, feature)) {
      return getAncillary(feature, value);
    }
    return false;
  }

  template <typename ValueType>
  bool getAncillary(const AVT::VmbAPI::FeaturePtr& feature, ValueType& value) const {
    return getFeature(feature, value);
  }

 protected:
  bool load(const AVT::VmbAPI::FramePtr& framePtr);

 private:
  std::shared_ptr<Device> device;

  uint64_t id;
  uint64_t timestamp;
  uint64_t frameCount;
  uint32_t width;
  uint32_t height;
  uint32_t size;

  // Original pixel format
  VmbPixelFormatType format;

  // Pointer to the data
  unsigned char* data;

  // Ancillery data access
  AVT::VmbAPI::AncillaryDataPtr ancilleryData;
  AVT::VmbAPI::FeaturePtrVector ancilleryFeatures;
};
}  // namespace OosVimba
