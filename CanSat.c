/* koder 
-> program for måling, lagring og overføring av data fra ballong.
---------------------------------------------------------
-> Bruker analoge I/O pin 0..5
-> Analog 0,1,2,3 til sensorer.  4 og 5 til Seriell GPS
-> SD-kort på digitale pinner 10(CS),11(MOSI),12(MISO) og 13(CLK)
----------------------------------------------------------
*/
#include <SoftwareSerial.h> //trengs til comm mot GPS
#include <SD.h> // rutiner til minnekortet
#include <TinyGPS.h> //rutiner for å lese GPS-data
// info for SD.kort
long nexttime;
const int SD_Select = 10; //chipselect for minnebrikken
String melding = ""; //lager til meldingen som skal lagres/sendes
char filnavn[13]="data.csv"; 
//********************************************************
// info for GPS'en 
float breddegrad,lengdegrad,hoyde,hastighet; // til info fra GPS'en
unsigned long alder,datoen,klokken;
int satellitter;
TinyGPS gps; // defineren forekomst av tinygps
SoftwareSerial nss(A4,A5); // rx og tx pinne mot GPS'en
SoftwareSerial radio(2,3); // rx og tx pinne mot radioen
static void gpsdump(TinyGPS &gps);
static bool feedgps();
// har kanhende ikke bruk for alle disse. feks:
// 'alder', 'satellitter','datoen'

void setup()
{ // alle pinner fom 2 tom 10 ut-pinner
//  pinMode(4,OUTPUT);
//  digitalWrite(4,LOW); // start senderen
  nss.begin(4800); //klargjør seriakomm med GPS
  Serial.begin(9600);
  radio.begin(9600);
  if (!SD.begin(SD_Select)) // komm med SD-kort
  { // Serial.println("sd-kort-problem");
    return;
  }  
  File datafil = SD.open(filnavn, FILE_WRITE);
  datafil.println("-----");
  datafil.println("Temp,Pressure,RH,UTC,North,East,Alt,Speed");
  datafil.close();
}
/************************************************************************************************/
static bool feedgps()
{
  nss.listen(); delay(100);
  while (nss.available())
  {if (gps.encode(nss.read())) return true;}
  return false;
}  
/************************************************************************************************/
void loop()
{
  melding="";
  // gjør 3 målinger (analogpin 0 til 3)og lagre dem i meldingen
  // vi måler temperatur, fuktighet og lufttrykk
  // termometer er NTC, HIH4000, MPX5100
  for (int analogPin = 0; analogPin < 3; analogPin++) 
  { int sensor = analogRead(analogPin);
    melding += sensor;  
    melding +=',';
  } 
  // nu ligger det tre verdier her. eks "xxx,yyy,zzz,"
  // les data fra GPS'en og legg den til i meldingen
  // vi trenger klokkeslett,breddegrad,lengddegrad,høyde og antallet satellitter
  bool newdata = false;
  unsigned long start = millis();  // 'gi GPS'en 'mat' i ett sekund
  while (millis() - start < 1000)
   { if (feedgps())  newdata=true; } //mating ferdig
  satellitter=(gps.satellites()); // antall satellitter
  hoyde=(gps.f_altitude()); 
  hastighet=gps.f_speed_kmph(); //hastighet i kilometer/time
  gps.get_datetime(&datoen,&klokken,&alder); // trenger tiden
  gps.f_get_position(&breddegrad, &lengdegrad, &alder);
  feedgps();
  melding+=klokken/100;  melding+=','; //kun hele sekunder
  //deimaltall må 'spesialbehandles'
  int helegrader=int(lengdegrad); // hele grader
  melding +=helegrader;  melding+='.';
  long minutter=1e6*(lengdegrad-helegrader);
  melding +=minutter;  melding+=','; 
  helegrader=int(breddegrad); // hele grader
  melding +=helegrader;  melding+='.';
  minutter=1e6*(breddegrad-helegrader);
  melding +=minutter;  melding+=',';
  melding +=int(hoyde);  melding+=',';
  melding +=int(hastighet); 
  File datafil = SD.open(filnavn, FILE_WRITE);
  datafil.println(melding);
  datafil.close();
  radio.listen(); delay(100);
  radio.println(melding);
  Serial.println(melding);
  while(millis()<nexttime) {} // vent til ett sekund er gått
  nexttime+=1000;  
}
