using System;
using System.Threading.Tasks;

namespace MyApp
{
    class Program
    {
        [STAThread()]
        public static void Main()
        {
            SuperluminalPerf.Initialize();
            var app = new App();
            app.InitializeComponent();
            app.Run();
        }
    }

}
