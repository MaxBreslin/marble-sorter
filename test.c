#pragma config(Sensor, in1,    line1,          sensorLineFollower)
#pragma config(Sensor, in2,    line2,          sensorLineFollower)
#pragma config(Sensor, in3,    light,          sensorReflection)
#pragma config(Sensor, dgtl1,  bumpSwitch,     sensorTouch)
#pragma config(Sensor, dgtl12, led,            sensorLEDtoVCC)
#pragma config(Motor,  port2,           motor2,        tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port3,           motor1,        tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port4,           servo,         tmotorServoStandard, openLoop)
#pragma config(Motor,  port9,           flashlight,    tmotorVexFlashlight, openLoop, reversed)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//

#define GREEN 0
#define WOOD 1
#define BLACK 2
#define NONE 3

#define min(X, Y) (((X) < (Y)) ? (X) : (Y))
#define max(X, Y) (((X) > (Y)) ? (X) : (Y))

// const int CLEAR_THRESHHOLD_1_LOW = 2610;
//const int CLEAR_THRESHHOLD_1_HIGH = 2700;
// const int GREEN_THRESHHOLD_1 = 2800; // greater than
// const int WOOD_THRESHHOLD_1 = 2350; // less than

const int NORMAL_LOW = 2800;
const int NORMAL_HIGH = 2900;

const int BLACK_THRESHHOLD_1 = 2920; // greater than
const int CLEAR_THRESHHOLD_1_HIGH = 100;
const int GREEN_THRESHHOLD_1 = 2960; // greater than
const int WOOD_THRESHHOLD_1 = 2800; // less than


/*
--------------Program flow--------------
Infinite loop
    Wait for bump switch to be pressed
        Sort all marbles
            Release a marble (release_from_container)
            Wait for the line sensor(s) to read a non-normal value (block_until_marble)
            Read and return the current marble (scan_with_line)
            Move the sorting servo to the correct location
            Transfer the marble from sensor to sorting location
            Increment total_sorted
                If total_sorted >= 15, go back to step # 2
                Else, go back to step #4

*/


void sort(int *arr , int n) {
    int i = 0;
    int j = 0;
    int temp = 0;
    for(i = 0; i < n; i++) {
        for(j = 0; j < n-1; j++) {
            if(arr[j] > arr[j+1]) {
                temp = arr[j];
                arr[j] = arr[j+1];
                arr[j+1] = temp;
            }
        }
    }
}


int get_median(int *arr, int n) {
    int filtered[25];

    int valid_nums = 0;
    for (int i = 0; i < n; i++) {
        if (valid_nums >= 24) {
            break;
        }
        if ((arr[i] < NORMAL_LOW || arr[i] > NORMAL_HIGH) && arr[i] != 0) {
            filtered[valid_nums] = arr[i];
            valid_nums ++;
        }
    }

    return (filtered[25/2]+filtered[(25/2)+1])/2;
}

void block_until_marble(int timeout=-1) {
	int highest = 0;
	int lowest = 0xffff;
	int reading;

	// block until a non-normal value is sensed or ~timeout ms have passed
	while (lowest > NORMAL_LOW && highest < NORMAL_HIGH) {
		if (timeout != -1) {
			if (timeout == 0) {
				break;
			}
			timeout --;
		}

		reading = SensorValue[line1];
		lowest = min(lowest,reading);
		highest = max(highest,reading);

		sleep(1);
	}
}

int scan_with_line(int wait_for=50) {
	int current_marble = NONE;
    int reading;
    int sum = 0;
    int added_nums = 0;
    int average = 0;
    int readings[75];
    int n = sizeof(readings)/sizeof(readings[0]);
    memset(readings,0,n*sizeof(int));

	// record readings
	while (wait_for>0)  { // this will delay for ~wait_for ms
		reading = SensorValue[line1];
		if (reading > NORMAL_LOW && reading < NORMAL_HIGH) {
            break;
        }
        if (added_nums < 75) {
            readings[added_nums] = reading;
            added_nums ++;
        }


		wait_for --;
		sleep(1);
	}

	sort(readings,sizeof(readings)/sizeof(readings[0]));

    int median_reading = get_median(readings, sizeof(readings)/sizeof(readings[0]));

    writeDebugStreamLine("median reading: %i",median_reading);


	if (median_reading > GREEN_THRESHHOLD_1) {
		current_marble = GREEN;
	}
	else if (median_reading > BLACK_THRESHHOLD_1) {
		current_marble = BLACK;
	}
	else if (median_reading < WOOD_THRESHHOLD_1) {
		current_marble = WOOD;
	}

	return current_marble;
}

void rotate_servo(int this_marble) {
	if (this_marble==GREEN) {
		motor[servo] = -127;
		writeDebugStreamLine("GREEN");
	}
	else if (this_marble==BLACK) {
		motor[servo] = -30;
		writeDebugStreamLine("BLACK");
	}
	else if (this_marble==WOOD) {
		motor[servo] = 50;
		writeDebugStreamLine("WOOD");
	}
	else if (this_marble==NONE) {
		writeDebugStreamLine("NONE");
	}
}

// returns false if all marbles have been sorted, true otherwise
bool sort_marble(int &total_sorted, int timeout=-1) {
	block_until_marble(timeout);
	int this_marble = scan_with_line();

	rotate_servo(this_marble);

	total_sorted ++;
	if (total_sorted>=15) {
		total_sorted = 0;
		return false;
	}
	return true;
}

task reverse_spin() {
	while (1) {
			if (bMotorReflected[motor1]) {
				bMotorReflected[motor1] = 0;
				sleep(2000);
			}
			else {
				bMotorReflected[motor1] = 1;
				sleep(1500);
			}
	}
}

task main() {
    int total_sorted = 0;
    bool working = false;
    startTask(reverse_spin);
    while(1) {
        SensorValue[led] = working;
        motor[motor1] = working ? 14 : 0;
        motor[motor2] = working ? -17 : 0;
        if (working) {
            writeDebugStreamLine("%i",total_sorted);
            working = sort_marble(total_sorted);

            sleep(1);
        }
        else {
            working = SensorValue[bumpSwitch];

            sleep(1);
        }
    }


}

// -57, 3, 80

/*
motor[motor1] = 127;
sleep(1500);
motor[motor1] = -127;
sleep(1500);
*/

/*
int total_sorted = 0;
    bool working = false;
    while(1) {
        SensorValue[led] = working;
        motor[flashlight] = working ? 127 : 0;
        if (working) {
            writeDebugStreamLine("%i",total_sorted);
            working = sort_marble(total_sorted);

            sleep(1);
        }
        else {
            working = SensorValue[bumpSwitch];

            sleep(1);
        }
    }
*/

/*
motor[flashlight] = 127;
    int highest = 0;
    int lowest = 0xffff;
    int val;
    int count = 0;
    while (1) {
        while (count<300) {
            val = SensorValue[light];
            highest = max(val,highest);
            lowest = min(val,lowest);
            count ++;
            sleep(1);
        }
        count = 0;
        writeDebugStreamLine("highest: %i\nlowest: %i\n%i",highest,lowest,val);
        highest = 0;
        lowest = 0xffff;
    }
    */
