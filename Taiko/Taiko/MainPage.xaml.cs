using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Shapes;
using Microsoft.Phone.Controls;
using PhoneDirect3DXamlAppComponent;

using System.IO.IsolatedStorage;
using System.Windows.Threading;
using Windows.Storage;
using Microsoft.Phone.Storage;

namespace PhoneDirect3DXamlAppInterop
{
    public partial class MainPage : PhoneApplicationPage
    {
        // Constructor
        public MainPage()
        {
            InitializeComponent();
        }

        private void Button_Click_Begin(object sender, RoutedEventArgs e)
        {
            //NavigationService.Navigate(new Uri("/PageGame.xaml?tja=tja/sdop2.tja", UriKind.Relative));
            NavigationService.Navigate(new Uri("/PageGame.xaml", UriKind.Relative));
        }

        private async void Button_Click_Manage(object sender, RoutedEventArgs e)
        {
            ExternalStorageDevice sdCard = (await ExternalStorage.GetExternalStorageDevicesAsync()).FirstOrDefault();
            if (sdCard != null)
            {
                NavigationService.Navigate(new Uri("/PageSdCard.xaml", UriKind.Relative));
            }
            else
            {
                NavigationService.Navigate(new Uri("/PageFtp.xaml", UriKind.Relative));
            }
        }

        private void Button_Click_Option(object sender, RoutedEventArgs e)
        {
            NavigationService.Navigate(new Uri("/PageOption.xaml", UriKind.Relative));
        }

        protected override async void OnNavigatedTo(System.Windows.Navigation.NavigationEventArgs e)
        {
            base.OnNavigatedTo(e);
            IsolatedStorageSettings settings = IsolatedStorageSettings.ApplicationSettings;
            if (!settings.Contains("hidepacked"))
            {
                try
                {
                    StorageFolder folder = await StorageFolder.GetFolderFromPathAsync("tja");
                    foreach(var i in await folder.GetFilesAsync())
                    {
                        if (i.Path.ToLower().EndsWith(".tja"))
                        {
                            await App.GameDB.InsertTja("\\tja", i.Name, i.Path);
                            settings.Add("hidepacked", "false");
                            settings.Save();
                        }
                    }
                }
                catch
                {
                }
            }
        }

    }
}