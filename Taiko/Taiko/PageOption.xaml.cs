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
using PhoneDirect3DXamlAppComponent;
using System.ComponentModel;

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
            switch (Orientation)
            {
                case PageOrientation.LandscapeLeft:
                    m_gameCallback.m_direction = 1;
                    break;
                case PageOrientation.PortraitDown:
                    m_gameCallback.m_direction = 2;
                    break;
                case PageOrientation.Landscape:
                case PageOrientation.LandscapeRight:
                    m_gameCallback.m_direction = 3;
                    break;
                default:
                    m_gameCallback.m_direction = 0;
                    break;
            }
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

        private void Button_Click_Drum_Scale(object sender, RoutedEventArgs e)
        {
            LayoutRoot.Visibility = Visibility.Collapsed;
            DrumScaleSlider.Visibility = Visibility.Visible;

            IsolatedStorageSettings settings = IsolatedStorageSettings.ApplicationSettings;
            if (!settings.Contains("drumscale"))
            {
                settings.Add("drumscale", 1.0f);
            }
            DrumScaleData data = new DrumScaleData();
            data.DrumScale = (float)settings["drumscale"];
            DrumScaleSlider.DataContext = data;
        }

        private static Direct3DBackground m_d3dBackground = null;
        private static Object m_Provider = null;
        GameCallback m_gameCallback = new GameCallback();

        protected override void OnOrientationChanged(OrientationChangedEventArgs e)
        {
            base.OnOrientationChanged(e);
            switch (e.Orientation)
            {
                case PageOrientation.LandscapeLeft:
                    m_gameCallback.m_direction = 1;
                    break;
                case PageOrientation.PortraitDown:
                    m_gameCallback.m_direction = 2;
                    break;
                case PageOrientation.Landscape:
                case PageOrientation.LandscapeRight:
                    m_gameCallback.m_direction = 3;
                    break;
                default:
                    m_gameCallback.m_direction = 0;
                    break;
            }
        }

        private void DrawingSurfaceBackground_Loaded(object sender, RoutedEventArgs e)
        {
            if (m_d3dBackground == null)
            {
                m_d3dBackground = new Direct3DBackground();
                m_d3dBackground.InitCulture("");
                m_d3dBackground.SetGameCallBack(m_gameCallback);

                // Set window bounds in dips
                m_d3dBackground.WindowBounds = new Windows.Foundation.Size(
                    (float)Application.Current.Host.Content.ActualWidth,
                    (float)Application.Current.Host.Content.ActualHeight
                    );

                // Set native resolution in pixels
                m_d3dBackground.NativeResolution = new Windows.Foundation.Size(
                    (float)Math.Floor(Application.Current.Host.Content.ActualWidth * Application.Current.Host.Content.ScaleFactor / 100.0f + 0.5f),
                    (float)Math.Floor(Application.Current.Host.Content.ActualHeight * Application.Current.Host.Content.ScaleFactor / 100.0f + 0.5f)
                    );

                // Set render resolution to the full native resolution
                m_d3dBackground.RenderResolution = new Windows.Foundation.Size(1280 / m_d3dBackground.NativeResolution.Height * m_d3dBackground.NativeResolution.Width, 1280);// m_d3dBackground.NativeResolution;

                m_Provider = m_d3dBackground.CreateContentProvider();
                m_d3dBackground.beginRender();
            }
            // Hook-up native component to DrawingSurfaceBackgroundGrid
            DrawingSurfaceBackground.SetBackgroundContentProvider(m_Provider);
            DrawingSurfaceBackground.SetBackgroundManipulationHandler(m_d3dBackground);
        }

        public class GameCallback : IGameCallback
        {
            public int m_direction = 0;
            public int getDirection()
            {
                return m_direction;
            }
            public void submitResult(string dbpath, bool trueScore, bool success, bool fullCombo, int score, int combo, int good, int normal, int bad)
            {
            }
        }

        class DrumScaleData : INotifyPropertyChanged
        {
            private float m_drumscale = 1.0f;
            public float DrumScale
            {
                get { return m_drumscale; }
                set
                {
                    m_drumscale = value;
                    IsolatedStorageSettings settings = IsolatedStorageSettings.ApplicationSettings;
                    settings["drumscale"] = value;
                    if (m_d3dBackground != null)
                    {
                        m_d3dBackground.setDrumScale(value);
                    }
                    NotifyPropertyChanged("DrumScale");
                }
            }

            public event PropertyChangedEventHandler PropertyChanged;
            public void NotifyPropertyChanged(string propertyName)
            {
                if (PropertyChanged != null)
                {
                    PropertyChanged(this,
                        new PropertyChangedEventArgs(propertyName));
                }
            }
        }
    }
}