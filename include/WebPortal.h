#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include "CC1101_Driver.h"
#include "SSTV_Demodulator.h"
#include "Doppler_Tracker.h"
#include "SD_Recorder.h"

typedef void (*CommandHandlerFunc)(char cmd);

class WebPortal {
public:
    WebPortal();

    void begin(CommandHandlerFunc cmdHandler);
    void update();

    bool isConnectedToStation() const { return stationConnected; }
    String getStationIP() const { return stationIP; }
    String getAPIP() const { return apIP; }

private:
    WebServer server;
    DNSServer dnsServer;
    Preferences prefs;
    CommandHandlerFunc onCommand;

    bool stationConnected;
    String stationIP;
    String apIP;
    uint32_t lastDnsUpdate;

    void setupRouting();
    void setupWiFi();
    void handleRoot();
    void handleStatus();
    void handleAction();
    void handleFiles();
    void handleDownload();
    void handleDelete();
    void handleScan();
    void handleConnect();
    void handleCaptiveRedirect();
};

extern WebPortal Portal;
