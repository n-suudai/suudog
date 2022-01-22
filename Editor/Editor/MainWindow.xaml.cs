using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace Editor
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        [DllImport("user32.dll")]
        private static extern bool SetWindowText(IntPtr hWnd, string lpString);

        [StructLayout(LayoutKind.Sequential)]
        public struct RECT
        {
            public int Left;
            public int Top;
            public int Right;
            public int Bottom;
        }

        [DllImport("user32.dll")]
        static extern bool GetClientRect(IntPtr hWnd, out RECT lpRect);

        [DllImport("user32.dll")]
        static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

        [DllImport("user32.dll")]
        private static extern bool EnumChildWindows(IntPtr hWnd, EnumChildWindowsDelegate lpEnumFunc, IntPtr lParam);

        private delegate bool EnumChildWindowsDelegate(IntPtr hWnd, IntPtr lParam);

        [DllImport("user32.dll")]
        private static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int x, int y, int cx, int cy, int uFlags);

        public MainWindow()
        {
            InitializeComponent();

            WindowsFormsHost.Loaded += WindowsFormsHost_Loaded;
        }

        private const int SWP_NOZORDER = 0x0004;
        private const int SWP_NOMOVE = 0x0002;

        private bool EnumChildWindowsProcedure(IntPtr hWnd, IntPtr lParam)
        {
            RECT rc;
            GetWindowRect(RuntimeWindow.Handle, out rc);
            SetWindowPos(hWnd, IntPtr.Zero,
                0,
                0,
                rc.Right - rc.Left,
                rc.Bottom - rc.Top,
                SWP_NOZORDER | SWP_NOMOVE);
            return true;
        }

        private void WindowsFormsHost_Loaded(object sender, RoutedEventArgs e)
        {
            SetWindowText(RuntimeWindow.Handle, "SUUDOG-Runtime");
        }

        private void RuntimeWindow_ClientSizeChanged(object sender, EventArgs e)
        {
            EnumChildWindows(RuntimeWindow.Handle, EnumChildWindowsProcedure, RuntimeWindow.Handle);
        }
    }
}
