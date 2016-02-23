using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using System.Globalization;
using Windows.Networking.Sockets;
using Windows.Networking.Connectivity;
using Windows.Networking;
using Windows.Storage;
using Windows.Storage.FileProperties;
using Windows.Storage.Streams;
using Windows.System.Threading;
using Windows.Foundation;

namespace PhoneDirect3DXamlAppInterop
{
    class TaikoFtp
    {
        class FtpClient
        {
            TaikoFtp m_server;
            private string m_path = "\\";
            private StreamSocket m_cmdSocket;
            private StreamWriter m_cmdWriter;
            private StreamSocketListener m_dataListener;
            private StreamSocket m_dataSocket;
            private byte[] m_portAddress = { 0, 0, 0, 0, 0, 0 };
            private string m_rnfrPath;

            private string getFullPath(string path)
            {
                string tpath = Path.Combine(m_path, path).TrimStart("\\/".ToCharArray());
                string fpath = Path.GetFullPath(Path.Combine(m_server.m_root, tpath));
                if (!fpath.StartsWith(m_server.m_root))
                    return null;
                return fpath;
            }
            private string getDBPath(string fullPath)
            {
                if (!fullPath.StartsWith(m_server.m_root))
                    return null;
                return fullPath.Substring(m_server.m_root.Length).TrimStart('\\');
            }

            public FtpClient(TaikoFtp server, StreamSocket socket)
            {
                m_server = server;
                m_cmdSocket = socket;
            }

            public void release()
            {
                if (m_cmdWriter != null)
                {
                    m_cmdWriter.Dispose();
                    m_cmdWriter = null;
                }
                if (m_cmdSocket != null)
                {
                    m_cmdSocket.Dispose();
                    m_cmdSocket = null;
                }
                if (m_dataListener != null)
                {
                    m_dataListener.Dispose();
                    m_dataListener = null;
                }
                if (m_dataSocket != null)
                {
                    m_dataSocket.Dispose();
                    m_dataSocket = null;
                }
            }

            private void onPASVConnection(
                StreamSocketListener sender,
                StreamSocketListenerConnectionReceivedEventArgs args)
            {
                if (m_dataSocket != null)
                {
                    m_dataSocket.Dispose();
                    m_dataSocket = null;
                }
                m_dataSocket = args.Socket;
            }

            private async Task waitDataConnect(string type)
            {
                if (m_dataSocket != null)
                {
                    await m_cmdWriter.WriteLineAsync("150 Opening PASV mode data transfer for " + type);
                    await m_cmdWriter.FlushAsync();
                }
                else
                {
                    try
                    {
                        m_dataSocket = new StreamSocket();
                        await m_dataSocket.ConnectAsync(new HostName(string.Format("{0}.{1}.{2}.{3}", m_portAddress[0], m_portAddress[1], m_portAddress[2], m_portAddress[3])), (m_portAddress[4] * 256 + m_portAddress[5]).ToString());
                        m_portAddress[4] = m_portAddress[5] = 0;
                    }
                    catch
                    {
                        if (m_dataSocket != null)
                        {
                            m_dataSocket.Dispose();
                            m_dataSocket = null;
                        }
                    }
                    if (m_dataSocket != null)
                    {
                        await m_cmdWriter.WriteLineAsync("150 Opening PORT mode data transfer for " + type);
                        await m_cmdWriter.FlushAsync();
                    }
                }
            }

            public async void handleClient()
            {
                if (m_cmdSocket == null)
                    return;
                try
                {
                    if (m_server.m_utf8)
                    {
                        m_cmdWriter = new StreamWriter(m_cmdSocket.OutputStream.AsStreamForWrite());
                    }
                    else
                    {
                        m_cmdWriter = new StreamWriter(m_cmdSocket.OutputStream.AsStreamForWrite(), DBCSCodePage.DBCSEncoding.GetDBCSEncoding("gb2312"));
                    }
                    await m_cmdWriter.WriteLineAsync("220 ready");
                    await m_cmdWriter.FlushAsync();

                    StreamReader reader = null;
                    if (m_server.m_utf8)
                    {
                        reader = new StreamReader(m_cmdSocket.InputStream.AsStreamForRead());
                    }
                    else
                    {
                        reader = new StreamReader(m_cmdSocket.InputStream.AsStreamForRead(), DBCSCodePage.DBCSEncoding.GetDBCSEncoding("gb2312"));
                    }
                    string line = null;
                    bool working = true;
                    while (working && !String.IsNullOrEmpty(line = await reader.ReadLineAsync()))
                    {
                        string response = "";
                        string[] spcmd = line.Split(' ');
                        string argfull = spcmd.Length > 1 ? line.Substring(spcmd[0].Length).TrimStart(' ') : "";
                        switch (spcmd[0].ToUpperInvariant())
                        {
                            case "USER":
                                response = "331 username ok, need password";
                                break;
                            case "PASS":
                                response = "230 user logged in";
                                break;
                            case "CWD":
                                response = await cmdCWD(argfull);
                                break;
                            case "CDUP":
                                response = await cmdCWD("..");
                                break;
                            case "PWD":
                                response = "257 \"" + m_path + "\" is current directory";
                                break;
                            case "TYPE":
                                response = cmdTYPE(spcmd);
                                break;
                            case "PORT":
                                response = cmdPORT(argfull);
                                break;
                            case "PASV":
                                response = await cmdPASV();
                                break;
                            case "LIST":
                                response = await cmdLIST(argfull);
                                break;
                            case "STOR":
                                response = await cmdSTOR(argfull);
                                break;
                            case "RETR":
                                response = await cmdRETR(argfull);
                                break;
                            //case "ABOR":
                            //response = await cmdABOR();
                            //break;
                            case "DELE":
                                response = await cmdDELE(argfull);
                                break;
                            case "MKD":
                                response = await cmdMKD(argfull);
                                break;
                            case "RMD":
                                response = await cmdRMD(argfull);
                                break;
                            case "RNFR":
                                response = await cmdRNFR(argfull);
                                break;
                            case "RNTO":
                                response = await cmdRNTO(argfull);
                                break;
                            case "OPTS":
                                if (!m_server.m_utf8)
                                {
                                    response = "502 Command not implemented";
                                }
                                else if (spcmd.Length > 2 && spcmd[1].ToUpperInvariant() == "UTF8" && spcmd[2].ToUpperInvariant() == "ON")
                                {
                                    response = "200 UTF8 mode Enabled";
                                }
                                else
                                {
                                    response = "504 Command not implemented for that parameter";
                                }
                                break;
                            case "SYST":
                                response = "215 UNIX emulated";
                                break;
                            case "FEAT":
                                if (m_server.m_utf8)
                                {
                                    response = "211-Features:\r\n UTF8\r\n211 End";
                                }
                                else
                                {
                                    response = "502 Command not implemented";
                                }
                                break;
                            case "NOOP":
                                response = "200 OK";
                                break;
                            case "QUIT":
                                working = false;
                                response = "221 quit";
                                break;
                            default:
                                response = "502 Command not implemented";
                                break;
                        }
                        if (response.Length > 0)
                        {
                            await m_cmdWriter.WriteLineAsync(response);
                            await m_cmdWriter.FlushAsync();
                        }
                    }
                }
                catch
                {
                }
                release();
                m_cmdSocket = null;
            }

            private async Task<string> cmdCWD(string arg)
            {
                if (string.IsNullOrEmpty(arg))
                    return "501 Syntax error in parameters or arguments";
                string tpath = Path.Combine(m_path, arg).TrimStart("\\/".ToCharArray());
                string fpath = Path.GetFullPath(Path.Combine(m_server.m_root, tpath));
                try
                {
                    if (fpath.StartsWith(m_server.m_root) && (await StorageFolder.GetFolderFromPathAsync(fpath)) != null)
                    {
                        m_path = fpath.Substring(m_server.m_root.Length);
                        if (!m_path.StartsWith("\\"))
                            m_path = "\\" + m_path;
                        return "250 change to new directory";
                    }
                }
                catch
                {
                }
                return "550 failed change directory";
            }
            private string cmdTYPE(string[] spcmd)
            {
                if (spcmd.Length < 2)
                    return "501 Syntax error in parameters or arguments";
                switch (spcmd[1])
                {
                    case "A":
                    case "I":
                        break;
                    case "E":
                    case "L":
                    default:
                        return "504 Command not implemented for that parameter.";
                }
                if (spcmd.Length >= 3)
                {
                    switch (spcmd[2])
                    {
                        case "N":
                            break;
                        case "T":
                        case "C":
                        default:
                            return "504 Command not implemented for that parameter.";
                    }
                }
                return "200 OK";
            }
            private string cmdPORT(string arg)
            {
                if (m_dataSocket != null)
                {
                    m_dataSocket.Dispose();
                    m_dataSocket = null;
                }
                string[] sparg = arg.Trim().TrimStart('(').TrimEnd(')').Split(',');
                if (sparg.Length != m_portAddress.Length)
                    return "501 Syntax error in parameters or arguments";
                for (int i = 0; i < m_portAddress.Length; i++)
                {
                    try
                    {
                        int num = Convert.ToInt32(sparg[i]);
                        if (num < 0 || num > 255)
                            return "501 Syntax error in parameters or arguments";
                        m_portAddress[i] = (byte)num;
                    }
                    catch
                    {
                        return "501 Syntax error in parameters or arguments";
                    }
                }
                return "200 enter port mode";
            }

            private async Task<string> cmdPASV()
            {
                if (m_dataSocket != null)
                {
                    m_dataSocket.Dispose();
                    m_dataSocket = null;
                }
                if (m_server.m_host == null)
                    return "500 error";
                if (m_dataListener != null)
                {
                    int port = Convert.ToInt32(m_dataListener.Information.LocalPort);
                    return string.Format("227 Entering Passive Mode ({0},{1},{2})", m_server.m_host.DisplayName.Replace('.', ','), port / 256, port % 256);
                }
                try
                {
                    m_dataListener = new StreamSocketListener();
                    m_dataListener.ConnectionReceived += onPASVConnection;
                    await m_dataListener.BindEndpointAsync(m_server.m_host, "");
                    int port = Convert.ToInt32(m_dataListener.Information.LocalPort);
                    return string.Format("227 Entering Passive Mode ({0},{1},{2})", m_server.m_host.DisplayName.Replace('.', ','), port / 256, port % 256);
                }
                catch
                {
                    if (m_dataListener != null)
                    {
                        m_dataListener.Dispose();
                        m_dataListener = null;
                    }
                    return "500 error";
                }
            }

            private async Task<string> cmdLIST(string arg)
            {
                string fpath = getFullPath(arg);
                if (fpath == null)
                    return "450 Requested file action not taken";
                StorageFolder listFolder = null;
                StorageFile listFile = null;
                try
                {
                    listFolder = await StorageFolder.GetFolderFromPathAsync(fpath);
                }
                catch (System.IO.FileNotFoundException)
                {
                    listFolder = null;
                }
                if (listFolder == null)
                {
                    try
                    {
                        listFile = await StorageFile.GetFileFromPathAsync(fpath);
                    }
                    catch (System.IO.FileNotFoundException)
                    {
                        return "450 Requested file action not taken";
                    }
                }
                if (listFolder == null && listFile == null)
                    return "450 Requested file action not taken";

                await waitDataConnect("LIST");
                if (m_dataSocket == null)
                    return "425 Can't open data connection";

                StreamWriter writer = null;
                if (m_server.m_utf8)
                {
                    writer = new StreamWriter(m_dataSocket.OutputStream.AsStreamForWrite());
                }
                else
                {
                    writer = new StreamWriter(m_dataSocket.OutputStream.AsStreamForWrite(), DBCSCodePage.DBCSEncoding.GetDBCSEncoding("gb2312"));
                }
                DateTime timeKey = DateTime.Now - TimeSpan.FromDays(180);
                if (listFolder != null)
                {
                    foreach (var i in await listFolder.GetFoldersAsync())
                    {
                        BasicProperties prop = await i.GetBasicPropertiesAsync();
                        string time = prop.DateModified < timeKey ?
                            prop.DateModified.ToString("MMM dd  yyyy", new CultureInfo("en-US")) :
                            prop.DateModified.ToString("MMM dd HH:mm", new CultureInfo("en-US"));
                        await writer.WriteLineAsync(string.Format("drwxrwxrwx 1 0 0 {0} {1} {2}", prop.Size, time, i.Name));
                    }
                    foreach (var i in await listFolder.GetFilesAsync())
                    {
                        BasicProperties prop = await i.GetBasicPropertiesAsync();
                        string time = prop.DateModified < timeKey ?
                            prop.DateModified.ToString("MMM dd  yyyy", new CultureInfo("en-US")) :
                            prop.DateModified.ToString("MMM dd HH:mm", new CultureInfo("en-US"));
                        await writer.WriteLineAsync(string.Format("-rwxrwxrwx 1 0 0 {0} {1} {2}", prop.Size, time, i.Name));
                    }
                }
                else if (listFile != null)
                {
                    BasicProperties prop = await listFile.GetBasicPropertiesAsync();
                    string time = prop.DateModified < timeKey ?
                        prop.DateModified.ToString("MMM dd  yyyy", new CultureInfo("en-US")) :
                        prop.DateModified.ToString("MMM dd HH:mm", new CultureInfo("en-US"));
                    await writer.WriteLineAsync(string.Format("-rwxrwxrwx 1 0 0 {0} {1} {2}", prop.Size, time, listFile.Name));
                }
                await writer.FlushAsync();
                writer.Dispose();
                m_dataSocket.Dispose();
                m_dataSocket = null;
                return "226 Transfer complete";
            }
            private async Task<string> cmdSTOR(string arg)
            {
                string fpath = getFullPath(arg);
                if (fpath == null)
                    return "450 Requested file action not taken";
                Stream storStream = null;
                try
                {
                    StorageFolder storFolder = await StorageFolder.GetFolderFromPathAsync(Path.GetDirectoryName(fpath));
                    if (storFolder != null)
                        storStream = await storFolder.OpenStreamForWriteAsync(Path.GetFileName(fpath), CreationCollisionOption.ReplaceExisting);
                }
                catch
                {
                    storStream = null;
                }
                if (storStream == null)
                    return "425 Can't open data connection";

                await waitDataConnect("STOR");
                if (m_dataSocket == null)
                    return "450 Requested file action not taken";

                try
                {
                    Stream dataStream = m_dataSocket.InputStream.AsStreamForRead();
                    await dataStream.CopyToAsync(storStream);
                    await dataStream.FlushAsync();
                    await storStream.FlushAsync();
                }
                catch
                {
                }
                storStream.Dispose();
                storStream = null;
                m_dataSocket.Dispose();
                m_dataSocket = null;
                if(fpath.ToLower().EndsWith(".tja"))
                    await App.GameDB.InsertTja(getDBPath(Path.GetDirectoryName(fpath)), Path.GetFileName(fpath), fpath);
                return "226 Transfer complete";
            }
            private async Task<string> cmdRETR(string arg)
            {
                string fpath = getFullPath(arg);
                if (fpath == null)
                    return "450 Requested file action not taken";
                Stream retrStream = null;
                try
                {
                    StorageFolder retrFolder = await StorageFolder.GetFolderFromPathAsync(Path.GetDirectoryName(fpath));
                    if (retrFolder != null)
                        retrStream = await retrFolder.OpenStreamForReadAsync(Path.GetFileName(fpath));
                }
                catch
                {
                    retrStream = null;
                }
                if (retrStream == null)
                    return "425 Can't open data connection";

                await waitDataConnect("RETR");
                if (m_dataSocket == null)
                    return "450 Requested file action not taken";

                try
                {
                    Stream dataStream = m_dataSocket.OutputStream.AsStreamForWrite();
                    await retrStream.CopyToAsync(dataStream);
                    await retrStream.FlushAsync();
                    await dataStream.FlushAsync();
                }
                catch
                {
                }
                retrStream.Dispose();
                retrStream = null;
                m_dataSocket.Dispose();
                m_dataSocket = null;
                return "226 Transfer complete";
            }
            private async Task<string> cmdDELE(string arg)
            {
                string fpath = getFullPath(arg);
                if (fpath == null)
                    return "450 Requested file action not taken";
                try
                {
                    StorageFile deleFile = await StorageFile.GetFileFromPathAsync(fpath);
                    if (deleFile != null)
                    {
                        await deleFile.DeleteAsync();
                        App.GameDB.DeleteTja(Path.GetDirectoryName(fpath).Substring(m_server.m_root.Length).TrimStart('\\'), Path.GetFileName(fpath));
                        return "250 delete file ok";
                    }
                }
                catch
                {
                }
                return "450 Requested file action not taken";
            }
            private async Task<string> cmdMKD(string arg)
            {
                string fpath = getFullPath(arg);
                if (fpath == null)
                    return "450 Requested file action not taken";
                try
                {
                    StorageFolder parentFolder = await StorageFolder.GetFolderFromPathAsync(Path.GetDirectoryName(fpath));
                    if (parentFolder != null)
                    {
                        await parentFolder.CreateFolderAsync(Path.GetFileName(fpath));
                        return "250 create folder ok";
                    }
                }
                catch
                {
                }
                return "450 Requested file action not taken";
            }
            private async Task<string> cmdRMD(string arg)
            {
                string fpath = getFullPath(arg);
                if (fpath == null)
                    return "450 Requested file action not taken";
                try
                {
                    StorageFolder rmdFolder = await StorageFolder.GetFolderFromPathAsync(fpath);
                    if (rmdFolder != null)
                    {
                        await rmdFolder.DeleteAsync();
                        return "250 delete folder ok";
                    }
                }
                catch
                {
                }
                return "450 Requested file action not taken";
            }
            private async Task<string> cmdRNFR(string arg)
            {
                m_rnfrPath = null;
                string fpath = getFullPath(arg);
                if (fpath == null)
                    return "450 Requested file action not taken";
                try
                {
                    StorageFolder rnfrFolder = await StorageFolder.GetFolderFromPathAsync(fpath);
                    if (rnfrFolder != null)
                    {
                        m_rnfrPath = fpath;
                        return "350 Directory exists, ready for destination name.";
                    }
                }
                catch
                {
                }
                try
                {
                    StorageFile rnfrFile = await StorageFile.GetFileFromPathAsync(fpath);
                    if (rnfrFile != null)
                    {
                        m_rnfrPath = fpath;
                        return "350 File exists, ready for destination name.";
                    }
                }
                catch
                {
                }
                return "450 Requested file action not taken";
            }
            private async Task<string> cmdRNTO(string arg)
            {
                if (string.IsNullOrEmpty(m_rnfrPath))
                    return "450 Requested file action not taken";
                string fpath = getFullPath(arg);
                if (fpath == null)
                    return "450 Requested file action not taken";
                if (fpath == m_rnfrPath)
                    return "450 Requested file action not taken";
                string frDir = Path.GetDirectoryName(m_rnfrPath);
                string frName = Path.GetFileName(m_rnfrPath);
                string toDir = Path.GetDirectoryName(fpath);
                string toName = Path.GetFileName(fpath);
                if (frDir == toDir)
                {
                    try
                    {
                        StorageFolder rnfrFolder = await StorageFolder.GetFolderFromPathAsync(m_rnfrPath);
                        if (rnfrFolder != null)
                        {
                            try
                            {
                                await rnfrFolder.RenameAsync(Path.GetFileName(fpath));
                                App.GameDB.RenameDir(getDBPath(m_rnfrPath), getDBPath(fpath));
                                return "250 file renamed successfully";
                            }
                            catch
                            {
                                return "450 Requested file action not taken";
                            }
                        }
                    }
                    catch (System.IO.FileNotFoundException)
                    {
                    }
                    try
                    {
                        StorageFile rnfrFile = await StorageFile.GetFileFromPathAsync(m_rnfrPath);
                        if (rnfrFile != null)
                        {
                            try
                            {
                                await rnfrFile.RenameAsync(toName);
                                App.GameDB.RenameTja(frDir, frName, toDir, toName);
                                return "250 file renamed successfully";
                            }
                            catch
                            {
                                return "450 Requested file action not taken";
                            }
                        }
                    }
                    catch (System.IO.FileNotFoundException)
                    {
                    }
                    return "450 Requested file action not taken";
                }
                else
                {
                    StorageFolder toFolder = null;
                    try
                    {
                        toFolder = await StorageFolder.GetFolderFromPathAsync(toDir);
                    }
                    catch (System.IO.FileNotFoundException)
                    {
                        toFolder = null;
                    }
                    if (toFolder == null)
                        return "450 Requested file action not taken";
                    StorageFolder rnfrFolder = null;
                    try
                    {
                        rnfrFolder = await StorageFolder.GetFolderFromPathAsync(m_rnfrPath);
                    }
                    catch (System.IO.FileNotFoundException)
                    {
                        rnfrFolder = null;
                    }
                    if (rnfrFolder != null)
                    {
                        return "450 Requested file action not taken";
                        //try
                        //{
                        //    await moveFolder(rnfrFolder, toFolder, toName);
                        //    App.GameDB.renameDir(getDBPath(m_rnfrPath), getDBPath(fpath));
                        //    return "250 file renamed successfully";
                        //}
                        //catch
                        //{
                        //    return "450 Requested file action not taken";
                        //}
                    }
                    try
                    {
                        StorageFile rnfrFile = await StorageFile.GetFileFromPathAsync(m_rnfrPath);
                        if (rnfrFile != null)
                        {
                            await rnfrFile.MoveAsync(toFolder);
                            if (frName != toName)
                                await rnfrFile.RenameAsync(toName);
                            App.GameDB.RenameTja(frDir, frName, toDir, toName);
                            return "250 file renamed successfully";
                        }
                    }
                    catch
                    {
                    }
                    return "450 Requested file action not taken";
                }
            }
            private async Task moveFolder(StorageFolder src, StorageFolder tarParent, string tarName)
            {
                StorageFolder tar = await tarParent.CreateFolderAsync(string.IsNullOrEmpty(tarName) ? src.Name : tarName);
                if (tar != null)
                {
                    foreach (var i in await src.GetFoldersAsync())
                        await moveFolder(i, tar, null);
                    foreach (var i in await src.GetFilesAsync())
                        await i.MoveAsync(tar);
                }
                await src.DeleteAsync();
            }
        }

        private string m_root;
        private bool m_utf8;
        private HostName m_host;
        private StreamSocketListener m_listener;
        private List<FtpClient> m_client = new List<FtpClient>();

        public async Task<bool> init(String root, bool utf8)
        {
            m_root = Path.GetFullPath(root);
            m_utf8 = utf8;
            m_host = null;
            foreach (var i in NetworkInformation.GetHostNames())
            {
                if (i.IPInformation.NetworkAdapter.IanaInterfaceType == 71)
                {
                    m_host = i;
                    break;
                }
            }
            if (m_host == null)
                return false;
            m_listener = new StreamSocketListener();
            m_listener.ConnectionReceived += OnConnection;
            await m_listener.BindEndpointAsync(m_host, "");
            return true;
        }
        public void release()
        {
            if (m_listener != null)
            {
                m_listener.Dispose();
                m_listener = null;
            }
            foreach (FtpClient i in m_client)
            {
                i.release();
            }
            m_client.Clear();
        }
        public string Address
        {
            get
            {
                if (m_listener == null)
                    return null;
                if (string.IsNullOrEmpty(m_listener.Information.LocalPort))
                    return null;
                return "ftp://" + m_host.DisplayName + ":" + m_listener.Information.LocalPort;
            }
        }

        private void OnConnection(
            StreamSocketListener sender,
            StreamSocketListenerConnectionReceivedEventArgs args)
        {
            FtpClient client = new FtpClient(this, args.Socket);
            client.handleClient();
            m_client.Add(client);
        }
    }
}
