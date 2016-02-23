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
    public partial class PageFtp : PhoneApplicationPage
    {
        TaikoFtp m_serv;
        public PageFtp()
        {
            InitializeComponent();
        }
        protected override async void OnNavigatedTo(NavigationEventArgs e)
        {
            base.OnNavigatedTo(e);
            Microsoft.Phone.Shell.PhoneApplicationService.Current.UserIdleDetectionMode =
                Microsoft.Phone.Shell.IdleDetectionMode.Disabled;
            try
            {
                await Windows.Storage.ApplicationData.Current.LocalFolder.CreateFolderAsync("taikodata", Windows.Storage.CreationCollisionOption.FailIfExists);
            }
            catch
            {
            }
            m_serv = new TaikoFtp();
            if (await m_serv.init(Windows.Storage.ApplicationData.Current.LocalFolder.Path + "\\taikodata", false))
            {
                TextHost.Text = m_serv.Address;
            }
            else
            {
                TextHost.Text = "！发生错误，请先连接wifi";
            }

            string param = "";
            if (NavigationContext.QueryString.TryGetValue("removeBackEntry", out param) && param == "1")
            {
                NavigationService.RemoveBackEntry();
            }
        }
        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            base.OnNavigatedFrom(e);
            m_serv.release();
            m_serv = null;
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