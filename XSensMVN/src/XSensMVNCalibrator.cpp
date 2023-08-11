// SPDX-FileCopyrightText: Fondazione Istituto Italiano di Tecnologia (IIT)
// SPDX-License-Identifier: BSD-3-Clause

#include "XsensMVNCalibrator.h"

#include <xsens/xmecalibrationresult.h>
#include <xsens/xmecontrol.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <map>
#include <thread>

namespace xsensmvn {

    /* ---------- *
     *  Utilities *
     * ---------- */

    // Map from XSens XMECalibrationQuality to xsens::CalibrationQuality
    const std::map<XmeCalibrationQuality, CalibrationQuality> CalibrationQualitiesMap{
        {XmeCalibrationQuality::XCalQ_Unknown, xsensmvn::CalibrationQuality::UNKNOWN},
        {XmeCalibrationQuality::XCalQ_Failed, xsensmvn::CalibrationQuality::FAILED},
        {XmeCalibrationQuality::XCalQ_Poor, xsensmvn::CalibrationQuality::POOR},
        {XmeCalibrationQuality::XCalQ_Acceptable, xsensmvn::CalibrationQuality::ACCEPTABLE},
        {XmeCalibrationQuality::XCalQ_Good, xsensmvn::CalibrationQuality::GOOD}};

    // Map from xsens::CalibrationQuality to std::string QualityLabel
    const std::map<CalibrationQuality, std::string> CalibrationQualityLabels{
        {xsensmvn::CalibrationQuality::UNKNOWN, "Unknown"},
        {xsensmvn::CalibrationQuality::GOOD, "Good"},
        {xsensmvn::CalibrationQuality::ACCEPTABLE, "Acceptable"},
        {xsensmvn::CalibrationQuality::POOR, "Poor"},
        {xsensmvn::CalibrationQuality::FAILED, "Failed"}};

    /* -------------------------- *
     *  Construtors / Destructors *
     * -------------------------- */

    XSensMVNCalibrator::XSensMVNCalibrator(XmeControl& connector,
                                           const xsensmvn::CalibrationQuality minAcceptableQuality)
        : m_suitsConnector(connector)
        , m_minimumAccaptableQuality(minAcceptableQuality)
        , m_achievedCalibrationQuality(CalibrationQuality::UNKNOWN)
        , m_usedCalibrationType("")
        , m_calibrationAborted(false)
        , m_calibrationInProgress(false)
        , m_calibrationProcessed(false)
        , m_operationCompleted(true)
    {
        m_suitsConnector.addCallbackHandler(static_cast<XmeCallback*>(this));
    }

    XSensMVNCalibrator::~XSensMVNCalibrator()
    {
        cleanup();
        m_suitsConnector.removeCallbackHandler(static_cast<XmeCallback*>(this));
    }

    /* ----------------- *
     *  Public Functions *
     * ----------------- */

    void XSensMVNCalibrator::getLastCalibrationInfo(std::string& type, CalibrationQuality quality)
    {
        type = m_usedCalibrationType;
        quality = m_achievedCalibrationQuality;
    }

    // Set user-specific body dimensions and apply them to the MVN engine
    bool XSensMVNCalibrator::setBodyDimensions(const std::map<std::string, double>& bodyDimensions)
    {
        // Check if the suit is connected
        if (!m_suitsConnector.status().isConnected()) {
            xsError << "Device not connected. Unable to set body dimensions." << std::endl;
            return false;
        }

        // If a calibration is in progress it is not possible to set the body dimensions
        if (!m_operationCompleted) {
            xsError << "Calibration in progress. Unable to set body dimensions." << std::endl;
            return false;
        }

        XsStringArray bodyDimList = m_suitsConnector.bodyDimensionLabelList();

        if (bodyDimensions.empty()) {
            xsError << "Empty body dimension list";
            return false;
        }
        // rise the flag to signal an operation is ongoing
        m_operationCompleted = false;

        // We do not cache the dimensions as we obtain always the most updated values from Xsens
        for (const auto& pair : bodyDimensions) {
            if (bodyDimList.find(pair.first) == -1) {
                xsWarning << "Body dimension: " << pair.first << " NOT found. Skipping.";
                continue;
            }
            m_suitsConnector.setBodyDimension(pair.first, pair.second);
        }

        // wait for completion
        while (!m_operationCompleted) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        xsInfo << "Body dimensions successfully updated." << std::endl;
        return m_operationCompleted;
    }

    // Get user-specific body dimensions from the MVN engine
    bool XSensMVNCalibrator::getBodyDimensions(std::map<std::string, double>& dimensions) const
    {
        // Check if the suit is connected
        if (!m_suitsConnector.status().isConnected()) {
            xsError << "Device not connected. Unable to retrieve body dimensions." << std::endl;
            return false;
        }

        // Get a list of bodies for which dimensions are defined
        XsStringArray bodyDimList = m_suitsConnector.bodyDimensionLabelList();
        for (const auto& body : bodyDimList) {
            // get the estimated dimension of the body
            double value = m_suitsConnector.bodyDimensionValueEstimate(body);

            // Check if the body dimension is known (-1 == unknown)
            if (static_cast<int>(value) != -1) {
                // Push the pair <bodyName, dimension> to the output vector
                dimensions.insert(std::map<std::string, double>::value_type(body.c_str(), value));
            }
        }

        xsInfo << "Body dimensions successfully retrieved from device." << std::endl;
        return true;
    }

    bool XSensMVNCalibrator::getBodyDimension(const std::string bodyName, double& dim)
    {
        std::map<std::string, double> dimensions;
        getBodyDimensions(dimensions);
        const auto it = dimensions.find(bodyName);
        if (it != dimensions.end()) {
            xsWarning << "Impossible to find " << bodyName;
            return false;
        }
        dim = it->second;
        return true;
    }

    // Calibrate MVN engine following the specified calibrationType routine
    bool XSensMVNCalibrator::calibrateWithType(std::string calibrationType)
    {
        m_calibrationInProgress = true;

        // Check if a previous calibration of the same type is already in use, if so, discard it.
        if (m_suitsConnector.isCalibrationPerformed(calibrationType)) {
            xsInfo << "Discarding previous " << calibrationType << " calibration";
            m_usedCalibrationType = "";
            m_achievedCalibrationQuality = CalibrationQuality::UNKNOWN;
            m_operationCompleted = false;
            m_suitsConnector.clearCalibration(calibrationType);
        }

        // Wait for the discard operation to be completed
        while (m_suitsConnector.isCalibrationPerformed(calibrationType) && !m_calibrationAborted) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        if (m_calibrationAborted) {
            cleanup();
            return false;
        }

        // Initialize MVN engine to start the calibration routine
        m_suitsConnector.initializeCalibration(calibrationType);

        // Get the phases of the selected calibration type
        XsIntArray calibPhases = m_suitsConnector.calibrationPhaseList();

        // Start the calibration data collection (Wait three seconds to give the subject enough time to take position)
        xsInfo << "Starting " << calibrationType << " calibration" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        m_suitsConnector.startCalibration();

        // Follow step-by-step the calibration phases of the selected type
        for (unsigned int phase = 0; phase < calibPhases.size() - 1; ++phase) {
            int startingFrame = calibPhases[phase];
            int endingFrame = calibPhases[phase + 1];
            xsInfo << m_suitsConnector.calibrationPhaseText(static_cast<int>(phase)) << std::endl;

            while ((startingFrame < endingFrame) && !m_calibrationAborted) {
                XmePose calibPose = m_suitsConnector.calibrationPose(startingFrame++);
                // Sleep for 16ms as suggested by XSens MVN2018 Calibration guidelines "recommended
                // sleep time is 16 milliseconds because the recording playback is shown at 60 Hz"
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
                std::cout << ".";
            }
            if (m_calibrationAborted) {
                cleanup();
                return false;
            }

            std::cout << std::endl;
        }

        // Stop the calibration data collection
        m_suitsConnector.stopCalibration();
        xsInfo << "Data collection for calibration completed." << std::endl;

        // Just to play safe sleep 100 ms to ensure the stopCalibration command has been received
        m_calibrationProcessed = false;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        xsInfo << "Processing calibration data just collected." << std::endl;

        // Wait for the onCalibrationProcessed Callback
        while (!m_calibrationProcessed && !m_calibrationAborted) {
            std::cout << ".";
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // Check if the positive wake has been due to an abortCalibration event
        if (m_calibrationAborted) {
            cleanup();
            return false;
        }

        xsInfo << "Calibration data processing completed";
        xsInfo << "Retrieving calibration results" << std::endl;

        XmeCalibrationResult calibrationResult =
            m_suitsConnector.calibrationResult(calibrationType);

        XmeCalibrationQuality quality = calibrationResult.m_quality;
        XsStringArray warnings = calibrationResult.m_warnings;

        // Notify the user about the calibration quality and the received hints / warnings
        xsInfo << "Calibration Quality: "
               << xsensmvn::CalibrationQualityLabels.at(CalibrationQualitiesMap.at(quality));

        if (!warnings.empty()) {
            xsInfo << "Calibration result warnings:";
            for (const auto& wrn : warnings) {
                xsInfo << wrn;
            }
        }

        if (CalibrationQualitiesMap.at(calibrationResult.m_quality) < m_minimumAccaptableQuality) {
            // Notify the user that the calibration quality is lower than the minimum required one
            xsInfo << "Minimum required quality: "
                   << xsensmvn::CalibrationQualityLabels.at(m_minimumAccaptableQuality).c_str();
            xsInfo << " Achieved quality: "
                   << xsensmvn::CalibrationQualityLabels.at(
                          CalibrationQualitiesMap.at(calibrationResult.m_quality));
            xsInfo << "Condition not met. Discarding. Please try again.";

            // Calibration cannot be considered acceptable. The fastest and safest way to discard it
            // is to manually trigger an abortCalibration event
            abortCalibration();
            while (!m_calibrationAborted) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            cleanup();
            return false;
        }
        else {
            // Notify the user the calibration can be applied
            xsInfo << "Ready to apply the obtained calibration";

            // Apply the calibration to the MVN Engine and wait for a positive feedback
            m_suitsConnector.finalizeCalibration();
            while (!m_operationCompleted && !m_calibrationAborted) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            // Check if the positive wake has been due to an abortCalibration event
            if (m_calibrationAborted) {
                cleanup();
                return false;
            }
            xsInfo << "Done! Calibration Completed.";
        }

        m_usedCalibrationType = calibrationType;
        m_achievedCalibrationQuality = CalibrationQualitiesMap.at(quality);

        // Calibration completed
        m_calibrationInProgress = false;
        return true;
    }

    // Abort the current ongoing calibration
    bool XSensMVNCalibrator::abortCalibration()
    {
        if (m_calibrationInProgress) {
            m_suitsConnector.abortCalibration();
            return true;
        }
        else {
            return false;
        }
    }

    // Check if there is a calibration currently in progress
    bool XSensMVNCalibrator::isCalibrationInProgress() { return m_calibrationInProgress; }

    // Set the minimum calibration quality considered to be satisfactory
    bool XSensMVNCalibrator::setMinimumAcceptableCalibrationQuality(
        const xsensmvn::CalibrationQuality quality)
    {
        m_minimumAccaptableQuality = quality;
        return true;
    }

    // Get the minimum calibration quality considered to be satisfactory
    xsensmvn::CalibrationQuality& XSensMVNCalibrator::getMinimumAcceptableCalibrationQuality()
    {
        return m_minimumAccaptableQuality;
    }

    /* ------------------------------------------ *
     *  Public XSens XME Callback Implementations *
     * ------------------------------------------ */

    // Called by XSens MVN Engine after successfully abort the calibration
    void XSensMVNCalibrator::onCalibrationAborted(XmeControl* dev) { m_calibrationAborted = true; }

    // Called by XSens MVN Engine after the completion of any request made to the device
    void XSensMVNCalibrator::onCalibrationComplete(XmeControl* dev) { m_operationCompleted = true; }

    // Called by XSens MVN Engine after the completion of the calibration data processing
    void XSensMVNCalibrator::onCalibrationProcessed(XmeControl* dev)
    {
        m_calibrationProcessed = true;
    }

    /* ------------------ *
     *  Private Functions *
     * ------------------ */

    // Reset internal state machine atomic variables
    void XSensMVNCalibrator::cleanup()
    {
        m_usedCalibrationType = "";
        m_achievedCalibrationQuality = CalibrationQuality::UNKNOWN;
        m_calibrationProcessed = m_operationCompleted = m_calibrationInProgress = false;
        m_calibrationAborted = false;
    }
} // namespace xsensmvn
