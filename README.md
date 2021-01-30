Cat food access control..
ESP-WROOM-32

After start - connect to wifi feed with password feedfeed , open browser http://192.168.4.1
and set value to run.

![caption](https://i.giphy.com/media/KNpCnF9C8g07kprRbc/source.gif )

```
As iTAG i use hm-10 with hm-10 firmware .
command for setup ble iTAG over uart:```

```using hc-10 iTag:

1.AT+RENEW Restores factory defaults
2.AT+RESET Reboot HM-10   
3.AT Wait for OK
4.AT+MARJ0x1234  Set iBeacon Major number to 0x1234 (hexadecimal)
5.AT+MINO0xFA01 Set iBeacon Minor number to 0xFA01 (hexadecimal)
6.AT+ADVI5 Set advertising interval to 5 (546.25 milliseconds) 
7.AT+NAMECAT Set HM-10 module name to DOPEY. Make this unique.
8.AT+ADTY3 Make non-connectable (save power)
9.AT+IBEA1 Enable iBeacon mode
10.AT+DELO2 iBeacon broadcast-only (save power)
11.AT+PWRM0 Enable auto-sleep. This reduces power from 8 to 0.18 mA
11.2.AT+POWE3
12.AT+RESET RebootNote
```
feed-ex pet feeder..

Based on https://www.instructables.com/ESP32-BLE-Presence-Detector/ Project
