#include "CANbus.h"

using namespace ModeMuxG2;
using namespace System;
using namespace canlibCLSNET;

CANbus::CANbus(System::Windows::Forms::TextBox^ newConsole)
{
	pConsole = newConsole;

	//FIX should only do this once
	canlibCLSNET::Canlib::canInitializeLibrary();

	CANchannel = 0;
	bitrate = 125000;

	needsInit = true;
	isBitrateSet = false;

	ChannelHandle = Canlib::canOpenChannel(CANchannel, 0);
}
void CANbus::initRoutine()
{
	if (isBitrateSet && needsInit)
	{
		Canlib::canStatus status = Canlib::canBusOn(ChannelHandle);
		if (status == Canlib::canStatus::canOK)
		{
			Console_WriteLine("CAN bus opened successfully");
			needsInit = false;
		}
		else
		{
			Console_WriteLine("CAN bus error: failed to open");
		}
	}
}
CANbus::~CANbus()
{
	Canlib::canStatus status;
	if (!needsInit)
	{
		status = Canlib::canBusOff(ChannelHandle);
	}
	status = Canlib::canClose(ChannelHandle);
}
CANbus::messageWrapper^ CANbus::waitForMessage(int senderID, int timeout)
{
	messageWrapper^ currentMessage = gcnew messageWrapper;
	currentMessage->validMessage = false;

	if (!needsInit)
	{
		bool notTimedOut = true;

		Int64 flags_;

		Canlib::canStatus status;
		int count = 0;

		do
		{
			Int32 dlc;
			Int32 identifier;
			Int32 flags;
			Int64 timestamp;
			array<unsigned char, 1> ^message_ = gcnew array<unsigned char>(8);

			if (count == 0)
			{
				status = Canlib::canReadWait(ChannelHandle, identifier, message_, dlc, flags, timestamp, timeout);
			}
			else
			{
				status = Canlib::canRead(ChannelHandle, identifier, message_, dlc, flags, timestamp);
			}
			int test = (int)status;
			//System::Diagnostics::Debug::WriteLine("Status2 = " + test);

			if (status == Canlib::canStatus::canOK)
			{
				array<unsigned char>^ message = gcnew array<unsigned char>(dlc);

				for (int k = 0; k < dlc; k++)
				{
					message[k] = message_[k];
				}

				if (flags & Canlib::canMSG_ERROR_FRAME)
				{
					System::Diagnostics::Debug::WriteLine("Error frame received, timestamp: " + timestamp);
				}
				else
				{
					if (identifier == senderID) //The code for replies from slave devices
					{
						currentMessage->ID = senderID;
						currentMessage->validMessage = true;
						currentMessage->data = message;
						currentMessage->timestamp = timestamp;
						//rxMessageQueue->Enqueue(currentMessage); //Not currently implemented - only returns one message
						//System::Diagnostics::Debug::WriteLine("Count = " + rxMessageQueue->Count);
					}
				}
			}
			else
			{
				if (count == 0)
				{
					notTimedOut = false;
				}
			}
			count++;
		} while (status == canlibCLSNET::Canlib::canStatus::canOK);
	}
	return currentMessage;
}
bool CANbus::sendMessage(int remoteID, array<unsigned char>^ messageData)
{
	bool success;
	if (!needsInit)
	{
		clearReceiveQueue();
		Canlib::canStatus status = Canlib::canWrite(ChannelHandle, remoteID, messageData, messageData->Length, 0);
		if (status == Canlib::canStatus::canOK)
		{
			success = true;
		}
		else
		{
			success = false;
		}
	}
	else
	{
		success = false;
	}
	return success;
}
void CANbus::clearReceiveQueue()
{
	Canlib::canFlushReceiveQueue(ChannelHandle);
}
void CANbus::setBitrate(int newBitrate)
{
	int bitrateFlag;
	bitrate = newBitrate;

	//FIX for all bitrates
	switch (newBitrate)
	{
	case 10000:
		bitrateFlag = Canlib::canBITRATE_10K;
		break;
	case 50000:
		bitrateFlag = Canlib::canBITRATE_50K;
		break;
	case 62000:
		bitrateFlag = Canlib::canBITRATE_62K;
		break;
	case 83000:
		bitrateFlag = Canlib::canBITRATE_83K;
		break;
	case 100000:
		bitrateFlag = Canlib::canBITRATE_100K;
		break;
	case 125000:
		bitrateFlag = Canlib::canBITRATE_125K;
		break;
	case 250000:
		bitrateFlag = Canlib::canBITRATE_250K;
		break;
	case 500000:
		bitrateFlag = Canlib::canBITRATE_500K;
		break;
	case 1000000:
		bitrateFlag = Canlib::canBITRATE_1M;
		break;
	default:
		bitrateFlag = Canlib::canBITRATE_125K;
		break;
	}

	Canlib::canStatus status = Canlib::canSetBusParams(ChannelHandle, bitrateFlag, 0, 0, 0, 0, 0);

	isBitrateSet = true;
	initRoutine();
}

int CANbus::getBitrate()
{
	return bitrate;
}

//void StepperMotorController::messageProcessWindowSpawn_thread()
//{
//	windowProcDel = gcnew CallbackFuncDelegateWindowProc(this, &StepperMotorController::messageProc);
//	wndclass = new WNDCLASS;
//
//	HINSTANCE hInstance = GetModuleHandle(NULL);
//	DEVMODE dmScreenSettings;                   // Device Mode
//
//	memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));       // Makes Sure Memory's Cleared
//	const LPCWSTR appname = TEXT("CAN message processor");
//
//	WNDPROC pMainWindFunc = (WNDPROC)System::Runtime::InteropServices::Marshal::GetFunctionPointerForDelegate(windowProcDel).ToPointer();
//
//	// Define the window class
//	if (!wndclassRegistered)
//	{
//		wndclass->style = 0;
//		wndclass->lpfnWndProc = pMainWindFunc;
//		wndclass->cbClsExtra = 0;
//		wndclass->cbWndExtra = 0;
//		wndclass->hInstance = hInstance;
//		wndclass->hIcon = LoadIcon(hInstance, appname);
//		wndclass->hCursor = LoadCursor(NULL, IDC_ARROW);
//		wndclass->hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
//		wndclass->lpszMenuName = appname;
//		wndclass->lpszClassName = appname;
//		// Register the window class
//		RegisterClass(wndclass);
//		//if (!RegisterClass(&wndclass)) return FALSE;
//	}
//	wndclassRegistered = true;
//
//	if (!hWnd)
//	{
//		hWnd = new HWND;
//	}
//
//	*hWnd = CreateWindowEx(0, appname,
//		appname,
//		WS_DISABLED,
//		0,
//		0,
//		0,
//		0,
//		HWND_MESSAGE,
//		NULL,
//		hInstance,
//		NULL);
//
//	//while (1);
//	//////////
//}


//void CALLBACK StepperMotorController::messageProc(HWND hWnd, UINT uMsg, WPARAM  wParam, LPARAM  lParam)
//{
//	array<unsigned char, 1> ^message_ = gcnew array<unsigned char>(8);
//
//	if (uMsg == Canlib::WM__CANLIB)
//	{
//		Canlib::canStatus status;
//		switch (lParam) 
//		{
//			case Canlib::canEVENT_RX:
//				System::Diagnostics::Debug::WriteLine("canEVENT_RX");
//				break;
//			case Canlib::canEVENT_TX:
//				System::Diagnostics::Debug::WriteLine("canEVENT_TX");
//				break;
//			case Canlib::canEVENT_ERROR:
//				System::Diagnostics::Debug::WriteLine("canEVENT_ERROR");
//				break;
//			case Canlib::canEVENT_STATUS:
//				System::Diagnostics::Debug::WriteLine("canEVENT_STATUS");
//				break;
//			default:
//				System::Diagnostics::Debug::WriteLine("*** UNKNOWN EVENT ***");
//				break;
//		}
//
//		do
//		{
//			Int32 dlc;
//			Int32 flags;
//			Int32 identifier;
//			Int64 timestamp;
//			status = Canlib::canRead(ChannelHandle, identifier, message_, dlc, flags, timestamp);
//
//			array<unsigned char>^ message = gcnew array<unsigned char>(dlc);
//
//			for (int k = 0; k < dlc; k++)
//			{
//				message[k] = message_[k];
//			}
//
//			if (status == Canlib::canStatus::canOK)
//			{
//				if (flags & Canlib::canMSG_ERROR_FRAME)
//				{
//					System::Diagnostics::Debug::WriteLine("Error frame received, timestamp: " + timestamp);
//				}
//				else
//				{
//					if (identifier == 64) //The code for replies from slave devices
//					{
//						messageWrapper^ currentMessage = gcnew messageWrapper;
//						currentMessage->data = message;
//						currentMessage->timestamp = timestamp;
//						rxMessageQueue->Enqueue(currentMessage);
//						System::Diagnostics::Debug::WriteLine("Count = "+rxMessageQueue->Count);
//					}
//				}
//			}
//		}
//		while ((status == Canlib::canStatus::canOK));
//
//		if (status != Canlib::canStatus::canERR_NOMSG)
//		{
//			System::Diagnostics::Debug::WriteLine("canRead", status);
//		}
//	}
//}

void CANbus::Console_WriteLine(System::String^ text)
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