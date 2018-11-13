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

        public WalletWindow(XDagWallet wallet)
        {
            if (wallet == null)
            {
                throw new ArgumentNullException("wallet is null.");
            }

            xdagWallet = wallet;

            InitializeComponent();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            xdagRuntime = new XDagRuntime(xdagWallet);
            
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
        }
        
        private void Load_LocalizedStrings()
        {
            string cultureInfo = walletConfig.ReadOrDefaultCultureInfo;

            Thread.CurrentThread.CurrentUICulture = CultureInfo.GetCultureInfo(cultureInfo);
            this.Title = string.Format("{0} ({1})", Properties.Strings.WalletWindow_Title, walletConfig.Version);

            this.lblBalanceTitle.Content = Properties.Strings.WalletWindow_BalanceTitle;
            this.lblStatusTitle.Content = Properties.Strings.WalletWindow_StatusTitle;
            this.lblAddressTitle.Content = Properties.Strings.WalletWindow_AddressTitle;

            this.btnTransfer.Content = Properties.Strings.WalletWindow_TransferTitle;
            this.lblWalletStatus.Content = WalletStateConverter.Localize(xdagWallet.State);
        }

        private void OnUpdatingState(WalletState state)
        {
            lblWalletStatus.Content = WalletStateConverter.Localize(state);
            
            switch(state)
            {
                case WalletState.ConnectedPool:
                case WalletState.ConnectedAndMining:
                    ellPortrait.Fill = new SolidColorBrush(Colors.Green);
                    break;
                case WalletState.TransferPending:
                    ellPortrait.Fill = new SolidColorBrush(Colors.Orange);
                    break;
                default:
                    break;
            }
        }

        private void OnUpdatingAddress(string address)
        {
            this.txtWalletAddress.Text = string.IsNullOrEmpty(address) ? "[N/A]" : address;
        }

        private void OnUpdatingBalance(double balance)
        {
            this.lblBalance.Content = XDagWallet.BalanceToString(balance);
        }

        private void btnTransfer_Click(object sender, RoutedEventArgs e)
        {
            TransferWindow tWindow = new TransferWindow(xdagWallet);
            tWindow.ShowDialog();
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
    }
}
