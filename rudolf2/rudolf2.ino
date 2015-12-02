//#define DEBUG 1
//#define digitalPinToInterrupt(p)  (p==2?0:(p==3?1:(p>=18&&p<=21?23-p:-1)))

// Self reset after going off the rails.
// Pulses on output pin keep reset high,
// if pulses stop, pin goes low, but only for a second

#define LED         13
#define TRAIN        2
#define HALFWAY     20
#define FINISHED    21

#define TIMEOUT     180000UL

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

unsigned long m_lasttime;
unsigned long d_lasttime;

char *statename[8] = {
	"STARTUP",
	"IDLE",
	"OUTBOUND",
	"PAUSING",
	"PAUSED",
	"INBOUND",
	"ALLDONE",
	"RESTING",
};

boolean direction;
boolean motorON;

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
	state = value;
}

void showstate()
{
static int last_state;
#ifdef DEBUG
	if (state != last_state)
		Serial.println(statename[state]);
#endif
	last_state = state;
}

void mydelay(int ms)
{
	int msdec = ms/10;
	while(msdec-- > 0) delayMicroseconds(10000);
}

boolean NotSoFast()
{
unsigned long now = millis();
	if ( ( now - m_lasttime < 200) ||  (now - d_lasttime) < 200 )
	{
#ifdef DEBUG
		Serial.print("Toggling motor too fast...");
#endif
		mydelay(1000);
#ifdef DEBUG
		Serial.println("ok now.");
#endif
	}
}


void motor(boolean m)
{
	if (m != motorON)
	{
		NotSoFast();
		motorON = m;
		m_lasttime = millis();
	}
	digitalWrite(MOTOR, m);
}

void set_direction(boolean d)
{
	if (d != direction) 
	{
		NotSoFast();
		direction = d;
		d_lasttime = millis();
	}
	digitalWrite(DIRECTION, direction);
}

void setup() 
{
#ifdef DEBUG
	Serial.begin(9600);
#endif
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
	digitalWrite(LED, 0);
	pinMode(MOTOR,    OUTPUT);
	motor(false);
	pinMode(DIRECTION,OUTPUT);
	set_direction(false);
	durationMS = 3000UL;

	occ = 0;
	count = 0;
	setstate(IDLE);
#ifdef DEBUG
	Serial.println("setup");
#endif
	d_lasttime = millis();
	m_lasttime = millis();
}

void clearall()
{
	setstate(IDLE);
	count = 0;
	motor(false);
	set_direction(false);
}

boolean phase()
{
static boolean last;
boolean p = !(((millis()-last_timestamp)/surgeMS)%2);
#ifdef DEBUG
	if (p != last) Serial.print(p);
#endif
	last = p;
	return(p);
}

void loop()
{
		if (state == IDLE && lockout > 0) {
			lockout--;         // Lockout Countdown
			if (lockout == 0)  // Now get ready for the train
			{
#ifdef DEBUG
				Serial.println("               READY");
#endif
				clearall();
				train = false;
			}
		}

		occ++;
		showstate();
		switch(state) {
			case INBOUND:
				set_direction(true);
				motor(phase());
				if (count++ > TIMEOUT) {
					set_direction(false);
#ifdef DEBUG
					Serial.print("inbound timeout");
#endif
					setstate(ALLDONE);
				}
				break;
			case OUTBOUND:
				set_direction(false);
				motor(phase());
				if (count++ > TIMEOUT)  //Outbound switch fail
				{
					motor(false);
					set_direction(true);
#ifdef DEBUG
					Serial.print("OUTbound timeout");
#endif
					setstate(PAUSING);
				}
				break;
			case PAUSING:
				count = 0;
				motor(false);
				set_direction(true);
				last_timestamp = millis();
				durationMS = 3000UL;
				setstate(PAUSED);
				break;
			case PAUSED:
				count = 0;
				if (millis() > last_timestamp + durationMS)
				{
					set_direction(true);
					setstate(INBOUND);
				}
				break;
			case IDLE:
				if (train) {
					set_direction(false);
					last_timestamp = millis();
					setstate(OUTBOUND);
					count = 0;
					lockout = SHORT_LOCKOUT;
					train = false;
				} else
					clearall();
				break;
			case ALLDONE:
				count = 0;
				motor(false);
				last_timestamp = millis();
				durationMS = 2000UL;
				setstate(RESTING);
				break;
			case RESTING:
				lockout = LONG_LOCKOUT;
				if (millis() > last_timestamp + durationMS)
					setstate(IDLE);
				break;
			default:
#ifdef DEBUG
				Serial.println("This shouldn't happen!");
#endif
				break;
		} // End Switch
		phase();
//	} // End (Optional) while(1)
} // End Loop
