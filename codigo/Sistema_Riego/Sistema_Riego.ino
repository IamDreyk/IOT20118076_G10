//Libreria para conectar la placa ESP a la red Wifi
#include <ESP8266WiFi.h>
/*
Documentación Libreria PubSubClient.h:
https://pubsubclient.knolleary.net/api
*/
#include <PubSubClient.h>
//Libreria para manejar el sensor DHT
#include <DHT.h>
//Pines usados 
#define TMP A0 //Para el Sensor de Humeda del Suelo
#define DHTPIN D1 // Para el DHT11
#define DHTTYPE DHT11
#define LED_RED D6 //RED :D6
#define LED_BLUE D8 //BLUE:D8
#define LED_GREEN D7 //GREEN:D7
#define relay D2 //Relay:D2

//Valores de referencia para los estados de Humedad
#define seco 1
#define normal 2
#define humedo 3
//Valores de referencia para los estados de Temperatura
#define frio 1
#define optimo 2
#define caliente 3
//Valores de referencia para los estados de Humedad_suelo
#define sueloseco 1
#define suelohumedo 2
#define suelomuyhumedo 3

//Valor de referencia para el tipo de riego
#define riego_automatico 0
#define riego_manual 1

// Datos de la red a conectarse
const char* ssid = "Wifi leyco";
const char* password = "18200069";
const char* mqtt_server = "192.168.1.38";//IP del servidor,que en este caso es local

//Declaracion sensor DHT11
DHT dht(DHTPIN, DHTTYPE);
//Declaracion cliente Wifi
WiFiClient espClient;
//Declaracion cliente MQTT
PubSubClient client(espClient);



//Variables globales
int humedad_suelo;
float temp;
float humedad;

//Variables que almacenaran el estado en el que se encuentre la lectura correspondiente
int estado_humedad;
int estado_temperatura;
int estado_humedadsuelo;

unsigned long lastMsg = 0;

//Variable para determinar que tipo de riego realizar:Manual o Automatico
int tiporiego=0; //De manera predeterminada sera automatico
/*
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;
*/


//Funcion para conectar la placa a la red Wifi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectandose a ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  //Iniciando Conexion
  WiFi.begin(ssid, password);
 //Bucle que se ejecuta hasta que se conecte
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("Direccion IP: ");
  Serial.println(WiFi.localIP());
}


/*
Si el cliente se configura como SUSCRIPTOR,se debe proporcionar una callback en el constructor.
Esta funcion se llamara cuando llegan nuevos mensajes al cliente
*/
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido de [");
  Serial.print(topic);
  Serial.println("] ");
  String payload_;
  for (int i = 0; i < length; i++) {
    //Concatena los caracteres en una variable cadena
    payload_+=(char)payload[i]; 
  }
  
  Serial.println(payload_);
  
   if ((char)payload[0] == '1') {
      digitalWrite(D2,HIGH); 
      Serial.println("LED ENCENDIDO"); 
    } else {
      digitalWrite(D2,LOW);
      Serial.println("LED APAGADO"); 
    }
}



//Funcion para reconexion al broker
void reconnect() {
  while (!client.connected()) {
      // Comprueba si el cliente esta conectado al broker
      /*la funcion devuelve 
        true: el cliente esta conectado
        false: el cliente no esta conectado
        */
    Serial.print("Intentando la conexión MQTT...");
    // Crea una ID Random para el cliente
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Intenta conectar con los parametros establecidos al cliente MQTT con el broker
        /*true:conexion exitosa
          false:conexion fallida*/
    if (client.connect(clientId.c_str())) {
      Serial.println("conectado");
      //Luego puede poner lineas de suscripcion al topic especifico o publicar
        
      } 
      else {
      Serial.print(" fallo conexion, rc=");
      //Mostrando el tipo de error de conexion
      Serial.print(client.state());
      Serial.println(" ...intentando denuevo en 5 segundos");
      delay(5000);
    }
  }
}


//Funcion para encender el led_rgb
void color (int R, int G, int B) {
  analogWrite(LED_RED, R);
  analogWrite(LED_GREEN, G);
  analogWrite(LED_BLUE, B);
}

//Funciones para determinar los estados 
//Funcion para determinar el estado de la temperatura
void estado_Temperatura(){
  //Si la temperatura es frio
  if(temp>=0 && temp<=18){
    estado_temperatura=1;
  }
  //Si la temperatura es optima
  else if(temp>18 && temp<=30){
    estado_temperatura=2;
  }
  //Si la temperatura es caliente
  else if(temp>30 && temp<=50){
    estado_temperatura=3;
  }
  else{
    estado_temperatura=0;
    }
}
//Funcion para determinar el estado de la Humedad
void estado_Humedad(){
  //Si la humedad es seco
  if(humedad>=0 && humedad<=25){
    estado_humedad=1;
  }
  //Si la humedad es optima
  else if(humedad>25 && humedad<=55){
    estado_humedad=2;
  }
  //Si la humedad es humeda
  else if(humedad>55 && humedad<=100){
    estado_humedad=3;
  }
  else{
    estado_humedad=0;
  }
}

//Funcion para determinar el estado de la HumedadSuelo
void estado_HumedadSuelo(){
  //Si la humedad_suelo es seco
  if(humedad_suelo>=0 && humedad_suelo<=20){
    estado_humedadsuelo=1;
  }
  //Si la humedad_suelo es humedo
  else if(humedad_suelo>20 && humedad_suelo<=45){
    estado_humedadsuelo=2;
  }
  //Si la humedad_suelo es muy humedo
  else if(humedad_suelo>45 && humedad_suelo<=100){
    estado_humedadsuelo=3;
  }
   else{
    estado_humedadsuelo=0;
    }
}

void activar_riego(){
    Serial.println("Riego activado.....");
    //El relay se activa con un LOW
    digitalWrite(relay,LOW);
    //Podemos prender un led
}

void apagar_riego(){
    Serial.println("Riego apagado.....");
    //El relay se desactiva con un HIGH
    digitalWrite(relay,HIGH);
   //Podemos apagar el led
}


void riego_autonomo(){
  //Primero determinamos los estados de las variables
  estado_Temperatura();
  estado_Humedad();
  estado_HumedadSuelo();
  if(estado_humedad==seco && estado_temperatura==frio && estado_humedadsuelo==sueloseco ){
    color(9,224,247);
    //Riego
    activar_riego();
  }
  else if(estado_humedad==normal && estado_temperatura==frio && estado_humedadsuelo==sueloseco){
    color(255,192,0);
    //Riego
    activar_riego();
  }  
  else if(estado_humedad==humedo && estado_temperatura==frio && estado_humedadsuelo==sueloseco){
    color(183,196,60);
    //Riego
    activar_riego();
  }  
  else if(estado_humedad==seco && estado_temperatura==optimo && estado_humedadsuelo==sueloseco){
    color(252,4,187);
    //Riego
    activar_riego();
  }  
  else if(estado_humedad==normal && estado_temperatura==optimo && estado_humedadsuelo==sueloseco){
    color(3,45,253);
    //Riego
    activar_riego();
  }  
  else if(estado_humedad==humedo && estado_temperatura==optimo && estado_humedadsuelo==sueloseco){
    color(145,250,6);
    //Apagado
    apagar_riego();
  }  
  else if(estado_humedad==seco && estado_temperatura==caliente && estado_humedadsuelo==sueloseco){
    color(196,89,17);
    //Riego
    activar_riego();
  }  
  else if(estado_humedad==normal && estado_temperatura==caliente && estado_humedadsuelo==sueloseco){
    color(112,48,160);
    //Riego
    activar_riego();
  } 
  else if(estado_humedad==humedo && estado_temperatura==caliente && estado_humedadsuelo==sueloseco){
    color(255,255,0);
    //Apagado
    apagar_riego();
  }  
  else if(estado_humedad==seco && estado_temperatura==frio && estado_humedadsuelo==suelohumedo){
    color(255,102,153);
    //Riego
    activar_riego();
  }  
  else if(estado_humedad==normal && estado_temperatura==frio && estado_humedadsuelo==suelohumedo){
    color(166,166,166);
    //Apagado
    apagar_riego();
  }  
  else if(estado_humedad==humedo && estado_temperatura==frio && estado_humedadsuelo==suelohumedo){
    color(255,152,1);
    //Apagado
    apagar_riego();
  }  
  else if(estado_humedad==seco && estado_temperatura==optimo && estado_humedadsuelo==suelohumedo){
    color(31,78,121);
    //Riego
    activar_riego();
  }  
  else if(estado_humedad==normal && estado_temperatura==optimo && estado_humedadsuelo==suelohumedo){
    color(83,129,53);
    //Apagado
    apagar_riego();
  }
  else if(estado_humedad==humedo && estado_temperatura==optimo && estado_humedadsuelo==suelohumedo){
    color(195,64,61);
    //Apagado
    apagar_riego();
  }
   else if(estado_humedad==seco && estado_temperatura==caliente && estado_humedadsuelo==suelohumedo){
    color(221,35,75);
    //Riego
    activar_riego();
  }  
   else if(estado_humedad==normal && estado_temperatura==caliente && estado_humedadsuelo==suelohumedo){
    color(222,29,227);
    //Apagado
    apagar_riego();
  }   
    else if(estado_humedad==humedo && estado_temperatura==caliente && estado_humedadsuelo==suelohumedo){
    color(104,246,128);
    //Riego
    activar_riego();
  }     
    else if(estado_humedadsuelo==suelomuyhumedo){
     color(255,0,0);
    //Apagado
    apagar_riego();
  }         
}


void setup() {
    
  // Definimos los pins
  //Led RGB
  pinMode(LED_RED, OUTPUT); // Red: D6
  pinMode(LED_GREEN, OUTPUT); // Green: D8
  pinMode(LED_BLUE, OUTPUT); // Blue: D7
   //Pin Relay
   pinMode(relay,OUTPUT);
  //Inicializamos el Sensor DHT11
  dht.begin();
  Serial.begin(115200);
  //Conectadose a la red Wifi
  setup_wifi();
  //Establece los parametros del broker : direccion y puerto
  client.setServer(mqtt_server, 1883);
  //Establece la función de devolución de llamada de mensajes(callback).
    //callback: es un puntero a la funcion de callback,la cual se llama cuando llega un mensaje para la suscripcion hecha por el cliente
  client.setCallback(callback);
  //Empieza apagado el led
  digitalWrite(LED_RED,LOW);
  digitalWrite(LED_GREEN,LOW);
  digitalWrite(LED_BLUE,LOW);
}


void loop() {
 // Comprobando si el cliente esta conectado al broker
  if (!client.connected()) {
    //Si no esta conectando se intenta la reconexion
    reconnect();
  }
  /*La funcion loop() debe llamarse con frecuencia para permitir que el cliente procese los mensajes 
   * entrantes y mantenga su conexion  con el broker
  */
  client.loop();

   humedad_suelo = map(analogRead(TMP), 0, 1023, 100, 0);
   // Leemos la humedad relativa
    humedad = dht.readHumidity();
   // Leemos la temperatura en grados centígrados (por defecto)
    temp = dht.readTemperature();
   if (isnan(humedad) || isnan(temp) ) {
    Serial.println("Error obteniendo los datos del sensor DHT11");
    return;
  }
    Serial.print("Humedad: ");
    Serial.print(humedad);
    Serial.print("% ");
    Serial.print("Temperatura: ");
    Serial.print(temp);
    Serial.print(" *C ");
    Serial.print("Humedad del Suelo: ");
    Serial.print(humedad_suelo);
    Serial.println("% ");
  
  //millis():Devuelve el número de milisegundos transcurridos desde que la placa Arduino comenzó a ejecutar el programa actual.
  unsigned long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
   client.publish("casa/jardin/temperatura", String(temp).c_str());
   delay(100);
   client.publish("casa/jardin/humedad", String(humedad).c_str());
   delay(100);
   client.publish("casa/jardin/humedadsuelo", String(humedad_suelo).c_str());
  }

  delay(5000);
  if(tiporiego==riego_automatico){
    riego_autonomo();
  }
  else if(tiporiego==riego_manual){
    //
  }
  
   delay(5000);
}
