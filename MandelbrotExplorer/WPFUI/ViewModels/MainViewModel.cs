using MandelbrotExplorerLib;
using Microsoft.Windows.Themes;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WPFUI.ViewModels
{
    public class MainViewModel : ViewModel
    {
        private const double ZOOM_FACTOR = 0.8;

        private MandelbrotRenderer _renderer;

        private int _surfaceWidth;
        public int SurfaceWidth 
        { 
            get { return _surfaceWidth; }
            set { 
                SetValue(ref _surfaceWidth, value);
                RefreshSurface();
                UpdateBorders();
            }
        }

        private int _surfaceHeight;
        public int SurfaceHeight
        {
            get { return _surfaceHeight; }
            set { 
                SetValue(ref _surfaceHeight, value);
                RefreshSurface();
                UpdateBorders();
            }
        }

        private int _zoomLevel;
        public int ZoomLevel
        {
            get { return _zoomLevel; }
            set { 
                SetValue(ref _zoomLevel, value);
                UpdateBorders();
            }
        }

        private double _centerX;
        public double CenterX 
        { 
            get { return _centerX; }
            set { 
                SetValue(ref _centerX, value);
                UpdateBorders();
            }
        }

        private double _centerY;
        public double CenterY
        {
            get { return _centerY; }
            set { 
                SetValue(ref _centerY, value);
                UpdateBorders();
            }
        }

        private double _top;
        public double Top
        {
            get { return _top; }
            private set { SetValue(ref _top, value); }
        }

        private double _left;
        public double Left
        {
            get { return _left; }
            private set { SetValue(ref _left, value); }
        }

        private double _right;
        public double Right
        {
            get { return _right; }
            private set { SetValue(ref _right, value); }
        }

        private double _bottom;
        public double Bottom
        {
            get { return _bottom; }
            private set { SetValue(ref _bottom, value); }
        }

        private int _gradientPeriod = 20;
        public int GradientPeriod
        {
            get { return _gradientPeriod; }
            set { 
                SetValue(ref _gradientPeriod, value);
                Draw();
            }
        }

        private uint[] _gradient = new uint[] { 0x000000, 0x000000, 0xFF0000, 0x00FF00, 0x0000FF };
        public uint[] Gradient
        {
            get { return _gradient; }
            set { 
                SetValue(ref _gradient, value);
            }
        }

        public MainViewModel(IntPtr instanceHandle, IntPtr surfaceHandle)
        {
            _renderer = new MandelbrotRenderer(instanceHandle, surfaceHandle);
            (uint width, uint height) = _renderer.GetSurfaceExtent();
            _surfaceWidth = (int)width;
            _surfaceHeight = (int)height;
            UpdateBorders();
        }

        public void RefreshSurface()
        {
            _renderer.RefreshSurface();
            (uint width, uint height) = _renderer.GetSurfaceExtent();
            _surfaceWidth = (int)width;
            _surfaceHeight = (int)height;
            UpdateBorders();
        }

        public void Draw()
        {
            _renderer.Top = (float)this.Top;
            _renderer.Left = (float)this.Left;
            _renderer.Right = (float)this.Right;
            _renderer.Bottom = (float)this.Bottom;

            _renderer.BailoutRadius = 256;
            _renderer.MaxIterations = 5000;


            double periodPercent = _gradientPeriod / 100.0;
            _renderer.GradientPeriodFactor = (float)periodPercent;

            // The first color listed on the UI will be the fill color.
            // The subsequent colors are the true gradient.
            if (_gradient.Length < 3)
            {
                uint[] newGradient = new uint[3] { 0x000000, 0x000000, 0xFFFFFF };

                for (int i = 0; i < _gradient.Length; i++)
                    newGradient[i] = _gradient[i];

                this.Gradient = newGradient;
            }

            _renderer.FillColor = _gradient[0];
            _renderer.Gradient = _gradient.Skip(1).ToArray();
            
            _renderer.Draw();
        }

        public void ZoomToPixel(int x, int y, int zoomDelta)
        {
            double r = this.Left + (this.Right - this.Left) * x / _surfaceWidth;
            double i = this.Top + (this.Bottom - this.Top) * y / _surfaceHeight;

            _zoomLevel += zoomDelta;
            _centerX = r;
            _centerY = i;

            UpdateBorders();
            PanPixels(x - _surfaceWidth / 2, y - _surfaceHeight / 2);
        }

        public void PanPixels(int deltaX, int deltaY)
        {
            double deltaR = (this.Right - this.Left) * deltaX / _surfaceWidth;
            double deltaI = (this.Bottom - this.Top) * deltaY / _surfaceHeight;

            _left -= deltaR;
            _right -= deltaR;
            _centerX -= deltaR;

            _top -= deltaI;
            _bottom -= deltaI;
            _centerY -= deltaI;
        }

        private void UpdateBorders()
        {
            _surfaceWidth = Math.Max(_surfaceWidth, 1);
            _surfaceHeight = Math.Max(_surfaceHeight, 1);

            double baseHeight = 4.0;
            double baseWidth = 4.0 * this.SurfaceWidth / this.SurfaceHeight;
            double zoomedHeight = baseHeight * Math.Pow(ZOOM_FACTOR, ZoomLevel);
            double zoomedWidth = baseWidth * Math.Pow(ZOOM_FACTOR, ZoomLevel);

            this.Left = this.CenterX - 0.5 * zoomedWidth;
            this.Right = this.CenterX + 0.5 * zoomedWidth;
            this.Top = this.CenterY + 0.5 * zoomedHeight;
            this.Bottom = this.CenterY - 0.5 * zoomedHeight;
        }
    }
}
