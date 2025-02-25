//http://www.instructables.com/id/Arduino-Timer-Interrupts/

#define outpin PD7
#define intpin PD5
boolean toggle1 = 0;

void setup(){
  Serial.begin(115200);
  Serial.println("20Hz Pulse generator");

  pinMode(PD5, OUTPUT);
  pinMode(PD7, OUTPUT);
  cli();//stop interrupts

  //set timer1 interrupt at 1Hz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 20hz increments
  OCR1A = 780;// = (16*10^6) / (20*1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12 and CS10 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);

  sei();//allow interrupts

}//end setup

void loop() {
  // put your main code here, to run repeatedly:

}

ISR(TIMER1_COMPA_vect){//timer1 interrupt 1KHz toggles pin PD7
//generates pulse wave of frequency 20Hz/2 = 10Hz (takes two cycles for full wave- toggle high then toggle low
    if (toggle1){
    digitalWrite(outpin,HIGH);
    Serial.print("HIGH\t");
    toggle1 = 0;
  }
  else{
    digitalWrite(outpin,LOW);
    Serial.println("LOW");
    toggle1 = 1;
  }
}
