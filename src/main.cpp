#define LOGGING
#define USE_NFC
//#define USE_SCALE
#define DEMO 


#include <Arduino.h>
#include <WiFiNINA.h>
#include <x_nucleo_nfc04.h>
#include <Wire.h>
#include <LCD.h>
#include <L298N.h>
#include <LiquidCrystal_I2C.h>
#include <HX711.h>

#include "secrets.h"
#include "constants.h"

#include "debughelper.h"
#include "Auth0Arduino.h"


#define SET_LCD_TEXT_2(X, Y) {\
    String line1(F(X));\
    String line2(F(Y));\
    updateLCD(&line1, &line2);\
}

#define SET_LCD_TEXT(X) {\
    String line1(F(X));\
    updateLCD(&line1);\
}

char ssid[] = WIFI_SSID; // your network SSID (name)
char pass[] = WIFI_PASS; // your network password (use for WPA, or use as key for WEP)
char ssid_fallback[] = FALLBACK_WIFI_SSID;
char pass_fallback[] = FALLBACK_WIFI_PASS;

int status = WL_IDLE_STATUS;

L298N pumps[2] = {
    {enOPEN, in1, in2},
    {enOPEN, in3, in4}
};

WiFiSSLClient sslClient;
LiquidCrystal_I2C lcd(I2C_ADDR, En_pin, Rw_pin, Rs_pin, D4_pin, D5_pin, D6_pin, D7_pin);

#ifdef USE_SCALE
HX711 scale;
int32_t baseWeight;
#endif

struct UserInfoWithDrink : UserInfo
{
    int32_t qty;
    int32_t id;
};


/**
 * Char Buffs
 */
void updateLCD(String *line1 = nullptr, String *line2 = nullptr) {

    lcd.home();
    lcd.clear();

    if (line1 != nullptr) {
        lcd.print(line1->c_str());  
    }
    
    if (line2 != nullptr) {
        lcd.setCursor(0, 1);
        lcd.print(line2->c_str());
    }
}

/**
    * at max rpm of 1.8L/m we should be pumping 0.03L/30ml per second 
    * for a 300ml drink this shall take 10 seconds, thus we will 
    * calculate what exactly needs to happen here.
    * qty is specified in ml.
    * 
    */
void serveDrink(const UserInfoWithDrink& info)
{
    DEBUG("PUMPS RUNNING");
    pumps[info.id].forward();
    delay(((float)info.qty/30) * 1000);
    pumps[info.id].stop();
    DEBUG("PUMPS STOPPED");
    // /**
    //  * Disabled atm as we only have 1 drink
    //  */ 
    // for (int i = 0; i < MAX_COMPLEXITY_OF_DRINK; i++)
    // {
    //     Ingredient ingredient = ingredients[i];

    //     // End.
    //     if (ingredient.id == -1)
    //     {
    //         return;
    //     }

    //     DEBUG(F("Pouring: ") + ingredient.qty + "ml of" + ingredient.id);
    //     pumps[ingredient.id].forwardFor(ingredient.qty / 30U);
    // }
}

void waitForUser()
{
#ifdef USE_SCALE
    SET_LCD_TEXT_2(
        "To begin",
        "put a glass"
    );

#ifdef USE_NFC
    X_Nucleo_Nfc04.ledOn(GREEN_LED);
#endif

    while (scale.get_units(3) < 120) {} 

#ifdef USE_NFC
    X_Nucleo_Nfc04.ledOff(GREEN_LED);
#endif

    SET_LCD_TEXT_2(
        "Please wait",
        "Getting code"
    );
#endif
}

void waitForUserAway()
{
#ifdef USE_SCALE
    SET_LCD_TEXT_2(
        "Done",
        "Please Remove"
    );

    while (scale.get_units(3) > 120) {} 
#endif
}

void onDeviceAuthorization(const DeviceAuthorizationResponse &deviceParams)
{
    DEBUG("Authz ready");
#ifdef USE_NFC
    SET_LCD_TEXT_2(
        "I'M READY",
        "TAP ON NFC or"
    );
    String nfcUrl = deviceParams.verificationUriComplete.substring(8);
    DEBUG(nfcUrl);

    int failure = X_Nucleo_Nfc04.writeURI(URI_ID_0x04_STRING, nfcUrl, "");
    if (failure)
    {
        DEBUG(String("Writing to NFC04A1 Failed") + failure);
        while (1);
    }

    #ifdef USE_NFC
        X_Nucleo_Nfc04.ledOn(BLUE_LED);
    #endif

    delay(5000);
#endif
    String out1(F("Goto u.nu/dhrx"));
    String out2(F("Code:"));
    out2 += deviceParams.userCode;

    updateLCD(
        &out1,
        &out2
    );

}

void demo()
{
    waitForUser();

    DEBUG("Generating auth code");

    UserInfoWithDrink userInfo;
    AuthorizationRequestParams authParams = {
        F(AUTH0_CLIENT_ID),
        F(AUTH0_SCOPE),
        F(AUTH0_AUDIENCE),
        F(AUTH0_RESPONSE_TYPE)
    };

    Auth0Arduino auth0(sslClient, AUTH0_DOMAIN, &onDeviceAuthorization);

    DEBUG("Initialized auth0 library");
    int failure = auth0.getUserInfo(authParams, userInfo, [](const JsonDocument &doc, UserInfoWithDrink &extendedInfo) {
        DEBUG("Extract drink info");
        extendedInfo.qty = doc["https://d.q/qty"].as<int32_t>();
        extendedInfo.id  = doc["https://d.q/id"].as<int32_t>();
    });
    
    DEBUG("Auth handshake finished");

    if (failure)
    {
        DEBUG("Auth failed or error in token" + failure);
        return failure;
    }

    #ifdef USE_NFC
        X_Nucleo_Nfc04.ledOff(BLUE_LED);
    #endif

    DEBUG("Got drink");

    String out1(F("Serving "));
    out1 += (userInfo.id == 0 ? "Sparkling Water" : "Budweiser");

    String out2(F("for "));
    out2 += userInfo.name;


    updateLCD(
        &out1,
        &out2
    );

    serveDrink(userInfo);
    waitForUserAway();
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
    lcd.begin(16, 2); //  <<----- My LCD was 16x2
    // Switch on the backlight
    lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
    lcd.setBacklight(HIGH);

    SET_LCD_TEXT("Starting");
    
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

    SET_LCD_TEXT(
        "Wifi Connecting"
    );
    
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

    SET_LCD_TEXT("WIFI CONNECTED");
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

#ifdef USE_SCALE
    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    scale.set_scale(2280.f); // this value is obtained by calibrating the scale with known weights; see the README for details
    scale.tare();            // reset the scale to 0
    baseWeight = scale.get_units(5);
#endif
#endif 
    DEBUG("Init Complete");
}

void clean() {
    pumps[0].forward();   
    pumps[1].forward();
    delay(10000);
    pumps[0].stop();   
    pumps[1].stop();
}
void loop()
{
    DEBUG("SHOULD DEMO NOW");
#ifdef DEMO
    demo();
#else 
    clean();
#endif 
}