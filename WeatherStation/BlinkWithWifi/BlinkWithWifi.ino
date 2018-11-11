/*
 Name:		BlinkWithWifi.ino
 Created:	11/10/2018 6:18:31 PM
 Author:	Günter
*/

// the setup function runs once when you press reset or power the board
void setup() {

	pinMode(LED_BUILTIN, OUTPUT);


}

// the loop function runs over and over again until power down or reset
void loop() {
	digitalWrite(LED_BUILTIN, LOW);
	delay(20);
	digitalWrite(LED_BUILTIN, HIGH);
	delay(1980);
}
