using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Navigation;
using Microsoft.Phone.Controls;
using Microsoft.Phone.Shell;

namespace PhoneDirect3DXamlAppInterop
{
    public partial class PageSdCard : PhoneApplicationPage
    {
        public PageSdCard()
        {
            InitializeComponent();
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            NavigationService.Navigate(new Uri("/PageFtp.xaml?removeBackEntry=1", UriKind.Relative));
        }

        private void HyperlinkButton_Click_Help(object sender, RoutedEventArgs e)
        {
            ButtonHelp.Visibility = Visibility.Visible;
        }

        private void Button_Click_Help(object sender, RoutedEventArgs e)
        {
            ButtonHelp.Visibility = Visibility.Collapsed;
        }
    }
}