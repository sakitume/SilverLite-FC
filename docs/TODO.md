
## CLean up OSD system

The area where "LOW BATTERY" is displayed is also used for some new messages such as "FLASHED", and "CRASH FLIP".
Refactor this code so that it is a "notification" area. "LOW BATTERY" is lower priority then "FLASHED" and "CRASH FLIP".
I can envsion needing another message: "** THROTTLE **".

### Menu system for OSD
This would be for configuring:

* Smart Audio VTX control
    * Channel/Band
    * Power level
* Rates
* Expo
