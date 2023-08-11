// SPDX-FileCopyrightText: Fondazione Istituto Italiano di Tecnologia (IIT)
// SPDX-License-Identifier: BSD-3-Clause

#ifndef WEARABLE_IACTUATOR_H
#define WEARABLE_IACTUATOR_H

#include "Wearable/IWear/Common.h"

#include <atomic>

namespace wearable {

    namespace actuator {

        using ActuatorName = std::string;

        enum class ActuatorType
        {
            Haptic = 0,
            Motor,
            Heater,
            Invalid
        };

        enum class ActuatorStatus
        {
            Error = 0,
            Ok,
            Unknown,
        };

        class IActuator;
    } // namespace actuator
} // namespace wearable

class wearable::actuator::IActuator : public wearable::IWearableDevice
{
protected:
    ActuatorName m_name;
    ActuatorType m_type;
    std::atomic<ActuatorStatus> m_status;

public:
    IActuator(ActuatorName aName = {}, ActuatorStatus aStatus = ActuatorStatus::Unknown)
        : m_name{aName}
        , m_status{aStatus}
    {
        m_wearable_element_type = ElementType::WearableActuator;
    }

    virtual ~IActuator() = default;

    inline ElementType getWearableElementType() const;

    inline ActuatorName getActuatorName() const;
    inline ActuatorType getActuatorType() const;
    inline ActuatorStatus getActuatorStatus() const;
};

inline wearable::ElementType wearable::actuator::IActuator::getWearableElementType() const
{
    return m_wearable_element_type;
}

inline wearable::actuator::ActuatorName wearable::actuator::IActuator::getActuatorName() const
{
    return m_name;
}

inline wearable::actuator::ActuatorType wearable::actuator::IActuator::getActuatorType() const
{
    return m_type;
}

inline wearable::actuator::ActuatorStatus wearable::actuator::IActuator::getActuatorStatus() const
{
    return m_status;
}

#endif // WEARABLE_IACTUATOR_H
