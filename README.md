# HUMIT-o-METRE
PROVES_ registrador d'humitat i temperatura a l'entrada i a la sortida d'un deshidratador solar

// Hardware :

esp8266  
BME280  x2 

SPI 1.8" TFT color display    amb   modul SD 



// Pinout : 

/* 

IO index     ESP8266 pin                    FUNCIO HUITOMETRE

  0 [*]       GPIO   16   --->   pulsador

  1           GPIO    5   --->   I2C                      SCL  

  2           GPIO    4   --->   I2C                      SDA 

  3           GPIO    0   --->           TFT_retroLED

  4           GPIO    2   --->   SPI                                SD_CS  


  5           GPIO  14   --->   SPI      TFT_SCLK                   SD_SCK

  6           GPIO  12   --->   SPI      TFT_DC   (A0)              SD_MISO

  7           GPIO  13   --->   SPI      TFT_MOSI                   SD_MOSI

  8           GPIO  15   --->   SPI      TFT_CS                 


  9          GPIO    3   --->   RX

 10          GPIO    1   --->   TX

 11          GPIO    9   --->   ???

 12          GPIO  10   --->   ???


A0  ------------------------------------------>  POTENCIOMETRE / SELECTOR ROTATIU    
                        
*/


// Posibles millores :

//

//  nomes queda un pin digital lliure (D0=GPIO16), potser per un boto amb multifuncio ?

//    1 toque rapid encen o apaga l'iluminacio de la pantalla

//    1 toque prolongat (8 segons) reinicia l'humitometre


//  al pin A0 li podirem posar un potenciomete o selector rotatiu amb diferents resistències

//  que ens faci canviar les dades a la pantalla.

//  100ohms-->
//  200ohms-->
//  300ohms-->
//  400ohms-->
//  500ohms-->
//  ...

//
//  i amb combinació amb el pulsador possibilitats infinites..

//

//

// possibilitat d'afegir-hi un RTC per registrar el temps de les mesures.

// ¿ potser podriem preguntar la hora mitjançant el robot del telegram ?.. pero nomes si tenim connexió a internet, clar..


Anims, salut i sol !!
