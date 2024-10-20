# Instructions

First, hang the EBox on the reservoir and position the plant and reservoir where you want them and in a place where you can power the device with the USB cable.

![hang ebox](images/plant133-hang-ebox.jpg)

Then, for each plant to be watered:

1. Install the float in the reservoir insert.

  ![install float](images/install-float.jpg)

  Then wire the float to the ebox.

![wire float](images/plant133-wire-float.jpg)

2. Put a pump in the reservoir insert.

  ![install motor](images/install-motor.jpg)

3. Connect a soil moisture sensor and pump to the EBox. The connectors should be in a vertical line under 1, 2, 3, or 4 on the EBox. The soil moisture sensor uses a 3-conductor wire with JST connectors.

   ![wire sensor and pump](images/plant133-wire-sensor-pump.jpg)

4. Run a tube from the pump, through the hole in the insert, and then to the soil in the pot.

  ![install motor-tube](images/install-motor-tube.jpg)

5. Cut the tube to the right length, and either:
    - Push the end through the hole in the moisture sensor cap to position it for watering the soil, or

   ![tube aimed at soil](images/tube-aimed-at-soil.jpg)

    - Insert the end of the tube into a soil spike and then insert the soil spike into the soil.

	  ![spike with tube](images/spike-with-tube.jpg)
	  ![spike in soil](images/spike-in-soil.jpg)
6. Power the EBox with the USB micro cable.
7. Look for the name of the device, and connect your phone or computer to a WiFi network with that name. The name of the ESSID may be shown on the device screen, along with the AP password (if set).
8. Open a web browser to http://192.168.4.1, and accept the non-encrypted connection.
9. Use the web interface to configure the device to connect to your home's WiFi network.
    - Press "WiFi Config" on the main page, and update the configuration. Press "Save", then "Back", the press "Restart" to reboot the device.

	![web WiFi config](images/web-wifi-config.png)

10. Look for the IP address of the device on its screen to open its web interface again.  If mDNS works in your home, you might be able to connect to it as "http://{device-name}.local" where "{device-name}" is replaced with the name of your device.
11. If you have an MQTT broker in your house, use the web interface to configure the connection to the broker.  If you are running Home Assistant and it works with your MQTT broker, the device will automatically show up in Home Assistant.

    ![web MQTT config](images/web-mqtt-config.png)

12. In the device web interface, select the plant to be configured, then press "Configure".

  - Select the minimum and maximum soil moisture levels you want, and select the number of seconds you want the pump to run at each iteration of watering.  Try 1 second for a small pot and 3 seconds for a medium-sized pot.
  - You can also calibrate the moisture sensor. Put the moisture sensor in a glass of water so that all except the cap is covered by water, and enter the ADC reading as the "max ADC reading". Remove the sensor out of the glass and dry it off, then enter the ADC reading as the "min ADC reading." Confusingly, the counts for the max reading should be less than the counts for the min reading, because max and min here refer to the moisture level not the counts.
  - You can set the time to wait between pump doses, which allows water to to spread through the soil and to the moisture sensor. The default is 15 minutes (900 seconds).
  - There is a safety feature to prevent over-watering if there is a problem with moisture sensing. After a maximum number of pump doses in a watering cycle, watering will pause for 12 hours before watering can continue. The default maximum number of doses is 5, but this can be changed.

    ![web watering config](images/web-water-config.png)

13. Enable watering, and enable reservoir water detection. Press "Save" and then "Back".
14. Fill the reservoir with water.
15. Monitor the plant-watering and tweak settings so it works the way you want it to.
16. You get bonus points for setting up Grafana and creating a dashboard to show how well the watering is working.
17. You can even more bonus points for using Home Assistant to send you a phone alert you when the reservoir is out of water.




