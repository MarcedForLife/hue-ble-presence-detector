/*
 This file contains code for the Hue BLE presence detector.
*/

#include <config.h>
#include <ArduinoLog.h>
#include <NimBLEDevice.h>
#include <DFRobot_mmWave_Radar.h>

static const NimBLEUUID LIGHT_SERVICE_UUID = NimBLEUUID("932c32bd-0000-47a2-835a-a8d455b859dd");
static const NimBLEUUID POWER_STATE_CHAR_UUID = NimBLEUUID("932c32bd-0002-47a2-835a-a8d455b859dd");
static const uint32_t SCAN_LENGTH = 0; // 0 = scan forever

static NimBLEAdvertisedDevice* advDevice;
static NimBLERemoteService* lightService;
static NimBLERemoteCharacteristic* powerStateChar;

static HardwareSerial mySerial(1);
static DFRobot_mmWave_Radar sensor(&mySerial);

static bool doConnect = false;
static bool connected = false;
static bool bulbOn = false;
static bool detectedState = false;

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
        // Reset our local state variables
        connected = false;
        doConnect = false;

        Log.warningln("%s Disconnected, starting scan", pClient->getPeerAddress().toString().c_str());
        NimBLEDevice::getScan()->start(SCAN_LENGTH);
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
        if (advertisedDevice->getAddress().toString() == BULB_MAC_ADDRESS) {
            Log.infoln("Found our configured bulb!");
            // Stop scan before connecting
            NimBLEDevice::getScan()->stop();
            // Save the device reference in a global for the client to use
            advDevice = advertisedDevice;
            // Ready to connect now
            doConnect = true;
        }
    }
};

// This will update our local variable for the bulb power state
bool storeBulbState(const uint8_t* bulbData) {
    return bulbOn = bulbData[0] == 1;
}

// Changes the connected bulb power state (if connected)
bool changeBulbState(bool on) {
    if (powerStateChar && powerStateChar->canWrite()) {
        if (on != bulbOn && powerStateChar->writeValue(on ? byte(1) : byte(0))) {
            Log.noticeln("Turned the bulb %s", on ? "on" : "off");
        }
    }
    return false;
}

// Notification / Indication receiving handler callback
void notifyCB(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    if (pRemoteCharacteristic->getUUID().equals(POWER_STATE_CHAR_UUID)) {
        Log.infoln("Received power state notification from bulb '%s'. The bulb is now %s",
            pRemoteCharacteristic->getRemoteService()->getClient()->getPeerAddress().toString().c_str(),
            storeBulbState(pData) ? "on" : "off");
    } else {
        Log.infoln("Received an unknown notification from bulb '%s', service '%s' & characteristic '%s'. Value: '%u'",
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
    Log.traceln("Bond status: %s", bonded ? "bonded" : "not bonded");
    if (!bonded) {
        Log.infoln("Bonding...");
        if (!pClient->secureConnection()) {
            Log.errorln("Failed to bond!");
            return false;
        }
        Log.infoln("Successfully bonded!");
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
bool connectToBulb() {
    NimBLEClient* pClient = nullptr;

    // Check if we have a client we should reuse first
    if (NimBLEDevice::getClientListSize()) {
        /*
         Special case when we already know this device, we send false as the
         second argument in connect() to prevent refreshing the service database.
         This saves considerable time and power.
        */
        pClient = NimBLEDevice::getClientByPeerAddress(advDevice->getAddress());
        if (pClient){
            if (!pClient->connect(advDevice, false)) {
                Log.errorln("Reconnect failed");
                return false;
            }
            Log.infoln("Reconnected client!");
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


        if (!pClient->connect(advDevice)) {
            // Created a client but failed to connect, don't need to keep it as it has no data
            NimBLEDevice::deleteClient(pClient);
            Log.errorln("Failed to connect, deleted the client");
            return false;
        }
    }

    if (!pClient->isConnected()) {
        if (!pClient->connect(advDevice)) {
            Log.errorln("Failed to connect!");
            return false;
        }
    }

    Log.infoln("Connected to: %s", pClient->getPeerAddress().toString().c_str());
    Log.traceln("RSSI: %d", pClient->getRssi());

    // To be able to read & write characteristics we need to be bonded to the bulb
    if (!ensureBonded(pClient)) {
        Log.errorln("We aren't bonded to the bulb '%s' which is required. See the readme for help",
            pClient->getPeerAddress().toString().c_str());
        return false;
    }

    // Now we can read/write/subscribe the characteristics of the services we are interested in
    lightService = pClient->getService(LIGHT_SERVICE_UUID);
    if (lightService) {
        powerStateChar = lightService->getCharacteristic(POWER_STATE_CHAR_UUID);
        if (powerStateChar) {
            if (powerStateChar->canRead()) {
                storeBulbState(powerStateChar->readValue().data());
                Log.infoln("The bulb is currently %s", bulbOn ? "on" : "off");
            }

            if (!initBulbNotifications(pClient, powerStateChar)) {
                Log.errorln("Failed to subscribe to power notifications for the bulb '%s'",
                    pClient->getPeerAddress().toString().c_str());
                return false;
            }
        }
    } else {
        Log.errorln("Light service not found.");
        return false;
    }

    return connected = true;
}

void setup() {
    Serial.begin(115200);
    // Initialise with log level and log output.
    Log.begin(LOG_LEVEL, &Serial);
    Log.infoln("Starting Presence Detector");

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

    // create new scan
    NimBLEScan* pScan = NimBLEDevice::getScan();

    // create a callback that gets called when advertisers are found
    pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());

    // Set scan interval (how often) and window (how long) in milliseconds
    pScan->setInterval(45);
    pScan->setWindow(15);
    // Active scan will gather scan response data from advertisers but will use more energy from both devices
    pScan->setActiveScan(true);
    // Start scanning for advertisers for the scan time specified (in seconds) 0 = forever
    pScan->start(SCAN_LENGTH);
}

void loop() {
    // Loop here until we find a bulb we want to connect to
    while (!doConnect) {
        delay(1);
    }

    // While we are connected to a bulb, toggle it on and off based on presence
    while (connected) {
        bool detected = sensor.readPresenceDetection();
        if (detected != detectedState) {
            Log.infoln("Presence state changed, new state: %s", detected ? "Present" : "Absent");
            changeBulbState(detectedState = detected);
        }
    }

    // Found the bulb we want to connect to, do it now
    if (connectToBulb()) {
        Log.infoln("Success! We should now be able to control the bulb based on presence!");
    } else {
        Log.errorln("Failed to connect to the configured bulb, will resume scanning");
        NimBLEDevice::getScan()->start(SCAN_LENGTH);
    }
}
