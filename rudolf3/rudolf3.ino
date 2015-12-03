//#define DEBUG 1
//#define digitalPinToInterrupt(p)  (p==2?0:(p==3?1:(p>=18&&p<=21?23-p:-1)))
// Add capacitance to interrupt input lines (Halfway is triggering early)
// Self reset after going off the rails.
// Pulses on output pin keep reset high,
// if pulses stop, pin goes low, but only for a second

// DEBUGGING: keep track of the number of resets

#define DEBUG_PIN 19

#include <EEPROM.h>
boolean once;
void debug_incr()
{
	int byte = EEPROM.read(0);
	byte = byte + 1;
	EEPROM.write(0,byte);
}
void debug_initialize()
{
	if (once and !digitalRead(DEBUG_PIN))
	{
		once = false;
		Serial.begin(9600);
		while (!Serial) { ; }
	}
}
void debug_resets()
{
	if (!digitalRead(DEBUG_PIN))
	{
	   int byte = EEPROM.read(0);
	   debug_initialize();
	   Serial.print(byte);
	   Serial.println(" resets");
	}
}
void debug_string(char *str)
{
	if (!digitalRead(DEBUG_PIN))
	{
	   debug_initialize();
	   Serial.println(str);
	}
}
void debug_int(int i)
{
	if (!digitalRead(DEBUG_PIN))
	{
	   debug_initialize();
	   Serial.print(i);
	}
}
		
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

#define LONG_LOCKOUT     260000UL
#define SHORT_LOCKOUT     80000UL
#define OUTBOUND_LOCKOUT  60000UL

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
void halfway()  { if (state == OUTBOUND && lockout == 0){ setstate(PAUSING);} }
void finished() { if (state == INBOUND) { lockout=LONG_LOCKOUT; train=false; setstate(ALLDONE);} }


void setstate(int value)
{
	state = value;
}

void showstate()
{
static int last_state;
	if (state != last_state)
		debug_string(statename[state]);
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
		debug_string("Toggling motor too fast...");
		mydelay(1000);
		debug_string("ok now.");
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
	pinMode(DEBUG_PIN,INPUT_PULLUP);
	once = true;
	debug_initialize(); // Conditionally start serial comm
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
	debug_string("setup");
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
	if (p != last) debug_int(p);
	last = p;
	return(p);
}

void loop()
{
	if (lockout > 0) {
		lockout--;
		if (lockout == 0)
		{
			debug_string("\nUnLocked");
			if (state == IDLE)  // Now get ready for the train
			{
			debug_string("               READY");
			clearall();
			train = false;
			}
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
					debug_string("inbound timeout");
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
					debug_string("OUTbound timeout");
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
				digitalWrite(LED,phase());
				if (train) {
					set_direction(false);
					last_timestamp = millis();
					setstate(OUTBOUND);
					count = 0;
					lockout = OUTBOUND_LOCKOUT;
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
				debug_string("This shouldn't happen!");
				break;
		} // End Switch
		phase();
//	} // End (Optional) while(1)
} // End Loop
