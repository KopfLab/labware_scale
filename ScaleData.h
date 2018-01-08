#include "device/DeviceDataSerial.h"

struct ScaleData : public DeviceDataSerial {
    ScaleData() : DeviceDataSerial() {
      setVariable("weight");
    };
};
