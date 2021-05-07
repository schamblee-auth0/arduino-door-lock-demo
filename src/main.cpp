#define LOGGING
#define USE_NFC
#define USE_SOLENOID
#define DEMO 


#include <Arduino.h>
#include <WiFiNINA.h>
#include <x_nucleo_nfc04.h>

#include "secrets.h"
#include "constants.h"

#include "debughelper.h"
#include "Auth0Arduino.h"


char ssid[] = WIFI_SSID;
char pass[] = WIFI_PASS;
char ssid_fallback[] = FALLBACK_WIFI_SSID;
char pass_fallback[] = FALLBACK_WIFI_PASS;

int status = WL_IDLE_STATUS;

WiFiSSLClient sslClient;

int solenoidPin = D13_pin;

void onDeviceAuthorization(const DeviceAuthorizationResponse &deviceParams)
{
    DEBUG("Authz ready");
    String nfcUrl = deviceParams.verificationUriComplete.substring(8);
    DEBUG(nfcUrl);

    #ifdef USE_NFC
        int failure = X_Nucleo_Nfc04.writeURI(URI_ID_0x04_STRING, nfcUrl, "");
        if (failure)
        {
            DEBUG(String("Writing to NFC04A1 Failed") + failure);
            while (1);
        }

        X_Nucleo_Nfc04.ledOff(GREEN_LED);
        X_Nucleo_Nfc04.ledOn(BLUE_LED);
    #endif

    #ifdef USE_SOLENOID
        digitalWrite(solenoidPin, LOW);
    #endif

    delay(5000);

    String out1(F("Goto u.nu/dhrx"));
    String out2(F("Code:"));
    out2 += deviceParams.userCode;
}

void demo()
{
    DEBUG("Generating auth code");

    AuthorizationRequestParams authParams = {
        F(AUTH0_CLIENT_ID),
        F(AUTH0_SCOPE),
        F(AUTH0_RESPONSE_TYPE)
    };

    Auth0Arduino auth0(sslClient, AUTH0_DOMAIN, &onDeviceAuthorization);

    UserInfo userInfo;

    DEBUG("Initialized auth0 library");
    int failure = auth0.getUserInfo(authParams, userInfo);
    
    DEBUG("Auth handshake finished");

    if (failure)
    {
        DEBUG("Auth failed or error in token" + failure);
        return failure;
    }

    #ifdef USE_NFC
        X_Nucleo_Nfc04.ledOff(BLUE_LED);
        X_Nucleo_Nfc04.ledOn(GREEN_LED);
    #endif

    #ifdef USE_SOLENOID
        digitalWrite(solenoidPin, HIGH);
    #endif

    return failure;
}

void printWiFiStatus()
{
    DEBUG("Connected to wifi");
    DEBUG("SSID: ");
    DEBUG(WiFi.SSID());
    IPAddress ip = WiFi.localIP();
    DEBUG("IP Address: ");
    DEBUG(ip);
    long rssi = WiFi.RSSI();
    DEBUG("signal strength (RSSI):");
    DEBUG(rssi);
    DEBUG(" dBm");
}

void setup()
{
    INIT_DEBUGGER(9600);
    #ifdef DEMO

        #ifdef USE_SOLENOID
            pinMode(solenoidPin, OUTPUT);
            digitalWrite(solenoidPin, LOW);
        #endif
        
        if (WiFi.status() == WL_NO_MODULE)
        {
            DEBUG("Communication with WiFi module failed!");
            // don't continue
            while (true)
                ;
        }

        String fv = WiFi.firmwareVersion();
        DEBUG(fv);
        if (fv < WIFI_FIRMWARE_LATEST_VERSION)
        {
            DEBUG("Please upgrade the firmware");
        }
        
        // attempt to connect to WiFi network:
        while (status != WL_CONNECTED)
        {

            DEBUG("Attempting to connect to SSID: ");
            DEBUG(ssid);

            // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
            status = WiFi.begin(ssid, pass);
            if (status != WL_CONNECTED) {
                status = WiFi.begin(ssid_fallback, pass_fallback);
                // wait 10 seconds for connection:
                delay(10000);
            }
        }
        printWiFiStatus();

        #ifdef USE_NFC

            DEBUG("Initializing X_Nucleo_Nfc04");

            if (X_Nucleo_Nfc04.begin() == 0)
            {
                DEBUG("Nucleo System Init done!");
            }
            else
            {
                DEBUG("System Init failed!");
                while (1)
                    ;
            }

        #endif
    #endif 
    DEBUG("Init Complete");
}

void loop()
{
    DEBUG("SHOULD DEMO NOW");
    #ifdef DEMO
        demo();
    #endif 
}