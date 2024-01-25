#pragma once
//#include <windows.h>

namespace ModeMuxG2 {

	ref class CANbus;

	ref class StepperMotorController
	{
	public:
		StepperMotorController(System::Windows::Forms::TextBox^ newConsole, CANbus^ newCbus);
		~StepperMotorController();

		void setOffsetFromLimit(float offset);
		void setMovementRange(float range);
		void setLimitSwitchEnd(bool isNegativeEnd);
		void setHasFaultyLimitSwitch(bool hasFaultySwitch);

		float getOffsetFromNegLimit();
		float getMovementRange();

		float getPosition();
		array<bool>^ getLimitSwitchStatus();
		void moveRelative(float relPos, bool useVprofiles);
		void moveAbsolute(float absPos, bool useVprofiles);

		void moveToEnd(bool positiveEnd);

		void setAcceleration(float accel);
		float getAcceleration();
		void setInitVelocity(float velocity);
		float getInitVelocity();
		void setMaxVelocity(float velocity);
		float getMaxVelocity();
		float getCurrentVelocity();

		void datumAtLimitSwitch();
		void centreAndDatum();

		System::String^ getVersionString();

		enum class GEAR_REDUCTION
		{
			DOUBLE_SPEED = (int)8,
			STANDARD = (int)4,
			HALF_SPEED = (int)2,
			QUARTER_SPEED = (int)1
		};


		void setGearboxReductionFactor(GEAR_REDUCTION factor);
		void zeroPositionMeter();

		void restorePowerupSettings(bool zeroPosition);

		bool abortMoves(bool useDecel);

		int getRemoteId();
		void setRemoteId(int ID);

	private:

		delegate void AppendTextCallback(System::String^ text);

		CANbus^ cbus;

		bool needsInit;
		bool remoteIDset;
		bool offsetSet;
		bool rangeSet;

		int remoteID;

		bool negIsLimitSwitch;
		bool faultyLimitSwitch;

		static const int DATALENGTH = 5;
		static const int TIMEOUT = 1000; //in ms
		static float VELOCITYSCALE = 0.0000943;
		static float POSSCALE = 0.0000236; //Figured this out using a micrometer
		static float MAXVELOCITY = 13.1;
		static int MASTERID = 64;

		void Console_WriteLine(System::String^ text);
		System::Windows::Forms::TextBox^ pConsole;

		System::Collections::Queue^ rxMessageQueue;

		void initRoutine();

		void setPosition_raw(float position);
		void move(float pos, bool useVprofiles, bool relative);
		System::String^ getStringFromAscii(int code);

		float maxPosition;
		float minPosition;

		float offsetFromLimit;
		float movementRange;
	};
}

