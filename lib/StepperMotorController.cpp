#include "StepperMotorController.h"
#include "CANbus.h"

using namespace ModeMuxG2;
using namespace System;

StepperMotorController::StepperMotorController(System::Windows::Forms::TextBox^ newConsole, CANbus^ newCbus)
{
	pConsole = newConsole;
	cbus = newCbus;

	needsInit = true;
	remoteIDset = false;
	offsetSet = false;
	rangeSet = false;
	negIsLimitSwitch = true;
}
StepperMotorController::~StepperMotorController()
{
}
void StepperMotorController::initRoutine()
{
	if (needsInit && remoteIDset && offsetSet && rangeSet)
	{
		needsInit = false;
		restorePowerupSettings(false);
		setAcceleration(0.0);
		setMaxVelocity(1);
		setInitVelocity(1);
		setGearboxReductionFactor(GEAR_REDUCTION::STANDARD);
	}
}
void StepperMotorController::setRemoteId(int ID)
{
	remoteID = ID;
	remoteIDset = true;
	needsInit = true;
	initRoutine();
}

int StepperMotorController::getRemoteId()
{
	return remoteID;
}

void StepperMotorController::setOffsetFromLimit(float offset)
{
	offsetFromLimit = offset;
	minPosition = offsetFromLimit - movementRange / 2;
	maxPosition = offsetFromLimit + movementRange / 2;
	offsetSet = true;
	initRoutine();
}
void StepperMotorController::setMovementRange(float range)
{
	movementRange = range;
	minPosition = offsetFromLimit - movementRange / 2;
	maxPosition = offsetFromLimit + movementRange / 2;
	rangeSet = true;
	initRoutine();
}
void StepperMotorController::setLimitSwitchEnd(bool isNegativeEnd)
{
	negIsLimitSwitch = isNegativeEnd;
}
void StepperMotorController::setHasFaultyLimitSwitch(bool hasFaultySwitch)
{
	faultyLimitSwitch = hasFaultySwitch;
}

float StepperMotorController::getOffsetFromNegLimit()
{
	return offsetFromLimit;
}

float StepperMotorController::getMovementRange()
{
	return movementRange;
}

float StepperMotorController::getPosition()
{
	array<unsigned char>^ messageData = gcnew array<unsigned char>(DATALENGTH);
	messageData[0] = '?';
	messageData[1] = 'P';

	float result = -10000000;

	initRoutine();
	if (!needsInit)
	{
		cbus->sendMessage(remoteID, messageData);

		CANbus::messageWrapper^ receivedMessage = cbus->waitForMessage(MASTERID, TIMEOUT);

		if (receivedMessage->validMessage)
		{
			Int32 rawNum = receivedMessage->data[1] + ((unsigned int)receivedMessage->data[2] << 8) + ((unsigned int)receivedMessage->data[3] << 16) + (((unsigned int)receivedMessage->data[4]) << 24);
			result = rawNum * POSSCALE;
		}
		else
		{
			Console_WriteLine("Error: no reply received");
		}
	}
	return result;
}
array<bool>^ StepperMotorController::getLimitSwitchStatus()
{
	array<unsigned char>^ messageData = gcnew array<unsigned char>(DATALENGTH);
	messageData[0] = '?';
	messageData[1] = 'B';

	array<bool>^ result = gcnew array<bool>(2);

	initRoutine();
	if (!needsInit)
	{
		cbus->sendMessage(remoteID, messageData);

		CANbus::messageWrapper^ receivedMessage = cbus->waitForMessage(MASTERID, TIMEOUT);

		if (receivedMessage->validMessage)
		{
			unsigned char rawNum = receivedMessage->data[1];
			rawNum = (rawNum & 0x06) >> 1;
			result[0] = rawNum & 0x01;
			result[1] = (rawNum & 0x02) >> 1;
		}
		else
		{
			Console_WriteLine("Error: no reply received");
		}
	}
	return result;
}
void StepperMotorController::setPosition_raw(float position)
{
	if (position > maxPosition)
	{
		//FIX For now, do nothing
		//position = maxPosition;
	}
	if (position < minPosition)
	{
		//position = minPosition;
	}
	array<unsigned char>^ messageData = gcnew array<unsigned char>(DATALENGTH);
	messageData[0] = 'P';

	int scaledPos = (position / POSSCALE + 0.5);
	messageData[1] = scaledPos & 0x000000FF;
	messageData[2] = (scaledPos >> 8) & 0x000000FF;
	messageData[3] = (scaledPos >> 16) & 0x000000FF;
	messageData[4] = (scaledPos >> 24) & 0x000000FF;

	initRoutine();
	if (!needsInit)
	{
		cbus->sendMessage(remoteID, messageData);
	}
}
void StepperMotorController::moveAbsolute(float absPos, bool useVprofiles)
{
	move(absPos, useVprofiles, false);
}
void StepperMotorController::moveRelative(float relPos, bool useVprofiles)
{
	move(relPos, useVprofiles, true);
}
void StepperMotorController::move(float pos, bool useVprofiles, bool relative)
{
	if (useVprofiles)
	{
		//setAcceleration(2); //Removed for testing
	}
	else
	{
		//setAcceleration(VELOCITYSCALE); //Removed for testing
	}

	float newPos;
	float currentPos = getPosition();
	
	if (relative)
	{
		newPos = currentPos + pos;
	}
	else
	{
		newPos = pos;
	}
	
	float distance = Math::Abs(newPos - currentPos);
	float speed = getMaxVelocity();
	float expectedTime = distance / speed * 1000;// in ms
	float timeOut = Math::Max((float)1.5*expectedTime, (float)TIMEOUT);

	setPosition_raw(newPos);

	//Not sure why this necessary?
	array<unsigned char>^ messageData = gcnew array<unsigned char>(DATALENGTH);
	messageData[0] = 'O';

	initRoutine();
	if (!needsInit)
	{
		cbus->sendMessage(remoteID, messageData);
		CANbus::messageWrapper^ receivedMessage = cbus->waitForMessage(MASTERID, timeOut);

		if (receivedMessage->validMessage)
		{
		}
		else
		{
			Console_WriteLine("Error: expected reply not received for command O");
		}
	}
}
void StepperMotorController::moveToEnd(bool positiveEnd)
{
	array<unsigned char>^ messageData = gcnew array<unsigned char>(DATALENGTH);

	if (positiveEnd)
	{
		messageData[0] = '+';
	}
	else
	{
		messageData[0] = '-';
	}

	initRoutine();
	if (!needsInit)
	{
		cbus->sendMessage(remoteID, messageData);
	}

	messageData[0] = 'C'; //not sure what this does
	messageData[1] = 1;
	if (!needsInit)
	{
		cbus->sendMessage(remoteID, messageData);
	}

	//setAcceleration(VELOCITYSCALE); Removed for testing
	//setInitVelocity(1); Removed for testing

	messageData[0] = 'G'; //not sure what this does
	messageData[1] = 0;
	if (!needsInit)
	{
		cbus->sendMessage(remoteID, messageData);
	}

	messageData[0] = 'O'; //not sure what this does
	if (!needsInit)
	{
		cbus->sendMessage(remoteID, messageData);
	}

	messageData[0] = 'C'; //not sure what this does
	if (!needsInit)
	{
		cbus->sendMessage(remoteID, messageData);
	}
}
void StepperMotorController::setAcceleration(float accel)
{
	if (accel > MAXVELOCITY)
	{
		accel = MAXVELOCITY;
	}
	array<unsigned char>^ messageData = gcnew array<unsigned char>(DATALENGTH);
	messageData[0] = 'S';
	
	int scaledAccel = (accel/VELOCITYSCALE+0.5);
	messageData[1] = scaledAccel & 0x000000FF;
	messageData[2] = (scaledAccel >> 8) & 0x000000FF;
	messageData[3] = (scaledAccel >> 16) & 0x000000FF;
	messageData[4] = (scaledAccel >> 24) & 0x000000FF;

	initRoutine();
	if (!needsInit)
	{
		cbus->sendMessage(remoteID, messageData);
	}
}
float StepperMotorController::getAcceleration()
{
	array<unsigned char>^ messageData = gcnew array<unsigned char>(DATALENGTH);
	messageData[0] = '?';
	messageData[1] = 'S';

	float result = -10000000;

	initRoutine();
	if (!needsInit)
	{
		cbus->sendMessage(remoteID, messageData);

		CANbus::messageWrapper^ receivedMessage = cbus->waitForMessage(MASTERID, TIMEOUT);

		if (receivedMessage->validMessage)
		{
			Int32 rawNum = receivedMessage->data[1] + ((unsigned int)receivedMessage->data[2] << 8) + ((unsigned int)receivedMessage->data[3] << 16) + (((unsigned int)receivedMessage->data[4]) << 24);

			result = rawNum * VELOCITYSCALE;
		}
		else
		{
			Console_WriteLine("Error: no reply received");
		}
	}
	return result;
}
void StepperMotorController::setInitVelocity(float velocity)
{
	if (velocity > MAXVELOCITY)
	{
		velocity = MAXVELOCITY;
	}
	array<unsigned char>^ messageData = gcnew array<unsigned char>(DATALENGTH);
	messageData[0] = 'F';

	int scaledVeloc = (velocity / VELOCITYSCALE + 0.5);
	messageData[1] = scaledVeloc & 0x000000FF;
	messageData[2] = (scaledVeloc >> 8) & 0x000000FF;
	messageData[3] = (scaledVeloc >> 16) & 0x000000FF;
	messageData[4] = (scaledVeloc >> 24) & 0x000000FF;

	initRoutine();
	if (!needsInit)
	{
		cbus->sendMessage(remoteID, messageData);
	}
}
float StepperMotorController::getInitVelocity()
{
	array<unsigned char>^ messageData = gcnew array<unsigned char>(DATALENGTH);
	messageData[0] = '?';
	messageData[1] = 'F';

	float result = -10000000;

	initRoutine();
	if (!needsInit)
	{
		cbus->sendMessage(remoteID, messageData);

		CANbus::messageWrapper^ receivedMessage = cbus->waitForMessage(MASTERID, TIMEOUT);

		if (receivedMessage->validMessage)
		{
			Int32 rawNum = receivedMessage->data[1] + ((unsigned int)receivedMessage->data[2] << 8) + ((unsigned int)receivedMessage->data[3] << 16) + (((unsigned int)receivedMessage->data[4]) << 24);

			result = rawNum * VELOCITYSCALE;
		}
		else
		{
			Console_WriteLine("Error: no reply received");
		}
	}
	return result;
}
void StepperMotorController::setMaxVelocity(float velocity)
{
	if (velocity > MAXVELOCITY)
	{
		velocity = MAXVELOCITY;
	}
	array<unsigned char>^ messageData = gcnew array<unsigned char>(DATALENGTH);
	messageData[0] = 'R';

	int scaledVeloc = (velocity / VELOCITYSCALE + 0.5);
	messageData[1] = scaledVeloc & 0x000000FF;
	messageData[2] = (scaledVeloc >> 8) & 0x000000FF;
	messageData[3] = (scaledVeloc >> 16) & 0x000000FF;
	messageData[4] = (scaledVeloc >> 24) & 0x000000FF;

	initRoutine();
	if (!needsInit)
	{
		cbus->sendMessage(remoteID, messageData);
	}
}
float StepperMotorController::getMaxVelocity()
{
	array<unsigned char>^ messageData = gcnew array<unsigned char>(DATALENGTH);
	messageData[0] = '?';
	messageData[1] = 'R';

	float result = -10000000;

	initRoutine();
	if (!needsInit)
	{
		cbus->sendMessage(remoteID, messageData);

		CANbus::messageWrapper^ receivedMessage = cbus->waitForMessage(MASTERID, TIMEOUT);

		if (receivedMessage->validMessage)
		{
			Int32 rawNum = receivedMessage->data[1] + ((unsigned int)receivedMessage->data[2] << 8) + ((unsigned int)receivedMessage->data[3] << 16) + (((unsigned int)receivedMessage->data[4]) << 24);

			result = rawNum * VELOCITYSCALE;
		}
		else
		{
			Console_WriteLine("Error: no reply received");
		}
	}
	return result;
}
float StepperMotorController::getCurrentVelocity()
{
	array<unsigned char>^ messageData = gcnew array<unsigned char>(DATALENGTH);
	messageData[0] = '?';
	messageData[1] = 'V';

	float result = -10000000;

	initRoutine();
	if (!needsInit)
	{
		cbus->sendMessage(remoteID, messageData);

		CANbus::messageWrapper^ receivedMessage = cbus->waitForMessage(MASTERID, TIMEOUT);

		if (receivedMessage->validMessage)
		{
			Int32 rawNum = receivedMessage->data[1] + ((unsigned int)receivedMessage->data[2] << 8) + ((unsigned int)receivedMessage->data[3] << 16) + (((unsigned int)receivedMessage->data[4]) << 24);

			result = rawNum * VELOCITYSCALE;
		}
		else
		{
			Console_WriteLine("Error: no reply received");
		}
	}
	return result;
}
void StepperMotorController::datumAtLimitSwitch()
{
	moveToEnd(!negIsLimitSwitch);


	if (faultyLimitSwitch)
	{
		float result = getCurrentVelocity();
		bool hitLimit = (result == 0);

		while (!hitLimit)
		{
			result = getCurrentVelocity();
			hitLimit = (result == 0);
		}
	}
	else
	{
		array<bool>^ result = getLimitSwitchStatus();
		bool hitLimit = result[1];

		while (!hitLimit)
		{
			result = getLimitSwitchStatus();
			hitLimit = result[1];
		}
	}

	zeroPositionMeter();
}
void StepperMotorController::centreAndDatum()
{
	datumAtLimitSwitch();
	moveAbsolute(offsetFromLimit + movementRange / 2,false);
	while (getCurrentVelocity() != 0);
	zeroPositionMeter();
}
System::String^ StepperMotorController::getVersionString()
{
	array<unsigned char>^ messageData = gcnew array<unsigned char>(DATALENGTH);
	messageData[0] = '?';
	messageData[1] = 16;

	String^ result = "";

	initRoutine();
	if (!needsInit)
	{
		cbus->sendMessage(remoteID, messageData);

		CANbus::messageWrapper^ receivedMessage = cbus->waitForMessage(MASTERID, TIMEOUT);

		if (receivedMessage->validMessage)
		{

			for (int k = 0; k < receivedMessage->data->Length; k++)
			{
				result += getStringFromAscii(receivedMessage->data[k]);
			}

		}
		else
		{
			Console_WriteLine("Error: no reply received");
		}
	}
	return result;
}
void StepperMotorController::setGearboxReductionFactor(GEAR_REDUCTION gearboxMode)
{
	array<unsigned char>^ messageData = gcnew array<unsigned char>(DATALENGTH);
	messageData[0] = 'M';
	messageData[1] = (unsigned char)gearboxMode;
	initRoutine();
	if (!needsInit)
	{
		cbus->sendMessage(remoteID, messageData);
	}
}
void StepperMotorController::zeroPositionMeter()
{
	//This is also called setting the Datum point in some literature
	array<unsigned char>^ messageData = gcnew array<unsigned char>(DATALENGTH);
	messageData[0] = 'A';
	initRoutine();
	if (!needsInit)
	{
		cbus->sendMessage(remoteID, messageData);
	}
}
void StepperMotorController::restorePowerupSettings(bool zeroPosition)
{
	array<unsigned char>^ messageData = gcnew array<unsigned char>(DATALENGTH);
	messageData[0] = 'I'; //Not sure what this does.  Seems to reset some of the settings
	//cbus->sendMessage(remoteID, messageData);

	if (zeroPosition)
	{
		zeroPositionMeter();
	}
	//setGearboxReductionFactor(GEAR_REDUCTION::STANDARD); //Removed for testing

	messageData[0] = '>'; //Not sure what this does
	messageData[2] = 2;
	initRoutine();
	if (!needsInit)
	{
		cbus->sendMessage(remoteID, messageData);
	}

	messageData[0] = '<'; //Not sure what this does
	messageData[2] = 4;
	if (!needsInit)
	{
		cbus->sendMessage(remoteID, messageData);
	}

	needsInit = false;
}
bool StepperMotorController::abortMoves(bool useDecel)
{
	array<unsigned char>^ messageData = gcnew array<unsigned char>(DATALENGTH);
	messageData[0] = 'K';

	if (useDecel)
	{
		messageData[1] = 1;
	}
	else
	{
		messageData[1] = 0;
	}

	initRoutine();
	if (!needsInit)
	{
		cbus->sendMessage(remoteID, messageData);

		CANbus::messageWrapper^ receivedMessage = cbus->waitForMessage(MASTERID, TIMEOUT);

		if (receivedMessage->validMessage)
		{
			//FIX how to check for correct response?
			Console_WriteLine("Move aborted");
		}
		else
		{
			Console_WriteLine("Error: no reply received");
		}

		//Added this in myself - seems to relax the motors? Actually, it reduces the speed
		//messageData[0] = 'I';
		//messageData[1] = 0;
		//status = Canlib::canWrite(0, remoteID, messageData, messageData->Length, 0);

		return receivedMessage->validMessage;
	}
	else
	{
		return false;
	}
}
String^ StepperMotorController::getStringFromAscii(int code)
{
	//Seems unecessary to do this but I couldn't find another way to convert!
	char temp[] = "a";
	temp[0] = (char)code;
	String^ result = gcnew String(temp);
	return result;
}
void StepperMotorController::Console_WriteLine(System::String^ text)
{
	try
	{
		if (this->pConsole->InvokeRequired)
		{
			AppendTextCallback^ d = gcnew AppendTextCallback(pConsole, &System::Windows::Forms::TextBox::AppendText);
			pConsole->Invoke(d, text + System::Environment::NewLine);
		}
		else
		{
			pConsole->Text = pConsole->Text + text + System::Environment::NewLine;
			pConsole->SelectionStart = pConsole->Text->Length;
			pConsole->ScrollToCaret();
			System::Diagnostics::Debug::WriteLine(text);
		}
	}
	catch (System::ObjectDisposedException^ exp)
	{
		System::Diagnostics::Debug::WriteLine(exp);
	}
	catch (System::InvalidOperationException^ exp)
	{
		System::Diagnostics::Debug::WriteLine(exp);
	}
}