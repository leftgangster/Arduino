void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while (!Serial);
}

String text;

//Unions! Instant conversion for different types! (Well I don't know if this statement is true)
typedef union{
  float floatValue;
  char floatChar[4];
} floatByte;

// Two independant variables.
floatByte charByte;
floatByte backToFloat;

void loop() {
  // put your main code here, to run repeatedly:
  if (Serial.available()){
    text=Serial.readString();

    // Assigning value to one of the variables
    charByte.floatValue=text.toFloat();
    for (int i=0;i<4;i++){

      // Assigning value to one of the variables
      backToFloat.floatChar[i]=charByte.floatChar[i];
    }

    // Automatically the second variable of the union has a value!
    Serial.print(charByte.floatChar[0],HEX);
    Serial.print(" ");
    Serial.print(charByte.floatChar[1],HEX);
    Serial.print(" ");
    Serial.print(charByte.floatChar[2],HEX);
    Serial.print(" ");
    Serial.print(charByte.floatChar[3],HEX);
    Serial.print(" ,");

    // Same here!
    Serial.println(backToFloat.floatValue);
  }
}
