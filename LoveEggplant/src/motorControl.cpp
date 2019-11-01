#include "vex.h"
#include "motorControl.h"
#include "GyroLib.h"
#include <ctime>

void gyroInit(){
  Brain.Screen.setPenColor(vex::color::cyan);
  Brain.Screen.setFont(vex::fontType::mono40);
  Brain.Screen.printAt(80, 136, "Initializing...");
      Gyro.startCalibration();
    while(Gyro.isCalibrating()){
      sleep(1);
    }
    int i = 30;
    while (i-->0) {
      Controller.rumble(".");
      sleep(100);
    }
    sleep(3000);
  while(0){
    Gyro.startCalibration();
    while(Gyro.isCalibrating()){
      sleep(1);
    }
    int i = 30;
    while (i-->0) {
      Controller.rumble(".");
      sleep(100);
    }
    double cur = Gyro.value(vex::analogUnits::mV);
    i = 10;
    while (i-->0) {
      Controller.rumble(".");
      sleep(100);
    }
    if(cur == Gyro.value(vex::analogUnits::mV) && Gyro.value(vex::analogUnits::mV) == 0)
      break;
  }
  Controller.rumble(" ");
  Brain.Screen.clearScreen();
}

void ResetMotor(){
  Ldeg = LeftMotor2.rotation(deg);
  Rdeg = RightMotor2.rotation(deg);
  LeftMotor1.setBrake(coast);
  LeftMotor2.setBrake(coast);
  RightMotor1.setBrake(coast);
  RightMotor2.setBrake(coast);
  LeftMotor1.resetRotation();
  LeftMotor2.resetRotation();
  RightMotor1.resetRotation();
  RightMotor2.resetRotation();
  LeftMotor1.setMaxTorque(2, Nm);
  LeftMotor2.setMaxTorque(2, Nm);
  RightMotor1.setMaxTorque(2, Nm);
  RightMotor2.setMaxTorque(2, Nm);
  LeftMotor1.setVelocity(0, pct);
  LeftMotor2.setVelocity(0, pct);
  RightMotor1.setVelocity(0, pct);
  RightMotor2.setVelocity(0, pct);
}

// void Move(int lPower, int rPower){
//   LeftMotor1.spin(fwd, lPower, pct);
//   LeftMotor2.spin(fwd, lPower, pct);
//   RightMotor1.spin(fwd, rPower, pct);
//   RightMotor2.spin(fwd, rPower, pct);
// }

void Move(float lPower, float rPower){
  LeftMotor1.spin(fwd, 0.128 * lPower, voltageUnits::volt);
  LeftMotor2.spin(fwd, 0.128 * lPower, voltageUnits::volt);
  RightMotor1.spin(fwd, 0.128 * rPower, voltageUnits::volt);
  RightMotor2.spin(fwd, 0.128 * rPower, voltageUnits::volt);
}

void sMove(float lPower, float rPower){
  LeftMotor1.spin(fwd, 2 * lPower, rpm);
  LeftMotor2.spin(fwd, 2 * lPower, rpm);
  RightMotor1.spin(fwd, 2 * rPower, rpm);
  RightMotor2.spin(fwd, 2 * rPower, rpm);
}

void Stop(brakeType type){
  LeftMotor1.stop(type);
  LeftMotor2.stop(type);
  RightMotor1.stop(type);
  RightMotor2.stop(type);
}

void Lift(int power){
  if(power == 0){
    LiftMotor.stop(hold);
  }
  else{
    LiftMotor.spin(fwd, power, pct);
  }
}

void Tray(float power, brakeType type, float rotation){
  if(rotation == 0){
    if(power == 0){
      TrayMotor.stop(type);
    }
    else{
      TrayMotor.spin(fwd, 0.128 * power, voltageUnits::volt);
    }
  }
  else{
    TrayMotor.setMaxTorque(2.0, Nm);
    while(TrayMotor.rotation(deg) < rotation){
      Tray(power);
    }
    TrayMotor.stop(type);
  }
}

void Intake(float power){
  LeftIntake.spin(fwd, 0.128 * power, voltageUnits::volt);
  RightIntake.spin(fwd, 0.128 * power, voltageUnits::volt);
}

//////////////////////AUTO CONTROL////////////////////////
void spread(){
  Intake(50);
  sleep(200);
  Intake(0);
  Move(-20, -20);
  while(!LimitBack.pressing()){
    Tray(-100);
  }
  TrayMotor.resetRotation();
  Tray(-20);
  sleep(100);
  Tray(0, hold);
  Intake(100);
  sleep(100);
}

bool goForward(int power, float target, float timeLimit){
	float errL = 0.0;
  float errR = 0.0;
	float last_errL = 0.0;
  float last_errR = 0.0;
  float total_errL = 0.0;
  float total_errR = 0.0;
	float delta_errL = 0.0;
  float delta_errR = 0.0;
	float voltL = 0.0;
  float voltR = 0.0;
  ResetMotor();
  Brain.resetTimer();
	while(Brain.timer(msec) < timeLimit){
    float curL = LeftMotor2.rotation(deg);
    float curR = RightMotor2.rotation(deg);
    errL = target - curL;
    errR = target - curR;
    if(curL / target > KI_START_PERCENT)  total_errL += errL;
    if(curR / target > KI_START_PERCENT)  total_errR += errR;
    delta_errL = errL - last_errL;
    delta_errR = errR - last_errR;
    if(errL < 1 || errR < 1){
      sMove(0,0);
      Move(0,0);
      return true;
    }

    voltL = KP * errL + KI * total_errL + KD * delta_errL;
    voltR = KP * errR + KI * total_errR + KD * delta_errR;

    float acc = CONSTRAIN(Brain.timer(msec) / 1000, 0, 1);
    voltL = voltL * acc;
    voltR = voltR * acc;

    float rpmAdjust = 0.0 * (LeftMotor1.velocity(rpm) - RightMotor2.velocity(rpm));// * (curL / target)
		float rotationAdjust = 0 * (LeftMotor2.rotation(deg) - RightMotor2.rotation(deg));// * (curL / target)
    Move(CONSTRAIN(voltL - rpmAdjust - rotationAdjust, -power, power),
         CONSTRAIN(voltR + rpmAdjust + rotationAdjust, -power, power));

    last_errL = errL;
    last_errR = errR;
		
		sleep(10);
	}
  Stop(hold);
  return false;
}

bool goBackward(int power, float target, float timeLimit){
	float errL = 0.0;
  float errR = 0.0;
	float last_errL = 0.0;
  float last_errR = 0.0;
  float total_errL = 0.0;
  float total_errR = 0.0;
	float delta_errL = 0.0;
  float delta_errR = 0.0;
	float voltL = 0.0;
  float voltR = 0.0;
  LeftMotor2.resetRotation();
  RightMotor2.resetRotation();
  Brain.resetTimer();
	while(Brain.timer(msec) < timeLimit){
    float curL = LeftMotor2.rotation(deg);
    float curR = RightMotor2.rotation(deg);
    errL = target - curL;
    errR = target - curR;
    if(curL / target > KI_START_PERCENT)  total_errL += errL;
    if(curR / target > KI_START_PERCENT)  total_errR += errR;
    delta_errL = errL - last_errL;
    delta_errR = errR - last_errR;
    if(errL > -1 || errR > -1){
      sMove(0,0);
      Move(0,0);
      return true;
    }

    voltL = KP * errL + KI * total_errL + KD * delta_errL;
    voltR = KP * errR + KI * total_errR + KD * delta_errR;

    float acc = CONSTRAIN(Brain.timer(msec) / 1000, 0, 1);
    voltL = voltL * acc;
    voltR = voltR * acc;

    float rpmAdjust = 0 * (LeftMotor1.velocity(rpm) - RightMotor2.velocity(rpm)) * (curL / target);
		float rotationAdjust = 0 * (LeftMotor2.rotation(deg) - RightMotor2.rotation(deg)) * (curL / target);
    Move(CONSTRAIN(voltL + rpmAdjust + rotationAdjust, -power, power),
         CONSTRAIN(voltR - rpmAdjust - rotationAdjust, -power, power));

    last_errL = errL;
    last_errR = errR;
		
		sleep(10);
	}
  Stop(hold);
  return false;
}


bool rushForward(int power, float target, float timeLimit){
  LeftMotor2.resetRotation();
  RightMotor2.resetRotation();
  Brain.resetTimer();
	while(Brain.timer(msec) < timeLimit){
    if((LeftMotor2.rotation(deg) + RightMotor2.rotation(deg)) / 2 > target){
      // Stop(hold);
      return true;
    }
    else  Move(power, power);
  }
  Stop(hold);
  return false;
}

bool rushBackward(int power, float target, float timeLimit){
  LeftMotor2.resetRotation();
  RightMotor2.resetRotation();
  Brain.resetTimer();
	while(Brain.timer(msec) < timeLimit){
    if((LeftMotor2.rotation(deg) + RightMotor2.rotation(deg)) / 2 < target){
      // Stop(hold);
      return true;
    }
    else  Move(-power, -power);
  }
  return false;
}

void backToWall(float power, int dis1, int dis2, int dis3, int dis4, int time, bool left){
  float k1 = 0.5;
  float k2 = 0.3;
  LeftMotor2.resetRotation();
  while(LeftMotor2.rotation(vex::rotationUnits::deg) > -dis1) {
    Move(-power, -power);
  }
  if(left){
    RightMotor2.resetRotation();
    while(RightMotor2.rotation(deg) > -dis2) {
      Move(-power * k1, -power);
    }
  }
  else{
    LeftMotor2.resetRotation();
    while(LeftMotor2.rotation(deg) > -dis2) {
      Move(-power, -power *k1);
    }
  }

  LeftMotor2.resetRotation();
  while(LeftMotor2.rotation(vex::rotationUnits::deg) > -dis3) {
    Move(-100.0, -100.0);
  }

  if(left){
    LeftMotor2.resetRotation();
    while(LeftMotor2.rotation(vex::rotationUnits::deg) > -dis4) {
      Move(-power, -power * k2);
    }
  }
  else{
    RightMotor2.resetRotation();
    while(RightMotor2.rotation(vex::rotationUnits::deg) > -dis4) {
      Move(-power * k2, -power);
    }
  }

  Move(-power, -power);
  sleep(time);
  Stop(hold);
}

bool turnLeft(int power, float target, float timeLimit){
  float errL = 0.0;
  float errR = 0.0;
	float err_lastL = 0.0;
  float err_lastR = 0.0;
	float voltL = 0.0;
  float voltR = 0.0;
	float integralL = 0.0;
  float integralR = 0.0;
	float indexL = 0.0;
  float indexR = 0.0;
  LeftMotor2.resetRotation();
  RightMotor2.resetRotation();
  while(1){
    float curL = LeftMotor2.rotation(deg);
    float curR = RightMotor2.rotation(deg);
		errL = target + curL;
    errR = target - curR;
		
		if(abs((int) errL) > target){
			indexL = 0;
		}
		else if(abs((int) errL) < target * KI_START_PERCENT){
			indexL = 1;
			integralL += errL / 2;
		}
		else{
			indexL = (target - abs((int) errL)) / KI_INDEX_PAR;
			integralL += errL / 2;
		}

    if(abs((int) errR) > target){
			indexR = 0;
		}
		else if(abs((int) errR) < target * KI_START_PERCENT){
			indexR = 1;
			integralR += errR / 2;
		}
		else{
			indexR = (target - abs((int) errR)) / KI_INDEX_PAR;
			integralR += errR / 2;
		}
		
		voltL = KP * errL + indexL * KI * integralL + KD * (errL - err_lastL);
    voltR = KP * errR + indexR * KI * integralR + KD * (errR - err_lastR);
    if(voltL < 0.01 || voltR < 0.01) return true;
		integralL += errL / 2;
    integralR += errR / 2;
		err_lastL = errL;
    err_lastR = errR;
		
    float rpmAdjust = 1.0 * (LeftMotor1.velocity(rpm) - RightMotor2.velocity(rpm));
		float rotationAdjust = 0;//2.0 * (LeftMotor2.rotation(deg) - RightMotor2.rotation(deg));
    Move(CONSTRAIN(- voltL + rpmAdjust + rotationAdjust, -power, 0),
         CONSTRAIN(voltR + rpmAdjust + rotationAdjust, 0, power));
	}
  Stop(hold);
  return false;
}

bool turnRight(int power, float target, float timeLimit){
  float errL = 0.0;
  float errR = 0.0;
	float err_lastL = 0.0;
  float err_lastR = 0.0;
	float voltL = 0.0;
  float voltR = 0.0;
	float integralL = 0.0;
  float integralR = 0.0;
	float indexL = 0.0;
  float indexR = 0.0;
  LeftMotor2.resetRotation();
  RightMotor2.resetRotation();
  while(1){
    float curL = LeftMotor2.rotation(deg);
    float curR = RightMotor2.rotation(deg);
		errL = target - curL;
    errR = target + curR;
		
		if(abs((int) errL) > target){
			indexL = 0;
		}
		else if(abs((int) errL) < target * KI_START_PERCENT){
			indexL = 1;
			integralL += errL / 2;
		}
		else{
			indexL = (target - abs((int) errL)) / KI_INDEX_PAR;
			integralL += errL / 2;
		}

    if(abs((int) errR) > target){
			indexR = 0;
		}
		else if(abs((int) errR) < target * KI_START_PERCENT){
			indexR = 1;
			integralR += errR / 2;
		}
		else{
			indexR = (target - abs((int) errR)) / KI_INDEX_PAR;
			integralR += errR / 2;
		}
		
		voltL = KP * errL + indexL * KI * integralL + KD * (errL - err_lastL);
    voltR = KP * errR + indexR * KI * integralR + KD * (errR - err_lastR);
    if(voltL < 0.01 || voltR < 0.01) return true;
		integralL += errL / 2;
    integralR += errR / 2;
		err_lastL = errL;
    err_lastR = errR;
		
    float rpmAdjust = 1.0 * (LeftMotor1.velocity(rpm) - RightMotor2.velocity(rpm));
		float rotationAdjust = 0;//2.0 * (LeftMotor2.rotation(deg) - RightMotor2.rotation(deg));
    Move(CONSTRAIN(voltL - rpmAdjust - rotationAdjust, 0, power),
         CONSTRAIN(- voltR - rpmAdjust - rotationAdjust, -power, 0));

    sleep(5);
	}
  Stop(hold);
  return false;
}

bool turnLeftWithGyro(int power, float target, float timeLimit, bool fullTime, float P, float I, float D){
  ResetMotor();
  float cur = GyroGetAngle();
  float err = target - cur, last_err = 0, total_err = 0, delta_err = 0, OUT;
  Brain.resetTimer();
  while (Brain.timer(msec) < timeLimit) {
    cur = GyroGetAngle();
    last_err = err;
    err = target - cur;
    if(cur / target > KI_TURN_START_PERCENT)
      total_err += err;
    delta_err = err - last_err;
    if(err >= -0.6) {
      if(!fullTime) return true;
      else if(err <= 0.6) return true;
    }

    OUT = P * err + I * total_err + D * delta_err;
    if(fabs(OUT) < power * 0.1){
      if(OUT > 0) OUT = power * 0.1;
      else        OUT = -(power * 0.1);
    }

    sMove(CONSTRAIN(OUT, -power, power),
          CONSTRAIN(-OUT, -power, power));

    sleep(10);
  }
  Stop(hold);
  return false;
}

bool turnRightWithGyro(int power, float target, float timeLimit, bool fullTime, float P, float I, float D){
  ResetMotor();
  float cur = GyroGetAngle();
  float err = target - cur, last_err = 0, total_err = 0, delta_err = 0, OUT;
  Brain.resetTimer();
  while (Brain.timer(msec) < timeLimit) {
    cur = GyroGetAngle();
    last_err = err;
    err = target - cur;
    if(cur / target > KI_TURN_START_PERCENT)
      total_err += err;
    delta_err = err - last_err;
    if(err <= 0.6) {
      if(!fullTime) return true;
      else if(err >= -0.6)  return true;
    }

    OUT = P * err + I * total_err + D * delta_err;
    if(fabs(OUT) < power * 0.1){
      if(OUT > 0) OUT = power * 0.1;
      else        OUT = -(power * 0.1);
    }

    sMove(CONSTRAIN(OUT, -power, power),
          CONSTRAIN(-OUT, -power, power));

    sleep(10);
  }
  Stop(hold);
  return false;
}

bool turnRightWithGyroL(int power, float target, float timeLimit, bool fullTime, float P, float I, float D){
  ResetMotor();
  float cur = GyroGetAngle();
  float err = target - cur, last_err = 0, total_err = 0, delta_err = 0, OUT;
  Brain.resetTimer();
  while (Brain.timer(msec) < timeLimit) {
    cur = GyroGetAngle();
    last_err = err;
    err = target - cur;
    if(cur / target > KI_TURN_START_PERCENT)
      total_err += err;
    delta_err = err - last_err;
    if(err <= 0.6) {
      if(!fullTime) return true;
      else if(err >= -0.6)  return true;
    }

    OUT = P * err + I * total_err + D * delta_err;
    if(fabs(OUT) < power * 0.1){
      if(OUT > 0) OUT = power * 0.1;
      else        OUT = -(power * 0.1);
    }

    
    LeftMotor1.spin(fwd, 3 * OUT, rpm);
    LeftMotor2.spin(fwd, 3 * OUT, rpm);
    RightMotor1.stop(hold);
    RightMotor2.stop(hold);

    sleep(10);
  }
  Stop(hold);
  return false;
}

bool turnLeftWithGyroR(int power, float target, float timeLimit, bool fullTime, float P, float I, float D){
  ResetMotor();
  float cur = GyroGetAngle();
  float err = target - cur, last_err = 0, total_err = 0, delta_err = 0, OUT;
  Brain.resetTimer();
  while (Brain.timer(msec) < timeLimit) {
    cur = GyroGetAngle();
    last_err = err;
    err = target - cur;
    if(cur / target > KI_TURN_START_PERCENT)
      total_err += err;
    delta_err = err - last_err;
    if(err <= 0.6) {
      if(!fullTime) return true;
      else if(err >= -0.6)  return true;
    }

    OUT = P * err + I * total_err + D * delta_err;
    if(fabs(OUT) < power * 0.1){
      if(OUT > 0) OUT = power * 0.1;
      else        OUT = -(power * 0.1);
    }

    
    RightMotor1.spin(fwd, -(3 * OUT), rpm);
    RightMotor2.spin(fwd, -(3 * OUT), rpm);
    LeftMotor1.stop(hold);
    LeftMotor2.stop(hold);

    sleep(10);
  }
  Stop(hold);
  return false;
}

void autoGoForward(float target,float a,int time1)
{
  float current = 0;
  float vmax = 200;
  float v = 0;
    
  ResetMotor();
  Brain.resetTimer();
  while(current < target -3 && Brain.timer(timeUnits::msec) < time1)
  {
      current = (fabs(RightMotor2.rotation(rotationUnits::deg)) + fabs(LeftMotor2.rotation(rotationUnits::deg)) + fabs(RightMotor1.rotation(rotationUnits::deg)) + fabs(LeftMotor1.rotation(rotationUnits::deg))) / 4;
      
      if((target - vmax*vmax/a) < 0)
      {
          if(current < target/2)
          {
              v = 10.0+sqrt(current*2*a);
          }
          else
          {
              v = sqrt((target-current)*2*a);
          }
      }
      else
      {
          if(current<vmax*vmax/(2*a))
          {
              v=5.0+sqrt(current*2*a);   
          }
          
          else if(current >= (vmax*vmax/(2*a)) && current < (target-vmax*vmax/(2*a)))
          {
              v=vmax;
          }
          else if(current>=(target-vmax*vmax/(2*a)))
          {
              v=sqrt((target-current)*2*a);
          }
    }
    Move(v, v);
    sleep(20);        
  }
  Stop(brake);
  sleep(100);
}


void autoTurnRight(float target,float a,int time1)
{
    float current = 0;
    float vmax = 200;
    float v = 0;
    

    RightMotor2.resetRotation();
    LeftMotor2.resetRotation();
    RightMotor1.resetRotation();
    LeftMotor1.resetRotation();
    Brain.resetTimer();
    while(current < target -3 && Brain.timer(timeUnits::msec) < time1)
    {
        current = (fabs(RightMotor2.rotation(rotationUnits::deg)) + fabs(LeftMotor2.rotation(rotationUnits::deg)) + fabs(RightMotor1.rotation(rotationUnits::deg)) + fabs(LeftMotor1.rotation(rotationUnits::deg))) / 4;
        
        if( current < 0)
        {
             current = -1 * current;
        }
        
        //current = ffabs(current);
        
        if((target - vmax*vmax/a) < 0)
        {
            if(current < target/2)
            {
                v = fabs(10+sqrt(current*2*a));
            }
            else
            {
                v = fabs(sqrt((target-current)*2*a));
            }
        }
        else
        {
            if(current<vmax*vmax/(2*a))
            {
                v=5+sqrt(current*2*a);   
            }
            
            else if(current >= (vmax*vmax/(2*a)) && current < (target-vmax*vmax/(2*a)))
            {
                v=vmax;
            }
            else if(current>=(target-vmax*vmax/(2*a)))
            {
                v=sqrt((target-current)*2*a);
            }
        }
        if(v < 0)
        {
          v = -v;
        }
        
            LeftMotor1.spin(directionType::fwd, v, velocityUnits::rpm);
            LeftMotor2.spin(directionType::fwd, v, velocityUnits::rpm);
            RightMotor1.spin(directionType::rev, v, velocityUnits::rpm);
            RightMotor2.spin(directionType::rev, v, velocityUnits::rpm);
           
        
        
        sleep(20);        
    }
    
    LeftMotor1.stop(brakeType::brake);
    LeftMotor2.stop(brakeType::brake);
    RightMotor1.stop(brakeType::brake);
    RightMotor2.stop(brakeType::brake);
    sleep(100);
}

void autoTurnLeft(float target,float a,int time1)
{
    float current = 0;
    float vmax = 200;
    float v = 0;
    

    RightMotor2.resetRotation();
    LeftMotor2.resetRotation();
    RightMotor1.resetRotation();
    LeftMotor1.resetRotation();
    Brain.resetTimer();
    while(current < target -3 && Brain.timer(timeUnits::msec) < time1)
    {
        current = (fabs(RightMotor2.rotation(rotationUnits::deg)) + fabs(LeftMotor2.rotation(rotationUnits::deg)) + fabs(RightMotor1.rotation(rotationUnits::deg)) + fabs(LeftMotor1.rotation(rotationUnits::deg))) / 4;
        
        if( current < 0)
        {
             current = -1 * current;
        }
        
        //current = fabs(current);
        
        if((target - vmax*vmax/a) < 0)
        {
            if(current < target/2)
            {
                v = fabs(10+sqrt(current*2*a));
            }
            else
            {
                v = fabs(sqrt((target-current)*2*a));
            }
        }
        else
        {
            if(current<vmax*vmax/(2*a))
            {
                v=5+sqrt(current*2*a);   
            }
            
            else if(current >= (vmax*vmax/(2*a)) && current < (target-vmax*vmax/(2*a)))
            {
                v=vmax;
            }
            else if(current>=(target-vmax*vmax/(2*a)))
            {
                v=sqrt((target-current)*2*a);
            }
        }
        if(v < 0)
        {
          v = -v;
        }
        
            LeftMotor1.spin(directionType::rev, v, velocityUnits::rpm);
            LeftMotor2.spin(directionType::rev, v, velocityUnits::rpm);
            RightMotor1.spin(directionType::fwd, v, velocityUnits::rpm);
            RightMotor2.spin(directionType::fwd, v, velocityUnits::rpm);
           
        
        sleep(20);        
    }
    
    LeftMotor1.stop(brakeType::brake);
    LeftMotor2.stop(brakeType::brake);
    RightMotor1.stop(brakeType::brake);
    RightMotor2.stop(brakeType::brake);
    sleep(100);
}