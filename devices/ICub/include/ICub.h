// SPDX-FileCopyrightText: Fondazione Istituto Italiano di Tecnologia (IIT)
// SPDX-License-Identifier: BSD-3-Clause

#ifndef WEARABLE_DEVICES_ICUB_H
#define WEARABLE_DEVICES_ICUB_H

#include "Wearable/IWear/IWear.h"

#include <yarp/dev/DeviceDriver.h>
#include <yarp/dev/IPreciselyTimed.h>

#include <memory>

namespace wearable {
    namespace devices {
        class ICub;
    } // namespace devices
} // namespace wearable

class wearable::devices::ICub final
        : public yarp::dev::DeviceDriver
        , public yarp::dev::IPreciselyTimed
        , public wearable::IWear
{
private:
    class ICubImpl;
    std::unique_ptr<ICubImpl> pImpl;

public:
    ICub();
    ~ICub() override;

    ICub(const ICub& other) = delete;
    ICub(ICub&& other) = delete;
    ICub& operator=(const ICub& other) = delete;
    ICub& operator=(ICub&& other) = delete;

    // =============
    // DEVICE DRIVER
    // =============

    bool open(yarp::os::Searchable& config) override;
    bool close() override;

    // ================
    // IPRECISELY TIMED
    // ================

    yarp::os::Stamp getLastInputStamp() override;

    // =====
    // IWEAR
    // =====

    // GENERIC
    // -------

    WearableName getWearableName() const override;
    WearStatus getStatus() const override;
    TimeStamp getTimeStamp() const override;

    SensorPtr<const sensor::ISensor> getSensor(const sensor::SensorName name) const override;

    VectorOfSensorPtr<const sensor::ISensor> getSensors(const sensor::SensorType) const override;

    // IMPLEMENTED SENSORS
    // -------------------

    SensorPtr<const sensor::IForceTorque6DSensor>
    getForceTorque6DSensor(const sensor::SensorName name) const override;

    SensorPtr<const sensor::IVirtualJointKinSensor>
    getVirtualJointKinSensor(const sensor::SensorName name) const override;

    SensorPtr<const sensor::IVirtualLinkKinSensor>
    getVirtualLinkKinSensor(const sensor::SensorName name) const override;

    // UNIMPLEMENTED SENSORS
    // ---------------------

    inline SensorPtr<const sensor::IFreeBodyAccelerationSensor>
    getFreeBodyAccelerationSensor(const sensor::SensorName /*name*/) const override;

    inline SensorPtr<const sensor::IMagnetometer>
    getMagnetometer(const sensor::SensorName /*name*/) const override;

    inline SensorPtr<const sensor::IOrientationSensor>
    getOrientationSensor(const sensor::SensorName /*name*/) const override;

    inline SensorPtr<const sensor::IPoseSensor>
    getPoseSensor(const sensor::SensorName /*name*/) const override;

    inline SensorPtr<const sensor::IPositionSensor>
    getPositionSensor(const sensor::SensorName /*name*/) const override;

    inline SensorPtr<const sensor::IVirtualSphericalJointKinSensor>
    getVirtualSphericalJointKinSensor(const sensor::SensorName /*name*/) const override;

    inline SensorPtr<const sensor::IAccelerometer>
    getAccelerometer(const sensor::SensorName /*name*/) const override;

    inline SensorPtr<const sensor::IEmgSensor>
    getEmgSensor(const sensor::SensorName /*name*/) const override;

    inline SensorPtr<const sensor::IForce3DSensor>
    getForce3DSensor(const sensor::SensorName /*name*/) const override;

    inline SensorPtr<const sensor::IGyroscope>
    getGyroscope(const sensor::SensorName /*name*/) const override;

    inline SensorPtr<const sensor::ISkinSensor>
    getSkinSensor(const sensor::SensorName /*name*/) const override;

    inline SensorPtr<const sensor::ITemperatureSensor>
    getTemperatureSensor(const sensor::SensorName /*name*/) const override;

    inline SensorPtr<const sensor::ITorque3DSensor>
    getTorque3DSensor(const sensor::SensorName /*name*/) const override;

    inline ElementPtr<const actuator::IActuator>
    getActuator(const actuator::ActuatorName name) const override;

    inline VectorOfElementPtr<const actuator::IActuator>
    getActuators(const actuator::ActuatorType type) const override;

    inline ElementPtr<const actuator::IHaptic>
    getHapticActuator(const actuator::ActuatorName) const override;

    inline ElementPtr<const actuator::IMotor>
    getMotorActuator(const actuator::ActuatorName) const override;

    inline ElementPtr<const actuator::IHeater>
    getHeaterActuator(const actuator::ActuatorName) const override;

};

inline wearable::SensorPtr<const wearable::sensor::IFreeBodyAccelerationSensor>
wearable::devices::ICub::getFreeBodyAccelerationSensor(const sensor::SensorName /*name*/) const
{
    return nullptr;
}

inline wearable::SensorPtr<const wearable::sensor::IMagnetometer>
wearable::devices::ICub::getMagnetometer(const sensor::SensorName /*name*/) const
{
    return nullptr;
}

inline wearable::SensorPtr<const wearable::sensor::IOrientationSensor>
wearable::devices::ICub::getOrientationSensor(const sensor::SensorName /*name*/) const
{
    return nullptr;
}

inline wearable::SensorPtr<const wearable::sensor::IPoseSensor>
wearable::devices::ICub::getPoseSensor(const sensor::SensorName /*name*/) const
{
    return nullptr;
}

inline wearable::SensorPtr<const wearable::sensor::IPositionSensor>
wearable::devices::ICub::getPositionSensor(const sensor::SensorName /*name*/) const
{
    return nullptr;
}

inline wearable::SensorPtr<const wearable::sensor::IVirtualSphericalJointKinSensor>
wearable::devices::ICub::getVirtualSphericalJointKinSensor(const sensor::SensorName /*name*/) const
{
    return nullptr;
}

inline wearable::SensorPtr<const wearable::sensor::IAccelerometer>
wearable::devices::ICub::getAccelerometer(const sensor::SensorName /*name*/) const
{
    return nullptr;
}

inline wearable::SensorPtr<const wearable::sensor::IEmgSensor>
wearable::devices::ICub::getEmgSensor(const sensor::SensorName /*name*/) const
{
    return nullptr;
}

inline wearable::SensorPtr<const wearable::sensor::IForce3DSensor>
wearable::devices::ICub::getForce3DSensor(const sensor::SensorName /*name*/) const
{
    return nullptr;
}

inline wearable::SensorPtr<const wearable::sensor::IGyroscope>
wearable::devices::ICub::getGyroscope(const sensor::SensorName /*name*/) const
{
    return nullptr;
}

inline wearable::SensorPtr<const wearable::sensor::ISkinSensor>
wearable::devices::ICub::getSkinSensor(const sensor::SensorName /*name*/) const
{
    return nullptr;
}

inline wearable::SensorPtr<const wearable::sensor::ITemperatureSensor>
wearable::devices::ICub::getTemperatureSensor(const sensor::SensorName /*name*/) const
{
    return nullptr;
}

inline wearable::SensorPtr<const wearable::sensor::ITorque3DSensor>
wearable::devices::ICub::getTorque3DSensor(const sensor::SensorName /*name*/) const
{
    return nullptr;
}

inline wearable::ElementPtr<const wearable::actuator::IActuator>
wearable::devices::ICub::getActuator(const actuator::ActuatorName name) const
{
    return nullptr;
}

inline wearable::VectorOfElementPtr<const wearable::actuator::IActuator>
wearable::devices::ICub::getActuators(const actuator::ActuatorType type) const
{
    return {};
}

inline wearable::ElementPtr<const wearable::actuator::IHaptic>
wearable::devices::ICub::getHapticActuator(const actuator::ActuatorName) const
{
    return nullptr;
}

inline wearable::ElementPtr<const wearable::actuator::IMotor>
wearable::devices::ICub::getMotorActuator(const actuator::ActuatorName) const
{
    return nullptr;
}

inline wearable::ElementPtr<const wearable::actuator::IHeater>
wearable::devices::ICub::getHeaterActuator(const actuator::ActuatorName) const
{
    return nullptr;
}

#endif // WEARABLE_DEVICES_ICUB_H
