#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

#define WIFI_SSID "TU RED WIFI"
#define WIFI_PASSWORD "CONTRASEÑA DE TU RED WIFI"
#define BOT_TOKEN "TU BOT_TOKEN"
#define ID_CHAT "TU ID_CHAT"

#define TIEMPO 1000         //TIEMPO medio entre mensajes de escaneo
float PROFUNDIDAD = 130.0;  //profundidad maxima
#define DISTANCIA_MINIMA 15

//pines
#define PIN_FLOTADOR 13
#define PIN_BOTON 14
#define PIN_LED_BOMBA 22
#define PIN_BOMBA 23
#define PIN_LED_WIFI 25
#define PIN_ECHO 32
#define PIN_TRIGGER 33


bool botonPresionado = false;
unsigned long tiempoEntreSensado = 0;



WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

TaskHandle_t task1;


unsigned long tiempoAnteriorMensaje;  //última vez que se realizó el análisis de mensajes
int distancia = 0, distanciaAnterior = -1, porcentajeNivel = 0;
bool
  enviarMensaje = false,
  enviarMenu = false,
  estadoBomba = false,        //indica el estado de la bomba
  masOpciones = false,        //controla al menu de mas opciones
  configuracion = false,      //controla al menu de configuracion
  configurando = false,       //indica que se esta configurando una opcion
  llenadoAutomatico = false;  //indica si el llenado automatico esta activado
String mensaje, opciones;



void mensajesNuevos(int numeroMensajes) {
  for (int i = 0; i < numeroMensajes; i++) {
    String text = bot.messages[i].text;
    //▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓SUBMENÚ DE AJUSTES▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
    if (configuracion) {

      //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░PROFUNDIDAD░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
      if (text.equalsIgnoreCase("Profundidad")) {
        sendMessage("Introduzca la profundidad del tinaco en cm (max 600 y min 30):");
        configurando = true;
      }


      //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░LLENADO AUTOMATICO░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
      else if (text.equalsIgnoreCase("Llenado automatico") || text.equalsIgnoreCase("Llenado manual")) {
        llenadoAutomatico = !llenadoAutomatico;
        if (llenadoAutomatico) {
          bot.sendMessageWithReplyKeyboard(ID_CHAT, "Llenado automatico activado", "", "[[\"Profundidad\"],[\"Llenado manual\"],[\"Reiniciar\"],[\"Regresar\"]]", true);
        } else {
          bot.sendMessageWithReplyKeyboard(ID_CHAT, "Llenado automatico desactivado", "", "[[\"Profundidad\"],[\"Llenado automatico\"], [\"Reiniciar\"], [\"Regresar\"]]", true);
        }
      }

      //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░REINICIAR░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
      else if (text.equalsIgnoreCase("Reiniciar")) {
        sendMessage("Reiniciando sistema");
        delay(1500);
        ESP.restart();
      }

      //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░REGRESAR░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
      else if (text.equalsIgnoreCase("Regresar")) {
        bot.sendMessageWithReplyKeyboard(ID_CHAT, "Opciones", "", "[[\"Ajustes\"], [\"Ayuda\", \"Acerca de\"], [\"Regresar\"]]", true);
        configuracion = false;
        configurando = false;
      }

      //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░CONFIGURANDO OPCIONES░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
      else if (configurando) {
        float condicion = text.toFloat();
        if (condicion >= 30 && condicion <= 600) {
          PROFUNDIDAD = condicion;
          sendMessage("Profundidad modificada");
        } else {
          sendMessage("Profundidad no modificada");
        }
        configurando = false;

      } else {
        sendMessage("Comando no reconocido");
      }
    }

    //▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓SUBMENÚ DE MÁS OPCIONES▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓
    else if (masOpciones) {
      //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░AJUSTES░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
      if (text.equalsIgnoreCase("Ajustes")) {
        if (llenadoAutomatico) {
          bot.sendMessageWithReplyKeyboard(ID_CHAT, "Ajustes", "", "[[\"Profundidad\"],[\"Llenado manual\"],[\"Reiniciar\"],[\"Regresar\"]]", true);
        } else {
          bot.sendMessageWithReplyKeyboard(ID_CHAT, "Ajustes", "", "[[\"Profundidad\"],[\"Llenado automatico\"], [\"Reiniciar\"], [\"Regresar\"]]", true);
        }
        configuracion = true;
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
        ayuda += "Enciende o apaga la bomba, el tiempo entre cada presionado es de 5 segundos.";

        sendMessage(ayuda);
      }

      //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ACERCA DE░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
      else if (text.equalsIgnoreCase("Acerca de")) {
        String msg = "Sistema de control de bomba de agua con Esp32.\n";
        msg += "Versión: 1.0\n";
        msg += "Fecha: 2023-06-24\n\n";
        msg += "Creado por los estudiantes de Ingeniería en Computacón: \n";
        msg += "Edgar Osciel Romero Lezma\n";
        msg += "Sergio Sahid Santana Flores\n";
        msg += "Thania Rufino Morales\n";
        msg += "Joel Alejandro Valadez Arellano\n";
        sendMessage(msg);
      }

      //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░REGRESAR░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
      else if (text.equalsIgnoreCase("Regresar")) {
        if (estadoBomba) {
          bot.sendMessageWithReplyKeyboard(ID_CHAT, "Menu", "", "[[\"Apagar\"], [\"Estado\"],[\"Más opciones\"]]", true);
        } else {
          bot.sendMessageWithReplyKeyboard(ID_CHAT, "Menu", "", "[[\"Encender\"], [\"Estado\"],[\"Más opciones\"]]", true);
        }
        masOpciones = false;
      } else {
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
          }
        } else {
          bot.sendMessageWithReplyKeyboard(ID_CHAT, "Tinaco lleno, bomba no encendida", "", "[[\"Encender\"], [\"Estado\"],[\"Más opciones\"]]", true);
        }
      }
      //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░APAGAR░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
      else if (text.equalsIgnoreCase("Apagar")) {
        if (estadoBomba) {
          bombaOff();
          bot.sendMessageWithReplyKeyboard(ID_CHAT, "Bomba apagada", "", "[[\"Encender\"], [\"Estado\"],[\"Más opciones\"]]", true);
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
        bot.sendMessageWithReplyKeyboard(ID_CHAT, "Opciones", "", "[[\"Ajustes\"], [\"Ayuda\", \"Acerca de\"], [\"Regresar\"]]", true);
        masOpciones = true;
      }
    }
  }
}

void sendMessage(String msg) {
  bot.sendMessage(ID_CHAT, msg, "");
}

void conectarWifi() {
  Serial.print(F("Conectando a la red "));
  Serial.print(WIFI_SSID);
  int intentos = 5;


  while (WiFi.status() != WL_CONNECTED) {
    if (intentos == 5) {
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      intentos = 0;
    }
    Serial.print(F("."));
    digitalWrite(PIN_LED_WIFI, HIGH);
    delay(500);
    digitalWrite(PIN_LED_WIFI, LOW);
    delay(500);
    intentos++;
  }
  Serial.print(F("\nConectado a la red wifi. Dirección IP: "));
  Serial.println(WiFi.localIP());
  Serial.println(F("Sistema preparado"));
  delay(500);
  sendMessage("Sistema de nivel de agua en funcionamiento.");
  delay(500);
  if (estadoBomba) {
    bot.sendMessageWithReplyKeyboard(ID_CHAT, "Bomba encendida", "", "[[\"Apagar\"], [\"Estado\"],[\"Más opciones\"]]", true);
  } else {
    bot.sendMessageWithReplyKeyboard(ID_CHAT, "Bomba apagada", "", "[[\"Encender\"], [\"Estado\"],[\"Más opciones\"]]", true);
  }
}

long calcularPorcentajeNivel() {
  digitalWrite(PIN_TRIGGER, LOW);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIGGER, HIGH);
  delayMicroseconds(25);
  digitalWrite(PIN_TRIGGER, LOW);
  distancia = 0.0341 * pulseIn(PIN_ECHO, HIGH, 3000) / 2;




  if (distancia > PROFUNDIDAD) {
    distancia = PROFUNDIDAD;
  }

  else if (distancia < DISTANCIA_MINIMA) {
    distancia = DISTANCIA_MINIMA;
  }
  if (distanciaAnterior == -1 || digitalRead(PIN_FLOTADOR) == 1) {  //solo al iniciar o al llenarse
    distanciaAnterior = distancia;
  }

  if (distanciaAnterior >= distancia - 3 && distanciaAnterior <= distancia + 3) {
    distanciaAnterior = distancia;
    porcentajeNivel = round((distancia - DISTANCIA_MINIMA) * -100 / (PROFUNDIDAD - DISTANCIA_MINIMA) + 100);
  }
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

  xTaskCreatePinnedToCore(sensarNivelTinaco, "sensarTinaco", 10000, NULL, 1, &task1, 0);
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);  //Agregar certificado raíz para api.telegram.org
  conectarWifi();
}

void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(WL_CONNECTED);

  }

  else if (millis() - tiempoAnteriorMensaje > TIEMPO) {
    int numerosMensajes = bot.getUpdates(bot.last_message_received + 1);

    while (numerosMensajes) {
      Serial.println(F("Comando recibido"));
      mensajesNuevos(numerosMensajes);
      numerosMensajes = bot.getUpdates(bot.last_message_received + 1);
    }
    tiempoAnteriorMensaje = millis();
  }

  if (enviarMensaje) {
    sendMessage(mensaje);
    enviarMensaje = false;
  }

  if (enviarMenu) {
    if (configuracion || masOpciones) {
      sendMessage(mensaje);
    } else {
      bot.sendMessageWithReplyKeyboard(ID_CHAT, mensaje, "", opciones, true);
    }
    enviarMenu = false;
  }
}


void sensarNivelTinaco(void *pvParameters) {
  bool mensajeEnviado = false;
  unsigned long tiempoBotonPresionado = 0;

  for (;;) {
    vTaskDelay(100 / portTICK_PERIOD_MS);
    if (millis() - tiempoEntreSensado >= 1000) {  //cada segundo
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
            if (!mensajeEnviado) {
              mensaje = "El agua ha superado el " + String(porcentajeNivel) + "%";  //enviar mensaje de nivel de agua
              mensajeEnviado = true;
              enviarMensaje = true;
            }
          } else if (porcentajeNivel % 20 > 15) {  //cuando falte 5% o menos para el sig. msg
            mensajeEnviado = false;
          }
        }

        //░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░cuando la bomba este apagada░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░

      }

      else if (porcentajeNivel <= 15) {
        if (llenadoAutomatico) {
          bombaOn();
          mensaje = "Tinaco al " + String(porcentajeNivel) + "%, bomba encendida automaticamente";  //enviar mensaje de bomba encendida
          opciones = "[[\"Apagar\"], [\"Estado\"],[\"Más opciones\"]]";

        } else {
          if (porcentajeNivel % 4 == 0) {  //cada 4% hacia abajo
            if (!mensajeEnviado) {
              mensaje = "Tinaco al " + String(porcentajeNivel) + "%, quieres encender la bomba";  //enviar mensaje de nivel de agua y que si quiere encender el tinaco
              opciones = "[[\"Encender\"], [\"Estado\"],[\"Más opciones\"]]";
              mensajeEnviado = true;
              enviarMenu = true;
            }
          } else if (porcentajeNivel % 4 > 1) {  //cuando falte 2% para el sig. msg
            mensajeEnviado = false;
          }
        }
      }
      tiempoEntreSensado = millis();
    }

    if (millis() - tiempoBotonPresionado >= 5000 && digitalRead(PIN_BOTON) == 1) {  //cada 5 segundos
      if (estadoBomba) {
        bombaOff();
        mensaje = "Bomba apagada manualmente";
        opciones = "[[\"Encender\"], [\"Estado\"],[\"Más opciones\"]]";
      }

      else if (digitalRead(PIN_FLOTADOR) == 0 && porcentajeNivel < 80) {
        bombaOn();
        mensaje = "Bomba encendida manualmente";
        opciones = "[[\"Apagar\"], [\"Estado\"],[\"Más opciones\"]]";
      }

      else {
        mensaje = "Tinaco lleno, bomba no encendida";
        opciones = "[[\"Encender\"], [\"Estado\"],[\"Más opciones\"]]";
      }

      enviarMenu = true;
      tiempoBotonPresionado = millis();
    }
    yield();
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