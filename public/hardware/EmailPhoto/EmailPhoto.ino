
#include <WiFiManager.h>
#include <base64.h>
#include "esp_camera.h"
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
#include "driver/rtc_io.h"
#include <EEPROM.h>  // read and write from flash memory
#include <ESP_Mail_Client.h>
#include "FS.h"
#include <LittleFS.h>
#include <WiFiClientSecure.h>
#include <ssl_client.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>  //https://github.com/bblanchon/ArduinoJson
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"


#define DEBUG

#ifndef LED_BUILTIN
#define LED_BUILTIN 33  // ESP32 DOES NOT DEFINE LED_BUILTIN
#endif

#define LED 33

#ifdef DEBUG
#define debugprint(x) Serial.print(x)
#define debugprintln(x) Serial.println(x)
#define debugprintF(x) Serial.print(F(x))
#else
#define debugprint(x)
#define debugprintF(x)
#endif

/** The smtp host name e.g. smtp.gmail.com for GMail or smtp.office365.com for Outlook or smtp.mail.yahoo.com */
#define SMTP_HOST "smtp.gmail.com"

/** The smtp port e.g.
 * 25  or esp_mail_smtp_port_25
 * 465 or esp_mail_smtp_port_465
 * 587 or esp_mail_smtp_port_587
 */
#define SMTP_PORT esp_mail_smtp_port_587

#define SERIAL_TX 1
#define SERIAL_RX 3
#define FLASH_BULB 4
#define RESET_BTN 12
#define SHUTTER 13
#define FORMAT_LITTLEFS_IF_FAILED true

struct Settings {
  char sender_name[50] = "Your Name";
  char sender_email[50] = "you@bar.com";
  char app_passwd[17] = "App Password";
  char receiver_email[50] = "them@foo.com";
} sett;

#define EEPROM_SIZE sizeof(Settings)

/* Declare the global SMTPSession object for SMTP transport */
SMTPSession smtp;

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);

// gets called when WiFiManager enters configuration mode
void configModeCallback(WiFiManager *myWiFiManager) {
  debugprintln("Entered config mode");
  debugprintln(WiFi.softAPIP());
  // if you used auto generated SSID, print it
  debugprintln(myWiFiManager->getConfigPortalSSID());
}

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

byte mac[6];

void startCameraServer();

// Not sure if WiFiClientSecure checks the validity date of the certificate.
// Setting clock just to be sure...
void setClock() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  debugprintF("Waiting for NTP time sync: ");
  time_t nowSecs = time(nullptr);
  while (nowSecs < 8 * 3600 * 2) {
    delay(500);
    debugprintF(".");
    yield();
    nowSecs = time(nullptr);
  }
  debugprintln();
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  debugprintF("Current time: ");
  debugprint(asctime(&timeinfo));
}

void setup() {
#ifdef debug
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
#endif

  // This is all camera config stuff you can ignore.
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  Serial.println("Brownout fuse set.");
  pinMode(LED, OUTPUT);
  pinMode(FLASH_BULB, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RESET_BTN, INPUT);
  pinMode(SHUTTER, INPUT);
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  // init with high specs to pre-allocate larger buffers
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  } else {
    Serial.println("Stuck with SVGA");
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
  }
  sensor_t *s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // flip it back
    s->set_brightness(s, 1);   // up the blightness just a bit
    s->set_saturation(s, -2);  // lower the saturation
  }
  s->set_framesize(s, FRAMESIZE_UXGA);

#if defined(CAMERA_MODEL_M5STACK_WIDE)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

  // Done with camera stuff to ignore

  WiFi.mode(WIFI_STA);  // explicitly set mode, esp defaults to STA+AP
  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;
  if (LittleFS.begin()) {
    Serial.println("mounted file system");
  } else {
    Serial.println("File system failed to mount");
  }
  // in case you need to reset everything.
  if (digitalRead(RESET_BTN) == HIGH) {
    Serial.println("Reset Button pressed!");
    wm.resetSettings();
    LittleFS.remove("/config.json");
    ESP.restart();
  }

  if (LittleFS.exists("/config.json")) {
    //file exists, reading and loading
    Serial.println("reading config file");
    File configFile = LittleFS.open("/config.json", "r");
    if (configFile) {
      Serial.println("opened config file");
      size_t size = configFile.size();
      // Allocate a buffer to store contents of the file.
      std::unique_ptr<char[]> buf(new char[size]);

      configFile.readBytes(buf.get(), size);

#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
      DynamicJsonDocument json(1024);
      auto deserializeError = deserializeJson(json, buf.get());
      serializeJson(json, Serial);
      if (!deserializeError) {
#else
      DynamicJsonBuffer jsonBuffer;
      JsonObject &json = jsonBuffer.parseObject(buf.get());
      json.printTo(Serial);
      if (json.success()) {
#endif
        Serial.println("\nparsed json");
        strcpy(sett.app_passwd, json["app_passwd"]);
        strcpy(sett.receiver_email, json["receiver_email"]);
        strcpy(sett.sender_email, json["sender_email"]);
        strcpy(sett.sender_name, json["sender_name"]);
      } else {
        Serial.println("failed to load json config");
      }
      configFile.close();
    }
  }
  Serial.println("Settings loaded");
  Serial.printf("Sender email: %s\n", sett.sender_email);
  Serial.printf("Sender Name: %s\n", sett.sender_name);
  Serial.printf("App Password: %s\n", sett.app_passwd);
  Serial.printf("Receiver Email: %s\n", sett.receiver_email);
  WiFiManagerParameter sender_name("sender_name", "Your Name", sett.sender_name, 50, " ");
  wm.addParameter(&sender_name);
  WiFiManagerParameter sender_email("sender_email", "Your Email", sett.sender_email, 50, " ");
  wm.addParameter(&sender_email);
  WiFiManagerParameter app_passwd("app_passwd", "Gmail App Password", sett.app_passwd, 17, " ");
  wm.addParameter(&app_passwd);
  WiFiManagerParameter receiver_email("receiver_email", "Email Recipient", sett.receiver_email, 50, " ");
  wm.addParameter(&receiver_email);
  // set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wm.setAPCallback(configModeCallback);
  //set config save notify callback
  wm.setSaveConfigCallback(saveConfigCallback);

  WiFi.macAddress(mac);
  Serial.print("MAC: ");
  Serial.print(mac[5], HEX);
  Serial.print(":");
  Serial.print(mac[4], HEX);
  Serial.print(":");
  Serial.print(mac[3], HEX);
  Serial.print(":");
  Serial.print(mac[2], HEX);
  Serial.print(":");
  Serial.print(mac[1], HEX);
  Serial.print(":");
  Serial.println(mac[0], HEX);
  // fetches ssid and pass and tries to connect
  // if it does not connect it starts an access point with the specified name
  // here  "AutoConnectAP"
  // and goes into a blocking loop awaiting configuration
  if (!wm.autoConnect()) {
    debugprintln("failed to connect and hit timeout");
    // reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(1000);
  }
  // if you get here you have connected to the WiFi
  debugprintln("Connected...Yay! :)");
  setClock();
  sett.sender_name[49] = '\0';
  strncpy(sett.sender_name, sender_name.getValue(), 50);
  sett.sender_email[49] = '\0';
  strncpy(sett.sender_email, sender_email.getValue(), 50);
  sett.app_passwd[17] = '\0';
  strncpy(sett.app_passwd, app_passwd.getValue(), 17);
  sett.receiver_email[49] = '\0';
  strncpy(sett.receiver_email, receiver_email.getValue(), 50);

  debugprint("App Password: \t");
  debugprintln(sett.app_passwd);
  debugprint("Sender Email: \t");
  debugprintln(sett.sender_email);
  debugprint("Sender name: \t");
  debugprintln(sett.sender_name);
  debugprint("Receiver Email: \t");
  debugprintln(sett.receiver_email);
  debugprintln();
  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
    DynamicJsonDocument json(1024);
#else
    DynamicJsonBuffer jsonBuffer;
    JsonObject &json = jsonBuffer.createObject();
#endif
    json["app_passwd"] = sett.app_passwd;
    json["sender_email"] = sett.sender_email;
    json["sender_name"] = sett.sender_name;
    json["receiver_email"] = sett.receiver_email;

    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
    serializeJson(json, Serial);
    serializeJson(json, configFile);
#else
    json.printTo(Serial);
    json.printTo(configFile);
#endif
    configFile.close();
    //end save
  }

  startCameraServer();
  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
  /*  Set the network reconnection option */
  MailClient.networkReconnect(true);
  /* Set the callback function to get the sending results */
  smtp.callback(smtpCallback);
}

// Read the contents of a file
void readFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return;
  }
  Serial.println("- read from file:");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

// Write a string to a file
void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  Serial.printf("Wrote %d bytes\n", file.size());
  file.close();
}

// delete a file
void deleteFile(fs::FS &fs, const char *path) {
  Serial.printf("Deleting file: %s\r\n", path);
  if (fs.remove(path)) {
    Serial.println("- file deleted");
  } else {
    Serial.println("- delete failed");
  }
}

// Here's where we sent the email
void sendEmail() {
  Session_Config sessConfig;
  /* Set the session config */
  sessConfig.server.host_name = SMTP_HOST;
  sessConfig.server.port = SMTP_PORT;
  sessConfig.login.email = sett.sender_email;
  sessConfig.login.password = sett.app_passwd;
  sessConfig.login.user_domain = F("otterize.com");
  sessConfig.time.ntp_server = F("pool.ntp.org,time.nist.gov");
  sessConfig.time.gmt_offset = 3;
  sessConfig.time.day_light_offset = 0;
  /* Declare the message class */
  SMTP_Message message;

  /* Enable the chunked data transfer with pipelining for large message if server supported */
  message.enable.chunking = true;

  /* Set the message headers */
  message.sender.name = F(sett.sender_name);
  message.sender.email = sett.sender_email;

  message.subject = F("A New Picture!");
  message.addRecipient(F(sett.sender_name), sett.receiver_email);

  message.html.content = F("<span style=\"color:#ff0000;\">The camera image.</span><br/><br/><img src=\"cid:image-001\" alt=\"esp32 cam image\"  width=\"1600\" height=\"1200\">");
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_base64;
  message.html.charSet = F("utf-8");
  SMTP_Attachment att[1];

  /** Set the inline image info e.g.
     * file name, MIME type, file path, file storage type,
     * transfer encoding and content encoding
     */
  att[0].descr.filename = F("image.jpg");
  att[0].descr.mime = F("image/jpg");
  att[0].file.path = F("/image.jpg");
  att[0].file.storage_type = esp_mail_file_storage_type_flash;

  /* Need to be base64 transfer encoding for inline image */
  att[0].descr.transfer_encoding = Content_Transfer_Encoding::enc_base64;

  /** The file is already base64 encoded file.
   * Then set the content encoding to match the transfer encoding
   * which no encoding was taken place prior to sending.
   */
  att[0].descr.content_encoding = Content_Transfer_Encoding::enc_base64;

  /* Add inline image to the message */
  message.addInlineImage(att[0]);
  message.addAttachment(att[0]);
  /* Connect to the server */
  if (!smtp.connect(&sessConfig)) {
    ESP_MAIL_PRINTF("Connection error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    return;
  }

  if (smtp.isAuthenticated())
    Serial.println("\nSuccessfully logged in.");
  else
    Serial.println("\nConnected with no Auth.");

  /* Start sending the Email and close the session */
  if (!MailClient.sendMail(&smtp, &message, true))
    ESP_MAIL_PRINTF("Error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());

  // to clear sending result log
  // smtp.sendingResult.clear();

  ESP_MAIL_PRINTF("Free Heap: %d\n", MailClient.getFreeHeap());
}
void loop() {
  if (digitalRead(RESET_BTN) == HIGH) {
    debugprintln("Resetting!!");
    // reset and try again
    ESP.restart();
    delay(1000);
  }
  if (digitalRead(SHUTTER) == HIGH) {
    camera_fb_t *fb = NULL;
    digitalWrite(FLASH_BULB, HIGH);
    debugprintln("Shutter Pressed\n");
    delay(500);
    // this actually takes the picture
    fb = esp_camera_fb_get();
    delay(500);
    digitalWrite(FLASH_BULB, LOW);
    Serial.println("flash off");
    if (!fb) {
      Serial.println("Camera capture failed");
      return;
    }
    debugprintln("picture taken");
    String encodedString = base64::encode(fb->buf, fb->len);
    Serial.printf("Base64 encoded size: %d\n", strlen(encodedString.c_str()));
    writeFile(LittleFS, "/image.jpg", encodedString.c_str());
    sendEmail();
    deleteFile(LittleFS, "/image.jpg");
    Serial.printf("Image /image.jpg deleted\n",)
    esp_camera_fb_return(fb);
    debugprintln("Done!");
  }
}

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status) {
  /* Print the current status */
  Serial.println(status.info());
  /* Print the sending result */
  if (status.success()) {
    // ESP_MAIL_PRINTF used in the examples is for format printing via debug Serial port
    // that works for all supported Arduino platform SDKs e.g. AVR, SAMD, ESP32 and ESP8266.
    // In ESP8266 and ESP32, you can use Serial.printf directly.

    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failed: %d\n", status.failedCount());
    Serial.println("----------------\n");

    for (size_t i = 0; i < smtp.sendingResult.size(); i++) {
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);

      // In case, ESP32, ESP8266 and SAMD device, the timestamp get from result.timestamp should be valid if
      // your device time was synched with NTP server.
      // Other devices may show invalid timestamp as the device time was not set i.e. it will show Jan 1, 1970.
      // You can call smtp.setSystemTime(xxx) to set device time manually. Where xxx is timestamp (seconds since Jan 1, 1970)

      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %s\n", MailClient.Time.getDateTimeString(result.timestamp, "%B %d, %Y %H:%M:%S").c_str());
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");

    // You need to clear sending result as the memory usage will grow up.
    smtp.sendingResult.clear();
  }
}
