#include "Auth0Arduino.h"

String encodeParams(ParamPair params[], size_t howMany) {
    String encodedParams;
    bool first = false;
    
    for (size_t i = 0; i < howMany; i++) {
        ParamPair *param = &params[i];

        if (first == false) {
            first = true;
        } else {
            encodedParams += "&";
        }

        encodedParams += URLEncoder.encode(param->key);
        encodedParams += "=";
        encodedParams += URLEncoder.encode(param->value);
    }
        
    return encodedParams;
}

int Auth0Arduino::executeTokenRequest(
    const AuthorizationRequestParams &params,
    const DeviceAuthorizationResponse &dar,
    AuthorizationResponse &response
) {

    DEBUG(F("Calling /oauth/token")); 

    ParamPair tokenParams[] = {
        {F("grant_type"), GRANT_TYPE_DEVICE_FLOW},
        {F("client_id"), params.clientId},
        {F("device_code"), dar.deviceCode}
    };

 
    int failure = this->client->post(
        OAUTH_TOKEN_ENDPOINT,
        CONTENT_TYPE_URL_ENCODED,
        encodeParams(tokenParams, 3)
    );

    if (failure) {
        DEBUG(F("HTTPClient: Failed to make a request due to :") + failure);
        return A0_DEVICE_TOKEN_FAILED;
    }

    int32_t responseCode = this->client->responseStatusCode();
    int32_t contentLength = this->client->contentLength();

    if (!contentLength) {
        DEBUG(F("We recieved no response, http failed"));
        return A0_DEVICE_AUTHZ_FAILED;
    }

    /**
     * Holds the decoded json document
     */ 
    const size_t capacity = JSON_OBJECT_SIZE(5) + 1250;
    DynamicJsonDocument doc(capacity);
    DeserializationError error = deserializeJson(doc, *(this->client));
    

    if (error) {
        DEBUG(F("deserializeJson() failed: "));
        DEBUG(error.c_str());
        return A0_DEVICE_AUTHZ_FAILED;
    }


    if (responseCode != 200) {

        String error = doc["error"].as<String >();
        String errorMessage = doc["error_message"].as<String >();
        DEBUG("Failed: " + error + "\n" + errorMessage);
        return responseCode;
    }


#ifdef OPENID_CONNECT 
    response.idToken = doc["id_token"].as<String >();
#endif

    response.accessToken = doc["access_token"].as<String>();
    response.expiresIn = doc["expires_in"].as<int32_t>();
    response.scope = doc["scope"].as<String >();
    
    DEBUG("/oauth/token Success");
    return 0;
}



int Auth0Arduino::executeDeviceAuthorizationRequest(
    const AuthorizationRequestParams &params,
    DeviceAuthorizationResponse &dar
) {
    ParamPair authParams[] = {
        {"client_id", params.clientId},
        {"response_type", params.responseType},
        {"scope", params.scope},
        {"audience", params.audience}
    };

    int failure = this->client->post(
        OAUTH_DEVICE_GRANT_ENDPOINT,
        CONTENT_TYPE_URL_ENCODED,
        encodeParams(authParams, 4)
    );

    if (failure) {
        DEBUG(F("Device Authorization Request failed because:") + failure);
        return A0_DEVICE_AUTHZ_FAILED;
    }

    int32_t responseCode = this->client->responseStatusCode();
    int32_t contentLength = this->client->contentLength();

    if (!contentLength) {
        DEBUG(F("Device Authorization Request failed because:"));
        return A0_DEVICE_AUTHZ_FAILED;
    }

    /**
     * Holds the decoded json document
     */ 
    const size_t capacity = JSON_OBJECT_SIZE(6) + 250;
    DynamicJsonDocument doc(capacity);
    DeserializationError error = deserializeJson(doc, *(this->client));
    

    if (error) {
        DEBUG(F("deserializeJson() failed: ") + error.c_str());
        return A0_DEVICE_AUTHZ_FAILED;
    }


    if (responseCode != 200) {
        String error = doc["error"].as<String >();
        String errorMessage = doc["error_message"].as<String >();
        DEBUG(F("Request Failed w/ Auth0 ") + error + "\n" + errorMessage);
        return responseCode;
    }

    dar.deviceCode = doc["device_code"].as<String >();
    dar.userCode = doc["user_code"].as<String >();
    dar.verificationUri = doc["verification_uri"].as<String >();
    dar.verificationUriComplete = doc["verification_uri_complete"].as<String >();
    dar.expiresIn = doc["expires_in"].as<int32_t>();
    dar.interval = doc["interval"].as<int32_t>();

    return 0;
}

int Auth0Arduino::pollTokenTillComplete(
    const AuthorizationRequestParams &params,
    const DeviceAuthorizationResponse &dar,
    AuthorizationResponse &response
) {
    const int32_t interval = dar.interval * 1000;
    int32_t retries  = 100;

    while(retries-- > 0) {
        DEBUG("Trying token request...");
        int32_t resp = executeTokenRequest(params, dar, response);
        if (resp == 0) {
            return 0;
        }
        DEBUG("Authorization not complete, Waiting...");
        delay(interval);
    }

    return A0_DEVICE_TOKEN_FAILED;
}

int Auth0Arduino::authorize(
    const AuthorizationRequestParams &params,
    AuthorizationResponse& response
) {
    DEBUG("Calling /oauth/device");
    DeviceAuthorizationResponse dar;
    int err = this->executeDeviceAuthorizationRequest(params, dar);

    if (err) {
        return err;
    }

    DEBUG("Calling external function");

    if (this->onDeviceAuthorization == nullptr) {
        DEBUG("EYY YOU CALLING NULL BRO!");
        return -99999999;
    }

    this->onDeviceAuthorization(dar);
    err = this->pollTokenTillComplete(
        params, 
        dar, 
        response
    );

    if (err) {
        return err;
    }
    DEBUG("Auth finished");
    return 0;
}

int Auth0Arduino::getAccessToken(
    const AuthorizationRequestParams &params,
    String& token
) {
    AuthorizationResponse response;
    int32_t authResult = this->authorize(params, response);
    if (authResult) {
        return authResult;
    }
    token = response.accessToken;
    return 0;
}

int Auth0Arduino::getUserInfo(
    const AuthorizationRequestParams &params,
    UserInfo &userInfo
) {
    this->getUserInfo(params, userInfo, [](const JsonDocument&, UserInfo&){});
}

int Auth0Arduino::getUserInfo(
    const String &accessToken,
    UserInfo &userInfo) {
        this->getUserInfo(accessToken, userInfo, [](const JsonDocument&, UserInfo&){});
    }

int Auth0Arduino::getUserInfo(
    const AuthorizationRequestParams &params,
    UserInfo &userInfo, 
    ExtendedPropertiesHandler handleExtendedProps
) {
    DEBUG(F("Trying to get access token for /userinfo"));
    String accessToken;
    int failure = this->getAccessToken(params, accessToken);
    if (failure) {
        DEBUG("Failed to Get access Token" + failure);
        return failure;
    }

    failure = this->getUserInfo(accessToken, userInfo, handleExtendedProps);

    if (failure) {
        return failure;
    }

    return 0;
}



int Auth0Arduino::getUserInfo(
    const String& accessToken,
    UserInfo& userInfo,
    ExtendedPropertiesHandler handleExtendedProps
) {
    DEBUG("Calling /userinfo endpoint");
    this->client->beginRequest();
    int failure = this->client->get(OAUTH_USERINFO_ENDPOINT);
    this->client->sendHeader("Authorization", String(F("Bearer ")) + accessToken);
    this->client->endRequest();

    DEBUG(F("AT:") + accessToken);

    if (failure) {
        DEBUG(F("Device Authorization Request failed because:") + failure);
        return A0_DEVICE_AUTHZ_FAILED;
    }

    int32_t responseCode = this->client->responseStatusCode();
    int32_t contentLength = this->client->contentLength();

    if (!contentLength) {
        DEBUG("We recieved no response, http failed");
        return A0_DEVICE_AUTHZ_FAILED;
    }

    /**
     * Holds the decoded json document
     */ 
    const size_t capacity = JSON_OBJECT_SIZE(7) + 800;
    DynamicJsonDocument doc(capacity);
    DeserializationError error = deserializeJson(doc, *(this->client));

    if (error) {
        DEBUG(F("deserializeJson() failed: ") + error.c_str());
        return A0_DEVICE_AUTHZ_FAILED;
    }

    if (failure) {
        return failure;
    }

    userInfo.sub = doc["sub"].as<String>();
    userInfo.name = doc["name"].as<String>();
    userInfo.given_name = doc["given_name"].as<String>();
    userInfo.family_name = doc["family_name"].as<String>();
    userInfo.middle_name = doc["middle_name"].as<String>();
    userInfo.nickname = doc["nickname"].as<String>();
    userInfo.preferred_username = doc["preferred_username"].as<String>();
    userInfo.profile = doc["profile"].as<String>();
    userInfo.picture = doc["picture"].as<String>();
    userInfo.website = doc["website"].as<String>();
    userInfo.email = doc["email"].as<String>();
    userInfo.email_verified = doc["email_verified"].as<bool>();
    userInfo.gender = doc["gender"].as<String>();
    userInfo.birthdate = doc["birthdate"].as<String>();
    userInfo.zoneinfo = doc["zoneinfo"].as<String>();
    userInfo.locale = doc["locale"].as<String>();
    userInfo.phone_number = doc["phone_number"].as<String>();
    userInfo.phone_number_verified = doc["phone_number_verified"].as<bool>();
    userInfo.address = doc["address"].as<String>();
    userInfo.updated_at = doc["updated_at"].as<int32_t>();

    handleExtendedProps(doc, userInfo);

    return 0;
}


Auth0Arduino::Auth0Arduino(
    WiFiSSLClient &sslClient,
    char domain[],
    DeviceAuthorizationHandler onDeviceAuthorization,
    int32_t port = 443
){
    this->client = new HttpClient(sslClient, domain, port);
    this->onDeviceAuthorization = onDeviceAuthorization;
}

Auth0Arduino::~Auth0Arduino() {

}
