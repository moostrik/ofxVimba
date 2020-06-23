#include "Stream.h"

using namespace ofxVimba;

Stream::Stream(const std::shared_ptr<Device> device,
               const unsigned int bufferSize)
    : logger("Stream"),
      device(device),
      running(false),
      capturing(false),
      connectedAt(0),
      resizedAt(0),
      frameAt(0) {
  logger.setScope(device->getId());
  frames.resize(bufferSize);

  SP_SET(observer, new StreamObserver(*this));
}

Stream::~Stream() { stop(); }

bool Stream::isStalled() const {
  auto now = ofGetElapsedTimeMillis();
  if (now - frameAt > CAMERA_STALLED_TIMEOUT) {
    return now - connectedAt > CAMERA_INITIALIZE_TIMEOUT;
  }

  return false;
}

bool Stream::isResized() const { return resizedAt > connectedAt; }

bool Stream::isAvailable() const {
  AVT::VmbAPI::FeaturePtr streamIdFeature;
  if (device->locate("StreamID", streamIdFeature)) {
    return !SP_ISNULL(streamIdFeature);
  }

  return false;
}

void Stream::start() {
  std::unique_lock<std::mutex> lock(mutex);
  if (running) return;

  running = true;
  thread = std::make_shared<std::thread>(std::bind(&Stream::run, this));
}

void Stream::stop() {
  std::shared_ptr<std::thread> threadToKill;

  {
    std::unique_lock<std::mutex> lock(mutex);
    running = false;
    thread.swap(threadToKill);
  }

  if (threadToKill) {
    // Wake up the threadToKill and wait for it to complete
    signal.notify_all();
    if (threadToKill->joinable()) threadToKill->join();
  }
}

void Stream::run() {
  std::unique_lock<std::mutex> lock(mutex);
  std::chrono::milliseconds timeout(100);

  logger.verbose("Setting up stream");

  while (isRunning()) {
    if (isCapturing()) {
      // If the connection is stalled, reconnect in the next loop
      if (isResized()) {
        logger.verbose("Detected resized stream, restarting stream");
        timeout = std::chrono::milliseconds(0);

        close();
      } else if (isStalled()) {
        logger.warning("Detected stalled stream, restarting stream");
        timeout = std::chrono::milliseconds(0);

        close();
      }
    } else if (open()) {
      // Whenever we open a stream, monitor it's health ever 100ms
      timeout = std::chrono::milliseconds(100);
      connectedAt = ofGetElapsedTimeMillis();
    } else {
      // Whenever we cannot open the device, retry in 5 seconds
      timeout = std::chrono::milliseconds(5000);
    }

    // Wait for a timeout
    signal.wait_for(lock, timeout, [&] { return !isRunning(); });
  }

  // Close the capture session before the thread exists
  if (isCapturing()) close();

  // Ensure all flags are reset when the thread exists
  running = false;
  capturing = false;
}

bool Stream::open() {
  logger.verbose("Attempting to open capture stream");

  if (!isAvailable()) {
    logger.warning("No stream available for device");
    return false;
  }

  if (prepare()) {
    logger.verbose("Succesfully started capture");

    if (device->isMaster() && !device->run("AcquisitionStart")) {
      logger.warning("Failed to start acquisition");
      teardown();
    }
  }

  return capturing;
}

bool Stream::close() {
  if (!capturing) return true;

  if (device->isMaster() && !device->run("AcquisitionStop")) {
    logger.error("Failed to stop acquisition");
  }

  return teardown();
}

bool Stream::receive(AVT::VmbAPI::FramePtr framePtr) {
  auto frame = std::make_shared<Frame>(device);

  if (frame->load(framePtr)) {
    // Keep track of our frame rate
    frameAt = ofGetElapsedTimeMillis();

    //    if (frame->getId() % 100 == 0) {
    //      logger.verbose("Received frame with id: " +
    //      ofToString(frame->getId()));
    //    }

    // Notify of new frame
    ofNotifyEvent(onFrame, frame, this);
    return true;
  } else {
    logger.error("Failed to extract frame data");
  }

  return false;
}

bool Stream::prepare() {
  if (allocate()) {
    auto error = device->getHandle()->StartCapture();

    if (error == VmbErrorSuccess) {
      if (queue()) {
        capturing = true;
      } else {
        logger.warning("Failed to queue frames");
        error = device->getHandle()->EndCapture();

        if (error != VmbErrorSuccess) {
          logger.warning("Failed to end capture");
        }
      }
    } else {
      if (error == VmbErrorInvalidAccess && !device->isMaster()) {
        logger.verbose(
            "Cannot start capturing in read only mode without a running "
            "acquisition",
            error);
      } else {
        logger.error("Failed to start capturing", error);
      }

      deallocate();
    }
  }

  return capturing;
}

bool Stream::teardown() {
  capturing = false;

  auto error = device->getHandle()->EndCapture();
  if (error == VmbErrorSuccess) {
    if (flush()) {
      if (deallocate()) {
        logger.verbose("Stopped capturing");
      } else {
        logger.warning("Failed to deallocate frames");
      }
    } else {
      logger.warning("Failed to flush queued frames");
    }
  }

  return capturing;
}

bool Stream::isAllocated(const VmbInt64_t& size) const {
  for (auto& frame : frames) {
    if (SP_ISNULL(frame)) return false;

    VmbUint32_t frameSize = 0;
    frame->GetBufferSize(frameSize);

    if (frame->GetBufferSize(frameSize) == VmbErrorSuccess) {
      if (frameSize != static_cast<VmbUint32_t>(size)) return false;
    }
  }

  return true;
}

bool Stream::allocate() {
  VmbInt64_t size = 0;
  VmbErrorType error = VmbErrorSuccess;

  if (!device->get("PayloadSize", size)) {
    logger.error("Failed to retrieve payload size");
    return false;
  }

  // If the payload size changed, reallocate
  if (!isAllocated(size)) {
    for (auto& frame : frames) {
      if (!SP_ISNULL(frame)) frame->UnregisterObserver();
      SP_SET(frame, new AVT::VmbAPI::Frame(size));

      error = frame->RegisterObserver(observer);
      if (error != VmbErrorSuccess) {
        logger.error("Failed to register frame observer", error);
        return false;
      }
    }
  }

  for (auto& frame : frames) {
    // Annoince frames
    error = device->getHandle()->AnnounceFrame(frame);
    if (error != VmbErrorSuccess) {
      logger.error("Failed to announce frame", error);
      return false;
    }
  }

  return true;
}

bool Stream::deallocate() {
  auto error = device->getHandle()->RevokeAllFrames();

  if (error != VmbErrorSuccess) {
    logger.error("Failed to revoke frames", error);
    return false;
  }

  return true;
}

bool Stream::queue() {
  VmbErrorType error = VmbErrorSuccess;

  for (auto& frame : frames) {
    error = device->getHandle()->QueueFrame(frame);

    if (error != VmbErrorSuccess) {
      logger.error("Failed to queue frame", error);
      return false;
    }
  }

  observe();

  return true;
}

bool Stream::flush() {
  unobserve();

  auto error = device->getHandle()->FlushQueue();

  if (error != VmbErrorSuccess) {
    logger.error("Failed to flush camera queue", error);
    return false;
  }

  return true;
}

void Stream::observe() {
  AVT::VmbAPI::FeaturePtr payload;

  if (device->locate("PayloadSize", payload)) {
    payload->RegisterObserver(observer);
  }

  observer->start();
}

void Stream::unobserve() {
  AVT::VmbAPI::FeaturePtr payload;

  if (device->locate("PayloadSize", payload)) {
    payload->UnregisterObserver(observer);
  }

  observer->stop();
}

void StreamObserver::start() {
  std::lock_guard<std::mutex> lock(mutex);
  running = true;
}

void StreamObserver::stop() {
  std::lock_guard<std::mutex> lock(mutex);
  running = false;
}

void StreamObserver::FrameReceived(AVT::VmbAPI::FramePtr frame) {
  std::lock_guard<std::mutex> lock(mutex);
  if (!running) return;

  VmbFrameStatusType statusType = VmbFrameStatusInvalid;
  auto error = frame->GetReceiveStatus(statusType);
  if (error == VmbErrorSuccess && statusType == VmbFrameStatusComplete) {
    stream.receive(frame);
  }

  m_pCamera->QueueFrame(frame);
}

void StreamObserver::FeatureChanged(const AVT::VmbAPI::FeaturePtr&) {
  std::lock_guard<std::mutex> lock(mutex);
  if (!running) return;

  VmbInt64_t size = 0;
  if (stream.device->get("PayloadSize", size)) {
    if (!stream.isAllocated(size)) {
      stream.logger.verbose("Stream payload size changed, scheduling resize");
      stream.resizedAt = ofGetElapsedTimeMillis();
    }
  }
}
