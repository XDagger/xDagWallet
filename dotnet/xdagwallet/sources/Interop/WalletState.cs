using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XDagNetWallet.Interop
{
    public enum WalletState
    {
        None = 0,               // Not an account
        Registering = 10,       // Creating a new wallet and waiting for response

        Idle = 20,              // Not connected to network

        LoadingBlocks = 30,     // Loading blocks from the local storage.
        Stopped = 40,           // Blocks loaded. Waiting for 'run' command.

        ConnectingNetwork = 50, // Trying to connect to the network.
        ConnectedNetwork = 55,  // Connected to the network

        ConnectingPool = 60,    // Trying to connect to the pool.

        ConnectedPool = 65,     // Connected to the pool. No mining.
        ConnectedAndMining = 67,// Connected to the pool. Mining on. Normal operation.

        Synchronizing = 70,     // Synchronizing with the network
        Synchronized = 75,     // Synchronized with the network

        TransferPending = 80,   // Waiting for transfer to complete.

        ResetingEngine = 90,    // The local storage is corrupted. Resetting blocks engine.
    }

    public class WalletStateConverter
    {
        private static Dictionary<string, WalletState> stateMessageDictionary = null;

        static WalletStateConverter()
        {
            stateMessageDictionary = new Dictionary<string, WalletState>();

            stateMessageDictionary.Add("Generating keys...", WalletState.Registering);
            stateMessageDictionary.Add("Synchronized with the main network. Normal operation.", WalletState.Synchronized);
            stateMessageDictionary.Add("Synchronized with the test network. Normal testing.", WalletState.Synchronized);
            stateMessageDictionary.Add("Connected to the mainnet pool. Mining on. Normal operation.", WalletState.ConnectedAndMining);
            stateMessageDictionary.Add("Connected to the testnet pool. Mining on. Normal testing.", WalletState.ConnectedAndMining);
            stateMessageDictionary.Add("Connected to the mainnet pool. No mining.", WalletState.ConnectedPool);
            stateMessageDictionary.Add("Connected to the testnet pool. No mining.", WalletState.ConnectedPool);
            stateMessageDictionary.Add("Waiting for transfer to complete.", WalletState.TransferPending);
            stateMessageDictionary.Add("Connected to the main network. Synchronizing.", WalletState.Synchronizing);
            stateMessageDictionary.Add("Connected to the test network. Synchronizing.", WalletState.Synchronizing);
            stateMessageDictionary.Add("Trying to connect to the mainnet pool.", WalletState.ConnectingPool);
            stateMessageDictionary.Add("Trying to connect to the testnet pool.", WalletState.ConnectingPool);
            stateMessageDictionary.Add("Trying to connect to the main network.", WalletState.ConnectingNetwork);
            stateMessageDictionary.Add("Trying to connect to the test network.", WalletState.ConnectingNetwork);
            stateMessageDictionary.Add("Blocks loaded. Waiting for 'run' command.", WalletState.Idle);
            stateMessageDictionary.Add("Loading blocks from the local storage.", WalletState.LoadingBlocks);
            stateMessageDictionary.Add("The local storage is corrupted. Resetting blocks engine.", WalletState.ResetingEngine);
            
        }

        public static WalletState ConvertFromMessage(string message)
        {
            if (stateMessageDictionary != null && stateMessageDictionary.ContainsKey(message))
            {
                return stateMessageDictionary[message];
            }

            return WalletState.None;
        }

        public static string Localize(WalletState state)
        {
            switch(state)
            {
                case WalletState.None:
                    return Properties.Strings.WalletState_None;
                case WalletState.Registering:
                    return Properties.Strings.WalletState_Registering;
                case WalletState.Idle:
                    return Properties.Strings.WalletState_Idle;
                case WalletState.LoadingBlocks:
                    return Properties.Strings.WalletState_LoadingBlocks;
                case WalletState.Stopped:
                    return Properties.Strings.WalletState_Stopped;
                case WalletState.ConnectingNetwork:
                    return Properties.Strings.WalletState_ConnectingNetwork;
                case WalletState.ConnectedNetwork:
                    return Properties.Strings.WalletState_ConnectedNetwork;
                case WalletState.ConnectingPool:
                    return Properties.Strings.WalletState_ConnectingPool;
                case WalletState.ConnectedPool:
                    return Properties.Strings.WalletState_ConnectedPool;
                case WalletState.ConnectedAndMining:
                    return Properties.Strings.WalletState_ConnectedAndMining;
                case WalletState.Synchronizing:
                    return Properties.Strings.WalletState_Synchronizing;
                case WalletState.Synchronized:
                    return Properties.Strings.WalletState_Synchronized;
                case WalletState.TransferPending:
                    return Properties.Strings.WalletState_TransferPending;

                case WalletState.ResetingEngine:
                    return Properties.Strings.WalletState_ResetingEngine;
                default:
                    return string.Empty;
            }
        }
    }

}
