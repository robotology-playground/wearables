// SPDX-FileCopyrightText: Fondazione Istituto Italiano di Tecnologia (IIT)
// SPDX-License-Identifier: BSD-3-Clause

#ifndef WEARABLE_ITORQUE_3D_SENSOR_H
#define WEARABLE_ITORQUE_3D_SENSOR_H

#include "Wearable/IWear/Sensors/ISensor.h"

namespace wearable {
    namespace sensor {
        class ITorque3DSensor;
    } // namespace sensor
} // namespace wearable

class wearable::sensor::ITorque3DSensor : public wearable::sensor::ISensor
{
public:
    ITorque3DSensor(SensorName aName = {}, SensorStatus aStatus = SensorStatus::Unknown)
        : ISensor(aName, aStatus)
    {
        m_type = SensorType::Torque3DSensor;
    }

    virtual ~ITorque3DSensor() = default;

    virtual bool getTorque3D(Vector3& torque) const = 0;

    inline static const std::string getPrefix();
};

inline const std::string wearable::sensor::ITorque3DSensor::getPrefix()
{
    return "t3D" + wearable::Separator;
}

#endif // WEARABLE_ITORQUE_3D_SENSOR_H
