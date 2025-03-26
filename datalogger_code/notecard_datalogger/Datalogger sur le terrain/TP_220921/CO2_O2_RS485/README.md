# TinyPico Low Power Board
## Communication RS485 
Pour ce datalogger, les capteurs CO2 et O2 sont au bout d'un câble d'environ 100 mètres. Afin de pouvoir transmettre les données sur cette distance, des transreceivers UART - RS485 sont utilisés. Un multiplexeur UART permet de communiquer avec les deux capteurs en utilisant un seul bus UART.
Tout l'électronique du côté des capteurs CO2 et O2 est alimenté par une batterie 6V régulée à 5V. Il y a également une notecard qui envoie les données sur notehub/iotplotter.   

## BOM
Multiplexeur UART: "Serial Port Expander" https://atlas-scientific.com/ezo-accessories/81-serial-port-expander/  
Capteur O2 UART: https://atlas-scientific.com/probes/oxygen-sensor/  
Capteur CO2 UART: https://atlas-scientific.com/probes/co2-sensor/  
Transreceiver UART - RS485 avec logique 3.3V: https://www.conrad.ch/fr/p/joy-it-joy-it-module-convertisseur-noir-2481412.html  
Voltage regulator: https://www.sparkfun.com/products/15208  


## Montage
Le montage nécessaire pour reproduire ce datalogger est schématisé dans l'image ci-dessous.
![Montage](img/MontageV2.png)

## Remarque
Les deux capteurs de Atlas Scientific sont en UART avec un baudrate de 2400. Pour l'instant, 4 des 5 fils dans le câble sont utilisés.

