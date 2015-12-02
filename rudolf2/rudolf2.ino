
#define digitalPinToInterrupt(p)  ((p)==2?0:((p)==3?1:((p)>=18&&(p)<=21?23-(p):-1)))

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
static int last_state;
	state = value;
	if (state != last_state)
		Serial.println(statename[value]);
	last_state = state;
}
void plh(unsigned long n)
{
byte a,b,c,d;
     a = (n>>24)&0xFF;
     b = (n>>16)&0xFF;
     c = (n>>8)&0xFF;
     d = (n>>8)&0xFF;
     Serial.print(a,HEX);Serial.print(b,HEX);
     Serial.print(c,HEX);Serial.print(d,HEX);
}

void mydelay(int ms)
{
	int msdec = ms/10;
	while(msdec-- > 0) delayMicroseconds(10000);
}

void motor(boolean m)
{
unsigned long now = millis();

	if (m != motorON)
	{
		if ( now - m_lasttime < 200 )
		{
		 Serial.println("Toggling motor too fast");
		 mydelay(1000);
		 Serial.println("now do it");
		}
		motorON = m;
		digitalWrite(MOTOR,motorON);
		m_lasttime = millis();
	}
}

void set_direction(boolean d)
{
unsigned long now = millis();
unsigned long diff;
	if (d != direction) 
	{
		if ( now - d_lasttime < 200 )
		{
			Serial.println("Changing direction too fast");
			mydelay(1000);
			Serial.println("now do it");
		}
		direction = d;
		digitalWrite(DIRECTION,direction);
		d_lasttime = millis();
	}
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
	digitalWrite(LED, 0);
	pinMode(MOTOR,    OUTPUT);
	digitalWrite(MOTOR, 0);
	pinMode(DIRECTION,OUTPUT);
	set_direction(false);

	occ = 0;
	setstate(IDLE);
	Serial.println("setup");
	d_lasttime = millis();
	m_lasttime = millis();
}

void clearall()
{
	setstate(IDLE);
	count = 0;
	digitalWrite(MOTOR,0);
	set_direction(false);
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
				set_direction(true);
				motor(phase());
				if (count++ > TIMEOUT) {
					Serial.print("inbound timeout");
					clearall();  // Return switch failed?
					lockout = LONG_LOCKOUT;
				}
				break;
			case OUTBOUND:
				set_direction(false);
				motor(phase());
				if (count++ > TIMEOUT)  //Outbound switch fail
				{
					Serial.print("OUTbound timeout");
					count = 0;
					motor(false);
					setstate(PAUSING);
					set_direction(true);
				}
				break;
			case PAUSING:
				digitalWrite(MOTOR, 0);
				last_timestamp = millis();
				durationMS = 3000UL;
				set_direction(true);
				setstate(PAUSED);
				break;
			case PAUSED:
				if (millis() > last_timestamp + durationMS)
					setstate(INBOUND);
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
				motor(false);
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
