using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using System.ComponentModel;
using System.Data.Linq;
using System.Data.Linq.Mapping;
using System.IO;
using Microsoft.Phone.Storage;

namespace PhoneDirect3DXamlAppInterop
{
    public class TaikoFlag
    {
        public int Course = 0;
        public int Level = 0;
        public int Player = 0;
        public bool Branch = false;
        public bool Played = false;
        public bool Success = false;
        public bool Fullcombo = false;
        public bool TrueScore = false;
        public int Flag
        {
            get
            {
                int tflag = (Player << 6) | (Course << 4) | Level;
                if (Branch)
                    tflag |= 1 << 8;
                if (Played)
                    tflag |= 1 << 9;
                if (Success)
                    tflag |= 1 << 10;
                if (Fullcombo)
                    tflag |= 1 << 11;
                if (TrueScore)
                    tflag |= 1 << 12;
                return tflag;
            }
            set
            {
                Level = value & 0xf;
                Course = (value >> 4) & 3;
                Player = (value >> 6) & 3;
                Branch = (value & (1 << 8)) != 0;
                Played = (value & (1 << 9)) != 0;
                Success = (value & (1 << 10)) != 0;
                Fullcombo = (value & (1 << 11)) != 0;
                TrueScore = (value & (1 << 12)) != 0;
            }
        }
    }

    [Table(Name = "score")]
    public class TaikoScore : INotifyPropertyChanged, INotifyPropertyChanging
    {
        private string _path;
        [Column(IsPrimaryKey = true)]
        public string Path
        {
            get { return _path; }
            set
            {
                if (_path != value)
                {
                    NotifyPropertyChanging("Path");
                    _path = value;
                    NotifyPropertyChanged("Path");
                }
            }
        }

        private string _title;
        [Column]
        public string Title
        {
            get { return _title; }
            set
            {
                if (_title != value)
                {
                    NotifyPropertyChanging("Title");
                    _title = value;
                    NotifyPropertyChanged("Title");
                }
            }
        }

        private string _subTitle;
        [Column]
        public string SubTitle
        {
            get { return _subTitle; }
            set
            {
                if (_subTitle != value)
                {
                    NotifyPropertyChanging("SubTitle");
                    _subTitle = value;
                    NotifyPropertyChanged("SubTitle");
                }
            }
        }

        private string _wav;
        [Column]
        public string Wav
        {
            get { return _wav; }
            set
            {
                if (_wav != value)
                {
                    NotifyPropertyChanging("Wav");
                    _wav = value;
                    NotifyPropertyChanged("Wav");
                }
            }
        }

        private int _songVol;
        [Column]
        public int SongVol
        {
            get { return _songVol; }
            set
            {
                if (_songVol != value)
                {
                    NotifyPropertyChanging("SongVol");
                    _songVol = value;
                    NotifyPropertyChanging("SongVol");
                }
            }
        }

        private int _demoStart;
        [Column]
        public int DemoStart
        {
            get { return _demoStart; }
            set
            {
                if (_demoStart != value)
                {
                    NotifyPropertyChanging("DemoStart");
                    _demoStart = value;
                    NotifyPropertyChanged("DemoStart");
                }
            }
        }

        private int _flag;
        [Column]
        public int Flag
        {
            get { return _flag; }
            set
            {
                if (_flag != value)
                {
                    NotifyPropertyChanging("Flag");
                    _flag = value;
                    NotifyPropertyChanged("Flag");
                }
            }
        }

        private int _score;
        [Column]
        public int Score
        {
            get { return _score; }
            set
            {
                if (_score != value)
                {
                    NotifyPropertyChanging("Score");
                    _score = value;
                    NotifyPropertyChanged("Score");
                }
            }
        }

        private int _combo;
        [Column]
        public int Combo
        {
            get { return _combo; }
            set
            {
                if (_combo != value)
                {
                    NotifyPropertyChanging("Combo");
                    _combo = value;
                    NotifyPropertyChanged("Combo");
                }
            }
        }

        private int _good;
        [Column]
        public int Good
        {
            get { return _good; }
            set
            {
                if (_good != value)
                {
                    NotifyPropertyChanging("Good");
                    _good = value;
                    NotifyPropertyChanged("Good");
                }
            }
        }

        private int _normal;
        [Column]
        public int Normal
        {
            get { return _normal; }
            set
            {
                if (_normal != value)
                {
                    NotifyPropertyChanging("Normal");
                    _normal = value;
                    NotifyPropertyChanged("Normal");
                }
            }
        }

        private int _bad;
        [Column]
        public int Bad
        {
            get { return _bad; }
            set
            {
                if (_bad != value)
                {
                    NotifyPropertyChanging("Bad");
                    _bad = value;
                    NotifyPropertyChanged("Bad");
                }
            }
        }

        [Column(IsVersion = true)]
        private Binary _version;

        #region INotifyPropertyChanged Members

        public event PropertyChangedEventHandler PropertyChanged;

        private void NotifyPropertyChanged(string PropertyName)
        {
            if (PropertyChanged != null)
                PropertyChanged(this, new PropertyChangedEventArgs(PropertyName));
        }

        #endregion

        #region INotifyPropertyChanging Members

        public event PropertyChangingEventHandler PropertyChanging;

        private void NotifyPropertyChanging(string PropertyName)
        {
            if (PropertyChanging != null)
                PropertyChanging(this, new PropertyChangingEventArgs(PropertyName));
        }

        #endregion
    }

    public class TaikoScoreDataContext : DataContext
    {
        public static string DBConnectionString = "DataSource=isostore:/Score.sdf";
        public TaikoScoreDataContext(string connectionString)
            : base(connectionString)
        { }

        public Table<TaikoScore> Score;
    }

    public class TaikoDB
    {
        private TaikoScoreDataContext scoreDB;

        public TaikoDB()
        {
            scoreDB = new TaikoScoreDataContext(TaikoScoreDataContext.DBConnectionString);
        }

        public string MakePath(string dir, string file, int index)
        {
            return dir + "\n" + file + "\n" + index.ToString();
        }
        public IQueryable<TaikoScore> QueryDir(string dir)
        {
            if (scoreDB == null)
                return null;
            string start = dir + "\n";
            return from score in scoreDB.Score where score.Path.StartsWith(start) select score;
        }
        public IQueryable<TaikoScore> QueryTja(string dir, string file)
        {
            if (scoreDB == null)
                return null;
            string start = dir + "\n" + file + "\n";
            return from score in scoreDB.Score where score.Path.StartsWith(start) select score;
        }
        public TaikoScore QueryScore(string path)
        {
            if (scoreDB == null)
                return null;
            return (from score in scoreDB.Score where score.Path == path select score).SingleOrDefault();
        }
        public IQueryable<TaikoScore> QueryAll()
        {
            if (scoreDB == null)
                return null;
            return from score in scoreDB.Score select score;
        }
        public async Task InsertTja(string dir, string file, string fullpath)
        {
            if (scoreDB == null)
                return;
            string title = "";
            string subtitle = "";
            string wav = "";
            int songvol = 100;
            int demostart = 0;
            int level = 1;
            int course = 3;
            var flags = new List<TaikoFlag>();
            ExternalStorageFile sdFile = null;
            Stream sdStream = null;
            StreamReader reader = null;
            if (dir.StartsWith("sdcard:\\"))
            {
                ExternalStorageDevice sdCard = (await ExternalStorage.GetExternalStorageDevicesAsync()).FirstOrDefault();
                if (sdCard != null)
                {
                    sdFile = await sdCard.GetFileAsync(fullpath);
                    sdStream = await sdFile.OpenForReadAsync();
                    reader = new StreamReader(sdStream, DBCSCodePage.DBCSEncoding.GetDBCSEncoding("gb2312"));
                }
            }
            else
            {
                reader = new StreamReader(fullpath, DBCSCodePage.DBCSEncoding.GetDBCSEncoding("gb2312"));
            }
            try
            {
                string line;
                while (reader != null && null != (line = await reader.ReadLineAsync()))
                {
                    if (line.StartsWith("TITLE:"))
                    {
                        title = line.Substring("TITLE:".Length);
                    }
                    else if (line.StartsWith("SUBTITLE:"))
                    {
                        subtitle = line.Substring("SUBTITLE:".Length);
                    }
                    else if (line.StartsWith("WAVE:"))
                    {
                        wav = line.Substring("WAVE:".Length);
                    }
                    else if (line.StartsWith("SONGVOL:"))
                    {
                        try
                        {
                            songvol = (int)Convert.ToDouble(line.Substring("SONGVOL:".Length));
                        }
                        catch
                        {
                        }
                    }
                    else if (line.StartsWith("DEMOSTART:"))
                    {
                        try
                        {
                            demostart = (int)(Convert.ToDouble(line.Substring("DEMOSTART:".Length)) * 1000);
                        }
                        catch
                        {
                        }
                    }
                    else if (line.StartsWith("LEVEL:"))
                    {
                        try
                        {
                            level = Math.Max(1, Math.Min(10, Convert.ToInt32(line.Substring("LEVEL:".Length))));
                        }
                        catch
                        {
                        }
                    }
                    else if (line.StartsWith("COURSE:"))
                    {
                        string ts = line.Substring("COURSE:".Length).Trim().ToLower();
                        if (ts.StartsWith("oni") || ts.StartsWith("edit") || ts.StartsWith("3") || ts.StartsWith("4"))
                        {
                            course = 3;
                        }
                        else if (ts.StartsWith("hard") || ts.StartsWith("2"))
                        {
                            course = 2;
                        }
                        else if (ts.StartsWith("normal") || ts.StartsWith("1"))
                        {
                            course = 1;
                        }
                        else if (ts.StartsWith("easy") || ts.StartsWith("0"))
                        {
                            course = 0;
                        }
                    }
                    else if (line.StartsWith("#START"))
                    {
                        var flag = new TaikoFlag();
                        flag.Course = course;
                        flag.Level = level;
                        string[] spline = line.Split(' ');
                        if (spline.Length > 1)
                        {
                            switch (spline[1])
                            {
                                case "P1":
                                    flag.Player = 1;
                                    break;
                                case "P2":
                                    flag.Player = 2;
                                    break;
                            }
                        }
                        flags.Add(flag);
                    }
                    else if (line.StartsWith("#BRANCHSTART"))
                    {
                        if (flags.Count > 0)
                            flags.Last().Branch = true;
                    }
                }
                if (flags.Count > 0)
                {
                    for (int i = 0; i < flags.Count; i++)
                    {
                        string tpath = MakePath(dir, file, i);
                        TaikoScore score = QueryScore(tpath);
                        if (score == null)
                        {
                            score = new TaikoScore
                            {
                                Path = tpath,
                                Title = title,
                                SubTitle = subtitle,
                                Wav = wav,
                                SongVol = songvol,
                                DemoStart = demostart,
                                Flag = flags[i].Flag,
                            };
                            scoreDB.Score.InsertOnSubmit(score);
                        }
                        else
                        {
                            score.Title = title;
                            score.SubTitle = subtitle;
                            score.Wav = wav;
                            score.SongVol = songvol;
                            score.DemoStart = demostart;
                            score.Flag = flags[i].Flag;
                        }
                    }
                    scoreDB.SubmitChanges();
                }
            }
            catch (Exception e)
            {
            }
            finally
            {
                if (reader != null)
                {
                    reader.Dispose();
                }
                if (sdStream != null)
                {
                    sdStream.Dispose();
                }
                if (sdFile != null)
                {
                    sdFile.Dispose();
                }
            }
        }
        public void DeleteTja(string dir, string file)
        {
            if (scoreDB == null)
                return;
            string start = dir + "\n" + file;
            var query = from score in scoreDB.Score where score.Path.StartsWith(start) select score;
            foreach (var i in query)
                scoreDB.Score.DeleteOnSubmit(i);
            scoreDB.SubmitChanges();
        }
        public delegate bool CheckValid(TaikoScore score);
        public void DeleteUnvalidTja(CheckValid check)
        {
            var query = from score in scoreDB.Score select score;
            foreach (var i in query)
            {
                if (!check(i))
                {
                    scoreDB.Score.DeleteOnSubmit(i);
                }
            }
            scoreDB.SubmitChanges();
        }
        public void RenameDir(string src, string tar)
        {
            if (scoreDB == null)
                return;
            var query = from score in scoreDB.Score where score.Path.StartsWith(src) select score;
            foreach (var i in query)
                i.Path = tar + i.Path.Substring(src.Length);
            scoreDB.SubmitChanges();
        }
        public void RenameTja(string srcDir, string srcFile, string tarDir, string tarFile)
        {
            RenameDir(srcDir + '\n' + srcFile, tarDir + '\n' + tarFile);
        }
        public TaikoScore UpdateScore(string path, bool trueScore, bool success, bool fullcombo, int score, int combo, int good, int normal, int bad, out bool newRecord)
        {
            newRecord = false;
            TaikoScore tscore = QueryScore(path);
            if (tscore == null)
                return null;
            var flag = new TaikoFlag();
            flag.Flag = tscore.Flag;
            if (score > tscore.Score || !flag.Played || (!flag.TrueScore && trueScore))
            {
                flag.Played = true;
                flag.Success |= success;
                flag.Fullcombo |= fullcombo;
                flag.TrueScore = trueScore;
                tscore.Flag = flag.Flag;
                tscore.Score = score;
                tscore.Combo = combo;
                tscore.Good = good;
                tscore.Normal = normal;
                tscore.Bad = bad;
                scoreDB.SubmitChanges();
                newRecord = true;
            }
            else if ((!flag.Success && success) || (!flag.Fullcombo && fullcombo))
            {
                flag.Success |= success;
                flag.Fullcombo |= fullcombo;
                tscore.Flag = flag.Flag;
                scoreDB.SubmitChanges();
            }
            return tscore;
        }
        public void SubmitChange()
        {
            if (scoreDB != null)
                scoreDB.SubmitChanges();
        }
    }
}
