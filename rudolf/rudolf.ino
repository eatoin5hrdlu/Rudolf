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
boolean train;  // Here comes the Train
boolean surge;  // Leapin' Reindeer

void detector() { if (state == IDLE)     train = true;    }
void halfway()  { if (state == OUTBOUND) state = RESTING; }
void finished() { if (state == INBOUND)  {train = false; state = IDLE;  } }

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

	pinMode(TRAIN,INPUT_PULLUP);
	pinMode(HALFWAY,INPUT_PULLUP);
	pinMode(FINISHED,INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(TRAIN),detector,FALLING);
	attachInterrupt(digitalPinToInterrupt(HALFWAY),halfway,FALLING);
	attachInterrupt(digitalPinToInterrupt(FINISHED),finished,FALLING);

	pinMode(LED,      OUTPUT);
	pinMode(MOTOR,    OUTPUT);
	pinMode(DIRECTION,OUTPUT);

	state = IDLE;
	surge = true;
}

void leap()
{
	digitalWrite(MOTOR,surge);
	if (surge) blink(4,50);
	delay(1000);
	surge = !surge;
	train = false;
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
	delay(2000);
	digitalWrite(DIRECTION,1);
	state = INBOUND;
}

void loop()
{
	while(1)  // Optional
	{
		if (count++ > 30) clearall();

		switch(state) {
			case INBOUND:
				Serial.print("<");
				leap();
				if (count > TIMEOUT) clearall();  // Return switch failed?
				break;
			case OUTBOUND:
				Serial.print(">");
				leap();
				if (count > TIMEOUT) {  // Outbound switch failed?
					digitalWrite(MOTOR,0);
					delay(3000);
					reverse();
				}
				break;
			case RESTING:
				Serial.print("_");
				blink(5,200);
				reverse();  // State -> INBOUND
				break;
			case IDLE:
				if (train) {
					Serial.print("*");
					blink(10,200);
					state = OUTBOUND;
					digitalWrite(DIRECTION,0);
					digitalWrite(MOTOR,1);
					delay(1000);
					train = false;
				} else {
					clearall();
					blink(1,200);
					Serial.println(".");
					delay(1000);
				}
				break;
			default :
				delay(1000);
				break;
		} // End Switch
	} // End (Optional) while(1)
} // End Loop
