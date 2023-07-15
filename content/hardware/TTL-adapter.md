---
title: "TTL Adapter"
date: 2023-07-11T15:34:13-04:00
weight: 4
section: true
---

## Setting up the USB to TTL (UART) adapter

Get out the USB to TTL adapter and remove the jumper clip from it.

![Remove the jumper clip from the USB to TTL adapter](/otterize-workshop/hardware/images/hardware-workshop-12.png)

Connect one set of jumper wires to the `5v` and `GND` pins on the USB to TTL adapter and another set of jumper wires to the ``TXD` and `RXD` pins.

![Connect the USB to TTL adapter to the breadboard](/otterize-workshop/hardware/images/hardware-workshop-13.png)

Connect the jumper wires from the `5v` and `GND` pins on the ESP32-CAM board to the rows marked **(-)** on the side of the breadboard.

![Connect the ESP32-CAM board to the breadboard](/otterize-workshop/hardware/images/hardware-workshop-14.png)

Finally connect the jumper wires from the `TXD` and `RXD` pins on the USB to TTL adapter to the `RX` and `TX` pins on the ESP32-CAM board.

**Note:** Remember that the `TXD` pin on the USB-TTL adapter connects to the `RX` pin on the ESP32-CAM board and the `RXD` pin on the USB-TTL adapter connects to the `TX` pin on the ESP32-CAM board.

You can now add the colored button covers to your tactile buttons to complete the hardware.

![Connect the USB to TTL adapter to the ESP32-CAM board](/otterize-workshop/hardware/images/hardware-workshop-16.png)

Your IoT Hardware build is now complete! Congratulations, you're now a hardware developer!
