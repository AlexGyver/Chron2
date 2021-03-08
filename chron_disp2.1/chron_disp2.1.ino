/*
Created 2016
by AlexGyver
AlexGyver Home Labs Inc.
*/
#include <EEPROM.h>   //библиотека для работы со внутренней памятью ардуино
#include <avr/sleep.h>   //библиотека режимов сна
#include "TM1637.h"  //библиотека дисплея
#define CLK 5  //пин дисплея     
#define DIO 4   //пин дисплея 
TM1637 disp(CLK,DIO);   //обозвать дисплей disp

int FAIL[]={15,10,1,16};  //надпись FAIL
int LOLO[]={6,12,18,2};  //надпись LOL
int tire[]={17,17,17,17};  //надпись ----
int SPED[]={18,5,20,18};  //надпись SP
int EN[]={18,15,21,18};  //надпись EN
int RAP[]={18,10,20,18};  //надпись RA
int CO[]={18,12,0,18};  //надпись CO

float dist=0.0896;       //расстояние между датчиками в метрах 
char masschar[5];  //массив символов для перевода
String massstring,velstring,velstring_km,rapidstring,rapidstring_s,shotstring,velstring_aver,energystring; //строки
int mode,setmass,i,rapidtime;
boolean initial,flagmass, flagmassset,sleep,sleep_flag,rapidflag,button,bstate,show,vel_en,state,flag_m,flag_m2,blink_flag;  //флажки
int n=1;            //номер выстрела, начиная с 1
unsigned int n_shot;
float velocity, energy;    //переменная для хранения скорости
float mass=0.25;       //масса снаряда в граммах 
volatile unsigned long gap1, gap2;    //отметки времени прохождения пулей датчиков
unsigned long sleep_timer,lastshot,time_press,lst;
int disp_text[4];
boolean set[4];
byte n_aver;
int mass_array[4],aver[5],sum,aver_velocity;
int num,dig;
uint8_t mass_mem;  //запоминает массу для записи в EEPROM

void setup() {
	Serial.begin(9600);    //открываем COM порт
	pinMode(8,INPUT);       //кнопка подключена сюда
	pinMode(9,OUTPUT);       //питание кнопки
	digitalWrite(9,HIGH);  //питание кнопки вкл
	attachInterrupt(1,start,RISING);     //аппаратное прерывание при прохождении первого датчика
	attachInterrupt(0,finish,RISING);      //аппаратное прерывание при прохождении второго датчика
	disp.init();  //инициализация дисплея
	disp.set(2);  //яркость (0-7)   
	mass=EEPROM.read(0)+(float)EEPROM.read(1)/100;  //прочитать массу из внутренней памяти
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
	sleep=0;   //не спать!
	sleep_timer=millis();  //сбросить счетчик сна
}

void print_disp(int x[]) {    //функция для удобной работы с дисплеем (на вход полаётся массив из 4 чисел, они выводятся на дисплей)
	for (int i=0;i<=3;i++) {
		disp.display(i,x[i]);   //вывести на дисплей содержимое 
	} 
}
void mass_set() {
	disp.point(POINT_ON);     //включить двоеточие
	int mass1_1=floor(mass);  //взять целую часть от массы пули
	int mass2_1=(mass-mass1_1)*100;  //взять дробную часть от массы и превратить ей в целые числа
	mass_array[0]=floor(mass1_1/10);   //далее все 4 цифры массы присваиваются в строку mass_array
	mass_array[1]=mass1_1-mass_array[0]*10;
	mass_array[2]=floor(mass2_1/10);
	mass_array[3]=mass2_1-mass_array[2]*10;

	print_disp(mass_array);  //выводим на дисплей полученную массу
	flag_m=0;  //обнуляем флаги
	num=0;
	while (flag_m==0) {   
		if (flag_m2==0) {
			dig=mass_array[num];   //изменяем цифру массы под номером num
			flag_m2=1;
		}
		mass_array[num]=dig;
		
		//-----------------------------------------------------
		if (digitalRead(8)==1 && state==0) {        //выбор режимов кнопкой. Если кнопка нажата
			delay(100);  //защита от дребезга
			state=1;
			button=1;      
			time_press=millis();              //запомнить время нажатия
			while (millis()-time_press<500) {        //выполнять, пока кнопка нажата не менее 500 миллисекунд
				if (digitalRead(8)==0) {
					button=0;
					break;
				}
			}
			if (button==0) {   //обработка нажатия: короткое - прибавить единицу, длинное - переключить на следующую цифру
				dig++;
				print_disp(mass_array);
				if (dig>=10) {dig=0;} 
			}
			if (button==1) {
				num++;
				flag_m2=0;
				if (num>=4) {
					flag_m=1;
				}
			}
		}
		if (digitalRead(8)==0 && state==1) {     //если кнопка отпущена
			state=0;           //скинуть флажок
		}
		//-----------------------------------------------------
		if (millis()-lst>700 && blink_flag==0) {
			print_disp(mass_array);
			disp.display(num,18);
			lst=millis();      
			blink_flag=1;
		}
		if (millis()-lst>200 && blink_flag==1) {
			print_disp(mass_array);
			lst=millis();
			blink_flag=0;
		}
	}  
	mass=mass_array[0]*10+mass_array[1]+(float)mass_array[2]/10+(float)mass_array[3]/100;
	mass_mem=mass_array[0]*10+mass_array[1];
	EEPROM.write(0,mass_mem);
	mass_mem=mass_array[2]*10+mass_array[3];
	EEPROM.write(1,mass_mem);
	disp.point(POINT_OFF);
	delay(500);
	print_disp(tire);
}

void energy_print() {
	disp.point(POINT_ON);
	switch (energystring.length()) {               //кароч тут измеряется длина строки и соотвествено выводится всё на дисплей
	case 4:
		disp.display(0,18);
		disp.display(1,energystring[0]- '0');
		disp.display(2,energystring[2]- '0');
		disp.display(3,energystring[3]- '0');
		break;
	case 5:
		disp.display(0,energystring[0]- '0');
		disp.display(1,energystring[1]- '0');
		disp.display(1,energystring[3]- '0');
		disp.display(1,energystring[4]- '0');
		break;
	} 
}

void black_print(String x) {
	disp.point(POINT_OFF);
	switch (x.length()) {         //кароч тут измеряется длина строки и соотвествено выводится всё на дисплей
	case 1:
		disp.display(0,18);
		disp.display(1,18);
		disp.display(2,18);
		disp.display(3,x[0]- '0');
		break;
	case 2:
		disp.display(0,18);
		disp.display(1,18);
		disp.display(2,x[0]- '0');
		disp.display(3,x[1]- '0');
		break;
	case 3:
		disp.display(0,18);
		disp.display(1,x[0]- '0');
		disp.display(2,x[1]- '0');
		disp.display(3,x[2]- '0');
		break;
	}
}

void loop() {
	if (initial==0) {                          //флажок первого запуска
		print_disp(LOLO);
		Serial.println("Press 0 speed measure mode (default)");        //выход из режимов
		Serial.println("Press 1 to enegry mode");                      //режим измерения скорострельности
		Serial.println("Press 2 to rapidity mode");                      //режим измерения скорострельности
		Serial.println("Press 3 to count mode");                      //режим измерения скорострельности
		Serial.println("Press 4 to mass set mode");                      //режим выбора массы снаряда
		Serial.println("Press 5 to service mode");                      //режим отладки (резисторы)
		Serial.println("System is ready, just pull the f*ckin trigger!");   //уведомление о том, что хрон готов к работе
		Serial.println(" ");
		initial=1;       //первый запуск, больше не показываем сообщения    
		delay(500);
		print_disp(tire);   // вывести -----
	}

	if (Serial.available() > 0 && mode!=2) {   //еси есть какие буквы на вход с порта и не выбран 2 режим
		int val=Serial.read();                  //прочитать что было послано в порт
		switch(val) {                           //оператор выбора

		case 48: mode=0; flagmass=0; rapidflag=0; initial=0; break;    //если приняли 0 то выбрать 0 режим
		case 49: mode=1; break;                //если приняли 1 то запустить режим 1
		case 50: mode=2; break;                //если приняли 2 то запустить режим 2
		case 51: mode=3; break;                //если приняли 3 то запустить режим 3
		case 52: mode=4; break;                //если приняли 4 то запустить режим 4
		case 53: mode=5; break;                //если приняли 5 то запустить режим 5
		}
	}

	if (mode==5) {                    //если 1 режим
		Serial.print("sensor 1: ");
		Serial.println(analogRead(2));  //показать значение на первом датчике
		Serial.print("sensor 2: "); 
		Serial.print(analogRead(4));   //показать значение на втором датчике
		Serial.println();
		Serial.println();              //ну типо два переноса строки
		delay(200);
	}

	if (mode==4) {             //если 2 режим
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
			initial=0;                 //показать приветственную надпись
			mode=0;
		}
	}

	if (gap1!=0 && gap2!=0 && gap2>gap1 && (mode==0 || mode==1)) {        //если пуля прошла оба датчика в 0 режиме
		velocity=(1000000*(dist)/(gap2-gap1));         //вычисление скорости как расстояние/время
		energy=velocity*velocity*mass/1000/2;              //вычисление энергии
		Serial.print("Shot #");                        
		Serial.println(n);                                 //вывод номера выстрела
		Serial.print("Speed: ");    
		Serial.println(velocity);                          //вывод скорости в COM
		Serial.print("Energy: ");    
		Serial.println(energy);                          //вывод энергии в COM
		Serial.println(" ");
		velstring=String(round(velocity));          //сделать строку velstring из округлённой скорости
		velstring_km=String(round(velocity*3.6)); 
		energystring=String(energy);    
		aver[n_aver]=velocity;
		n_aver++;
		if (n_aver>=5) { n_aver=0;}
		sum=0;
		for (int i=0;i<=4;i++) {
			sum=sum+aver[i]; }
		aver_velocity=sum/5;
		velstring_aver=String(round(aver_velocity));
		gap1=0;                                   //сброс значений
		gap2=0;
		show=1;                                 //показать значение на дисплее
		n++;                                      //номер выстрела +1    
	}

	if (mode==2) {             //тест скорострельности
		if (rapidflag==0) {
			Serial.println("Welcome to the rapidity test!");
			Serial.println("");
			rapidflag=1;
		}
		if (gap1!=0) {
			rapidtime=60/((float)(gap1-lastshot)/1000000);  //расчет
			lastshot=gap1;                                    //запоминаем время последнего выстрела
			rapidstring=String(round(rapidtime));         //перевод в строку
			rapidstring_s=String(round(rapidtime/60));
			Serial.print("Rapidity (shot/min): ");
			Serial.println(rapidtime);
			Serial.println(" ");
			gap1=0;
			show=1;                     //показать на дисплее
		}
	}

	//------------------------------------отработка нажатия-----------------------------------
	//----------------------------------------------------------------------------------------
	if (digitalRead(8)==1 && state==0) {        //выбор режимов кнопкой. Если кнопка нажата
		state=1;
		button=1;
		time_press=millis();              //запомнить время нажатия
		while (millis()-time_press<500) {        //выполнять, пока кнопка нажата не менее 500 миллисекунд
			if (digitalRead(8)==0) {
				button=0;
				break;
			}
		}
		switch (button) {
		case 0:
			mode++;
			if (mode>=4) {mode=0;}
			disp.point(POINT_OFF);
			switch (mode) {
			case 0:
				print_disp(SPED);
				delay(300);
				print_disp(tire);
				break;
			case 1:
				print_disp(EN);
				delay(300);
				print_disp(tire);
				break;
			case 2:
				print_disp(RAP);
				delay(300);
				print_disp(tire);
				break;
			case 3:
				print_disp(CO);
				delay(300);
				print_disp(tire);
				break;
			}
			break;
		case 1:
			set[mode]=!set[mode];
			break;
		}
		show=1;
	}
	if (digitalRead(8)==0 && state==1) {     //если кнопка отпущена
		state=0;           //скинуть флажок
	}
	//----------------------------------------------------------------------------------------
	//----------------------------------------------------------------------------------------

	if (show==1) {
		switch (mode) {
		case 0:
			if (set[mode]==0) {black_print(velstring);} else {black_print(velstring_aver
				
				);}
			break;
		case 1:      
			if (set[mode]==0) {energy_print();} else {mass_set();set[mode]=0;}
			break;
		case 2:
			if (set[mode]==0) {black_print(rapidstring);} else {black_print(rapidstring_s);}
			break;
		case 3:
			if (set[mode]==0) {black_print(shotstring);} else {n_shot=0;set[3]=0;shotstring=String(n_shot);black_print(shotstring);}
			break;
		}
		show=0;
	}

	if (mode==3) {
		if (gap1!=0) {
			n_shot++;
			shotstring=String(n_shot);
		}
		gap1=0;
		show=1;
	}

	if (micros()-gap1>1000000 && gap1!=0 && mode!=5) { // (если пуля прошла первый датчик) И (прошла уже 1 секунда, а второй датчик не тронут)
		disp.point(POINT_OFF);
		print_disp(FAIL);  //вывести fail 
		Serial.println("FAIL"); //выдаёт FAIL через 1 секунду, если пуля прошла через первый датчик, а через второй нет
		gap1=0;
		gap2=0;
		delay(400);
		print_disp(tire);   // вывести -----
		sleep=0;
		sleep_timer=millis();
	}

	if (millis()-sleep_timer>30000 && sleep==0 && mode!=5 && (mode!=1 && set[1]!=1)) {    //если после последнего выстрела прошло
		sleep=1;
		sleep_flag=1;
		disp.point(POINT_ON);
		for (int i=0;i<=3;i++) { 
			disp.display(i,18);              //погасить дисплей
		}
		delay(100);
		set_sleep_mode(SLEEP_MODE_PWR_DOWN); // выбор режима энергопотребления
		sleep_mode();                        // уходим в спячку
	}

	delay(5);

}

