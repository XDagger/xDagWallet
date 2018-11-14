using System;
using System.Collections.Generic;
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

namespace XDagNetWallet.UI.Windows
{
    /// <summary>
    /// Interaction logic for PasswordWindow.xaml
    /// </summary>
    public partial class PasswordWindow : Window
    {
        private Action<string> passwordHandler = null;

        public PasswordWindow(string prompt, Action<string> passwordHandler)
        {
            InitializeComponent();

            this.Title = prompt;
            this.lblPrompt.Content = prompt;

            this.passwordHandler = passwordHandler;
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            this.txtPassword.Focus();

            this.btnConfirm.Content = Properties.Strings.PasswordWindow_Enter;
        }

        private void txtPassword_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                ConfirmClose();
            }
        }

        private void ConfirmClose()
        {
            string password = txtPassword.Password.Trim();
            this.passwordHandler?.Invoke(password);

            // Note: to ensure the userInputPassword is set after the PasswordWindow closed, we need to sleep for a while
            Thread.Sleep(20);

            this.Close();
        }

        private void btnConfirm_Click(object sender, RoutedEventArgs e)
        {
            ConfirmClose();
        }
    }
}
