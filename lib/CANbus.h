#pragma once
//#include <windows.h>

namespace ModeMuxG2 {
	ref class CANbus
	{
	public:
		CANbus(System::Windows::Forms::TextBox^ newConsole);
		~CANbus();

		ref struct messageWrapper
		{
			bool validMessage;
			int ID;
			array<unsigned char>^ data;
			System::Int64 timestamp;
		};

		void setBitrate(int newBitrate);
		int getBitrate();
		void clearReceiveQueue();
		messageWrapper^ waitForMessage(int senderID, int timeout);
		bool sendMessage(int remoteID, array<unsigned char>^ messageData);

	private:


		delegate void AppendTextCallback(System::String^ text);
		void initRoutine();

		int CANchannel;
		System::Int32 ChannelHandle;
		int bitrate;
		bool needsInit;
		bool isBitrateSet;

		static const int TIMEOUT = 1000; //in ms
		
		void Console_WriteLine(System::String^ text);
		System::Windows::Forms::TextBox^ pConsole;
	};
}

