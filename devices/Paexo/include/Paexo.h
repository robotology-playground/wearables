// SPDX-FileCopyrightText: Fondazione Istituto Italiano di Tecnologia (IIT)
// SPDX-License-Identifier: BSD-3-Clause

#ifndef WEARABLE_PAEXO_H
#define WEARABLE_PAEXO_H

#include "Wearable/IWear/IWear.h"

#include <yarp/dev/DeviceDriver.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/dev/IWrapper.h>
#include <yarp/dev/IMultipleWrapper.h>
#include <yarp/dev/ISerialDevice.h>
#include <yarp/dev/IPreciselyTimed.h>

#include <yarp/os/PeriodicThread.h>

#include "paexo-cmake-config.h"

namespace wearable {
    namespace devices {
        class Paexo;
    } // namespace devices
} // namespace wearable

class wearable::devices::Paexo :
        public yarp::dev::DeviceDriver,
        public yarp::os::PeriodicThread,
        public yarp::dev::IWrapper,
        public yarp::dev::IMultipleWrapper,
        public yarp::dev::IPreciselyTimed,
        public wearable::IWear
{

private:
    class PaexoImpl;
    std::unique_ptr<PaexoImpl> pImpl;

public:
    Paexo();
    ~Paexo() override;

    // DeviceDriver
    bool open(yarp::os::Searchable& config) override;
    bool close() override;

    // PeriodicThread
    void run() override;
    void threadRelease() override;

    // IWrapper interface
    bool attach(yarp::dev::PolyDriver* poly) override;
    bool detach() override;

    // IMultipleWrapper interface
    bool attachAll(const yarp::dev::PolyDriverList& driverList) override;
    bool detachAll() override;

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

    ElementPtr<const actuator::IActuator> getActuator(const actuator::ActuatorName name) const override;

    VectorOfElementPtr<const actuator::IActuator> getActuators(const actuator::ActuatorType type) const override;

    // IMPLEMENTED SENSORS
    // -------------------

    SensorPtr<const sensor::IVirtualJointKinSensor>
    getVirtualJointKinSensor(const sensor::SensorName name) const override;

    SensorPtr<const sensor::IForce3DSensor>
    getForce3DSensor(const sensor::SensorName name) const override;

    SensorPtr<const sensor::ITorque3DSensor>
    getTorque3DSensor(const sensor::SensorName name) const override;

#ifdef ENABLE_PAEXO_USE_iFEELDriver

    SensorPtr<const sensor::IForceTorque6DSensor>
    getForceTorque6DSensor(const sensor::SensorName name) const override;

#else
    inline SensorPtr<const sensor::IForceTorque6DSensor>
    getForceTorque6DSensor(const sensor::SensorName /*name*/) const override;

#endif
    // IMPLEMENTED ACTUATORS
    // ---------------------

    ElementPtr<const actuator::IMotor>
    getMotorActuator(const actuator::ActuatorName) const override;

    // UNIMPLEMENTED SENSORS
    // ---------------------

    inline SensorPtr<const sensor::IVirtualLinkKinSensor>
    getVirtualLinkKinSensor(const sensor::SensorName /*name*/) const override;

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

    inline SensorPtr<const sensor::IGyroscope>
    getGyroscope(const sensor::SensorName /*name*/) const override;

    inline SensorPtr<const sensor::ISkinSensor>
    getSkinSensor(const sensor::SensorName /*name*/) const override;

    inline SensorPtr<const sensor::ITemperatureSensor>
    getTemperatureSensor(const sensor::SensorName /*name*/) const override;

    // UNIMPLEMENTED ACTUATORS
    // -----------------------

    inline ElementPtr<const actuator::IHaptic>
    getHapticActuator(const actuator::ActuatorName) const override;

    inline ElementPtr<const actuator::IHeater>
    getHeaterActuator(const actuator::ActuatorName) const override;

};

#ifndef ENABLE_PAEXO_USE_iFEELDriver
inline wearable::SensorPtr<const wearable::sensor::IForceTorque6DSensor>
wearable::devices::Paexo::getForceTorque6DSensor(const sensor::SensorName /*name*/) const
{
    return nullptr;
}
#endif

inline wearable::SensorPtr<const wearable::sensor::IVirtualLinkKinSensor>
wearable::devices::Paexo::getVirtualLinkKinSensor(const sensor::SensorName /*name*/) const
{
    return nullptr;
}

inline wearable::SensorPtr<const wearable::sensor::IFreeBodyAccelerationSensor>
wearable::devices::Paexo::getFreeBodyAccelerationSensor(const sensor::SensorName /*name*/) const
{
    return nullptr;
}

inline wearable::SensorPtr<const wearable::sensor::IMagnetometer>
wearable::devices::Paexo::getMagnetometer(const sensor::SensorName /*name*/) const
{
    return nullptr;
}

inline wearable::SensorPtr<const wearable::sensor::IOrientationSensor>
wearable::devices::Paexo::getOrientationSensor(const sensor::SensorName /*name*/) const
{
    return nullptr;
}

inline wearable::SensorPtr<const wearable::sensor::IPoseSensor>
wearable::devices::Paexo::getPoseSensor(const sensor::SensorName /*name*/) const
{
    return nullptr;
}

inline wearable::SensorPtr<const wearable::sensor::IPositionSensor>
wearable::devices::Paexo::getPositionSensor(const sensor::SensorName /*name*/) const
{
    return nullptr;
}

inline wearable::SensorPtr<const wearable::sensor::IVirtualSphericalJointKinSensor>
wearable::devices::Paexo::getVirtualSphericalJointKinSensor(const sensor::SensorName /*name*/) const
{
    return nullptr;
}

inline wearable::SensorPtr<const wearable::sensor::IAccelerometer>
wearable::devices::Paexo::getAccelerometer(const sensor::SensorName /*name*/) const
{
    return nullptr;
}

inline wearable::SensorPtr<const wearable::sensor::IEmgSensor>
wearable::devices::Paexo::getEmgSensor(const sensor::SensorName /*name*/) const
{
    return nullptr;
}

inline wearable::SensorPtr<const wearable::sensor::IGyroscope>
wearable::devices::Paexo::getGyroscope(const sensor::SensorName /*name*/) const
{
    return nullptr;
}

inline wearable::SensorPtr<const wearable::sensor::ISkinSensor>
wearable::devices::Paexo::getSkinSensor(const sensor::SensorName /*name*/) const
{
    return nullptr;
}

inline wearable::SensorPtr<const wearable::sensor::ITemperatureSensor>
wearable::devices::Paexo::getTemperatureSensor(const sensor::SensorName /*name*/) const
{
    return nullptr;
}

inline wearable::ElementPtr<const wearable::actuator::IHaptic>
wearable::devices::Paexo::getHapticActuator(const actuator::ActuatorName) const
{
    return nullptr;
}

inline wearable::ElementPtr<const wearable::actuator::IHeater>
wearable::devices::Paexo::getHeaterActuator(const actuator::ActuatorName) const
{
    return nullptr;
}

#endif // WEARABLE_PAEXO_H
