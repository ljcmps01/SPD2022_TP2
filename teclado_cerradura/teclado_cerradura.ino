/*
TP2 SPD 2022 Cerradura
Campos Alejo, Obregon Alex

Extras:
-Servo para simular pestillo de cerradura
-Tamaño dinamico de contraseña (respetando el maximo)
-Caracteres especiales de LCD    
*/

#include <Keypad.h>
#include <LiquidCrystal.h>
#include <Servo.h>

//tamaño del teclado matricial
#define FILAS 4
#define COLUMNAS 4

//cantidad maxima de caracteres de la contraseña
#define MAX_CARACTER 7

//pines de los botones
#define TEST 12
#define RST A4
#define SET A5

//pin del servo y angulos de bloqueo y apertura
#define SERVO 9
#define LOCK_ON 90
#define LOCK_OFF 0

//pines de led
#define LED_OK 10
#define LED_FAIL 11


//***********************SETUP KEYPAD/Botones******************************
//declaracion e inicializacion del teclado matricial
char teclado[FILAS][COLUMNAS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte columnasPins[COLUMNAS] = {3,2,1,0};
byte filasPins[FILAS] = {7,6,5,4};
Keypad keypad = Keypad( makeKeymap(teclado), filasPins, columnasPins, FILAS, COLUMNAS );

//variables de estado previo de los botones
int botonTESTAntes=0;
int botonRSTAntes=0;
int botonSETAntes=0;

//***********************FIN SETUP KEYPAD******************************

//***********************SETUP LCD******************************
//inicializacion del display LCD
const int rs = 13, en = 8, d4 = A0, d5 = A1, d6 = A2, d7 = A3;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//Caracter especial cerradura cerrada
byte lockOn[8]=
{
  B01110,
    B01010,
    B11111,
    B11111,
    B11011,
    B11011,
    B01110,
    B00000,
};
//Caracter especial cerradura 
byte lockOff[8]=
{
  B01110,
    B01010,
    B01000,
    B11111,
    B11111,
    B11011,
    B11011,
    B01110,
};

//***********************FIN SETUP LCD******************************


//***********************SETUP Servo******************************
Servo servoLock;

//***********************FIN SETUP Servo******************************

char contra[MAX_CARACTER + 1]= "2C2021";  //contra inicial
char contraIngresada[MAX_CARACTER + 1];   //contra ingresada por el usuario
int cont = 0;                             //contador de caracteres ingresados

unsigned long prevMillis=0;
bool flagPrincipal=0;
unsigned int duracion;


//paso una string de tamaño tam y lo inicializo en nulo
void initVector(char vec[], int tam);

//Pide al usuario ingresar ingresar una contraseña nueva hasta alcanzar el maximo de caracteres o hasta que se toque el boton de test 
void ingresoNuevaContra(char aContra[], int tam);

//Funcion de cambio de contraseña actual por la contraseña pasada como parametro
void asignarContra(char nueva[], char actual[], int tam);

//Imprime si la contraseña ingresada coincide con la contraseña del sistema
void imprimirComparacion(char contraI[], int tam);

//borrado del display
void limpiarPantalla();

//imprime el titulo pasado como parametro junto con un caracter especial de ser requerido
//Y acomoda el cursor para nuevos ingresos
void imprimirTitulo(char *titulo, int cChar);

//Funcion que corrobora si el si pasó el tiempo pasado como parametro en milisegundos
//Si pasó un intervalo mayor o igual a dicho tiempo retorna 1, caso contrario devuelve 0 
int intervalo(unsigned long *prev,unsigned int tiempo);

//Funcion que analiza el estado del boton y llama a la funcion de callback correspondiente
void estadoBoton(int *estadoPrevio,int *estadoActual, char* contra, int tam,void(*callback)(char*,int));

//funcion de callback del boton TEST
//Compara la contraseña ingresada e imprime el resultado y prepara el tiempo para intervalo()
//Y levanta la bandera principal
void funcionTEST(char contraI[],int tam);

//funcion de callback del boton RST
//Reinicializa el vector ingresado por el usuario (contra ingresada) y el resetea 
//el contador de caracteres a 0
void funcionRST(char contraI[],int tam);


//funcion de callback del boton SET
//Le pide al usuario una nueva contraseña 
void funcionSET(char contra[],int tam);

//***********************SETUP******************************
void setup(){
  pinMode(TEST, INPUT);
  pinMode(TEST, INPUT);
  pinMode(TEST, INPUT);
  
  pinMode(LED_OK, OUTPUT);  
  pinMode(LED_FAIL, OUTPUT);
  
  servoLock.attach(SERVO);
  servoLock.write(LOCK_ON);
  
  initVector(contraIngresada, MAX_CARACTER + 1);
  
  lcd.begin(16,2);
  lcd.createChar(7,lockOn);
  lcd.createChar(8,lockOff);  
  
  imprimirTitulo("Password ",7);  
}
//***********************FIN SETUP******************************

//***********************LOOP******************************
void loop(){
  char tecla = keypad.getKey();
  
  int botonTESTAhora = digitalRead(TEST);
  int botonRSTAhora = digitalRead(RST);
  int botonSETAhora = digitalRead(SET);
  
  if(tecla)
  {
    lcd.setCursor(0,1);
    contraIngresada[cont] = tecla;
    lcd.print(contraIngresada);
    cont++;
  }
  
  estadoBoton(&botonTESTAntes, &botonTESTAhora, contraIngresada, MAX_CARACTER, funcionTEST);
  estadoBoton(&botonRSTAntes, &botonRSTAhora, contraIngresada, MAX_CARACTER, funcionRST);
  estadoBoton(&botonSETAntes, &botonSETAhora, contra, MAX_CARACTER, funcionSET);
  
  if(cont == MAX_CARACTER)
  {
    imprimirComparacion(contraIngresada, MAX_CARACTER);
    prevMillis=millis();
    duracion = 2000;
    flagPrincipal=1;    
  }
  
  if(flagPrincipal)
  {
    if(intervalo(&prevMillis,duracion))
    {
      imprimirTitulo("Password ",7);
      flagPrincipal=0;
      digitalWrite(LED_OK,0);
      digitalWrite(LED_FAIL,0);
      servoLock.write(LOCK_ON);
    }
  }
  
  delay(5);
}

//***********************FIN LOOP******************************


void imprimirComparacion(char contraI[], int tam)
{
  limpiarPantalla();
  int comContra=strcmp(contraI,contra);
  if(comContra)
  {

    lcd.print("Password FAIL!");
    lcd.write(7); 

    digitalWrite(LED_FAIL,1);
  }
  else
  {

    lcd.print("Password OK!");
    lcd.write(8);
    digitalWrite(LED_OK,1);
    servoLock.write(LOCK_OFF);
  }
  lcd.setCursor(0,1);
  cont = 0;
  initVector(contraI, tam + 1);  
}

void initVector(char vec[], int tam)
{
  for(int i = 0; i < tam; i++)
  {
    vec[i] = '\0';
  }
}


void ingresoNuevaContra(char aContra[], int tam)
{
  char tecla = '\0';
  char nContra[tam + 1];
  int setFlag = 1;

  initVector(nContra, tam + 1);

  for(int i = 0; i < tam && setFlag; i++)
  {
    while(!tecla && setFlag)
    {
      tecla = keypad.getKey();
      
      if(digitalRead(TEST) && i>0)
      {
        setFlag = 0;
        botonTESTAntes=1;
      }
    }  

    nContra[i] = tecla;
    lcd.print(tecla);
    tecla = '\0';
  }
  lcd.setCursor(0,0);
  lcd.print("Contra guardada!");
  asignarContra(nContra, aContra, tam);
}

void asignarContra(char nueva[], char actual[], int tam)
{
  initVector(actual, tam + 1);

  for(int i = 0; i < tam; i++)
  {
    actual[i] = nueva[i];
  }
}


void limpiarPantalla()
{
  lcd.clear();
  lcd.setCursor(0,0);
}

void imprimirTitulo(char *titulo, int cChar)
{
  limpiarPantalla();
  lcd.print(titulo);
  if(cChar)
  {
    lcd.write(cChar);
  }
  lcd.setCursor(0,1);
}

int intervalo(unsigned long *prev,unsigned int tiempo)
{
  unsigned long actual=millis();
  if(actual-*prev>=tiempo)
  {
    *prev=actual;
    return 1;
  }
  return 0;
}
  
void estadoBoton(int *estadoPrevio,int *estadoActual, char* contra, int tam,void(*callback)(char*,int))
{
  if(*estadoPrevio == 0&& *estadoActual == 1)
  {
    callback(contra,tam);
  }
  
  *estadoPrevio = *estadoActual;
  return 0; 
}


void funcionTEST(char contraI[],int tam)
{
  imprimirTitulo("Test Pass",0);
    imprimirComparacion(contraI, tam);
    prevMillis=millis();
    duracion = 2000;
    flagPrincipal = 1;
}

void funcionRST(char contraI[],int tam)
{
  imprimirTitulo("Reset",0);
    initVector(contraI, tam);
    cont=0;
    prevMillis=millis();
    duracion = 2000;
    flagPrincipal = 1;
}

void funcionSET(char contra[],int tam)
{
  imprimirTitulo("Nueva contra",0);
    ingresoNuevaContra(contra, tam);
    prevMillis=millis();
    duracion = 1000;
    flagPrincipal = 1;
}
