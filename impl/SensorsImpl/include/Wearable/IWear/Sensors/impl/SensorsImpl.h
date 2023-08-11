// SPDX-FileCopyrightText: Fondazione Istituto Italiano di Tecnologia (IIT)
// SPDX-License-Identifier: BSD-3-Clause

#ifndef SENSORSIMPL_H
#define SENSORSIMPL_H

#include "Wearable/IWear/IWear.h"
#include <mutex>
#include <vector>

namespace wearable {
    namespace sensor {
        namespace impl {
            class Accelerometer;
            class EmgSensor;
            class Force3DSensor;
            class ForceTorque6DSensor;
            class FreeBodyAccelerationSensor;
            class Gyroscope;
            class Magnetometer;
            class OrientationSensor;
            class PoseSensor;
            class PositionSensor;
            class SkinSensor;
            class TemperatureSensor;
            class Torque3DSensor;
            class VirtualLinkKinSensor;
            class VirtualJointKinSensor;
            class VirtualSphericalJointKinSensor;
        } // namespace impl
    } // namespace sensor
} // namespace wearable

class wearable::sensor::impl::Accelerometer : public wearable::sensor::IAccelerometer
{
public:
    wearable::Vector3 m_buffer;
    mutable std::mutex m_mutex;

    Accelerometer(wearable::sensor::SensorName n = {},
                  wearable::sensor::SensorStatus s = wearable::sensor::SensorStatus::Unknown);
    ~Accelerometer() override = default;

    bool getLinearAcceleration(wearable::Vector3& linearAcceleration) const override;

    void setBuffer(const wearable::Vector3& data);
    inline void setStatus(const wearable::sensor::SensorStatus status) { m_status = status; }
};

class wearable::sensor::impl::EmgSensor : public wearable::sensor::IEmgSensor
{
public:
    double m_value;
    double m_normalization;
    mutable std::mutex m_mutex;

    EmgSensor(wearable::sensor::SensorName n, wearable::sensor::SensorStatus s);
    ~EmgSensor() override = default;

    bool getEmgSignal(double& emgSignal) const override;
    bool getNormalizationValue(double& normalizationValue) const override;

    void setBuffer(const double value, const double normalization);
    inline void setStatus(const wearable::sensor::SensorStatus status) { m_status = status; }
};

class wearable::sensor::impl::Force3DSensor : public wearable::sensor::IForce3DSensor
{
public:
    wearable::Vector3 m_buffer;
    mutable std::mutex m_mutex;

    Force3DSensor(wearable::sensor::SensorName n, wearable::sensor::SensorStatus s);
    ~Force3DSensor() override = default;

    bool getForce3D(wearable::Vector3& force) const override;

    void setBuffer(const wearable::Vector3& data);
    inline void setStatus(const wearable::sensor::SensorStatus status) { m_status = status; }
};

class wearable::sensor::impl::ForceTorque6DSensor : public wearable::sensor::IForceTorque6DSensor
{
public:
    wearable::Vector3 m_force;
    wearable::Vector3 m_torque;
    mutable std::mutex m_mutex;

    ForceTorque6DSensor(wearable::sensor::SensorName n, wearable::sensor::SensorStatus s);
    ~ForceTorque6DSensor() override = default;

    bool getForceTorque6D(wearable::Vector3& force3D, wearable::Vector3& torque3D) const override;

    void setBuffer(const wearable::Vector3& force, const wearable::Vector3& torque);
    inline void setStatus(const wearable::sensor::SensorStatus status) { m_status = status; }
};

class wearable::sensor::impl::FreeBodyAccelerationSensor
    : public wearable::sensor::IFreeBodyAccelerationSensor
{
public:
    wearable::Vector3 m_buffer;
    mutable std::mutex m_mutex;

    FreeBodyAccelerationSensor(
        wearable::sensor::SensorName n = {},
        wearable::sensor::SensorStatus s = wearable::sensor::SensorStatus::Unknown);
    ~FreeBodyAccelerationSensor() override = default;

    bool getFreeBodyAcceleration(wearable::Vector3& freeBodyAcceleration) const override;

    void setBuffer(const wearable::Vector3& data);
    inline void setStatus(const wearable::sensor::SensorStatus status) { m_status = status; }
};

class wearable::sensor::impl::Gyroscope : public wearable::sensor::IGyroscope
{
public:
    wearable::Vector3 m_buffer;
    mutable std::mutex m_mutex;

    Gyroscope(wearable::sensor::SensorName n = {},
              wearable::sensor::SensorStatus s = wearable::sensor::SensorStatus::Unknown);
    ~Gyroscope() override = default;

    bool getAngularRate(wearable::Vector3& angularRate) const override;

    void setBuffer(const wearable::Vector3& data);
    inline void setStatus(const wearable::sensor::SensorStatus status) { m_status = status; }
};

class wearable::sensor::impl::Magnetometer : public wearable::sensor::IMagnetometer
{
public:
    wearable::Vector3 m_buffer;
    mutable std::mutex m_mutex;

    Magnetometer(wearable::sensor::SensorName n = {},
                 wearable::sensor::SensorStatus s = wearable::sensor::SensorStatus::Unknown);
    ~Magnetometer() override = default;

    bool getMagneticField(wearable::Vector3& magneticField) const override;

    void setBuffer(const wearable::Vector3& data);
    inline void setStatus(const wearable::sensor::SensorStatus status) { m_status = status; }
};

class wearable::sensor::impl::OrientationSensor : public wearable::sensor::IOrientationSensor
{
public:
    wearable::Quaternion m_buffer;
    mutable std::mutex m_mutex;

    OrientationSensor(wearable::sensor::SensorName n = {},
                      wearable::sensor::SensorStatus s = wearable::sensor::SensorStatus::Unknown);
    ~OrientationSensor() override = default;

    bool getOrientationAsQuaternion(wearable::Quaternion& orientation) const override;

    void setBuffer(const wearable::Quaternion& data);
    inline void setStatus(const wearable::sensor::SensorStatus status) { m_status = status; }
};

class wearable::sensor::impl::PoseSensor : public wearable::sensor::IPoseSensor
{
public:
    wearable::Vector3 m_position;
    wearable::Quaternion m_orientation;
    mutable std::mutex m_mutex;

    PoseSensor(wearable::sensor::SensorName n = {},
               wearable::sensor::SensorStatus s = wearable::sensor::SensorStatus::Unknown);
    ~PoseSensor() override = default;

    bool getPose(wearable::Quaternion& orientation, wearable::Vector3& position) const override;

    void setBuffer(const wearable::Quaternion& orientation, const wearable::Vector3& position);
    inline void setStatus(const wearable::sensor::SensorStatus status) { m_status = status; }
};

class wearable::sensor::impl::PositionSensor : public wearable::sensor::IPositionSensor
{
public:
    wearable::Vector3 m_buffer;
    mutable std::mutex m_mutex;

    PositionSensor(wearable::sensor::SensorName n = {},
                   wearable::sensor::SensorStatus s = wearable::sensor::SensorStatus::Unknown);
    ~PositionSensor() override = default;

    bool getPosition(wearable::Vector3& position) const override;

    void setBuffer(const wearable::Vector3& data);
    inline void setStatus(const wearable::sensor::SensorStatus status) { m_status = status; }
};

class wearable::sensor::impl::SkinSensor : public wearable::sensor::ISkinSensor
{
public:
    mutable std::mutex m_mutex;
    std::vector<double> m_values;

    SkinSensor(wearable::sensor::SensorName n = {},
               wearable::sensor::SensorStatus s = wearable::sensor::SensorStatus::Unknown);
    ~SkinSensor() override = default;

    bool getPressure(std::vector<double>& pressure) const override;

    void setBuffer(const std::vector<double> values);
    inline void setStatus(const wearable::sensor::SensorStatus status) { m_status = status; }
};

class wearable::sensor::impl::TemperatureSensor : public wearable::sensor::ITemperatureSensor
{
public:
    mutable std::mutex m_mutex;
    double m_value;

    TemperatureSensor(wearable::sensor::SensorName n = {},
                      wearable::sensor::SensorStatus s = wearable::sensor::SensorStatus::Unknown);
    ~TemperatureSensor() override = default;

    bool getTemperature(double& temperature) const override;

    void setBuffer(const double value);
    inline void setStatus(const wearable::sensor::SensorStatus status) { m_status = status; }
};

class wearable::sensor::impl::Torque3DSensor : public wearable::sensor::ITorque3DSensor
{
public:
    wearable::Vector3 m_buffer;
    mutable std::mutex m_mutex;

    Torque3DSensor(wearable::sensor::SensorName n = {},
                   wearable::sensor::SensorStatus s = wearable::sensor::SensorStatus::Unknown);
    ~Torque3DSensor() override = default;

    bool getTorque3D(wearable::Vector3& torque) const override;

    void setBuffer(const wearable::Vector3& data);
    inline void setStatus(const wearable::sensor::SensorStatus status) { m_status = status; }
};

class wearable::sensor::impl::VirtualLinkKinSensor : public wearable::sensor::IVirtualLinkKinSensor
{
public:
    wearable::Vector3 m_linearAcc;
    wearable::Vector3 m_angularAcc;
    wearable::Vector3 m_linearVel;
    wearable::Vector3 m_angularVel;
    wearable::Vector3 m_position;
    wearable::Quaternion m_orientation;
    mutable std::mutex m_mutex;

    VirtualLinkKinSensor(
        wearable::sensor::SensorName n = {},
        wearable::sensor::SensorStatus s = wearable::sensor::SensorStatus::Unknown);
    ~VirtualLinkKinSensor() override = default;

    bool getLinkAcceleration(wearable::Vector3& linear, wearable::Vector3& angular) const override;
    bool getLinkPose(wearable::Vector3& position, wearable::Quaternion& orientation) const override;
    bool getLinkVelocity(wearable::Vector3& linear, wearable::Vector3& angular) const override;

    void setBuffer(const wearable::Vector3& linearAcc,
                   const wearable::Vector3& angularAcc,
                   const wearable::Vector3& linearVel,
                   const wearable::Vector3& angularVel,
                   const wearable::Vector3& position,
                   const wearable::Quaternion& orientation);

    inline void setStatus(const wearable::sensor::SensorStatus status) { m_status = status; }
};

class wearable::sensor::impl::VirtualJointKinSensor
    : public wearable::sensor::IVirtualJointKinSensor
{
public:
    double m_position;
    double m_velocity;
    double m_acceleration;
    mutable std::mutex m_mutex;

    VirtualJointKinSensor(
        wearable::sensor::SensorName n = {},
        wearable::sensor::SensorStatus s = wearable::sensor::SensorStatus::Unknown);
    ~VirtualJointKinSensor() override = default;

    bool getJointPosition(double& position) const override;
    bool getJointVelocity(double& velocity) const override;
    bool getJointAcceleration(double& acceleration) const override;

    void setBuffer(const double& position,
                   const double& velocity,
                   const double& acceleration);

    inline void setStatus(const wearable::sensor::SensorStatus status) { m_status = status; }
};

class wearable::sensor::impl::VirtualSphericalJointKinSensor
    : public wearable::sensor::IVirtualSphericalJointKinSensor
{
public:
    wearable::Vector3 m_angleAsRPY;
    wearable::Vector3 m_velocities;
    wearable::Vector3 m_accelerations;
    mutable std::mutex m_mutex;

    VirtualSphericalJointKinSensor(
        wearable::sensor::SensorName n = {},
        wearable::sensor::SensorStatus s = wearable::sensor::SensorStatus::Unknown);
    ~VirtualSphericalJointKinSensor() override = default;

    bool getJointAnglesAsRPY(wearable::Vector3& angleAsRPY) const override;
    bool getJointVelocities(wearable::Vector3& velocities) const override;
    bool getJointAccelerations(wearable::Vector3& accelerations) const override;

    void setBuffer(const wearable::Vector3& angleAsRPY,
                   const wearable::Vector3& velocities,
                   const wearable::Vector3& accelerations);

    inline void setStatus(const wearable::sensor::SensorStatus status) { m_status = status; }
};

#endif // SENSORSIMPL_H
