using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Navigation;
using Microsoft.Phone.Controls;
using Microsoft.Phone.Shell;
using PhoneDirect3DXamlAppComponent;

using System.Windows.Media;
using System.ComponentModel;
using System.IO;
using System.IO.IsolatedStorage;
using Windows.Storage;
using Microsoft.Phone.Storage;
using System.Threading.Tasks;

namespace PhoneDirect3DXamlAppInterop
{
    public class GameCallback : IGameCallback
    {
        public int m_direction = 0;
        public int getDirection()
        {
            return m_direction;
        }
        public PageGame m_pageGame;
        public void submitResult(string dbpath, bool trueScore, bool success, bool fullCombo, int score, int combo, int good, int normal, int bad)
        {
            if (m_pageGame != null)
                m_pageGame.submitResult(dbpath, trueScore, success, fullCombo, score, combo, good, normal, bad);
        }
    }

    public partial class PageGame : PhoneApplicationPage
    {
        private static Direct3DBackground m_d3dBackground = null;
        private static Object m_Provider = null;
        //private string m_tja = "";
        private int m_index = 0;
        private static GameCallback m_gameCallback = new GameCallback();

        public PageGame()
        {
            InitializeComponent();
        }

        private void DrawingSurfaceBackground_Loaded(object sender, RoutedEventArgs e)
        {
            m_gameCallback.m_pageGame = this;
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
                m_d3dBackground.RenderResolution = new Windows.Foundation.Size(1280/m_d3dBackground.NativeResolution.Height*m_d3dBackground.NativeResolution.Width, 1280);// m_d3dBackground.NativeResolution;

                m_Provider = m_d3dBackground.CreateContentProvider();
            }
            // Hook-up native component to DrawingSurfaceBackgroundGrid
            DrawingSurfaceBackground.SetBackgroundContentProvider(m_Provider);
            DrawingSurfaceBackground.SetBackgroundManipulationHandler(m_d3dBackground);
            //if(!string.IsNullOrEmpty(m_tja))
            //    StartGame(m_tja, m_index);
        }

        protected override void OnNavigatedTo(System.Windows.Navigation.NavigationEventArgs e)
        {
            base.OnNavigatedTo(e);
            //m_tja = "";
            //NavigationContext.QueryString.TryGetValue("tja", out m_tja);
            switch(Orientation)
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
            m_root = Windows.Storage.ApplicationData.Current.LocalFolder.Path + "\\taikodata";
            StopGame();
            PanelRB.Visibility = System.Windows.Visibility.Collapsed;
            PanelRT.Visibility = System.Windows.Visibility.Collapsed;
            PanelResult.Visibility = System.Windows.Visibility.Collapsed;
            refreshStorage();
        }

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

        protected override void OnBackKeyPress(System.ComponentModel.CancelEventArgs e)
        {
            if (UIRoot.Visibility == System.Windows.Visibility.Collapsed)
            {
                StopGame();
                e.Cancel = true;
            }
            else
            {
                e.Cancel = false;
            }
        }

        private async void Button_Click(object sender, RoutedEventArgs e)
        {
            if (ListItem.SelectedItem != null)
            {
                TaikoItem ts = ListItem.SelectedItem as TaikoItem;
                if (ts.DBItem != null)
                {
                    var path = ts.Path.Split('\n');
                    if (path.Length > 2)
                    {
                        try
                        {
                            if (path[0].StartsWith(m_sdCardRoot))
                            {
                                this.IsEnabled = false;
                                ProgressRefresh.Visibility = Visibility.Visible;
                                m_d3dBackground.clearMemoryFileData();
                                string tjapath = Path.Combine(path[0], path[1]);
                                string wavepath = Path.Combine(path[0], ts.DBItem.Wav);
                                if (await loadSdCardFile(tjapath) && await loadSdCardFile(wavepath))
                                {
                                    StartGame(tjapath, wavepath, ts.Path, Convert.ToInt32(path[2]));
                                }
                                ProgressRefresh.Visibility = Visibility.Collapsed;
                                this.IsEnabled = true;
                            }
                            else if (!path[0].StartsWith("\\"))
                            {
                                string[] pathseg = { m_root, path[0], path[1] };
                                string tjapath = Path.Combine(pathseg);
                                pathseg[2] = ts.DBItem.Wav;
                                string wavepath = Path.Combine(pathseg);
                                int index = Convert.ToInt32(path[2]);
                                if (!StartGame(tjapath, wavepath, ts.Path, index))
                                {
                                    this.IsEnabled = false;
                                    ProgressRefresh.Visibility = Visibility.Visible;
                                    if (await LoadLocalFile(tjapath) && await LoadLocalFile(wavepath))
                                    {
                                        StartGame(tjapath, wavepath, ts.Path, index);
                                    }
                                    ProgressRefresh.Visibility = Visibility.Collapsed;
                                    this.IsEnabled = true;
                                }
                            }
                            else
                            {
                                string dir = path[0].TrimStart('\\');
                                string tjapath = Path.Combine(dir, path[1]);
                                string wavepath = Path.Combine(dir, ts.DBItem.Wav);
                                int index = Convert.ToInt32(path[2]);
                                StartGame(tjapath, wavepath, ts.Path, index);
                            }
                        }
                        catch
                        {
                        }
                    }
                }
            }
        }

        private async void ListItem_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (ListItem.SelectedItem != null)
            {
                TaikoItem ts = ListItem.SelectedItem as TaikoItem;
                if (ts.DBItem == null)
                {
                    ListPath(ts.Path);
                }
                else if (ts.DBItem != null)
                {
                    TextCaption.Text = ts.DBItem.Title;
                    TextSubCaption.Text = ts.DBItem.SubTitle;
                    string slevel = "";
                    switch (ts.Flag.Course)
                    {
                        case 0:
                            slevel = "简单";
                            break;
                        case 1:
                            slevel = "普通";
                            break;
                        case 2:
                            slevel = "困难";
                            break;
                        case 3:
                            slevel = "鬼神";
                            break;
                    }
                    slevel += new string('★', ts.Flag.Level);
                    TextBranch.Visibility = ts.Flag.Branch ? System.Windows.Visibility.Visible : System.Windows.Visibility.Collapsed;
                    TextLevel.Text = slevel;
                    TextScore.Text = "最高得分" + ts.DBItem.Score;
                    TextPerfect.Text = "良" + ts.DBItem.Good;
                    TextGood.Text = "可" + ts.DBItem.Normal;
                    TextBad.Text = "不可" + ts.DBItem.Bad;
                    if (ts.DBItem.Good + ts.DBItem.Normal + ts.DBItem.Bad != 0)
                    {
                        TextRatio.Text = "击中率" + (ts.DBItem.Good + ts.DBItem.Normal) * 100 / (ts.DBItem.Good + ts.DBItem.Normal + ts.DBItem.Bad) + "%";
                    }
                    else
                    {
                        TextRatio.Text = "击中率0%";
                    }
                    TextCombo.Text = "连段" + ts.DBItem.Combo;
                    PanelRB.Visibility = System.Windows.Visibility.Visible;
                    PanelRT.Visibility = System.Windows.Visibility.Visible;
                    if (!(sender is bool) || (bool)sender)
                    {
                        var path = ts.Path.Split('\n');
                        if (path.Length > 2)
                        {
                            try
                            {
                                if (path[0].StartsWith(m_sdCardRoot))
                                {
                                    this.IsEnabled = false;
                                    ProgressRefresh.Visibility = Visibility.Visible;
                                    m_d3dBackground.clearMemoryFileData();
                                    string wavepath = Path.Combine(path[0], ts.DBItem.Wav);
                                    if (await loadSdCardFile(wavepath))
                                    {
                                        m_d3dBackground.playDemo(wavepath, ts.DBItem.DemoStart * 0.001, ts.DBItem.SongVol * 0.01);
                                    }
                                    ProgressRefresh.Visibility = Visibility.Collapsed;
                                    this.IsEnabled = true;
                                }
                                else if (!path[0].StartsWith("\\"))
                                {
                                    this.IsEnabled = false;
                                    ProgressRefresh.Visibility = Visibility.Visible;
                                    string[] pathseg = { m_root, path[0], ts.DBItem.Wav };
                                    string wavepath = Path.Combine(pathseg);
                                    if (!m_d3dBackground.playDemo(wavepath, ts.DBItem.DemoStart * 0.001, ts.DBItem.SongVol * 0.01))
                                    {
                                        if (await LoadLocalFile(wavepath))
                                        {
                                            m_d3dBackground.playDemo(wavepath, ts.DBItem.DemoStart * 0.001, ts.DBItem.SongVol * 0.01);
                                        }
                                    }
                                    ProgressRefresh.Visibility = Visibility.Collapsed;
                                    this.IsEnabled = true;
                                }
                                else
                                {
                                    string wavepath = Path.Combine(path[0].TrimStart('\\'), ts.DBItem.Wav);
                                    m_d3dBackground.playDemo(wavepath, ts.DBItem.DemoStart * 0.001, ts.DBItem.SongVol * 0.01);
                                }
                            }
                            catch
                            {
                            }
                        }
                    }
                }
            }
        }

        private void PanelResult_Tap(object sender, System.Windows.Input.GestureEventArgs e)
        {
            if (PanelResult.Visibility != System.Windows.Visibility.Collapsed)
                PanelResult.Visibility = System.Windows.Visibility.Collapsed;
        }

        public bool StartGame(string tja, string wave, string dbpath, int index)
        {
            if (m_d3dBackground == null || !m_d3dBackground.beginGame(tja, index, wave, dbpath, BnAuto.IsChecked.GetValueOrDefault()))
            {
                return false;
            }
            //m_tja = "";
            UIRoot.Visibility = System.Windows.Visibility.Collapsed;
            return true;
        }

        public void StopGame()
        {
            if (m_d3dBackground == null)
                return;
            m_d3dBackground.stopGame();
            UIRoot.Visibility = System.Windows.Visibility.Visible;
            BnStart.Visibility = System.Windows.Visibility.Collapsed;
            BnStart.Visibility = System.Windows.Visibility.Visible;
        }

        public void submitResult(string dbpath, bool trueScore, bool success, bool fullCombo, int score, int combo, int good, int normal, int bad)
        {
            Deployment.Current.Dispatcher.BeginInvoke(() =>
            {
                TaikoScore dbscore = null;
                bool newRecord = false;
                if (trueScore)
                    dbscore = App.GameDB.UpdateScore(dbpath, trueScore, success, fullCombo, score, combo, good, normal, bad, out newRecord);
                string text = success ? "演奏成功！" : "演奏失败 ";
                if (fullCombo)
                    text += "全连！";
                if (newRecord)
                    text += "新纪录！";
                text += "得分" + score.ToString();
                TextResult0.Text = text;
                int ratio = good + normal + bad != 0 ? (good + normal) * 100 / (good + normal + bad) : 0;
                TextResult1.Text = string.Format("良{0} 可{1} 不可{2} 击中率{3}% 连段{4}", good, normal, bad, ratio, combo);
                if (dbscore != null && ListItem.SelectedItem != null)
                {
                    TaikoItem titem = ListItem.SelectedItem as TaikoItem;
                    if (titem.DBItem != null && titem.DBItem.Path == dbpath)
                        titem.DBItem = dbscore;
                    ListItem_SelectionChanged(false, null);
                }
                UIRoot.Visibility = System.Windows.Visibility.Visible;
                PanelResult.Visibility = System.Windows.Visibility.Visible;
            });
        }

        public class TaikoItem : INotifyPropertyChanged
        {
            public Brush BackBrush { get; set; }
            private string _prefix;
            public string Prefix
            {
                get { return _prefix; }
                set
                {
                    if (value != _prefix)
                    {
                        _prefix = value;
                        NotifyPropertyChanged("Prefix");
                    }
                }
            }
            public string ItemCaption { get; set; }
            public string Path;
            public TaikoFlag Flag = new TaikoFlag();
            private TaikoScore _DBItem = null;
            public TaikoScore DBItem
            {
                get { return _DBItem; }
                set
                {
                    _DBItem = value;
                    Path = _DBItem.Path;
                    Flag.Flag = _DBItem.Flag;
                    string prefix = Flag.Branch ? "➰" : "";
                    switch (Flag.Course)
                    {
                        case 0:
                            prefix += "简";
                            break;
                        case 1:
                            prefix += "普";
                            break;
                        case 2:
                            prefix += "难";
                            break;
                        case 3:
                            prefix += "鬼";
                            break;
                    }
                    prefix += Flag.Level.ToString();
                    prefix += Flag.Success ? (Flag.Fullcombo ? "♚" : "♔") : " ";
                    Prefix = prefix;
                    ItemCaption = _DBItem.Title;
                    switch (Flag.Player)
                    {
                        case 1:
                            ItemCaption += " P1";
                            break;
                        case 2:
                            ItemCaption += " P2";
                            break;
                    }
                }
            }

            public TaikoItem()
            {
            }
            public TaikoItem(TaikoScore ts)
            {
                DBItem = ts;
            }

            #region INotifyPropertyChanged Members

            public event PropertyChangedEventHandler PropertyChanged;

            private void NotifyPropertyChanged(string PropertyName)
            {
                if (PropertyChanged != null)
                    PropertyChanged(this, new PropertyChangedEventArgs(PropertyName));
            }

            #endregion
        };
        private async void ListPath(string path)
        {
            if (path == null)
            {
                return;
            }
            ListItem.IsEnabled = false;
            PanelRB.Visibility = System.Windows.Visibility.Collapsed;
            PanelRT.Visibility = System.Windows.Visibility.Collapsed;
            PanelResult.Visibility = System.Windows.Visibility.Collapsed;

            string dir = null;
            string sdDir = null;
            StorageFolder folder = null;
            ExternalStorageFolder sdFolder = null;
            if (path == "\\")
            {
                dir = "";
                sdDir = m_sdCardRoot;
                try
                {
                    folder = await StorageFolder.GetFolderFromPathAsync(m_root);
                }
                catch (System.IO.FileNotFoundException)
                {
                }
                ExternalStorageDevice sdCard = (await ExternalStorage.GetExternalStorageDevicesAsync()).FirstOrDefault();
                if (sdCard != null)
                {
                    try
                    {
                        sdFolder = await sdCard.GetFolderAsync(m_sdCardFolder);
                    }
                    catch (System.IO.FileNotFoundException)
                    {
                    }
                }
            }
            else if (path.StartsWith(m_root))
            {
                dir = path.Substring(m_root.Length).TrimStart('\\');
                folder = await StorageFolder.GetFolderFromPathAsync(path);
            }
            else if (path.StartsWith(m_sdCardRoot))
            {
                sdDir = path;
                ExternalStorageDevice sdCard = (await ExternalStorage.GetExternalStorageDevicesAsync()).FirstOrDefault();
                if (sdCard != null)
                {
                    sdFolder = await sdCard.GetFolderAsync(m_sdCardFolder + path.Substring(m_sdCardRoot.Length));
                }
            }
            else
            {
                return;
            }
            m_path = path;

            List<TaikoItem> tlist = new List<TaikoItem>();
            var backbrush = new LinearGradientBrush();
            backbrush.StartPoint = new Point(0, 0);
            backbrush.EndPoint = new Point(1, 0);
            backbrush.GradientStops.Add(new GradientStop { Color = Color.FromArgb(25, 0, 0, 0), Offset = 0.0 });
            backbrush.GradientStops.Add(new GradientStop { Color = Color.FromArgb(25, 0, 0, 0), Offset = 0.4 });
            backbrush.GradientStops.Add(new GradientStop { Color = Color.FromArgb(10, 0, 0, 0), Offset = 1.0 });
            if (path != "\\")
            {
                string parent = null;
                int p = path.LastIndexOf('\\');
                if (p < 0)
                {
                    parent = "\\";
                }
                else
                {
                    parent = path.Substring(0, p);
                }
                if (parent + "\\" == m_sdCardRoot)
                {
                    parent = "\\";
                }
                tlist.Add(new TaikoItem { BackBrush = backbrush, ItemCaption = "..", Path = parent });
            }
            if (folder != null)
            {
                foreach (var i in await folder.GetFoldersAsync())
                {
                    var item = new TaikoItem { ItemCaption = i.Name, Path = i.Path };
                    item.BackBrush = (tlist.Count % 2 == 0) ? backbrush : null;
                    tlist.Add(item);
                }
                foreach (var i in await folder.GetFilesAsync())
                {
                    if (i.Name.ToLower().EndsWith(".tja"))
                    {
                        if (App.GameDB.QueryTja(dir, i.Name).FirstOrDefault() == null)
                        {
                            ProgressRefresh.Visibility = Visibility.Visible;
                            await App.GameDB.InsertTja(dir, i.Name, i.Path);
                        }
                    }
                }
            }
            if (sdFolder != null)
            {
                foreach (var i in await sdFolder.GetFoldersAsync())
                {
                    var item = new TaikoItem { ItemCaption = i.Name, Path = m_sdCardRoot + i.Path.Substring(m_sdCardFolder.Length) };
                    item.BackBrush = (tlist.Count % 2 == 0) ? backbrush : null;
                    tlist.Add(item);
                }
                foreach (var i in await sdFolder.GetFilesAsync())
                {
                    if (i.Name.ToLower().EndsWith(".tja"))
                    {
                        if (App.GameDB.QueryTja(sdDir, i.Name).FirstOrDefault() == null)
                        {
                            ProgressRefresh.Visibility = Visibility.Visible;
                            await App.GameDB.InsertTja(sdDir, i.Name, i.Path);
                        }
                    }
                }
            }
            for (int i = 0; i < 2; i++)
            {
                if ((i == 0 && folder == null) || (i != 0 && sdFolder == null))
                {
                    continue;
                }
                foreach (var score in App.GameDB.QueryDir(i == 0 ? dir : sdDir))
                {
                    var pathSplit = score.Path.Split('\n');
                    if (pathSplit.Length > 2)
                    {
                        var item = new TaikoItem(score);
                        item.BackBrush = (tlist.Count % 2 == 0) ? backbrush : null;
                        tlist.Add(item);
                    }
                }
            }
            if (path == "\\")
            {
                IsolatedStorageSettings settings = IsolatedStorageSettings.ApplicationSettings;
                if (!settings.Contains("hidepacked") || (settings["hidepacked"] as string) == "false")
                {
                    foreach (var i in App.GameDB.QueryDir("\\tja"))
                    {
                        var item = new TaikoItem(i);
                        item.BackBrush = (tlist.Count % 2 == 0) ? backbrush : null;
                        tlist.Add(item);
                    }
                }
            }
            ListItem.ItemsSource = tlist;
            ListItem.IsEnabled = true;
            ProgressRefresh.Visibility = Visibility.Collapsed;
        }

        private async void refreshStorage()
        {
            ProgressRefresh.Visibility = Visibility.Visible;
            ExternalStorageDevice sdCard = (await ExternalStorage.GetExternalStorageDevicesAsync()).FirstOrDefault();
            if (sdCard == null)
            {
                if (ListItem.ItemsSource == null || ListItem.ItemsSource.Count == 0)
                {
                    ListPath("\\");
                }
                ProgressRefresh.Visibility = Visibility.Collapsed;
                return;
            }
            HashSet<string> checkList = new HashSet<string>();
            foreach (var i in App.GameDB.QueryAll())
            {
                string[] path = i.Path.Split('\n');
                if (path.Length > 1)
                {
                    checkList.Add(Path.Combine(path[0], path[1]));
                }
            }
            HashSet<string> removeList = new HashSet<string>();
            foreach (var path in checkList)
            {
                try
                {
                    if (path.StartsWith(m_sdCardRoot))
                    {
                        var file = await sdCard.GetFileAsync(m_sdCardFolder + path.Substring(m_sdCardRoot.Length));
                        file.Dispose();
                    }
                    else if (!path.StartsWith("\\"))
                    {
                        await StorageFile.GetFileFromPathAsync(Path.Combine(m_root, path));
                    }
                }
                catch (System.IO.FileNotFoundException)
                {
                    removeList.Add(path);
                }
            }
            App.GameDB.DeleteUnvalidTja(score =>
            {
                string[] path = score.Path.Split('\n');
                if (path.Length > 1)
                {
                    return !removeList.Contains(Path.Combine(path[0], path[1]));
                }
                else
                {
                    return false;
                }
            });
            if (ListItem.ItemsSource == null || ListItem.ItemsSource.Count == 0)
            {
                ListPath("\\");
            }
            ProgressRefresh.Visibility = Visibility.Collapsed;
        }

        private async Task<bool> loadSdCardFile(string path)
        {
            if (!path.StartsWith(m_sdCardRoot))
            {
                return false;
            }
            ExternalStorageDevice sdCard = (await ExternalStorage.GetExternalStorageDevicesAsync()).FirstOrDefault();
            if (sdCard != null)
            {
                try
                {
                    using (ExternalStorageFile file = await sdCard.GetFileAsync(m_sdCardFolder +  path.Substring(m_sdCardRoot.Length)))
                    {
                        using (Stream stream = await file.OpenForReadAsync())
                        {
                            byte[] data = new byte[stream.Length];
                            await stream.ReadAsync(data, 0, (int)stream.Length);
                            m_d3dBackground.setMemoryFileData(path, data);
                            return true;
                        }
                    }
                }
                catch (System.IO.FileNotFoundException)
                {
                }
            }
            return false;
        }

        private async Task<bool> LoadLocalFile(string path)
        {
            try
            {
                StorageFile file = await StorageFile.GetFileFromPathAsync(Path.Combine(m_root, path));
                using (Stream stream = (await file.OpenReadAsync()).AsStreamForRead())
                {
                    byte[] data = new byte[stream.Length];
                    await stream.ReadAsync(data, 0, (int)stream.Length);
                    m_d3dBackground.setMemoryFileData(path, data);
                    return true;
                }
            }
            catch (System.IO.FileNotFoundException)
            {
            }
            return false;
        }

        private string m_path;
        private string m_root;
        private string m_sdCardRoot = "sdcard:\\";
        private string m_sdCardFolder = "太鼓练习室\\";
    }
}