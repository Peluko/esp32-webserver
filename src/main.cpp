#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#include <esp_task_wdt.h>

#include "wifi_config.h"

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASS;

WebServer server(80);

const int led = 2;

#define WATCHDOG_TIME_SECONDS 5

unsigned long _init_millis;
unsigned long _request_count;


void handleRoot()
{
    digitalWrite(led, 1);
    auto uptime = (millis() - _init_millis) / 1000;
    auto uptime_days = uptime / (24 * 60 * 60);
    auto uptime_hours = (uptime % (24 * 60 * 60)) / (60 * 60);
    auto uptime_mins = (uptime % (60 * 60)) / 60;
    auto uptime_secs = uptime % 60;
    _request_count++;
    server.send(200, "text/plain", String("Working! \n") + "Uptime: " + uptime_days + " " + uptime_hours + ":" + uptime_mins + ":" + uptime_secs + "\n" + "Request count: " + _request_count);
    digitalWrite(led, 0);
}

void handleNotFound()
{
    digitalWrite(led, 1);
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++)
    {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
    digitalWrite(led, 0);
}

void setup(void)
{
    pinMode(led, OUTPUT);
    digitalWrite(led, 1);
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("");

    // Wait for connection
    int i = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        i++;
        if(i > 60*2) {
            ESP.restart();
        }
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    if (MDNS.begin("esp32"))
    {
        Serial.println("MDNS responder started");
    }

    server.on("/", handleRoot);

    server.on("/inline", []()
              { server.send(200, "text/plain", "this works as well"); });

    server.onNotFound(handleNotFound);

    server.begin();
    Serial.println("HTTP server started");
    digitalWrite(led, 0);

    esp_task_wdt_init(WATCHDOG_TIME_SECONDS, true);
    esp_task_wdt_add(NULL);

    _init_millis = millis();
}

void loop(void)
{
    if(WiFi.status() != WL_CONNECTED)
    {
        delay(5000);
        ESP.restart();
    }

    server.handleClient();

    esp_task_wdt_reset();
    delay(50); //allow the cpu to switch to other tasks
}
