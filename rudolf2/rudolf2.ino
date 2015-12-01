
#define LED         13

#define TRAIN        2
#define HALFWAY     20
#define FINISHED    21

#define TIMEOUT     290000UL

#define MOTOR       5
#define DIRECTION   6

#define STARTUP     0
#define IDLE        1
#define OUTBOUND    2
#define PAUSING     3
#define PAUSED      4
#define INBOUND     5
#define ALLDONE     6
#define RESTING     7

#define LONG_LOCKOUT   350000UL
#define SHORT_LOCKOUT   80000UL

char *statename[8] = {
	"STARTUP",
	"IDLE",
	"OUTBOUND",
	"PAUSING",
	"PAUSED",
	"INBOUND",
	"ALLDONE",
	"RESTING" };

unsigned long int last_timestamp;
unsigned long int durationMS;
unsigned long int surgeMS = 1200UL;

unsigned long int lockout;    // Don't re-trigger during cycle (or even a few seconds after)

int occ;
int state;                // What are we doing?
unsigned long int count;  // Did we get lost somehow?
boolean train;            // Here comes the Train

void detector() { if (state == IDLE && lockout == 0) {count=0; train=true; }}
void halfway()  { if (state == OUTBOUND){ setstate(PAUSING);} }
void finished() { if (state == INBOUND) { lockout=LONG_LOCKOUT; train=false; setstate(ALLDONE);} }


void setstate(int value)
{
static int last_state;
	state = value;
	if (state != last_state)
		Serial.println(statename[value]);
	last_state = state;
}


void setup() 
{
	Serial.begin(9600);
	setstate(STARTUP);
	lockout = LONG_LOCKOUT;  // Cannot trigger immediately after a reset
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

	occ = 0;
	setstate(IDLE);
	Serial.println("setup");
}

void clearall()
{
	setstate(IDLE);
	count = 0;
	digitalWrite(MOTOR,0);
	digitalWrite(DIRECTION,0);
}

void show() { if (occ%10000 == 0) Serial.print(statename[state]); }

boolean phase()
{
static boolean last;
boolean p = !(((millis()-last_timestamp)/surgeMS)%2);
	if (p != last) Serial.print(p);
	last = p;
	return(p);
}

void loop()
{
		if (state == IDLE && lockout > 0) {
			lockout--;         // Lockout Countdown
			if (lockout == 0)  // Now get ready for the train
			{
				Serial.println("               READY");
				clearall();
				train = false;
			}
		}

		occ++;
		switch(state) {
			case INBOUND:
				digitalWrite(DIRECTION,1);
				digitalWrite(MOTOR, phase());
				if (count++ > TIMEOUT) {
					Serial.print("inbound timeout");
					clearall();  // Return switch failed?
					lockout = LONG_LOCKOUT;
				}
				break;
			case OUTBOUND:
				digitalWrite(DIRECTION,0);
				digitalWrite(MOTOR, phase());
				if (count++ > TIMEOUT)  //Outbound switch fail
				{
					Serial.print("OUTbound timeout");
					count = 0;
					digitalWrite(MOTOR, 0);
					setstate(PAUSING);
					digitalWrite(DIRECTION,1);
				}
				break;
			case PAUSING:
				digitalWrite(MOTOR, 0);
				last_timestamp = millis();
				durationMS = 3000UL;
				digitalWrite(DIRECTION,1);
				setstate(PAUSED);
				break;
			case PAUSED:
				if (millis() > last_timestamp + durationMS)
					setstate(INBOUND);
				break;
			case IDLE:
				if (train) {
					digitalWrite(DIRECTION,0);
					last_timestamp = millis();
					setstate(OUTBOUND);
					count = 0;
					lockout = SHORT_LOCKOUT;
					train = false;
				} else
					clearall();
				break;
			case ALLDONE:
				digitalWrite(MOTOR,0);
				last_timestamp = millis();
				durationMS = 2000UL;
				setstate(RESTING);
				break;
			case RESTING:
				if (millis() > last_timestamp + durationMS)
					setstate(IDLE);
				break;
			default:
				Serial.println("This shouldn't happen!");
		} // End Switch
		phase();
//	} // End (Optional) while(1)
} // End Loop
