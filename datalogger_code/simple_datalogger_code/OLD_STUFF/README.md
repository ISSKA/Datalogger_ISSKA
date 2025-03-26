# TinyPico Low Power Board
## Explications 
Le dossier TinyPico_datalogger contient le code principal du logger. 
Pour le faire fonctionner il faut ajouter, dans le même dossier que le script principal, les deux fichiers "sensors" qui sont dans les autres dossiers
Ces fichiers permettent de lire les différents capteurs en titre du dossier. 

Il faut donc faire un copier-coller des ces fichiers sensors suivant les capteurs qui se trouvent sur le datalogger. 

!!!ATTENTION!!! Le temps d'execution du code varie suivant les capteurs utilisés. Des corrections de temps peuvent être faites aux lignes 165 et 168 du code principal. Sinon, il est probable que le pas de temps de corresponde pas exactement au pas de temps choisi sur la carte SD.

## Montage
All manufacturing documents are available in the "manufacturing" folder.

### Passive components
TinyPico:
- Mosfet TN0702 : DRAIN on GND, GATE in pin 14 and source on GND rail

### Sensors connected to MUX
All sensors connected to MUX should have pull-ups enabled.

> By default, pull-ups on MS5803-01 are disabled !

## Configuration

### Sensors configuration
Rename file `Sensors.example.cpp` from directory `low_power_board` in `Sensors.cpp` and configure your sensors.

### Pas de temps entre les mesures
Pour définir le pas de temps entre les mesures, ajouter un fichier nommé `conf.txt` à la racine de la carte microSD. Son contenu doit être sous la forme `<pas de temps en secondes>;`, soit `15;` pour un pas de 15 secondes.

## Consommation
Consommation actuelle :
- Environ 150 uA en veille
- Environ 50 mA durant la phase de mesure

Calculateur de durée de fonctionnement : http://www.of-things.de/battery-life-calculator.php.

En prenant en compte que les mesures durent 5 s et qu'on a des pas de mesures de 15 min, la batterie tient environ 460 jours !


## Optimisations possibles
A voir


## Améliorations futures ?
A voir
