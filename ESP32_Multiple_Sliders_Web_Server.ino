#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
#include <Servo.h>

// Informations de connexion Wi-Fi
const char* ssid = ""; //PoleDeVinci_IFT
const char* password = "";  //*c.r4UV@VfPn_0

// Initialisation du serveur WebSocket et du servomoteur
WebSocketsServer webSocket = WebSocketsServer(80);  // Crée un serveur WebSocket qui écoute sur le port 80
Servo servo;                                        // Crée un objet servomoteur pour contrôler un servomoteur
const int servoPin = D4;                            // Définit la broche à laquelle le servomoteur est connecté
const int potentiometerPin = A0;                    // Définit la broche analogique à laquelle le potentiomètre est connecté (je ne pense pas l'utiliser)
const int ledPin = D5;                              // Broche à laquelle la LED est connectée
int currentAngle = 90;                              // Angle initial du servomoteur

void setup() {
  Serial.begin(115200);        // Initialise la communication série à un débit de 115200 bauds pour le débogage
  WiFi.begin(ssid, password);  // Connecte l'ESP8266 au réseau Wi-Fi spécifié
  // Boucle jusqu'à ce que l'ESP8266 soit connecté au Wi-Fi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);         // Attend 500ms entre chaque tentative de connexion
    Serial.print(".");  // Affiche un point pour indiquer la tentative de connexion
  }
  Serial.println("\nConnected to WiFi");  // Affiche un message une fois connecté au Wi-Fi
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  // Affiche l'adresse IP obtenue par l'ESP8266

  // Configuration du servomoteur
  servo.attach(servoPin);     // Attache le servomoteur à la broche spécifiée (D4)
  servo.write(currentAngle);  // Positionne le servomoteur à l'angle initial de 90 degrés

  // Configuration de la LED
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);  // Assure que la LED est éteinte au démarrage

  // Démarrage du serveur WebSocket
  webSocket.begin();                    // Démarre le serveur WebSocket
  webSocket.onEvent(onWebSocketEvent);  // Associe la fonction 'onWebSocketEvent' pour gérer les événements WebSocket
}

// Fonction appelée pour gérer les événements WebSocket (connexion, déconnexion, messages)
void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  // Vérifie le type d'événement WebSocket
  switch (type) {
    case WStype_DISCONNECTED:
      // Quand un client se déconnecte
      Serial.printf("[%u] Disconnected!\n", num);
      break;

    case WStype_CONNECTED:
      {
        // Quand un nouveau client se connecte
        IPAddress ip = webSocket.remoteIP(num);       // Obtient l'adresse IP du client connecté
        Serial.printf("[%u] Connection from ", num);  // Affiche l'identifiant du client
        Serial.println(ip.toString());                // Affiche l'adresse IP du client
      }
      break;

    case WStype_TEXT:
      {
        // Quand un message texte est reçu depuis un client WebSocket
        String message = String((char*)payload);         // Convertit le message reçu en chaîne de caractères
        Serial.printf("[%u] Text: %s\n", num, payload);  // Affiche le message reçu

        // Analyse de la commande reçue (ex: ANGLE:90)
        if (message.startsWith("ANGLE:")) {
          int angle = message.substring(6).toInt();  // Extrait la valeur de l'angle
          // Vérifie que l'angle est dans la plage autorisée (0 à 180)
          if (angle >= 0 && angle <= 180) {
            servo.write(angle);                                                // Déplace le servomoteur à l'angle spécifié
            currentAngle = angle;                                              // Met à jour la position actuelle du servomoteur
            Serial.printf("Déplacement du servomoteur à %d degrés\n", angle);  // Affiche l'angle atteint
          }
        }

        // Commande pour allumer la LED
        if (message == "LED_ON") {
          digitalWrite(ledPin, HIGH);  // Allume la LED
          Serial.println("LED allumée");
        }

        // Commande pour éteindre la LED
        if (message == "LED_OFF") {
          digitalWrite(ledPin, LOW);  // Éteint la LED
          Serial.println("LED éteinte");
        }
      }
      break;

    default:
      // Ne fait rien pour les autres types d'événements
      break;
  }
}

void loop() {
  // Lire la valeur du potentiomètre
  int potentiometerValue = analogRead(potentiometerPin);       // Lit la valeur analogique du potentiomètre (0 à 1023) 
  int mappedAngle = map(potentiometerValue, 0, 1023, 0, 180);  // Convertit cette valeur en un angle entre 0 et 180 degrés

  // Si la position du servomoteur doit être mise à jour
  if (mappedAngle != currentAngle) {
    currentAngle = mappedAngle;  // Met à jour la variable de l'angle actuel
    servo.write(currentAngle);   // Déplace le servomoteur à la nouvelle position

    // Envoie la nouvelle position du servomoteur aux clients connectés via WebSocket
    String message = "ANGLE:" + String(currentAngle);  // Crée un message au format "ANGLE:<valeur>"
    webSocket.broadcastTXT(message);                   // Envoie le message à tous les clients connectés
  }

  // Gérer les connexions et messages WebSocket
  webSocket.loop();  // Traite les nouvelles connexions, déconnexions et messages WebSocket
  delay(50);         // Délai pour éviter de saturer le WebSocket avec trop de messages
}
