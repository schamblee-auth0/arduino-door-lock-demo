# arduino-door-lock-demo

Currently, this project is pretty much a clone of https://github.com/auth0/drink-dispenser-demo. The goal is to use OAuth2 device flow to authenticate a user and unlock a door.

## Components
* **Development board:** Arduino Uno WiFi Rev2
* **NFC Shield:** X-Nucleo-NFC04A1
* **Solenoid:** [Lock-style Solenoid - 12VDC](https://www.adafruit.com/product/1512)
* [1 Channel Relay Module](https://www.amazon.com/dp/B00XT0OSUQ?psc=1&ref=ppx_yo2_dt_b_product_details)
* [5.5x2.1mm Female CCTV Wire Connector DC Power Jack Adapter Plug](https://www.amazon.com/dp/B07PKNYN22?psc=1&ref=ppx_yo2_dt_b_product_details)
* [12V DC 1000mA (1A) regulated switching power adapter](https://www.adafruit.com/product/798?gclid=Cj0KCQjwp86EBhD7ARIsAFkgakhEprG1xGb5vyMJLJ7Wp811uUZHpqRvuqRTKSJ-yoaT8Jj68JB39k8aAtSSEALw_wcB)

## TODO
* Resolve issue where wifi module is not detected when NFC shield is attached
* Replace drink dispenser functiolity with solenoid functionality
