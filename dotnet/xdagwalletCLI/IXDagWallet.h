#pragma once
using namespace System;
using namespace System::Collections::Generic;
using namespace System::Runtime::InteropServices;


namespace XDagNetWalletCLI {

	public interface class IXDagWallet
	{
	public:

		///
		/// Returning empty string indicates user cancelled
		/// 
		String^ OnPromptInputPassword(String^ promptMessage, unsigned passwordSize);

		///
		///
		///
		int OnUpdateState(String^ state, String^ balance, String^ address, String^ message);


		void OnStateUpdated(String^ newState);

		void OnBalanceUpdated(String^ newBalance);

		void OnAddressUpdated(String^ address);

		void OnMessage(String^ message);

		void OnError(int errorCode, String^ errorString);

	};
};