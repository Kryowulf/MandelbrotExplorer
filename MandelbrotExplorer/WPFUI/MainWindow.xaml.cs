using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Forms;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using Accessibility;
using MandelbrotExplorerLib;
using WPFUI.Converters;
using WPFUI.ViewModels;

namespace WPFUI
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        private MainViewModel _viewmodel;
        private RGBColorListConverter _rgbListConverter = new();
        private int _panLastX;
        private int _panLastY;
        private bool _resizing = false;

        public MainWindow()
        {
            InitializeComponent();

            IntPtr instanceHandle = Process.GetCurrentProcess().Handle;
            IntPtr surfaceHandle = fractalSurface.Handle;

            _viewmodel = new MainViewModel(instanceHandle, surfaceHandle);
            _viewmodel.PropertyChanged += this._viewmodel_PropertyChanged;

            this.DataContext = _viewmodel;
        }

        private void _viewmodel_PropertyChanged(object? sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            if (e.PropertyName == nameof(MainViewModel.Gradient))
            {
                // Controls updated via data binding can make the 
                // picturebox control refresh itself, erasing the mandelbrot.
                DelayDraw();
            }
        }

        private void Draw()
        {
            DateTime start = DateTime.Now;
            _viewmodel.Draw();
            DateTime end = DateTime.Now;
            int time = (int) (end - start).TotalMilliseconds;

            // To keep the picturebox control from refreshing itself,
            // don't use databinding for this message.
            outputMessageTextBlock.Text = $"Render time: {time} ms.";

            _resizing = false;
        }

        private void Window_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            var w1 = SystemParameters.WorkArea.Width;                   // <- dpi dependent
            var w2 = Screen.PrimaryScreen?.WorkingArea.Width ?? w1;     // <-- actual pixel width

            fractalSurface.Location = new System.Drawing.Point(5, 5);
            fractalSurface.Width = (int)((w2 / w1) * wfContainer.ActualWidth) - 5;
            fractalSurface.Height = (int)((w2 / w1) * wfContainer.ActualHeight) - 5;
            _viewmodel.RefreshSurface();
            DelayDraw();
        }

        private void DelayDraw()
        {
            // When resizing or maximizing the window, WPF somehow refreshes the pictureBox control
            // at some point after this event completes, which erases the rendering.
            // If we want the Mandelbrot to still automatically show up, 
            // the Draw call needs to be delayed a bit.
            if (!_resizing)
            {
                _resizing = true;
                Dispatcher.BeginInvoke(Draw, System.Windows.Threading.DispatcherPriority.Background);
            }
        }

        private void fractalSurface_MouseDown(object sender, System.Windows.Forms.MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Left)
            {
                _panLastX = e.X;
                _panLastY = e.Y;
            }
        }

        private void fractalSurface_MouseMove(object sender, System.Windows.Forms.MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Left)
            {
                _viewmodel.PanPixels(e.X - _panLastX, e.Y - _panLastY);
                _panLastX = e.X;
                _panLastY = e.Y;
                Draw();
            }
        }

        private void fractalSurface_MouseWheel(object sender, System.Windows.Forms.MouseEventArgs e)
        {
            if (e.Delta < 0)
                _viewmodel.ZoomToPixel(e.X, e.Y, -1);
            else if (e.Delta > 0)
                _viewmodel.ZoomToPixel(e.X, e.Y, 1);

            Draw();
        }

        private void rbFire_Checked(object sender, RoutedEventArgs e)
        {
            _viewmodel.Gradient = _rgbListConverter.ConvertBack("Yellow, Red, Orange, Black");
        }

        private void rbIce_Checked(object sender, RoutedEventArgs e)
        {
            _viewmodel.Gradient = _rgbListConverter.ConvertBack("Black, Black, Blue, White");
        }

        private void rbStorm_Checked(object sender, RoutedEventArgs e)
        {
            _viewmodel.Gradient = _rgbListConverter.ConvertBack("1F, White, Black");
        }

        private void rbPlasma_Checked(object sender, RoutedEventArgs e)
        {
            _viewmodel.Gradient = _rgbListConverter.ConvertBack("Black, Black, Lime, Black, Purple, Black, Yellow");
        }

        private void rbPsychedelic_Checked(object sender, RoutedEventArgs e)
        {
            _viewmodel.Gradient = _rgbListConverter.ConvertBack("Black, Black, Red, Lime, Blue");
        }

        private void rbCustom_Checked(object sender, RoutedEventArgs e)
        {

        }

        private void gradientTextBox_KeyDown(object sender, System.Windows.Input.KeyEventArgs e)
        {
            rbCustom.IsChecked = true;
        }
    }
}
