# DIY / Hacking the Bosch Easy Series alarm system
## How it began
Several years ago - after a break-in - I got myself a Bosch Easy Series alarm system, which promised an easy, extendable alarm system without cable. The sensors where indeed compact and non obstructive but the installation was anything but easy, and did indeed involve cables, especially between the control panel, the radio hub and the central unit. Regardless with its RFID tokens and voice feedback easy enough to use for the family.

![image](https://github.com/user-attachments/assets/bb482f8f-9ae0-40af-8f41-db5cfea1d02e)

While you could have extended the system with eg. a Bosch ITS-DX4020-G GPRS/GSM to send you SMS notifications, those additional moduls had been pricy and like the main system, are by now discontinued, making me look for alternatives how to interface with the system. After some years of toying with the system I think it is finally time to document my journey here.

## First iteration, sending status via SMS
Gladly the system is relatively well documented and provides additional internal ports. So my first attempt was to use an Arduino board with a cheap SIM800 GSM shield to replicate the SMS notification system. The power was taken from the internal 12V rail and 5V bug converted to power the arduino and shield. All items together cost a couple of euros on aliexpress.

## Second iteration, integrating to the HA
Over time the electronic equipment in the home grew, including Philips Hue bulbs or Tuya thermostats, nearly all of them following the Zigbee standard, while the Easy Series operates on a proprietary 868 MHz connection. Trying to consolidate the different hubs and providing a single point of entry I decided to get myself a Raspbery Pi. The idea was to not only get status updates via SMS but to be able to program, read or change the alarm states via the home assitant.

## Third stage, recreating the panel from scratch
While I love and want to keep the design of the panel, the technology behind is fairly outdated. Even the used security tokens are not NFC compatible but use 125 Khz RFID with a working distance of less then 7 cm. So sourcing new compatible tokens or handling faulty equipment will soonare or later become a real issue, so it is time to recreate the system. My choice fell on using the ESP32-C6 modul as microprocessor running the alarm panel, which allows me to integrate it into my ZigBee network. Additionally I can use LVGL to draw the GUI on a regular LCD screen. 

<sup>Images of the alarm panel are courtesy of © Bosch Sicherheitssysteme GmbH if not otherwise stated.</sup>
