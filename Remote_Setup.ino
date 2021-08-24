#include <WiFi101.h>  // For MKR1000, LGPL
//#include <WiFiNINA.h> // For MKR1010, LGPL
#include <WiFiClient.h>
#include <WiFiServer.h>
#include <WiFiSSLClient.h>
#include <WiFiUdp.h>
#include <FlashStorage.h> //LGPL-2.1

// Custom settings for wifi, sensor location, and Air Quality Limits

char apssid[] = "MKR1000AP"; // ssid for AP for setting WiFi credentials
int status = WL_IDLE_STATUS;
boolean readingNetwork = false;
boolean readingPassword = false;
boolean readingSensorLocation = false;
String password = "";
String network = "";
String sensorLocation = "";
boolean needCredentials = true;
boolean needWiFi = false; // will be set to true once credentials are found
boolean AP_running = false;
boolean credentials_changed = false;

// initial part of html code for list of possible networks to connect to
String htmlWifiList = "<select name=\"network\" id=\"network\"/>\n";

WiFiServer server(80);

// Set up storage of password/SSID to FlashStorage so that credentials will be saved after restart of device.
typedef struct
{
    boolean valid;
    char network[33]; // SSID length limits are always less than 32 chars
    char password[65]; // password length limits are always less than 64 chars
    char sensorLocation[100];
} Credentials;

// Set up flash storage
FlashStorage(stored_credentials, Credentials);
Credentials my_credentials;

void setup()
{
    Serial.begin(9600); // Serial is the RX/TX on the USB
    while (!Serial); // Wait for Serial to come online before printing

    // Fetch ssid, password, and sensorLocation from Storage if available from previous save.
    get_saved_credentials();  // If retrieved, needCredentials will be set to false and needWiFi to true.

    // Generate the list of available networks you can connect to in html.
    // This needs to be done regardless if credentials aren't needed in case the credentials are bad.
    int numSsid = WiFi.scanNetworks();
    // This step must be done here before the antenna enters AP mode or client mode.
    for (int thisNet = 0; thisNet < numSsid; thisNet++)
    {
        htmlWifiList += "<option value=\"";
        htmlWifiList += WiFi.SSID(thisNet);
        htmlWifiList += "\">";
        htmlWifiList += WiFi.SSID(thisNet);
        htmlWifiList += "    (";
        htmlWifiList += WiFi.RSSI(thisNet);
        htmlWifiList += " dBm)</option>\n";
    }
    htmlWifiList += "</select>\n";
    Serial.println(htmlWifiList);

    //  Include setup of the rest of your project here:
    //
    //
    //
    //
    //
    //
    //
}

void loop()
{
    if (needCredentials) // This enters in and out of this function each time taking further
    {
        if (not(AP_running))
        {
            Serial.println("Access Point Web Server");
            Serial.print("Creating access point named: ");
            Serial.println(apssid);

            if (WiFi.beginAP(apssid) != WL_AP_LISTENING)
            {
                Serial.println("Creating access point failed");
                while (true);
            }
                delay(1000);
                server.begin();
                printAPStatus();
                AP_running = true;
        }
        getCredentials(); // steps towards getting credentials, allowing continuous running (looping)
                          // of void loop() since this function is entered each time waiting for requests.
                          // This allows you to use your loop to do other responsibilities while user sets
                          // up WiFi.
    }
    if (needCredentials == false &&  WiFi.status() != WL_CONNECTED) // If WiFi is dropped and you don't need to fetch credentials
        needWiFi = true;
    if (needWiFi)
        getWiFi();

    //  Include the rest of your project here that needs to run in the function loop():
    //
    //
    //
    //
    //
    //
    //
}

void getCredentials()
{
    WiFiClient client = server.available();
    if (client)
    {
        Serial.println("new client");
        String currentLine = "";
        while(client.connected())
        {
            if(client.available())
            {
                char c = client.read();
                Serial.print(c);
                if (c=='?')
                    readingNetwork = true;
                if (readingNetwork)
                {
                    if (c=='!')
                    {
                        readingPassword = true;
                        readingNetwork = false;
                    }
                    else if (c!='?')
                        network += c;
                }
                if (readingPassword)
                {
                    if (c==',')
                    {
                        readingSensorLocation = true;
                        readingPassword = false;
                    }
                    else if (c!='!')
                        password += c;
                }
                if (readingSensorLocation)
                {
                    if (c=='*')
                    {
                        Serial.println();
                        Serial.print("Network Name: ");
                        Serial.println(network);
                        Serial.print("Password: ");
                        Serial.println(password);
                        Serial.print("Sensor Location: ");
                        Serial.println(sensorLocation);
                        Serial.println();
                        client.stop();
                        WiFi.end();
                        AP_running = false;
                        credentials_changed = true;
                        needCredentials = false;
                        needWiFi = true;
                    }
                    else if (c!=',')
                        sensorLocation += c;
                }
                if (c == '\n') // Generate HTTP request and webpage for WiFi Setup
                {
                    if (currentLine.length() == 0)
                    {
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println();
                        //client.println("<html>");
                        client.print("<html>\n<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"> ");
                        client.println("<head>");
                        client.println("<style type=\"text/css\"> body {font-family: sans-serif; margin:50px; padding:20px; line-height: 250% } </style>");
                        client.println("<title>Arduino Setup</title>");
                        client.println("</head>");
                        client.println("<body>");
                        client.println("<h2>WiFi Credentials</h2>");
                        client.print("<h4>Network/Password can't use: ? ! , *</h4>");
                        client.print("Network Name: ");
                        client.print(htmlWifiList);
                        client.print("Password: ");
                        client.print("<input id=\"password\"/><br>");
                        client.print("Choose a label for your sensor: ");
                        client.print("<input id=\"sensorLocation\"/><br>");
                        client.print("<button type=\"button\" onclick=\"SendText()\">CONNECT</button><br><br>");
                        client.print("<div id=\"Instruct\">If your sensor successfully connects to your wifi, then its screen will display the ip address you should connect to in order to monitor the air from your browser.</div>");
                        // Put a bunch of whitespace below so that page auto-scrolldown actually scrolls to avoid confusing user after inputting credentials.
                        client.print("<br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br><br>.");
                        client.println("</body>");
                        client.println("<script>");
                        client.println("var network = document.querySelector('#network');");
                        client.println("var password = document.querySelector('#password');");
                        client.println("var sensorLocation = document.querySelector('#sensorLocation');");
                        client.println("function SendText() {");
                        client.println("document.getElementById(\"Instruct\").scrollIntoView();");
                        client.println("nocache=\"&nocache=\" + Math.random() * 1000000;");
                        client.println("var request =new XMLHttpRequest();");
                        client.println("netText = \"&txt=\" + \"?\" + network.value + \"!\" + password.value + \",\" + sensorLocation.value + \"*&end=end\";");
                        client.println("request.open(\"GET\", \"ajax_inputs\" + netText + nocache, true);");
                        client.println("request.send(null)");
                        client.println("network.value=''");
                        client.println("password.value=''");
                        client.println("sensorLocation.value=''}");
                        client.println("</script>");
                        client.println("</html>");
                        client.println();
                        break;
                    }
                    else
                        currentLine = "";
                }
                else if (c != '\r')
                    currentLine += c;
            }
        }
        client.stop();
        Serial.println("client disconnected");
        Serial.println();
    }
}

void store_credentials()
{
    // Write ssid, password, and sensorLocation to storage
    my_credentials.valid = true;
    network.toCharArray(my_credentials.network, 33);
    password.toCharArray(my_credentials.password, 65);
    sensorLocation.toCharArray(my_credentials.sensorLocation, 100);

    stored_credentials.write(my_credentials);
    Serial.println("Just Stored Credentials");
    Serial.println("Password: ");
    Serial.println(my_credentials.password);
    Serial.println("Network: ");
    Serial.print(my_credentials.network);
    Serial.println("Location: ");
    Serial.print(my_credentials.sensorLocation);
    Serial.println("");
}

void get_saved_credentials()
{
    my_credentials = stored_credentials.read();
    if (my_credentials.valid) // if they exist in storage
    {
        network = my_credentials.network;
        password = my_credentials.password;
        sensorLocation = my_credentials.sensorLocation;
        needCredentials = false;
        needWiFi = true;
    Serial.println("Just Retrieved Credentials");
        Serial.println("Stored Network:");
        Serial.print(network);
        Serial.println("Stored Password:");
        Serial.print(password);
        Serial.println("Stored Location:");
        Serial.print(sensorLocation);
        Serial.println("");
    }
    else
    {
        needWiFi = false;
        needCredentials = true;
    }
}

// Make sure network and password globals are set before calling this function
void getWiFi() // Try to connect a number of times, if it fails keep flag needCredentials = true
{
    int numAttempts = 1;
    int attemptsLimit = 4;

    if (network == "" or password == "")
    {
        Serial.println("Invalid WiFi credentials");
        needWiFi = false; // needWiFi will be set to true once new credentials are set
        needCredentials = true;
        return;
    }
    while (status != WL_CONNECTED && numAttempts < attemptsLimit)
    {
        numAttempts++;
        Serial.print("Attempting to connect to SSID: ");
        Serial.println(network);

        const char* network_arr = network.c_str();
        const char* password_arr = password.c_str();
        network_arr = &network[0];
        password_arr = &password[0];

        Serial.println(network_arr);
        Serial.println(password_arr);

        status = WiFi.begin(network_arr, password_arr);
        delay(10000);
    }

    if(status != WL_CONNECTED)
    {
        Serial.println("WiFi connection failed");
        needCredentials = true;
        needWiFi = false; // needWiFi will be set to true once new credentials are created by the user again
    }
    else
    {
        Serial.println("WiFi connection successful");
        server.begin();
        printWiFiStatus();
        needWiFi = false;
        needCredentials = false;
        if(credentials_changed)
            store_credentials(); // Now that we know we have good credentials, save them
    }
   delay(1000);
}


void printAPStatus()
{
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);
    Serial.print("To connect, open a browser to http://");
    Serial.println(ip);
}

void printWiFiStatus()
{
    // print the SSID of the network you're attached to:
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
    // print where to go in a browser:
    Serial.print("To see this page in action, open a browser to http://");
    Serial.println(ip);

    delay(10000);
}
