using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace XDagWallet.UI
{
    /// <summary>
    /// Interaction logic for LogonWindow.xaml
    /// </summary>
    public partial class LogonWindow : Window
    {
        public LogonWindow()
        {
            InitializeComponent();

            // Core Controls
            ////this.Resources.MergedDictionaries.Add(new Xceed.Wpf.Themes.Metro.MetroDarkThemeResourceDictionary(new SolidColorBrush(Colors.Green)));
            // Toolkit Controls
            ////this.Resources.MergedDictionaries.Add(new Xceed.Wpf.Toolkit.Themes.Metro.ToolkitMetroDarkThemeResourceDictionary(new SolidColorBrush(Colors.Green)));
            
        }
    }
}
