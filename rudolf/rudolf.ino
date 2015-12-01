#define LED         13

#define TRAIN        2
#define HALFWAY     20
#define FINISHED    21

#define TIMEOUT     20

#define MOTOR       5
#define DIRECTION   6

#define STARTUP     0
#define IDLE        1
#define OUTBOUND    2
#define RESTING     3
#define INBOUND     4
#define ALLDONE     5

int occ;
int state;  // What are we doing?
int count;  // Did we get lost somehow?
int lockout;    // Don't re-trigger during cycle (or even a few seconds after)
boolean train;  // Here comes the Train
boolean surge;  // Leapin' Reindeer

void detector() { if (state == IDLE && lockout == 0) {count=0; train=true; }}
void halfway()  { if (state == OUTBOUND){ setstate(RESTING);} }
void finished() { if (state == INBOUND) {setstate(ALLDONE); lockout=14000; train=false; }}

void mydelay(unsigned int md)
{
int mums;
int didwe = 0;
	for(mums=0; mums < 100 ; mums++)
	{
		delayMicroseconds(1000);
		didwe++;
	}
}

void blink(int n,unsigned long int d)
{
int i;
unsigned long int dly;
	for(i=0;i<n;i++)
	{
		digitalWrite(LED,1);
		dly = d;
		mydelay(dly);
		digitalWrite(LED,0);
		dly = d;
		mydelay(dly);
	}
}

// 1=setvalue, 2=return_value, 3=return_addr, 4=printval
int *safespace(int cmd, int val)
{
static int _myval_;

	if      (cmd == 1) _myval_ = val;
	else if (cmd == 2) return((int *) _myval_);
	else if (cmd == 3) return(&_myval_);
	else if (cmd == 4) Serial.println(_myval_);
        return(&_myval_);
}


void setstate(int value)
{
	state = value;
	safespace(1, value);
}


void setup() 
{
	Serial.begin(9600);
	mydelay(200);
	Serial.print("Entering setup safestate value is:");
	safespace(4,-1);
	if ( ((int)safespace(2,-1)) != STARTUP )
	{
		Serial.print((int)safespace(2,-1));
		Serial.print(" != ");
		Serial.print(STARTUP);
		Serial.println(" --> out of sequence reset?");
	} 
	setstate(STARTUP);
	lockout = 14;  // Cannot trigger immediately after a reset
	train = false;

	pinMode(TRAIN,INPUT_PULLUP);
	pinMode(HALFWAY,INPUT_PULLUP);
	pinMode(FINISHED,INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(TRAIN),detector,FALLING);
	attachInterrupt(digitalPinToInterrupt(HALFWAY),halfway,FALLING);
	attachInterrupt(digitalPinToInterrupt(FINISHED),finished,FALLING);

	pinMode(LED,      OUTPUT);
	pinMode(MOTOR,    OUTPUT);
	pinMode(DIRECTION,OUTPUT);

	surge = true;
	occ = 0;
	setstate(IDLE);
	Serial.println("setup_done");
}

void leap()
{
int de = 800;
	train = false;
	digitalWrite(MOTOR,surge);
	if (surge) blink(4,30UL);
	mydelay(de);
	surge = !surge;
}

void clearall()
{
	setstate(IDLE);
	count = 0;
	digitalWrite(MOTOR,0);
	digitalWrite(DIRECTION,0);
}

void reverse()
{
	count = 0;
	digitalWrite(MOTOR, 0);
	mydelay(2000);
	digitalWrite(DIRECTION,1);
	lockout = 4;
	setstate(INBOUND);
}

void loop()
{
int tmp;
	Serial.print("l");
//	while(1)  // Optional
//	{
	occ++;
	if (occ%30000 == 0) Serial.println("while(1)");

		if (state == IDLE && lockout > 0) {
			lockout--;         // Lockout Countdown
			if (lockout == 0)  // Now get ready for the train
			{
				Serial.println("lockout finished");
				clearall();
				train = false;
			}
		}

		if (occ%30000 == 0)
			Serial.println(state);
		switch(state) {
			case INBOUND:
				Serial.print("<");
				leap();
				if (count++ > TIMEOUT) {
					Serial.print("inbound timeout");
					clearall();  // Return switch failed?
					lockout = 14;
				}
				break;
			case OUTBOUND:
				Serial.print(">");
				leap();
				if (count++ > TIMEOUT) { //Outbound switch fail
					Serial.print("OUTbound timeout");
					reverse(); // 4-seconds/zeros count
				}
				break;
			case RESTING:
				Serial.print("_");
				blink(5,100UL);
				reverse();  // State -> INBOUND
				break;
			case IDLE:
				if (train) {
					Serial.print("train");
					digitalWrite(DIRECTION,0);
					count = 0;
					setstate(OUTBOUND);
					lockout = 6;
					surge = true;
					train = false;
					leap();
					Serial.print("*");
					blink(6,30UL);
				} else {
					clearall();
					blink(2,100UL);
					Serial.print(".");
				}
                                mydelay(1000);
				break;
			case ALLDONE:
	           		Serial.print("All done...");
				mydelay(2000);
				setstate(IDLE);
				Serial.println("BACK TO IDLE");
				break;
			default :
				mydelay(100);
		} // End Switch
//	} // End (Optional) while(1)
} // End Loop
