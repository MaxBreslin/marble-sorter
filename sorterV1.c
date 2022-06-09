#pragma config(Sensor, in1,    line1,          sensorLineFollower)
#pragma config(Sensor, dgtl1,  bump_switch,     sensorTouch)
#pragma config(Sensor, dgtl12, led,            sensorLEDtoVCC)
#pragma config(Motor,  port2,           low_gate,        tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port3,           high_gate,        tmotorVex393_MC29, openLoop)
#pragma config(Motor,  port4,           servo,         tmotorServoStandard, openLoop)
//*!!Code automatically generated by 'ROBOTC' configuration wizard               !!*//


/*
Authored during May-June 2022 by Max Breslin, Abhi Polavarapu, and Ella Kluge. This program is written in RobotC
and is designed to be run on the Vex Cortex 2.0. It should be compiled with the RobotC compiler using the NL library.

-------------------Program Task-------------------
PLTW assignment 3.3.1 details specific criteria and constraints for a marble sorting machine. This program, in
conjunction with the physical marble sorter, was written to meet those requirements and sort 15/15 marbles accurately 
and efficiently.

-------------------Program Flow-------------------
Infinite loop
    Wait for bump switch to be pressed
        Sort all marbles
            |
            V          
        Wait for the line sensor to read a non-normal value (block_until_marble)
        Read and return the current marble (scan_with_line)
        Move the sorting servo to the correct location (rotate_servo)
        Increment total_sorted
            If total_sorted >= 15, repeat the sorting process
            Else, wait for the bump switch to be pressed
*/

// Definitions to improve readability
#define GREEN 0
#define WOOD 1
#define BLACK 2
#define NONE 3

#define min(X, Y) (((X) < (Y)) ? (X) : (Y))
#define max(X, Y) (((X) > (Y)) ? (X) : (Y))

// Default sensor values will be x where 2800 < x < 2900
const int NORMAL_LOW = 2800;
const int NORMAL_HIGH = 2900;

// Threshholds to identify the current marble
const int BLACK_THRESHHOLD = 2920; // greater than
const int GREEN_THRESHHOLD = 2960; // greater than
const int WOOD_THRESHHOLD = 2800; // less than


/*
Bubble sort in C. Accepts a pointer to an array and the size of the array
as input. Sorts the array in-memory. Returns nothing.
*/
void sort(int *arr , int n) {
    int temp = 0;

    for (i = 0; i < n; i++) {
        for (j = 0; j < n-1; j++) {
            if (arr[j] > arr[j+1]) {
                temp = arr[j];
                arr[j] = arr[j+1];
                arr[j+1] = temp;
            }
        }
    }
}

/*
Accepts a pointer to an array and the size of the array as input. Loops through
the array and appends the first 25 valid values to a new array. It then returns
the median of this filtered array.
*/
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

    //return (filtered[25/2]+filtered[(25/2)+1])/2;
    return filtered[25/2];
}

// Blocks execution until a marble is sensed
void block_until_marble() {
    int highest = 0;
    int lowest = 0xffff;
    int reading;

    while (lowest > NORMAL_LOW && highest < NORMAL_HIGH) {
        reading = SensorValue[line1];
        lowest = min(lowest,reading);
        highest = max(highest,reading);

        sleep(1);
    }
}

/*
Records up to 75 readings from the line sensor in wait_for milliseconds. Sorts the recorded
readings and uses the median of the sorted array to determine which marble was detected.
Returns the detected marble.
*/
int scan_with_line(int wait_for=50) {
    int current_marble = NONE;
    int reading;
    int added_nums = 0;
    int readings[75];
    int n = sizeof(readings)/sizeof(readings[0]);

    // Initialize readings array to zeros
    memset(readings,0,n*sizeof(int));

    // This will block execution for ~wait_for ms or until a normal reading is detected
    while (wait_for > 0)  {
        reading = SensorValue[line1];

        if ((reading > NORMAL_LOW && reading < NORMAL_HIGH) || added_nums >= 75) {
            break;
        }

        readings[added_nums] = reading;
        added_nums ++;

        wait_for --;
        sleep(1);
    }

    sort(readings,sizeof(readings)/sizeof(readings[0]));

    int median_reading = get_median(readings, sizeof(readings)/sizeof(readings[0]));

    writeDebugStreamLine("median reading: %i",median_reading);


    if (median_reading > GREEN_THRESHHOLD) {
        current_marble = GREEN;
    }
    else if (median_reading > BLACK_THRESHHOLD) {
        current_marble = BLACK;
    }
    else if (median_reading < WOOD_THRESHHOLD) {
        current_marble = WOOD;
    }

    return current_marble;
}


// Rotates the servo to the position specified by this_marble
void rotate_servo(int &this_marble) {
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

/*
Master function that calls the previously declared functions in a specific order.
1. Execution is blocked until a marble is detected
2. The type of marble is determined
3. The servo is rotated to match the detected marble
4. The passed variable, total_sorted, is incremented
5. Returns false if all marbles have been sorted : True otherwise
*/
bool sort_marble(int &total_sorted) {
    block_until_marble();
    int this_marble = scan_with_line();

    rotate_servo(this_marble);

    total_sorted ++;
    if (total_sorted>=15) {
        total_sorted = 0;
        return false;
    }
    return true;
}

/*
Simple task that reverses the direction of the low_gate motor periodically.
Implemented to prevent marble jams.
*/
task reverse_spin() {
    while (1) {
        if (bMotorReflected[low_gate]) {
            bMotorReflected[low_gate] = 0;
            sleep(2000);
        }
        else {
            bMotorReflected[low_gate] = 1;
            sleep(1500);
        }
    }
}

task main() {
    int total_sorted = 0;

    // Controls which parts of the sorter are functioning at any given time
    bool working = false;

    startTask(reverse_spin);

    while(1) {
        SensorValue[led] = working;

        motor[low_gate] = working ? 14 : 0;
        motor[high_gate] = working ? -17 : 0;

        if (working) {
            writeDebugStreamLine("%i",total_sorted);
            working = sort_marble(total_sorted);

            sleep(1);
        }
        else {
            working = SensorValue[bump_switch];

            sleep(1);
        }
    }
}
