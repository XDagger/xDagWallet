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

namespace XDagNetWallet.UI.Windows
{
    /// <summary>
    /// Interaction logic for SettingsWindow.xaml
    /// </summary>
    public partial class SettingsWindow : Window
    {
        private WalletConfig walletConfig = null;


        public SettingsWindow()
        {
            InitializeComponent();

            walletConfig = WalletConfig.Current;
        }


        private void btnConfirm_Click(object sender, RoutedEventArgs e)
        {
            if (this.rdbLanguageZH.IsChecked ?? false)
            {
                walletConfig.CultureInfo = "zh-Hans";
            }
            else
            {
                walletConfig.CultureInfo = "en-US";
            }

            walletConfig.SaveToFile();

            this.Close();
        }

        private void btnCancel_Click(object sender, RoutedEventArgs e)
        {
            this.Close();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            if(walletConfig.ReadOrDefaultCultureInfo == "zh-Hans")
            {
                this.rdbLanguageZH.IsChecked = true;
            }
            else
            {
                this.rdbLanguageEN.IsChecked = true;
            }

            Load_LocalizedStrings();
        }

        private void Load_LocalizedStrings()
        {
            string cultureInfo = walletConfig.ReadOrDefaultCultureInfo;
            Thread.CurrentThread.CurrentUICulture = CultureInfo.GetCultureInfo(cultureInfo);

            btnConfirm.Content = Properties.Strings.Common_Confirm;
            btnCancel.Content = Properties.Strings.Common_Cancel;
            grpLanguage.Header = Properties.Strings.SettingsWindow_ChooseLanguage;

            this.Title = Properties.Strings.SettingsWindow_Title;

        }
    }
}
