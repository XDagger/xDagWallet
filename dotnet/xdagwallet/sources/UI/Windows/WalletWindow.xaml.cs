using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using XDagNetWallet.Components;
using XDagNetWallet.UI.Windows;
using XDagNetWalletCLI;
using XDagNetWallet.Interop;
using XDagNetWallet.UI.Async;
using System.Timers;
using System.Collections.ObjectModel;

namespace XDagNetWallet.UI.Windows
{
    /// <summary>
    /// Interaction logic for WalletWindow.xaml
    /// </summary>
    public partial class WalletWindow : Window
    {
        private XDagWallet xdagWallet = null;

        private XDagRuntime xdagRuntime = null;

        private WalletConfig walletConfig = WalletConfig.Current;

        private System.Timers.Timer refreshWalletDataTimer = null;

        private System.Timers.Timer refreshWalletDataRepeatTimer = null;

        private System.Timers.Timer fillTransactionHistoryTimer = null;

        private readonly object refreshWalletDataLock = new object();

        private ObservableCollection<XDagTransaction> transactionHistory = new ObservableCollection<XDagTransaction>();

        private HashSet<XDagTransaction> fillingTransactionHistory = new HashSet<XDagTransaction>();

        private RefreshingOperation refreshingTransactionHistory = null;

        public WalletWindow(XDagWallet wallet)
        {
            if (wallet == null)
            {
                throw new ArgumentNullException("wallet is null.");
            }

            InitializeComponent();

            xdagWallet = wallet;

            refreshWalletDataTimer = new System.Timers.Timer();
            refreshWalletDataTimer.AutoReset = false;   // Only be run once
            refreshWalletDataTimer.Interval = 500;
            refreshWalletDataTimer.Elapsed += new ElapsedEventHandler(this.OnRefreshWalletData);

            refreshWalletDataRepeatTimer = new System.Timers.Timer();
            refreshWalletDataRepeatTimer.Interval = 10 * 1000;
            refreshWalletDataRepeatTimer.Elapsed += new ElapsedEventHandler(this.OnRefreshWalletData);

            fillTransactionHistoryTimer = new System.Timers.Timer();
            fillTransactionHistoryTimer.Interval = 1000;
            fillTransactionHistoryTimer.Elapsed += new ElapsedEventHandler(this.OnFillTransactionHistoryData);
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            xdagRuntime = new XDagRuntime(xdagWallet);
            refreshingTransactionHistory = new RefreshingOperation(() => { this.LoadTransactionHistory(); }, TimeSpan.FromMinutes(1));

            xdagWallet.SetPromptInputPasswordFunction((prompt, passwordSize) =>
            {
                return this.Dispatcher.Invoke(() =>
                {
                    return InputPassword(prompt, passwordSize);
                });
            });

            xdagWallet.SetStateChangedAction((state) =>
            {
                this.Dispatcher.Invoke(() =>
                {
                    OnUpdatingState(state);
                });
            });

            xdagWallet.SetAddressChangedAction((address) =>
            {
                this.Dispatcher.Invoke(() =>
                {
                    OnUpdatingAddress(address);
                });
            });

            xdagWallet.SetBalanceChangedAction((balance) =>
            {
                this.Dispatcher.Invoke(() =>
                {
                    OnUpdatingBalance(balance);
                });
            });

            OnUpdatingState(xdagWallet.State);
            OnUpdatingAddress(xdagWallet.Address);
            OnUpdatingBalance(xdagWallet.Balance);

            Load_LocalizedStrings();

            xdagRuntime.RefreshData();

            refreshWalletDataRepeatTimer.Start();
            
            dgdTransactionHistorySimple.ItemsSource = transactionHistory;
            fillTransactionHistoryTimer.Start();
        }

        private void Load_LocalizedStrings()
        {
            string cultureInfo = walletConfig.ReadOrDefaultCultureInfo;

            Thread.CurrentThread.CurrentUICulture = CultureInfo.GetCultureInfo(cultureInfo);
            this.Title = string.Format("{0} ({1})", Properties.Strings.WalletWindow_Title, walletConfig.Version);
            this.lblVersion.Content = string.Format(Properties.Strings.WalletWindow_VersionLabel, walletConfig.Version);

            this.lblBalanceTitle.Content = Properties.Strings.WalletWindow_BalanceTitle;
            this.lblAddressTitle.Content = Properties.Strings.WalletWindow_AddressTitle;
            
            this.btnTransfer.Content = Properties.Strings.WalletWindow_TransferTitle;
            this.lblWalletStatus.Content = WalletStateConverter.Localize(xdagWallet.State);

            this.tabAccount.Header = Properties.Strings.WalletWindow_TabAccount;
            this.tabTransfer.Header = Properties.Strings.WalletWindow_TabTransfer;
            this.tabHistory.Header = Properties.Strings.WalletWindow_TabHistory;

            // Transfer
            this.lblTransferToAddress.Content = Properties.Strings.WalletWindow_Transfer_ToAddress;
            this.lblTransferAmount.Content = Properties.Strings.WalletWindow_Transfer_Amount;

            // DataGrid for TransactionHistory
            this.tabHistory.Header = Properties.Strings.WalletWindow_TabHistory;
            ////this.biTransactionHistoryLoading.BusyContent = Properties.Strings.WalletWindow_HistoryBusy;
            this.grdBusyIndicatorText.Text = Properties.Strings.WalletWindow_HistoryBusy;

            this.transactionColumn_TimeStamp.Header = Properties.Strings.WalletWindow_HistoryColumns_TimeStamp;
            this.transactionColumn_Direction.Header = Properties.Strings.WalletWindow_HistoryColumns_Direction;
            this.transactionColumn_Amount.Header = Properties.Strings.WalletWindow_HistoryColumns_Amount;
            this.transactionColumn_Status.Header = Properties.Strings.WalletWindow_HistoryColumns_Status;
            this.transactionColumn_BlockAddress.Header = Properties.Strings.WalletWindow_HistoryColumns_BlockAddress;
            this.transactionColumn_PartnerAddress.Header = Properties.Strings.WalletWindow_HistoryColumns_PartnerAddress;

        }

        private String InputPassword(String promptMessage, uint passwordSize)
        {
            string userInputPassword = string.Empty;
            PasswordWindow passwordWindow = new PasswordWindow(Properties.Strings.PasswordWindow_InputPassword, (passwordInput) =>
            {
                userInputPassword = passwordInput;
            });
            passwordWindow.ShowDialog();

            Thread.Sleep(50);

            return userInputPassword;
        }

        private void OnUpdatingState(WalletState state)
        {
            lblWalletStatus.Content = WalletStateConverter.Localize(state);

            refreshingTransactionHistory.ResetTime();

            switch(state)
            {
                case WalletState.ConnectedPool:
                case WalletState.ConnectedAndMining:
                    imgStatus.Source = new BitmapImage(new Uri(@"/XDagWallet;component/Resources/icon-status-online.png", UriKind.Relative));
                    break;
                case WalletState.TransferPending:
                    imgStatus.Source = new BitmapImage(new Uri(@"/XDagWallet;component/Resources/icon-status-pending.png", UriKind.Relative));
                    break;
                default:
                    imgStatus.Source = new BitmapImage(new Uri(@"/XDagWallet;component/Resources/icon-status-offline.png", UriKind.Relative));
                    break;
            }

            // Set up a timer to trigger
            refreshWalletDataTimer.Start();
        }

        private void OnUpdatingAddress(string address)
        {
            this.txtWalletAddress.Text = string.IsNullOrEmpty(address) ? "[N/A]" : address;
        }

        private void OnUpdatingBalance(double balance)
        {
            this.lblBalance.Content = XDagWallet.BalanceToString(balance);
        }

        private void StartRefreshWalletData()
        {
            
        }

        private void OnRefreshWalletData(object sender, ElapsedEventArgs e)
        {
            lock (refreshWalletDataLock)
            {
                xdagRuntime.RefreshData();
                fillingTransactionHistory.Clear();
            }
        }

        private void OnFillTransactionHistoryData(object sender, ElapsedEventArgs e)
        {
            DateTime startTime = e.SignalTime;

            foreach (XDagTransaction transaction in transactionHistory)
            {
                if (startTime + TimeSpan.FromSeconds(15) < DateTime.Now)
                {
                    return;
                }

                if (!string.IsNullOrWhiteSpace(transaction.PartnerAddress))
                {
                    continue;
                }

                if (fillingTransactionHistory.Contains(transaction))
                {
                    continue;
                }
                
                try
                {
                    fillingTransactionHistory.Add(transaction);

                    XDagTransactionProvider.FillTransactionData(xdagWallet.Address, transaction);

                    this.Dispatcher.Invoke(() =>
                    {
                        this.dgdTransactionHistorySimple.Items.Refresh();
                    });
                }
                catch (Exception)
                {
                    // Ignore the exception
                }
                finally
                {
                    try
                    { 
                        fillingTransactionHistory.Remove(transaction);
                    }
                    catch (Exception)
                    {
                        // Ignore the exception
                    }
                }
            }
        }

        private void btnTransfer_Click(object sender, RoutedEventArgs e)
        {
            GenerateTransaction();
        }

        private void btnCopyWalletAddress_Click(object sender, RoutedEventArgs e)
        {
            Clipboard.SetText(this.txtWalletAddress.Text);
            MessageBox.Show(Properties.Strings.WalletWindow_AddressCopied);
        }

        private void btnLangZH_Click(object sender, RoutedEventArgs e)
        {
            SwitchLanguage("zh-Hans");
        }

        private void btnLangEN_Click(object sender, RoutedEventArgs e)
        {
            SwitchLanguage("en-US");
        }

        private void SwitchLanguage(string cultureString)
        {
            walletConfig.CultureInfo = cultureString;
            walletConfig.SaveToFile();

            Load_LocalizedStrings();
        }

        private void btnSettings_Click(object sender, RoutedEventArgs e)
        {
            SettingsWindow settings = new SettingsWindow();
            settings.ShowDialog();


            Load_LocalizedStrings();
        }

        private void btnExit_Click(object sender, RoutedEventArgs e)
        {
            Environment.Exit(0);
        }

        private void Rectangle_MouseDown(object sender, MouseButtonEventArgs e)
        {
            if (e.ChangedButton != MouseButton.Left)
            {
                return;
            }

            this.DragMove();
        }

        private void GenerateTransaction()
        {
            double amount = 0;

            string amountString = this.txtTransferAmount.Text.Trim();
            if (!double.TryParse(amountString, out amount) || amount <= 0)
            {
                MessageBox.Show(Properties.Strings.TransferWindow_AmountFormatError);
                return;
            }

            if (amount > xdagWallet.Balance)
            {
                MessageBox.Show(Properties.Strings.TransferWindow_InsufficientAmount);
                return;
            }

            string targetWalletAddress = this.txtTransferTargetAddress.Text.Trim();
            if (!xdagRuntime.ValidateWalletAddress(targetWalletAddress))
            {
                MessageBox.Show(Properties.Strings.TransferWindow_AccountFormatError);
                return;
            }

            string confirmMessage = string.Format(Properties.Strings.TransferWindow_ConfirmTransfer, amount, targetWalletAddress);
            MessageBoxResult result = MessageBox.Show(confirmMessage, Properties.Strings.Common_ConfirmTitle, MessageBoxButton.YesNo);
            if (result == MessageBoxResult.No)
            {
                return;
            }

            BackgroundWork.CreateWork(
                this,
                () => {
                    this.materialTabControl.IsEnabled = false;
                    ShowStatus(Properties.Strings.TransferWindow_CommittingTransaction);
                },
                () => {
                    xdagRuntime.TransferToAddress(targetWalletAddress, amount);

                    return 0;
                },
                (taskResult) => {

                    if (taskResult.HasError)
                    {
                        if (taskResult.Exception is PasswordIncorrectException)
                        {
                            MessageBox.Show(Properties.Strings.Message_PasswordIncorrect, Properties.Strings.Common_MessageTitle);
                        }
                        else
                        {
                            MessageBox.Show(Properties.Strings.TransferWindow_CommitFailed + taskResult.Exception.Message);
                        }
                    }
                    else
                    {
                        // Success
                        MessageBox.Show(Properties.Strings.TransferWindow_CommitSuccess, Properties.Strings.Common_MessageTitle);
                        this.txtTransferAmount.Text = string.Empty;
                        this.tabAccount.IsSelected = true;
                    }

                    this.materialTabControl.IsEnabled = true;
                    HideStatus();
                }
            ).Execute();
        }


        private void ShowStatus(string message)
        {
            //this.lblWalletStatus.Visibility = Visibility.Visible;
            //this.lblWalletStatus.Content = message;

            this.progressBar.Visibility = Visibility.Visible;
            this.progressBar.IsIndeterminate = true;
        }

        private void HideStatus()
        {
            //this.lblWalletStatus.Visibility = Visibility.Hidden;
            this.progressBar.Visibility = Visibility.Hidden;
            this.progressBar.IsIndeterminate = false;
        }

        private void tabHistory_Loaded(object sender, RoutedEventArgs e)
        {
            
        }

        private void tabHistory_GotFocus(object sender, RoutedEventArgs e)
        {
            if (xdagWallet == null)
            {
                return;
            }

            refreshingTransactionHistory.Refresh();
        }

        private void LoadTransactionHistory()
        {
            BackgroundWork< List<XDagTransaction>>.CreateWork(
                this,
                () => {
                    ////biTransactionHistoryLoading.IsBusy = true;
                    this.grdBusyIndicator.Visibility = Visibility.Visible;
                    this.tabItemHistory.IsEnabled = false;
                },
                () => {
                    List<XDagTransaction> transactions = XDagTransactionProvider.GetTransactionHistory(xdagWallet.Address);
                    return transactions;
                },
                (taskResult) => {

                    ////biTransactionHistoryLoading.IsBusy = false;
                    this.grdBusyIndicator.Visibility = Visibility.Hidden;
                    this.tabItemHistory.IsEnabled = true;

                    if (taskResult.HasError)
                    {
                        // Do nothing
                        return;
                    }

                    List<XDagTransaction> transactions = taskResult.Result;

                    // Merge the Transaction results into the current history
                    foreach (XDagTransaction tran in transactions)
                    {
                        if (!transactionHistory.Any(t => (t.BlockAddress == tran.BlockAddress)))
                        {
                            transactionHistory.Add(tran);
                        }
                        else
                        {
                            XDagTransaction existing = transactionHistory.FirstOrDefault(t => (t.BlockAddress == tran.BlockAddress));
                            existing.MergeWith(tran);
                        }
                    }

                    /*
                    foreach(XDagTransaction transaction in transactionHistory)
                    {
                        LoadTransactionDataAsync(transaction);
                    }
                    */
                }
            ).Execute();
        }

        private void LoadTransactionDataAsync(XDagTransaction transaction)
        {
            if (!string.IsNullOrWhiteSpace(transaction.PartnerAddress))
            {
                return;
            }

            BackgroundWork.CreateWork(
                this,
                () => {
                },
                () => {
                    XDagTransactionProvider.FillTransactionData(xdagWallet.Address, transaction);
                    return 0;
                },
                (taskResult) => {
                    this.dgdTransactionHistorySimple.Items.Refresh();
                }
            ).Execute();
        }
    }
}
