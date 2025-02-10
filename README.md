# IoT-ETA-Gateway

![](https://github.com/ene9ba/IoT-ETA-Gateway/blob/main/GitHubDoc/Geh%C3%A4use%20ETA%20v13-1.png)
Das Gehäuse wird über einen im Gehäuse integrierten Magneten einfach am Heizkessel befestigt

## **IoT Gateway für die serielle Schnittstelle am Stückholz Heizkessel ETA SH30.**
Der RS232-Konverter auf TTL wurde im Internet als IoT-Komponente zugekauft.
Das Gateway benötigt noch eine 5 V Versorgung, die auch über den Seriellen 
Stecker D-Sub angebunden ist.
Der IoT-Kern ist ein ESP8266 WEMOS D1

![](https://github.com/ene9ba/IoT-ETA-Gateway/blob/main/GitHubDoc/Geh%C3%A4use%20ETA%20v13.png)


## Funktion: 
Das Gateway startet die Kommunikation mit dem Ofen über ein Startprotokoll,
wartet dann auf die zyklisch gemeldeten Daten, die dann checksummengeprüft 
über MQTT an openHAB übertragen werden. Die Visualisierung erfolgt mit Grafana.
          

Repoitory enthält Schaltplan, PCB, Software, Gehäusedsign 

![](https://github.com/ene9ba/IoT-ETA-Gateway/blob/main/GitHubDoc/openHAB.png)
Seitenansicht openHB Sitemap







![](https://github.com/ene9ba/IoT-ETA-Gateway/blob/main/GitHubDoc/Grafana.png)
Auswertung Beispiel mit Grafana

 
