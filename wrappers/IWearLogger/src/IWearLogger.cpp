// SPDX-FileCopyrightText: Fondazione Istituto Italiano di Tecnologia (IIT)
// SPDX-License-Identifier: BSD-3-Clause

#include "IWearLogger.h"
#include "Wearable/IWear/IWear.h"

#include <algorithm>
#include <functional>
#include <mutex>
#include <vector>
#include <yarp/dev/IPreciselyTimed.h>
#include <yarp/os/BufferedPort.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/Network.h>
#include <yarp/sig/Vector.h>

#include <robometry/BufferManager.h>

const std::string WrapperName = "IWearLogger";
const std::string logPrefix = WrapperName + " :";
constexpr double DefaultPeriod = 0.01;

namespace wearable {
    namespace wrappers {
        struct IWearLoggerSettings;

        enum class LoggerType
        {
            MATLAB = 0,
            YARP,
            MATLAB_YARP,
            NONE,
        };
    } // namespace wrappers
} // namespace wearable

struct wearable::wrappers::IWearLoggerSettings
{
    bool saveBufferManagerConfiguration{false};
    bool logAllQuantities{false};
    bool logAccelerometers{false};
    bool logEMGSensors{false};
    bool logForce3DSensors{false};
    bool logForceTorque6DSensors{false};
    bool logFreeBodyAccelerationSensors{false};
    bool logGyroscopes{false};
    bool logMagnetometers{false};
    bool logOrientationSensors{false};
    bool logPoseSensors{false};
    bool logPositionSensors{false};
    bool logTemperatureSensors{false};
    bool logTorque3DSensors{false};
    bool logVirtualLinkKinSensors{false};
    bool logVirtualJointKinSensors{false};
    bool logVirtualSphericalJointKinSensors{false};
    bool logSkinSensors{false};
};

using namespace wearable;
using namespace wearable::wrappers;

using WerableSensorName = std::string;
using MatlabChannelName = std::string;
using YarpBufferedPort = yarp::os::BufferedPort<yarp::sig::Vector>;

class IWearLogger::impl
{
public:
    LoggerType loggerType;
    void setLoggerType(std::string& str);

    bool loadSettingsFromConfig(yarp::os::Searchable& config);
    void checkAndLoadBooleanOption(yarp::os::Property& prop,
                                   const std::string& optionName,
                                   bool& option);

    bool configureMatlabBufferManager(const std::string& sensorName, const size_t& channelSize);
    bool configureYarpBufferManager(const std::string& sensorName);
    bool configureBufferManager();

    inline void prepareYarpBottle(const std::vector<double>& sensorData, yarp::sig::Vector& b)
    {
        b.clear();

        for (const auto& e : sensorData) {
            b.push_back(e);
        }
    }

    inline std::vector<std::string> split(const std::string& s, const std::string& delimiter)
    {
        std::size_t pos_start = 0, pos_end, delim_len = delimiter.length();
        std::string token;
        std::vector<std::string> res;

        while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
            token = s.substr(pos_start, pos_end - pos_start);
            pos_start = pos_end + delim_len;
            res.push_back(token);
        }

        res.push_back(s.substr(pos_start));
        return res;
    }

    inline std::string getValidName(const std::string& sensorName, const char& c)
    {
        std::string name{sensorName};

        // Replace special characters with desired char c
        std::replace(name.begin(), name.end(), '#', c);
        std::replace(name.begin(), name.end(), '@', c);
        std::replace(name.begin(), name.end(), '/', c);
        std::replace(name.begin(), name.end(), '(', c);
        std::replace(name.begin(), name.end(), ')', c);

        auto vecStr = split(name, wearable::Separator);

        std::string validName;
        for (auto& str : vecStr) {
            if (validName.empty()) {
                validName = str;
            }
            else {
                validName = validName + c + str;
            }
        }

        return validName;
    }

    inline std::string convertSensorNameToValidMatlabVarName(const std::string& sensorName)
    {
        return getValidName(sensorName, '_');
    }

    inline std::string convertSensorNameToValidYarpPortName(const std::string& sensorName)
    {
        return ('/' + getValidName(sensorName, '/'));
    }

    inline void
    prefixVecWithSensorStatus(const std::shared_ptr<const wearable::sensor::ISensor>& sensor,
                              std::vector<double>& saveVar)
    {
        // prefix sensor status
        auto it = saveVar.begin();
        saveVar.insert(it, static_cast<double>(sensor->getSensorStatus()));
    }

    bool firstRun = true;
    size_t waitingFirstReadCounter = 1;

    wearable::IWear* iWear = nullptr;
    yarp::dev::IPreciselyTimed* iPreciselyTimed = nullptr;
    std::mutex loggerMutex;
    IWearLoggerSettings settings;
    robometry::BufferConfig bufferConfig;
    robometry::BufferManager bufferManager;

    wearable::VectorOfSensorPtr<const wearable::sensor::IAccelerometer> accelerometers;
    wearable::VectorOfSensorPtr<const wearable::sensor::IEmgSensor> emgSensors;
    wearable::VectorOfSensorPtr<const wearable::sensor::IForce3DSensor> force3DSensors;
    wearable::VectorOfSensorPtr<const wearable::sensor::IForceTorque6DSensor> forceTorque6DSensors;
    wearable::VectorOfSensorPtr<const wearable::sensor::IFreeBodyAccelerationSensor>
        freeBodyAccelerationSensors;
    wearable::VectorOfSensorPtr<const wearable::sensor::IGyroscope> gyroscopes;
    wearable::VectorOfSensorPtr<const wearable::sensor::IMagnetometer> magnetometers;
    wearable::VectorOfSensorPtr<const wearable::sensor::IOrientationSensor> orientationSensors;
    wearable::VectorOfSensorPtr<const wearable::sensor::IPoseSensor> poseSensors;
    wearable::VectorOfSensorPtr<const wearable::sensor::IPositionSensor> positionSensors;
    wearable::VectorOfSensorPtr<const wearable::sensor::ITemperatureSensor> temperatureSensors;
    wearable::VectorOfSensorPtr<const wearable::sensor::ITorque3DSensor> torque3DSensors;
    wearable::VectorOfSensorPtr<const wearable::sensor::IVirtualLinkKinSensor>
        virtualLinkKinSensors;
    wearable::VectorOfSensorPtr<const wearable::sensor::IVirtualJointKinSensor>
        virtualJointKinSensors;
    wearable::VectorOfSensorPtr<const wearable::sensor::IVirtualSphericalJointKinSensor>
        virtualSphericalJointKinSensors;
    wearable::VectorOfSensorPtr<const wearable::sensor::ISkinSensor>
        skinSensors;

    std::unordered_map<WerableSensorName, MatlabChannelName> wearable2MatlabNameLookup;
    std::unordered_map<WerableSensorName, std::unique_ptr<YarpBufferedPort>>
        wearable2YarpPortLookup;
};

IWearLogger::IWearLogger()
    : yarp::os::PeriodicThread(DefaultPeriod)
    , pImpl{new impl()}
{}

IWearLogger::~IWearLogger()
{
    detachAll();
    close();
}

// ========================
// PeriodicThread interface
// ========================

void IWearLogger::run()
{
    if (!pImpl->iWear) {
        yError() << logPrefix << "The IWear pointer is null in the driver loop.";
        askToStop();
        return;
    }

    if (!pImpl->iPreciselyTimed) {
        yError() << logPrefix << "The IPreciselyTimed pointer is null in the driver loop.";
        askToStop();
        return;
    }

    while (pImpl->iWear->getStatus() == WearStatus::Calibrating
           || pImpl->iWear->getStatus() == WearStatus::WaitingForFirstRead) {
        if (pImpl->waitingFirstReadCounter++ % 1000 == 0) {
            pImpl->waitingFirstReadCounter = 1;
            yInfo() << logPrefix << "IWear interface waiting for first data. Waiting...";
        }
        return;
    }

    if (pImpl->iWear->getStatus() == WearStatus::Error
        || pImpl->iWear->getStatus() == WearStatus::Unknown) {
        yError() << logPrefix << "The status of the IWear interface is not Ok ("
                 << static_cast<int>(pImpl->iWear->getStatus()) << ")";
        askToStop();
        return;
    }

    // case status is TIMEOUT or DATA_OVERFLOW
    if (pImpl->iWear->getStatus() != WearStatus::Ok) {
        yWarning() << logPrefix << "The status of the IWear interface is not Ok ("
                   << static_cast<int>(pImpl->iWear->getStatus()) << ")";
    }

    if (pImpl->firstRun) {
        pImpl->firstRun = false;
        pImpl->accelerometers = pImpl->iWear->getAccelerometers();
        pImpl->emgSensors = pImpl->iWear->getEmgSensors();
        pImpl->force3DSensors = pImpl->iWear->getForce3DSensors();
        pImpl->forceTorque6DSensors = pImpl->iWear->getForceTorque6DSensors();
        pImpl->freeBodyAccelerationSensors = pImpl->iWear->getFreeBodyAccelerationSensors();
        pImpl->gyroscopes = pImpl->iWear->getGyroscopes();
        pImpl->magnetometers = pImpl->iWear->getMagnetometers();
        pImpl->orientationSensors = pImpl->iWear->getOrientationSensors();
        pImpl->poseSensors = pImpl->iWear->getPoseSensors();
        pImpl->positionSensors = pImpl->iWear->getPositionSensors();
        pImpl->temperatureSensors = pImpl->iWear->getTemperatureSensors();
        pImpl->torque3DSensors = pImpl->iWear->getTorque3DSensors();
        pImpl->virtualLinkKinSensors = pImpl->iWear->getVirtualLinkKinSensors();
        pImpl->virtualJointKinSensors = pImpl->iWear->getVirtualJointKinSensors();
        pImpl->virtualSphericalJointKinSensors = pImpl->iWear->getVirtualSphericalJointKinSensors();
        pImpl->skinSensors = pImpl->iWear->getSkinSensors();
    }

    yarp::os::Stamp timestamp = pImpl->iPreciselyTimed->getLastInputStamp();

    if (pImpl->settings.logAllQuantities || pImpl->settings.logAccelerometers) {
        for (const auto& sensor : pImpl->accelerometers) {
            wearable::Vector3 vector3;
            if (!sensor->getLinearAcceleration(vector3)) {
                yWarning() << logPrefix << "[Accelerometers] "
                           << "Failed to read data, "
                           << "sensor status is " << static_cast<int>(sensor->getSensorStatus());
            }
            else {

                std::vector<double> saveVar{vector3.begin(), vector3.end()};
                pImpl->prefixVecWithSensorStatus(sensor, saveVar);

                if (pImpl->loggerType == LoggerType::MATLAB
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    const auto& channelName =
                        pImpl->wearable2MatlabNameLookup.at(sensor->getSensorName());
                    pImpl->bufferManager.push_back(saveVar, timestamp.getTime(), channelName);
                }

                if (pImpl->loggerType == LoggerType::YARP
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    auto& port = pImpl->wearable2YarpPortLookup.at(sensor->getSensorName());
                    yarp::sig::Vector& data = port->prepare();
                    pImpl->prepareYarpBottle(saveVar, data);
                    port->setEnvelope(timestamp);
                    port->write();
                }
            }
        }
    }

    if (pImpl->settings.logAllQuantities || pImpl->settings.logEMGSensors) {
        for (const auto& sensor : pImpl->emgSensors) {
            double value, normalization;
            // double normalizationValue;
            if (!sensor->getEmgSignal(value) || !sensor->getNormalizationValue(normalization)) {
                yWarning() << logPrefix << "[EmgSensors] "
                           << "Failed to read data, "
                           << "sensor status is " << static_cast<int>(sensor->getSensorStatus());
            }
            else {
                std::vector<double> saveVar;
                saveVar.emplace_back(value);
                saveVar.emplace_back(normalization);
                pImpl->prefixVecWithSensorStatus(sensor, saveVar);

                if (pImpl->loggerType == LoggerType::MATLAB
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    const auto& channelName =
                        pImpl->wearable2MatlabNameLookup.at(sensor->getSensorName());
                    pImpl->bufferManager.push_back(saveVar, timestamp.getTime(), channelName);
                }

                if (pImpl->loggerType == LoggerType::YARP
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    auto& port = pImpl->wearable2YarpPortLookup.at(sensor->getSensorName());
                    yarp::sig::Vector& data = port->prepare();
                    pImpl->prepareYarpBottle(saveVar, data);
                    port->setEnvelope(timestamp);
                    port->write();
                }
            }
        }
    }

    if (pImpl->settings.logAllQuantities || pImpl->settings.logForce3DSensors) {
        for (const auto& sensor : pImpl->force3DSensors) {
            wearable::Vector3 vector3;
            if (!sensor->getForce3D(vector3)) {
                yWarning() << logPrefix << "[Force3DSensors] "
                           << "Failed to read data, "
                           << "sensor status is ";
            }
            else {
                std::vector<double> saveVar{vector3.begin(), vector3.end()};
                pImpl->prefixVecWithSensorStatus(sensor, saveVar);
                if (pImpl->loggerType == LoggerType::MATLAB
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    const auto& channelName =
                        pImpl->wearable2MatlabNameLookup.at(sensor->getSensorName());
                    pImpl->bufferManager.push_back(saveVar, timestamp.getTime(), channelName);
                }

                if (pImpl->loggerType == LoggerType::YARP
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    auto& port = pImpl->wearable2YarpPortLookup.at(sensor->getSensorName());
                    yarp::sig::Vector& data = port->prepare();
                    pImpl->prepareYarpBottle(saveVar, data);
                    port->setEnvelope(timestamp);
                    port->write();
                }
            }
        }
    }

    if (pImpl->settings.logAllQuantities || pImpl->settings.logForceTorque6DSensors) {
        for (const auto& sensor : pImpl->forceTorque6DSensors) {
            wearable::Vector6 vector6;
            if (!sensor->getForceTorque6D(vector6)) {
                yWarning() << logPrefix << "[ForceTorque6DSensors] "
                           << "Failed to read data, "
                           << "sensor status is " << static_cast<int>(sensor->getSensorStatus());
            }
            else {
                std::vector<double> saveVar{vector6.begin(), vector6.end()};
                pImpl->prefixVecWithSensorStatus(sensor, saveVar);

                if (pImpl->loggerType == LoggerType::MATLAB
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    const auto& channelName =
                        pImpl->wearable2MatlabNameLookup.at(sensor->getSensorName());
                    pImpl->bufferManager.push_back(saveVar, timestamp.getTime(), channelName);
                }

                if (pImpl->loggerType == LoggerType::YARP
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    auto& port = pImpl->wearable2YarpPortLookup.at(sensor->getSensorName());
                    yarp::sig::Vector& data = port->prepare();
                    pImpl->prepareYarpBottle(saveVar, data);
                    port->setEnvelope(timestamp);
                    port->write();
                }
            }
        }
    }

    if (pImpl->settings.logAllQuantities || pImpl->settings.logFreeBodyAccelerationSensors) {
        for (const auto& sensor : pImpl->freeBodyAccelerationSensors) {
            wearable::Vector3 vector3;
            if (!sensor->getFreeBodyAcceleration(vector3)) {
                yWarning() << logPrefix << "[FreeBodyAccelerationSensors] "
                           << "Failed to read data, "
                           << "sensor status is " << static_cast<int>(sensor->getSensorStatus());
            }
            else {
                std::vector<double> saveVar{vector3.begin(), vector3.end()};
                pImpl->prefixVecWithSensorStatus(sensor, saveVar);

                if (pImpl->loggerType == LoggerType::MATLAB
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    const auto& channelName =
                        pImpl->wearable2MatlabNameLookup.at(sensor->getSensorName());
                    pImpl->bufferManager.push_back(saveVar, timestamp.getTime(), channelName);
                }

                if (pImpl->loggerType == LoggerType::YARP
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    auto& port = pImpl->wearable2YarpPortLookup.at(sensor->getSensorName());
                    yarp::sig::Vector& data = port->prepare();
                    pImpl->prepareYarpBottle(saveVar, data);
                    port->setEnvelope(timestamp);
                    port->write();
                }
            }
        }
    }

    if (pImpl->settings.logAllQuantities || pImpl->settings.logGyroscopes) {
        for (const auto& sensor : pImpl->gyroscopes) {
            wearable::Vector3 vector3;
            if (!sensor->getAngularRate(vector3)) {
                yWarning() << logPrefix << "[Gyroscopes] "
                           << "Failed to read data, "
                           << "sensor status is " << static_cast<int>(sensor->getSensorStatus());
            }
            else {
                std::vector<double> saveVar{vector3.begin(), vector3.end()};
                pImpl->prefixVecWithSensorStatus(sensor, saveVar);

                if (pImpl->loggerType == LoggerType::MATLAB
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    const auto& channelName =
                        pImpl->wearable2MatlabNameLookup.at(sensor->getSensorName());
                    pImpl->bufferManager.push_back(saveVar, timestamp.getTime(), channelName);
                }

                if (pImpl->loggerType == LoggerType::YARP
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    auto& port = pImpl->wearable2YarpPortLookup.at(sensor->getSensorName());
                    yarp::sig::Vector& data = port->prepare();
                    pImpl->prepareYarpBottle(saveVar, data);
                    port->setEnvelope(timestamp);
                    port->write();
                }
            }
        }
    }

    if (pImpl->settings.logAllQuantities || pImpl->settings.logMagnetometers) {
        for (const auto& sensor : pImpl->magnetometers) {
            wearable::Vector3 vector3;
            if (!sensor->getMagneticField(vector3)) {
                yWarning() << logPrefix << "[Magnetometers] "
                           << "Failed to read data, "
                           << "sensor status is " << static_cast<int>(sensor->getSensorStatus());
            }
            else {
                std::vector<double> saveVar{vector3.begin(), vector3.end()};
                pImpl->prefixVecWithSensorStatus(sensor, saveVar);

                if (pImpl->loggerType == LoggerType::MATLAB
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    const auto& channelName =
                        pImpl->wearable2MatlabNameLookup.at(sensor->getSensorName());
                    pImpl->bufferManager.push_back(saveVar, timestamp.getTime(), channelName);
                }

                if (pImpl->loggerType == LoggerType::YARP
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    auto& port = pImpl->wearable2YarpPortLookup.at(sensor->getSensorName());
                    yarp::sig::Vector& data = port->prepare();
                    pImpl->prepareYarpBottle(saveVar, data);
                    port->setEnvelope(timestamp);
                    port->write();
                }
            }
        }
    }

    if (pImpl->settings.logAllQuantities || pImpl->settings.logOrientationSensors) {
        for (const auto& sensor : pImpl->orientationSensors) {
            wearable::Quaternion quaternion;
            if (!sensor->getOrientationAsQuaternion(quaternion)) {
                yWarning() << logPrefix << "[OrientationSensors] "
                           << "Failed to read data, "
                           << "sensor status is " << static_cast<int>(sensor->getSensorStatus());
            }
            else {
                std::vector<double> saveVar{quaternion.begin(), quaternion.end()};
                pImpl->prefixVecWithSensorStatus(sensor, saveVar);

                if (pImpl->loggerType == LoggerType::MATLAB
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    const auto& channelName =
                        pImpl->wearable2MatlabNameLookup.at(sensor->getSensorName());
                    pImpl->bufferManager.push_back(saveVar, timestamp.getTime(), channelName);
                }

                if (pImpl->loggerType == LoggerType::YARP
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    auto& port = pImpl->wearable2YarpPortLookup.at(sensor->getSensorName());
                    yarp::sig::Vector& data = port->prepare();
                    pImpl->prepareYarpBottle(saveVar, data);
                    port->setEnvelope(timestamp);
                    port->write();
                }
            }
        }
    }

    if (pImpl->settings.logAllQuantities || pImpl->settings.logPoseSensors) {
        for (const auto& sensor : pImpl->poseSensors) {
            wearable::Vector3 vector3;
            wearable::Quaternion quaternion;
            if (!sensor->getPose(quaternion, vector3)) {
                yWarning() << logPrefix << "[PoseSensors] "
                           << "Failed to read data, "
                           << "sensor status is " << static_cast<int>(sensor->getSensorStatus());
            }
            else {
                std::vector<double> saveVar;
                std::copy(vector3.begin(), vector3.end(), std::back_inserter(saveVar));
                std::copy(quaternion.begin(), quaternion.end(), std::back_inserter(saveVar));
                pImpl->prefixVecWithSensorStatus(sensor, saveVar);

                if (pImpl->loggerType == LoggerType::MATLAB
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    const auto& channelName =
                        pImpl->wearable2MatlabNameLookup.at(sensor->getSensorName());
                    pImpl->bufferManager.push_back(saveVar, timestamp.getTime(), channelName);
                }

                if (pImpl->loggerType == LoggerType::YARP
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    auto& port = pImpl->wearable2YarpPortLookup.at(sensor->getSensorName());
                    yarp::sig::Vector& data = port->prepare();
                    pImpl->prepareYarpBottle(saveVar, data);
                    port->setEnvelope(timestamp);
                    port->write();
                }
            }
        }
    }

    if (pImpl->settings.logAllQuantities || pImpl->settings.logPositionSensors) {
        for (const auto& sensor : pImpl->positionSensors) {
            wearable::Vector3 vector3;
            if (!sensor->getPosition(vector3)) {
                yWarning() << logPrefix << "[PositionSensors] "
                           << "Failed to read data, "
                           << "sensor status is " << static_cast<int>(sensor->getSensorStatus());
            }
            else {
                std::vector<double> saveVar{vector3.begin(), vector3.end()};
                pImpl->prefixVecWithSensorStatus(sensor, saveVar);

                if (pImpl->loggerType == LoggerType::MATLAB
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    const auto& channelName =
                        pImpl->wearable2MatlabNameLookup.at(sensor->getSensorName());
                    pImpl->bufferManager.push_back(saveVar, timestamp.getTime(), channelName);
                }

                if (pImpl->loggerType == LoggerType::YARP
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    auto& port = pImpl->wearable2YarpPortLookup.at(sensor->getSensorName());
                    yarp::sig::Vector& data = port->prepare();
                    pImpl->prepareYarpBottle(saveVar, data);
                    port->setEnvelope(timestamp);
                    port->write();
                }
            }
        }
    }

    if (pImpl->settings.logAllQuantities || pImpl->settings.logTemperatureSensors) {
        for (const auto& sensor : pImpl->temperatureSensors) {
            double value;
            if (!sensor->getTemperature(value)) {
                yWarning() << logPrefix << "[TemperatureSensors] "
                           << "Failed to read data, "
                           << "sensor status is " << static_cast<int>(sensor->getSensorStatus());
            }
            else {
                std::vector<double> saveVar;
                saveVar.emplace_back(value);
                pImpl->prefixVecWithSensorStatus(sensor, saveVar);

                if (pImpl->loggerType == LoggerType::MATLAB
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    const auto& channelName =
                        pImpl->wearable2MatlabNameLookup.at(sensor->getSensorName());
                    pImpl->bufferManager.push_back(saveVar, timestamp.getTime(), channelName);
                }

                if (pImpl->loggerType == LoggerType::YARP
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    auto& port = pImpl->wearable2YarpPortLookup.at(sensor->getSensorName());
                    yarp::sig::Vector& data = port->prepare();
                    pImpl->prepareYarpBottle(saveVar, data);
                    port->setEnvelope(timestamp);
                    port->write();
                }
            }
        }
    }

    if (pImpl->settings.logAllQuantities || pImpl->settings.logTorque3DSensors) {
        for (const auto& sensor : pImpl->torque3DSensors) {
            wearable::Vector3 vector3;
            if (!sensor->getTorque3D(vector3)) {
                yWarning() << logPrefix << "[Torque3DSensors] "
                           << "Failed to read data, "
                           << "sensor status is " << static_cast<int>(sensor->getSensorStatus());
            }
            else {
                std::vector<double> saveVar{vector3.begin(), vector3.end()};
                pImpl->prefixVecWithSensorStatus(sensor, saveVar);

                if (pImpl->loggerType == LoggerType::MATLAB
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    const auto& channelName =
                        pImpl->wearable2MatlabNameLookup.at(sensor->getSensorName());
                    pImpl->bufferManager.push_back(saveVar, timestamp.getTime(), channelName);
                }

                if (pImpl->loggerType == LoggerType::YARP
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    auto& port = pImpl->wearable2YarpPortLookup.at(sensor->getSensorName());
                    yarp::sig::Vector& data = port->prepare();
                    pImpl->prepareYarpBottle(saveVar, data);
                    port->setEnvelope(timestamp);
                    port->write();
                }
            }
        }
    }

    if (pImpl->settings.logAllQuantities || pImpl->settings.logVirtualLinkKinSensors) {
        for (const auto& sensor : pImpl->virtualLinkKinSensors) {
            wearable::Vector3 linearAcc;
            wearable::Vector3 angularAcc;
            wearable::Vector3 linearVel;
            wearable::Vector3 angularVel;
            wearable::Vector3 position;
            wearable::Quaternion orientation;
            if (!sensor->getLinkAcceleration(linearAcc, angularAcc)
                || !sensor->getLinkPose(position, orientation)
                || !sensor->getLinkVelocity(linearVel, angularVel)) {
                yWarning() << logPrefix << "[VirtualLinkKinSensors] "
                           << "Failed to read data, "
                           << "sensor status is " << static_cast<int>(sensor->getSensorStatus());
            }
            else {
                std::vector<double> saveVar;
                std::copy(position.begin(), position.end(), std::back_inserter(saveVar));
                std::copy(orientation.begin(), orientation.end(), std::back_inserter(saveVar));
                std::copy(linearVel.begin(), linearVel.end(), std::back_inserter(saveVar));
                std::copy(angularVel.begin(), angularVel.end(), std::back_inserter(saveVar));
                std::copy(linearAcc.begin(), linearAcc.end(), std::back_inserter(saveVar));
                std::copy(angularAcc.begin(), angularAcc.end(), std::back_inserter(saveVar));
                pImpl->prefixVecWithSensorStatus(sensor, saveVar);

                if (pImpl->loggerType == LoggerType::MATLAB
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    const auto& channelName =
                        pImpl->wearable2MatlabNameLookup.at(sensor->getSensorName());
                    pImpl->bufferManager.push_back(saveVar, timestamp.getTime(), channelName);
                }

                if (pImpl->loggerType == LoggerType::YARP
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    auto& port = pImpl->wearable2YarpPortLookup.at(sensor->getSensorName());
                    yarp::sig::Vector& data = port->prepare();
                    pImpl->prepareYarpBottle(saveVar, data);
                    port->setEnvelope(timestamp);
                    port->write();
                }
            }
        }
    }

    if (pImpl->settings.logAllQuantities || pImpl->settings.logVirtualJointKinSensors) {
        for (const auto& sensor : pImpl->virtualJointKinSensors) {
            double jointPos;
            double jointVel;
            double jointAcc;
            if (!sensor->getJointPosition(jointPos) || !sensor->getJointVelocity(jointVel)
                || !sensor->getJointAcceleration(jointAcc)) {
                yError() << logPrefix << "[VirtualJointKinSensors] "
                         << "Failed to read data";
                askToStop();
                return;
            }
            else {
                std::vector<double> saveVar;
                saveVar.emplace_back(jointPos);
                saveVar.emplace_back(jointVel);
                saveVar.emplace_back(jointAcc);
                pImpl->prefixVecWithSensorStatus(sensor, saveVar);

                if (pImpl->loggerType == LoggerType::MATLAB
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    const auto& channelName =
                        pImpl->wearable2MatlabNameLookup.at(sensor->getSensorName());
                    pImpl->bufferManager.push_back(saveVar, timestamp.getTime(), channelName);
                }

                if (pImpl->loggerType == LoggerType::YARP
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    auto& port = pImpl->wearable2YarpPortLookup.at(sensor->getSensorName());
                    yarp::sig::Vector& data = port->prepare();
                    pImpl->prepareYarpBottle(saveVar, data);
                    port->setEnvelope(timestamp);
                    port->write();
                }
            }
        }
    }

    if (pImpl->settings.logAllQuantities || pImpl->settings.logVirtualSphericalJointKinSensors) {
        for (const auto& sensor : pImpl->virtualSphericalJointKinSensors) {
            wearable::Vector3 jointAngles;
            wearable::Vector3 jointVel;
            wearable::Vector3 jointAcc;
            if (!sensor->getJointAnglesAsRPY(jointAngles) || !sensor->getJointVelocities(jointVel)
                || !sensor->getJointAccelerations(jointAcc)) {
                yWarning() << logPrefix << "[VirtualSphericalJointKinSensors] "
                           << "Failed to read data, "
                           << "sensor status is " << static_cast<int>(sensor->getSensorStatus());
            }
            else {
                std::vector<double> saveVar;
                std::copy(jointAngles.begin(), jointAngles.end(), std::back_inserter(saveVar));
                std::copy(jointVel.begin(), jointVel.end(), std::back_inserter(saveVar));
                std::copy(jointAcc.begin(), jointAcc.end(), std::back_inserter(saveVar));
                pImpl->prefixVecWithSensorStatus(sensor, saveVar);

                if (pImpl->loggerType == LoggerType::MATLAB
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    const auto& channelName =
                        pImpl->wearable2MatlabNameLookup.at(sensor->getSensorName());
                    pImpl->bufferManager.push_back(saveVar, timestamp.getTime(), channelName);
                }

                if (pImpl->loggerType == LoggerType::YARP
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    auto& port = pImpl->wearable2YarpPortLookup.at(sensor->getSensorName());
                    yarp::sig::Vector& data = port->prepare();
                    pImpl->prepareYarpBottle(saveVar, data);
                    port->setEnvelope(timestamp);
                    port->write();
                }
            }
        }
    }

    if (pImpl->settings.logAllQuantities || pImpl->settings.logSkinSensors)
    {
        for (const auto& sensor : pImpl->skinSensors) {
            std::vector<double> pressureVector;
            if (!sensor->getPressure(pressureVector)) {
                yWarning() << logPrefix << "[SkinSensors] "
                           << "Failed to read data, "
                           << "sensor status is " << static_cast<int>(sensor->getSensorStatus());
            }
            else
            {
                std::vector<double> saveVar{pressureVector.begin(), pressureVector.end()};
                pImpl->prefixVecWithSensorStatus(sensor, saveVar);

                if (pImpl->loggerType == LoggerType::MATLAB
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    const auto& channelName =
                        pImpl->wearable2MatlabNameLookup.at(sensor->getSensorName());
                    pImpl->bufferManager.push_back(saveVar, timestamp.getTime(), channelName);
                }

                if (pImpl->loggerType == LoggerType::YARP
                    || pImpl->loggerType == LoggerType::MATLAB_YARP) {
                    auto& port = pImpl->wearable2YarpPortLookup.at(sensor->getSensorName());
                    yarp::sig::Vector& data = port->prepare();
                    pImpl->prepareYarpBottle(saveVar, data);
                    port->setEnvelope(timestamp);
                    port->write();
                }
            }
        }
    }
}

// ======================
// DeviceDriver interface
// ======================

bool IWearLogger::open(yarp::os::Searchable& config)
{
    std::lock_guard<std::mutex> guard(pImpl->loggerMutex);
    if (!config.check("period")) {
        yInfo() << logPrefix << "Using default period: " << DefaultPeriod << "s";
    }

    const double period = config.check("period", yarp::os::Value(DefaultPeriod)).asFloat64();
    setPeriod(period);

    // Load settings in the class
    bool ok = pImpl->loadSettingsFromConfig(config);
    if (!ok) {
        yError() << logPrefix << "Problem in loading settings from config.";
        return false;
    }

    return true;
}

void IWearLogger::impl::setLoggerType(std::string& str)
{
    if (!std::strcmp(str.c_str(), "matlab")) {
        this->loggerType = LoggerType::MATLAB;
    }

    if (!std::strcmp(str.c_str(), "yarp")) {
        if (this->loggerType == LoggerType::MATLAB) {
            this->loggerType = LoggerType::MATLAB_YARP;
        }
        else {
            this->loggerType = LoggerType::YARP;
        }
    }
}

bool IWearLogger::impl::loadSettingsFromConfig(yarp::os::Searchable& config)
{
    // Check for logLevel parameter
    this->loggerType = LoggerType::NONE;
    if (!(config.check("LoggerType")
          && (config.find("LoggerType").isString() || config.find("LoggerType").isList()))) {
        yInfo() << logPrefix << "Using default LoggerType : MATLAB";
        this->loggerType = LoggerType::MATLAB;
    }
    else if (config.check("LoggerType") && config.find("LoggerType").isList()) {
        yarp::os::Bottle* loggerTypeList = config.find("LoggerType").asList();

        for (size_t i = 0; i < loggerTypeList->size(); i++) {
            std::string option = loggerTypeList->get(i).asString();

            setLoggerType(option);
        }
    }
    else if (config.check("LoggerType") && config.find("LoggerType").isString()) {
        std::string option = config.find("LoggerType").asString();
        setLoggerType(option);
    }

    // Display the current logger level
    switch (this->loggerType) {
        case LoggerType::MATLAB: {
            yInfo() << logPrefix << "LoggerType set to MATLAB";
            break;
        }
        case LoggerType::YARP: {
            yInfo() << logPrefix << "LoggerType set to YARP";
            break;
        }
        case LoggerType::MATLAB_YARP: {
            yInfo() << logPrefix << "LoggerType set to MATLAB & YARP";
            break;
        }
        default:
            break;
    }

    yarp::os::Property prop;
    prop.fromString(config.toString().c_str());

    // load logger flag settings
    checkAndLoadBooleanOption(prop, "logAllQuantities", settings.logAllQuantities);
    checkAndLoadBooleanOption(prop, "logAccelerometers", settings.logAccelerometers);
    checkAndLoadBooleanOption(prop, "logEMGSensors", settings.logEMGSensors);
    checkAndLoadBooleanOption(prop, "logForce3DSensors", settings.logForce3DSensors);
    checkAndLoadBooleanOption(prop, "logForceTorque6DSensors", settings.logForceTorque6DSensors);
    checkAndLoadBooleanOption(
        prop, "logFreeBodyAccelerationSensors", settings.logFreeBodyAccelerationSensors);
    checkAndLoadBooleanOption(prop, "logGyroscopes", settings.logGyroscopes);
    checkAndLoadBooleanOption(prop, "logMagnetometers", settings.logMagnetometers);
    checkAndLoadBooleanOption(prop, "logOrientationSensors", settings.logOrientationSensors);
    checkAndLoadBooleanOption(prop, "logPoseSensors", settings.logPoseSensors);
    checkAndLoadBooleanOption(prop, "logPositionSensors", settings.logPositionSensors);
    checkAndLoadBooleanOption(prop, "logTemperatureSensors", settings.logTemperatureSensors);
    checkAndLoadBooleanOption(prop, "logTorque3DSensors", settings.logTorque3DSensors);
    checkAndLoadBooleanOption(prop, "logVirtualLinkKinSensors", settings.logVirtualLinkKinSensors);
    checkAndLoadBooleanOption(
        prop, "logVirtualJointKinSensors", settings.logVirtualJointKinSensors);
    checkAndLoadBooleanOption(
        prop, "logVirtualSphericalJointKinSensors", settings.logVirtualSphericalJointKinSensors);
    checkAndLoadBooleanOption(prop, "logSkinSensors", settings.logSkinSensors);

    // load buffer manager configuration settings
    checkAndLoadBooleanOption(
        prop, "saveBufferManagerConfiguration", settings.saveBufferManagerConfiguration);

    std::string experimentName = "experimentName";
    if (prop.check(experimentName.c_str()) && prop.find(experimentName.c_str()).isString()) {
        bufferConfig.filename = prop.find(experimentName.c_str()).asString();
    }
    else {
        yError() << logPrefix << " missing parameter: " << experimentName;
        return false;
    }

    std::string path = "path";
    if (prop.check(path.c_str()) && prop.find(path.c_str()).isString()) {
        bufferConfig.path = prop.find(path.c_str()).asString();
    }

    std::string n_samples = "n_samples";
    if (prop.check(n_samples.c_str()) && prop.find(n_samples.c_str()).isInt32()) {
        bufferConfig.n_samples = prop.find(n_samples.c_str()).asInt32();
    }
    else {
        yError() << logPrefix << " missing parameter: " << n_samples;
        return false;
    }

    std::string save_periodically = "save_periodically";
    if (prop.check(save_periodically.c_str()) && prop.find(save_periodically.c_str()).isBool()) {
        bufferConfig.save_periodically = prop.find(save_periodically.c_str()).asBool();
    }

    if (bufferConfig.save_periodically) {
        std::string save_period = "save_period";
        if (prop.check(save_period.c_str()) && prop.find(save_period.c_str()).isFloat64()) {
            bufferConfig.save_period = prop.find(save_period.c_str()).asFloat64();
        }
        else {
            yError() << logPrefix << " missing parameter: " << save_period;
            return false;
        }

        std::string data_threshold = "data_threshold";
        if (prop.check(data_threshold.c_str()) && prop.find(data_threshold.c_str()).isInt32()) {
            bufferConfig.data_threshold = prop.find(data_threshold.c_str()).asInt32();
        }
    }

    std::string auto_save = "auto_save";
    if (prop.check(auto_save.c_str()) && prop.find(auto_save.c_str()).isBool()) {
        bufferConfig.auto_save = prop.find(auto_save.c_str()).asBool();
    }

    if (!(bufferConfig.auto_save || bufferConfig.save_periodically)) {
        yError()
            << logPrefix
            << " both auto_save and save_periodically are set to false, nothing will be saved.";
        return false;
    }

    return true;
}

void IWearLogger::impl::checkAndLoadBooleanOption(yarp::os::Property& prop,
                                                  const std::string& optionName,
                                                  bool& option)
{
    if (prop.check(optionName.c_str())) {
        option = prop.find(optionName.c_str()).asBool();
    }
}

bool IWearLogger::close()
{
    if (!pImpl->bufferConfig.auto_save) {
        pImpl->bufferManager.saveToFile();
    }
    bool ok{true};
    if (pImpl->settings.saveBufferManagerConfiguration) {
        auto buffConfToSave = pImpl->bufferManager.getBufferConfig();
        ok = bufferConfigToJson(buffConfToSave,
                                buffConfToSave.path + "bufferConfig" + buffConfToSave.filename
                                    + ".json");
    }
    return ok;
}

// ==================
// IWrapper interface
// ==================

bool IWearLogger::attach(yarp::dev::PolyDriver* poly)
{
    if (!poly) {
        yError() << logPrefix << "Passed PolyDriver is nullptr.";
        return false;
    }

    if (pImpl->iWear || !poly->view(pImpl->iWear) || !pImpl->iWear) {
        yError() << logPrefix << "Failed to view the IWear interface from the PolyDriver.";
        return false;
    }

    if (pImpl->iPreciselyTimed || !poly->view(pImpl->iPreciselyTimed) || !pImpl->iPreciselyTimed) {
        yError() << logPrefix
                 << "Failed to view the IPreciselyTimed interface from the PolyDriver.";
        return false;
    }

    if (!pImpl->configureBufferManager()) {
        yError() << logPrefix << "Failed to configure buffer manager for the logger.";
        return false;
    }

    // Start the PeriodicThread loop
    if (!start()) {
        yError() << logPrefix << "Failed to start the loop.";
        return false;
    }

    yDebug() << logPrefix << "attach() successful";
    return true;
}

bool IWearLogger::impl::configureMatlabBufferManager(const std::string& sensorName,
                                                     const size_t& channelSize)
{
    MatlabChannelName channelName = convertSensorNameToValidMatlabVarName(sensorName);
    wearable2MatlabNameLookup[sensorName] = channelName;

    bool ok = bufferManager.addChannel({channelName, {channelSize, 1}});

    if (!ok) {
        yError() << logPrefix << " matlab buffer manager configuration failed for " << sensorName;
        return false;
    }

    return true;
}

bool IWearLogger::impl::configureYarpBufferManager(const std::string& sensorName)
{
    auto portName = convertSensorNameToValidYarpPortName(sensorName);

    auto port = std::make_unique<YarpBufferedPort>();

    // Check yarp network initialization
    if (!yarp::os::Network::isNetworkInitialized()) {
        yInfo() << logPrefix << "Initializing yarp network";
        yarp::os::Network::init();
    }

    // Open yarp port
    if (!port->open(portName)) {
        yError() << logPrefix << "Failed to open yarp port " << portName;
        return false;
    }

    wearable2YarpPortLookup[sensorName] = std::move(port);

    return true;
}

bool IWearLogger::impl::configureBufferManager()
{
    bool ok{true};
    // Prepare the buffer manager for the logger
    if (ok && (settings.logAllQuantities || settings.logAccelerometers)) {
        for (const auto& s : iWear->getAccelerometers()) {
            auto sensorName = s->getSensorName();
            yInfo() << logPrefix << "Adding (4, 1) accelerometer channels for " << sensorName
                    << " prefixed with sensor status.";

            if (loggerType == LoggerType::MATLAB || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureMatlabBufferManager(sensorName, 4);
            }

            if (loggerType == LoggerType::YARP || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureYarpBufferManager(sensorName);
            }
        }
    }

    if (ok && (settings.logAllQuantities || settings.logEMGSensors)) {
        for (const auto& s : iWear->getEmgSensors()) {
            auto sensorName = s->getSensorName();
            yInfo() << logPrefix << "Adding (3, 1) EMG sensor channels value+normalization for "
                    << sensorName << " prefixed with sensor status.";

            if (loggerType == LoggerType::MATLAB || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureMatlabBufferManager(sensorName, 3);
            }

            if (loggerType == LoggerType::YARP || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureYarpBufferManager(sensorName);
            }
        }
    }

    if (ok && (settings.logAllQuantities || settings.logForce3DSensors)) {
        for (const auto& s : iWear->getForce3DSensors()) {
            auto sensorName = s->getSensorName();
            yInfo() << logPrefix << "Adding (4, 1) 3d force sensor channels for " << sensorName
                    << " prefixed with sensor status.";

            if (loggerType == LoggerType::MATLAB || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureMatlabBufferManager(sensorName, 4);
            }

            if (loggerType == LoggerType::YARP || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureYarpBufferManager(sensorName);
            }
        }
    }

    if (ok && (settings.logAllQuantities || settings.logForceTorque6DSensors)) {
        for (const auto& s : iWear->getForceTorque6DSensors()) {
            auto sensorName = s->getSensorName();
            yInfo() << logPrefix << "Adding (7, 1) 6D force torque sensor channels for "
                    << sensorName << " prefixed with sensor status.";

            if (loggerType == LoggerType::MATLAB || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureMatlabBufferManager(sensorName, 7);
            }

            if (loggerType == LoggerType::YARP || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureYarpBufferManager(sensorName);
            }
        }
    }

    if (ok && (settings.logAllQuantities || settings.logFreeBodyAccelerationSensors)) {
        for (const auto& s : iWear->getFreeBodyAccelerationSensors()) {
            auto sensorName = s->getSensorName();
            yInfo() << logPrefix << "Adding (4, 1) free body acceleration sensor channels for "
                    << sensorName << " prefixed with sensor status.";

            if (loggerType == LoggerType::MATLAB || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureMatlabBufferManager(sensorName, 4);
            }

            if (loggerType == LoggerType::YARP || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureYarpBufferManager(sensorName);
            }
        }
    }

    if (ok && (settings.logAllQuantities || settings.logGyroscopes)) {
        for (const auto& s : iWear->getGyroscopes()) {
            auto sensorName = s->getSensorName();
            yInfo() << logPrefix << "Adding (4, 1) gyroscope channels for " << sensorName
                    << " prefixed with sensor status.";

            if (loggerType == LoggerType::MATLAB || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureMatlabBufferManager(sensorName, 4);
            }

            if (loggerType == LoggerType::YARP || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureYarpBufferManager(sensorName);
            }
        }
    }

    if (ok && (settings.logAllQuantities || settings.logMagnetometers)) {
        for (const auto& s : iWear->getMagnetometers()) {
            auto sensorName = s->getSensorName();
            yInfo() << logPrefix << "Adding (4, 1) magnetometer channels for " << sensorName
                    << " prefixed with sensor status.";

            if (loggerType == LoggerType::MATLAB || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureMatlabBufferManager(sensorName, 4);
            }

            if (loggerType == LoggerType::YARP || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureYarpBufferManager(sensorName);
            }
        }
    }

    if (ok && (settings.logAllQuantities || settings.logOrientationSensors)) {
        for (const auto& s : iWear->getOrientationSensors()) {
            std::string sensorName = s->getSensorName();
            yInfo() << logPrefix << "Adding (5, 1) quaternion wxyz channels for " << sensorName
                    << " prefixed with sensor status.";

            if (loggerType == LoggerType::MATLAB || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureMatlabBufferManager(sensorName, 5);
            }

            if (loggerType == LoggerType::YARP || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureYarpBufferManager(sensorName);
            }
        }
    }

    if (ok && (settings.logAllQuantities || settings.logPoseSensors)) {
        for (const auto& s : iWear->getPoseSensors()) {
            auto sensorName = s->getSensorName();
            yInfo() << logPrefix << "Adding (8, 1) pose sensor (pos+quat) channels for "
                    << sensorName << " prefixed with sensor status.";

            if (loggerType == LoggerType::MATLAB || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureMatlabBufferManager(sensorName, 8);
            }

            if (loggerType == LoggerType::YARP || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureYarpBufferManager(sensorName);
            }
        }
    }

    if (ok && (settings.logAllQuantities || settings.logPositionSensors)) {
        for (const auto& s : iWear->getPositionSensors()) {
            auto sensorName = s->getSensorName();
            yInfo() << logPrefix << "Adding (4, 1) pose sensor channels for " << sensorName
                    << " prefixed with sensor status.";

            if (loggerType == LoggerType::MATLAB || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureMatlabBufferManager(sensorName, 4);
            }

            if (loggerType == LoggerType::YARP || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureYarpBufferManager(sensorName);
            }
        }
    }

    if (ok && (settings.logAllQuantities || settings.logTemperatureSensors)) {
        for (const auto& s : iWear->getTemperatureSensors()) {
            auto sensorName = s->getSensorName();
            yInfo() << logPrefix << "Adding (2, 1) temperature sensor channels for " << sensorName
                    << " prefixed with sensor status.";

            if (loggerType == LoggerType::MATLAB || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureMatlabBufferManager(sensorName, 2);
            }

            if (loggerType == LoggerType::YARP || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureYarpBufferManager(sensorName);
            }
        }
    }

    if (ok && (settings.logAllQuantities || settings.logTorque3DSensors)) {
        for (const auto& s : iWear->getTorque3DSensors()) {
            auto sensorName = s->getSensorName();
            yInfo() << logPrefix << "Adding (4, 1) 3D torque sensor channels for " << sensorName
                    << " prefixed with sensor status.";

            if (loggerType == LoggerType::MATLAB || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureMatlabBufferManager(sensorName, 4);
            }

            if (loggerType == LoggerType::YARP || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureYarpBufferManager(sensorName);
            }
        }
    }

    if (ok && (settings.logAllQuantities || settings.logVirtualLinkKinSensors)) {
        for (const auto& s : iWear->getVirtualLinkKinSensors()) {
            auto sensorName = s->getSensorName();
            yInfo() << logPrefix << "Adding (20, 1) pos+quat+v+omega+a+alpha channels for "
                    << sensorName << " prefixed with sensor status.";

            if (loggerType == LoggerType::MATLAB || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureMatlabBufferManager(sensorName, 20);
            }

            if (loggerType == LoggerType::YARP || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureYarpBufferManager(sensorName);
            }
        }
    }

    if (ok && (settings.logAllQuantities || settings.logVirtualJointKinSensors)) {
        for (const auto& s : iWear->getVirtualJointKinSensors()) {
            auto sensorName = s->getSensorName();
            yInfo() << logPrefix << "Adding (4, 1) virtual joint kinematics channels for "
                    << sensorName << " prefixed with sensor status.";

            if (loggerType == LoggerType::MATLAB || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureMatlabBufferManager(sensorName, 4);
            }

            if (loggerType == LoggerType::YARP || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureYarpBufferManager(sensorName);
            }
        }
    }

    if (ok && (settings.logAllQuantities || settings.logVirtualSphericalJointKinSensors)) {
        for (const auto& s : iWear->getVirtualSphericalJointKinSensors()) {
            auto sensorName = s->getSensorName();
            yInfo() << logPrefix
                    << "Adding (10, 1) rpy+vel+acc virtual spherical joint kinematics channels for "
                    << sensorName << " prefixed with sensor status.";

            if (loggerType == LoggerType::MATLAB || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureMatlabBufferManager(sensorName, 10);
            }

            if (loggerType == LoggerType::YARP || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureYarpBufferManager(sensorName);
            }
        }
    }

    if (ok && (settings.logAllQuantities || settings.logSkinSensors)) {
        for (const auto& s : iWear->getSkinSensors()) {
            auto sensorName = s->getSensorName();

            // retrieve data size
            std::vector<double> pressureVector;
            s->getPressure(pressureVector);
            size_t dataSize = pressureVector.size()+1;

            yInfo() << logPrefix
                    << "Adding ("<< dataSize<<", 1) pressure vector channels for "
                    << sensorName << " prefixed with sensor status.";

            if (loggerType == LoggerType::MATLAB || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureMatlabBufferManager(sensorName, dataSize);
            }

            if (loggerType == LoggerType::YARP || loggerType == LoggerType::MATLAB_YARP) {
                ok = ok && configureYarpBufferManager(sensorName);
            }
        }
    }

    ok = ok && bufferManager.configure(bufferConfig);
    if (ok) {
        yDebug() << logPrefix << " buffer manager configured successfully.";
    }

    return ok;
}

void IWearLogger::threadRelease() {}

bool IWearLogger::detach()
{
    std::lock_guard<std::mutex> guard(pImpl->loggerMutex);
    while (isRunning()) {
        stop();
    }

    pImpl->iWear = nullptr;
    pImpl->iPreciselyTimed = nullptr;

    return true;
}

// ==========================
// IMultipleWrapper interface
// ==========================

bool IWearLogger::attachAll(const yarp::dev::PolyDriverList& driverList)
{
    if (driverList.size() > 1) {
        yError() << logPrefix << "This wrapper accepts only one attached PolyDriver.";
        return false;
    }

    const yarp::dev::PolyDriverDescriptor* driver = driverList[0];

    if (!driver) {
        yError() << logPrefix << "Passed PolyDriverDescriptor is nullptr.";
        return false;
    }

    return attach(driver->poly);
}

bool IWearLogger::detachAll()
{
    return detach();
}
