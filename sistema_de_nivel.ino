#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#define WIFI_SSID "TU RED WIFI"
#define WIFI_PASSWORD "LA CONTRASEÑA DE TU RED WIFI"
#define BOT_TOKEN "TU BOT_TOKEN"
#define ID_CHAT "TU ID_CHAT"

float PROFUNDIDAD = 130.0;   //profundidad maxima
#define DISTANCIA_MINIMA 24  //el sensor SJN-SRO4 no puede medir menos de 24 cm

//pines
#define PIN_FLOTADOR 13
#define PIN_BOTON 14
#define PIN_LED_BOMBA 22
#define PIN_BOMBA 23
#define PIN_LED_WIFI 25
#define PIN_ECHO 32
#define PIN_TRIGGER 33



WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

unsigned long tiempoEntreSensado = 0;
unsigned long tiempoAnteriorMensaje = 0;  //última vez que se realizó el análisis de mensajes
unsigned long tiempoSensorNivel = 0;
unsigned long tiempoSensorSonico = 0;
unsigned long tiempoBoton = 0;
unsigned long tiempoConexion = 0;
unsigned long tiempoConexionLed = 0;
int distancia = 0, distanciaAnterior = -1, porcentajeNivel = 0;

bool
  enviarMensaje = false,      //Indica si debe enviarse el mensaje
  enviarMenu = false,         //Indica si debe enviarse el menu
  enviado = false,            //Indica si el mesdaje o menu fue enviado
  estadoBomba = false,        //indica el estado de la bomba
  estadoLedWifi = false,      //indica el estado del led de la conexion wifi
  reconexionWifi = false,     //indica si hay reconexion wifi
  masOpciones = false,        //controla al menu de mas opciones
  configurando = false,       //indica que se esta configurando una opcion
  llenadoAutomatico = false;  //indica si el llenado automatico esta activado
String mensaje, opciones;



void mensajesNuevos() {
  String text = bot.messages[0].text;

  //▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓CONFIGURANDO PROFUNDIDAD▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
  if (configurando) {

    //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░REGRESAR░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
    if (text.equalsIgnoreCase("Regresar")) {
      if (llenadoAutomatico) {
        sendMessageButtons("Opciones", "[[\"Cambiar profundidad\"],[\"Llenado manual\"],[\"Ayuda\", \"Acerca de\"],[\"Reiniciar\"],[\"Regresar\"]]");
      } else {
        sendMessageButtons("Opciones", "[[\"Cambiar profundidad\"],[\"Llenado automatico\"],[\"Ayuda\", \"Acerca de\"],[\"Reiniciar\"],[\"Regresar\"]]");
      }
      configurando = false;
    } else {
      float profundidadN = text.toFloat();

      //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░VALOR INVALIDO░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
      if (!isNumber(text)) {
        sendMessage("Valor invalido, introduzca solo numeros.");
      }

      //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░MODIFICAR░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
      else if (profundidadN >= 30 && profundidadN <= 600) {
        PROFUNDIDAD = profundidadN;
        sendMessage("Profundidad modificada");
      } else {
        sendMessage("Profundidad fuera del rango permitido, profundidad no modificada");
      }
    }
  } else {

    //▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓SUBMENÚ DE MÁS OPCIONES▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
    if (masOpciones) {
      //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░PROFUNDIDAD░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
      if (text.equalsIgnoreCase("Cambiar profundidad")) {
        sendMessageButtons("Seleccione o introduzca la profundidad del tinaco en cm (max 600 y min 30):", "[[\"30\",\"100\",\"130\",\"200\"],[\"300\",\"400\",\"500\",\"600\"],[\"Regresar\"]]");
        configurando = true;
      }

      //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░LLENADO AUTOMATICO░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
      else if (text.equalsIgnoreCase("Llenado automatico")) {

        if (!llenadoAutomatico) {
          sendMessageButtons("Llenado automatico activado", "[[\"Cambiar profundidad\"],[\"Llenado manual\"],[\"Ayuda\", \"Acerca de\"],[\"Reiniciar\"],[\"Regresar\"]]"),

            llenadoAutomatico = true;
        } else {
          sendMessage("El llenado automatico ya esta activado");
        }
      }
      //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░LLENADO MANUAL░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
      else if (text.equalsIgnoreCase("Llenado manual")) {
        if (llenadoAutomatico) {
          sendMessageButtons("Llenado automatico desactivado", "[[\"Cambiar profundidad\"],[\"Llenado automatico\"],[\"Ayuda\", \"Acerca de\"],[\"Reiniciar\"],[\"Regresar\"]]");
          llenadoAutomatico = false;
        } else {
          sendMessage("El llenado automatico ya esta desactivado");
        }
      }

      //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░AYUDA░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
      else if (text.equalsIgnoreCase("Ayuda")) {
        String ayuda = "Ajustes.\n";
        ayuda += "Rango de lectura de la profundidad: la profundidad minima es de 30 cm y la maxima es de 600 cm.\nPor defecto tiene una distancia de 130 cm.\n";
        ayuda += "El llenado automático enciende la bomba cuando el nivel baja a menos del 15% de capacidad\nPor defecto está desactivado.\n";
        ayuda += "Los ajustes realizados se borran al reiniciar el sistema.\n\n";
        ayuda += "Leds.\n";
        ayuda += "El led azul indica el estado de la bomba.\n";
        ayuda += "El led rojo parpadeante indica que el sistema está desconectado de la red.\n\n";
        ayuda += "Botón físico.\n";
        ayuda += "Enciende o apaga la bomba, el tiempo entre cada presionado es de 15 segundos.";

        sendMessage(ayuda);
      }

      //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ACERCA DE░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
      else if (text.equalsIgnoreCase("Acerca de")) {
        String msg = "Sistema de control de bomba de agua con Esp32.\n";
        msg += "Versión: 2.0\n";
        msg += "Fecha: 2026-05-06\n\n";
        msg += "Creado por Joel Alejandro Valadez Arellano";
        sendMessage(msg);
      }

      //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░REINICIAR░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
      else if (text.equalsIgnoreCase("Reiniciar")) {
        sendMessage("Reiniciando sistema");
        bombaOff();
        while (bot.getUpdates(bot.last_message_received + 1)) {}
        ESP.restart();
      }

      //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░REGRESAR░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
      else if (text.equalsIgnoreCase("Regresar")) {
        if (estadoBomba) {
          sendMessageButtons("Menu", "[[\"Apagar\"], [\"Estado\"],[\"Más opciones\"]]");
        } else {
          sendMessageButtons("Menu", "[[\"Encender\"], [\"Estado\"],[\"Más opciones\"]]");
        }
        configurando = false;
        masOpciones = false;
      }

      //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░COMANDO INEXISTENTE░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
      else {
        sendMessage("Comando no reconocido");
      }
    }

    //▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓MENÚ PRINCIPAL▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
    else {
      //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ENCENDER░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
      if (text.equalsIgnoreCase("Encender")) {
        if (digitalRead(PIN_FLOTADOR) == 0 && porcentajeNivel < 85) {
          if (!estadoBomba) {
            bombaOn();
            bot.sendMessageWithReplyKeyboard(ID_CHAT, "Bomba encendida", "", "[[\"Apagar\"], [\"Estado\"],[\"Más opciones\"]]", true);
          } else {
            sendMessage("La bomba ya estaba encendida");
          }
        } else {
          bot.sendMessageWithReplyKeyboard(ID_CHAT, "Tinaco lleno, bomba no encendida", "", "[[\"Encender\"], [\"Estado\"],[\"Más opciones\"]]", true);
        }
      }

      //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░APAGAR░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
      else if (text.equalsIgnoreCase("Apagar")) {
        if (estadoBomba) {
          bombaOff();
          bot.sendMessageWithReplyKeyboard(ID_CHAT, "Bomba apagada", "", "[[\"Encender\"],[\"Estado\"],[\"Más opciones\"]]", true);
        } else {
          sendMessage("La bomba ya estaba apagada");
        }
      }

      //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ESTADO░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
      else if (text.equalsIgnoreCase("Estado")) {
        String mensaje = "Bomba ";
        if (estadoBomba) {
          mensaje += "encendida";
        } else {
          mensaje += "apagada";
        }
        sendMessage(mensaje + "\nPorcentaje: " + String(porcentajeNivel) + "%");
      }

      //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░MÁS OPCIONES░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
      else if (text.equalsIgnoreCase("Más Opciones")) {
        if (llenadoAutomatico) {
          sendMessageButtons("Opciones", "[[\"Cambiar profundidad\"],[\"Llenado manual\"],[\"Ayuda\", \"Acerca de\"],[\"Reiniciar\"],[\"Regresar\"]]");

        } else {
          sendMessageButtons("Opciones", "[[\"Cambiar profundidad\"],[\"Llenado automatico\"],[\"Ayuda\", \"Acerca de\"],[\"Reiniciar\"],[\"Regresar\"]]");
        }
        masOpciones = true;
      }

      //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░COMANDO INEXISTENTE░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
      else {
        sendMessage("Comando no reconocido");
      }
    }
  }
}

void sendMessage(String msg) {
  if (WiFi.status() == WL_CONNECTED) {
    bot.sendMessage(ID_CHAT, msg, "");
  }
}

void sendMessageButtons(String msg, String opcs) {
  if (WiFi.status() == WL_CONNECTED) {
    bot.sendMessageWithReplyKeyboard(ID_CHAT, msg, "", opcs, true);
  }
}

void calcularPorcentajeNivel() {
  long tiempo;
  float sumaDistancias = 0;
  int lecturasValidas = 0;

  for (int i = 0; i < 10; i++) {
    digitalWrite(PIN_TRIGGER, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_TRIGGER, HIGH);
    delayMicroseconds(20);
    digitalWrite(PIN_TRIGGER, LOW);

    tiempo = pulseIn(PIN_ECHO, HIGH, 30000);

    float distanciaMuestra = tiempo * 0.0175;
    if (distanciaMuestra > DISTANCIA_MINIMA && distanciaMuestra <= PROFUNDIDAD) {
      sumaDistancias += distanciaMuestra;
      lecturasValidas++;
    }
    delay(30);
  }

  if (lecturasValidas > 0) {
    distancia = sumaDistancias / lecturasValidas;  // Promedio de muestras validas
  } else {
    distancia = distanciaAnterior;
  }

  if (distancia > PROFUNDIDAD) {
    distancia = PROFUNDIDAD;
  } else if (distancia < DISTANCIA_MINIMA) {
    distancia = DISTANCIA_MINIMA;
  }

  if (distanciaAnterior == -1 || digitalRead(PIN_FLOTADOR) == 1) {
    distanciaAnterior = distancia;
  }

  if (abs(distancia - distanciaAnterior) < 4) {
    distanciaAnterior = distancia;
    porcentajeNivel = round(((PROFUNDIDAD - distancia) * 100) / (PROFUNDIDAD));
  }
}

bool isNumber(String texto) {
  if (texto.length() == 0) return false;

  int puntos = 0;
  for (int i = 0; i < texto.length(); i++) {
    if (texto.charAt(i) == '.') {
      puntos++;
      if (puntos > 1) return false;
    } else if (!isDigit(texto.charAt(i))) {
      return false;
    }
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BOMBA, OUTPUT);
  pinMode(PIN_LED_BOMBA, OUTPUT);
  pinMode(PIN_LED_WIFI, OUTPUT);
  pinMode(PIN_TRIGGER, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_BOTON, INPUT);
  pinMode(PIN_FLOTADOR, INPUT);
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);  //Agregar certificado raíz para api.telegram.org
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  estadoLedWifi = true;
  digitalWrite(PIN_LED_WIFI, HIGH);
}

void loop() {
  //cada 15 segundos por seguridad del motor
  if (millis() - tiempoBoton >= 15000 && digitalRead(PIN_BOTON) == 1) {
    if (estadoBomba) {
      bombaOff();
      mensaje = "Bomba apagada manualmente";
      opciones = "[[\"Encender\"], [\"Estado\"],[\"Más opciones\"]]";
    } else if (digitalRead(PIN_FLOTADOR) == 0 && porcentajeNivel < 80) {
      bombaOn();
      mensaje = "Bomba encendida manualmente";
      opciones = "[[\"Apagar\"], [\"Estado\"],[\"Más opciones\"]]";
    } else {
      mensaje = "Tinaco lleno, bomba no encendida";
      opciones = "[[\"Encender\"], [\"Estado\"],[\"Más opciones\"]]";
    }

    enviarMenu = true;
    tiempoBoton = millis();
  }

  //sensar cada segundo
  if (millis() - tiempoEntreSensado >= 1000) {
    tiempoEntreSensado = millis();
    calcularPorcentajeNivel();

    String pr = "\nporcentaje = " + String(porcentajeNivel);
    Serial.println(pr);

    String dist = "distancia = " + String(distancia);
    Serial.println(dist);

    //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░cuando la bomba este encendida░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
    if (estadoBomba) {                       //bomba encendida
      if (digitalRead(PIN_FLOTADOR) == 1) {  //flotador arriba
        bombaOff();
        mensaje = "Tinaco lleno, bomba apagada";  //enviar mensaje de bomba apagada
        opciones = "[[\"Encender\"], [\"Estado\"],[\"Más opciones\"]]";
        enviarMenu = true;
      } else {

        if (porcentajeNivel % 20 <= 3) {  //cada 20% con un margen de 3
          if (!enviado) {
            sendMessage("El agua ha superado el " + String(porcentajeNivel) + "%");
            enviado = true;
          }
        } else if (porcentajeNivel % 20 > 15) {  //cuando falte 5% o menos para el sig. msg
          enviado = false;
        }
      }
    }
    //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░cuando la bomba este apagada░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
    else if (porcentajeNivel <= 15) {
      if (llenadoAutomatico) {
        bombaOn();
        mensaje = "Tinaco al " + String(porcentajeNivel) + "%, bomba encendida automaticamente";  //enviar mensaje de bomba encendida
        opciones = "[[\"Apagar\"], [\"Estado\"],[\"Más opciones\"]]";
        enviarMenu = true;

      } else {
        if (porcentajeNivel % 4 == 0) {  //cada 4% hacia abajo
          if (!enviado) {
            mensaje = "Tinaco al " + String(porcentajeNivel) + "%, quieres encender la bomba";  //enviar mensaje de nivel de agua y que si quiere encender el tinaco
            opciones = "[[\"Encender\"], [\"Estado\"],[\"Más opciones\"]]";
            enviarMenu = true;
            enviado = true;
          }
        } else if (porcentajeNivel % 4 > 1) {  //cuando falte EL 2% o 3% para el sig. msg
          enviado = false;
        }
      }
    }
  }
  //intentar conectar cada 5 segundos si esta desconectado
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - tiempoConexion >= 5000) {
      Serial.print(F("Conectando a la red "));
      Serial.print(WIFI_SSID);
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      delay(100);
      digitalWrite(PIN_LED_WIFI, HIGH);

      tiempoConexion = millis();
    }
    if (!reconexionWifi) {
      reconexionWifi = true;
    }

    if (millis() - tiempoConexionLed >= 500) {
      if (estadoLedWifi) {
        digitalWrite(PIN_LED_WIFI, LOW);
      } else {
        digitalWrite(PIN_LED_WIFI, HIGH);
      }
      estadoLedWifi = !estadoLedWifi;
      tiempoConexionLed = millis();
    }
  }
  //al conectarse...
  else {
    if (reconexionWifi) {
      digitalWrite(PIN_LED_WIFI, LOW);
      estadoLedWifi = false;

      Serial.print(F("\nConectado a la red wifi. Dirección IP: "));
      Serial.println(WiFi.localIP());
      Serial.println(F("Sistema preparado"));

      while (bot.getUpdates(bot.last_message_received + 1)) {}
      mensaje = "Sistema de nivel de agua en funcionamiento.\nEstado de la bomba: ";
      if (estadoBomba) {
        mensaje += "encendida";
        opciones = "[[\"Apagar\"], [\"Estado\"],[\"Más opciones\"]]";
      } else {
        mensaje += "apagada";
        opciones = "[[\"Encender\"], [\"Estado\"],[\"Más opciones\"]]";
      }
      sendMessageButtons(mensaje, opciones);

      reconexionWifi = false;
    }
    // cada medio segundo responder mensajes
    else if (millis() - tiempoAnteriorMensaje > 500) {
      int numeroMensajes = bot.getUpdates(bot.last_message_received + 1);
      if (numeroMensajes > 0) {
        Serial.println(F("Comando recibido"));
        mensajesNuevos();
      }
      tiempoAnteriorMensaje = millis();
    }

    if (enviarMensaje) {
      sendMessage(mensaje);
      enviarMensaje = false;
    }

    if (enviarMenu) {
      if (masOpciones) {
        sendMessage(mensaje);
      } else {
        sendMessageButtons(mensaje, opciones);
      }
      enviarMenu = false;
    }
  }
}



void bombaOn() {
  digitalWrite(PIN_BOMBA, HIGH);
  digitalWrite(PIN_LED_BOMBA, HIGH);
  estadoBomba = true;
}

void bombaOff() {
  digitalWrite(PIN_BOMBA, LOW);
  digitalWrite(PIN_LED_BOMBA, LOW);
  estadoBomba = false;
}