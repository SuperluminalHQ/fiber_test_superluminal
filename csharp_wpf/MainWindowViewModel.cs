using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;

using MvvmGen;

namespace MyApp
{
    public class CommandHandler : ICommand
    {
        private Action _action;
        private Func<bool> _canExecute;

        public CommandHandler(Action action)
        {
            _action = action;
            _canExecute = delegate () { return true; };
        }
        public CommandHandler(Action action, Func<bool> canExecute)
        {
            _action = action;
            _canExecute = canExecute;
        }

        public event EventHandler CanExecuteChanged
        {
            add { CommandManager.RequerySuggested += value; }
            remove { CommandManager.RequerySuggested -= value; }
        }

        public bool CanExecute(object parameter)
        {
            return _canExecute.Invoke();
        }

        public void Execute(object parameter)
        {
            _action();
        }
    }

    public enum UploadState {
        Failure,
        Uploading,
        Success
    }

    [ViewModel]
    public partial class MainWindowViewModel {
      [Property] private string _executableName;
        [Property] private ObservableCollection<DataInfo> _stackFrames;
        [Property] private int _uploadValue = 0;

        public ICommand MyCommand { get; }

        [Property] private UploadState _uploadState;

        private async Task UploadFunc()
        {
            using (SuperluminalPerf.BeginEvent("UploadFunc"))
            {

                for (int i = 1; i <= 100; i++)
                {
                    UploadValue = i;
                    await Task.Delay(10);
                }
                UploadState = UploadState.Success;
            }
        }
        [Command]
        private void StartUpload()
        {
            using (SuperluminalPerf.BeginEvent("StartUpload"))
            {
                UploadState = UploadState.Uploading;
                Task.Run(UploadFunc);
            }
        }

        [Command]
        public void Reset()
        {
            using (SuperluminalPerf.BeginEvent("Reset"))
            {
                UploadState = UploadState.Failure;
            }
        }

        partial void OnInitialize() {
            ExecutableName = "OneApp.exe";
            UploadState = UploadState.Failure;

            StackFrames = new ObservableCollection<DataInfo>(
                new DataInfo[] { 
                    new DataInfo { Line = 1, Data = "DataA", Filename = "File1"},
                    new DataInfo { Line = 1, Data = "DataA", Filename = "File1"},
                    new DataInfo { Line = 1, Data = "DataA", Filename = "File1"},
                    new DataInfo { Line = 1, Data = "DataA", Filename = "File1"},
                    new DataInfo { Line = 1, Data = "DataA", Filename = "File1"},
                    new DataInfo { Line = 1, Data = "DataB", Filename = "File1"},
                    new DataInfo { Line = 1, Data = "DataA", Filename = "File3"},
                    new DataInfo { Line = 1, Data = "DataA", Filename = "File1"},
                    new DataInfo { Line = 1, Data = "CCCCC", Filename = "File1"},
                    new DataInfo { Line = 1, Data = "DataA", Filename = "File1"},
                    new DataInfo { Line = 1, Data = "BBBBB", Filename = "File1"},
                    new DataInfo { Line = 1, Data = "DataA", Filename = "File1"},
                    new DataInfo { Line = 1, Data = "DataA", Filename = "File1"},
                    new DataInfo { Line = 1, Data = "DataA", Filename = "File1"},
                    new DataInfo { Line = 1, Data = "DataA", Filename = "File1"},
                    new DataInfo { Line = 1, Data = "DataA", Filename = "File1"},
                    new DataInfo { Line = 1, Data = "DataA", Filename = "File1"},
                }
            );


        }
    }
}
