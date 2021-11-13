#include <esp_event.h>
#include <esp_event_loop.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <nvs_flash.h>

#include "mongoose.h"
#include "sdkconfig.h"

#define SSID     "SSID"
#define PASSWORD "PASSWORD"

static const char *s_address = "broker.losant.com:1883";
static const char *s_client_id = "CLIENT_ID";
static const char *s_user_name = "USER_NAME";
static const char *s_password = "PASSWORD";
static struct mg_mqtt_topic_expression s_topic_expr = {NULL, 0};

static char tag []="mongooseTests";

const char *mongoose_eventToString(int ev) {
    static char temp[100];
    switch (ev) {
        case MG_EV_CONNECT:
            return "MG_EV_CONNECT";
        case MG_EV_ACCEPT:
            return "MG_EV_ACCEPT";
        case MG_EV_CLOSE:
            return "MG_EV_CLOSE";
        case MG_EV_SEND:
            return "MG_EV_SEND";
        case MG_EV_RECV:
            return "MG_EV_RECV";
        case MG_EV_HTTP_REQUEST:
            return "MG_EV_HTTP_REQUEST";
        case MG_EV_HTTP_REPLY:
            return "MG_EV_HTTP_REPLY";
        case MG_EV_MQTT_CONNACK:
            return "MG_EV_MQTT_CONNACK";
        case MG_EV_MQTT_CONNACK_ACCEPTED:
            return "MG_EV_MQTT_CONNACK";
        case MG_EV_MQTT_CONNECT:
            return "MG_EV_MQTT_CONNECT";
        case MG_EV_MQTT_DISCONNECT:
            return "MG_EV_MQTT_DISCONNECT";
        case MG_EV_MQTT_PINGREQ:
            return "MG_EV_MQTT_PINGREQ";
        case MG_EV_MQTT_PINGRESP:
            return "MG_EV_MQTT_PINGRESP";
        case MG_EV_MQTT_PUBACK:
            return "MG_EV_MQTT_PUBACK";
        case MG_EV_MQTT_PUBCOMP:
            return "MG_EV_MQTT_PUBCOMP";
        case MG_EV_MQTT_PUBLISH:
            return "MG_EV_MQTT_PUBLISH";
        case MG_EV_MQTT_PUBREC:
            return "MG_EV_MQTT_PUBREC";
        case MG_EV_MQTT_PUBREL:
            return "MG_EV_MQTT_PUBREL";
        case MG_EV_MQTT_SUBACK:
            return "MG_EV_MQTT_SUBACK";
        case MG_EV_MQTT_SUBSCRIBE:
            return "MG_EV_MQTT_SUBSCRIBE";
        case MG_EV_MQTT_UNSUBACK:
            return "MG_EV_MQTT_UNSUBACK";
        case MG_EV_MQTT_UNSUBSCRIBE:
            return "MG_EV_MQTT_UNSUBSCRIBE";
        case MG_EV_WEBSOCKET_HANDSHAKE_REQUEST:
            return "MG_EV_WEBSOCKET_HANDSHAKE_REQUEST";
        case MG_EV_WEBSOCKET_HANDSHAKE_DONE:
            return "MG_EV_WEBSOCKET_HANDSHAKE_DONE";
        case MG_EV_WEBSOCKET_FRAME:
            return "MG_EV_WEBSOCKET_FRAME";
    }
    sprintf(temp, "Unknown event: %d", ev);
    return temp;
}

char *mgStrToStr(struct mg_str mgStr) {
    char *retStr = (char *) malloc(mgStr.len + 1);
    memcpy(retStr, mgStr.p, mgStr.len);
    retStr[mgStr.len] = 0;
    return retStr;
}

void mongoose_event_handler(struct mg_connection *nc, int ev, void *evData) {
    struct mg_mqtt_message *msg = (struct mg_mqtt_message *) evData;
    (void) nc;

    if (ev != MG_EV_POLL) printf("USER HANDLER GOT EVENT %s\n", mongoose_eventToString(ev));

    switch (ev) {
        case MG_EV_CONNECT: {
            struct mg_send_mqtt_handshake_opts opts;
            memset(&opts, 0, sizeof(opts));
            opts.user_name = s_user_name;
            opts.password = s_password;

            mg_set_protocol_mqtt(nc);
            mg_send_mqtt_handshake_opt(nc, s_client_id, opts);
            break;
        }
        case MG_EV_MQTT_CONNACK:
            if (msg->connack_ret_code != MG_EV_MQTT_CONNACK_ACCEPTED) {
                printf("Got mqtt connection error: %d\n", msg->connack_ret_code);
                exit(1);
            }

            //char s_topic[100];
            //snprintf(s_topic, sizeof(s_topic), "losant/%s/state", s_client_id);
            printf("Publishing to /test\n");
            mg_mqtt_publish(nc, "/test", 65, MG_MQTT_QOS(0), "Hello", 5);
            break;
        case MG_EV_MQTT_PUBACK:
            printf("Message publishing acknowledged (msg_id: %d)\n", msg->message_id);
            break;
        case MG_EV_CLOSE:
            printf("Connection closed\n");
            exit(1);
    }
}

void mongooseTask(void *data) {
    ESP_LOGD(tag, "Mongoose task starting");
    struct mg_mgr mgr;
    ESP_LOGD(tag, "Mongoose: Starting setup");
    mg_mgr_init(&mgr, NULL);
    ESP_LOGD(tag, "Mongoose: Succesfully inited");
    struct mg_connection *c = mg_connect(&mgr, s_address, mongoose_event_handler);
    ESP_LOGD(tag, "Mongoose Successfully bound");
    if (c == NULL) {
        ESP_LOGE(tag, "No connection from the mg_bind()");
        vTaskDelete(NULL);
        return;
    }

    while (1) {
        mg_mgr_poll(&mgr, 1000);
    }
}

esp_err_t wifi_event_handler(void *ctx, system_event_t *event) {
    if (event->event_id == SYSTEM_EVENT_STA_GOT_IP) {
        ESP_LOGD(tag, "Got an IP: " IPSTR, IP2STR(&event->event_info.got_ip.ip_info.ip));
        xTaskCreatePinnedToCore(&mongooseTask, "mongooseTask", 20000, NULL, 5, NULL,0);
    }
    if (event->event_id == SYSTEM_EVENT_STA_START) {
        ESP_LOGD(tag, "Received a start request");
        ESP_ERROR_CHECK(esp_wifi_connect());
    }
    return ESP_OK;
}

extern "C" int app_main(void) {
    nvs_flash_init();
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(wifi_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_config_t sta_config = {};
    strcpy((char*)sta_config.sta.ssid, SSID);
    strcpy((char*)sta_config.sta.password, PASSWORD);
    sta_config.sta.bssid_set = 0;

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    return 0;
}
