//Pins
const uint8_t trigPin = 2;
const uint8_t echoPin = 8;
const uint8_t LEDPIN = 13;
//Global
float duration, distance;

void setup() {
  pinMode(LEDPIN, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  Serial.begin(9600);
  attachInterrupt(digitalPinToInterrupt(trigPin), led, CHANGE);
}

void loop() {
  delay(100);
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration*.0343)/2;
  Serial.print("Distance: ");
  Serial.println(distance);
}

void led()
{
  if(distance <= 10)
  {
    digitalWrite(LEDPIN, HIGH);
  }
  else
  {
    digitalWrite(LEDPIN, LOW);  
  }
}
