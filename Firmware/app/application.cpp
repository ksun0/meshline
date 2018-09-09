#include <tytherm.h>
#include <user_config.h>
#include <SmingCore/SmingCore.h>

HashMap<String, String> db;
HashMap<String, int> conn_db;
DNSServer dnsServer;

Timer counterTimer;
Timer wifiScanDelayTimer;
Timer DownloadTimer;
Timer ConnectTimer;
HttpClient httpClient;

void counter_loop();
void try_download();
void try_scan();
unsigned long counter = 0;

void listNetworks(bool succeeded, BssList list);

IPAddress apIP(192, 168, random(5, 255), 1);

const byte DNS_PORT = 53;

void counter_loop()
{
	counter++;
}

void start_scan() {
    WifiStation.startScan(listNetworks);
}

void try_scan() {
    if (!wifiScanDelayTimer.isStarted()) {
        wifiScanDelayTimer.initializeMs(20000, start_scan);
        wifiScanDelayTimer.start();
   }
}

void try_connect() {
    if (conn_db.count() > 0) {
        int min_val = 10000;
        int min_idx = 10000;
        for (unsigned int i = 0; i < conn_db.count(); i++) {
            int val = counter - conn_db.valueAt(i);
            if (val < min_val) {
                min_val = val;
                min_idx = i;
            }
        }
        conn_db[conn_db.keyAt(min_idx)] = counter;
        WifiStation.disconnect();
        WifiStation.config(conn_db.keyAt(min_idx), String(""), false, false); // Put you SSID and Password here
        WifiStation.connect();
    } else {
        try_scan();
    };
}

// Will be called when WiFi station network scan was completed
void listNetworks(bool succeeded, BssList list)
{
	if(!succeeded) {
		Serial.println("Failed to scan networks");
		start_scan();
		return;
	}

	Serial.print("\tWiFi: ");
    HashMap<String, int> seen_db;
    for(unsigned int i = 0; i < list.count(); i++) {
        Serial.print("\tWiFi: ");
        Serial.print(list[i].ssid);

        if (list[i].ssid.startsWith(String("MeshLine")) && list[i].isOpen()) {
            Serial.print("\tfound2: ");
            seen_db[list[i].ssid] = 0;
            Serial.print(list[i].ssid);
            if (!conn_db.contains(list[i].ssid)) {
                conn_db[list[i].ssid] = counter-20;
            }
		}
	}

    Serial.print("\tWiFi list: ");

    for(unsigned int i = 0; i < conn_db.count();) {
        if (!seen_db.contains(conn_db.keyAt(i))) {
            conn_db.remove(conn_db.keyAt(i));
        } else {
            Serial.print(conn_db.valueAt(i));
            Serial.print(conn_db.keyAt(i));
            i++;
        }
    }

    try_connect();
}

int onDownload(HttpConnection& connection, bool success) {
    DownloadTimer.stop();
    Serial.println("got download");
    if (success) {
        debugf("\n=========[ URL: %s ]============", connection.getRequest()->uri.toString().c_str());
        debugf("RemoteIP: %s", (char*)connection.getRemoteIp());
        debugf("Got response code: %d", connection.getResponseCode());

        debugf("Got content starting with: %s", connection.getResponseString().c_str());

        DynamicJsonBuffer buffer;
        JsonObject& root = buffer.parseObject(connection.getResponseString().c_str());

        root.prettyPrintTo(Serial);

        for (JsonObject::iterator it=root.begin(); it!=root.end(); ++it) {
            Serial.println("adding");
            Serial.println(it->key);
            Serial.println(it->value.asString());
            db[String(it->key)] = String(it->value.asString());
        }
    }

    try_scan();

    return 0;
}


void try_download() {
    Serial.print("got connection");
    IPAddress their_ip = WifiStation.getIP();
    their_ip[3] = 1;

    String req_str = String(String("http://") + their_ip.toString() + String("/sync"));
    Serial.print(req_str);

    httpClient.downloadString(req_str.c_str(), onDownload);

    DownloadTimer.stop();
    DownloadTimer.initializeMs(10000, try_scan);
    DownloadTimer.startOnce();

    //httpClient.send(request);
}

void OnConnect(String, uint8_t, uint8_t[6], uint8_t) {
    if (!WifiStation.getIP() == IPAddress(0, 0, 0, 1)) {
        try_download();
    } else {
        try_scan();
    }
}

// Will be called when WiFi station was connected to AP
void connectOk(IPAddress ip, IPAddress mask, IPAddress gateway)
{
    if (!(WifiStation.getIP() == IPAddress(0, 0, 0, 1))) {
        try_download();
    } else {
        try_scan();
    }
}

// Will be called when WiFi station was disconnected
void connectFail(String ssid, uint8_t ssidLength, uint8_t* bssid, uint8_t reason)
{
    try_scan();
    debugf("Disconnected from %s. Reason: %d", ssid.c_str(), reason);
}

// Will be called when WiFi hardware and software initialization was finished
// And system initialization was completed
void ready()
{
    WifiEvents.onStationGotIP(connectOk);
    WifiEvents.onStationConnect(OnConnect);
    WifiEvents.onStationDisconnect(connectFail);

    startWebServer();


    // Print available access points
    WifiStation.startScan(listNetworks); // In Sming we can start network scan from init method without additional code

    debugf("READY!");

	// If AP is enabled:
	debugf("AP. ip: %s mac: %s", WifiAccessPoint.getIP().toString().c_str(), WifiAccessPoint.getMAC().c_str());
}

void init()
{
    String mac = WifiAccessPoint.getMAC();

    int mac_int = 0;

    for (char i : mac) {
        mac_int += i;
    }
    randomSeed(mac_int);

    apIP[2] = random(5, 255);

    spiffs_mount();					// Mount file system, in order to work with files




    system_update_cpu_freq(SYS_CPU_160MHZ);
    wifi_set_sleep_type(NONE_SLEEP_T);

	Serial.begin(SERIAL_BAUD_RATE);
	Serial.systemDebugOutput(true); // Allow debug print to serial
	Serial.println("Sming. Let's do smart things!");

	// Set system ready callback method
	System.onReady(ready);

    startWebServer();

    // Soft access point
	WifiAccessPoint.enable(true);
	WifiAccessPoint.config((String("MeshLine ") + mac).c_str(), "", AUTH_OPEN);
	dnsServer.start(DNS_PORT, "*", apIP);

    // Station - WiFi client
	WifiStation.enable(true);
	// WifiStation.config(WIFI_SSID, WIFI_PWD); // Put you SSID and Password here
    startWebServer();

	WifiAccessPoint.setIP(apIP);

    counterTimer.initializeMs(1000, counter_loop).start();
    startWebServer();
}
