// Simple test which toggles a given pin 

int pin = 57;

void setup()
{
  pinMode(pin, OUTPUT);
}

void loop() 
{
  //analogWrite(pin, 255);
  digitalWrite(pin, HIGH);
  
  delay(1000);
  
  //analogWrite(pin, 0);
  digitalWrite(pin, LOW);
  
  delay(1000);
}
