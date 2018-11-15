using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using XDagNetWallet.Interop;
using XDagNetWallet.Utils;
using XDagNetWalletCLI;

namespace XDagNetWallet.Components
{
    public class XDagWallet : IXDagWallet
    {
        private Func<string, uint, string> promptInputPasswordFunc = null;
        private Func<string, string, string, int> updateStateFunc = null;

        private Action<WalletState> onUpdatingState = null;
        private Action<double> onUpdatingBalance = null;
        private Action<string> onUpdatingAddress = null;
        private Action<string> onMessage = null;
        private Action<XDagErrorCode> onError = null;

        private WalletOptions walletOptions = null;

        public WalletState walletState = WalletState.None;

        private Logger logger = Logger.GetInstance();

        public static string BalanceToString(double balance)
        {
            if (balance >= 1000)
            {
                return String.Format("{0:0,0.000000000}", balance);
            }
            else
            {
                return String.Format("{0:0.000000000}", balance);
            }
        }

        public XDagWallet(WalletOptions walletOptions = null)
        {
            if (walletOptions == null)
            {
                this.walletOptions = WalletOptions.Default();
            }
        }

        public WalletState State
        {
            get
            {
                return walletState;
            }
            set
            {
                walletState = value;
            }
        }

        public string GetLocalizedState()
        {
            return WalletStateConverter.Localize(this.walletState);
        }

        public string PaswordPlain
        {
            get; set;
        }

        public string RandomKey
        {
            get; set;
        }

        public string PublicKey
        {
            get; set;
        }

        public double Balance
        {
            get; set;
        }

        public string Address
        {
            get; set;
        }

        public bool IsConnected
        {
            get
            {
                return (walletState == WalletState.ConnectedPool
                    || walletState == WalletState.ConnectedAndMining
                    || walletState == WalletState.Synchronizing
                    || walletState == WalletState.Synchronized
                    || walletState == WalletState.TransferPending);
            }
        }

        public string BalanceString
        {
            get
            {
                return BalanceToString(this.Balance);
            }
        }

        public void SetPromptInputPasswordFunction(Func<string, uint, string> f)
        {
            this.promptInputPasswordFunc = f;
        }

        public void SetUpdateStateFunction(Func<string, string, string, int> f)
        {
            this.updateStateFunc = f;
        }

        public void SetBalanceChangedAction(Action<double> f)
        {
            this.onUpdatingBalance = f;
        }

        public void SetAddressChangedAction(Action<string> f)
        {
            this.onUpdatingAddress = f;
        }

        public void SetStateChangedAction(Action<WalletState> f)
        {
            this.onUpdatingState = f;
        }

        public void SetMessageAction(Action<string> f)
        {
            this.onMessage = f;
        }

        public void SetErrorAction(Action<XDagErrorCode> f)
        {
            this.onError = f;
        }

        #region Interface Methods

        /// <summary>
        /// 
        /// </summary>
        /// <param name="promptMessage"></param>
        /// <param name="passwordSize"></param>
        /// <returns></returns>
        public string OnPromptInputPassword(string promptMessage, uint passwordSize)
        {
            return this.promptInputPasswordFunc?.Invoke(promptMessage, passwordSize);
        }


        public void OnStateUpdated(string newState)
        {
            WalletState newStateEnum = WalletStateConverter.ConvertFromMessage(newState);

            if (newStateEnum != WalletState.None && newStateEnum != walletState)
            {
                this.onUpdatingState?.Invoke(newStateEnum);
                walletState = newStateEnum;
            }
        }

        public void OnBalanceUpdated(string newBalance)
        {
            double balanceValue = 0;
            if (double.TryParse(newBalance, out balanceValue))
            {
                this.onUpdatingBalance?.Invoke(balanceValue);
                this.Balance = balanceValue;
            }
        }

        public void OnAddressUpdated(string address)
        {
            if (IsValidAddress(address))
            {
                this.onUpdatingAddress?.Invoke(address);
                this.Address = address;
            }
        }

        public void OnMessage(string message)
        {
            this.onMessage?.Invoke(message);
        }

        public void OnError(int errorCode, string errorMessage)
        {
            XDagErrorCode error = XDagErrorHandler.ParseErrorCode(errorCode);
            this.onError?.Invoke(error);
        }

        #endregion

        /// <summary>
        /// This should be legacy
        /// </summary>
        /// <param name="state"></param>
        /// <param name="balance"></param>
        /// <param name="address"></param>
        /// <returns></returns>
        public int OnUpdateState(string state, string balance, string address, string message)
        {
            logger.Trace(string.Format("State=[{0}] Message=[{1}]", state, message));
            
            return 0;
        }

        public static bool IsValidAddress(string addressString)
        {
            if (string.IsNullOrEmpty(addressString))
            {
                return false;
            }

            if (addressString.Length != 32)
            {
                return false;
            }

            return true;
        }

    }
}
