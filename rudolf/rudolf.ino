#define LED         13

#define TRAIN        2
#define HALFWAY     20
#define FINISHED    21

#define TIMEOUT     10

#define MOTOR       5
#define DIRECTION   6

#define IDLE        0
#define OUTBOUND    1
#define RESTING     2
#define INBOUND     3

int state;      // What are we doing?
int count;      // Did we get lost somehow?
int lockout;    // Don't re-trigger during cycle (or even a few seconds after)
boolean train;  // Here comes the Train
boolean surge;  // Leapin' Reindeer

void detector() { if (state == IDLE && lockout == 0)     train = true;   }
void halfway()  { if (state == OUTBOUND && lockout == 0){ state = RESTING;} }
void finished() { if (state == INBOUND && lockout == 0) {lockout=14; train=false; state=IDLE;}}

void blink(int n,int d)
{
int i;
	for(i=0;i<n;i++)
	{
		digitalWrite(LED,1);
		delay(d);
		digitalWrite(LED,0);
		delay(d);
	}
}

void setup() 
{
	Serial.begin(9600);
	lockout = 14;  // Cannot trigger immediately after a reset
	train = false;

	pinMode(TRAIN,INPUT_PULLUP);
	pinMode(HALFWAY,INPUT_PULLUP);
	pinMode(FINISHED,INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(TRAIN),detector,FALLING);
	attachInterrupt(digitalPinToInterrupt(HALFWAY),halfway,FALLING);
	attachInterrupt(digitalPinToInterrupt(FINISHED),finished,FALLING);

	pinMode(LED,      OUTPUT); digitalWrite(LED,0);
	pinMode(MOTOR,    OUTPUT);
	pinMode(DIRECTION,OUTPUT);

	state = IDLE;
	surge = true;
	Serial.print("SETUP");
}

// Leap can be interrupted by a state change
void leap()
{
	int i = 8;
	int sstate = state;
	train = false;
	digitalWrite(MOTOR,surge);
	if (surge) blink(4,30);
	while (sstate == state && i-- > 0) delay(100);
	surge = !surge;
}

void clearall()
{
	state = IDLE;
	count = 0;
	digitalWrite(MOTOR,0);
	digitalWrite(DIRECTION,0);
}

void reverse()
{
	count = 0;
	digitalWrite(MOTOR, 0);
	digitalWrite(DIRECTION,1);
	delay(2000);
	lockout = 4;
	state = INBOUND;
}

void loop()
{
	while(1)  // Optional
	{
		if (lockout > 0) {
			lockout--;         // Lockout Countdown
			if (state==IDLE && lockout == 0)  // Now get ready for the train
			{
				clearall();
				train = false;
			}
		}

		switch(state) {
			case INBOUND:
				Serial.print("<");
				leap();
				if (count++ > TIMEOUT) {
					Serial.print("Inbound timeout");
					clearall();  // Return switch failed?
					lockout = 14;
				}
				break;
			case OUTBOUND:
				Serial.print(">");
				leap();
				if (count++ > TIMEOUT) //Outbound switch fail
				{
					Serial.print("Outbound timeout");
					reverse(); // 4-seconds/zeros count
				}
				break;
			case RESTING:
				Serial.print("_");
				blink(5,100);
				reverse();  // State -> INBOUND
				break;
			case IDLE:
				if (train) {
					digitalWrite(DIRECTION,0);
					state = OUTBOUND;
					lockout = 6;
					surge = true;
					train = false;
					leap();
					Serial.print("*");
					blink(6,30);
				} else {
					clearall();
					blink(2,100);
					Serial.println(".");
					delay(500);
				}
				break;
			default :
				delay(100);
				break;
		} // End Switch
	} // End (Optional) while(1)
} // End Loop
