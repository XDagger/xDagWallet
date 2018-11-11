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
using XDagNetWallet.UI.Async;
using XDagNetWalletCLI;

namespace XDagNetWallet.UI.Windows
{
    /// <summary>
    /// Interaction logic for TransferWindow.xaml
    /// </summary>
    public partial class TransferWindow : Window
    {
        private XDagWallet xdagWallet = null;

        private XDagRuntime xdagRuntime = null;

        private WalletConfig walletConfig = WalletConfig.Current;

        public TransferWindow(XDagWallet wallet)
        {
            InitializeComponent();

            this.xdagWallet = wallet;
            this.xdagRuntime = new XDagRuntime(wallet);
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            xdagWallet.SetPromptInputPasswordFunction((prompt, passwordSize) =>
            {
                return this.Dispatcher.Invoke(() =>
                {
                    return InputPassword(prompt, passwordSize);
                });
            });

            Load_LocalizedStrings();
        }

        private void Load_LocalizedStrings()
        {
            string cultureInfo = walletConfig.ReadOrDefaultCultureInfo;

            Thread.CurrentThread.CurrentUICulture = CultureInfo.GetCultureInfo(cultureInfo);
            this.Title = Properties.Strings.TransferWindow_Title;

            this.lblAddressTitle.Content = Properties.Strings.TransferWindow_AddressTitle;
            this.btnTransfer.Content = Properties.Strings.TransferWindow_TransferTitle;
            this.btnCancel.Content = Properties.Strings.Common_Cancel;

            this.lblWalletStatus.Content = WalletStateConverter.Localize(xdagWallet.State);
        }

        private void btnCancel_Click(object sender, RoutedEventArgs e)
        {
            this.Close();
        }

        private void btnTransfer_Click(object sender, RoutedEventArgs e)
        {
            DoTransfer();
        }

        private String InputPassword(String promptMessage, uint passwordSize)
        {
            string userInputPassword = string.Empty;
            PasswordWindow passwordWindow = new PasswordWindow(Properties.Strings.PasswordWindow_InputPassword, (passwordInput) =>
            {
                userInputPassword = passwordInput;
            });
            passwordWindow.ShowDialog();
            
            return userInputPassword;
        }

        private void DoTransfer()
        {
            double amount = 0;

            string amountString = this.txtAmount.Text.Trim();
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

            string targetWalletAddress = this.txtTargetWalletAddress.Text.Trim();
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
                    this.btnTransfer.IsEnabled = false;
                    this.btnCancel.IsEnabled = false;
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
                        MessageBox.Show(Properties.Strings.TransferWindow_CommitSuccess, Properties.Strings.Common_MessageTitle);
                        this.Close();
                    }

                    this.btnTransfer.IsEnabled = true;
                    this.btnCancel.IsEnabled = true;
                    HideStatus();
                }
            ).Execute();
        }

        private void ShowStatus(string message)
        {
            this.lblWalletStatus.Visibility = Visibility.Visible;
            this.lblWalletStatus.Content = message;

            this.prbProgress.Visibility = Visibility.Visible;
            this.prbProgress.IsIndeterminate = true;
        }

        private void HideStatus()
        {
            this.lblWalletStatus.Visibility = Visibility.Hidden;
            this.prbProgress.Visibility = Visibility.Hidden;
            this.prbProgress.IsIndeterminate = false;
        }

        private void txtAmount_TextChanged(object sender, TextChangedEventArgs e)
        {
        }

        private void txtAmount_KeyDown(object sender, KeyEventArgs e)
        {
        }
    }

}