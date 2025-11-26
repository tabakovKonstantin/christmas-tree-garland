#ifndef MqttManager_h
#define MqttManager_h

#include "Config.h"
#include "LedControl.h"
#include <AsyncMqttClient.h>
#include "mqtt/MqttRouter.h"
#include "mqtt/LightCommandHandler.h"
#include "mqtt/OtaCommandHandler.h"

#define DISCOVERY_TOPIC_TEMPLATE "homeassistant/light/%s/config"
#define COMMAND_TOPIC_TEMPLATE   "home/lights/%s/set"
#define OTA_TOPIC_TEMPLATE       "home/lights/%s/ota"
#define STATE_TOPIC_TEMPLATE     "home/lights/%s/state"

class MqttManager
{
public:
    explicit MqttManager(LedControl& led_Control);
    void init();

private:
    Config config;
    LedControl& ledControl;

    // Topics for this device instance
    String commandTopic;
    String otaTopic;
    String stateTopic;

    // Router and handlers for message distribution
    MqttRouter router;
    LightCommandHandler lightHandler;
    OtaCommandHandler otaHandler;

    void connectToMqtt();
    void onMqttConnect(bool sessionPresent);
    void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
    void onMqttMessage(char *topic,
                       char *payload,
                       AsyncMqttClientMessageProperties properties,
                       size_t len,
                       size_t index,
                       size_t total);
    void publishDiscoveryMessage();
    void publishInitialState();

    String getProductId();
    String buildDiscoveryTopic() const;
    String buildCommandTopic() const;
    String buildOtaTopic() const;
    String buildStateTopic() const;
};

#endif