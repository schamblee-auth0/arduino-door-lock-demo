/**
 * Device hardware
 */ 
#define I2C_ADDR    0x27 // <<----- Add your address here.  Find it from I2C Scanner
#define BACKLIGHT_PIN 3
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7

// #define BOARDA
#define BOARDB

#ifdef BOARDA
// motors
#define enOPEN 11

#define in1 2
#define in2 3
#define in3 4
#define in4 5

#define LOADCELL_DOUT_PIN 8
#define LOADCELL_SCK_PIN 9
#endif 

#ifdef BOARDB 
#define enOPEN 6
#define in1 10
#define in2 11
#define in3 12
#define in4 13

#define LOADCELL_DOUT_PIN 8
#define LOADCELL_SCK_PIN 9
#endif



#define MAX_COMPLEXITY_OF_DRINK 1
/**
 * Wonder if we can use #Defines for these as well
 */
#define AUTH0_CLIENT_ID "FWy6EnUCp5v7yTDJaEbn7q72CZW8BwNW"
#define AUTH0_SCOPE "openid profile"
#define AUTH0_AUDIENCE "https://test.com"
#define AUTH0_RESPONSE_TYPE "token"
#define AUTH0_DOMAIN "example-connections.us.auth0.com"
// #define AUTH0_DOMAIN "auth0.tryappleid.com"
// #define AUTH0_DOMAIN "entvrx00gy7y.x.pipedream.net"