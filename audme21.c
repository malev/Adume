/*
 * PROGRAMA ADUME 2.1
 * Tareas:
 *
 *  uC = dsPIC30F4013
 */

// Variables globales
int Samples[256] absolute 0x0C00;
int Samples2[128]; // La uso en Encontrar Fc para guardar los resultados
                   // de la fft ordenados y de ahí buscar el máximo
int impulsos[25];  // como máximo 2Hz de Fc, tomando 2 mínimos por onda
                   // tenemos 20 impulsos. Más 5 por si las dudas
int indices[10];
short flag = 0;
unsigned Adrr = 0;

// Prototipos de Interfaz
void MostrarDatos(short modo);

// Prototipos temporales
void Segundos(unsigned cantidad);

// Prototipos de manejo de señal
unsigned ReadAdc();
void CargarDatos(short f);
short EncontrarFc();
float Fract2Float(int input);

// Prototipos del algoritmo
void Derivando(void);
int tipo(short ini, short fin, short T);
short Promediar(short T);
short encontrar_pico(short inicio, short T);
void CalculoIndices(short dir, short freq);

// Prototipos de manejo de memoria
void GuardarDatos(short modo);
void Formatear (void);
void REeprom(char Adrr1, char Adrr0);
void WEeprom(char Adrr1, char Adrr0);
char LeerByte(char Adrr1, char Adrr0);
void EscribirByte(char Adrr1, char Adrr0, unsigned int dato);
short IntToShort(int valor16);

//Prototipos de manejo del uart
void TextToUart(unsigned char *m);
void EnviarByte(unsigned dato);
void NewLine();
void EnviarBloque(char *m);
void EnviarDatos(void);

// Prototipos de configuraciones
void InitAdc();
void InitLCD();
void InitConfig();

// Solo para probar!!!!
void Generar_Senal(short modo){
  unsigned i=0;
  unsigned arg;
  unsigned temp;

  while(i<256){
    arg = 360 * 0.02 * i;
    temp = 1.1 * SinE3(1.5 * arg) + 0.95 * SinE3(3 * arg) + 0.2 * SinE3(4.5 * arg);
    if (modo == 2) temp = 1.1 * SinE3(1.9 * arg) + 0.95 * SinE3(3 * arg) + 0.2 * SinE3(4.5 * arg);
    Samples[i++] = temp;  //Parte real
    if (modo == 0) Samples[i++] = 0;             //Parte imaginaria
  }
}//~

void main(void){
  short i, T, A=0, freq, opt;
  char txt3[3], txt6[6];

  InitConfig();

 Inicio:
  // Rutinas de verificación
  LCD_Custom_Out(1,1,"Iniciando");
  LCD_Custom_Out(2,1,"Audme 2.1");
  delay_ms(2000);
  LCD_Custom_Out(1,1,"Iniciar       ");
  LCD_Custom_Out(2,1,"Presione Enter");

  // Presentando Menu
  opt = 0;
  while(1){
    if(Button(&PORTD, 0, 100, 0)){
      if(opt == 0) goto Empieza;
      if(opt == 1) Formatear();
      if(opt == 2) EnviarDatos();
      delay_ms(100);
      goto Inicio;
    } else if (Button(&PORTD, 1, 100,0)){
      opt++;
      if(opt == 3) opt = 0;
      if (opt == 0) LCD_Custom_Out(1,1,"Iniciar       ");
      if (opt == 1) LCD_Custom_Out(1,1,"Formatear     ");
      if (opt == 2) LCD_Custom_Out(1,1,"Enviar Datos  ");
      delay_ms(100);
    }
  }

 Empieza:
  // Revisa que los electrodos esten conectados
  // Esto se puede hacer comprobando alguna entrada del
  // PORTB
  // inicia el proceso
  LCD_Custom_Out(1,1,"Cargando datos");
  while(1){
    CargarDatos(0);
    freq = EncontrarFc();
    if(freq > 5 && freq < 20) break;
    LCD_Custom_Cmd(LCD_CLEAR);
    LCD_Custom_Out(1,1,"No se encuentra la");
    LCD_Custom_Out(2,1,"senal.");
    Segundos(5);
    flag++;
    if(flag == 5){
      LCD_Custom_Cmd(LCD_CLEAR);
      LCD_Custom_Out(1,1,"Error 69");
      Segundos(5);
      goto Inicio;
    }
    LCD_Custom_Cmd(LCD_CLEAR);
    LCD_Custom_Out(1,1,"Buscando senal");
  }
  // Buscar un espacio en la memoria para guardar
  LCD_Custom_Cmd(LCD_CLEAR);
  GuardarDatos(99);
  // Calculo de índices de CONTROL!
  CargarDatos(1);
  CalculoIndices(0, freq);
  //LCD_Custom_Out(1,1,"NdR");
  delay_ms(500);
  GuardarDatos(0);
  // Guardo la fc
  delay_ms(200);
  EscribirByte(Adrr,47,freq);

  //Enviar datos por uart2
  NewLine();
  TextToUart("Datos nuevos");
  NewLine();
  for(i=0;i<50;i++){
    IntToStr(Samples2[i], txt6);
    TextToUart(txt6);
    NewLine();
    delay_ms(5);
  }
  TextToUart("-- done --");
  NewLine;


  // Falta activar sistema de bomba
  // Espera 3minutos
  LCD_Custom_Out(2,1,"3...");
  Segundos(5);
  LCD_Custom_Out(2,1,"2...");
  Segundos(5);
  LCD_Custom_Out(2,1,"1...");
  Segundos(5);

  // Desactiva sistema de bomba

  LCD_Custom_Out(2,1,"Post Oclusion");
  // 30segs
  Segundos(25);
  CargarDatos(2);
  CalculoIndices(2, freq);
  GuardarDatos(2);

  Lcd_Custom_Out(2,1,"Procesando      ");

  // 60segs
  Segundos(25);
  CargarDatos(1);
  CalculoIndices(4, freq);
  GuardarDatos(4);

  // 90segs
  Segundos(25);
  CargarDatos(2);
  CalculoIndices(6, freq);
  GuardarDatos(6);

  // 120segs
  Segundos(25);
  CargarDatos(1);
  CalculoIndices(8, freq);
  GuardarDatos(8);

  Lcd_Custom_Cmd(LCD_CLEAR);
  Lcd_Custom_Out(1,1,"Finalizado");

  // Busco los indices mas grandes
  // Los presento
  i=0;
  flag = freq;
  MostrarDatos(0);
  while(1){
    if(Button(&PORTD, 0, 100,0)){
      i++;
      if(i == 6) i = 0;
      MostrarDatos(i);
      delay_ms(200);
    } else if(Button(&PORTD, 1, 100,0)){
      LCD_Custom_Cmd(LCD_CLEAR);
      LCD_Custom_Out(1,1,"Salir?");
      LCD_Custom_Out(2,1,"Si -> Start");
      while(1){
        if(Button(&PORTD, 0, 100, 0)){
          Delay_ms(200);
          goto Inicio;
        }else if (Button(&PORTD, 1, 100,0)){
          LCD_Custom_Cmd(LCD_CLEAR);
          MostrarDatos(0);
          break;
        }
      }
    }
  }
  goto Inicio;

}//~

void MostrarDatos(short modo){

  char txt6[6];
  char txt4[4];

  LCD_Custom_Cmd(LCD_CLEAR);
  if(modo == 0){
    IntToStr(indices[0], txt6);
    LCD_Custom_Out(1,1,"RIctrl: ");
    LCD_Custom_Out(1,8,txt6);
    IntToStr(indices[1], txt6);
    LCD_Custom_Out(2,1,"dTctrl: ");
    LCD_Custom_Out(2,8,txt6);
  }else if (modo == 5){
    IntToStr(Adrr, txt6);
    ShortToStr(flag, txt4);
    LCD_Custom_Cmd(LCD_CLEAR);
    Lcd_Custom_Out(1,1,"NdReg: ");
    Lcd_Custom_Out(1,10,txt6);
    Lcd_Custom_Out(2,1,"freq: ");
    Lcd_Custom_Out(2,8,txt4);
  }else if ((modo > 0) && (modo < 5)){
    IntToStr(indices[modo], txt6);
    ShortToStr(modo, txt4);
    LCD_Custom_Out(1,1,"RI    :");
    LCD_Custom_Out(1,3,txt4);
    LCD_Custom_Out(1,10,txt6);
    IntToStr(indices[modo + 1], txt6);
    ShortToStr(modo, txt4);
    LCD_Custom_Out(2,1,"dT    :");
    LCD_Custom_Out(2,3,txt4);
    LCD_Custom_Out(2,10,txt6);
  }
}//~

void Segundos(unsigned cantidad){
  unsigned short i;
  for(i=0; i < cantidad; i++) delay_ms(1000);
}//~

unsigned ReadAdc() {
  ADCON1.F1 = 1;                    // Start AD conversion
  while (ADCON1.F0 == 0)            // Wait for ADC to finish
    asm nop;
  return ADCBUF0;                   // Get ADC value
}//~

void CargarDatos(short f){
  //modo 1 => comun
  //para simular!
  Generar_Senal(f);
  /*unsigned i = 0;
  while(i<256){
    Samples[i++] = ReadAdc();
    if(f == 0) Samples[i++] = 0;
    delay_ms(20);
  }*/
}//~

short EncontrarFc(){
  // Recogiendo muestras para FFT
  float    Rer, Imr, tmpR;
  char index = 0;
  unsigned i, j;

  FFT(7, TwiddleCoeff_128, Samples);
  BitReverseComplex(7, Samples);

  j = 0;
  for (i=0;i<256;i++){
    Rer = Fract2Float(Samples[i]);
    Imr = Fract2Float(Samples[++i]);
    Samples2[j++] = sqrt(Rer * Rer + Imr * Imr)*100;
  }
  j = Samples2[0]; //Busco la frec fundamental
  for(i=1;i<128;i++){
    if (Samples2[i] > j){
      j = Samples2[i];
      index = i;
    }
  }
  index = index * 1.9;
  return index;
}//~

float Fract2Float(int input) {
// posta que ya no es esto!
// es para que al elevarlo al cuadrado no se vaya de rango!
//-------------- Auxiliary function for converting 1.15 radix point to
//               IEEE floating point variable (needed for sqrt).
   if (input < 0) input = - input;
   return (input / 32768.);
}//~

void Derivando(void){
/*
 * Si bien se llama derivando
 * solo busca los cambios de signo de la derivada
 * de negativo a positivo.
 * Guarda en el vector impulsos
 * todas las direcciones de los mínimos relativos
 */
    unsigned i;
    unsigned j=0;
    int temp1, temp2;

    //Inicializando el vector impulsos
    for(i=0;i<25;i++) impulsos[i] = 0;

    temp1 = samples[1] - samples[0];
    for(i=1;i<254;i++){   //
	temp2 = Samples[i+1] - Samples[i];
	if (temp1 < 0 && temp2 > 0) impulsos[j++] = i+1;  // habría que probar sin el  +1!!!
	temp1 = temp2;
    }
}//~

int tipo(short ini, short fin, short T){
/* Determina la distancia entre los puntos inicio y fin
 * Y en relacion al periodo de la señal encuentra devuelve
 * 2 si la distancia es aprox a T
 * 3 si la distancia está entre T y 2T
 * 4 si la distancia es aprox 2T
 * Esto permite estimar de que tipo de onda se trata.
 *
 * name: Tipo
 * @param - inicio, fin y periodo (T)
 * @return - entero
 *
 */
float tol=0.15;
unsigned dif;
dif = fin - ini;
if (dif > 2*(T + T*tol)) return 0;
if(dif >= (T-T*tol)){
	if (dif <= (T+T*tol)) return 2;
	if (dif >= 2*(T-T*tol)) return 4;
	return 3;
	}
return 0;
}//~

short Promediar(short T){
/*
 * Aqui está la magia
 * Esta funcion determina que clase de onda está entrando en base
 * a la función tipo. Efectua los corrimientos necesarios para
 * asegurarse caer en el inicio de una onda.
 * luego guarda todo su contenido en un vector.
 * Realiza esto 5 veces por lo menos, a no se que se agote
 * el contenido del vector.
 * luego hace el promedio
 **/

	short j=0, cant = 0;
	short A=0, num1;
        unsigned i;

	for(i=0;i<128;i++) samples2[i] = 0; //inicio el latido promedio

	while (impulsos[cant] != 0) cant++;

	i = 1;  // también podría ser i=0
	while(A <= 4 || i > cant - 3){
		num1 = tipo(impulsos[i], impulsos[i+2], T);
		if(num1 == 2){
			if(samples[impulsos[i]] > samples[impulsos[i+1]]) i++;
			for(j=0;j<T;j++) samples2[j] = samples2[j] + samples[impulsos[i] + j];
			//printf("num1: %d, inicia en %d, el prox sera %d\n", num1, impulsos[i], impulsos[i+1]);
			A++; i++;
		} else if (num1 == 3) {
			i++;
			for(j=0;j<T;j++) samples2[j] = samples2[j] + samples[impulsos[i] + j];
			//printf("num1: %d, inicia en %d, el prox sera %d\n", num1, impulsos[i], impulsos[i+1]);
			A++; i++;
		} else if (num1 == 4) {
			for(j=0;j<T;j++) samples2[j] = samples2[j] + samples[impulsos[i] + j];
			//printf("num1: %d, inicia en %d, el prox sera %d\n", num1, impulsos[i], impulsos[i+1]);
			i++; A++;
		} else {
			i++;
		}

	}
	for(i=0;i<T;i++) samples2[i] = samples2[i] /A;
	return A;
}//~

short encontrar_pico(short inicio, short T){
/* Esta es cualkiera
 * toma un vector y busca un maximos, a partir
 * del punto inicio.
 *
 * name: encontrar_pico
 * @param - un latido, la dirección de inicio y el periodo.
 * @return - un entero que especifica la dirección del máximo
 *
 */
        int i = 0;
        for (i = inicio + 1;i < T-1;i++){
		if((samples2[i] > samples2[i-1]) && (samples2[i] > samples2[i+1])) return i;
	}
	return 0;
}//~

void CalculoIndices(short dir, short freq){

  short k, T, A;
  unsigned dir_a, dir_b=0, deltaT;
  int ampli_a=0, ampli_b=0, ri;
  float tempa, tempb;

  for(k=0;k<128;k++) Samples2[k] = 0;
  Derivando();
  T = 500/freq;
  A = Promediar(T);
  dir_a = encontrar_pico(0,T);
  dir_b = encontrar_pico(dir_a, T);
  if (dir_b > 0){
      Ampli_a = Samples2[dir_a] - Samples2[T-1];
      Ampli_b = Samples2[dir_b] - Samples2[T-1];
      tempa = Fract2Float(Ampli_a);
      tempb = Fract2Float(Ampli_b);
      ri =  100 * tempb / tempa;
      deltaT = dir_b - dir_a;
      indices[dir++] = ri;
      indices[dir++] = deltaT;
  } else {
      Ampli_a = Samples2[dir_a] - Samples2[T-1];
      indices[dir++] = Ampli_a;
      indices[dir++] = 0;
  }
}//~

void GuardarDatos(short modo){
  /* 9   -> Averigua la direccion
     5   -> Guarda datos e indices
    < 5  -> Guarda solo datos
   */
  char Adrr1 = 0, Adrr0 = 0;
  char txt6[6];
  unsigned i;
  short stemp = 0;

  if (modo == 99){
    Adrr = LeerByte(0x00,0x00);
    Adrr = Adrr + 1;
    delay_ms(100);
    //guarda la nueva direccion
    EscribirByte(0x00,0x00,Adrr);
    LCD_Custom_Out(1,1,"NdR: ");
    IntToStr(Adrr,txt6);
    LCD_Custom_Out(1,6,txt6);
    return;
  } else if(modo < 10){
    delay_ms(500);
    // Convierto a 8bits
    for(i=0;i<50;i++){
      stemp = IntToShort(Samples2[i]);
      samples2[i] = stemp;
    }
    WEeprom(Adrr, modo * 50);
    delay_ms(500);
    EscribirByte(Adrr,48,Indices[modo]);
    EscribirByte(Adrr,49,Indices[modo + 1]);
  }
  return;
}//~

void Formatear (void){
  short i;
  LCD_Custom_Out(2,1,"Confirma?   ");
  while(1){
    if(Button(&PORTD, 0, 100, 0)){
      Lcd_Custom_Cmd(LCD_CLEAR);
      Lcd_Custom_Out(1,1,"Formateando..");
      /*
       En los primeros 5 bytes esta guardada la información
       importante, perdiendo esto, ya ningún otro dato
       tiene sentido
       */
       for(i=0;i<50;i++) Samples2[i] = 0x0000;
       WEeprom(0x00, 0x00);
       delay_ms(2000);
       Lcd_Custom_Out(2,1,"Finalizado   ");
       Delay_ms(2000);
       break;
    }else if (Button(&PORTD, 1, 100,0)){
       Lcd_Custom_Out(1,1,"Cancelado  ");
       Delay_ms(2000);
       break;
    }
  }
  LCD_Custom_Cmd(LCD_CLEAR);
  LCD_Custom_Out(1,1,"Formatear     ");
  LCD_Custom_Out(2,1,"Presione Enter");
}//~

void WEeprom(char Adrr1, char Adrr0){

  char i;
  I2c_Start();
  I2c_Write(0xA2);            // Envia control, A2,A1,A0 (todos del chip)
                              // y un 0 -> W
  I2c_Write(Adrr1);          // Envia la direccion donde grabar
  I2c_Write(Adrr0);
  for(i=0;i<48;i++) I2c_Write(Samples2[i]);
  I2c_Stop();

}//~

void REeprom(char Adrr1, char Adrr0){

  char i;
  I2c_Start();
  I2c_Write(0xA2);               // send byte via I2C  (device address + W)

  I2c_Write(Adrr1);               // send byte (data address)
  I2c_Write(Adrr0);

  I2c_Restart();                 // issue I2C signal repeated start

  I2c_Write(0xA3);               // send byte (device address + R)

  for(i=0;i<49;i++) Samples2[i] = I2c_Read(0);
  Samples2[49] = I2c_Read(1);
  I2c_Stop();
}//~

unsigned char LeerByte(char Adrr1, char Adrr0){
  unsigned char temp1;
  I2c_Start();                   // issue I2C start signal
  I2c_Write(0xA2);               // send byte via I2C  (device address + W)
  I2c_Write(Adrr1);               // send byte (data address)
  I2c_Write(Adrr0);
  I2c_Restart();                 // issue I2C signal repeated start
  I2c_Write(0xA3);               // send byte (device address + R)
  temp1 = I2c_Read(1);           // Read the data (NO acknowledge)
  I2c_Stop();
  return temp1;
}//~

void EscribirByte(char Adrr1, char Adrr0, unsigned int dato){
    I2C_Start();
    I2C_Write(0xA2);    //RW = 0
    I2C_Write(Adrr1);
    I2C_Write(Adrr0);
    I2C_Write(dato);
    I2C_Stop();
}//~

short IntToShort(int valor16){
  float fTemp;
  short sTemp;
  if (valor16 == 0) return 0;
  if (valor16 > 0){
    fTemp = valor16 * 0.0668;
    sTemp = fTemp;
    return fTemp;
  } else if (valor16 < 0){
    fTemp = valor16 * 0.06736842;
    sTemp = fTemp;
    return fTemp;
  }
}//~

void TextToUart(unsigned char *m){
  unsigned char i=0;
  while(m[i]!=0){
    EnviarByte(m[i]);
    i++;
    }
}//~

void EnviarByte(unsigned dato){
  U2STA.F10 = 1;  // habilita el bit UTEN
  U2TXREG = dato;  // Carga los datos a enviar
  delay_ms(5);
}//~

void NewLine(){
  EnviarByte(0x0D); //Send carriage-return
  EnviarByte(0x0A); //Send line-feed
}//~

void EnviarBloque(char *m){
  unsigned i;
  char txt3[3];

  TextToUart(m);
  NewLine();
  TextToUart("----");
  NewLine();
  for(i=0;i<50;i++){
    ByteToStr(Samples2[i], txt3);
    TextToUart(txt3);
    NewLine();
  }
  TextToUart("----");
  NewLine();
}//~

void EnviarDatos(void){
  unsigned Addr, i;
  LCD_Custom_Out(1,1,"Enviando datos");
  Addr = LeerByte(0x00, 0x00);
  for(i=1;i<=Addr;i++){
    REeprom(i, 0x00);
    delay_ms(100);
    EnviarBloque("0x00");
    REeprom(i, 0x32);
    delay_ms(100);
    EnviarBloque("0x32");
    REeprom(i, 0x64);
    delay_ms(100);
    EnviarBloque("0x64");
    REeprom(i, 0x96);
    delay_ms(100);
    EnviarBloque("0x96");
    REeprom(i, 0xC8);
    delay_ms(100);
    EnviarBloque("0xC8");
  }
  LCD_Custom_Out(1,1,"Done.");
  delay_ms(1000);
  LCD_Custom_Cmd(LCD_Clear);
}

void InitAdc() {
  ADPCFG = 0xFFFE;    // Selecciona al RB2 como entrada analógica
  ADCSSL = 0;         // Selecciona el canal a ser explorado NO SCAN
  ADCHS =  2;
  ADCON2.F15 = 1;
  ADCON3 = 0x1F3F;    // sample time = 31 Tad.
  TRISB.F2 = 1;       // RB2 o AN2 es una entrada!
  ADCON1 = 0x80E0; // turn ADC ON, unsigned Integer resoult. Internal Conuter
                   // ends conversion
}//~

void InitLCD(){
  Lcd_Custom_Config(&PORTB, 5,4,3,2, &PORTD, 9, 3, 2);
  Lcd_Custom_Cmd(LCD_CURSOR_OFF);
  Lcd_Custom_Cmd(LCD_CLEAR);
}//~

void InitConfig(){
  TRISB = 0x01FF;    // TRIS -> 0: Salida
                     //      -> 1: Entrada
                     // RB0 a RB8 - > entradas
                     // RB9 a RB12 -> salidas
  TRISD = 0x0003;    // RD0 y RD1 son pulsadores de entrada
  InitAdc();
  InitLCD();
  Uart2_Init(9600);
  I2C_Init(10000);
  Twiddle_Factors_Init();
}//~