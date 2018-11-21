#pragma once

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Runtime::InteropServices;

#include "IXDagWallet.h"

#pragma unmanaged

#include "xdag_runtime.h"

// external stdcall methods connects with xdag_runtime.h
typedef int(__stdcall * InputPasswordStd)(const char*, char*, unsigned);
typedef int(__stdcall * ShowStateStd)(const char*, const char*, const char*);

typedef int(__stdcall * EventCallbackStd)(void*, xdag_event *);

#pragma managed

namespace XDagNetWalletCLI {

	public ref class XDagRuntime
	{
	public:

		XDagRuntime(IXDagWallet^ wallet);
		~XDagRuntime();

		// Delegate method to pass to unmanaged code
		[UnmanagedFunctionPointer(CallingConvention::StdCall)]
		delegate int InputPasswordDelegate(const char*, char*, unsigned);
		[UnmanagedFunctionPointer(CallingConvention::StdCall)]
		delegate int ShowStateDelegate(const char*, const char*, const char*);

		[UnmanagedFunctionPointer(CallingConvention::StdCall)]
		delegate int EventCallbackDelegate(void*, xdag_event *);


		// Interop methods to translate the parameters from unmanaged code to managed code
		int InputPassword(const char *prompt, char *buf, unsigned size);
		int OnUpdateState(const char *state, const char *balance, const char *address);

		int EventCallback(void* obj, xdag_event * eve);

		void Start(String^ poolAddress);

		bool HasExistingAccount();

		void DoTesting();

		void TransferToAddress(String^ toAddress, double amount);

		bool ValidateWalletAddress(String^ address);

		void RefreshData();

	private:

		GCHandle gch;

		IXDagWallet^ xdagWallet;

		String^ ConvertFromConstChar(const char* str);

		const char* ConvertFromString(String^ str);

		System::IntPtr pooAddressPtr;
	};

}
