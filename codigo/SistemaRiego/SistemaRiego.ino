//Libreria para conectar la placa ESP a la red Wifi
#include <ESP8266WiFi.h>
//Una biblioteca cliente para mensajería MQTT.Esta biblioteca le permite enviar y recibir mensajes MQTT. 
#include <PubSubClient.h>
//Libreria para manejar el sensor DHT
#include <DHT.h>
//Pines usados 
#define TMP A0 //Para el Sensor de Humeda del Suelo
#define DHTPIN D1 // Para el DHT11
#define LED_RED D6 //RED :D6
#define LED_BLUE D8 //BLUE:D8
#define LED_GREEN D7 //GREEN:D7
#define relay D2 //Relay:D2
#define PinTrig D4 //Sensor ultrasonido:Pin Trigger ->2
#define PinEcho D5 //Sensor ultrasonido:Pin Echo ->14

//const int PinTrig = 2;  //D4
//const int PinEcho = 14 ;  //D5


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
#define tipoautomatico false
#define tipomanual true

// Datos de la red a conectarse
const char* ssid = "Wifi leyco";
const char* password = "18200069";
const char* mqtt_server = "192.168.1.38";//IP del servidor,que en este caso es local

//Definimos el tipo de sensor DHT
#define DHTTYPE DHT11
//Declaracion sensor DHT11
DHT dht(DHTPIN, DHTTYPE);
//Declaracion cliente Wifi
WiFiClient espClient;
//Declaracion cliente MQTT
PubSubClient client(espClient);


//Variables globales
int humedad_suelo;//Almacena el valor de humedad_suelo
float temp;//Almacena el valor de temperatura
float humedad;//Almacena el valor de humedad

//true:Activa la bomba cuando sea tipo riego automatico
//false:Desactiva la bomba  cuando sea tipo riego automatico
//Se obtiene el valor en la funcion determinarEstado()
bool riego_automatico;

//true:Activa la bomba cuando sea tipo riego manual
//false:Desactiva la bomba cuando sea tipo riego manual
//Se obtiene el valor desde el dashboard,suscribiendose al topico ../riegomanual
bool riego_manual=false;


//Variables que almacenaran el estado en el que se encuentre la lectura correspondiente
int estado_humedad;
int estado_temperatura;
int estado_humedadsuelo;

unsigned long lastMsg = 0;

//Variable para determinar que tipo de riego realizar:Manual o Automatico
//Se suscribe al topico .../tiporiego para saberlo
float tiporiego=false; //De manera predeterminada sera automatico
/*
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;
*/

const String topicTipoRiego ="casa/jardin/tiporiego";
const String topicRiegoManual ="casa/jardin/riegomanual";

//Para el sensor ultrasonido---------------------
// Constante velocidad sonido en cm/s
const float VelSon = 34000.0;
 
// Número de muestras
const int numLecturas = 25;//Cantidad de lecuras que realiza el ultrasonido hasta arrojar una media
 
// Distancia a los 100 ml y vacío
//Esto variara segun el recipiente ,se hicieron estas mediciones para determinar su valor
const float distancia100 = 1.38; //Altura del liquido a 100ml
const float distanciaVacio = 11.21; //Altura desde el ultrasonido hasta el fondo del recipiente cuando esta vacio


float lecturas[numLecturas]; // Array para almacenar lecturas
int lecturaActual = 0; // Lectura por la que vamos
float total = 0; // Total de las que llevamos
float media = 0; // Media de las medidas
bool primeraMedia = false; // Para saber que ya hemos calculado por lo menos una media


float distanciaLleno ; //Almacena la distancia del recipiente que esta con liquido
float cantidadLiquido ; //Almacena la cantidad del liquido en el recipiente
int porcentaje ; //Almacena el procentaje de liquido en el recipiente

//--------------------------------------------------------FUNCIONES------------------------------------------------------------------------------------------
//Funcion para realizar la lectura de la humedad-temperatura-humedadsuelo
void lecturaValores(){
  // Leemos la humedad del suel0 y realizamos un mapeo para transformar los valores a un rango de 0 a 100 
    humedad_suelo = map(analogRead(TMP), 0, 1023, 100, 0);
   // Leemos la humedad relativa
    humedad = dht.readHumidity();
   // Leemos la temperatura en grados centígrados (por defecto)
    temp = dht.readTemperature();
}
//Funcion para mostrar por monitorSerial los valores leidos de temperatura-humedad-humedadSuelo
void MostrarValores(){
    Serial.print("Humedad: ");
    Serial.print(humedad);
    Serial.print("% ");
    Serial.print("Temperatura: ");
    Serial.print(temp);
    Serial.print(" *C ");
    Serial.print("Humedad del Suelo: ");
    Serial.print(humedad_suelo);
    Serial.println("% ");
}

void inicializarArray(){
   for (int i = 0; i < numLecturas; i++)
  {
    lecturas[i] = 0;
  }
}

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
  String topic_=topic;
  for (int i = 0; i < length; i++) {
    //Concatena los caracteres en una variable cadena
    payload_+=(char)payload[i]; 
  }

  //Si el mesaje se recibe del topic ...
  if(topic_==topicTipoRiego){
     if(payload_ == "true"){
      tiporiego=true; 
     }
     else{
      tiporiego=false;
     }
  }
  else if(topic_ == topicRiegoManual){
      if(payload_ == "true"){
      riego_manual=true; 
     }
     else{
      riego_manual=false;
     }
  
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
        client.subscribe(topicTipoRiego.c_str()); //Suscribiendose para recibir que tipo de riego desea
        client.subscribe(topicRiegoManual.c_str()); //Suscribiendose para determinar si riega o no manualmente
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
void hallarestado_Temperatura(){
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
void hallarestado_Humedad(){
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
void hallarestado_HumedadSuelo(){
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

//Para determinar el estado de los valores temperatura-humedad-humedadsuelo y encender el led segun los valores
void determinarEstado(){
  //Primero determinamos los estados de las variables
  hallarestado_Temperatura();
  hallarestado_Humedad();
  hallarestado_HumedadSuelo();
  if(estado_humedad==seco && estado_temperatura==frio && estado_humedadsuelo==sueloseco ){
    color(9,224,247);
    //Riego
    riego_automatico=true;
  }
  else if(estado_humedad==normal && estado_temperatura==frio && estado_humedadsuelo==sueloseco){
    color(255,192,0);
    //Riego
    riego_automatico=true;
  }  
  else if(estado_humedad==humedo && estado_temperatura==frio && estado_humedadsuelo==sueloseco){
    color(183,196,60);
    //Riego
    riego_automatico=true;
  }  
  else if(estado_humedad==seco && estado_temperatura==optimo && estado_humedadsuelo==sueloseco){
    color(252,4,187);
    //Riego
    riego_automatico=true;
  }  
  else if(estado_humedad==normal && estado_temperatura==optimo && estado_humedadsuelo==sueloseco){
    color(3,45,253);
    //Riego
    riego_automatico=true;
  }  
  else if(estado_humedad==humedo && estado_temperatura==optimo && estado_humedadsuelo==sueloseco){
    color(145,250,6);
    //Apagado
    riego_automatico=false;
  }  
  else if(estado_humedad==seco && estado_temperatura==caliente && estado_humedadsuelo==sueloseco){
    color(196,89,17);
    //Riego
    riego_automatico=true;
  }  
  else if(estado_humedad==normal && estado_temperatura==caliente && estado_humedadsuelo==sueloseco){
    color(112,48,160);
    //Riego
    riego_automatico=true;
  } 
  else if(estado_humedad==humedo && estado_temperatura==caliente && estado_humedadsuelo==sueloseco){
    color(255,255,0);
    //Apagado
    riego_automatico=false;
  }  
  else if(estado_humedad==seco && estado_temperatura==frio && estado_humedadsuelo==suelohumedo){
    color(255,102,153);
    //Riego
    riego_automatico=true;
  }  
  else if(estado_humedad==normal && estado_temperatura==frio && estado_humedadsuelo==suelohumedo){
    color(166,166,166);
    //Apagado
    riego_automatico=false;
  }  
  else if(estado_humedad==humedo && estado_temperatura==frio && estado_humedadsuelo==suelohumedo){
    color(255,152,1);
    //Apagado
    riego_automatico=false;
  }  
  else if(estado_humedad==seco && estado_temperatura==optimo && estado_humedadsuelo==suelohumedo){
    color(31,78,121);
    //Riego
    riego_automatico=true;
  }  
  else if(estado_humedad==normal && estado_temperatura==optimo && estado_humedadsuelo==suelohumedo){
    color(83,129,53);
    //Apagado
    riego_automatico=false;
  }
  else if(estado_humedad==humedo && estado_temperatura==optimo && estado_humedadsuelo==suelohumedo){
    color(195,64,61);
    //Apagado
    riego_automatico=false;
  }
   else if(estado_humedad==seco && estado_temperatura==caliente && estado_humedadsuelo==suelohumedo){
    color(221,35,75);
    //Riego
    riego_automatico=true;
  }  
   else if(estado_humedad==normal && estado_temperatura==caliente && estado_humedadsuelo==suelohumedo){
    color(222,29,227);
    //Apagado
    riego_automatico=false;
  }   
    else if(estado_humedad==humedo && estado_temperatura==caliente && estado_humedadsuelo==suelohumedo){
    color(104,246,128);
    //Riego
    riego_automatico=true;
  }     
    else if(estado_humedadsuelo==suelomuyhumedo){
     color(255,0,0);
    //Apagado
    riego_automatico=false;
  }         
}

// Funcion que inicia la secuencia del Trigger para comenzar a medir distancia
void iniciarTrigger()
{
  // Ponemos el Triiger en estado bajo y esperamos 2 ms
  digitalWrite(PinTrig, LOW);
  delayMicroseconds(2);
 
  // Ponemos el pin Trigger a estado alto y esperamos 10 ms
  digitalWrite(PinTrig, HIGH);
  delayMicroseconds(10);
 
  // Comenzamos poniendo el pin Trigger en estado bajo
  digitalWrite(PinTrig, LOW);
}

void MostrarUltrasonido(){
   // Solo mostramos si hemos calculado por lo menos una media
  if (primeraMedia)
  {
    distanciaLleno = distanciaVacio - media;
    cantidadLiquido = distanciaLleno * 100 / distancia100;
    porcentaje = (int) (distanciaLleno * 100 / distanciaVacio);
    Serial.print(media);
    Serial.println(" cm");
 
    Serial.print(cantidadLiquido);
    Serial.println(" ml");

    Serial.print(porcentaje);
    Serial.println(" %");
  }
}

void lecturaUltrasonido(){
  // Eliminamos la última medida
  total = total - lecturas[lecturaActual];
 
  iniciarTrigger();
  
  // La función pulseIn obtiene el tiempo que tarda en cambiar entre estados, en este caso a HIGH
  unsigned long tiempo = pulseIn(PinEcho, HIGH);
 
  // Obtenemos la distancia en cm, hay que convertir el tiempo en segudos ya que está en microsegundos
  // por eso se multiplica por 0.000001
  float distancia = tiempo * 0.000001 * VelSon / 2.0;
  
  // Almacenamos la distancia en el array
  lecturas[lecturaActual] = distancia;
 
  // Añadimos la lectura al total
  total = total + lecturas[lecturaActual];
 
  // Avanzamos a la siguiente posición del array
  lecturaActual = lecturaActual + 1;
 
  // Calculamos la media
  media = total / numLecturas;

  delay(200);
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
  // Ponemos el pin Trig en modo salida
  pinMode(PinTrig, OUTPUT);
  // Ponemos el pin Echo en modo entrada
  pinMode(PinEcho, INPUT);
  //Inicializar array de lectura de ultrasonido
  inicializarArray();
  Serial.begin(115200);
  //Conectadose a la red Wifi
  setup_wifi();
  //Establece los parametros del broker : direccion y puerto
  client.setServer(mqtt_server, 1883);
  //Establece la función de devolución de llamada de mensajes(callback).
    //callback: es un puntero a la funcion de callback,la cual se llama cuando llega un mensaje para la suscripcion hecha por el cliente
  client.setCallback(callback);
  //Empieza apagado el led rgb
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
    
     //EMPEZAMOS :D .....
    //Iniciamos con la lectura del sensor ultrasonido el cual sera una media para tener mas precision
    while(lecturaActual<numLecturas){
       lecturaUltrasonido();
    }
    // Comprobamos si hemos llegado al final del array de lectura
    if (lecturaActual>= numLecturas)
    {
      primeraMedia = true;
      lecturaActual = 0;
    }

    //Luego realizamos la lectura de valores del DHT11 y el Higrometro
    lecturaValores();
    if (isnan(humedad) || isnan(temp) ) {
        Serial.println("Error obteniendo los datos del sensor DHT11");
        return;
    }

    //SI TODO ESTA OK ,MOSTRAMOS LOS VALORES LEIDOS
     //Mostamos los valores de lectura del ultrasonido
     MostrarUltrasonido();
    //Mosramos los valores de lectura del DHT11 e Higrometro
     MostrarValores();
     
  //millis():Devuelve el número de milisegundos transcurridos desde que la placa Arduino comenzó a ejecutar el programa actual.
  unsigned long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
   client.publish("casa/jardin/temperatura", String(temp).c_str());
   delay(100);
   client.publish("casa/jardin/humedad", String(humedad).c_str());
   delay(100);
   client.publish("casa/jardin/humedadsuelo", String(humedad_suelo).c_str());
   delay(100);
   client.publish("casa/jardin/cantidadLiquido", String(cantidadLiquido).c_str());
   delay(100);
   client.publish("casa/jardin/porcentajeLiquido", String(porcentaje).c_str());
  }

  delay(2000);
  //Encendemos el led rgb segun el valor de los estados
  determinarEstado();

  //Empezamos el riego
  //Si el riego es manual
  if(tiporiego){
    Serial.println("Tipo de Riego:Manual");
    //Comprobamos si el riego manual es true o false
    if(riego_manual){
      activar_riego();
    }else{
      apagar_riego();
    }
  }
  //Si el riego es automatico
  else{
    Serial.println("Tipo de Riego:Automatico");
    //
     if(riego_automatico){
      activar_riego();
    }else{
      apagar_riego();
    }
  }
  
  delay(3000);
}
