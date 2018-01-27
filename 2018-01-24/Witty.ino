#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <AzureIoTHubMQTTClient.h>

const char *AP_SSID = "[YOUR_SSID_NAME]";
const char *AP_PASS = "[YOUR_SSID_PASS]";

// Azure IoT Hub Settings --> CHANGE THESE
#define IOTHUB_HOSTNAME         "[YOUR_IOTHUB_NAME].azure-devices.net"
#define DEVICE_ID               "[YOUR_DEVICE_ID]"
#define DEVICE_KEY              "[YOUR_DEVICE_KEY]" //Primary key of the device

WiFiClientSecure tlsClient;
AzureIoTHubMQTTClient client(tlsClient, IOTHUB_HOSTNAME, DEVICE_ID, DEVICE_KEY);
WiFiEventHandler e1, e2;

const int LDR = A0;
const int BUTTON = 4;
const int RED = 15;
const int GREEN = 12;
const int BLUE = 13;
unsigned long lastMillis = 0;

void connectToIoTHub(); // <- predefine connectToIoTHub() for setup()
void onMessageCallback(const MQTT::Publish &msg);

void onSTAGotIP(WiFiEventStationModeGotIP ipInfo)
{
    Serial.printf("Got IP: %s\r\n", ipInfo.ip.toString().c_str());

    analogWrite(RED, 0);

    //do connect upon WiFi connected
    connectToIoTHub();
}

void onSTADisconnected(WiFiEventStationModeDisconnected event_info)
{
    Serial.printf("Disconnected from SSID: %s\n", event_info.ssid.c_str());
    Serial.printf("Reason: %d\n", event_info.reason);
}

void onClientEvent(const AzureIoTHubMQTTClient::AzureIoTHubMQTTClientEvent event)
{
    if (event == AzureIoTHubMQTTClient::AzureIoTHubMQTTClientEventConnected)
    {

        Serial.println("Connected to Azure IoT Hub");

        //Add the callback to process cloud-to-device message/command
        client.onMessage(onMessageCallback);
    }
}

void onActivateLedCommand(String cmdName, JsonVariant jsonValue)
{
    //Parse cloud-to-device message JSON. In this example, I send the command message with following format:
    //{"Name":"ActivateLed","Parameters":{"Activated":0}}

    JsonObject &jsonObject = jsonValue.as<JsonObject>();
    if (jsonObject.containsKey("Parameters"))
    {
        auto params = jsonValue["Parameters"];
        auto isAct = (params["Activated"]);
        if (isAct)
        {
            Serial.println("Activated true");
            digitalWrite(BLUE, HIGH); //visualize relay activation with the LED
        }
        else
        {
            Serial.println("Activated false");
            digitalWrite(BLUE, LOW);
        }
    }
}

void setup()
{
    Serial.begin(115200);

    while (!Serial)
    {
        yield();
    }
    delay(2000);

    Serial.setDebugOutput(true);

    pinMode(LDR, INPUT);
    pinMode(BUTTON, INPUT);
    pinMode(RED, OUTPUT);
    pinMode(GREEN, OUTPUT);
    pinMode(BLUE, OUTPUT);

    analogWrite(RED, 500);

    Serial.print("Connecting to WiFi...");
    //Begin WiFi joining with provided Access Point name and password
    WiFi.begin(AP_SSID, AP_PASS);

    //Handle WiFi events
    e1 = WiFi.onStationModeGotIP(onSTAGotIP); // As soon WiFi is connected, start the Client
    e2 = WiFi.onStationModeDisconnected(onSTADisconnected);

    //Handle client events
    client.onEvent(onClientEvent);

    //Add command to handle and its handler
    //Command format is assumed like this: {"Name":"[COMMAND_NAME]","Parameters":[PARAMETERS_JSON_ARRAY]}
    client.onCloudCommand("ActivateLed", onActivateLedCommand);
}

void onMessageCallback(const MQTT::Publish &msg)
{
    //Handle Cloud to Device message by yourself.

    // if (msg.payload_len() == 0)
    // {
    //     return;
    // }

    // Serial.println(msg.payload_string());
}

void connectToIoTHub()
{
    Serial.print("\nBeginning Azure IoT Hub Client... ");
    if (client.begin())
    {
        Serial.println("OK");
    }
    else
    {
        Serial.println("Could not connect to MQTT");
    }
}

void loop()
{
    //MUST CALL THIS in loop()
    client.run();

    if (client.connected())
    {
        analogWrite(GREEN, 500);

        // Publish a message roughly every 3 second. Only after time is retrieved and set properly.
        if (millis() - lastMillis > 3000 && timeStatus() != timeNotSet)
        {
            lastMillis = millis();

            int adcValue = analogRead(LDR);
            Serial.print("LDR: ");
            Serial.println(adcValue);
            Serial.print("BUTTON: ");
            Serial.println(digitalRead(BUTTON));

            //Get current timestamp, using Time lib
            time_t currentTime = now();

            //Or instead, use this more convenient way
            AzureIoTHubMQTTClient::KeyValueMap keyVal = {{"ADC", adcValue}, {"DeviceId", DEVICE_ID}, {"EventTime", currentTime}};
            client.sendEventWithKeyVal(keyVal);
        }
    }
    else
    {
        Serial.println("Not connected to IoT Hub");
    }

    delay(10); // <- fixes some issues with WiFi stability
}
