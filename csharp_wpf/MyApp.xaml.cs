using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;

namespace MyApp
{
    /// <summary>
    /// Interaction logic for MyApp.xaml
    /// </summary>
    public partial class App : Application
    {
        private void Application_Startup(object sender, StartupEventArgs e)
        {
            var mainWindowViewModel = new MainWindowViewModel();
            MainWindow window = new MainWindow(mainWindowViewModel);
            window.Show();
        }
    }
}
