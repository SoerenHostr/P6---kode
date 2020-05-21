#include <Servo_ESP32.h>                            //Library til at styre servomotoren med ESP32
#include <math.h>                                   //Library til matematiske funktioner

static const int servoPin = 2;                      //Sætter servoPin til digital pin 2
Servo_ESP32 servo1;                                 //Opretter servo objekt, som hedder servo1
const int pi = 3.1415926535897932384626433832795;   //Konstanten PI

int angleMax = 25;                                  //Maksimal vinkel på servomotoren

const int sample_frekvens = 300;                    //Sætter sampelfrekvensen i Hz
const int sample_tid = 1000000 / sample_frekvens;   //Timerens længde

double maaling = 0;                                 //Input-værdien fra sensoren
double mV = 0;                                      //Sensorværdi angivet i mV
double F_sensor;                                    //Kraften registreret af sensoren

double F_input = 0;                                 //Den udregende kraft til servomotoren
int V_aktuator = 0;                                 //Servomotorens vinkel, rundes af til nærmeste heltal med integer

int potentiometer = 0;                              //Input fra potentiometeret

hw_timer_t * timer = NULL;                            //Indikerer starttid
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED; //Indikerer sluttid

//Interrupt timer funktion
void IRAM_ATTR onTimer()
{
  portENTER_CRITICAL_ISR(&timerMux);                  //Må ikke afbrydes herfra
  maaling = analogRead(36);                           //Tager målinger af sensorens output
  portEXIT_CRITICAL_ISR(&timerMux);                   //Må ikke afbrydes, stopper
}


//Funktion til beregning af kraften registreret af sensoren
void kraft()
{
  mV = maaling * 0.805;                                 // Omregner sensordata fra bit-niveauer til mV
  F_sensor = 0.001445 * exp(2.684 * pow(10, -3) * mV);  // Finder kraft i N for sensor ved brug af  ekspotentielt fit
}

//Funktion til encoding af signalet
void coding() 
{
  if (potentiometer <= 1800)                            //Mode 1
  {
    F_input = (0.8893) * F_sensor;
  }
  else if (potentiometer > 1800)                        //Mode 2
  {
    F_input = 3 * F_sensor;
  }
}

//Funktion til beregnign af feedback leveret af servomotoren
void aktuator()
{
  V_aktuator = 2.61 * exp(0.259 * F_input);          //Omregning fra kraft [N] til antal grader ved brug af ekspotentielt fit

  if (V_aktuator > angleMax)                         //Sikrer at der ikke gives en større vinkel end det servomotoren maksimalt kan levere af kraft
  {                      
    V_aktuator = angleMax;
  }
  servo1.write(V_aktuator);                          //Den beregnede vinkel "sendes" til servomotoren
}

void setup()
{
  Serial.begin(115200);                              //Baud rate for seriel datatransmission
  pinMode(36, INPUT);                                //Konfigurer den specificeret pin til et input
  pinMode(35, INPUT);                                //Konfigurer den specificeret pin til et input
  pinMode(25, INPUT);                                //Konfigurer den specificeret pin til et input
  dacWrite(25, 0);                                   //Sætter højtaleren på M5Stacken til at være lydløs - forhindre auditorisk støj

  servo1.attach(servoPin);                           //Tilknytter den valgte pin(servoPin) til servo objekt(servo1)

  timer = timerBegin(0, 80, true);              //Timer 0 (der er 4 ialt). Prescaler på 80 -> 12.5 nS*80 = 1 mikro sekund, true betyder, at vi tæller op med et mikrosekund.
  timerAttachInterrupt(timer, &onTimer, true);  //Sætter variablen timer til funktionen onTimer, true betyder edge triggering
  timerAlarmWrite(timer, sample_tid, true);     //Sætter timeren til at tælle til sample_tid (3333,33) -> 300 Hz, true betyder enable autoreload (tæller forfra) tal i mikrosenkunder den skal tælle til før der kommer interrupt
  timerAlarmEnable(timer);                      //Starter timer
}


void loop()
{
  potentiometer = analogRead(35) * 0.805;            //Aflæser potentiometerværdi
  kraft();                                           //Kalder funktion for beregning af kraft fra sensoren
  coding();                                          //Kalder funktion for encoding af signalet
  aktuator();                                        //Kalder funktion for beregning af vinkel til servomotor
}
