
/**
 * Created by K. Suwatchai (Mobizt)
 *
 * Email: k_suwatchai@hotmail.com
 *
 * Github: https://github.com/mobizt/Firebase-ESP-Client
 *
 * Copyright (c) 2023 mobizt
 *
 */

/**
 * This is the basic example to simulate the sensor data updates.
 *
 * This example used the FireSense, The Programmable Data Logging and IO Control library to
 * control the IOs.
 *
 * The status of sensors showed at /{DATA_PATH}/status/Node-ESP-{DEVICE_ID}
 *
 * Where the {DATA_PATH} is the base path (rooth) for all data will be stored.
 *
 * The {DEVICE_ID} is the uniqued id of the device which can get from FireSense.getDeviceId().
 *
 *
 * To control the device, send the test command at /{DATA_PATH}/control/Node-ESP-{DEVICE_ID}/cmd.
 *
 * The supported commands and the descriptions are following,
 * the result of processing is at /{DATA_PATH}/status/Node-ESP-{DEVICE_ID}/terminal
 *
 * time                 To get the device timestamp
 *
 * time {TIMESTAMP}     To set the device timestamp {TIMESTAMP}
 *
 * config               To load the config from database. If no config existed in database,
 *                      the default config may load within the user default config callback function.
 *
 * condition            To load the conditions, the IF THEN ELSE statments's conditions and statements.
 *
 * run                  To run the conditions checking tasks, the condition checking tasks will run automatically by default.
 *                      To disable conditions checking to run at the device startup, call FireSense.enableController(false)
 *                      after FireSense.begin.
 *
 * stop                 to stop the conditions checking tasks.
 *
 * store                To store the channels's status (value) to the database under /{DATA_PATH}/status/Node-ESP-{DEVICE_ID}
 *                      This depends on the the channal's status parameter value.
 *                      The device will store the channels's current values to database when its value changed and the channal's status parameter value is 'true'.
 *
 * restore              To restore (read) the status at /{DATA_PATH}/status/Node-ESP-{DEVICE_ID} and set to the channels's values.
 *
 * clear                To clear all log data at /{DATA_PATH}/logs/Node-ESP-{DEVICE_ID}.
 *                      The device will store the channnels's value to this logs path at the preconfigured interval and the channels's log parameter value is 'true'.
 *
 * ping                 To test the device is running or stream is still connected.
 *
 * restart              To restart the device.
 *
 */

#include <Arduino.h>
#if defined(ESP32) || defined(PICO_RP2040)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include <addons/TokenHelper.h>

// The FireSense class used in this example.
#include <addons/FireSense/FireSense.h>

/* 1. Define the WiFi credentials */
#define WIFI_SSID "WIFI_AP"
#define WIFI_PASSWORD "WIFI_PASSWORD"

// For the following credentials, see examples/Authentications/SignInAsUser/EmailPassword/EmailPassword.ino

/* 2. Define the API Key */
#define API_KEY "API_KEY"

/* 3. Define the RTDB URL */
#define DATABASE_URL "URL" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app

/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "USER_EMAIL"
#define USER_PASSWORD "USER_PASSWORD"

/* 5. Define the database secret in case we need to access database rules*/
#define DATABASE_SECRET "DATABASE_SECRET"

// Define Firebase Data object
FirebaseData fbdo1;
FirebaseData fbdo2;

FirebaseAuth auth;
FirebaseConfig config;

// The config data for FireSense class
Firesense_Config fsConfig;

unsigned long randomMillis = 0;

float humidity = 0;
float temperature = 0;

void printSerial(const char *msg)
{
    if (strlen(msg) > 0)
        Serial.println(msg);
}

void randomDemoSensorData()
{
    if (millis() - randomMillis > 5000)
    {
        randomMillis = millis();
        humidity = (float)random(100, 1000) / 10.1;
        temperature = (float)random(100, 500) / 10.1;
    }
}

void loadDefaultConfig()
{
    // The config can be set manually from below or read from the stored config file.

    // To store the current config file (read from database and store to device storage),
    // FireSense.backupConfig("/filename.txt" /* path of file to save */, mem_storage_type_flash /* or mem_storage_type_sd */);

    // To restore the current config (read from device storage and add to database),
    // FireSense.restoreConfig("/filename.txt" /* path of back up file to read */, mem_storage_type_flash /* or mem_storage_type_sd */);

    FireSense_Channel channel[2];

    channel[0].id = "HUMID1";
    channel[0].name = "Humidity sensor data";
    channel[0].location = "Room1 1";
    channel[0].type = Firesense_Channel_Type::Value;
    channel[0].status = true;   // to store value to the database status
    channel[0].log = true;      // to store value to the database log
    channel[0].value_index = 0; // this the index of bound user variable which added with FireSense.addUserValue
    FireSense.addChannel(channel[0]);

    channel[1].id = "TEMP1";
    channel[1].name = "Temperature sensor data";
    channel[1].location = "Room1 1";
    channel[1].type = Firesense_Channel_Type::Value;
    channel[1].status = true;   // to store value to the database status
    channel[1].log = true;      // to store value to the database log
    channel[1].value_index = 1; // this the index of bound user variable which added with FireSense.addUserValue
    FireSense.addChannel(channel[1]);

    FireSense_Condition cond[2];

    cond[0].IF = "HUMID1 < 30 || HUMID1 > 70";
    cond[0].THEN = "func(0,1,'\\n[FUNC0] The humidity ({HUMID1} %) is out of range.\\n')";
    FireSense.addCondition(cond[0]);

    cond[1].IF = "TEMP1 < 15 || TEMP1 > 40";
    cond[1].THEN = "func(0,1,'\\n[FUNC0] The temperature ({TEMP1} °C) is out of range.\\n')";
    FireSense.addCondition(cond[1]);
}

void setup()
{

    Serial.begin(115200);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

    /* Assign the api key (required) */
    config.api_key = API_KEY;

    /* Assign the user sign in credentials */
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;

    /* Assign the RTDB URL (required) */
    config.database_url = DATABASE_URL;

    /* Assign the callback function for the long running token generation task */
    config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

    Firebase.begin(&config, &auth);

    Firebase.reconnectWiFi(true);

    // Set up the config
    fsConfig.basePath = "/Sensors";
    fsConfig.deviceId = "Node1";
    fsConfig.time_zone = 3; // change for your local time zone
    fsConfig.daylight_offset_in_sec = 0;
    fsConfig.last_seen_interval = 60 * 1000;     // store timestamp
    fsConfig.log_interval = 60 * 1000;           // store log data every 60 seconds
    fsConfig.condition_process_interval = 20;    // check conditions every 20 mSec
    fsConfig.dataRetainingPeriod = 24 * 60 * 60; // keep the log data for 1 day
    fsConfig.shared_fbdo = &fbdo1;               // for store/restore the data
    fsConfig.stream_fbdo = &fbdo2;               // for stream, if set this stream_fbdo to nullptr, the stream will connected through shared FirebaseData object.
    fsConfig.debug = true;

    // Add the user variable that will bind to the channels
    FireSense.addUserValue(&humidity);
    FireSense.addUserValue(&temperature);

    // Add the callback function that will bind to the func syntax
    FireSense.addCallbackFunction(printSerial);

    // Initiate the FireSense class
    FireSense.begin(&fsConfig, DATABASE_SECRET); // The database secret can be empty string when using the OAuthen2.0 sign-in method

    // Load the config from database or create the default config
    if (!FireSense.loadConfig())
    {
        loadDefaultConfig(); // The loadDefaultConfig is the function to configure the channels and condition information.
        FireSense.updateConfig();
    }
}

void loop()
{
    FireSense.run();
    // do not use delay or blocking operating code heare
    randomDemoSensorData();
}