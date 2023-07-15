---
title: "Configure the IDE"
date: 2023-07-11T14:29:43-04:00
draft: false
weight: 2
section: true
---

## We need to configure the IDE

In order to communicate with the hardware, and to build the proper embedded software, the IDE needs to be configured.

## Update for the hardware

We will also need to install the proper board support package for the ESP32-CAM board. For this exercise we will be using [ESP32 Arduino 2.0.x](https://github.com/espressif/arduino-esp32/releases) which is the most recent release.

  To install this release, go to the Ardunio IDE and open the `Preferences` menu.

  ![Preferences](/otterize-workshop/hardware/images/preferences.png)

  In the `Additional Board Managers URL` field paste the following: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_dev_index.json` and click `OK`.

  Next, go to the `Boards Manager` menu and search for `ESP32`. You should see the option to install `esp32 by espressif systems` and the version should be `2.0.9`. Click `Install`.

The IDE should now be configured for the ESP32-CAM board.

## Add the libraries we need

Now it's time to install the required libraries.

Go to the Libraries Manager in the IDE:

![Library Manager](/otterize-workshop/intro-pre-requisites/images/library-manager.png)

That will open up the Library Manager:

![Library Panel](/otterize-workshop/intro-pre-requisites/images/library-panel.png)

Search for each of the libraries listed and click to install them.

- WiFiManager
- ArduinoJson
- base64 <- Capitalization is key here!
- ESP32 Mail Client

![library search](/otterize-workshop/intro-pre-requisites/images/library-search.png)

Just to make sure you have the latest versions, find any libraries that are updatable and apply the updates.

![updates](/otterize-workshop/intro-pre-requisites/images/update-libraries.gif)

## Congratulations!

Your Arduino IDE should now be able to build and upload code to the ESP32-CAM board.