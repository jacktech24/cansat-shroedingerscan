
#include <Arduino.h>

#include "base.h"

//konstruktor tridy, inicializace
Base::Base() : webSocket(81), radio(SS, D2, true, D2), payloadJson(100) {
}

void Base::setup() {
    ready = false;

    //spousti seriovou komunikaci, baudrate 115200
    Serial.begin(115200);
    while (!Serial) { }
    Serial.println();
    Serial.println("[BASE] Serial komunikace bezi");

    //spousti WiFi
    WiFi.mode(WIFI_AP);
    //nastavi nazev site a pristupove heslo
    WiFi.softAP("ShroedingersCan", "mf123456");
    Serial.println("WiFi sit 'ShroedingersCan' bezi");

    //spousti Server pro pripojeni klientu (prijem dat)
    webSocket.begin();

    //nastavuje 433 Mhz radio
    Serial.println("Nastavuji RFM69 radio");
    bool response = radio.initialize(FREQUENCY, BASE_ID, NETWORK_ID);
    if (response) {
        radio.setHighPower();
        radio.promiscuous(true);
        radio.encrypt(ENCRYPTKEY);
        Serial.println("RFM69 nastaveno a pripojeno, sifrovani aktivni");
    } else {
        Serial.println("Komunikace s RFM69 selhala, zkontrolujte zapojeni...");
        return;
    }

    //vsechno funguje, zarizeni je pripraveno
    ready = true;
}

void Base::loop() {

    //obslouzi pripojene klienty a vyridi komunikaci
    webSocket.loop();

    //pokud vsechno funguje
    if (ready) {
        //pokud nam dorazila celá zpráva přes 433Mhz
        if (radio.receiveDone()) {

            //pripravime si zpravu pro klienty
            payloadJson = "{\"sender\":";
            payloadJson += (int) radio.SENDERID;
            payloadJson += ",\"rssi\":";
            payloadJson += radio.RSSI;
            payloadJson += ",\"length\":";
            payloadJson += radio.DATALEN;
            payloadJson += ",\"data\":";
            for (byte i = 0; i < radio.DATALEN; i++) {
                payloadJson += (char) radio.DATA[i];
            }
            payloadJson += "}";

            //odesleme zpravu klientum
            webSocket.broadcastTXT(payloadJson);

            //pokud si vysilac vyzadal potvrzeni doruceni, odesleme ho
            if (radio.ACK_REQUESTED) {
                radio.sendACK();
            }
        }
    } else {
        //zarizeni nefunguje, nic nedelame
        Serial.println("Zarizeni neni ve funkcnim stavu, je vyzadovana akce");
        delay(1000);
    }
}