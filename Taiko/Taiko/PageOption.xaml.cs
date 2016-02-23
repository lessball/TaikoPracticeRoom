using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Navigation;
using Microsoft.Phone.Controls;
using Microsoft.Phone.Shell;
using Microsoft.Phone.Tasks;
using System.IO.IsolatedStorage;

namespace PhoneDirect3DXamlAppInterop
{
    public partial class PageOption : PhoneApplicationPage
    {
        public PageOption()
        {
            InitializeComponent();
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            base.OnNavigatedTo(e);
            IsolatedStorageSettings settings = IsolatedStorageSettings.ApplicationSettings;
            CBHide.IsChecked = settings.Contains("hidepacked") && (settings["hidepacked"] as string) != "false";
        }

        private void CheckBox_Click(object sender, RoutedEventArgs e)
        {
            IsolatedStorageSettings settings = IsolatedStorageSettings.ApplicationSettings;
            if (!settings.Contains("hidepacked"))
            {
                settings.Add("hidepacked", CBHide.IsChecked.GetValueOrDefault() ? "true" : "false");
            }
            else
            {
                settings["hidepacked"] = CBHide.IsChecked.GetValueOrDefault() ? "true" : "false";
            }
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            MarketplaceReviewTask marketplaceReviewTask = new MarketplaceReviewTask();
            marketplaceReviewTask.Show();
        }
    }
}