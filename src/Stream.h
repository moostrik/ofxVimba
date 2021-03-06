#pragma once

#include <atomic>
#include <chrono>
#include <mutex>

#include "VimbaCPP/Include/VimbaCPP.h"
#include "ofMain.h"

#include "Device.h"
#include "Frame.h"
#include "Logger.h"

namespace ofxVimba {
static const uint64_t CAMERA_STALLED_TIMEOUT = 1500;
static const uint64_t CAMERA_INITIALIZE_TIMEOUT = 1000;

class StreamObserver;
class Stream {
  friend StreamObserver;

 public:
  Stream(Stream const&) = delete;
  Stream& operator=(Stream const&) = delete;

  Stream(const std::shared_ptr<Device> device, unsigned int bufferSize = 4);
  ~Stream();

  bool isRunning() const { return running.load(); };
  bool isCapturing() const { return capturing.load(); };
  bool isResized() const;
  bool isStalled() const;
  bool isAvailable() const;

  void start();
  void stop();

  ofEvent<const std::shared_ptr<Frame>> onFrame;

 protected:
  void run();
  bool open();
  bool close();

  // Process frames
  bool receive(AVT::VmbAPI::FramePtr frame);

  // Prepare and teardown stream
  bool prepare();
  bool teardown();

  // Frame allocation
  bool isAllocated(const VmbInt64_t& size) const;
  bool allocate();
  bool deallocate();

  // Start and stop the observer
  void observe();
  void unobserve();

  // Frame queuing
  bool queue();
  bool flush();

 private:
  Logger logger;

  std::shared_ptr<Device> device;
  AVT::VmbAPI::FramePtrVector frames;
  SP_DECL(StreamObserver) observer;

  // Thread and communication
  std::mutex mutex;
  std::shared_ptr<std::thread> thread;
  std::condition_variable signal;

  // State flags
  std::atomic<bool> running;
  std::atomic<bool> capturing;

  // Timestamps
  std::atomic<uint64_t> connectedAt;
  std::atomic<uint64_t> resizedAt;
  std::atomic<uint64_t> frameAt;
};

class StreamObserver : public AVT::VmbAPI::IFrameObserver,
                       public AVT::VmbAPI::IFeatureObserver {
 public:
  StreamObserver(Stream& stream)
      : AVT::VmbAPI::IFrameObserver(stream.device->getHandle()),
        stream(stream){};

  void start();
  void stop();

  void FrameReceived(AVT::VmbAPI::FramePtr frame) override;
  void FeatureChanged(const AVT::VmbAPI::FeaturePtr&) override;

 private:
  Stream& stream;
  std::mutex mutex;
  bool running = false;
};
}  // namespace ofxVimba
