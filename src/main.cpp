/*
 This file contains code for the Hue BLE presence detector.
*/

#include <map>
#include <config.h>
#include <ArduinoLog.h>
#include <NimBLEDevice.h>
#include <DFRobot_mmWave_Radar.h>

static const NimBLEUUID LIGHT_SERVICE_UUID = NimBLEUUID("932c32bd-0000-47a2-835a-a8d455b859dd");
static const NimBLEUUID POWER_STATE_CHAR_UUID = NimBLEUUID("932c32bd-0002-47a2-835a-a8d455b859dd");
/*
 How long in seconds to scan for. If set to 0, the scan will run forever.
 However, if non-zero the current code will just start the scan again if there are unconnected bulbs.
*/
const uint32_t SCAN_LENGTH = 0;
/*
 The maximum amount of time to wait for the mmWave sensor to resume in seconds.
 This is helpful as the sensor reports no presence for a few seconds after resuming.
*/
const int SENSOR_RESUME_BUFFER = 10000;

static HardwareSerial mySerial(1);
static DFRobot_mmWave_Radar sensor(&mySerial);

// General state vars
static bool detectedState = false;
static bool sensorPaused = false;
static int connectedBulbs = 0;
static int pausedBulbs = 0;

struct BulbData {
    NimBLEAdvertisedDevice* advDevice;
    NimBLERemoteService* lightService;
    NimBLERemoteCharacteristic* powerStateChar;
    bool connected;
    bool poweredOn;
    bool paused;
};

static std::map<std::string, BulbData*> bulbs;
static BulbData* bulbToConnect;

// Will pause the mmWave sensor
void pauseSensor() {
    sensor.stop();
    sensorPaused = true;
}

// Will resume the mmWave sensor
void resumeSensor() {
    sensor.start();
    sensorPaused = false;
}

class ClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* pClient) {
        /*
         After connection we should change the parameters if we don't need fast response times.
         These settings are 150ms interval, 0 latency, 450ms timeout.
         Timeout should be a multiple of the interval, minimum is 100ms.
         I find a multiple of 3-5 * the interval works best for quick response/reconnect.
         Min interval: 120 * 1.25ms = 150, Max interval: 120 * 1.25ms = 150, 0 latency, 100 * 10ms = 1000ms/1s timeout
        */
        pClient->updateConnParams(120, 120, 0, 100);
    };

    void onDisconnect(NimBLEClient* pClient) {
        std::string bulbAddress = pClient->getPeerAddress().toString();
        Log.warningln("Disconnected from the bulb '%s'", bulbAddress.c_str());

        // Reset our local state variables
        BulbData* disconnectedBulb = bulbs.find(bulbAddress)->second;
        disconnectedBulb->connected = false;
        connectedBulbs--;
    };

    /*
     Called when the peripheral requests a change to the connection parameters.
     Return true to accept and apply them or false to reject and keep
     the currently used parameters. Default will return true.
    */
    bool onConnParamsUpdateRequest(NimBLEClient* pClient, const ble_gap_upd_params* params) {
        if(params->itvl_min < 24) { // 1.25ms units
            return false;
        } else if (params->itvl_max > 40) { // 1.25ms units
            return false;
        } else if (params->latency > 2) { // Number of intervals allowed to skip
            return false;
        } else if (params->supervision_timeout > 100) { // 10ms units
            return false;
        }
        return true;
    };
};

// Define a class to handle the callbacks when advertisements are received
class AdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
        Log.traceln("Advertised device found: %s", advertisedDevice->toString().c_str());
        std::string deviceAddress = advertisedDevice->getAddress().toString();
        if (bulbs.count(deviceAddress)) {
            Log.infoln("Found a configured bulb!");
            // NimBLE cannot scan and connect at the same time, so stop scanning now
            NimBLEDevice::getScan()->stop();
            // Save reference to the device in the appropriate bulb data
            BulbData* bulb = bulbs.find(deviceAddress)->second;
            bulb->advDevice = advertisedDevice;
            // We can't connect from here due to API blocking calls, so instead save a reference for the main loop to do it
            bulbToConnect = bulb;
        }
    }
};

// Simple helper to get the power value for a bulb from the raw data
bool getPoweredOn(const uint8_t* powerData) {
    return powerData[0] == 1;
}

// Change the power state of the given bulb
bool changeBulbState(BulbData* bulb, bool powerOn) {
    // Check the state of the bulb first, which shouldn't count as failure
    if (bulb->connected && !bulb->paused && powerOn != bulb->poweredOn) {
        NimBLERemoteCharacteristic* powerStateChar = bulb->powerStateChar;
        std::string bulbAddress = bulb->advDevice->getAddress().toString();
        // For external control detection race conditions, store the state before actually updating
        bulb->poweredOn = powerOn;
        // These should all pass unless something is wrong
        if (powerStateChar && powerStateChar->canWrite() && powerStateChar->writeValue(powerOn ? byte(1) : byte(0))) {
            Log.noticeln("Turned the bulb '%s' %s", bulbAddress.c_str(), powerOn ? "on" : "off");
            return true;
        }
        Log.errorln("There was an issue changing the power characteristic for the bulb '%s'",
            bulbAddress.c_str());
        // Since we updated our local state first, revert it
        bulb->poweredOn = !powerOn;
        return false;
    }
    return true;
}

// Changes the power states for connected, non-paused bulbs
bool changeBulbStates(bool powerOn) {
    bool allSucceeded = true;
    for (auto& bulb : bulbs) {
        if (!changeBulbState(bulb.second, powerOn)) {
            Log.errorln("Failed to power the bulb '%s' %s", bulb.first.c_str(), powerOn ? "on": "off");
            allSucceeded = false;
        }
    }
    return allSucceeded;
}

/*
 This handles checking whether or not a bulb should pause/resume and update the appropriate state.
 Note that, this does not start or stop the sensor due to race conditions and the need for delay between commands.
*/
void evaluatePausing(BulbData* bulb, bool poweredOn) {
    // Check if pausing is enabled and whether the bulb changed power state from something else
    if (PAUSE_ON_EXTERNAL_CONTROL && bulb->poweredOn != poweredOn || bulb->paused) {
        Log.infoln("The bulb was externally controlled and 'PAUSE_ON_EXTERNAL_CONTROL' was enabled.");
        // Check if we should pause or resume
        if (!bulb->paused) {
            Log.infoln("Sensor control will be **paused** until we detect a second instance of external power control");
            bulb->paused = true;
            pausedBulbs++;
        } else {
            Log.infoln("Sensor control will **resume** as this is the second instance of external power control");
            bulb->paused = false;
            pausedBulbs--;
        }
    }
}

// Notification / Indication receiving handler callback
void notifyCB(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    if (pRemoteCharacteristic->getUUID().equals(POWER_STATE_CHAR_UUID)) {
        std::string bulbAddress = pRemoteCharacteristic->getRemoteService()->getClient()->getPeerAddress().toString();
        // We shouldn't ever get a notification for a non-configured bulb, so don't bother with safety checks
        BulbData* bulb = bulbs.find(bulbAddress)->second;
        bool poweredOn = getPoweredOn(pData);
        Log.infoln("Received power state notification from bulb '%s'. The bulb is now %s",
            bulbAddress.c_str(), poweredOn ? "on" : "off");
        // Pause/Resume the bulb if enabled and the appropriate conditions are met
        evaluatePausing(bulb, poweredOn);
        // Make sure we store the current power state
        bulb->poweredOn = poweredOn;
    } else {
        Log.infoln("Received an unknown notification from device '%s', service '%s' & characteristic '%s'. Value: '%u'",
            pRemoteCharacteristic->getRemoteService()->getClient()->getPeerAddress().toString().c_str(),
            pRemoteCharacteristic->getRemoteService()->getUUID().toString().c_str(),
            pRemoteCharacteristic->getUUID().toString().c_str(),
            *pData);
    }
}

// Create a single global instance of the callback class to be used by all clients
static ClientCallbacks clientCB;

// This ensures we are bonded to the given connected device
bool ensureBonded(NimBLEClient* pClient) {
    bool bonded = BLEDevice::isBonded(pClient->getPeerAddress());
    std::string deviceAddress = pClient->getPeerAddress().toString();
    Log.traceln("The bulb '%s' is currently %s", deviceAddress.c_str(), bonded ? "bonded" : "not bonded");
    if (!bonded) {
        Log.infoln("Bonding...");
        NimBLEDevice::startSecurity(pClient->getPeerAddress());
        if (!pClient->secureConnection()) {
            Log.errorln("Failed to bond with '%s'!", deviceAddress.c_str());
            return false;
        }
        Log.infoln("Successfully bonded with '%s'!", deviceAddress.c_str());
    }
    return true;
}

// This subscribes to power state notifications for the given connected device
bool initBulbNotifications(NimBLEClient* pClient, NimBLERemoteCharacteristic* powerStateChar) {
    if(powerStateChar->canNotify()) {
        if(!powerStateChar->subscribe(true, notifyCB)) {
            // Disconnect if subscribe failed
            pClient->disconnect();
            return false;
        }
    } else if(powerStateChar->canIndicate()) {
        // Send false as first argument to subscribe to indications instead of notifications
        if(!powerStateChar->subscribe(false, notifyCB)) {
            // Disconnect if subscribe failed
            pClient->disconnect();
            return false;
        }
    }
    return true;
}

// Handles the provisioning of clients and connects / interfaces with the bulb
bool connectToBulb(BulbData* bulb) {
    std::string bulbAddress = bulb->advDevice->getAddress().toString();
    NimBLEClient* pClient = nullptr;

    // Check if we have a client we should reuse first
    if (NimBLEDevice::getClientListSize()) {
        /*
         Special case when we already know this device, we send false as the
         second argument in connect() to prevent refreshing the service database.
         This saves considerable time and power.
        */
        pClient = NimBLEDevice::getClientByPeerAddress(bulb->advDevice->getAddress());
        if (pClient){
            if (!pClient->connect(bulb->advDevice, false)) {
                Log.errorln("Failed to reconnect to '%s'", bulbAddress.c_str());
                return false;
            }
            Log.infoln("Successfully reconnected to '%s'!", bulbAddress.c_str());
        }
        /*
         We don't already have a client that knows this device,
         we will check for a client that is disconnected that we can use.
        */
        else {
            pClient = NimBLEDevice::getDisconnectedClient();
        }
    }

    // No client to reuse? Create a new one
    if (!pClient) {
        if (NimBLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS) {
            Log.errorln("Max clients reached - no more connections available");
            return false;
        }

        pClient = NimBLEDevice::createClient();
        Log.traceln("Created a new client");

        pClient->setClientCallbacks(&clientCB, false);
        /*
         Set initial connection parameters: These settings are 15ms interval, 0 latency, 120ms timeout.
         These settings are safe for 3 clients to connect reliably, can go faster if you have less
         connections. Timeout should be a multiple of the interval, minimum is 100ms.
         Min interval: 12 * 1.25ms = 15, Max interval: 12 * 1.25ms = 15, 0 latency, 51 * 10ms = 510ms timeout
        */
        pClient->setConnectionParams(12,12,0,51);
        // Set how long we are willing to wait for the connection to complete (seconds), default is 30
        pClient->setConnectTimeout(5);


        if (!pClient->connect(bulb->advDevice)) {
            // Created a client but failed to connect, don't need to keep it as it has no data
            NimBLEDevice::deleteClient(pClient);
            Log.errorln("Failed to connect to '%s', deleted the client", bulbAddress.c_str());
            return false;
        }
    }

    if (!pClient->isConnected()) {
        if (!pClient->connect(bulb->advDevice)) {
            Log.errorln("Failed to connect to '%s'!", bulbAddress.c_str());
            return false;
        }
    }
    Log.traceln("Connected to: %s, RSSI: %d", bulbAddress.c_str(), pClient->getRssi());
    connectedBulbs++;

    // To be able to read & write characteristics we need to be bonded to the bulb
    if (!ensureBonded(pClient)) {
        Log.errorln("We aren't bonded to the bulb '%s' which is required. See the readme for help",
            bulbAddress.c_str());
        NimBLEDevice::deleteClient(pClient);
        return false;
    }

    // Now we can read/write/subscribe the characteristics of the services we are interested in
    bulb->lightService = pClient->getService(LIGHT_SERVICE_UUID);
    if (bulb->lightService) {
        bulb->powerStateChar = bulb->lightService->getCharacteristic(POWER_STATE_CHAR_UUID);
        if (bulb->powerStateChar) {
            if (bulb->powerStateChar->canRead()) {
                bulb->poweredOn = getPoweredOn(bulb->powerStateChar->readValue().data());
                Log.infoln("The bulb is currently %s", bulb->poweredOn ? "on" : "off");
            }

            if (!initBulbNotifications(pClient, bulb->powerStateChar)) {
                Log.errorln("Failed to subscribe to power notifications for the bulb '%s'",
                    bulbAddress.c_str());
                return false;
            }
        }
    } else {
        Log.errorln("Light service not found.");
        return false;
    }

    return bulb->connected = true;
}

/*
 This will scan for any of the configured bulbs and attempt to connect to one if found.
 Since the NimBLE scanner is blocking, if no configured bulbs can be found it will scan forever.
*/
void connectToBulb() {
    // Check if we need to start/resume scanning
    if (!NimBLEDevice::getScan()->isScanning() && bulbToConnect == nullptr) {
        Log.infoln("%d/%d bulbs are unconnected, resuming scan", connectedBulbs, bulbs.size());
        // Scanning is a blocking call so we might as well pause sensor
        pauseSensor();
        NimBLEDevice::getScan()->start(SCAN_LENGTH);
    }

    // Check if we have found a bulb to connect to
    if (bulbToConnect != nullptr) {
        // Attempt to connect to the bulb
        if (connectToBulb(bulbToConnect)) {
            Log.infoln("Successfully connected to the bulb '%s'! We should now be able to control the bulb based on presence!",
                bulbToConnect->advDevice->getAddress().toString().c_str());
        } else {
            Log.errorln("Failed to connect to the bulb '%s'", bulbToConnect->advDevice->getAddress().toString().c_str());
        }
        bulbToConnect = nullptr;
    }
}

// This handles checking presence from the sensor and controlling unpaused bulbs
void evaluatePresence() {
    // Check presence from sensor and control any connected, unpaused bulbs
    bool detected = sensor.readPresenceDetection();
    if (detected != detectedState) {
        Log.infoln("Presence state changed, new state: %s", detected ? "Present" : "Absent");
        // This will handle turning all of the connected bulbs on or off with appropriate handling
        if (!changeBulbStates(detectedState = detected)) {
            Log.errorln("There was an issue changing the state of at least one bulb");
        }
    }
}

// This will evaluate whether or not we can use the mmWave sensor to detect presence
bool canDetectPresence() {
    // Check whether the sensor should be stopped or started before returning the current state
    if (!sensorPaused && pausedBulbs == bulbs.size()) {
        Log.infoln("Stopping the mmWave sensor as every bulb is paused");
        pauseSensor();
    } else if (sensorPaused && pausedBulbs < bulbs.size()) {
        Log.infoln("Resuming the mmWave sensor as there are valid bulbs to control");
        resumeSensor();
        /*
         Since the sensor has just started, it will report no presence for a few seconds.
         To stop our logic from turning off bulbs in an occupied room, we wait to see if it
         detects presence for our configured limit before continuing and updating bulbs.
        */
        unsigned long startedWaiting = millis();
        bool detectedState = false;
        while(!detectedState && millis() - startedWaiting <= SENSOR_RESUME_BUFFER) {
            detectedState = sensor.readPresenceDetection();
        }
        changeBulbStates(detectedState);
    }
    // Return the current state of the sensor
    return !sensorPaused;
}

void setup() {
    Serial.begin(115200);
    // Initialise with log level and log output
    Log.begin(LOG_LEVEL, &Serial);
    Log.infoln("Starting Presence Detector");

    // Init an entry in our map for every configured bulb
    for (std::string bulb : BULB_MAC_ADDRESSES) {
        bulbs.insert({ bulb, new BulbData() });
    }

    // Configure the DFRobot sensor
    mySerial.begin(115200, SERIAL_8N1, RX, TX);
    sensor.factoryReset(); // Restore to the factory settings
    sensor.DetRangeCfg(SENSOR_DISTANCE_START, SENSOR_DISTANCE_END);
    sensor.OutputLatency(SENSOR_PRESENCE_LATENCY, SENSOR_ABSENCE_LATENCY);

    // Initialize NimBLE, no device name specified as we are not advertising
    NimBLEDevice::init("");

    // Configure security & bonding
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
    NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
    NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
    NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM | BLE_SM_PAIR_AUTHREQ_SC);

    // Optional: set the transmit power, default is 3db
#ifdef ESP_PLATFORM
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); // +9db
#else
    NimBLEDevice::setPower(9); // +9db
#endif

    // Log currently bonded devices for debugging
    Log.traceln("Current bonded devices: %d", NimBLEDevice::getNumBonds());
    for (int i = 0; i < NimBLEDevice::getNumBonds(); i++) {
        NimBLEAddress address = NimBLEDevice::getBondedAddress(i);
        Log.traceln("Device %d: %s", i + 1, address.toString().c_str());
    }

    // create new scan
    NimBLEScan* pScan = NimBLEDevice::getScan();

    // create a callback that gets called when advertisers are found
    pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());

    // Set scan interval (how often) and window (how long) in milliseconds
    pScan->setInterval(45);
    pScan->setWindow(15);
    // Active scan will gather scan response data from advertisers but will use more energy from both devices
    pScan->setActiveScan(true);
}

void loop() {
    // The NimBLE scanner is blocking, so either handle connections or do presence detection
    if (connectedBulbs < bulbs.size()) {
        // This will search for and connect to any configured bulb
        connectToBulb();
    }
    // All configured bulbs are connected
    else if (canDetectPresence()) {
        evaluatePresence();
    }
}
