/*
Created 2016
by AlexGyver
AlexGyver Home Labs Inc.
*/
char masschar[5];  //массив символов для перевода
String massstring;  //строка, в которую задаётся масса в режиме выбора массы
int set,setmass,rapidtime;
boolean initial,flagmass, flagmassset, rapidflag;  //флажки
int n=1;            //номер выстрела, начиная с 1
float velocity, energy;    //переменная для хранения скорости
float mass=0.00035;       //масса снаряда в килограммах
float dist=0.0845;       //расстояние между датчиками в метрах  
volatile unsigned long gap1, gap2;    //отметки времени прохождения пулей датчиков
unsigned long lastshot;

void setup() {
	Serial.begin(9600);    //открываем COM порт
	attachInterrupt(1,start,RISING);     //аппаратное прерывание при прохождении первого датчика
	attachInterrupt(0,finish,RISING);      //аппаратное прерывание при прохождении второго датчика
}
void start() 
{
	if (gap1==0) {   //если измерение еще не проводилось
		gap1=micros(); //получаем время работы ардуино с момента включения до момента пролетания первой пули
	}
}
void finish() 
{
	if (gap2==0) {  //если измерение еще не проводилось
		gap2=micros();  //получаем время работы ардуино с момента включения до момента пролетания второй пули
	}
}
void loop() {
	if (initial==0) {                          //флажок первого запуска
		Serial.println("Press 3 to rapidity mode");                      //режим измерения скорострельности
		Serial.println("Press 2 to mass set mode");                      //режим выбора массы снаряда
		Serial.println("Press 1 to service mode");                      //режим отладки (резисторы)
		Serial.println("Press 0 speed measure mode (default)");        //выход из режимов
		Serial.println("System is ready, just pull the f*ckin trigger!");   //уведомление о том, что хрон готов к работе
		Serial.println(" ");
		initial=1;       //первый запуск, больше не показываем сообщения
	}

	if (Serial.available() > 0 && set!=2) {   //еси есть какие буквы на вход с порта и не выбран 2 режим
		int val=Serial.read();                  //прочитать что было послано в порт
		switch(val) {                           //оператор выбора

		case 48: set=0; flagmass=0; rapidflag=0; initial=0; break;    //если приняли 0 то выбрать 0 режим
		case 49: set=1; break;                //если приняли 1 то запустить режим 1
		case 50: set=2; break;                //если приняли 2 то запустить режим 2
		case 51: set=3; break;                //если приняли 3 то запустить режим 3
		}
	}

	if (set==1) {                    //если 1 режим
		Serial.print("sensor 1: ");
		Serial.println(analogRead(2));  //показать значение на первом датчике
		Serial.print("sensor 2: "); 
		Serial.print(analogRead(4));   //показать значение на втором датчике
		Serial.println();
		Serial.println();              //ну типо два переноса строки
	}

	if (set==2) {             //если 2 режим
		if (flagmass==0) {      //флажок чтобы показать надпись только 1 раз
			Serial.print("Set the mass of bullet (gramm): ");     //надпись
			flagmass=1;
		}
		if(Serial.available() > 0)         //если есть что на вход с порта
		{
			massstring = Serial.readStringUntil('\n');   //присвоить massstring всё что было послано в порт
			flagmassset=1;   //поднять флажок
		}
		if (flagmassset==1) {      //если флажок поднят (приняли значение в порт)
			Serial.println(massstring);   //написать введённое значение
			Serial.println(" ");
			massstring.toCharArray(masschar,sizeof(masschar));   //перевод значения в float (десятичная дробь)
			mass=atof(masschar)/1000;                             //всё ещё перевод
			flagmass=0;                     //опустить все флажки    
			flagmassset=0;
			initial=0; 
			set=0;                //показать приветственную надпись
		}
	}

	if (gap1!=0 && gap2!=0 && gap2>gap1 && set==0) {        //если пуля прошла оба датчика в 0 режиме
		velocity=(1000000*(dist)/(gap2-gap1));         //вычисление скорости как расстояние/время
		energy=velocity*velocity*mass/2;              //вычисление энергии
		Serial.print("Shot #");                        
		Serial.println(n);                                 //вывод номера выстрела
		Serial.print("Speed: ");    
		Serial.println(velocity);                          //вывод скорости в COM
		Serial.print("Energy: ");    
		Serial.println(energy);                          //вывод энергии в COM
		Serial.println(" "); 
		gap1=0;                                   //сброс значений
		gap2=0;
		n++;                                      //номер выстрела +1
	}

	if (micros()-gap1>1000000 && gap1!=0 && set!=1) { // (если пуля прошла первый датчик) И (прошла уже 1 секунда, а второй датчик не тронут)
		Serial.println("FAIL"); //выдаёт FAIL через 1 секунду, если пуля прошла через первый датчик, а через второй нет
		gap1=0;
		gap2=0;
	}

	if (set==3) {          //режим скорострельности
		if (rapidflag==0) {
			Serial.println("Welcome to the rapidity test!");
			Serial.println("");
			rapidflag=1;          //показать надпись один раз
		}
		if (gap1!=0) {         //если пролетели через первый датчик
			rapidtime=60/((float)(gap1-lastshot)/1000000);     //расчет скорострельности выстр/мин
			lastshot=gap1;                                   //запомнить время между выстрелами
			Serial.print("Rapidity (shot/min): ");    //вывод
			Serial.println(rapidtime);
			Serial.println(" ");
			gap1=0;
		}
	}
	delay(200);    //задержка для стабильности
}
