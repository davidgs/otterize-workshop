---
title: "Wifi Setup"
date: 2023-07-14T17:01:08-04:00
draft: false
section: true
---

## Connecting to WiFi

Now that youve got the hardware up, look in the Serial Monitor output for the following line:
```
*wm:AP IP address: 192.168.4.1
Entered config mode
192.168.4.1
ESP32_7BA8CC84
*wm:Starting Web Portal
```

That's the current network configuration. The key here is the `ESP_7BA8CC84` bit.
Open your phone and look for that in the "Available Networks" and connect to it.

Once connected, you should be greeted with the Welcome page:

![Welcome page](/otterize-workshop/hardware/images/wifi-manager.png)

Click on `Configure WiFi` and you will see the configuration page:

![Configure WiFi page](/otterize-workshop/hardware/images/config.png)

  - `SSID`: The name of the WiFi network you are connecting to.
  - `Password`: The password for the WiFi network you are connecting to.
  - `Your Name`: This should be your name, or what you want to appear in the "From" header.
  - `Your Email`: Your email address.
  - `GMail App Password`: The app-specific password from GMail.
  - `Email Recipien`: The email address where you want your pictures sent.

We will be using this configuration page a few times, so having your GMail App password information handy is a good idea. You can always get back to this configuration page by holding down the first button on your board and waiting for the board to reset and give you the SSID again in the Serial Monitor.

Your ESP32-CAM should now be connected to the WiFi Network and be ready to submit pictures.

