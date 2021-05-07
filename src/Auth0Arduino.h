#define A0_WL_NOT_CONNECTED -1
#define A0_DEVICE_AUTHZ_FAILED -2
#define A0_DEVICE_TOKEN_FAILED -3

#define OAUTH_DEVICE_GRANT_ENDPOINT "/oauth/device/code"
#define CONTENT_TYPE_URL_ENCODED "application/x-www-form-urlencoded"
#define OAUTH_TOKEN_ENDPOINT "/oauth/token"
#define OAUTH_USERINFO_ENDPOINT "/userinfo"

/**
 * Higher level imports from ArduinoHTTPLibs
 */
#include <Arduino.h>
#include <WiFiNINA.h>
#include <String.h>
#include <ArduinoJson.h>
#include <ArduinoHttpClient.h>


#include "debughelper.h"

#ifndef AUTH0_ARDUINO_H
#define AUTH0_ARDUINO_H
/**
 * Optionals
 */
#ifndef AUTH0
#define AUTH0 1
#endif

/**
 * Options
 */
#ifndef OPENID_CONNECT
#define OPENID_CONNECT 1
#endif

/**
 * Enables 
 */
#ifndef REFRESH_TOKENS
#define REFRESH_TOKENS 1
#endif

#define GRANT_TYPE_DEVICE_FLOW "urn:ietf:params:oauth:grant-type:device_code"

struct UserInfo
{
    String sub;
    String name;
    String given_name;
    String family_name;
    String middle_name;
    String nickname;
    String preferred_username;
    String profile;
    String picture;
    String website;
    String email;
    bool email_verified;
    String gender;
    String birthdate;
    String zoneinfo;
    String locale;
    String phone_number;
    bool phone_number_verified;
    String address;
    String updated_at;
};

#ifdef OPENID_CONNECT

/**
 * struct for idTokenClaims
 */
struct IDTokenClaims : UserInfo
{
    String iss;
    String aud;
    int32_t exp;
    int32_t nbf;
    int32_t iat;
    String jti;
    String azp;
    String nonce;
    String auth_time;
    String at_hash;
    String c_hash;
    String acr;
    String amr;
    String sub_jwk;
    String cnf;
    String sid;
};
#endif

struct ParamPair
{
    String key;
    String value;
};

struct AuthorizationRequestParams
{
    /**
     * Client ID 
     */
    String clientId;

    /**
     * The default scope to be used on authentication requests.
     * `openid profile email` is always added to all requests.
     */
    String scope;

    String responseType;
#ifdef OPENID_CONNECT
    /**
     * - `'page'`: displays the UI with a full page view
     * - `'popup'`: displays the UI with a popup window
     * - `'touch'`: displays the UI in a way that leverages a touch interface
     * - `'wap'`: displays the UI with a "feature phone" type interface
     */
    String display;
    /**
     * - `'none'`: do not prompt user for login or consent on reauthentication
     * - `'login'`: prompt user for reauthentication
     * - `'consent'`: prompt user for consent before processing request
     * - `'select_account'`: prompt user to select an account
     */
    String prompt;
    /**
     * Maximum allowable elasped time (in seconds) since authentication.
     * If the last time the user authenticated is greater than this value,
     * the user must be reauthenticated.
     */
    String max_age;
    /**
     * The space-separated list of language tags, ordered by preference.
     * For example: `'fr-CA fr en'`.
     */
    String ui_locales;

    /**
     * Previously issued ID Token.
     */
    String id_token_hint;
    /**
     * The user's email address or other identifier. When your app knows
     * which user is trying to authenticate, you can provide this parameter
     * to pre-fill the email box or select the right session for sign-in.
     */
    String login_hint;
    String acr_values;
    /**
     * The default audience to be used for requesting API access.
     */
#endif

#ifdef AUTH0
    /**
     * The name of the connection configured for your application.
     * If null, it will redirect to the Auth0 Login Page and show
     * the Login Widget.
     */
    String connection;
#endif
};

struct AuthorizationResponse
{

#ifdef OPENID_CONNECT
    String idToken;
#endif

    String accessToken;
    int32_t expiresIn;
    String scope;
};

struct DeviceAuthorizationResponse
{
    String deviceCode;
    String userCode;
    String verificationUri;
    String verificationUriComplete;
    int32_t expiresIn;
    int32_t interval;
};

using DeviceAuthorizationHandler = void (*) (const DeviceAuthorizationResponse& );
using ExtendedPropertiesHandler = void (*) (const JsonDocument&, UserInfo&);

class Auth0Arduino
{

private:
    DeviceAuthorizationHandler onDeviceAuthorization;
    HttpClient *client;

public:
    Auth0Arduino(
        WiFiSSLClient &sslClient,
        char domain[],
        DeviceAuthorizationHandler onDeviceAuthorization,
        int32_t port = 443
    );
    ~Auth0Arduino();

    /**
         * Log the user in
         */
    int authorize(
        const AuthorizationRequestParams &params,
        AuthorizationResponse &response);

    int getAccessToken( 
        const AuthorizationRequestParams &params,
        String &token);

    int getUserInfo(
        const AuthorizationRequestParams &params,
        UserInfo &userInfo);

    int getUserInfo(
        const String &accessToken,
        UserInfo &userInfo);

    int getUserInfo(
        const AuthorizationRequestParams &params,
        UserInfo &userInfo,
        ExtendedPropertiesHandler handleExtendedProps
    );

    int getUserInfo(
        const String &accessToken,
        UserInfo &userInfo,
        void (*handleExtendedProps)(const JsonDocument &, UserInfo &));

private:
    int executeDeviceAuthorizationRequest(
        const AuthorizationRequestParams &params,
        DeviceAuthorizationResponse &dar);

    int executeTokenRequest(
        const AuthorizationRequestParams &params,
        const DeviceAuthorizationResponse &dar,
        AuthorizationResponse &response);

    int pollTokenTillComplete(
        const AuthorizationRequestParams &params,
        const DeviceAuthorizationResponse &dar,
        AuthorizationResponse &response);
};

#endif