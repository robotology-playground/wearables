// SPDX-FileCopyrightText: Fondazione Istituto Italiano di Tecnologia (IIT)
// SPDX-License-Identifier: BSD-3-Clause

#ifndef WEARABLE_IACCELEROMETER_H
#define WEARABLE_IACCELEROMETER_H

#include "Wearable/IWear/Sensors/ISensor.h"

namespace wearable {
    namespace sensor {
        class IAccelerometer;
    } // namespace sensor
} // namespace wearable

class wearable::sensor::IAccelerometer : public wearable::sensor::ISensor
{
public:
    IAccelerometer(SensorName aName = {}, SensorStatus aStatus = SensorStatus::Unknown)
        : ISensor(aName, aStatus)
    {
        m_type = SensorType::Accelerometer;
    }

    virtual ~IAccelerometer() = default;

    virtual bool getLinearAcceleration(Vector3& linearAcceleration) const = 0;

    inline static const std::string getPrefix();
};

inline const std::string wearable::sensor::IAccelerometer::getPrefix()
{
    return "acc" + wearable::Separator;
}
#endif // WEARABLE_IACCELEROMETER_H
