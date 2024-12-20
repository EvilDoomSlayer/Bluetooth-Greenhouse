/*
 * Bluetooth Greenhouse Arduino code
 * Created: 02/11/2024 05:48:52 p. m.
 * Author : alexy
 */

/*
* Programa del Invernadero
* Este programa controla:
* - shield_boba
* - shield_temperatura
* - shield_luz
* - shield_luz_led
* - shield_humedad
* - hc05 (Bluetooth)
* 
* Este programa automatiza los procesos del sistema de invernadero.
*/

// Librerias
#include <SoftwareSerial.h>

// Definición de pines
#define pin_foco 9            
#define pin_abanico 8         
#define pin_lm35 A0            
#define pin_bomba 10            
#define pin_ldr A1             
#define pin_led 11             
#define pin_humedad A2         
#define hc05_rx 12
#define hc05_tx 13

// Variables globales
#define VREF 5.0 // Voltaje de referencia

// Temperatura
float temperatura_objetivo;      // Temperatura deseada en el invernadero
float temperatura_actual;              // Variable para almacenar la temperatura en grados

// Luz
#define nivel_luz_minimo 3.5           // Voltaje mínimo requerido para activar la luz LED
bool led_state;

// Humedad
#define nivel_humedad_minimo 1.05       // Voltaje mínimo requerido para activar la bomba de agua
#define _100_humedo 2.2                 // Voltaje que representa un porcentaje de humedad del 100%
#define tiempo_de_riego 3000            // Es el tiempo que regara la planta cuando se presione el boton de regar ahora

// Bluetooth
SoftwareSerial invernadero(hc05_rx, hc05_tx); 	// pin 12 como RX, pin 13 como TX
String data_recibed; //Variable donde se guardaran los datos recibidos del celular

// Datos enviados de la app al Arduino
float temp_des = 30.0; 
int ilum_mode = 1; //Modo automático o manual del control de la iluminacion
int riego_mode = 1; //Modo automático o manual del control de la humedad
int ilum_state; 
int riego_state; //Activar o no activar el riego instantaneo
int anterior_riego_state;  // Variable que guarda el estado de riego enviado por ultima vez al arduino

// Macros de control de dispositivos
#define encender_foco     digitalWrite(pin_foco, HIGH)
#define apagar_foco       digitalWrite(pin_foco, LOW)

#define encender_abanico  digitalWrite(pin_abanico, HIGH)
#define apagar_abanico    digitalWrite(pin_abanico, LOW)

#define encender_bomba    digitalWrite(pin_bomba, HIGH)
#define apagar_bomba      digitalWrite(pin_bomba, LOW)

#define encender_led      digitalWrite(pin_led, HIGH)
#define apagar_led        digitalWrite(pin_led, LOW)


// Prototipos de funciones
void ports_init(void);
float read_light_level(void);
void control_light(float (*get_voltage)());
float read_humidity_level(void);
void control_humidity(float (*humidity_voltage)());
float read_temperature_level(void);
void control_temperature(float (*temperature)());



void setup() {
  Serial.begin(9600);
  ports_init();
  invernadero.begin(38400); 	// Se inicia la velocidad de comunicacion del HC-05
  encender_abanico;
}

void loop() {
  unsigned long currentMillis = millis();
  static unsigned long lastUpdateMillis = 0;

  // Lee el Bluetooth si está disponible
  hc05_read();

  // Controla temperatura
  temperatura_objetivo = temp_des;
  control_temperature(read_temperature_level);

  // Control de iluminación
  if (ilum_mode == 1) {
    control_light(read_light_level);
  } else {
    led_state = ilum_state;
    digitalWrite(pin_led, ilum_state ? HIGH : LOW);
  }

  // Control de riego
  if (riego_mode == 1) {
    control_humidity(read_humidity_level);
  } else {
    apagar_bomba;
  }

  // Maneja riego instantáneo
  if (riego_state != anterior_riego_state) {
    anterior_riego_state = riego_state;
    encender_bomba;
    delay(tiempo_de_riego); // Reemplazar con millis si es necesario evitar bloqueo
    apagar_bomba;
  }

  // Envía datos por Bluetooth cada 500ms
  if (currentMillis - lastUpdateMillis >= 500) {
    lastUpdateMillis = currentMillis;
    int humedad_send = (int)((read_humidity_level() * 100) / _100_humedo);
    bool iluminacion_send = led_state;
    float temperatura_send = read_temperature_level();
    String message = String(humedad_send) + "," + iluminacion_send + "," + temperatura_send;

    invernadero.print(message);
    Serial.println(message);
  }
}





void ports_init(void) {
  // Temperatura
  pinMode(pin_foco, OUTPUT);
  pinMode(pin_abanico, OUTPUT);
  pinMode(pin_lm35, INPUT);
  // Luz
  pinMode(pin_ldr, INPUT);
  // LED
  pinMode(pin_led, OUTPUT);
  // Bomba de agua
  pinMode(pin_bomba, OUTPUT);
  // Humedad
  pinMode(pin_humedad, INPUT);
}



float read_light_level(void) {
  // Lee y convierte el valor de la fotoresistencia en voltaje
  float lectura_ldr = analogRead(pin_ldr);
  float voltage = 5.0 - ((lectura_ldr / 1023.0) * VREF);
  return voltage;
}

void control_light(float (*ldr_voltage)()) { 
  float voltage = ldr_voltage();
  if (voltage >= nivel_luz_minimo) { // Si el voltaje es suficiente, enciende el LED
      encender_led;
      led_state = 1;
  } else { // Si el voltaje es menor al nivel mínimo, apaga el LED
      apagar_led;
      led_state = 0;
  }
}




float read_humidity_level(void) { 
  // Lee y convierte el valor del sensor de humedad en voltaje
  float lectura_humedad = analogRead(pin_humedad);
  float voltage = (VREF/1023)*(lectura_humedad); //Calcula la humedad en volts
  return voltage;
}


void control_humidity(float (*humidity_voltage)()) { 
  float voltage = humidity_voltage();
  if (voltage < nivel_humedad_minimo){ //Si la humedad en la tierra es menor:
      encender_bomba; 
  }
  else { //Si la humedad en la tierra mayor:
      apagar_bomba; 
  }
}




float read_temperature_level(void) { 
  // Lee y convierte el valor del sensor de temperatura en grados Celsius
  float milivolts = analogRead(pin_lm35) * ((VREF*1000.0) / 1023.0); //Convierte el valor analogico en milivolts
  float temperatura = milivolts / 10.0; // LM35 tiene una salida de 10mV por grado Celsius
  return temperatura;
}


void control_temperature(float (*temperature)()) { //Funcion que toma deciciones respecto a la temperatura medida
  float temp = temperature();
  if (temp < temperatura_objetivo){ //Si la temperatura del invernadero es menor al objetivo:
      encender_foco;
  }
  else { //Si la temperatura promedio del invernadero es mayor al objetivo:
      apagar_foco; 
  }
}



void hc05_read(void) {
    if (invernadero.available() > 0) {
        data_recibed = invernadero.readString();
        if (data_recibed.startsWith("<")) {
            int parsedTemp, parsedIlumMode, parsedRiegoMode, parsedIlumState, parsedRiegoState;
            sscanf(data_recibed.c_str(), "<%d,%d,%d,%d,%d>", &parsedTemp, &parsedIlumMode, &parsedRiegoMode, &parsedIlumState, &parsedRiegoState);
            temp_des = parsedTemp;
            ilum_mode = parsedIlumMode;
            riego_mode = parsedRiegoMode;
            ilum_state = parsedIlumState;
            riego_state = parsedRiegoState;
        }
    }
}

