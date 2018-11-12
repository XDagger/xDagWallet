﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
/// using System.Timers;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using XDagNetWallet.Components;
using XDagNetWallet.UI.Async;
using XDagNetWallet.Utils;
using XDagNetWalletCLI;
using System.Threading;
using System.Globalization;

namespace XDagNetWallet.UI.Windows
{
    /// <summary>
    /// Interaction logic for LogonWindow.xaml
    /// </summary>
    public partial class LogonWindow : Window
    {
        private enum LogonStatus
        {
            None,
            Registering,
            RegisteringConfirmPassword,
            RegisteringRandomKeys,
            RegisteringPending,
            Connecting,
            ConnectingPending,
        }

        private LogonStatus logonStatus = LogonStatus.None;

        private Logger xdagLogger = Logger.GetInstance();

        private WalletConfig walletConfig = WalletConfig.Current;

        private XDagWallet xdagWallet = null;
        private XDagRuntime runtime = null;

        private string userInputPassword = string.Empty;

        private System.Timers.Timer registeringTimer = null;
        private DateTime registeringTimerStartTime = DateTime.MinValue;

        public LogonWindow()
        {
            InitializeComponent();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            xdagWallet = new XDagWallet();
            runtime = new XDagRuntime(xdagWallet);

            registeringTimer = new System.Timers.Timer();
            registeringTimer.Interval = 1000;
            registeringTimer.Elapsed += new System.Timers.ElapsedEventHandler(this.OnRegisteringTimerTick);
            

            Load_LocalizedStrings();
            
            xdagWallet.SetPromptInputPasswordFunction((prompt, passwordSize) =>
            {
                return this.Dispatcher.Invoke(() =>
                {
                    return InputPassword(prompt, passwordSize);
                });
            });

            xdagWallet.SetUpdateStateFunction((state, balance, address) =>
            {
                return this.Dispatcher.Invoke(() =>
                {
                    return UpdateState(state, balance, address);
                });
            });

            xdagWallet.SetStateChangedAction((state) =>
            {
                this.Dispatcher.Invoke(() =>
                {
                    UpdateState(state);
                });
            });

            if (!runtime.HasExistingAccount())
            {
                btnRegisterAccount.Visibility = Visibility.Visible;
                btnConnectAccount.Visibility = Visibility.Hidden;
            }
            else
            {
                btnConnectAccount.Visibility = Visibility.Visible;
                btnRegisterAccount.Visibility = Visibility.Hidden;
            }
        }

        private void Load_LocalizedStrings()
        {
            string cultureInfo = walletConfig.ReadOrDefaultCultureInfo;

            Thread.CurrentThread.CurrentUICulture = CultureInfo.GetCultureInfo(cultureInfo);

            btnConnectAccount.Content = Properties.Strings.LogonWindow_ConnectWallet;
            btnRegisterAccount.Content = Properties.Strings.LogonWindow_RegisterWallet;
            this.Title = string.Format("{0} ({1})", Properties.Strings.LogonWindow_Title, walletConfig.Version);
        }

        private void btnRegisterAccount_Click(object sender, RoutedEventArgs e)
        {
            RegisterAccount();
        }

        private void btnConnectAccount_Click(object sender, RoutedEventArgs e)
        {
            ConnectAccount();
        }

        private void OnRegisteringTimerTick(object sender, System.Timers.ElapsedEventArgs e)
        {
            if (registeringTimer == null)
            {
                return;
            }

            if (logonStatus != LogonStatus.Registering && logonStatus != LogonStatus.RegisteringConfirmPassword
                && logonStatus != LogonStatus.RegisteringRandomKeys && logonStatus != LogonStatus.RegisteringPending)
            {
                registeringTimer.Stop();
                this.Dispatcher.Invoke(()=> { HideStatus(); });
            }

            TimeSpan timeSpan = DateTime.UtcNow - registeringTimerStartTime;
            string message = string.Format(Properties.Strings.LogonWindow_InitializingElapsedTime, timeSpan.ToString(@"mm\:ss"));
            this.Dispatcher.Invoke(() => {
                this.lblWalletStatus.Content = message;
            });
        }

        private void StartRegisteringTimer()
        {
            if (registeringTimer == null)
            {
                return;
            }

            this.lblWalletStatus.Visibility = Visibility.Visible;

            registeringTimerStartTime = DateTime.UtcNow;
            registeringTimer.Start();
        }

        private void RegisterAccount()
        {
            if (runtime == null || logonStatus != LogonStatus.None)
            {
                return;
            }

            PasswordWindow passwordWindow = new PasswordWindow(Properties.Strings.PasswordWindow_SetPassword, (passwordInput) =>
            {
                userInputPassword = passwordInput;
            });
            passwordWindow.ShowDialog();

            if (string.IsNullOrEmpty(userInputPassword))
            {
                return;
            }

            string confirmedPassword = string.Empty;
            passwordWindow = new PasswordWindow(Properties.Strings.PasswordWindow_RetypePassword, (passwordInput) =>
            {
                confirmedPassword = passwordInput;
            });
            passwordWindow.ShowDialog();
            if (!string.Equals(userInputPassword, confirmedPassword))
            {
                MessageBox.Show(Properties.Strings.PasswordWindow_PasswordMismatch);
                return;
            }

            btnRegisterAccount.IsEnabled = false;
            btnLang.IsEnabled = false;
            BackgroundWork.CreateWork(
                this,
                () => {
                    logonStatus = LogonStatus.Registering;
                    StartRegisteringTimer();

                    ShowStatus(Properties.Strings.LogonWindow_InitializingAccount);
                },
                () => {
                    runtime.Start();

                    return 0;
                },
                (taskResult) => {

                    if (taskResult.HasError)
                    {
                        MessageBox.Show(Properties.Strings.LogonWindow_InitializeFailed + taskResult.Exception.Message);

                        btnRegisterAccount.IsEnabled = true;
                        btnLang.IsEnabled = true;

                        HideStatus();
                        return;
                    }

                    // Keep waiting after this finished, wait for it callback to update status
                }
            ).Execute();
        }

        private void ConnectAccount()
        {
            xdagLogger.Trace("Begin ConnectAccount().");

            if (runtime == null)
            {
                return;
            }

            PasswordWindow passwordWindow = new PasswordWindow(Properties.Strings.PasswordWindow_InputPassword, (passwordInput) =>
            {
                userInputPassword = passwordInput;
            });
            passwordWindow.ShowDialog();

            if (string.IsNullOrEmpty(userInputPassword))
            {
                xdagLogger.Trace("User input empty password, cancel the connect.");
                return;
            }

            btnConnectAccount.IsEnabled = false;
            btnLang.IsEnabled = false;

            BackgroundWork.CreateWork(
                this,
                () => {
                    ShowStatus(Properties.Strings.LogonWindow_SearchingConnection);
                },
                () => {
                    logonStatus = LogonStatus.Connecting;
                    runtime.Start();

                    return 0;
                },
                (taskResult) => {

                    xdagLogger.Trace("ConnectAccount work finished.");
                    if (taskResult.HasError)
                    {
                        if (taskResult.Exception is PasswordIncorrectException)
                        {
                            MessageBox.Show(Properties.Strings.Message_PasswordIncorrect, Properties.Strings.Common_MessageTitle);
                        }

                        btnConnectAccount.IsEnabled = true;
                        btnLang.IsEnabled = true;

                        HideStatus();
                        return;
                    }
                    
                    // Do not call HideStatus here if no error, since it will continue to wait for callback
                }
            ).Execute();

            xdagLogger.Trace("End ConnectAccount().");
        }

        private String InputPassword(String promptMessage, uint passwordSize)
        {
            if (logonStatus == LogonStatus.Registering)
            {
                logonStatus = LogonStatus.RegisteringConfirmPassword;
                return userInputPassword;
            }
            else if (logonStatus == LogonStatus.RegisteringConfirmPassword)
            {
                //// xDagWallet.PaswordPlain = confirmedPassword;
                logonStatus = LogonStatus.RegisteringRandomKeys;
                return userInputPassword;
            }
            else if (logonStatus == LogonStatus.RegisteringRandomKeys)
            {
                // Dont let customer to fill the random letters, just randomly create one
                string randomkey = Guid.NewGuid().ToString();
                xdagWallet.RandomKey = randomkey;

                logonStatus = LogonStatus.RegisteringPending;
                return randomkey;
            }
            else if (logonStatus == LogonStatus.Connecting)
            {
                logonStatus = LogonStatus.ConnectingPending;
                ShowStatus(Properties.Strings.LogonWindow_ConnectingAccount);
                return userInputPassword;
            }

            return string.Empty;
        }

        private int UpdateState(String state, String balance, String address)
        {
            /// MessageBox.Show(string.Format("Get State Update: State=[{0}], Balance=[{1}], Address=[{2}]", state, balance, address));
            

            return 0;
        }

        private void UpdateState(WalletState state)
        {
            ShowStatus(WalletStateConverter.Localize(state));

            if (state == WalletState.ConnectedPool)
            {
                xdagWallet.State = state;

                WalletWindow walletWindow = new WalletWindow(xdagWallet);
                walletWindow.Show();

                this.Close();
            }
        }

        private void ShowStatus(string message)
        {
            this.btnConnectAccount.Content = message;
            this.btnRegisterAccount.Content = message;

            //// this.lblWalletStatus.Visibility = Visibility.Visible;
            this.lblWalletStatus.Content = message;

            this.prbProgress.Visibility = Visibility.Visible;
            this.prbProgress.IsIndeterminate = true;
        }

        private void HideStatus()
        {
            this.btnConnectAccount.Content = Properties.Strings.LogonWindow_ConnectWallet;
            this.btnRegisterAccount.Content = Properties.Strings.LogonWindow_RegisterWallet;


            this.lblWalletStatus.Visibility = Visibility.Hidden;
            this.prbProgress.Visibility = Visibility.Hidden;
            this.prbProgress.IsIndeterminate = false;
        }

        private void btnLangEn_Click(object sender, RoutedEventArgs e)
        {
            SwitchLanguage("en-US");
        }

        private void btnLangZh_Click(object sender, RoutedEventArgs e)
        {
            SwitchLanguage("zh-Hans");
        }

        private void SwitchLanguage(string cultureString)
        {
            walletConfig.CultureInfo = cultureString;
            walletConfig.SaveToFile();

            Load_LocalizedStrings();
        }
    }
}